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
#define nfmpi_inq_vartype_ NFMPI_INQ_VARTYPE
#elif defined(F77_NAME_LOWER_2USCORE)
#define nfmpi_inq_vartype_ nfmpi_inq_vartype__
#elif !defined(F77_NAME_LOWER_USCORE)
#define nfmpi_inq_vartype_ nfmpi_inq_vartype
/* Else leave name alone */
#endif


/* Prototypes for the Fortran interfaces */
#include "mpifnetcdf.h"
FORTRAN_API void FORT_CALL nfmpi_inq_vartype_ ( int *v1, int *v2, int *v3, MPI_Fint *ierr ){
    int l2 = *v2 - 1;
    *ierr = ncmpi_inq_vartype( *v1, l2, v3 );
}
