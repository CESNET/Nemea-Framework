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
#include "trap_ifc.h"

#define TIME_PARAM             "time="
#define TIME_PARAM_LEN         strlen(TIME_PARAM)
#define SIZE_PARAM             "size="
#define SIZE_PARAM_LEN         strlen(SIZE_PARAM)
#define TIME_FORMAT_STRING     ".%Y%m%d%H%M"
#define TIME_FORMAT_STRING_LEN strlen(TIME_FORMAT_STRING)
#define FILE_SIZE_SUFFIX_LEN   6
#define FILENAME_TEMPLATE_LEN  PATH_MAX + 256

typedef struct file_buffer_s {
    uint32_t wr_index;                      /**< Pointer to first free byte in buffer payload */
    uint8_t *header;                        /**< Pointer to first byte in buffer */
    uint8_t *data;                          /**< Pointer to first byte of buffer payload */
    uint8_t finished;                       /**< Flag indicating whether buffer is full and ready to be sent */
} file_buffer_t;

typedef struct file_private_s {
   trap_ctx_priv_t *ctx;
   FILE *fd;
   time_t create_time;
   char **files;
   char filename_tmplt[FILENAME_TEMPLATE_LEN];
   char filename[PATH_MAX];
   char mode[3];
   char is_terminated;
   uint8_t neg_initialized;
   uint16_t file_index;
   uint16_t file_cnt;
   uint32_t file_change_size;
   uint32_t file_change_time;
   uint32_t buffer_size;                   /**< Buffer size [bytes] */
   uint32_t ifc_idx;                       /**< Index of interface in 'ctx->out_ifc_list' array */

   file_buffer_t buffer;
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
 *  @param[in] params <filename>:<mode>:<time>:<size>
 *                    <mode> is optional, w - write, a - append. Write is set as default mode.
 *                    <time> is optional, periodically rotates file in which the data is being written.
 *                    <size> is optional, rotates file in which the data is being written once it reaches specified size.
 *  @param[out] ifc Created interface.
 *  @return Error code (0 on success). Generated interface is returned in ifc.
 */
int create_file_send_ifc(trap_ctx_priv_t *ctx, const char *params, trap_output_ifc_t *ifc, uint32_t idx);

#endif
