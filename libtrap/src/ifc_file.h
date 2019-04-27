/**
 * \file ifc_file.h
 * \brief TRAP file interfaces
 * \author Tomas Jansky <janskto1@fit.cvut.cz>
 * \date 2015
 */
/*
 * Copyright (C) 2015, 2016 CESNET
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */
#ifndef _TRAP_IFC_FILE_H_
#define _TRAP_IFC_FILE_H_

#include <limits.h>

#ifdef HAVE_ZLIB_H
#  include <zlib.h>
#  define ZLIB_CHUNK (2 * TRAP_IFC_MESSAGEQ_SIZE)
#endif

#include "trap_ifc.h"


typedef struct file_private_s {
   trap_ctx_priv_t *ctx;
   FILE *fd;
   char **files;
   char filename_tmplt[PATH_MAX];
   char filename[PATH_MAX];
   char mode[3];
   char is_terminated;
   char is_gzip;
   char is_input;
   uint8_t neg_initialized;
   time_t create_time;
   size_t file_index;
   uint32_t file_cnt;
   uint32_t ifc_idx;
   uint32_t file_change_size;
   uint32_t file_change_time;
#ifdef HAVE_ZLIB_H
   /**
    * Internal structure for zlib
    */
   z_stream zlib_strm;

   /**
    * State of zlib inflate, signalizes Z_STREAM_END.
    */
   int zlib_instate;

   /**
    * Input buffer for zlib
    */
   unsigned char zlib_outdata[ZLIB_CHUNK];

   /**
    * Output buffer for zlib
    */
   unsigned char zlib_indata[ZLIB_CHUNK];

   /**
    * Pointer into zlib_outdata, used to serve decompressed data via zlib_fread()
    */
   unsigned char *outdata_p;

   /**
    * Size of available decompressed data via zlib_fread()
    */
   uint32_t outdata_avail;
#endif
} file_private_t;

/** Create file receive interface (input ifc).
 *  Receive function of this interface reads data from defined file.
 *  @param[in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 *  @param[in] params <filename> expected.
 *  @param[out] ifc Created interface.
 *  @return Error code (0 on success). Generated interface is returned in ifc.
 */
int create_file_recv_ifc(trap_ctx_priv_t *ctx, const char *params, trap_input_ifc_t *ifc, uint32_t idx);


/** Create file send interface (output ifc).
 *  Send function of this interface stores data into defined file.
 *  @param[in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 *  @param[in] params <filename>:<mode>
 *                    <mode> is optional, w - write, a - append. Append is set as default mode.
 *  @param[out] ifc Created interface.
 *  @return Error code (0 on success). Generated interface is returned in ifc.
 */
int create_file_send_ifc(trap_ctx_priv_t *ctx, const char *params, trap_output_ifc_t *ifc, uint32_t idx);

/** Universal fread of file IFC, it can be used to read any supported file type.
 *  This function should be used outside the file IFC source code.
 *  @param[in] priv    Pointer to the private file IFC context.
 *  @param[out] ptr    Pointer to memory where to store data.
 *  @param[in] size    Size of element to read.
 *  @param[in] nmemb   Number of elements to read.
 *  @return Number of read elements.
 */
size_t file_ifc_fread(file_private_t *priv, void *ptr, size_t size, size_t nmemb);

#endif
