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
#define nfmpi_get_var_int1_ NFMPI_GET_VAR_INT1
#elif defined(F77_NAME_LOWER_2USCORE)
#define nfmpi_get_var_int1_ nfmpi_get_var_int1__
#elif !defined(F77_NAME_LOWER_USCORE)
#define nfmpi_get_var_int1_ nfmpi_get_var_int1
/* Else leave name alone */
#endif


/* Prototypes for the Fortran interfaces */
#include "mpifnetcdf.h"
FORTRAN_API int FORT_CALL nfmpi_get_var_int1_ ( int *v1, int *v2, char *v3 FORT_MIXED_LEN(d3) FORT_END_LEN(d3) ){
    int ierr;
    ierr = ncmpi_get_var_schar( *v1, *v2, v3 );
    return ierr;
}
