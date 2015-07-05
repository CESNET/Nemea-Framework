/**
 * \file ifc_file.c
 * \brief TRAP file interfaces
 * \author Tomas Jansky <janskto1@fit.cvut.cz>
 * \date 2015
 */
/*
 * Copyright (C) 2015 CESNET
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
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include "../include/libtrap/trap.h"
#include "trap_ifc.h"
#include "trap_internal.h"
#include "trap_error.h"

/**
 * \addtogroup trap_ifc TRAP communication module interface
 * @{
 */
/**
 * \addtogroup file_ifc file interface module
 * @{
 */

typedef struct file_private_s {
   trap_ctx_priv_t *ctx;
   FILE *fd;
   char *filename;
   char mode[3];
   char is_terminated;
} file_private_t;

/**
 * \brief Close file and free allocated memory.
 * \param[in] priv   pointer to module private data
 */
void file_destroy(void *priv)
{
   file_private_t *config = (file_private_t*) priv;

   if (config) {
      fclose(config->fd);
      free(config->filename);
      free(config);
   } else {
      VERBOSE(CL_ERROR, "FILE IFC: attempt to destroy IFC that is probably not initialized.");
   }
}

/**
 * \brief Set interface state as terminated.
 * \param[in] priv   pointer to module private data
 */
void file_terminate(void *priv)
{
   if (priv) {
      ((file_private_t*)priv)->is_terminated = 1;
   } else {
      VERBOSE(CL_ERROR, "FILE IFC: attempt to terminate IFC that is probably not initialized.");
   }
}

/**
 * \brief Create file dump with current configuration.
 * \param[in] priv   pointer to module private data
 * \param[in] idx    number of interface
 * \param[in] path   path where dump file will be created
 */
static void file_create_dump(void *priv, uint32_t idx, const char *path)
{
   int ret;
   char *config_file = NULL;
   FILE *fd = NULL;

   file_private_t *cf = (file_private_t*) priv;
   ret = asprintf(&config_file, "%s/trap-i%02"PRIu32"-config.txt", path, idx);
   if (ret != -1) {
      VERBOSE(CL_ERROR, "FILE IFC: not enough memory, dump failed. (%s:%d)", __FILE__, __LINE__);
      return;
   }

   fd = fopen(config_file, "w");
   if (fd == NULL) {
      free(config_file);
      VERBOSE(CL_ERROR, "FILE IFC: unable to write to dump file. (%s:%d)", __FILE__, __LINE__);
      return;
   }

   fprintf(fd, "Filename: %s\nMode: %s\nTerminated status: %c\n", cf->filename, cf->mode, cf->is_terminated);
   fclose(fd);
}

/***** Receiver *****/

/**
 * \addtogroup file_receiver
 * @{
 */

/**
 * \brief Read data from a file.
 * \param[in] priv   pointer to module private data
 * \param[out] data  pointer to a memory block in which data is to be stored
 * \param[out] size  pointer to a memory block in which size of read data is to be stored
 * \param[in] timeout   NOT USED IN THIS INTERFACE
 * \return 0 on success (TRAP_E_OK), TTRAP_E_IO_ERROR if error occurs during reading, TRAP_E_TERMINATED if interface was terminated.
 */
int file_recv(void *priv, void *data, uint32_t *size, int timeout)
{
   size_t loaded;
   file_private_t *config = (file_private_t*) priv;

   if (config->is_terminated) {
      return trap_error(config->ctx, TRAP_E_TERMINATED);
   }

   /* Reads 4 bytes from the file, determinating the length of bytes to be read to @param[out] data */
   loaded = fread(size, sizeof(uint32_t), 1, config->fd);

   if (loaded != 1) {
      if (feof(config->fd)) {
         (*size) = 0;
         return TRAP_E_OK;
      }

      VERBOSE(CL_ERROR, "INPUT FILE IFC: read error occured in file: %s", config->filename);
      return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "INPUT FILE IFC: unable to read");
   }

   /* Reads (*size) bytes from the file */
   loaded = fread(data, 1, (*size), config->fd);

   if (loaded != (*size)) {
      VERBOSE(CL_ERROR, "INPUT FILE IFC: loaded incorrect number of bytes from file: %s", config->filename);
   }

   return TRAP_E_OK;
}

/**
 * \brief Initiate file input interface
 * This function is called by TRAP library to initialize one input interface.
 *
 * \param[in,out] ctx   pointer to the private libtrap context data (trap_ctx_init())
 * \param[in] params    configuration string containing *file_name*,
 * where file_name is a path to a file from which data is to be read
 * \param[in,out] ifc   IFC interface used for calling file module.
 * \return 0 on success (TRAP_E_OK), TRAP_E_MEMORY, TRAP_E_BADPARAMS on error
 */
int create_file_recv_ifc(trap_ctx_priv_t *ctx, const char *params, trap_input_ifc_t *ifc)
{
   file_private_t *priv;
   size_t name_length = strlen(params);

   if (params == NULL) {
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "parameter is null pointer");
   }

   /* Create structure to store private data */
   priv = calloc(1, sizeof(file_private_t));
   if (!priv) {
      return trap_error(ctx, TRAP_E_MEMORY);
   }

   priv->ctx = ctx;
   priv->filename = (char *) calloc(name_length + 1, sizeof(char));
   if (!priv->filename) {
      free(priv->filename);
      free(priv);
      return trap_error(ctx, TRAP_E_MEMORY);
   }

   /* Stets mode and filename */
   priv->mode[0] = 'r';
   priv->mode[1] = 'b';
   priv->mode[2] = '\0';
   strncpy(priv->filename, params, name_length + 1);

   /* Attempts to open the file */
   priv->fd = fopen(priv->filename, priv->mode);
   if (priv->fd == NULL) {
      free(priv->filename);
      free(priv);
      VERBOSE(CL_ERROR, "CREATE INPUT FILE IFC: unable to open file --%s--. Possible reasons: non-existing file, bad permission.", priv->filename);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "unable to open file");
   }

   priv->is_terminated = 0;

   /* Fills interface structure */
   ifc->recv = file_recv;
   ifc->terminate = file_terminate;
   ifc->destroy = file_destroy;
   ifc->create_dump = file_create_dump;
   ifc->priv = priv;

   return TRAP_E_OK;
}

/**
 * @}
 *//* file_receiver */

/***** Sender *****/

/**
 * \addtogroup file_sender
 * @{
 */

/**
 * \brief Write data to a file.
 * Data to write are expected as a trap_buffer_header_t structure, thus actual length of data to be written is determined from trap_buffer_header_t->data_length
 * trap_buffer_header_t->data_length is expected to be in network byte order (little endian)
 *
 * \param[in] priv   pointer to module private data
 * \param[in] data   pointer to data to write
 * \param[in] size   size of data to write - NOT USED IN THIS INTERFACE
 * \param[in] timeout   NOT USED IN THIS INTERFACE
 * \return 0 on success (TRAP_E_OK), TTRAP_E_IO_ERROR if error occurs during writing, TRAP_E_TERMINATED if interface was terminated.
 */
int file_send(void *priv, const void *data, uint32_t size, int timeout)
{
   file_private_t *config = (file_private_t*) priv;
   const trap_buffer_header_t *data_struct = (trap_buffer_header_t *) data;
   size_t written;
   uint32_t size_little_e;

   if (config->is_terminated) {
      return trap_error(config->ctx, TRAP_E_TERMINATED);
   }

   /* Converts data_length to host byte order (little endian) */
   size_little_e = ntohl(data_struct->data_length);

   /* Writes data_length to the file in host file order (little endian) */
   written = fwrite(&size_little_e, sizeof(uint32_t), 1, config->fd);
   if (written != 1) {
      VERBOSE(CL_ERROR, "OUTPUT FILE IFC: unable to write to file: %s", config->filename);
      return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "FILE_SENDER: unable to write");
   }

   /* Writes data_length bytes to the file */
   written = fwrite(data_struct->data, 1, size_little_e, config->fd);
   if (written != size_little_e) {
      VERBOSE(CL_ERROR, "OUTPUT FILE IFC: unable to write to file: %s", config->filename);
      return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "OUTPUT FILE IFC: unable to write");
   }

   return TRAP_E_OK;
}

int32_t file_get_client_count(void *priv)
{
   return 1;
}

/**
 * \brief Initiate file output interface
 * \param[in,out] ctx   pointer to the private libtrap context data (trap_ctx_init())
 * \param[in] params    configuration string containing colon separated values of these parameters (in this exact order): *file_name*:*open_mode*,
 * where file_name is a path to a file in which data is to be written and
 * open_mode is either a - append or w - write, if no mode is specified, the file will be opened in append mode.
 * \param[in,out] ifc   IFC interface used for calling file module.
 * \return 0 on success (TRAP_E_OK), TRAP_E_MEMORY, TRAP_E_BADPARAMS on error
 */
int create_file_send_ifc(trap_ctx_priv_t *ctx, const char *params, trap_output_ifc_t *ifc)
{
   file_private_t *priv;
   char *dest, *ret;

   if (params == NULL) {
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "parameter is null pointer");
   }

   /* Create structure to store private data */
   priv = calloc(1, sizeof(file_private_t));
   if (!priv) {
      return trap_error(ctx, TRAP_E_MEMORY);
   }

   priv->ctx = ctx;


   /* Parses and sets filename and mode */
   priv->filename = dest = NULL;
   ret = trap_get_param_by_delimiter(params, &(priv->filename), ':');

   if (!ret) {
      if (priv->filename) {
         priv->mode[0] = 'a';
      } else {
         free(priv);
         return trap_error(ctx, TRAP_E_MEMORY);
      }
   } else {
      trap_get_param_by_delimiter(ret, &dest, ':');
      if (!dest) {
         free(priv);
         return trap_error(ctx, TRAP_E_MEMORY);
      }

      priv->mode[0] = dest[0];
   }

   priv->mode[1] = 'b';
   priv->mode[2] = '\0';

   free(dest);

   /* Attempts to open the file */
   priv->fd = fopen(priv->filename, priv->mode);
   if (priv->fd == NULL) {
      free(priv->filename);
      free(priv);
      VERBOSE(CL_ERROR, "CREATE OUTPUT FILE IFC : unable to open file --%s--. Possible reasons: non-existing file, bad permission.", priv->filename);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "unable to open file");
   }

   priv->is_terminated = 0;

   /* Fills interface structure */
   ifc->send = file_send;
   ifc->terminate = file_terminate;
   ifc->destroy = file_destroy;
   ifc->get_client_count = file_get_client_count;
   ifc->create_dump = file_create_dump;
   ifc->priv = priv;

   return TRAP_E_OK;
}

/**
 * @}
 *//* file_sender */

/**
 * @}
 *//* file_ifc module */

/**
 * @}
 *//* ifc modules */

