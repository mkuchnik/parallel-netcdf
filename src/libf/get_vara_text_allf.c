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
#define nfmpi_get_vara_text_all_ NFMPI_GET_VARA_TEXT_ALL
#elif defined(F77_NAME_LOWER_2USCORE)
#define nfmpi_get_vara_text_all_ nfmpi_get_vara_text_all__
#elif !defined(F77_NAME_LOWER_USCORE)
#define nfmpi_get_vara_text_all_ nfmpi_get_vara_text_all
/* Else leave name alone */
#endif


/* Prototypes for the Fortran interfaces */
#include "mpifnetcdf.h"
FORTRAN_API void FORT_CALL nfmpi_get_vara_text_all_ ( int *v1, int *v2, int * v3[], int * v4[], char *v5 FORT_MIXED_LEN(d5), MPI_Fint *ierr FORT_END_LEN(d5) ){
    *ierr = ncmpi_get_vara_text_all( *v1, *v2, (const size_t *)(*v3), (const size_t *)(*v4), v5 );

    {char *p = v5;
        while (*p) p++;
        while ((p-v5) < d5) { *p++ = ' '; }
    }
}
