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
#define nfmpi_get_vara_int_ NFMPI_GET_VARA_INT
#elif defined(F77_NAME_LOWER_2USCORE)
#define nfmpi_get_vara_int_ nfmpi_get_vara_int__
#elif !defined(F77_NAME_LOWER_USCORE)
#define nfmpi_get_vara_int_ nfmpi_get_vara_int
/* Else leave name alone */
#endif


/* Prototypes for the Fortran interfaces */
#include "mpifnetcdf.h"
FORTRAN_API void FORT_CALL nfmpi_get_vara_int_ ( int *v1, int *v2, int v3[], int v4[], MPI_Fint *v5, MPI_Fint *ierr ){
    size_t *l3;
    size_t *l4;

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
    *ierr = ncmpi_get_vara_int( *v1, *v2, l3, l4, v5 );

#ifdef HAVE_SIZET_LARGER_THAN_FINT
    if (l3) { free(l3); }
#endif

#ifdef HAVE_SIZET_LARGER_THAN_FINT
    if (l4) { free(l4); }
#endif

    {char *p = v5;
        while (*p) p++;
        while ((p-v5) < d5) { *p++ = ' '; }
    }
}
