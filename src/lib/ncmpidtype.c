/*****************************************************************************
 *
 * This file is created by Northwestern University and Argonne National
 * Laboratory
 *
 ****************************************************************************/


#include <stdio.h>
#include <mpi.h>
#include "nc.h"

/* FIXME: working around MPICH2 enum-type design of COMBINER constants.

     All COPMBINERs means all values by MPI-2, including Fortran values.

     4 cases: (1) Using #define but not all COMBINERs are defined, like MPICH1  
	      (2) Using #define to give all COMINERs 
	      (3) Using enum-type to enumerate (all COMBINERs)
	      (4) None of the COMBINER constants is defined in either way

     If any second MPI implemenation also uses enum-type, fixme! And,
     if it doesn't have all COMBINERs enumerated, that's a real trouble,
     in which case, we need configure to check each of the COMBINERs.

     Indeed, if some MPI impl chooses to use enum-type, it should enumerate
     all values, otherwise, it's hard for C preprocessor to check
     whether a value is code-able or not, unless it has a "#if declared(x)"
     directive. 
*/
#define NCMPI_HAVEALL_MPICOMBINERS		\
  ( (MPI_VERSION>=2) || defined(MPICH2) )

/* FIXME: working around some MPI impl without all COMBINER constants.
     Keep it for now, and we may not need it in the future, or use "configure".
     Assume existing COMBINER constants won't coincide with following values.
*/
#if !NCMPI_HAVEALL_MPICOMBINERS 
#  ifndef MPI_COMBINER_NAMED
#  define MPI_COMBINER_NAMED		(-1001)
#  endif
#  ifndef MPI_COMBINER_DUP
#  define MPI_COMBINER_DUP		(-1002)
#  endif
#  ifndef MPI_COMBINER_CONTIGUOUS
#  define MPI_COMBINER_CONTIGUOUS	(-1003)
#  endif
#  ifndef MPI_COMBINER_VECTOR
#  define MPI_COMBINER_VECTOR		(-1004)
#  endif
#  ifndef MPI_COMBINER_HVECTOR_INTEGER
#  define MPI_COMBINER_HVECTOR_INTEGER	(-1005)
#  endif
#  ifndef MPI_COMBINER_HVECTOR
#  define MPI_COMBINER_HVECTOR		(-1006)
#  endif
#  ifndef MPI_COMBINER_INDEXED
#  define MPI_COMBINER_INDEXED		(-1007)
#  endif
#  ifndef MPI_COMBINER_HINDEXED_INTEGER
#  define MPI_COMBINER_HINDEXED_INTEGER	(-1008)
#  endif
#  ifndef MPI_COMBINER_HINDEXED
#  define MPI_COMBINER_HINDEXED		(-1009)
#  endif
#  ifndef MPI_COMBINER_INDEXED_BLOCK
#  define MPI_COMBINER_INDEXED_BLOCK	(-1010)
#  endif
#  ifndef MPI_COMBINER_STRUCT_INTEGER
#  define MPI_COMBINER_STRUCT_INTEGER	(-1011)
#  endif
#  ifndef MPI_COMBINER_STRUCT
#  define MPI_COMBINER_STRUCT		(-1012)
#  endif
#  ifndef MPI_COMBINER_SUBARRAY
#  define MPI_COMBINER_SUBARRAY		(-1013)
#  endif
#  ifndef MPI_COMBINER_DARRAY
#  define MPI_COMBINER_DARRAY		(-1014)
#  endif
#  ifndef MPI_COMBINER_F90_REAL
#  define MPI_COMBINER_F90_REAL		(-1015)
#  endif
#  ifndef MPI_COMBINER_F90_COMPLEX
#  define MPI_COMBINER_F90_COMPLEX	(-1016)
#  endif
#  ifndef MPI_COMBINER_F90_INTEGER
#  define MPI_COMBINER_F90_INTEGER	(-1017)
#  endif
#  ifndef MPI_COMBINER_RESIZED
#  define MPI_COMBINER_RESIZED		(-1018)
#  endif
#endif

/* Check for Fortran MPI Types to be supported */
/* FIXME: if some MPI implementation choose to use enum-type for 
	  MPI Predefined type constants, fixme!
     Assume existing MPI Type constants won't coincide with following values
*/
#define NCMPI_HAVEALL_FORTRAN_MPITYPES defined(MPICH2)
#if !NCMPI_HAVEALL_FORTRAN_MPITYPES
#  ifndef MPI_CHARACTER
#  define MPI_CHARACTER			(-1001)
#  endif
#  ifndef MPI_INTEGER1
#  define MPI_INTEGER1			(-1001)
#  endif
#  ifndef MPI_INTEGER2
#  define MPI_INTEGER2			(-1001)
#  endif
#  ifndef MPI_INTEGER4
#  define MPI_INTEGER4			(-1001)
#  endif
#  ifndef MPI_INTEGER
#  define MPI_INTEGER			(-1001)
#  endif
#  ifndef MPI_REAL
#  define MPI_REAL			(-1001)
#  endif
#  ifndef MPI_REAL4
#  define MPI_REAL4			(-1001)
#  endif
#  ifndef MPI_REAL8
#  define MPI_REAL8			(-1001)
#  endif
#  ifndef MPI_DOUBLE_PRECISION
#  define MPI_DOUBLE_PRECISION		(-1001)
#  endif
#endif

#define NCMPI_FORTRAN_TYPE_FILTER(ptype)				\
( 									\
  (ptype == MPI_CHARACTER) ? MPI_CHAR : (				\
  (ptype == MPI_INTEGER1) ? MPI_BYTE : (				\
  (ptype == MPI_INTEGER2) ? MPI_SHORT : (				\
  (ptype == MPI_INTEGER4 || ptype == MPI_INTEGER) ? MPI_INT : (		\
  (ptype == MPI_REAL || ptype == MPI_REAL4) ? MPI_FLOAT : (		\
  (ptype == MPI_REAL8 || ptype == MPI_DOUBLE_PRECISION) ? MPI_DOUBLE : 	\
   ptype )))))								\
)

/*@
  ncmpi_darray_get_totalblock - Count the total number of blocks assigned
  to the calling process by the darray datatype.

  Input:
. rank - rank of calling process
. ndims - number of dimensions for the array and process grid
. array_of_gsizes - Number of elements of type oldtype in each dimension
                    of global array (array of positive integers)
. array_of_distribs - Distribution of array in each dimension (array of state)
. array_of_dargs - Distribution argument in each dimension
                   (array of positive integers)
. array_of_psizes - Size of process grid in each dimension
                    (array of positive integers)

  Return:
. total number of blocks assigned from the distrubted array
@*/

int ncmpii_darray_get_totalblks(int rank,
				int ndims,
                                int array_of_gsizes[],
                                int array_of_distribs[],
                                int array_of_dargs[],
                                int array_of_psizes[])
{
  int total_blocks = 1;
  int subblocks, cycle, remain_cycle;
  int i;
  int pcoord;

  /* Process Grid ranking is always in C ORDER, 
     so compute proc coordinates from last dim */

  for (i=ndims-1; i>=0; i--) {
    if ( array_of_distribs[i] == MPI_DISTRIBUTE_NONE ) {
      total_blocks *= array_of_gsizes[i];
    } else {

      pcoord = rank % array_of_psizes[i];
      rank /= array_of_psizes[i];
      if ( array_of_dargs[i] == MPI_DISTRIBUTE_DFLT_DARG ) {
        subblocks = array_of_gsizes[i]/array_of_psizes[i];
	if ( subblocks*array_of_psizes[i]+pcoord < array_of_gsizes[i] )
	  subblocks++;
      } else {
	cycle = array_of_dargs[i]*array_of_psizes[i];
	remain_cycle = array_of_gsizes[i] % cycle;

	subblocks = remain_cycle - pcoord*array_of_dargs[i];
	if (subblocks > array_of_dargs[i])
	  subblocks = array_of_dargs[i];
	else if (subblocks < 0)
	  subblocks = 0;
	
	subblocks += array_of_gsizes[i]/cycle * array_of_dargs[i];
      }

      if (subblocks == 0)
        return 0;
      total_blocks *= subblocks;

    }
  }

  return total_blocks;
}


/*@
  ncmpii_dtype_decode - Decode the MPI derived datatype to get the bottom
  level primitive datatype information.

  Input:
. dtype - The datatype to be decoded (can be predefined type).

  Output:
. ptype - The bottom level primitive datatype (only one allowed)
. el_size - The size of the ptype
. nelems - Number of elements/entries of such ptype
. iscontig_of_ptypes - Whether dtype is a contiguous number of ptype
@*/

int ncmpii_dtype_decode(MPI_Datatype dtype, 
			MPI_Datatype *ptype, 
			int *el_size,
			int *nelems, 
			int *iscontig_of_ptypes)
{
  int i;
  int tmpnelems, tmpel_size, total_blocks;
  MPI_Datatype tmpptype;
  int num_ints, num_adds, num_dtypes, combiner;
  int *array_of_ints;
  MPI_Aint *array_of_adds;
  MPI_Datatype *array_of_dtypes;
  void *arraybuf;
  int memsz;
  int count;
  int ndims;
  int status = NC_NOERR;

  if (dtype == MPI_DATATYPE_NULL) {
    *nelems = 0;
    *ptype = dtype;
    *el_size = 0;
    *iscontig_of_ptypes = 1;
    return NC_NOERR;
  }

  MPI_Type_get_envelope(dtype, &num_ints, &num_adds, &num_dtypes, &combiner);

  if ( combiner == MPI_COMBINER_F90_INTEGER ||
       combiner == MPI_COMBINER_F90_REAL ||
       combiner == MPI_COMBINER_F90_COMPLEX )
  {
    fprintf(stderr, "FIXME: F90_INTEGER, F90_REAL or F90_COMPLEX are not supported.\n");
  }

  if ( combiner == MPI_COMBINER_NAMED ) {	
    /* Predefined datatype */
    *nelems = 1;
    *ptype = NCMPI_FORTRAN_TYPE_FILTER(dtype);
    MPI_Type_size(dtype, el_size);
    *iscontig_of_ptypes = 1;
    return NC_NOERR;
  }

  memsz = num_ints*sizeof(int)
	+ num_adds*sizeof(MPI_Aint)
	+ num_dtypes*sizeof(MPI_Datatype);
  arraybuf = (void *)malloc(memsz);
  array_of_ints = (int *)(arraybuf);
  array_of_adds = (MPI_Aint *)(array_of_ints + num_ints);
  array_of_dtypes = (MPI_Datatype *)(array_of_adds + num_adds);

  MPI_Type_get_contents(dtype, num_ints, num_adds, num_dtypes, 
			array_of_ints, array_of_adds, array_of_dtypes);

  switch (combiner) {

    /* single etype */

    case MPI_COMBINER_DUP:
    case MPI_COMBINER_CONTIGUOUS:
    case MPI_COMBINER_HVECTOR_INTEGER:
    case MPI_COMBINER_HVECTOR:
    case MPI_COMBINER_VECTOR:
    case MPI_COMBINER_INDEXED_BLOCK:
    case MPI_COMBINER_HINDEXED_INTEGER:
    case MPI_COMBINER_HINDEXED:
    case MPI_COMBINER_INDEXED:
    case MPI_COMBINER_SUBARRAY:
    case MPI_COMBINER_DARRAY:
    case MPI_COMBINER_RESIZED:

	status = ncmpii_dtype_decode(array_of_dtypes[0], ptype, el_size, 
				     nelems, iscontig_of_ptypes);
	if (*ptype != array_of_dtypes[0]) 
	  MPI_Type_free(array_of_dtypes);

	break;

    case MPI_COMBINER_STRUCT_INTEGER:
    case MPI_COMBINER_STRUCT:
	count = array_of_ints[0];
	*el_size = 0;
	for (i=0; i<count && *el_size==0; i++) {
	  /* need to skip null/marker types */
	  status = ncmpii_dtype_decode(array_of_dtypes[i], 
				       ptype,
				       el_size, 
				       nelems, 
				       iscontig_of_ptypes);
	  if (status != NC_NOERR)
	    return status;
	  if (*ptype != array_of_dtypes[i]) 
	    MPI_Type_free(array_of_dtypes+i);
	  if (*el_size > 0)
	    *nelems *= array_of_ints[1+i];
	}
	for ( ; i<count; i++) {
	  status = ncmpii_dtype_decode(array_of_dtypes[i], 
				       &tmpptype,
				       &tmpel_size, 
				       &tmpnelems, 
				       iscontig_of_ptypes);
	  if (status != NC_NOERR)
	    return status;
	  if (tmpptype != array_of_dtypes[i]) 
	    MPI_Type_free(array_of_dtypes+i);
	  if (tmpel_size > 0) {
	    if (tmpptype != *ptype)
	      return NC_EMULTITYPES;
	    *nelems += tmpnelems*array_of_ints[1+i];
          }
	}

	*iscontig_of_ptypes = 0;
	  
	break;

    default:
	break;
  }

  switch (combiner) {

    /* single etype */

    case MPI_COMBINER_CONTIGUOUS:
	total_blocks = array_of_ints[0];
	break;

    case MPI_COMBINER_HVECTOR_INTEGER:
    case MPI_COMBINER_HVECTOR:
    case MPI_COMBINER_VECTOR:
    case MPI_COMBINER_INDEXED_BLOCK:
	*iscontig_of_ptypes = 0;
	total_blocks = array_of_ints[0]*array_of_ints[1];
	break;

    case MPI_COMBINER_HINDEXED_INTEGER:
    case MPI_COMBINER_HINDEXED:
    case MPI_COMBINER_INDEXED:
	*iscontig_of_ptypes = 0;
	for (i=0, total_blocks=0; i<array_of_ints[0]; i++)
	  total_blocks += array_of_ints[1+i];
	break;

    case MPI_COMBINER_SUBARRAY:
	*iscontig_of_ptypes = 0;
	ndims = array_of_ints[0];
	for (i=0, total_blocks=1; i<ndims; i++)
	  total_blocks *= array_of_ints[1+ndims+i];
	break;

    case MPI_COMBINER_DARRAY:
	*iscontig_of_ptypes = 0;
	ndims = array_of_ints[2];

	/* seldom reached, so put it in a separate function */
	total_blocks = ncmpii_darray_get_totalblks(array_of_ints[1],
						   ndims,
						   array_of_ints+3,
						   array_of_ints+3+ndims,
						   array_of_ints+3+2*ndims,
						   array_of_ints+3+3*ndims);
	break;

    case MPI_COMBINER_RESIZED:
	*iscontig_of_ptypes = 0;
	total_blocks = 1;
	break;

    default: /* DUP etc. */
	total_blocks = 1;
	break;
  }

  *nelems *= total_blocks;

  if (memsz > 0)
    free(arraybuf);

  return status;
}

/**
 * ncmpii_get_ptype - only purpose is to extract out the single primitive
 * datatype used to construct the derived datatype
 *
 * (Deprecated, no use so far )
 */

int ncmpii_dtype_get_ptype(MPI_Datatype dtype, 
			   MPI_Datatype *ptype,
			   int *iscontig_of_ptypes) 
{

  int i;
  int num_ints, num_adds, num_dtypes, tmpnum_dtypes, combiner;
  int *array_of_ints;
  MPI_Aint *array_of_adds;
  MPI_Datatype *array_of_dtypes;
  void *arraybuf;
  int memsz, oldmemsz;
  int typesz;

  if (dtype == MPI_DATATYPE_NULL) {
    /* ERROR */
    *ptype = MPI_DATATYPE_NULL;
    *iscontig_of_ptypes = 1;
    return NC_NOERR;
  }

  oldmemsz = 0;

  *ptype = dtype;
  MPI_Type_get_envelope(*ptype, &num_ints, &num_adds, &num_dtypes, &combiner);
  *iscontig_of_ptypes = 1;

  while (combiner != MPI_COMBINER_NAMED) {

#   ifndef MPI_COMBINER_DUP
#   define MPI_COMBINER_DUP MPI_COMBINER_CONTIGUOUS
#   endif

    if (combiner != MPI_COMBINER_CONTIGUOUS && combiner != MPI_COMBINER_DUP)
      *iscontig_of_ptypes = 0;

    memsz = num_ints*sizeof(int) 
	  + num_adds*sizeof(MPI_Aint) 
	  + num_dtypes*sizeof(MPI_Datatype);
    if (oldmemsz < memsz) {
      if (oldmemsz > 0) free(arraybuf);
      arraybuf = (void *)malloc(memsz);
      oldmemsz = memsz;
    }
    array_of_ints = (int *)(arraybuf);
    array_of_adds = (MPI_Aint *)(array_of_ints + num_ints);
    array_of_dtypes = (MPI_Datatype *)(array_of_adds + num_adds);
    MPI_Type_get_contents(*ptype, num_ints, num_adds, num_dtypes,
                          array_of_ints, array_of_adds, array_of_dtypes);

    if (*ptype != dtype)
      MPI_Type_free(ptype);

    /* assuming there's only one bottom level primitive datatype */
    /* so, every path leads to that same final primitive datatype */
    /* TODO: do we need error checking for multiple primitive datatypes? */

    /* skip null/marker types */
    for (i=0, typesz=0; i<num_dtypes && typesz==0; i++) {
      *ptype = array_of_dtypes[i];
      if (*ptype != MPI_DATATYPE_NULL) {
        MPI_Type_size(*ptype, &typesz);
	if ( typesz == 0 ) {
          MPI_Type_get_envelope(*ptype,
	  		        &num_ints, &num_adds, &tmpnum_dtypes, 
			        &combiner);
          if ( combiner != MPI_COMBINER_NAMED)
            MPI_Type_free(array_of_dtypes+i);
        }
      }
    }

    while (i<num_dtypes) {
      if (array_of_dtypes[i] != MPI_DATATYPE_NULL) {
        MPI_Type_get_envelope(array_of_dtypes[i], 
	  		      &num_ints, &num_adds, &tmpnum_dtypes, 
			      &combiner);
        if (combiner != MPI_COMBINER_NAMED) 
          MPI_Type_free(array_of_dtypes+i);
      }
      i++;
    }

    if (typesz == 0) {
      /* something wrong, end in a NULL type */
      *ptype = MPI_DATATYPE_NULL;
      break;
    } else {
      MPI_Type_get_envelope(*ptype,
	  		    &num_ints, &num_adds, &num_dtypes, 
			    &combiner);
    }
  }

  if (oldmemsz > 0) free(arraybuf);

  return NC_NOERR;
}

/*@
  ncmpii_data_repack - copy data between two buffers with different datatypes.
  
  Input:
. inbuf - input buffer where data is copied from
. incount - number of input elements
. intype - datatype of each element in input buffer
. outbuf - output buffer where data is copied to
. outcount - number of output elements
. outtype - datatype of each element in output buffer  
@*/  

int ncmpii_data_repack(void *inbuf, 
		       int incount,
		       MPI_Datatype intype, 
		       void *outbuf,
		       int outcount,
		       MPI_Datatype outtype) 
{
  int intypesz, outtypesz;
  int packsz;
  void *packbuf;
  int packpos;

  MPI_Type_size(intype, &intypesz);
  MPI_Type_size(outtype, &outtypesz);

  if (incount*intypesz != outcount*outtypesz) {
    /* input data amount does not match output data amount */
    /* NOTE: we ignore it for user responsibility or add error handling ? */

    /* for rescue, guarantee output data amount <= input data amount */
    if (incount*intypesz < outcount*outtypesz)
      outcount = incount*intypesz/outtypesz;
  }

  if (incount == 0)
    return NC_NOERR;
  
  /* local pack-n-unpack, using MPI_COMM_SELF */
  MPI_Pack_size(incount, intype, MPI_COMM_SELF, &packsz);
  packbuf = (void *)malloc(packsz);
  packpos = 0;
  MPI_Pack(inbuf, incount, intype, packbuf, packsz, &packpos, MPI_COMM_SELF);
  packpos = 0;
  MPI_Unpack(packbuf, packsz, &packpos, outbuf, outcount, outtype, MPI_COMM_SELF);
  free(packbuf);

  return NC_NOERR;
}