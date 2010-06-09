/*
 *  Copyright (C) 2003, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */

#include "nc.h"
#include "ncx.h"
#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <assert.h>

#include "ncmpidtype.h"
#include "macro.h"


/* buffer layers:       
        
        User Level              buf     (user defined buffer of MPI_Datatype)
        MPI Datatype Level      cbuf    (contiguous buffer of ptype)
        NetCDF XDR Level        xbuf    (XDR I/O buffer)
*/

/*----< ncmpi_put_vars() >---------------------------------------------------*/
int
ncmpi_put_vars(int               ncid,
               int               varid,
               const MPI_Offset  start[],
               const MPI_Offset  count[],
               const MPI_Offset  stride[],
               const void       *buf,
               MPI_Offset        bufcount,
               MPI_Datatype      datatype)
{
    int     status;
    NC     *ncp;
    NC_var *varp;

    CHECK_NCID
    CHECK_WRITE_PERMISSION
    if (NC_indef(ncp)) return NC_EINDEFINE;
    CHECK_INDEP_FH
    CHECK_VARID(varid, varp)

    return ncmpii_getput_vars(ncp, varp, start, count, stride,
                              (void*)buf, bufcount, datatype,
                              WRITE_REQ, INDEP_IO);
}

#define PUT_VARS_COMMON(datatype)                               \
    int         status;                                         \
    NC         *ncp;                                            \
    NC_var     *varp;                                           \
    MPI_Offset  nelems;                                         \
                                                                \
    CHECK_NCID                                                  \
    CHECK_WRITE_PERMISSION                                      \
    if (NC_indef(ncp)) return NC_EINDEFINE;                     \
    CHECK_INDEP_FH                                              \
    CHECK_VARID(varid, varp)                                    \
    GET_NUM_ELEMENTS                                            \
                                                                \
    return ncmpii_getput_vars(ncp, varp, start, count, stride,  \
                              (void*)op, nelems, datatype,      \
                              WRITE_REQ, INDEP_IO);

/*----< ncmpi_put_vars_text() >----------------------------------------------*/
int
ncmpi_put_vars_text(int               ncid,
                    int               varid,
                    const MPI_Offset  start[],
                    const MPI_Offset  count[],
                    const MPI_Offset  stride[],
                    const char       *op)
{
    PUT_VARS_COMMON(MPI_CHAR)
}

/*----< ncmpi_put_vars_schar() >---------------------------------------------*/
int
ncmpi_put_vars_schar(int                ncid,
                     int                varid,
                     const MPI_Offset   start[],
                     const MPI_Offset   count[],
                     const MPI_Offset   stride[],
                     const signed char *op)
{
    PUT_VARS_COMMON(MPI_BYTE)
}

/*----< ncmpi_put_vars_uchar() >---------------------------------------------*/
int
ncmpi_put_vars_uchar(int                  ncid,
                     int                  varid,
                     const MPI_Offset     start[],
                     const MPI_Offset     count[],
                     const MPI_Offset     stride[],
                     const unsigned char *op)
{
    PUT_VARS_COMMON(MPI_UNSIGNED_CHAR)
}

/*----< ncmpi_put_vars_short() >---------------------------------------------*/
int
ncmpi_put_vars_short(int               ncid,
                     int               varid,
                     const MPI_Offset  start[],
                     const MPI_Offset  count[],
                     const MPI_Offset  stride[],
                     const short      *op)
{
    PUT_VARS_COMMON(MPI_SHORT)
}

/*----< ncmpi_put_vars_int() >-----------------------------------------------*/
int
ncmpi_put_vars_int(int               ncid,
                   int               varid,
                   const MPI_Offset  start[],
                   const MPI_Offset  count[],
                   const MPI_Offset  stride[],
                   const int        *op)
{
    PUT_VARS_COMMON(MPI_INT)
}

/*----< ncmpi_put_vars_long() >----------------------------------------------*/
int
ncmpi_put_vars_long(int               ncid,
                    int               varid,
                    const MPI_Offset  start[],
                    const MPI_Offset  count[],
                    const MPI_Offset  stride[],
                    const long       *op)
{
    PUT_VARS_COMMON(MPI_LONG)
}

/*----< ncmpi_put_vars_float() >---------------------------------------------*/
int
ncmpi_put_vars_float(int               ncid,
                     int               varid,
                     const MPI_Offset  start[],
                     const MPI_Offset  count[],
                     const MPI_Offset  stride[],
                     const float      *op)
{
    PUT_VARS_COMMON(MPI_FLOAT)
}

/*----< ncmpi_put_vars_double() >--------------------------------------------*/
int
ncmpi_put_vars_double(int               ncid,
                      int               varid,
                      const MPI_Offset  start[],
                      const MPI_Offset  count[],
                      const MPI_Offset  stride[],
                      const double     *op)
{
    PUT_VARS_COMMON(MPI_DOUBLE)
}

/*----< ncmpi_get_vars() >---------------------------------------------------*/
int
ncmpi_get_vars(int               ncid,
               int               varid,
               const MPI_Offset  start[],
               const MPI_Offset  count[],
               const MPI_Offset  stride[],
               void             *buf,
               MPI_Offset        bufcount,
               MPI_Datatype      datatype)
{
    int     status;
    NC     *ncp;
    NC_var *varp;

    CHECK_NCID
    if (NC_indef(ncp)) return NC_EINDEFINE;
    CHECK_INDEP_FH
    CHECK_VARID(varid, varp)

    return ncmpii_getput_vars(ncp, varp, start, count, stride,
                              buf, bufcount, datatype,
                              READ_REQ, INDEP_IO);
}

#define GET_VARS_COMMON(datatype)                               \
    int         status;                                         \
    NC         *ncp;                                            \
    NC_var     *varp;                                           \
    MPI_Offset  nelems;                                         \
                                                                \
    CHECK_NCID                                                  \
    if (NC_indef(ncp)) return NC_EINDEFINE;                     \
    CHECK_INDEP_FH                                              \
    CHECK_VARID(varid, varp)                                    \
    GET_NUM_ELEMENTS                                            \
                                                                \
    return ncmpii_getput_vars(ncp, varp, start, count, stride,  \
                              ip, nelems, datatype,             \
                              READ_REQ, INDEP_IO);

/*----< ncmpi_get_vars_text() >----------------------------------------------*/
int
ncmpi_get_vars_text(int               ncid,
                    int               varid,
                    const MPI_Offset  start[],
                    const MPI_Offset  count[],
                    const MPI_Offset  stride[],
                    char             *ip)
{
    GET_VARS_COMMON(MPI_CHAR)
}

/*----< ncmpi_get_vars_schar() >---------------------------------------------*/
int
ncmpi_get_vars_schar(int               ncid,
                     int               varid,
                     const MPI_Offset  start[],
                     const MPI_Offset  count[],
                     const MPI_Offset  stride[],
                     signed char      *ip)
{
    GET_VARS_COMMON(MPI_BYTE)
}

/*----< ncmpi_get_vars_uchar() >---------------------------------------------*/
int
ncmpi_get_vars_uchar(int               ncid,
                     int               varid,
                     const MPI_Offset  start[],
                     const MPI_Offset  count[],
                     const MPI_Offset  stride[],
                     unsigned char    *ip)
{
    GET_VARS_COMMON(MPI_UNSIGNED_CHAR)
}

/*----< ncmpi_get_vars_short() >---------------------------------------------*/
int
ncmpi_get_vars_short(int               ncid,
                     int               varid,
                     const MPI_Offset  start[],
                     const MPI_Offset  count[],
                     const MPI_Offset  stride[],
                     short            *ip)
{
    GET_VARS_COMMON(MPI_SHORT)
}

/*----< ncmpi_get_vars_int() >-----------------------------------------------*/
int
ncmpi_get_vars_int(int               ncid,
                   int               varid,
                   const MPI_Offset  start[],
                   const MPI_Offset  count[],
                   const MPI_Offset  stride[],
                   int              *ip)
{
    GET_VARS_COMMON(MPI_INT)
}

/*----< ncmpi_get_vars_long() >----------------------------------------------*/
int
ncmpi_get_vars_long(int               ncid,
                    int               varid,
                    const MPI_Offset  start[],
                    const MPI_Offset  count[],
                    const MPI_Offset  stride[],
                    long             *ip)
{
    GET_VARS_COMMON(MPI_LONG)
}

/*----< ncmpi_get_vars_float() >---------------------------------------------*/
int
ncmpi_get_vars_float(int               ncid,
                     int               varid,
                     const MPI_Offset  start[],
                     const MPI_Offset  count[],
                     const MPI_Offset  stride[],
                     float            *ip)
{
    GET_VARS_COMMON(MPI_FLOAT)
}

/*----< ncmpi_get_vars_double() >--------------------------------------------*/
int
ncmpi_get_vars_double(int               ncid,
                      int               varid,
                      const MPI_Offset  start[],
                      const MPI_Offset  count[],
                      const MPI_Offset  stride[],
                      double           *ip)
{
    GET_VARS_COMMON(MPI_DOUBLE)
}

/*----< ncmpi_put_vars_all() >-----------------------------------------------*/
int
ncmpi_put_vars_all(int               ncid,
                   int               varid,
                   const MPI_Offset  start[],
                   const MPI_Offset  count[],
                   const MPI_Offset  stride[],
                   const void       *buf,
                   MPI_Offset        bufcount,
                   MPI_Datatype      datatype)
{
    int     status;
    NC     *ncp;
    NC_var *varp;

    CHECK_NCID
    CHECK_WRITE_PERMISSION
    if (NC_indef(ncp)) return NC_EINDEFINE;
    CHECK_COLLECTIVE_FH
    CHECK_VARID(varid, varp)

    return ncmpii_getput_vars(ncp, varp, start, count, stride,
                              (void*)buf, bufcount, datatype,
                              WRITE_REQ, COLL_IO);
}

#define PUT_VARS_ALL_COMMON(datatype)                            \
    int         status;                                          \
    NC         *ncp;                                             \
    NC_var     *varp;                                            \
    MPI_Offset  nelems;                                          \
                                                                 \
    CHECK_NCID                                                   \
    CHECK_WRITE_PERMISSION                                       \
    if (NC_indef(ncp)) return NC_EINDEFINE;                      \
    CHECK_COLLECTIVE_FH                                          \
    CHECK_VARID(varid, varp)                                     \
    GET_NUM_ELEMENTS                                             \
                                                                 \
    return ncmpii_getput_vars(ncp, varp, start, count, stride,   \
                              (void*)op, nelems, datatype,       \
                              WRITE_REQ, COLL_IO);

/*----< ncmpi_put_vars_text_all() >------------------------------------------*/
int
ncmpi_put_vars_text_all(int               ncid,
                        int               varid,
                        const MPI_Offset  start[],
                        const MPI_Offset  count[],
                        const MPI_Offset  stride[],
                        const char       *op)
{
    PUT_VARS_ALL_COMMON(MPI_CHAR)
}

/*----< ncmpi_put_vars_schar_all() >-----------------------------------------*/
int
ncmpi_put_vars_schar_all(int                ncid,
                         int                varid,
                         const MPI_Offset   start[],
                         const MPI_Offset   count[],
                         const MPI_Offset   stride[],
                         const signed char *op)
{
    PUT_VARS_ALL_COMMON(MPI_BYTE)
}

/*----< ncmpi_put_vars_uchar_all() >-----------------------------------------*/
int
ncmpi_put_vars_uchar_all(int                  ncid,
                         int                  varid,
                         const MPI_Offset     start[],
                         const MPI_Offset     count[],
                         const MPI_Offset     stride[],
                         const unsigned char *op)
{
    PUT_VARS_ALL_COMMON(MPI_UNSIGNED_CHAR)
}

/*----< ncmpi_put_vars_short_all() >-----------------------------------------*/
int
ncmpi_put_vars_short_all(int               ncid,
                         int               varid,
                         const MPI_Offset  start[],
                         const MPI_Offset  count[],
                         const MPI_Offset  stride[],
                         const short      *op)
{
    PUT_VARS_ALL_COMMON(MPI_SHORT)
}

/*----< ncmpi_put_vars_int_all() >-------------------------------------------*/
int
ncmpi_put_vars_int_all(int               ncid,
                       int               varid,
                       const MPI_Offset  start[],
                       const MPI_Offset  count[],
                       const MPI_Offset  stride[],
                       const int        *op)
{
    PUT_VARS_ALL_COMMON(MPI_INT)
}

/*----< ncmpi_put_vars_long_all() >------------------------------------------*/
int
ncmpi_put_vars_long_all(int               ncid,
                        int               varid,
                        const MPI_Offset  start[],
                        const MPI_Offset  count[],
                        const MPI_Offset  stride[],
                        const long       *op)
{
    PUT_VARS_ALL_COMMON(MPI_LONG)
}

/*----< ncmpi_put_vars_float_all() >-----------------------------------------*/
int
ncmpi_put_vars_float_all(int               ncid,
                         int               varid,
                         const MPI_Offset  start[],
                         const MPI_Offset  count[],
                         const MPI_Offset  stride[],
                         const float      *op)
{
    PUT_VARS_ALL_COMMON(MPI_FLOAT)
}

/*----< ncmpi_put_vars_double_all() >----------------------------------------*/
int
ncmpi_put_vars_double_all(int               ncid,
                          int               varid,
                          const MPI_Offset  start[],
                          const MPI_Offset  count[],
                          const MPI_Offset  stride[],
                          const double     *op)
{
    PUT_VARS_ALL_COMMON(MPI_DOUBLE)
}

/*----< ncmpi_get_vars_all() >-----------------------------------------------*/
int
ncmpi_get_vars_all(int               ncid,
                   int               varid,
                   const MPI_Offset  start[],
                   const MPI_Offset  count[],
                   const MPI_Offset  stride[],
                   void             *buf,
                   MPI_Offset        bufcount,
                   MPI_Datatype      datatype)
{
    int     status;
    NC     *ncp;
    NC_var *varp;

    CHECK_NCID
    if (NC_indef(ncp)) return NC_EINDEFINE;
    CHECK_COLLECTIVE_FH
    CHECK_VARID(varid, varp)

    return ncmpii_getput_vars(ncp, varp, start, count, stride,
                              buf, bufcount, datatype,
                              READ_REQ, COLL_IO);
}

#define GET_VARS_ALL_COMMON(datatype)                            \
    int         status;                                          \
    NC         *ncp;                                             \
    NC_var     *varp;                                            \
    MPI_Offset  nelems;                                          \
                                                                 \
    CHECK_NCID                                                   \
    if (NC_indef(ncp)) return NC_EINDEFINE;                      \
    CHECK_COLLECTIVE_FH                                          \
    CHECK_VARID(varid, varp)                                     \
    GET_NUM_ELEMENTS                                             \
                                                                 \
    return ncmpii_getput_vars(ncp, varp, start, count, stride,   \
                              ip, nelems, datatype,              \
                              READ_REQ, COLL_IO);

/*----< ncmpi_get_vars_text_all() >------------------------------------------*/
int
ncmpi_get_vars_text_all(int               ncid,
                        int               varid,
                        const MPI_Offset  start[],
                        const MPI_Offset  count[],
                        const MPI_Offset  stride[],
                        char             *ip)
{
    GET_VARS_ALL_COMMON(MPI_CHAR)
}

/*----< ncmpi_get_vars_schar_all() >-----------------------------------------*/
int
ncmpi_get_vars_schar_all(int               ncid,
                         int               varid,
                         const MPI_Offset  start[],
                         const MPI_Offset  count[],
                         const MPI_Offset  stride[],
                         signed char      *ip)
{
    GET_VARS_ALL_COMMON(MPI_BYTE)
}

/*----< ncmpi_get_vars_uchar_all() >-----------------------------------------*/
int
ncmpi_get_vars_uchar_all(int               ncid,
                         int               varid,
                         const MPI_Offset  start[],
                         const MPI_Offset  count[],
                         const MPI_Offset  stride[],
                         unsigned char    *ip)
{
    GET_VARS_ALL_COMMON(MPI_UNSIGNED_CHAR)
}

/*----< ncmpi_get_vars_short_all() >-----------------------------------------*/
int
ncmpi_get_vars_short_all(int               ncid,
                         int               varid,
                         const MPI_Offset  start[],
                         const MPI_Offset  count[],
                         const MPI_Offset  stride[],
                         short            *ip)
{
    GET_VARS_ALL_COMMON(MPI_SHORT)
}

/*----< ncmpi_get_vars_int_all() >-------------------------------------------*/
int
ncmpi_get_vars_int_all(int               ncid,
                       int               varid,
                       const MPI_Offset  start[],
                       const MPI_Offset  count[],
                       const MPI_Offset  stride[],
                       int              *ip)
{
    GET_VARS_ALL_COMMON(MPI_INT)
}

/*----< ncmpi_get_vars_long_all() >------------------------------------------*/
int
ncmpi_get_vars_long_all(int               ncid,
                        int               varid,
                        const MPI_Offset  start[],
                        const MPI_Offset  count[],
                        const MPI_Offset  stride[],
                        long             *ip)
{
    GET_VARS_ALL_COMMON(MPI_LONG)
}

/*----< ncmpi_get_vars_float_all() >-----------------------------------------*/
int
ncmpi_get_vars_float_all(int               ncid,
                         int               varid,
                         const MPI_Offset  start[],
                         const MPI_Offset  count[],
                         const MPI_Offset  stride[],
                         float            *ip)
{
    GET_VARS_ALL_COMMON(MPI_FLOAT)
}

/*----< ncmpi_get_vars_double_all() >----------------------------------------*/
int
ncmpi_get_vars_double_all(int               ncid,
                          int               varid,
                          const MPI_Offset  start[],
                          const MPI_Offset  count[],
                          const MPI_Offset  stride[],
                          double           *ip)
{
    GET_VARS_ALL_COMMON(MPI_DOUBLE)
}

#define CALLING_MPI_WRITE {                                                    \
    if (io_method == COLL_IO) {                                                \
        mpireturn = MPI_File_write_all(fh, xbuf, nbytes, MPI_BYTE, &mpistatus);\
        CHECK_MPI_ERROR("MPI_File_write_all", NC_EWRITE)                       \
    }                                                                          \
    else { /* io_method == INDEP_IO */                                         \
        mpireturn = MPI_File_write(fh, xbuf, nbytes, MPI_BYTE, &mpistatus);    \
        CHECK_MPI_ERROR("MPI_File_write", NC_EWRITE)                           \
    }                                                                          \
}

#define CALLING_MPI_READ {                                                     \
    if (io_method == COLL_IO) {                                                \
        mpireturn = MPI_File_read_all(fh, xbuf, nbytes, MPI_BYTE, &mpistatus); \
        CHECK_MPI_ERROR("MPI_File_read_all", NC_EREAD)                         \
    }                                                                          \
    else { /* io_method == INDEP_IO */                                         \
        mpireturn = MPI_File_read(fh, xbuf, nbytes, MPI_BYTE, &mpistatus);     \
        CHECK_MPI_ERROR("MPI_File_read", NC_EREAD)                             \
    }                                                                          \
}

/*----< ncmpii_getput_vars() >-----------------------------------------------*/
int
ncmpii_getput_vars(NC               *ncp,
                   NC_var           *varp,
                   const MPI_Offset  start[],
                   const MPI_Offset  count[],
                   const MPI_Offset  stride[],
                   void             *buf,
                   MPI_Offset        bufcount,
                   MPI_Datatype      datatype,
                   int               rw_flag,
                   int               io_method)
{
    void *xbuf=NULL, *cbuf=NULL;
    int i, warning=NC_NOERR, el_size, iscontig_of_ptypes, mpireturn;
    int err, status; /* err is for fatal return and status is not */
    MPI_Offset nelems, cnelems, nbytes, offset;
    MPI_Status mpistatus;
    MPI_Datatype ptype;
    MPI_File fh;

    if (varp->ndims > 0) assert(start != NULL);
    if (varp->ndims > 0) assert(count != NULL);

    if (io_method == COLL_IO)
        fh = ncp->nciop->collective_fh;
    else
        fh = ncp->nciop->independent_fh;

    CHECK_DATATYPE(datatype, ptype, el_size, cnelems, iscontig_of_ptypes)
    CHECK_ECHAR(varp)
    CHECK_NELEMS(varp, cnelems, count, bufcount, nelems, nbytes)
    if (cnelems == 0 && io_method == INDEP_IO)
        return NCcoordck(ncp, varp, start);
    /* for collective I/O, even cnelems == 0, must go on to participate the
       collective call */

    if (!iscontig_of_ptypes) {
        /* handling for derived datatype: pack into a contiguous buffer */
        cbuf = (void*) NCI_Malloc( cnelems * el_size );
        if (rw_flag == WRITE_REQ) {
            err = ncmpii_data_repack((void*)buf, bufcount, datatype,
                                     cbuf, cnelems, ptype);
            if (err != NC_NOERR)
                return err;
        }
    } else {
        cbuf = (void*) buf;
    }

    if ( ncmpii_need_convert(varp->type, ptype) ) {
        /* allocate new buffer for data type conversion */
        xbuf = NCI_Malloc(nbytes);

        if (rw_flag == WRITE_REQ) {
            /* automatic numeric datatype conversion + swap if necessary */
            DATATYPE_PUT_CONVERT(varp->type, xbuf, cbuf, cnelems, ptype)
            /* status may be set after DATATYPE_PUT_CONVERT() */
        }
    } else if ( ncmpii_need_swap(varp->type, ptype) ) {
        if (rw_flag == WRITE_REQ) /* perform array in-place byte swap */
            ncmpii_in_swapn(cbuf, nelems, ncmpix_len_nctype(varp->type));
        xbuf = cbuf;
    } else {
        /* else, just assign contiguous buffer */
        xbuf = cbuf;
    }

    /* if record variables are too big (so big that we cannot store the stride
     * between records in an MPI_Aint, for example) then we will have to
     * process this one record at a time.  
     *
     * It stinks that we have to make this change in multiple places by the way
     */

    /* check if the request is contiguous in file */
    if (stride == NULL && ncmpii_is_request_contiguous(varp, start, count)) {
        err = NCedgeck(ncp, varp, start, count);

        if (err != NC_NOERR ||
            (rw_flag == READ_REQ && IS_RECVAR(varp) &&
             start[0] + count[0] > NC_get_numrecs(ncp))) {
            err = NCcoordck(ncp, varp, start);
            if (err != NC_NOERR)
                return err;
            else
                return NC_EEDGE;
        }

        /* this is a contiguous file access, no need to set fileview */
        err = ncmpii_get_offset(ncp, varp, start, NULL, NULL, &offset);
        if (err != NC_NOERR)
            return err;

        mpireturn = MPI_File_seek(fh, offset, MPI_SEEK_SET);
        CHECK_MPI_ERROR("MPI_File_seek", NC_EWRITE)

        if (rw_flag == WRITE_REQ)
            CALLING_MPI_WRITE
        else
            CALLING_MPI_READ
    }
    else { /* non-contiguous I/O, set the MPI fileview */
        /* this request is non-contiguous in file, set the mpi file view */
        MPI_Datatype filetype;
        err = ncmpii_vars_create_filetype(ncp, varp, start, count, stride,
                                          rw_flag, &offset, &filetype);
        if (err != NC_NOERR)
            return err;
 
        mpireturn = MPI_File_set_view(fh, offset, MPI_BYTE, filetype,
                                      "native", MPI_INFO_NULL);
        CHECK_MPI_ERROR("MPI_File_set_view", NC_EFILE)

        if (filetype != MPI_BYTE)
            MPI_Type_free(&filetype);

        if (rw_flag == WRITE_REQ)
            CALLING_MPI_WRITE
        else
            CALLING_MPI_READ

        /* reset the file view so the entire file is visible again */
        MPI_File_set_view(fh, 0, MPI_BYTE, MPI_BYTE, "native", MPI_INFO_NULL);
    }

    if (rw_flag == READ_REQ) {
        if ( ncmpii_need_convert(varp->type, ptype) ) {
            /* automatic numeric datatype conversion + swap if necessary */
            DATATYPE_GET_CONVERT(varp->type, xbuf, cbuf, cnelems, ptype)
        } else if ( ncmpii_need_swap(varp->type, ptype) ) {
            /* perform array in-place byte swap */
            ncmpii_in_swapn(cbuf, nelems, ncmpix_len_nctype(varp->type));
        }

        if (!iscontig_of_ptypes) {
            /* handling derived datatype: unpack from the contiguous buffer */
            err = ncmpii_data_repack(cbuf, cnelems, ptype,
                                     (void*)buf, bufcount, datatype);
            if (err != NC_NOERR)
                return err;
        }
    }
    else {
        if (ncmpii_need_swap(varp->type, ptype) && cbuf == buf && cbuf == xbuf)
            ncmpii_in_swapn(cbuf, nelems, ncmpix_len_nctype(varp->type));

        if (status == NC_NOERR && IS_RECVAR(varp)) {
            /* update header's number of records in memory */
            MPI_Offset newnumrecs;
            if (stride == NULL)
                newnumrecs = start[0] + count[0];
            else
                newnumrecs = start[0] + (count[0] - 1) * stride[0] + 1;

            if (io_method == INDEP_IO) {
                ncp->numrecs = newnumrecs;
                set_NC_ndirty(ncp);
            }
            else
                ncmpii_update_numrecs(ncp, newnumrecs);
        }
    }

    if (xbuf != cbuf && xbuf != NULL)
        NCI_Free(xbuf);
    if (cbuf != buf && cbuf != NULL)
        NCI_Free(cbuf);
 
    return ((warning != NC_NOERR) ? warning : status);
}