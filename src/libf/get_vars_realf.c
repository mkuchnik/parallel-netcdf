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
#define nfmpi_get_vars_real_ NFMPI_GET_VARS_REAL
#elif defined(F77_NAME_LOWER_2USCORE)
#define nfmpi_get_vars_real_ nfmpi_get_vars_real__
#elif !defined(F77_NAME_LOWER_USCORE)
#define nfmpi_get_vars_real_ nfmpi_get_vars_real
/* Else leave name alone */
#endif


/* Prototypes for the Fortran interfaces */
#include "mpifnetcdf.h"
FORTRAN_API void FORT_CALL nfmpi_get_vars_real_ ( int *v1, int *v2, int v3[], int v4[], int v5[], float*v6, MPI_Fint *ierr ){
    size_t *l3;
    size_t *l4;
    size_t *l5;

#ifdef HAVE_SIZET_LARGER_THAN_FINT
    { int ln = ncxVardim(*v1,*v2);
    if (ln > 0) {
        int li;
        l3 = (size_t *)malloc( ln * sizeof(size_t) );
        for (li=0; li<ln; li++) 
            l3[li] = v3[li];
    }
    else l3 = 0;
    }
#else 
    l3 = v3;
#endif

#ifdef HAVE_SIZET_LARGER_THAN_FINT
    { int ln = ncxVardim(*v1,*v2);
    if (ln > 0) {
        int li;
        l4 = (size_t *)malloc( ln * sizeof(size_t) );
        for (li=0; li<ln; li++) 
            l4[li] = v4[li];
    }
    else l4 = 0;
    }
#else 
    l4 = v4;
#endif

#ifdef HAVE_SIZET_LARGER_THAN_FINT
    { int ln = ncxVardim(*v1,*v2);
    if (ln > 0) {
        int li;
        l5 = (size_t *)malloc( ln * sizeof(size_t) );
        for (li=0; li<ln; li++) 
            l5[li] = v5[li];
    }
    else l5 = 0;
    }
#else 
    l5 = v5;
#endif
    *ierr = ncmpi_get_vars_float( *v1, *v2, l3, l4, l5, v6 );

#ifdef HAVE_SIZET_LARGER_THAN_FINT
    if (l3) { free(l3); }
#endif

#ifdef HAVE_SIZET_LARGER_THAN_FINT
    if (l4) { free(l4); }
#endif

#ifdef HAVE_SIZET_LARGER_THAN_FINT
    if (l5) { free(l5); }
#endif
}
