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
#define nfmpi_get_vara_double_all_ NFMPI_GET_VARA_DOUBLE_ALL
#elif defined(F77_NAME_LOWER_2USCORE)
#define nfmpi_get_vara_double_all_ nfmpi_get_vara_double_all__
#elif !defined(F77_NAME_LOWER_USCORE)
#define nfmpi_get_vara_double_all_ nfmpi_get_vara_double_all
/* Else leave name alone */
#endif

FORTRAN_API void FORT_CALL nfmpi_get_vara_double_all_ ( int *v1, int *v2, int v3[], int v4[], double*v5, MPI_Fint *ierr ){
    *ierr = ncmpi_get_vara_double_all( *v1, *v2, v3, v4, v5 );
}
