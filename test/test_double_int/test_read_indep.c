/***********************************************************
 *
 * This test program reads a netCDF file, and then write it
 * out to another netCDF file, using the parallel netCDF 
 * library using MPI-IO. The two files should be identical. 
 *
 * Input File: "../data/test_double.nc"  generated from original netcdf-3.
 * Output File: "testread.nc"
 *
 *  The CDL notation for the test dataset is shown below:
 *
 *    netcdf test {
 *
 *       dimensions:
 *
 *            x = 100, y = 100, z = 100, time = NC_UNLIMITED;
 *
 *
 *       variables:  // variable types, names, shapes, attributes
 *
 *            double square(x, y);
 *                     squre: description = "2-D integer array";
 *
 *            double cube(x,y,z);
 *
 *            double time(time);  // coordinate & record variable
 *
 *            double xytime(time, x, y);  // record variable
 *
 *
 *      // global attributes
 *
 *           :title = "example netCDF dataset";
 *
 *
 *      data:  // data written for variables
 *          square  = 0, 1, 2, 3,  ... , 9999;
 *          cube    = 0, 1, 2, 3,  ... , 999999;
 *          time    = 0, 1, 2, 3,  ... , 99;    // 100 records
 *          xytime  = 0, 1, 2, 3,  ... , 9999;  // 100 records
 *   }
 *
 *
 *
 * This test uses non-collective APIs to read/write variable data and 
 * only deals with integer variables. 
 *
 * This test assume # of processors = 4
 *
 **********************************************************/

#include <stdio.h>
#include <mpi.h>
#include <netcdf.h>

void handle_error(int status) {
  printf("%s\n", nc_strerror(status));
}

main(int argc, char **argv) {

  int i, j;
  int status;
  int ncid1, ncid2;
  char *infname = "pvfs:../data/test_double.nc", *outfname = "pvfs:testread.nc";
  int ndims, nvars, ngatts, unlimdimid;
  char name[NC_MAX_NAME];
  nc_type type, vartypes[NC_MAX_VARS];
  size_t len, shape[NC_MAX_VAR_DIMS], varsize, numrecs, start[NC_MAX_VAR_DIMS];
  void *valuep;
  int dimids[NC_MAX_DIMS], varids[NC_MAX_VARS];
  int vardims[NC_MAX_VARS][NC_MAX_VAR_DIMS];
  int varndims[NC_MAX_VARS], varnatts[NC_MAX_VARS];
  int isRecvar;

  int rank;
  int nprocs;
  MPI_Comm comm = MPI_COMM_WORLD;
  

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

if (rank == 0)
  fprintf(stderr, "Testing read ... ");

  /**********  START OF NETCDF ACCESS **************/


  /* Read a netCDF file and write it out to another file */

  /**
   * Open the input dataset - ncid1:
   *   File name: "../data/test_double.nc"
   *   Dataset API: Collective
   * And create the output dataset - ncid2:
   *   File name: "testread.nc"
   *   Dataset API: Collective
   */

  status = ncmpi_open(comm, infname, 0, MPI_INFO_NULL, &ncid1);
  if (status != NC_NOERR) handle_error(status);

  status = ncmpi_create(comm, outfname, NC_CLOBBER, MPI_INFO_NULL, &ncid2);
  if (status != NC_NOERR) handle_error(status);


  /**
   * Inquire the dataset definitions of input dataset AND
   * Add dataset definitions for output dataset.
   */

  status = nc_inq(ncid1, &ndims, &nvars, &ngatts, &unlimdimid);
  if (status != NC_NOERR) handle_error(status);


  /* Inquire global attributes, assume CHAR attributes. */

  for (i = 0; i < ngatts; i++) {
    status = nc_inq_attname(ncid1, NC_GLOBAL, i, name);
    if (status != NC_NOERR) handle_error(status);
    status = nc_inq_att (ncid1, NC_GLOBAL, name, &type, &len);
    if (status != NC_NOERR) handle_error(status);
    switch (type) {
      case NC_CHAR: 
	valuep = (void *)malloc(len * sizeof(char));
	status = nc_get_att_text(ncid1, NC_GLOBAL, name, valuep);
	if (status != NC_NOERR) handle_error(status);
	status = nc_put_att_text (ncid2, NC_GLOBAL, name, len, (char *)valuep);
	if (status != NC_NOERR) handle_error(status);
	free(valuep);
        break;
      case NC_SHORT:
        valuep = (void *)malloc(len * sizeof(short));
        status = nc_get_att_short(ncid1, NC_GLOBAL, name, valuep);
        if (status != NC_NOERR) handle_error(status);
        status = nc_put_att_short (ncid2, NC_GLOBAL, name, type, len, (short *)valuep);
        if (status != NC_NOERR) handle_error(status);
        free(valuep);
        break;
      case NC_INT:
        valuep = (void *)malloc(len * sizeof(int));
        status = nc_get_att_int(ncid1, NC_GLOBAL, name, valuep);
        if (status != NC_NOERR) handle_error(status);
        status = nc_put_att_int (ncid2, NC_GLOBAL, name, type, len, (int *)valuep);
        if (status != NC_NOERR) handle_error(status);
        free(valuep);
        break;
      case NC_FLOAT:
        valuep = (void *)malloc(len * sizeof(float));
        status = nc_get_att_float(ncid1, NC_GLOBAL, name, valuep);
        if (status != NC_NOERR) handle_error(status);
        status = nc_put_att_float (ncid2, NC_GLOBAL, name, type, len, (float *)valuep);
        if (status != NC_NOERR) handle_error(status);
        free(valuep);
        break;
      case NC_DOUBLE:
        valuep = (void *)malloc(len * sizeof(double));
        status = nc_get_att_double(ncid1, NC_GLOBAL, name, valuep);
        if (status != NC_NOERR) handle_error(status);
        status = nc_put_att_double (ncid2, NC_GLOBAL, name, type, len, (double *)valuep);
        if (status != NC_NOERR) handle_error(status);
        free(valuep);
        break;

    }
  }

  /* Inquire dimension */

  for (i = 0; i < ndims; i++) {
    status = nc_inq_dim(ncid1, i, name, &len);
    if (status != NC_NOERR) handle_error(status);
    if (i == unlimdimid)
      len = NC_UNLIMITED;
    status = nc_def_dim(ncid2, name, len, dimids+i);
    if (status != NC_NOERR) handle_error(status);
  }

  /* Inquire variables */

  for (i = 0; i < nvars; i++) {
    status = nc_inq_var (ncid1, i, name, vartypes+i, varndims+i, vardims[i], varnatts+i);
    if (status != NC_NOERR) handle_error(status);

    status = nc_def_var(ncid2, name, vartypes[i], varndims[i], vardims[i], varids+i);
    if (status != NC_NOERR) handle_error(status);

    /* var attributes, assume CHAR attributes */

    for (j = 0; j < varnatts[i]; j++) {
      status = nc_inq_attname(ncid1, varids[i], j, name);
      if (status != NC_NOERR) handle_error(status);
      status = nc_inq_att (ncid1, varids[i], name, &type, &len);
      if (status != NC_NOERR) handle_error(status);
      switch (type) {
        case NC_CHAR: 
	  valuep = (void *)malloc(len * sizeof(char));
	  status = nc_get_att_text(ncid1, varids[i], name, valuep);
	  if (status != NC_NOERR) handle_error(status);
	  status = nc_put_att_text (ncid2, varids[i], name, len, (char *)valuep);
	  if (status != NC_NOERR) handle_error(status);
	  free(valuep);
          break;
        case NC_SHORT:
          valuep = (void *)malloc(len * sizeof(short));
          status = nc_get_att_short(ncid1, varids[i], name, valuep);
          if (status != NC_NOERR) handle_error(status);
          status = nc_put_att_short (ncid2, varids[i], name, type, len, (short *)valuep);
          if (status != NC_NOERR) handle_error(status);
          free(valuep);
          break;
        case NC_INT:
          valuep = (void *)malloc(len * sizeof(int));
          status = nc_get_att_int(ncid1, varids[i], name, valuep);
          if (status != NC_NOERR) handle_error(status);
          status = nc_put_att_int (ncid2, varids[i], name, type, len, (int *)valuep);
          if (status != NC_NOERR) handle_error(status);
          free(valuep);
          break;
        case NC_FLOAT:
          valuep = (void *)malloc(len * sizeof(float));
          status = nc_get_att_float(ncid1, varids[i], name, valuep);
          if (status != NC_NOERR) handle_error(status);
          status = nc_put_att_float (ncid2, varids[i], name, type, len, (float *)valuep);
          if (status != NC_NOERR) handle_error(status);
          free(valuep);
          break;
        case NC_DOUBLE:
          valuep = (void *)malloc(len * sizeof(double));
          status = nc_get_att_double(ncid1, varids[i], name, valuep);
          if (status != NC_NOERR) handle_error(status);
          status = nc_put_att_double (ncid2, varids[i], name, type, len, (double *)valuep);
          if (status != NC_NOERR) handle_error(status);
          free(valuep);
          break;
      }
    }
  }

  /**
   * End Define Mode (switch to data mode) for output dataset
   *   Dataset API: Collective
   */

  status = ncmpi_enddef(ncid2);
  if (status != NC_NOERR) handle_error(status);

  /**
   * Read data of variables from input dataset 
   * (ONLY DEAL WITH: NC_INT, NC_FLOAT, NC_DOUBLE for now)
   * Write the data out to the corresponding variables in the output dataset
   *
   *  Data Partition (Assume 4 processors):
   *   square: 2-D, (Block, *), 25*100 from 100*100
   *   cube:   3-D, (Block, *, *), 25*100*100 from 100*100*100
   *   xytime: 3-D, (Block, *, *), 25*100*100 from 100*100*100
   *   time:   1-D, Block-wise, 25 from 100
   *
   *  Data Mode API: non-collective
   */

  status = ncmpi_begin_indep_data(ncid1);
  if (status != NC_NOERR) handle_error(status);
  status =ncmpi_begin_indep_data(ncid2);
  if (status != NC_NOERR) handle_error(status);

  for (i = 0; i < NC_MAX_VAR_DIMS; i++)
    start[i] = 0;
  for (i = 0; i < nvars; i++) {
    isRecvar = 0;
    varsize = 1;
    for (j = 0; j < varndims[i]; j++) {
      status = nc_inq_dim(ncid1, vardims[i][j], name, shape + j);
      if (status != NC_NOERR) handle_error(status);
      if (j == 0) {
        shape[j] /= nprocs;
	start[j] = shape[j] * rank;
      }
      varsize *= shape[j];
      if (vardims[i][j] == unlimdimid)
	isRecvar = 1;
    }
    switch (vartypes[i]) {
      case NC_CHAR: 
        break;
      case NC_SHORT:
        valuep = (void *)malloc(varsize * sizeof(short));
        status = ncmpi_get_vara_short(ncid1, i, start, shape, (short *)valuep);
        if (status != NC_NOERR) handle_error(status);
        status = ncmpi_put_vara_short(ncid2, varids[i],
                                     start, shape, (short *)valuep);
        if (status != NC_NOERR) handle_error(status);
        free(valuep);
        break; 
      case NC_INT:
	valuep = (void *)malloc(varsize * sizeof(int));
        status = ncmpi_get_vara_int(ncid1, i, start, shape, (int *)valuep);
        if (status != NC_NOERR) handle_error(status);
        status = ncmpi_put_vara_int(ncid2, varids[i],
                                     start, shape, (int *)valuep);
        if (status != NC_NOERR) handle_error(status);
        free(valuep);
	break;
      case NC_FLOAT:
        valuep = (void *)malloc(varsize * sizeof(float));
        status = ncmpi_get_vara_float(ncid1, i, start, shape, (float *)valuep);
        if (status != NC_NOERR) handle_error(status);
        status = ncmpi_put_vara_float(ncid2, varids[i],
                                     start, shape, (float *)valuep);
        if (status != NC_NOERR) handle_error(status);
        free(valuep);
        break;
      case NC_DOUBLE:
        valuep = (void *)malloc(varsize * sizeof(int));
        status = ncmpi_get_vara_int(ncid1, i, start, shape, (int *)valuep);
        if (status != NC_NOERR) handle_error(status);
        status = ncmpi_put_vara_int(ncid2, varids[i],
                                     start, shape, (int *)valuep);
        if (status != NC_NOERR) handle_error(status);
        free(valuep);
        break;
    }
  }

  status = ncmpi_end_indep_data(ncid1);
  if (status != NC_NOERR) handle_error(status);
  status =ncmpi_end_indep_data(ncid2);
  if (status != NC_NOERR) handle_error(status);

  /**
   * Close the datasets
   *   Dataset API:  collective
   */

  status = ncmpi_close(ncid1);
  if (status != NC_NOERR) handle_error(status);
  status = ncmpi_close(ncid2);
  if (status != NC_NOERR) handle_error(status);

  /*******************  END OF NETCDF ACCESS  ****************/

if (rank == 0)
  fprintf(stderr, "OK\nInput file %s copied to: %s!\n", infname, outfname);

  MPI_Finalize();
}
