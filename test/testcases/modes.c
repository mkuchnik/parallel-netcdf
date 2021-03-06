/*********************************************************************
 *
 *  Copyright (C) 2014, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 *
 *********************************************************************/
/* $Id$ */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * This program tests if the correct error codes are returns given various
 * create/open modes.
 *
 * NC_EINVAL_CMODE should be returned when creating a file using
 * comde with both NC_64BIT_OFFSET & NC_64BIT_DATA flags set.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strcpy() */
#include <unistd.h> /* unlink(), access() */
#include <mpi.h>
#include <pnetcdf.h>

#include <testutils.h>

#define ERR {if(err!=NC_NOERR)printf("Error at line=%d: %s\n", __LINE__, ncmpi_strerror(err));}

#define EXPECT_ERR(err_no) \
    if (err != err_no) { \
        nerrs++; \
        printf("Error at line %d: expect error code %s but got %s\n", \
               __LINE__,nc_err_code_name(err_no),nc_err_code_name(err)); \
    }

#define EXPECT_ERR2(err_no1, err_no2) \
    if (err != err_no1 && err != err_no2) { \
        nerrs++; \
        printf("Error at line %d: expect error code %s but got %s\n", \
               __LINE__,nc_err_code_name(err_no1),nc_err_code_name(err)); \
    }

static
int check_modes(char *filename)
{
    int rank, err, nerrs=0;
    int ncid, cmode;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* delete the file and ignore error */
    unlink(filename);
    MPI_Barrier(MPI_COMM_WORLD);

    /* create a new file and test various cmodes ----------------------------*/
    cmode = NC_CLOBBER;

    /* It is illegal to use both NC_64BIT_OFFSET and NC_64BIT_DATA together */
    cmode |= NC_64BIT_OFFSET | NC_64BIT_DATA;

    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    EXPECT_ERR(NC_EINVAL_CMODE)

    /* The file should not be created */
    if (rank == 0) {
        if (access(filename, F_OK) == 0) {
            printf("Error at line %d: file (%s) should not be created\n",
                   __LINE__, filename);
            nerrs++;
            /* delete the file and ignore error */
            unlink(filename);
        }
        /* else : file does not exist */
    }
    MPI_Barrier(MPI_COMM_WORLD);

    /* Collectively opening a non-existing file for read, expect error code
     * NC_ENOENT on all processes */
    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    /* older version of OpenMPI and MPICH may return MPI_ERR_IO instead of
     * MPI_ERR_NO_SUCH_FILE */
    EXPECT_ERR2(NC_ENOENT, NC_EFILE)

    /* The file should not be created */
    if (rank == 0) {
        if (access(filename, F_OK) == 0) {
            printf("Error at line %d: file (%s) should not be created\n",
                   __LINE__, filename);
            nerrs++;
            /* delete the file and ignore error */
            unlink(filename);
        }
        /* else : file does not exist */
    }
    MPI_Barrier(MPI_COMM_WORLD);

    /* Collectively opening a non-existing file for write, expect error code
     * NC_ENOENT on all processes */
    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_WRITE, MPI_INFO_NULL, &ncid);
    /* older version of OpenMPI and MPICH may return MPI_ERR_IO instead of
     * MPI_ERR_NO_SUCH_FILE */
    EXPECT_ERR2(NC_ENOENT, NC_EFILE)

    /* The file should not be created */
    if (rank == 0) {
        if (access(filename, F_OK) == 0) {
            printf("Error at line %d: file (%s) should not be created\n",
                   __LINE__, filename);
            nerrs++;
            /* delete the file and ignore error */
            unlink(filename);
        }
        /* else : file does not exist */
    }
    MPI_Barrier(MPI_COMM_WORLD);

    return nerrs;
}

int main(int argc, char** argv)
{
    char filename[256];
    int rank, err, nerrs=0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc > 2) {
        if (!rank) printf("Usage: %s [filename]\n",argv[0]);
        MPI_Finalize();
        return 0;
    }
    if (argc == 2) snprintf(filename, 256, "%s", argv[1]);
    else           strcpy(filename, "testfile.nc");
    MPI_Bcast(filename, 256, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        char *cmd_str = (char*)malloc(strlen(argv[0]) + 256);
        sprintf(cmd_str, "*** TESTING C   %s for file create/open modes ", argv[0]);
        printf("%-66s ------ ", cmd_str); fflush(stdout);
        free(cmd_str);
    }

    /* test under safe mode enabled */
    setenv("PNETCDF_SAFE_MODE", "1", 1);
    nerrs += check_modes(filename);

    /* test under safe mode disabled */
    setenv("PNETCDF_SAFE_MODE", "0", 1);
    nerrs += check_modes(filename);

    /* check if PnetCDF freed all internal malloc */
    MPI_Offset malloc_size, sum_size;
    err = ncmpi_inq_malloc_size(&malloc_size);
    if (err == NC_NOERR) {
        MPI_Reduce(&malloc_size, &sum_size, 1, MPI_OFFSET, MPI_SUM, 0, MPI_COMM_WORLD);
        if (rank == 0 && sum_size > 0)
            printf("heap memory allocated by PnetCDF internally has %lld bytes yet to be freed\n",
                   sum_size);
    }

    MPI_Allreduce(MPI_IN_PLACE, &nerrs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (rank == 0) {
        if (nerrs) printf(FAIL_STR,nerrs);
        else       printf(PASS_STR);
    }

    MPI_Finalize();
    return 0;
}

