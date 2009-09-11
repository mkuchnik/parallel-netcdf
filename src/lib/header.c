/***********************************************************************
 *
 * This file is written by Northwestern University and Argonne National
 * Laboratory
 *
 ***********************************************************************/

#include "nc.h"

#include <mpi.h>
#ifdef HAVE_MPIO_H
#include <mpio.h>
#endif
#include <assert.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <stdio.h>

#include "ncx.h"

#ifndef MAX
#define MAX(mm,nn) (((mm) > (nn)) ? (mm) : (nn))
#endif
#ifndef MIN
#define MIN(mm,nn) (((mm) < (nn)) ? (mm) : (nn))
#endif

/* Prototypes for functions used only in this file */
static MPI_Offset hdr_len_NC_string(const NC_string *ncstrp, MPI_Offset sizeof_t);
static MPI_Offset hdr_len_NC_dim(const NC_dim *dimp, MPI_Offset sizeof_t);
static MPI_Offset hdr_len_NC_dimarray(const NC_dimarray *ncap, MPI_Offset sizeof_t);
static MPI_Offset hdr_len_NC_attr(const NC_attr *attrp, MPI_Offset sizeof_t);
static MPI_Offset hdr_len_NC_attrarray(const NC_attrarray *ncap, MPI_Offset sizeof_t);
static MPI_Offset hdr_len_NC_var(const NC_var *varp, MPI_Offset sizeof_off_t, MPI_Offset sizeof_t);
static MPI_Offset hdr_len_NC_vararray(const NC_vararray *ncap, MPI_Offset sizeof_off_t, MPI_Offset sizeof_t);
static int hdr_put_NCtype(bufferinfo *pbp, NCtype type);
static int hdr_put_nc_type(bufferinfo *pbp, const nc_type *typep);
static int hdr_put_NC_string(bufferinfo *pbp, const NC_string *ncstrp);
static int hdr_put_NC_attrV(bufferinfo *pbp, const NC_attr *attrp);
static int hdr_put_NC_dim(bufferinfo *pbp, const NC_dim *dimp);
static int hdr_put_NC_attr(bufferinfo *pbp, const NC_attr *attrp);
static int hdr_put_NC_var(bufferinfo *pbp, const NC_var *varp);
static int hdr_put_NC_dimarray(bufferinfo *pbp, const NC_dimarray *ncap);
static int hdr_put_NC_attrarray(bufferinfo *pbp, const NC_attrarray *ncap);
static int hdr_put_NC_vararray(bufferinfo *pbp, const NC_vararray *ncap);
static int hdr_fetch(bufferinfo *gbp);
static int hdr_check_buffer(bufferinfo *gbp, MPI_Offset nextread);
static int hdr_get_NCtype(bufferinfo *gbp, NCtype *typep);
static int hdr_get_size_t(bufferinfo *gbp, MPI_Offset *sp);
static int hdr_get_NC_string(bufferinfo *gbp, NC_string **ncstrpp);
static int hdr_get_NC_dim(bufferinfo *gbp, NC_dim **dimpp);
static int hdr_get_NC_dimarray(bufferinfo *gbp, NC_dimarray *ncap);
static int hdr_get_nc_type(bufferinfo *gbp, nc_type *typep);
static int hdr_get_NC_attrV(bufferinfo *gbp, NC_attr *attrp);
static int hdr_get_NC_attr(bufferinfo *gbp, NC_attr **attrpp);
static int hdr_get_NC_attrarray(bufferinfo *gbp, NC_attrarray *ncap);
static int hdr_get_NC_var(bufferinfo *gbp, NC_var **varpp);
static int hdr_get_NC_vararray(bufferinfo *gbp, NC_vararray *ncap);

/*
 * "magic number" at beginning of file: 0x43444601 (big endian) 
 */
static const schar ncmagic[] = {'C', 'D', 'F', 0x02}; 
static const schar ncmagic1[] = {'C', 'D', 'F', 0x01}; 
static const schar ncmagic2[] = {'C', 'D', 'F', 0x05}; 

/*
 * Recompute the shapes of all variables
 * Sets ncp->begin_var to start of first variable.
 * Sets ncp->begin_rec to start of first record variable.
 * Returns -1 on error. The only possible error is an reference
 * to a non existent dimension, which would occur for a corrupt
 * netcdf file.
 */ 
int
ncmpii_NC_computeshapes(NC *ncp)
{
        NC_var **vpp = (NC_var **)ncp->vars.value;
        NC_var *const *const end = &vpp[ncp->vars.nelems];
        NC_var *first_var = NULL;       /* first "non-record" var */
        NC_var *first_rec = NULL;       /* first "record" var */
        int status;
 
        ncp->begin_var = (MPI_Offset) ncp->xsz;
        ncp->begin_rec = (MPI_Offset) ncp->xsz;
        ncp->recsize = 0;
 
        if(ncp->vars.nelems == 0)
                return(0);




        for( /*NADA*/; vpp < end; vpp++)
        {	
                status = ncmpii_NC_var_shape64(*vpp, &ncp->dims);


                if(status != ENOERR)
                        return(status);
 
                if(IS_RECVAR(*vpp))
                {
                        if(first_rec == NULL)
                                first_rec = *vpp;
                        ncp->recsize += (*vpp)->len;
                }
                else if(first_var == NULL)
                {
                        first_var = *vpp;
                        /*
                         * Overwritten each time thru.
                         * Usually overwritten in first_rec != NULL clause.
                         */
                        ncp->begin_rec = (*vpp)->begin + (MPI_Offset)(*vpp)->len;
                }
        }
 
        if(first_rec != NULL)
        {
                assert(ncp->begin_rec <= first_rec->begin);
                ncp->begin_rec = first_rec->begin;
                /*
                 * for special case of exactly one record variable, pack value
                 */
                if(ncp->recsize == first_rec->len)
                        ncp->recsize = *first_rec->dsizes * first_rec->xsz;
        }
 
        if(first_var != NULL)
        {
                ncp->begin_var = first_var->begin;
        }
        else
        {
                ncp->begin_var = ncp->begin_rec;
        }

        assert(ncp->begin_var > 0);
        assert(ncp->xsz <= ncp->begin_var);
        assert(ncp->begin_rec > 0);
        assert(ncp->begin_var <= ncp->begin_rec);
 
        return(ENOERR);
} 

/* 
 * To compute how much space will the xdr'd header take 
 */
 
#define X_SIZEOF_NC_TYPE X_SIZEOF_INT
#define X_SIZEOF_NCTYPE X_SIZEOF_INT

static MPI_Offset
hdr_len_NC_string(const NC_string *ncstrp, MPI_Offset sizeof_t)
{
//      MPI_Offset sz = X_SIZEOF_SIZE_T; /* nchars */
        int sz = sizeof_t; /* nchars */
 
        assert(ncstrp != NULL);
 
        if(ncstrp->nchars != 0)
                sz += _RNDUP(ncstrp->nchars, X_ALIGN);
 
        return sz;
}
 
static MPI_Offset
hdr_len_NC_dim(const NC_dim *dimp, MPI_Offset sizeof_t)
{
        MPI_Offset sz;
 
        assert(dimp != NULL);
 
        sz = hdr_len_NC_string(dimp->name, sizeof_t);
//        sz += X_SIZEOF_SIZE_T;
        sz += sizeof_t;
 
        return(sz);
}
 
static MPI_Offset
hdr_len_NC_dimarray(const NC_dimarray *ncap, MPI_Offset sizeof_t)
{
        MPI_Offset xlen = X_SIZEOF_NCTYPE;  /* type */
//        xlen += X_SIZEOF_SIZE_T;        /* count */
        xlen += sizeof_t;        /* count */
        if(ncap == NULL)
                return xlen;
        /* else */
        {
                const NC_dim **dpp = (const NC_dim **)ncap->value;
                const NC_dim *const *const end = &dpp[ncap->nelems];
                for(  /*NADA*/; dpp < end; dpp++)
                {
                        xlen += hdr_len_NC_dim(*dpp, sizeof_t);
                }
        }
        return xlen;
} 

static MPI_Offset
hdr_len_NC_attr(const NC_attr *attrp, MPI_Offset sizeof_t)
{
        MPI_Offset sz;
 
        assert(attrp != NULL);
 
        sz = hdr_len_NC_string(attrp->name, sizeof_t);
        sz += X_SIZEOF_NC_TYPE; /* type */
//        sz += X_SIZEOF_SIZE_T; /* nelems */
        sz += sizeof_t; /* nelems */
        sz += attrp->xsz;
 
        return(sz);
}
 
static MPI_Offset
hdr_len_NC_attrarray(const NC_attrarray *ncap, MPI_Offset sizeof_t)
{
        MPI_Offset xlen = X_SIZEOF_NCTYPE;  /* type */
//        xlen += X_SIZEOF_SIZE_T;        /* count */
        xlen += sizeof_t;        /* count */
        if(ncap == NULL)
                return xlen;
        /* else */
        {
                const NC_attr **app = (const NC_attr **)ncap->value;
                const NC_attr *const *const end = &app[ncap->nelems];
                for( /*NADA*/; app < end; app++)
                {
                        xlen += hdr_len_NC_attr(*app, sizeof_t);
                }
        }
        return xlen;
}
 
static MPI_Offset
hdr_len_NC_var(const NC_var *varp, MPI_Offset sizeof_off_t, MPI_Offset sizeof_t)
{
        MPI_Offset sz;
 
        assert(varp != NULL);
	assert(sizeof_off_t == 4 || sizeof_off_t == 8);
 
        sz = hdr_len_NC_string(varp->name,sizeof_t);
//      sz += X_SIZEOF_SIZE_T; /* ndims */
        sz += sizeof_t; /* ndims */
        if (sizeof_t == 8)
		sz += ncmpix_len_long(varp->ndims); /* dimids */
        else
		sz += ncmpix_len_int(varp->ndims); /* dimids */
        sz += hdr_len_NC_attrarray(&varp->attrs, sizeof_t);
        sz += X_SIZEOF_NC_TYPE; /* type */
//      sz += X_SIZEOF_SIZE_T; /* len */
        sz += sizeof_t; /* len */
        sz += sizeof_off_t; /* begin */
 
        return(sz);
} 

static MPI_Offset
hdr_len_NC_vararray(const NC_vararray *ncap, MPI_Offset sizeof_off_t, MPI_Offset sizeof_t)
{
        MPI_Offset xlen = X_SIZEOF_NCTYPE;  /* type */
//      xlen += X_SIZEOF_SIZE_T;        /* count */
        xlen += sizeof_t;           /* count */
        if(ncap == NULL)
                return xlen;
        /* else */
        {
                const NC_var **vpp = (const NC_var **)ncap->value;
                const NC_var *const *const end = &vpp[ncap->nelems];
                for( /*NADA*/; vpp < end; vpp++)
                {
                        xlen += hdr_len_NC_var(*vpp, sizeof_off_t, sizeof_t);
                }
        }
        return xlen;
}
 
MPI_Offset
ncmpii_hdr_len_NC(const NC *ncp, MPI_Offset sizeof_off_t)
{
        MPI_Offset xlen = sizeof(ncmagic);
 
        assert(ncp != NULL);
	if (fIsSet(ncp->flags, NC_64BIT_DATA)) {
	        xlen += X_SIZEOF_LONG; /* numrecs */
                xlen += hdr_len_NC_dimarray(&ncp->dims, 8);/* int-> long????????*/
                xlen += hdr_len_NC_attrarray(&ncp->attrs, 8);
                xlen += hdr_len_NC_vararray(&ncp->vars, sizeof_off_t, 8);
	} else {
	        xlen += X_SIZEOF_SIZE_T;
                xlen += hdr_len_NC_dimarray(&ncp->dims, 4);
                xlen += hdr_len_NC_attrarray(&ncp->attrs, 4);
                xlen += hdr_len_NC_vararray(&ncp->vars, sizeof_off_t, 4);
	}

//        xlen += hdr_len_NC_dimarray(&ncp->dims, sizeof_off_t);/* int-> long????????*/
 
        return xlen;
} 

/* Begin Of put NC */

static int
hdr_put_NCtype(bufferinfo *pbp, NCtype type) {
  int status;
  const int itype = (int)type;

  status = ncmpix_put_int_int(pbp->pos, &itype);
  if (status != ENOERR)
    return status;
  pbp->pos = (void *)((char *)pbp->pos + X_SIZEOF_INT); 
  return status;
}

static int 
hdr_put_nc_type(bufferinfo *pbp, const nc_type *typep) {
  int status;
  const int itype = (int) *typep;
  
  status =  ncmpix_put_int_int(pbp->pos, &itype);
  if (status != ENOERR)
    return status;
  pbp->pos = (void *)((char *)pbp->pos + X_SIZEOF_INT);
  
  return status; 
}

static int 
hdr_put_NC_string(bufferinfo *pbp, const NC_string *ncstrp) {
  int status;

  status = ncmpix_put_size_t(&pbp->pos, &ncstrp->nchars, pbp->version == 5 ? 8 : 4);
  if (status != ENOERR)
    return status;

  status = ncmpix_pad_putn_text(&pbp->pos, ncstrp->nchars, ncstrp->cp); 
  if (status != ENOERR)
    return status;

  return ENOERR;
}

/*
 * Put the values of an attribute 
 */
static int
hdr_put_NC_attrV(bufferinfo *pbp, const NC_attr *attrp) {
  void *value = attrp->xvalue;
  MPI_Offset padding, esz;

  esz = ncmpix_len_nctype(attrp->type);
  padding = attrp->xsz - esz * attrp->nelems;

  (void) memcpy(pbp->pos, value, esz * attrp->nelems);
  pbp->pos = (void *)((char *)pbp->pos + esz * attrp->nelems);
  (void) memset(pbp->pos, 0, padding);
  pbp->pos = (void *)((char *)pbp->pos + padding);
    
  return ENOERR;
}

static int 
hdr_put_NC_dim(bufferinfo *pbp, const NC_dim *dimp) {
  int status; 

  status = hdr_put_NC_string(pbp, dimp->name);
  if (status != ENOERR)
    return status;
 
  status = ncmpix_put_size_t(&pbp->pos, &dimp->size, pbp->version == 5 ? 8 : 4);
  if (status != ENOERR)
    return status;
  
  return ENOERR;
}

static int
hdr_put_NC_attr(bufferinfo *pbp, const NC_attr *attrp) {
  int status;

  status = hdr_put_NC_string(pbp, attrp->name);
  if (status != ENOERR)
    return status;

  status = hdr_put_nc_type(pbp, &attrp->type);
  if (status != ENOERR)
    return status;

  status = ncmpix_put_size_t(&pbp->pos, &attrp->nelems, pbp->version == 5 ? 8 : 4);
  if (status != ENOERR)
    return status;

  status = hdr_put_NC_attrV(pbp, attrp);
  if (status != ENOERR)
    return status;

  return ENOERR;
}

static int
hdr_put_NC_var(bufferinfo *pbp, const NC_var *varp) {
  int status;
  int i;
  MPI_Offset tmp_ndims;
  status = hdr_put_NC_string(pbp, varp->name);
  if (status != ENOERR)
    return status;

  tmp_ndims = varp->ndims;
  status = ncmpix_put_size_t(&pbp->pos, &tmp_ndims, pbp->version == 5 ? 8 : 4);
  if (status != ENOERR)
    return status;

  for (i=0; i< varp->ndims; i++){  
        const MPI_Offset dim_id  = (const MPI_Offset) varp->dimids[i];
 	status = ncmpix_put_size_t(&pbp->pos, &dim_id, pbp->version == 5 ? 8 : 4);
  }
/*
 if (pbp->version == 1)
  status = ncmpix_putn_int_int(&pbp->pos,
                        varp->ndims, varp->dimids);
  else
  status = ncmpix_putn_int_long(&pbp->pos,
                        varp->ndims, varp->dimids);
*/
  if (status != ENOERR)
    return status;


  for (i=0; i< varp->ndims; i++){
  }

  status = hdr_put_NC_attrarray(pbp, &varp->attrs);
  if (status != ENOERR)
    return status;

  status = hdr_put_nc_type(pbp, &varp->type); 
  if (status != ENOERR)
    return status;

  status = ncmpix_put_size_t(&pbp->pos, &varp->len, pbp->version == 5 ? 8 : 4); 
  if (status != ENOERR)
    return status;

  status = ncmpix_put_off_t(&pbp->pos, &varp->begin, pbp->version == 1 ? 4 : 8);
  if (status != ENOERR)
    return status;

  return ENOERR;
}

static int 
hdr_put_NC_dimarray(bufferinfo *pbp, const NC_dimarray *ncap) {
  int status;
  
  assert(pbp != NULL);
  
  if (ncap == NULL || ncap->nelems == 0) {
     /* ABSENT */
    const MPI_Offset nosz = 0;
    status = hdr_put_NCtype(pbp, NC_UNSPECIFIED);
    if (status != ENOERR)
      return status;

    status = ncmpix_put_size_t(&pbp->pos, &nosz, pbp->version == 5 ? 8 : 4);
    if (status != ENOERR)
      return status;
  } else {
    const NC_dim **dpp = (const NC_dim **)ncap->value; 
    const NC_dim *const *const end = &dpp[ncap->nelems]; 

    status = hdr_put_NCtype(pbp, NC_DIMENSION);
    if (status != ENOERR)
      return status;

    status = ncmpix_put_size_t(&pbp->pos, &ncap->nelems, pbp->version == 5 ? 8 : 4);
    if (status != ENOERR)
      return status;

    for ( /*NADA*/; dpp < end; dpp++) {
      status = hdr_put_NC_dim(pbp, *dpp);
      if (status != ENOERR)
        return status;
    }
  }

  return ENOERR;
}

static int
hdr_put_NC_attrarray(bufferinfo *pbp, const NC_attrarray *ncap) {
  int status;

  assert(pbp != NULL);
  if (ncap == NULL || ncap->nelems == 0) {
     /* ABSENT */
    const MPI_Offset nosz = 0;
    status = hdr_put_NCtype(pbp, NC_UNSPECIFIED);
    if (status != ENOERR)
      return status;

    status = ncmpix_put_size_t(&pbp->pos, &nosz, pbp->version == 5 ? 8 : 4);
    if (status != ENOERR)
      return status;
  } else {
    const NC_attr **app = (const NC_attr **)ncap->value;
    const NC_attr *const *const end = &app[ncap->nelems]; 

    status = hdr_put_NCtype(pbp, NC_ATTRIBUTE);
    if (status != ENOERR)
      return status;

    status = ncmpix_put_size_t(&pbp->pos, &ncap->nelems, pbp->version == 5 ? 8 : 4);
    if (status != ENOERR)
      return status;
  
    for ( /*NADA*/; app < end; app++) {
      status = hdr_put_NC_attr(pbp, *app);
      if (status != ENOERR)
        return status;
    }
  }

  return ENOERR;
}

static int
hdr_put_NC_vararray(bufferinfo *pbp, const NC_vararray *ncap){
  int status;

  assert(pbp != NULL);

  if (ncap == NULL || ncap->nelems == 0) {
     /* ABSENT */
    const MPI_Offset nosz = 0;
    status = hdr_put_NCtype(pbp, NC_UNSPECIFIED);
    if (status != ENOERR)
      return status;

    status = ncmpix_put_size_t(&pbp->pos, &nosz, pbp->version == 5 ? 8 : 4);
    if (status != ENOERR)
      return status;
  } else { 
    const NC_var **vpp = (const NC_var **)ncap->value; 
    const NC_var *const *const end = &vpp[ncap->nelems]; 

    status = hdr_put_NCtype(pbp, NC_VARIABLE);
    if (status != ENOERR)
      return status;

    status = ncmpix_put_size_t(&pbp->pos, &ncap->nelems, pbp->version == 5 ? 8 : 4);
    if (status != ENOERR)
      return status;

    for( /*NADA*/; vpp < end; vpp++) {
      status = hdr_put_NC_var(pbp, *vpp); 
      if (status != ENOERR)
        return status;
    }
  }

  return ENOERR;
}

int 
ncmpii_hdr_put_NC(NC *ncp, void *buf) {
  int status;
  bufferinfo putbuf;
  MPI_Offset nrecs=0; 

  putbuf.nciop = NULL;
  putbuf.offset = 0;
  putbuf.pos = putbuf.base = buf;
  putbuf.size = ncp->xsz;

  if (ncp->flags & NC_64BIT_DATA)
	  putbuf.version = 5;
  else if (ncp->flags & NC_64BIT_OFFSET)
	  putbuf.version = 2;
  else
	  putbuf.version = 1;

  if (putbuf.version == 5)
	  status = ncmpix_putn_schar_schar(&putbuf.pos, sizeof(ncmagic2), ncmagic2);
  else if (putbuf.version == 2)
	  status = ncmpix_putn_schar_schar(&putbuf.pos, sizeof(ncmagic), ncmagic);
  else
	  status = ncmpix_putn_schar_schar(&putbuf.pos, sizeof(ncmagic1), ncmagic1);

  if (status != ENOERR)
	  return status;

  nrecs = ncp->numrecs; 
  status = ncmpix_put_size_t(&putbuf.pos, &nrecs, putbuf.version == 5 ? 8 : 4);
  if (status != ENOERR)
    return status;

  assert((char *)putbuf.pos < (char *)putbuf.base + putbuf.size);

  status = hdr_put_NC_dimarray(&putbuf, &ncp->dims);
  if (status != ENOERR)
    return status;

  status = hdr_put_NC_attrarray(&putbuf, &ncp->attrs); 
  if (status != ENOERR)
    return status;

  status = hdr_put_NC_vararray(&putbuf, &ncp->vars);
  if (status != ENOERR)
    return status;

  return status;
}

/* End Of put NC */

/* Begin Of get NC */

/*
 * Fetch the next header chunk.  the chunk is 'gbp->size' bytes big
 * Takes care to not overwrite leftover (unused) data in the buffer before
 * fetching a new chunk: the current aproach is to re-read the extra data.  
 *
 * NOTE: An alternate approach (which we do not do) would be to save the old
 *       data, read the next chunk and then copy the old data into the new
 *       chunk.  This alternate aproach might help if it is important for reads
 *       to be aligned.
 */
static int
hdr_fetch(bufferinfo *gbp) {
  int rank;
  MPI_Comm comm;
  int mpireturn;
  MPI_Offset slack;		/* any leftover data in the buffer */

  assert(gbp->base != NULL);
  
  comm = gbp->nciop->comm;
  MPI_Comm_rank(comm, &rank);

  /* XXX: this pointer math might not be good on 64 bit platforms */
  slack = gbp->size - ((char *)gbp->pos - (char *)gbp->base);
  /* . if gbp->pos and gbp->base are the same, there is no leftover buffer data
   *   to worry about.  
   * In the other extreme, where gbp->size == (gbp->pos - gbp->base), then all
   * data in the buffer has been consumed */

  if (slack == gbp->size) slack = 0;

  (void) memset(gbp->base, 0, gbp->size);
  gbp->pos = gbp->base;
  gbp->index = 0;
  mpireturn = MPI_File_set_view(gbp->nciop->collective_fh, 0, MPI_BYTE, MPI_BYTE, 
		    "native", gbp->nciop->mpiinfo);
  if (mpireturn != MPI_SUCCESS) {
	ncmpii_handle_error(rank, mpireturn, "MPI_File_set_view");
        MPI_Finalize();
        return NC_EREAD;
  }

  if (rank == 0) {
    MPI_Status mpistatus;
    mpireturn = MPI_File_read_at(gbp->nciop->collective_fh, (gbp->offset)-slack, gbp->base, 
	             gbp->size, MPI_BYTE, &mpistatus);  
    if (mpireturn != MPI_SUCCESS) {
	ncmpii_handle_error(rank, mpireturn, "MPI_File_read_at");
        MPI_Finalize();
        return NC_EREAD;
    }

  }
  /* we might have had to backtrack */
  gbp->offset += (gbp->size - slack); 

  MPI_Bcast(gbp->base, gbp->size, MPI_BYTE, 0, comm);

  return ENOERR;
}

/*
 * Ensure that 'nextread' bytes are available.
 */
static int
hdr_check_buffer(bufferinfo *gbp, MPI_Offset nextread) {
  if ((char *)gbp->pos + nextread <= (char *)gbp->base + gbp->size)
    return ENOERR;
  return hdr_fetch(gbp);
}

static int
hdr_get_NCtype(bufferinfo *gbp, NCtype *typep) {
  int type = 0;
//  int sizeof_t = (gbp->version == 5) ? 8 : 4;
  int sizeof_t = 4;
  int status = hdr_check_buffer(gbp, sizeof_t);
  if (status != ENOERR)
    return status;

  status =  ncmpix_get_int_int(gbp->pos, &type);
  gbp->pos = (void *)((char *)gbp->pos + X_SIZEOF_INT);
  gbp->index += X_SIZEOF_INT;
  if (status != ENOERR)
    return status;
  *typep = (NCtype) type;
  return ENOERR;
}

static int
hdr_get_size_t(bufferinfo *gbp, MPI_Offset *sp) {
  MPI_Offset sizeof_t = (gbp->version == 5) ? 8 : 4;
  int status = hdr_check_buffer(gbp, sizeof_t);
  if (status != ENOERR)
    return status; 
  gbp->index += sizeof_t;
  return ncmpix_get_size_t((const void **)(&gbp->pos), sp, sizeof_t);
}

static int
hdr_get_NC_string(bufferinfo *gbp, NC_string **ncstrpp) {
  int status;
  MPI_Offset  nchars = 0, nbytes, padding, bufremain, strcount; 
  NC_string *ncstrp;
  char *cpos;
  char pad[X_ALIGN-1];

  status = hdr_get_size_t(gbp, &nchars);
  if (status != ENOERR)
    return status;

  ncstrp = ncmpii_new_NC_string(nchars, NULL);

  if (ncstrp == NULL)
    return NC_ENOMEM;

  nbytes = nchars * X_SIZEOF_CHAR;
  padding = _RNDUP(X_SIZEOF_CHAR * ncstrp->nchars, X_ALIGN)
            - X_SIZEOF_CHAR * ncstrp->nchars;
  bufremain = gbp->size - (MPI_Offset)((char *)gbp->pos - (char *)gbp->base);
  cpos = ncstrp->cp;

  while (nbytes > 0) {
    if (bufremain > 0) {
      strcount = MIN(bufremain, nbytes); 
      (void) memcpy(cpos, gbp->pos, strcount);
      nbytes -= strcount;
      gbp->pos = (void *)((char *)gbp->pos + strcount);
      gbp->index += strcount;
      cpos += strcount; 
      bufremain -= strcount;
    } else {
      status = hdr_fetch(gbp);
      if(status != ENOERR) {
        ncmpii_free_NC_string(ncstrp);
        return status;
      } 
      bufremain = gbp->size;
    }
  }

  if (padding > 0) {
    (void) memset(pad, 0, X_ALIGN-1);
    if (memcmp(gbp->pos, pad, padding) != 0) {
      ncmpii_free_NC_string(ncstrp);
      return EINVAL;
    }
    gbp->pos = (void *)((char *)gbp->pos + padding);
    gbp->index += padding;
  }
  
  *ncstrpp = ncstrp;
  
  return ENOERR;  
}

static int
hdr_get_NC_dim(bufferinfo *gbp, NC_dim **dimpp) {
  int status;
  NC_string *ncstrp;
  NC_dim *dimp;

  status = hdr_get_NC_string(gbp, &ncstrp);
  if (status != ENOERR)
    return status;

  dimp = ncmpii_new_x_NC_dim(ncstrp);
  if(dimp == NULL)
    return NC_ENOMEM;

  status = hdr_get_size_t(gbp, &dimp->size);
  if(status != ENOERR) {
    ncmpii_free_NC_dim(dimp); /* frees name */
    return status;
  }

  *dimpp = dimp;
  return ENOERR;
}

static int
hdr_get_NC_dimarray(bufferinfo *gbp, NC_dimarray *ncap) {
  int status;
  NCtype type = NC_UNSPECIFIED; 
  NC_dim **dpp, **end;

  assert(gbp != NULL && gbp->pos != NULL);
  assert(ncap != NULL);
  assert(ncap->value == NULL);

  status = hdr_get_NCtype(gbp, &type);
  if(status != ENOERR)
    return status; 

  status = hdr_get_size_t(gbp, &ncap->nelems);
  if(status != ENOERR)
    return status;
 

  if(ncap->nelems == 0) {
    if (type != NC_DIMENSION && type != NC_UNSPECIFIED)
      return EINVAL;
  } else {
    if(type != NC_DIMENSION)
      return EINVAL;

    ncap->value = (NC_dim **) malloc(ncap->nelems * sizeof(NC_dim *));
    if(ncap->value == NULL)
      return NC_ENOMEM;
    ncap->nalloc = ncap->nelems;

    dpp = ncap->value;
    end = &dpp[ncap->nelems];

    for( /*NADA*/; dpp < end; dpp++) {
      status = hdr_get_NC_dim(gbp, dpp);
      if (status != ENOERR) {
        ncap->nelems = dpp - ncap->value;
        ncmpii_free_NC_dimarrayV(ncap);
        return status;
      }
    }
  }

  return ENOERR;
}

static int
hdr_get_nc_type(bufferinfo *gbp, nc_type *typep) {
  int type = 0;
  int status = hdr_check_buffer(gbp, X_SIZEOF_INT);
  if(status != ENOERR)
    return status;

  status =  ncmpix_get_int_int(gbp->pos, &type);
  gbp->pos = (void *)((char *)gbp->pos + X_SIZEOF_INT); 
  gbp->index += X_SIZEOF_INT;
  if(status != ENOERR)
    return status;

  if (   type != NC_BYTE
      && type != NC_CHAR
      && type != NC_SHORT
      && type != NC_INT
      && type != NC_FLOAT
      && type != NC_DOUBLE) 
    return EINVAL; 
 
  *typep = (nc_type) type;

  return ENOERR;
}

MPI_Offset 
ncmpix_len_nctype(nc_type type) {
  switch(type) {
    case NC_BYTE:
    case NC_CHAR:
        return X_SIZEOF_CHAR;
    case NC_SHORT:
        return X_SIZEOF_SHORT;
    case NC_INT:
        return X_SIZEOF_INT;
    case NC_FLOAT:
        return X_SIZEOF_FLOAT;
    case NC_DOUBLE:
        return X_SIZEOF_DOUBLE;
    default: 
	assert("ncmpix_len_nctype bad type" == 0);
  }
  return 0;                
}

/*
 * Get the values of an attribute  
 */
static int
hdr_get_NC_attrV(bufferinfo *gbp, NC_attr *attrp) {
  int status;
  void *value = attrp->xvalue;
  char pad[X_ALIGN-1]; 
  MPI_Offset nbytes, esz, padding, bufremain, attcount;

  esz = ncmpix_len_nctype(attrp->type);
  padding = attrp->xsz - esz * attrp->nelems;
  bufremain = gbp->size - (MPI_Offset)((char *)gbp->pos - (char *)gbp->base);
  nbytes = esz * attrp->nelems;

  while (nbytes > 0) {
    if (bufremain > 0) {
      attcount = MIN(bufremain, nbytes);
      (void) memcpy(value, gbp->pos, attcount);
      nbytes -= attcount;
      gbp->pos = (void *)((char *)gbp->pos + attcount);
      gbp->index += attcount;
      value = (void *)((char *)value + attcount);
      bufremain -= attcount;
    } else {
      status = hdr_fetch(gbp);
      if(status != ENOERR) 
        return status;
      bufremain = gbp->size;
    }
  }
 
  if (padding > 0) {
    (void) memset(pad, 0, X_ALIGN-1);
    if (memcmp(gbp->pos, pad, padding) != 0) 
      return EINVAL;
    gbp->pos = (void *)((char *)gbp->pos + padding);
    gbp->index += padding;
  }

  return ENOERR;
}

static int
hdr_get_NC_attr(bufferinfo *gbp, NC_attr **attrpp) {
  NC_string *strp;
  int status;
  nc_type type; 
  MPI_Offset nelems=0;
  NC_attr *attrp;

  status = hdr_get_NC_string(gbp, &strp);
  if(status != ENOERR)
    return status;

  status = hdr_get_nc_type(gbp, &type);
  if(status != ENOERR) {
    ncmpii_free_NC_string(strp);
    return status;
  }

  status = hdr_get_size_t(gbp, &nelems); 
  if(status != ENOERR) {
    ncmpii_free_NC_string(strp);
    return status;
  }

  attrp = ncmpii_new_x_NC_attr(strp, type, nelems);
  if(attrp == NULL) {
    ncmpii_free_NC_string(strp);
    return status;
  }

  status = hdr_get_NC_attrV(gbp, attrp);
  if(status != ENOERR) {
    ncmpii_free_NC_attr(attrp); /* frees strp */ 
    return status;
  }

  *attrpp = attrp; 
  
  return ENOERR; 
}

static int
hdr_get_NC_attrarray(bufferinfo *gbp, NC_attrarray *ncap){
  int status;
  NCtype type = NC_UNSPECIFIED;
  NC_attr **app, **end;

  assert(gbp != NULL && gbp->pos != NULL);
  assert(ncap != NULL);
  assert(ncap->value == NULL);

  status = hdr_get_NCtype(gbp, &type);
  if(status != ENOERR)
    return status; 

  status = hdr_get_size_t(gbp, &ncap->nelems);
  if(status != ENOERR)
    return status;

  if(ncap->nelems == 0) {
    if (type != NC_ATTRIBUTE && type != NC_UNSPECIFIED)
      return EINVAL;
  } else {
    if(type != NC_ATTRIBUTE)
      return EINVAL;

    ncap->value = (NC_attr **) malloc(ncap->nelems * sizeof(NC_attr *));
    if(ncap->value == NULL)
      return NC_ENOMEM;
    ncap->nalloc = ncap->nelems; 

    app = ncap->value;
    end = &app[ncap->nelems];
    for( /*NADA*/; app < end; app++) {
      status = hdr_get_NC_attr(gbp, app);
      if (status != ENOERR) {
        ncap->nelems = app - ncap->value;
        ncmpii_free_NC_attrarrayV(ncap);
        return status;
      }
    }
  }
  
  return ENOERR;
}

static int
hdr_get_NC_var(bufferinfo *gbp, NC_var **varpp) {
  NC_string *strp;
  int status;
  MPI_Offset ndims=0, dim;
  MPI_Offset tmp_dimids=0;
  NC_var *varp;
  int i;

  status = hdr_get_NC_string(gbp, &strp);
  if(status != ENOERR)
    return status;
   
  status = hdr_get_size_t(gbp, &ndims);
  if(status != ENOERR) {
     ncmpii_free_NC_string(strp); 
     return status;
  }
 
  varp = ncmpii_new_x_NC_var(strp, ndims);
  if(varp == NULL) {
    ncmpii_free_NC_string(strp);
    return NC_ENOMEM;
  }

  for (dim = 0; dim < ndims; dim++ ) {
    status = hdr_check_buffer(gbp, (gbp->version == 5 ? 8 : 4));
    if(status != ENOERR) {
      ncmpii_free_NC_var(varp);
      return status;
    }
    status = hdr_get_size_t(gbp, &tmp_dimids);
    varp->dimids[dim] = (int)tmp_dimids;
    if(status != ENOERR) {
     return status;
    }

/*  
    if (gbp->version == 5) {
	status = ncmpix_getn_long_long((const void **)(&gbp->pos), 
                              1, (MPI_Offset*)varp->dimids + dim);
    } else {
	status = ncmpix_getn_int_int((const void **)(&gbp->pos), 
                              1, (int*) varp->dimids + dim);
    }

    if(status != ENOERR) {
      ncmpii_free_NC_var(varp);
      return status;
    }
*/
  }

  for (i=0; i< varp->ndims; i++){
  }

  status = hdr_get_NC_attrarray(gbp, &varp->attrs);
  if(status != ENOERR) {
    ncmpii_free_NC_var(varp);
    return status;
  }

  status = hdr_get_nc_type(gbp, &varp->type);
  if(status != ENOERR) {
    ncmpii_free_NC_var(varp);
    return status;
  } 

  status = hdr_get_size_t(gbp, &varp->len);
  if(status != ENOERR) {
    ncmpii_free_NC_var(varp);
    return status;
  }
  
  status = hdr_check_buffer(gbp, (gbp->version == 1 ? 4 : 8));
  if(status != ENOERR) {
    ncmpii_free_NC_var(varp);
    return status;
  }
  status = ncmpix_get_off_t((const void **)&gbp->pos,
                        &varp->begin, (gbp->version == 1 ? 4 : 8));
  if(status != ENOERR) {
    ncmpii_free_NC_var(varp);
    return status;
  }

  *varpp = varp;
  return ENOERR;
}

static int
hdr_get_NC_vararray(bufferinfo *gbp, NC_vararray *ncap) {
  int status;
  NCtype type = NC_UNSPECIFIED;
  NC_var **vpp, **end;
  int i;

  assert(gbp != NULL && gbp->pos != NULL);
  assert(ncap != NULL);
  assert(ncap->value == NULL); 

  status = hdr_get_NCtype(gbp, &type);
  if(status != ENOERR)
    return status;
 
  status = hdr_get_size_t(gbp, &ncap->nelems);
  if(status != ENOERR)
    return status;
 
  if(ncap->nelems == 0) {
    if (type != NC_VARIABLE && type != NC_UNSPECIFIED)
      return EINVAL;
  } else {
    if(type != NC_VARIABLE)
      return EINVAL;
    ncap->value = (NC_var **) malloc(ncap->nelems * sizeof(NC_var *));
    if(ncap->value == NULL)
      return NC_ENOMEM; 
    ncap->nalloc = ncap->nelems;
    vpp = ncap->value;
    end = &vpp[ncap->nelems];


    for( /*NADA*/; vpp < end; vpp++) {
      status = hdr_get_NC_var(gbp, vpp);
      if (status != ENOERR) {
        ncap->nelems = vpp - ncap->value;
        ncmpii_free_NC_vararrayV(ncap);
        return status;
      }
    }
  }

  return ENOERR;
}


int
ncmpii_hdr_get_NC(NC *ncp) {
  int status;
  bufferinfo getbuf;
  schar magic[sizeof(ncmagic)];
  MPI_Offset nrecs = 0;

  assert(ncp != NULL);

  /* Initialize the get buffer */

  getbuf.nciop = ncp->nciop;
  getbuf.offset = 0; 	/* read from start of the file */
  getbuf.size = _RNDUP( MAX(MIN_NC_XSZ, ncp->chunk), X_ALIGN );
  if (getbuf.size > 4096)
    getbuf.size = 4096;

  getbuf.pos = getbuf.base = (void *)malloc(getbuf.size);
  getbuf.index = 0;

  status = hdr_fetch(&getbuf);
  
  /* Get the header from get buffer */

  (void) memset(magic, 0, sizeof(magic));
  status = ncmpix_getn_schar_schar(
          (const void **)(&getbuf.pos), sizeof(magic), magic);
  getbuf.index += sizeof(magic);
  /* don't need to worry about CDF-1 or CDF-2 
   * 	if the first bits are not 'CDF-'  */
  if(memcmp(magic, ncmagic, sizeof(ncmagic)-1) != 0) {
    free(getbuf.base);
    return NC_ENOTNC;
  }
  /* check version number in last byte of magic */
  if (magic[sizeof(ncmagic)-1] == 0x1) {
	  getbuf.version = 1;
  } else if (magic[sizeof(ncmagic)-1] == 0x2) {
	  getbuf.version = 2;
	  fSet(ncp->flags, NC_64BIT_OFFSET);
	  if (sizeof(MPI_Offset) != 8) {
		  /* take the easy way out: if we can't support all CDF-2
		   * files, return immediately */
		  free(getbuf.base);
		  return NC_ESMALL;
	  }
  } else if (magic[sizeof(ncmagic)-1] == 0x5) {
	  getbuf.version = 5;
	  fSet(ncp->flags, NC_64BIT_DATA);
	  if (sizeof(MPI_Offset) != 8) {
		  free(getbuf.base);
		  return NC_ESMALL;
	  }
  } else {
	  free(getbuf.base);
	  return NC_ENOTNC;
  }
  

  status = hdr_check_buffer(&getbuf, (getbuf.version == 1) ? 4 : 8);
  if(status != ENOERR) {
    free(getbuf.base);
    return status;
  } 
  status = ncmpix_get_size_t((const void **)(&getbuf.pos), &nrecs, (getbuf.version == 5) ? 8 : 4);

  if (getbuf.version == 5) {
	getbuf.index += X_SIZEOF_LONG;
  } else {
	getbuf.index += X_SIZEOF_SIZE_T;
  }
  if(status != ENOERR) {
    free(getbuf.base);
    return status;
  }
  ncp->numrecs = nrecs;

  assert((char *)getbuf.pos < (char *)getbuf.base + getbuf.size);

  status = hdr_get_NC_dimarray(&getbuf, &ncp->dims);
  if(status != ENOERR) {
    free(getbuf.base);
    return status;
  }
	
 

  status = hdr_get_NC_attrarray(&getbuf, &ncp->attrs); 
  if(status != ENOERR) {
    free(getbuf.base);
    return status;
  }


  status = hdr_get_NC_vararray(&getbuf, &ncp->vars);
  if(status != ENOERR) {
    free(getbuf.base);
    return status; 
  }
 
  
  ncp->xsz = ncmpii_hdr_len_NC(ncp, (getbuf.version == 1) ? 4 : 8 );
  status = ncmpii_NC_computeshapes(ncp);
  free(getbuf.base);
  
  
  return status;
}

/* End Of get NC */

int ncmpii_comp_dims(NC_dimarray *nc_dim1, NC_dimarray *nc_dim2){
	int i;
	if (nc_dim1->nelems != nc_dim2->nelems){
           return NC_EDIMS_NELEMS_MULTIDEFINE;
	} else {	
           for (i=0; i<nc_dim1->nelems; i++){
	      if (nc_dim1->value[i]->size != nc_dim2->value[i]->size){
		return NC_EDIMS_SIZE_MULTIDEFINE;
	      } else {
        	   if ((nc_dim1->value[i]->name->nchars != nc_dim1->value[i]->name->nchars)||(strcmp(nc_dim1->value[i]->name->cp, nc_dim2->value[i]->name->cp)!=0)){
	              printf("Warning: The dimination name of NC definations on multiprocesses conflict. \nNOTE: Definitions across all processes must agree with one another\n");
		   } 
	     }
	   } 

        }
	return NC_NOERR;
}

int ncmpii_comp_attrs(NC_attrarray *nc_attr1, NC_attrarray *nc_attr2){
	int i;
	if (nc_attr1->nelems != nc_attr2->nelems){
           printf("Warning: The number of attributes of NC definations on multiprocesses conflict. \nNOTE: Definitions across all processes must agree with one another\n");
	} else {
		for (i=0; i<nc_attr1->nelems; i++){
		   if (nc_attr1->value[i]->xsz != nc_attr2->value[i]->xsz){
                       printf("Warning: The size of attribute of NC definations on multiprocesses conflict. \nNOTE: Definitions across all processes must agree with one another\n");
		   }
		   if ((nc_attr1->value[i]->name->nchars != nc_attr2->value[i]->name->nchars)||(strcmp(nc_attr1->value[i]->name->cp, nc_attr2->value[i]->name->cp))){
                       printf("Warning: The name of attribute of NC definations on multiprocesses conflict. \nNOTE: Definitions across all processes must agree with one another\n");
	           }
                   if (strcmp(nc_attr1->value[i]->xvalue, nc_attr2->value[i]->xvalue)){
                    printf("Warning: The value of attribute of NC definations on multiprocesses conflict. \nNOTE: Definitions across all processes must agree with one another\n");
		   }	
		   if (nc_attr1->value[i]->type != nc_attr2->value[i]->type){
                    printf("Warning: The type of attribute of NC definations on multiprocesses conflict. \nNOTE: Definitions across all processes must agree with one another\n");
		   }
		   if (nc_attr1->value[i]->nelems != nc_attr2->value[i]->nelems){
                    printf("Warning: The length of attribute of NC definations on multiprocesses conflict. \nNOTE: Definitions across all processes must agree with one another\n");
		   }
		}
	}	
    return NC_NOERR;
}

int ncmpii_comp_vars(NC_vararray *nc_var1, NC_vararray *nc_var2){
       int i, j;
	if (nc_var1->nelems != nc_var2->nelems){
           return NC_EVARS_NELEMS_MULTIDEFINE;
	} else {
		for (i=0; i<nc_var1->nelems; i++){
		   if ((nc_var1->value[i]->name->nchars !=  nc_var2->value[i]->name->nchars)||strcmp(nc_var1->value[i]->name->cp,nc_var2->value[i]->name->cp))
		   {
                    printf("Warning: The name of variable of NC definations on multiprocesses conflict. \nNOTE: Definitions across all processes must agree with one another\n");
		   }
		   if ((nc_var1->value[i]->ndims != nc_var2->value[i]->ndims)){
           		return NC_EVARS_NDIMS_MULTIDEFINE;
		   }
                   for (j=0; j<nc_var1->value[i]->ndims; j++){
		     if (nc_var1->value[i]->dimids[j] != nc_var2->value[i]->dimids[j]){
           		return NC_EVARS_DIMIDS_MULTIDEFINE;
		     }
		   }
		   if (nc_var1->value[i]->type != nc_var2->value[i]->type){
           		return NC_EVARS_TYPE_MULTIDEFINE;
		   }
		   if (nc_var1->value[i]->len != nc_var2->value[i]->len){
           		return NC_EVARS_LEN_MULTIDEFINE;
		   }
		   if (nc_var1->value[i]->begin != nc_var2->value[i]->begin){
           		return NC_EVARS_BEGIN_MULTIDEFINE;
                   }
		   ncmpii_comp_attrs(&nc_var1->value[i]->attrs, &nc_var2->value[i]->attrs);
		}
	}
      return NC_NOERR;
};

int
ncmpii_hdr_check_NC(bufferinfo *getbuf, NC *ncp) {
  int status;
  schar magic[sizeof(ncmagic)];
  NC *temp_ncp;
  MPI_Offset nrecs = 0;
  int rank;
  MPI_Offset chunksize = 4096;

  temp_ncp = ncmpii_new_NC(&chunksize);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  assert(ncp != NULL);
  (void) memset(magic, 0, sizeof(magic));
  status = ncmpix_getn_schar_schar(
          (const void **)&getbuf->pos, sizeof(magic), magic);
  getbuf->index += sizeof(magic);
  /* don't need to worry about CDF-1 or CDF-2
 *    *    if the first bits are not 'CDF-'  */
  if(memcmp(magic, ncmagic, sizeof(ncmagic)-1) != 0) {
    free(getbuf->base);
    return NC_ENOTNC;
  }
  /* check version number in last byte of magic */
  if (magic[sizeof(ncmagic)-1] == 0x1) {
          getbuf->version = 1;
  } else if (magic[sizeof(ncmagic)-1] == 0x2) {
          getbuf->version = 2;
          fSet(temp_ncp->flags, NC_64BIT_OFFSET);
          if (sizeof(MPI_Offset) != 8) {
                  /* take the easy way out: if we can't support all CDF-2
 *                    * files, return immediately */
                  free(getbuf->base);
                  return NC_ESMALL;
          }
  } else if (magic[sizeof(ncmagic)-1] == 0x5) {
          getbuf->version = 5;
          fSet(temp_ncp->flags, NC_64BIT_DATA);
          if (sizeof(MPI_Offset) != 8) {
                  free(getbuf->base);
                  return NC_ESMALL;
          }
  } else {
          free(getbuf->base);
          return NC_ENOTNC;
  }


  status = hdr_check_buffer(getbuf, (getbuf->version == 1) ? 4 : 8);
  if(status != ENOERR) {
    free(getbuf->base);
    return status;
  }

  status = ncmpix_get_size_t((const void **)(&getbuf->pos), &nrecs, (getbuf->version == 5) ? 8 : 4);

  if (getbuf->version == 5) {
        getbuf->index += X_SIZEOF_LONG;
  } else {
        getbuf->index += X_SIZEOF_SIZE_T;
  }
  if(status != ENOERR) {
    free(getbuf->base);
    return status;
  }
  temp_ncp->numrecs = nrecs;

  if (temp_ncp->numrecs != ncp->numrecs){
     return NC_ENUMRECS_MULTIDEFINE;
  }

  assert((char *)getbuf->pos < (char *)getbuf->base + getbuf->size);

  status = hdr_get_NC_dimarray(getbuf, &temp_ncp->dims);
  if(status != ENOERR) {
    free(getbuf->base);
    return status;
  }

  status = ncmpii_comp_dims(&temp_ncp->dims, &ncp->dims);
  if(status != ENOERR) {
    free(getbuf->base);
    return status;
  }

  status = hdr_get_NC_attrarray(getbuf, &temp_ncp->attrs);
  if(status != ENOERR) {
    free(getbuf->base);
    return status;
  }

  status = ncmpii_comp_attrs(&temp_ncp->attrs, &ncp->attrs);
  if(status != ENOERR) {
    free(getbuf->base);
    return status;
  }

  status = hdr_get_NC_vararray(getbuf, &temp_ncp->vars);
  if(status != ENOERR) {
    free(getbuf->base);
    return status;
  }

  status = ncmpii_comp_vars(&temp_ncp->vars, &ncp->vars);
  if(status != ENOERR) {
    free(getbuf->base);
    return status;
  }

  temp_ncp->xsz = ncmpii_hdr_len_NC(temp_ncp, (getbuf->version == 1) ? 4 : 8 );
  status = ncmpii_NC_computeshapes(temp_ncp);

  
  ncmpii_free_NC(temp_ncp);
  return status;
}

