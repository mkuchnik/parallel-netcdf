/*********************************************************************************
 *
 * This file is written by Northwestern University and Argonne National Laboratory
 *
 ********************************************************************************/

#include <mpi.h>
#include <assert.h>
#include "nc.h"
#include "ncx.h"

#undef MAX  /* system may define MAX somewhere and complain */
#undef MIN  /* system may define MIN somewhere and complain */
#define MAX(mm,nn) (((mm) > (nn)) ? (mm) : (nn))
#define MIN(mm,nn) (((mm) < (nn)) ? (mm) : (nn))

/*
 * "magic number" at beginning of file: 0x43444601 (big endian) 
 */
static const schar ncmagic[] = {'C', 'D', 'F', 0x01}; 

typedef struct bufferinfo {
  ncio *nciop;		
  MPI_Offset offset;	/* current read/write offset in the file */
  void *base;     	/* beginning of read/write buffer */
  void *pos;      	/* current position in buffer */
  size_t size;		/* size of the buffer */
} bufferinfo;  

/*
 * Recompute the shapes of all variables
 * Sets ncp->begin_var to start of first variable.
 * Sets ncp->begin_rec to start of first record variable.
 * Returns -1 on error. The only possible error is an reference
 * to a non existent dimension, which would occur for a corrupt
 * netcdf file.
 */ 
int
NC_computeshapes(NC *ncp)
{
        NC_var **vpp = (NC_var **)ncp->vars.value;
        NC_var *const *const end = &vpp[ncp->vars.nelems];
        NC_var *first_var = NULL;       /* first "non-record" var */
        NC_var *first_rec = NULL;       /* first "record" var */
        int status;
 
        ncp->begin_var = (off_t) ncp->xsz;
        ncp->begin_rec = (off_t) ncp->xsz;
        ncp->recsize = 0;
 
        if(ncp->vars.nelems == 0)
                return(0);
 
        for( /*NADA*/; vpp < end; vpp++)
        {
                status = NC_var_shape(*vpp, &ncp->dims);
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
                        ncp->begin_rec = (*vpp)->begin + (off_t)(*vpp)->len;
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
        assert(ncp->xsz <= (size_t)ncp->begin_var);
        assert(ncp->begin_rec > 0);
        assert(ncp->begin_var <= ncp->begin_rec);
 
        return(ENOERR);
} 

/* 
 * To compute how much space will the xdr'd header take 
 */
 
#define X_SIZEOF_NC_TYPE X_SIZEOF_INT
#define X_SIZEOF_NCTYPE X_SIZEOF_INT

size_t
hdr_len_NC_string(const NC_string *ncstrp)
{
        size_t sz = X_SIZEOF_SIZE_T; /* nchars */
 
        assert(ncstrp != NULL);
 
        if(ncstrp->nchars != 0)
                sz += _RNDUP(ncstrp->nchars, X_ALIGN);
 
        return sz;
}
 
size_t
hdr_len_NC_dim(const NC_dim *dimp)
{
        size_t sz;
 
        assert(dimp != NULL);
 
        sz = hdr_len_NC_string(dimp->name);
        sz += X_SIZEOF_SIZE_T;
 
        return(sz);
}
 
size_t
hdr_len_NC_dimarray(const NC_dimarray *ncap)
{
        size_t xlen = X_SIZEOF_NCTYPE;  /* type */
        xlen += X_SIZEOF_SIZE_T;        /* count */
        if(ncap == NULL)
                return xlen;
        /* else */
        {
                const NC_dim **dpp = (const NC_dim **)ncap->value;
                const NC_dim *const *const end = &dpp[ncap->nelems];
                for(  /*NADA*/; dpp < end; dpp++)
                {
                        xlen += hdr_len_NC_dim(*dpp);
                }
        }
        return xlen;
} 

size_t
hdr_len_NC_attr(const NC_attr *attrp)
{
        size_t sz;
 
        assert(attrp != NULL);
 
        sz = hdr_len_NC_string(attrp->name);
        sz += X_SIZEOF_NC_TYPE; /* type */
        sz += X_SIZEOF_SIZE_T; /* nelems */
        sz += attrp->xsz;
 
        return(sz);
}
 
size_t
hdr_len_NC_attrarray(const NC_attrarray *ncap)
{
        size_t xlen = X_SIZEOF_NCTYPE;  /* type */
        xlen += X_SIZEOF_SIZE_T;        /* count */
        if(ncap == NULL)
                return xlen;
        /* else */
        {
                const NC_attr **app = (const NC_attr **)ncap->value;
                const NC_attr *const *const end = &app[ncap->nelems];
                for( /*NADA*/; app < end; app++)
                {
                        xlen += hdr_len_NC_attr(*app);
                }
        }
        return xlen;
}
 
size_t
hdr_len_NC_var(const NC_var *varp)
{
        size_t sz;
 
        assert(varp != NULL);
 
        sz = hdr_len_NC_string(varp->name);
        sz += X_SIZEOF_SIZE_T; /* ndims */
        sz += ncx_len_int(varp->ndims); /* dimids */
        sz += hdr_len_NC_attrarray(&varp->attrs);
        sz += X_SIZEOF_NC_TYPE; /* type */
        sz += X_SIZEOF_SIZE_T; /* len */
        sz += X_SIZEOF_OFF_T; /* begin */
 
        return(sz);
} 

size_t
hdr_len_NC_vararray(const NC_vararray *ncap)
{
        size_t xlen = X_SIZEOF_NCTYPE;  /* type */
        xlen += X_SIZEOF_SIZE_T;        /* count */
        if(ncap == NULL)
                return xlen;
        /* else */
        {
                const NC_var **vpp = (const NC_var **)ncap->value;
                const NC_var *const *const end = &vpp[ncap->nelems];
                for( /*NADA*/; vpp < end; vpp++)
                {
                        xlen += hdr_len_NC_var(*vpp);
                }
        }
        return xlen;
}
 
size_t
hdr_len_NC(const NC *ncp)
{
        size_t xlen = sizeof(ncmagic);
 
        assert(ncp != NULL);
 
        xlen += X_SIZEOF_SIZE_T; /* numrecs */
        xlen += hdr_len_NC_dimarray(&ncp->dims);
        xlen += hdr_len_NC_attrarray(&ncp->attrs);
        xlen += hdr_len_NC_vararray(&ncp->vars);
 
        return xlen;
} 

/* Begin Of put NC */

int
hdr_put_NCtype(bufferinfo *pbp, NCtype type) {
  int status;
  const int itype = (int)type;

  status = ncx_put_int_int(pbp->pos, &itype);
  if (status != ENOERR)
    return status;
  pbp->pos = (void *)((char *)pbp->pos + X_SIZEOF_INT); 
  return status;
}

int 
hdr_put_nc_type(bufferinfo *pbp, const nc_type *typep) {
  int status;
  const int itype = (int) *typep;
  
  status =  ncx_put_int_int(pbp->pos, &itype);
  if (status != ENOERR)
    return status;
  pbp->pos = (void *)((char *)pbp->pos + X_SIZEOF_INT);
  
  return status; 
}

int 
hdr_put_NC_string(bufferinfo *pbp, const NC_string *ncstrp) {
  int status;

  status = ncx_put_size_t(&pbp->pos, &ncstrp->nchars);
  if (status != ENOERR)
    return status;

  status = ncx_pad_putn_text(&pbp->pos, ncstrp->nchars, ncstrp->cp); 
  if (status != ENOERR)
    return status;

  return ENOERR;
}

/*
 * Put the values of an attribute 
 */
int
hdr_put_NC_attrV(bufferinfo *pbp, const NC_attr *attrp) {
  int status;
  void *value = attrp->xvalue;

  assert(pbp->size % XALIGN == 0);

  (void) memcpy(pbp->pos, value, attrp->xsz);
  pbp->pos = (void *)((char *)pbp->pos + attrp->xsz);
    
  return ENOERR;
}

int 
hdr_put_NC_dim(bufferinfo *pbp, const NC_dim *dimp) {
  int status; 

  status = hdr_put_NC_string(pbp, dimp->name);
  if (status != ENOERR)
    return status;

  status = ncx_put_size_t(&pbp->pos, &dimp->size);
  if (status != ENOERR)
    return status;
  
  return ENOERR;
}

int
hdr_put_NC_attr(bufferinfo *pbp, const NC_attr *attrp) {
  int status;

  status = hdr_put_NC_string(pbp, attrp->name);
  if (status != ENOERR)
    return status;

  status = hdr_put_nc_type(pbp, &attrp->type);
  if (status != ENOERR)
    return status;

  status = ncx_put_size_t(&pbp->pos, &attrp->nelems);
  if (status != ENOERR)
    return status;

  status = hdr_put_NC_attrV(pbp, attrp);
  if (status != ENOERR)
    return status;

  return ENOERR;
}

int
hdr_put_NC_var(bufferinfo *pbp, const NC_var *varp) {
  int status;

  status = hdr_put_NC_string(pbp, varp->name);
  if (status != ENOERR)
    return status;

  status = ncx_put_size_t(&pbp->pos, &varp->ndims);
  if (status != ENOERR)
    return status;

  status = ncx_putn_int_int(&pbp->pos,
                        varp->ndims, varp->dimids);
  if (status != ENOERR)
    return status;

  status = hdr_put_NC_attrarray(pbp, &varp->attrs);
  if (status != ENOERR)
    return status;

  status = hdr_put_nc_type(pbp, &varp->type); 
  if (status != ENOERR)
    return status;

  status = ncx_put_size_t(&pbp->pos, &varp->len); 
  if (status != ENOERR)
    return status;

  status = ncx_put_off_t(&pbp->pos, &varp->begin);
  if (status != ENOERR)
    return status;

  return ENOERR;
}

int 
hdr_put_NC_dimarray(bufferinfo *pbp, const NC_dimarray *ncap) {
  int status;
  
  assert(pbp != NULL);
  
  if (ncap == NULL || ncap->nelems == 0) {
     /* ABSENT */
    const size_t nosz = 0;
    status = hdr_put_NCtype(pbp, NC_UNSPECIFIED);
    if (status != ENOERR)
      return status;

    status = ncx_put_size_t(&pbp->pos, &nosz);
    if (status != ENOERR)
      return status;
  } else {
    const NC_dim **dpp = (const NC_dim **)ncap->value; 
    const NC_dim *const *const end = &dpp[ncap->nelems]; 

    status = hdr_put_NCtype(pbp, NC_DIMENSION);
    if (status != ENOERR)
      return status;

    status = ncx_put_size_t(&pbp->pos, &ncap->nelems);
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

int
hdr_put_NC_attrarray(bufferinfo *pbp, const NC_attrarray *ncap) {
  int status;

  assert(pbp != NULL);

  if (ncap == NULL || ncap->nelems == 0) {
     /* ABSENT */
    const size_t nosz = 0;
    status = hdr_put_NCtype(pbp, NC_UNSPECIFIED);
    if (status != ENOERR)
      return status;

    status = ncx_put_size_t(&pbp->pos, &nosz);
    if (status != ENOERR)
      return status;
  } else {
    const NC_attr **app = (const NC_attr **)ncap->value;
    const NC_attr *const *const end = &app[ncap->nelems]; 

    status = hdr_put_NCtype(pbp, NC_ATTRIBUTE);
    if (status != ENOERR)
      return status;

    status = ncx_put_size_t(&pbp->pos, &ncap->nelems);
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

int
hdr_put_NC_vararray(bufferinfo *pbp, const NC_vararray *ncap){
  int status;

  assert(pbp != NULL);

  if (ncap == NULL || ncap->nelems == 0) {
     /* ABSENT */
    const size_t nosz = 0;
    status = hdr_put_NCtype(pbp, NC_UNSPECIFIED);
    if (status != ENOERR)
      return status;

    status = ncx_put_size_t(&pbp->pos, &nosz);
    if (status != ENOERR)
      return status;
  } else { 
    const NC_var **vpp = (const NC_var **)ncap->value; 
    const NC_var *const *const end = &vpp[ncap->nelems]; 

    status = hdr_put_NCtype(pbp, NC_VARIABLE);
    if (status != ENOERR)
      return status;

    status = ncx_put_size_t(&pbp->pos, &ncap->nelems);
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
hdr_put_NC(NC *ncp, void *buf) {
  int status;
  bufferinfo putbuf;
  size_t nrecs; 

  putbuf.nciop = NULL;
  putbuf.offset = 0;
  putbuf.pos = putbuf.base = buf;
  putbuf.size = ncp->xsz;

  status = ncx_putn_schar_schar(&putbuf.pos, sizeof(ncmagic), ncmagic);

  nrecs = ncp->numrecs; 
  status = ncx_put_size_t(&putbuf.pos, &nrecs);
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
 * Fetch the next header chunk.
 */
int
hdr_fetch(bufferinfo *gbp) {
  int status;
  int rank;
  MPI_Comm comm;

  assert(gbp->base != NULL);
  
  comm = gbp->nciop->comm;
  MPI_Comm_rank(comm, &rank);

  (void) memset(gbp->base, 0, gbp->size);
  gbp->pos = gbp->base;
  MPI_File_set_view(gbp->nciop->collective_fh, 0, MPI_BYTE, MPI_BYTE, 
		    "native", gbp->nciop->mpiinfo);
  if (rank == 0) {
    MPI_Status mpistatus;
    MPI_File_read_at(gbp->nciop->collective_fh, gbp->offset, gbp->base, 
	             gbp->size, MPI_BYTE, &mpistatus);  
  }
  gbp->offset += gbp->size; 

  MPI_Bcast(gbp->base, gbp->size, MPI_BYTE, 0, comm);

  return ENOERR;
}

/*
 * Ensure that 'nextread' bytes are available.
 */
int
hdr_check_buffer(bufferinfo *gbp, size_t nextread) {
  if ((char *)gbp->pos + nextread <= (char *)gbp->base + gbp->size)
    return ENOERR;
  return hdr_fetch(gbp);
}

int
hdr_get_NCtype(bufferinfo *gbp, NCtype *typep) {
  int type = 0;
  int status = hdr_check_buffer(gbp, X_SIZEOF_INT);
  if (status != ENOERR)
    return status;

  status =  ncx_get_int_int(gbp->pos, &type);
  gbp->pos = (void *)((char *)gbp->pos + X_SIZEOF_INT);
  if (status != ENOERR)
    return status;
  *typep = (NCtype) type;
  return ENOERR;
}

int
hdr_get_size_t(bufferinfo *gbp, size_t *sp) {
  int status = hdr_check_buffer(gbp, X_SIZEOF_SIZE_T);
  if (status != ENOERR)
    return status; 
  return ncx_get_size_t((const void **)(&gbp->pos), sp);
}

int
hdr_get_NC_string(bufferinfo *gbp, NC_string **ncstrpp) {
  int status;
  size_t  nchars = 0, padding, bufremain, strcount; 
  NC_string *ncstrp;
  char *cpos;
  char pad[X_ALIGN-1];

  status = hdr_get_size_t(gbp, &nchars);
  if (status != ENOERR)
    return status;

  ncstrp = new_NC_string(nchars, NULL);
  if (ncstrp == NULL)
    return NC_ENOMEM;

  padding = _RNDUP(X_SIZEOF_CHAR * ncstrp->nchars, X_ALIGN)
            - X_SIZEOF_CHAR * ncstrp->nchars;
  bufremain = gbp->size - (size_t)((char *)gbp->pos - (char *)gbp->base);
  cpos = ncstrp->cp;

  while (nchars > 0) {
    if (bufremain > 0) {
      strcount = MIN(bufremain, X_SIZEOF_CHAR * nchars); 
      (void) memcpy(cpos, gbp->pos, strcount);
      nchars -= strcount/X_SIZEOF_CHAR;
      gbp->pos = (void *)((char *)gbp->pos + strcount);
      cpos += strcount; 
      bufremain -= strcount;
    } else {
      status = hdr_fetch(gbp);
      if(status != ENOERR) {
        free_NC_string(ncstrp);
        return status;
      } 
      bufremain = gbp->size;
    }
  }

  memset(pad, 0, X_ALIGN-1);
  if (memcmp(gbp->pos, pad, padding) != 0) {
    free_NC_string(ncstrp);
    return EINVAL;
  }
  gbp->pos = (void *)((char *)gbp->pos + padding);
  
  *ncstrpp = ncstrp;
  
  return ENOERR;  
}

int
hdr_get_NC_dim(bufferinfo *gbp, NC_dim **dimpp) {
  int status;
  NC_string *ncstrp;
  NC_dim *dimp;

  status = hdr_get_NC_string(gbp, &ncstrp);
  if (status != ENOERR)
    return status;

  dimp = new_x_NC_dim(ncstrp);
  if(dimp == NULL)
    return NC_ENOMEM;

  status = hdr_get_size_t(gbp, &dimp->size);
  if(status != ENOERR) {
    free_NC_dim(dimp); /* frees name */
    return status;
  }

  *dimpp = dimp;

  return ENOERR;
}

int
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
        free_NC_dimarrayV(ncap);
        return status;
      }
    }
  }

  return ENOERR;
}

int
hdr_get_nc_type(bufferinfo *gbp, nc_type *typep) {
  int type = 0;
  int status = hdr_check_buffer(gbp, X_SIZEOF_INT);
  if(status != ENOERR)
    return status;

  status =  ncx_get_int_int(gbp->pos, &type);
  gbp->pos = (void *)((char *)gbp->pos + X_SIZEOF_INT); 
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

size_t 
ncx_len_nctype(nc_type type) {
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
  }
  /* default */
  assert("ncx_len_nctype bad type" == 0);
  return 0;                
}

/*
 * Get the values of an attribute  
 */
int
hdr_get_NC_attrV(bufferinfo *gbp, NC_attr *attrp) {
  int status;
  void *value = attrp->xvalue;
  char pad[X_ALIGN-1]; 
  size_t nvalues = attrp->nelems, esz, padding, bufremain, attcount;

  esz = ncx_len_nctype(attrp->type);
  padding = attrp->xsz - esz * nvalues;
  bufremain = gbp->size - (size_t)((char *)gbp->pos - (char *)gbp->base);

  while (nvalues > 0) {
    if (bufremain > 0) {
      attcount = MIN(bufremain, esz * nvalues);
      (void) memcpy(value, gbp->pos, attcount);
      nvalues -= attcount/esz;
      gbp->pos = (void *)((char *)gbp->pos + attcount);
      value = (void *)((char *)value + attcount);
      bufremain -= attcount;
    } else {
      status = hdr_fetch(gbp);
      if(status != ENOERR) 
        return status;
      bufremain = gbp->size;
    }
  }
 
  memset(pad, 0, X_ALIGN-1);
  if (memcmp(gbp->pos, pad, padding) != 0) 
    return EINVAL;
  gbp->pos = (void *)((char *)gbp->pos + padding);

  return ENOERR;
}

int
hdr_get_NC_attr(bufferinfo *gbp, NC_attr **attrpp) {
  NC_string *strp;
  int status;
  nc_type type; 
  size_t nelems;
  NC_attr *attrp;

  status = hdr_get_NC_string(gbp, &strp);
  if(status != ENOERR)
    return status;

  status = hdr_get_nc_type(gbp, &type);
  if(status != ENOERR) {
    free_NC_string(strp);
    return status;
  }

  status = hdr_get_size_t(gbp, &nelems); 
  if(status != ENOERR) {
    free_NC_string(strp);
    return status;
  }

  attrp = new_x_NC_attr(strp, type, nelems);
  if(attrp == NULL) {
    free_NC_string(strp);
    return status;
  }

  status = hdr_get_NC_attrV(gbp, attrp);
  if(status != ENOERR) {
    free_NC_attr(attrp); /* frees strp */ 
    return status;
  }

  *attrpp = attrp; 
  
  return ENOERR; 
}

int
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
        free_NC_attrarrayV(ncap);
        return status;
      }
    }
  }
  
  return ENOERR;
}

int
hdr_get_NC_var(bufferinfo *gbp, NC_var **varpp) {
  NC_string *strp;
  int status;
  size_t ndims, dim;
  NC_var *varp;

  status = hdr_get_NC_string(gbp, &strp);
  if(status != ENOERR)
    return status;

  status = hdr_get_size_t(gbp, &ndims);
  if(status != ENOERR) {
     free_NC_string(strp); 
     return status;
  }

  varp = new_x_NC_var(strp, ndims);
  if(varp == NULL) {
    free_NC_string(strp);
    return NC_ENOMEM;
  }

  for (dim = 0; dim < ndims; dim++ ) {
    status = hdr_check_buffer(gbp, X_SIZEOF_INT);
    if(status != ENOERR) {
      free_NC_var(varp);
      return status;
    }
    status = ncx_getn_int_int((const void **)(&gbp->pos), 
                              1, varp->dimids + dim);
    if(status != ENOERR) {
      free_NC_var(varp);
      return status;
    }
  }

  status = hdr_get_NC_attrarray(gbp, &varp->attrs);
  if(status != ENOERR) {
    free_NC_var(varp);
    return status;
  }

  status = hdr_get_nc_type(gbp, &varp->type);
  if(status != ENOERR) {
    free_NC_var(varp);
    return status;
  } 

  status = hdr_get_size_t(gbp, &varp->len);
  if(status != ENOERR) {
    free_NC_var(varp);
    return status;
  }

  status = hdr_check_buffer(gbp, X_SIZEOF_OFF_T);
  if(status != ENOERR) {
    free_NC_var(varp);
    return status;
  }
  status = ncx_get_off_t((const void **)&gbp->pos,
                        &varp->begin);
  if(status != ENOERR) {
    free_NC_var(varp);
    return status;
  }

  *varpp = varp;
  return ENOERR;
}

int
hdr_get_NC_vararray(bufferinfo *gbp, NC_vararray *ncap) {
  int status;
  NCtype type = NC_UNSPECIFIED;
  NC_var **vpp, **end;

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
        free_NC_vararrayV(ncap);
        return status;
      }
    }
  }

  return ENOERR;
}

int
hdr_get_NC(NC *ncp) {
  int status;
  bufferinfo getbuf;
  schar magic[sizeof(ncmagic)];
  size_t nrecs = 0;

  assert(ncp != NULL);

  /* Initialize the get buffer */

  getbuf.nciop = ncp->nciop;
  getbuf.offset = 0; 	/* read from start of the file */
  getbuf.size = _RNDUP( MAX(MIN_NC_XSZ, ncp->chunk), X_ALIGN );
  if (getbuf.size > 4096)
    getbuf.size = 4096;
  getbuf.pos = getbuf.base = (void *)malloc(getbuf.size);

  status = hdr_fetch(&getbuf);
  
  /* Get the header from get buffer */

  (void) memset(magic, 0, sizeof(magic));
  status = ncx_getn_schar_schar(
          (const void **)(&getbuf.pos), sizeof(magic), magic);
  if(memcmp(magic, ncmagic, sizeof(ncmagic)) != 0) {
    free(getbuf.base);
    return NC_ENOTNC;
  }

  status = hdr_check_buffer(&getbuf, X_SIZEOF_SIZE_T);
  if(status != ENOERR) {
    free(getbuf.base);
    return status;
  } 
  status = ncx_get_size_t((const void **)(&getbuf.pos), &nrecs);
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

  ncp->xsz = hdr_len_NC(ncp); 
  status = NC_computeshapes(ncp);
  free(getbuf.base);

  return status;
}

/* End Of get NC */
