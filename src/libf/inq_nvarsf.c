/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * This file is automatically generated by buildiface -infile=../lib/pnetcdf.h -deffile=defs
 * DO NOT EDIT
 */
#include "mpinetcdf_impl.h"


#ifdef F77_NAME_UPPER
#define nfmpi_inq_nvars_ NFMPI_INQ_NVARS
#elif defined(F77_NAME_LOWER_2USCORE)
#define nfmpi_inq_nvars_ nfmpi_inq_nvars__
#elif !defined(F77_NAME_LOWER_USCORE)
#define nfmpi_inq_nvars_ nfmpi_inq_nvars
/* Else leave name alone */
#endif

FORTRAN_API void FORT_CALL nfmpi_inq_nvars_ ( int *v1, int*v2, MPI_Fint *ierr ){
    *ierr = ncmpi_inq_nvars( *v1, v2 );
}
