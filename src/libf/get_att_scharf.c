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
#define nfmpi_get_att_schar_ NFMPI_GET_ATT_SCHAR
#elif defined(F77_NAME_LOWER_2USCORE)
#define nfmpi_get_att_schar_ nfmpi_get_att_schar__
#elif !defined(F77_NAME_LOWER_USCORE)
#define nfmpi_get_att_schar_ nfmpi_get_att_schar
/* Else leave name alone */
#endif


/* Prototypes for the Fortran interfaces */
#include "mpifnetcdf.h"
FORTRAN_API void FORT_CALL nfmpi_get_att_schar_ ( int *v1, int *v2, char *v3 FORT_MIXED_LEN(d3), char *v4 FORT_MIXED_LEN(d4), MPI_Fint *ierr FORT_END_LEN(d3) FORT_END_LEN(d4) ){
    *ierr = ncmpi_get_att_schar( *v1, *v2, v3, v4 );
}
