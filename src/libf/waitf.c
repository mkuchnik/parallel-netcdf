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
#define nfmpi_wait_ NFMPI_WAIT
#elif defined(F77_NAME_LOWER_2USCORE)
#define nfmpi_wait_ nfmpi_wait__
#elif !defined(F77_NAME_LOWER_USCORE)
#define nfmpi_wait_ nfmpi_wait
/* Else leave name alone */
#endif


/* Prototypes for the Fortran interfaces */
#include "mpifnetcdf.h"
FORTRAN_API int FORT_CALL nfmpi_wait_ ( NCMPI_Request*v1 ){
    int ierr;
    ierr = ncmpi_wait( v1 );
    return ierr;
}
