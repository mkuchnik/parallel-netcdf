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
#define nfmpi_get_vara_all_ NFMPI_GET_VARA_ALL
#elif defined(F77_NAME_LOWER_2USCORE)
#define nfmpi_get_vara_all_ nfmpi_get_vara_all__
#elif !defined(F77_NAME_LOWER_USCORE)
#define nfmpi_get_vara_all_ nfmpi_get_vara_all
/* Else leave name alone */
#endif


/* Prototypes for the Fortran interfaces */
#include "mpifnetcdf.h"
FORTRAN_API int FORT_CALL nfmpi_get_vara_all_ ( int *v1, int *v2, int v3[], int v4[], void*v5, int *v6, MPI_Fint *v7 ){
    int ierr;
    int l2 = *v2 - 1;
    size_t *l3 = 0;
    size_t *l4 = 0;

    { int ln = ncmpixVardim(*v1,*v2-1);
    if (ln > 0) {
        int li;
        l3 = (size_t *)malloc( ln * sizeof(size_t) );
        for (li=0; li<ln; li++) 
            l3[li] = v3[ln-1-li] - 1;
    }
    else if (ln < 0) {
        /* Error return */
        ierr = ln; 
	return ierr;
    }
    }

    { int ln = ncmpixVardim(*v1,*v2-1);
    if (ln > 0) {
        int li;
        l4 = (size_t *)malloc( ln * sizeof(size_t) );
        for (li=0; li<ln; li++) 
            l4[li] = v4[ln-1-li];
    }
    else if (ln < 0) {
        /* Error return */
        ierr = ln; 
	return ierr;
    }
    }
    ierr = ncmpi_get_vara_all( *v1, l2, l3, l4, v5, *v6, (MPI_Datatype)(*v7) );

    if (l3) { free(l3); }

    if (l4) { free(l4); }
    return ierr;
}
