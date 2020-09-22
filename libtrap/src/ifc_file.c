/**
 * \file ifc_file.c
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
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <wordexp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "../include/libtrap/trap.h"
#include "trap_ifc.h"
#include "trap_internal.h"
#include "trap_error.h"
#include "ifc_file.h"

/**
 * \addtogroup trap_ifc TRAP communication module interface
 * @{
 */
/**
 * \addtogroup file_ifc file interface module
 * @{
 */

/**
 * \brief Close file and free allocated memory.
 * \param[in] priv   pointer to module private data
 */
void file_destroy(void *priv)
{
   int i;
   file_private_t *config = (file_private_t*) priv;

   if (config) {
      if (config->file_cnt != 0) {
         for (i = 0; i < config->file_cnt; i++) {
            free(config->files[i]);
         }

         free(config->files);
      }

      if (config->fd) {
         fclose(config->fd);
      }

      if (config->buffer.header) {
         free(config->buffer.header);
      }

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
 * Author Jonathon Reinhart
 * Adapted from https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 * \brief Create path, recursive.
 * \param[in] path where file will be created
 * \return 0 on success, -1 otherwise
 */
int _mkdir(const char *path)
{
   mode_t perm = S_IRWXU | S_IXGRP | S_IRGRP | S_IROTH | S_IXOTH;
   const size_t len = strlen(path);
   char _path[PATH_MAX];
   char *p;

   if (len > sizeof(_path) - 1) {
      return -1;
   }

   strcpy(_path, path);
   for (p = _path + 1; *p; p++) {
      if (*p == '/') {
         /* Temporarily truncate */
         *p = '\0';

         if (mkdir(_path, perm) != 0) {
            if (errno != EEXIST) {
               return -1;
            }
         }

         *p = '/';
      }
   }

   return 0;
}

/**
 * \brief Create file dump with current configuration (for debugging)
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
   if (ret == -1) {
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
   free(config_file);
}

/**
 * \brief Create a new path and filename from the template created during interface initialization.
 *        New filename is stored in file_private_t->filename.
 *
 * \param[in,out] config Pointer to module private data.
 *
 * \return TRAP_E_OK        on success,
           TRAP_E_MEMORY    if function time(NULL) returns -1,
           TRAP_E_IO_ERROR  if error occurs during directory creation,
           TRAP_E_BADPARAMS if the specified path and filename exceeds MAX_PATH - 1 bytes.
 */
int create_next_filename(file_private_t *config)
{
   char buf[PATH_MAX];
   char suffix[FILE_SIZE_SUFFIX_LEN + 1];
   uint8_t valid_suffix_present = 0;

   config->create_time = time(NULL);
   if (config->create_time == -1) {
      VERBOSE(CL_ERROR, "FILE IFC[%"PRIu32"]: Unable to retrieve current timestamp.", config->ifc_idx);
      return TRAP_E_MEMORY;
   }

   /* Get actual time a round the time based on user specified parameter */
   if (config->file_change_time > 0) {
      config->create_time -= (config->create_time % (config->file_change_time * 60));
   }

   /* Create valid path string based on the template and actual time */
   size_t len = strftime(buf, PATH_MAX - FILE_SIZE_SUFFIX_LEN, config->filename_tmplt, localtime(&config->create_time));
   if (len == 0) {
      VERBOSE(CL_ERROR, "FILE IFC[%"PRIu32"]: Path and filename exceeds maximum size: %u.", config->ifc_idx, PATH_MAX - FILE_SIZE_SUFFIX_LEN);
      return TRAP_E_BADPARAMS;
   }

   /* Recursively create specified directory and subdirectories*/
   if (_mkdir(buf) != 0) {
      VERBOSE(CL_ERROR, "FILE IFC[%"PRIu32"]: Unable to create specified directory.", config->ifc_idx);
      return TRAP_E_IO_ERROR;
   }

   /* If the user specified append mode, get the lowest possible numeric suffix for which there does not exist a file */
   if (config->mode[0] == 'a') {
      while (42) {
         if (sprintf(suffix, ".%05" PRIu16, config->file_index) < 0) {
            VERBOSE(CL_ERROR, "FILE IFC[%"PRIu32"]: sprintf failed.", config->ifc_idx);
            return TRAP_E_IO_ERROR;
         }

         memcpy(buf + len, suffix, FILE_SIZE_SUFFIX_LEN);
         buf[len + FILE_SIZE_SUFFIX_LEN] = 0;
         config->file_index++;

         /* Detected overflow */
         if (config->file_index == 0) {
            VERBOSE(CL_ERROR, "FILE IFC[%"PRIu32"]: No valid file names left.", config->ifc_idx);
            return TRAP_E_IO_ERROR;
         }

         if (access(buf, F_OK) != 0) {
            len += FILE_SIZE_SUFFIX_LEN;
            valid_suffix_present = 1;
            break;
         }
      }
   }

   /* If the user specified file splitting based on size (and suffix was not yet added due to 'append' mode) */
   if (config->file_change_size != 0 && !valid_suffix_present) {
      if (sprintf(suffix, ".%05" PRIu16, config->file_index) < 0) {
         VERBOSE(CL_ERROR, "FILE IFC[%"PRIu32"]: sprintf failed.", config->ifc_idx);
         return TRAP_E_IO_ERROR;
      }

      memcpy(buf + len, suffix, FILE_SIZE_SUFFIX_LEN);
      len += FILE_SIZE_SUFFIX_LEN;
      buf[len] = 0;
      config->file_index++;
   }

   /* Copy newly created path to context inner data structure */
   strncpy(config->filename, buf, len);
   return TRAP_E_OK;
}

/**
 * \brief Close previous file, open next file (name taken in file_private_t->filename).
 *        Negotiation must be performed after changing the file.
 *
 * \param[in,out] c Pointer to module private data.
 *
 * \return TRAP_E_OK         on success,
 *         TRAP_E_BADPARAMS  if the next file cannot be opened.
 */
int switch_file(file_private_t *c)
{
   if (c->fd != NULL) {
      fclose(c->fd);
      c->fd = NULL;
   }

   c->neg_initialized = 0;
   c->fd = fopen(c->filename, c->mode);
   if (c->fd == NULL) {
      VERBOSE(CL_ERROR, "FILE IFC[%"PRIu32"]: unable to open file \"%s\" in mode \"%c\". Possible reasons: non-existing file, bad permission, file can not be opened in this mode.", c->ifc_idx, c->filename, c->mode[0]);
      return TRAP_E_BADPARAMS;
   }

   return TRAP_E_OK;
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
 * \return 0 on success (TRAP_E_OK), TRAP_E_IO_ERROR if error occurs during reading, TRAP_E_TERMINATED if interface was terminated.
 */
int file_recv(void *priv, void *data, uint32_t *size, int timeout)
{
   size_t loaded;
   /* Header of message inside the buffer */
   uint16_t *m_head = data;
   uint32_t data_size = 0;

   file_private_t *config = (file_private_t*) priv;

   if (config->is_terminated) {
      return trap_error(config->ctx, TRAP_E_TERMINATED);
   }

   /* Check whether the file stream is open */
   if (config->fd == NULL) {
      return trap_error(config->ctx, TRAP_E_NOT_INITIALIZED);
   }

#ifdef ENABLE_NEGOTIATION
neg_start:
   if (config->neg_initialized == 0) {
      switch(input_ifc_negotiation((void *) config, TRAP_IFC_TYPE_FILE)) {
      case NEG_RES_FMT_UNKNOWN:
         VERBOSE(CL_VERBOSE_LIBRARY, "FILE INPUT IFC[%"PRIu32"] negotiation result: failed (unknown data format of the output interface).", config->ifc_idx);
         return TRAP_E_FORMAT_MISMATCH;

      case NEG_RES_CONT:
         VERBOSE(CL_VERBOSE_LIBRARY, "FILE INPUT IFC[%"PRIu32"] negotiation result: success.", config->ifc_idx);
         config->neg_initialized = 1;
         break;

      case NEG_RES_RECEIVER_FMT_SUBSET:
         VERBOSE(CL_VERBOSE_LIBRARY, "FILE INPUT IFC[%"PRIu32"] negotiation result: success (data specifier of the input interface is subset of the output interface data specifier).", config->ifc_idx);
         config->neg_initialized = 1;
         break;

      case NEG_RES_SENDER_FMT_SUBSET:
         VERBOSE(CL_VERBOSE_LIBRARY, "FILE INPUT IFC[%"PRIu32"] negotiation result: success (new data specifier of the output interface is subset of the old one; it was not first negotiation).", config->ifc_idx);
         config->neg_initialized = 1;
         break;

      case NEG_RES_FAILED:
         VERBOSE(CL_VERBOSE_LIBRARY, "FILE INPUT IFC[%"PRIu32"] negotiation result: failed (error while receiving hello message from output interface).", config->ifc_idx);
         return TRAP_E_FORMAT_MISMATCH;

      case NEG_RES_FMT_MISMATCH:
         VERBOSE(CL_VERBOSE_LIBRARY, "FILE INPUT IFC[%"PRIu32"] negotiation result: failed (data format or data specifier mismatch).", config->ifc_idx);
         return TRAP_E_FORMAT_MISMATCH;

      default:
         VERBOSE(CL_VERBOSE_LIBRARY, "FILE INPUT IFC[%"PRIu32"] negotiation result: default case.", config->ifc_idx);
         break;
      }
   }
#endif

   /* Read 4 bytes from the file, determining the length of bytes to be read to @param[out] data */
   loaded = fread(&data_size, sizeof(uint32_t), 1, config->fd);
   if (loaded != 1) {
      if (feof(config->fd)) {

         /* Test whether this was the last file */
         if (++(config->file_index) >= config->file_cnt) {
            /* Set size of buffer to the size of 1 message (including its header) */
            (*size) = 2;
            /* Set the header of message to 0B */
            *m_head = 0;

            return TRAP_E_OK;
         }

         strncpy(config->filename, config->files[config->file_index], sizeof(config->filename) - 1);
         if (switch_file(config) == TRAP_E_OK) {
#ifdef ENABLE_NEGOTIATION
            goto neg_start;
#endif
         } else {
            return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "INPUT FILE IFC[%"PRIu32"]: Unable to open next file.", config->ifc_idx);
         }
      } else {
         VERBOSE(CL_ERROR, "INPUT FILE IFC[%"PRIu32"]: Read error occurred in file: %s", config->ifc_idx, config->filename);
         return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "INPUT FILE IFC[%"PRIu32"]: Unable to read.", config->ifc_idx);
      }
   }

   *size = ntohl(data_size);
   /* Read (*size) bytes from the file */
   loaded = fread(data, 1, (*size), config->fd);
   if (loaded != (*size)) {
         VERBOSE(CL_ERROR, "INPUT FILE IFC[%"PRIu32"]: Read incorrect number of bytes from file: %s. Attempted to read %d bytes, but the actual count of bytes read was %zu.", config->ifc_idx, config->filename, (*size), loaded);
   }

   return TRAP_E_OK;
}

char *file_recv_ifc_get_id(void *priv)
{
   return ((priv) ? ((file_private_t *) priv)->filename : NULL);
}

uint8_t file_recv_ifc_is_conn(void *priv)
{
   if (priv == NULL) {
      return 0;
   }
   file_private_t *config = (file_private_t *) priv;
   if (config->fd != NULL) {
      return 1;
   }
   return 0;
}

/**
 * \brief Allocate and initiate file input interface.
 * This function is called by TRAP library to initialize one input interface.
 *
 * \param[in,out] ctx   Pointer to the private libtrap context data (trap_ctx_init()).
 * \param[in] params    Configuration string containing *file_name*,
 * where file_name is a path to a file from which data is to be read
 * \param[in,out] ifc   IFC interface used for calling file module.
 * \param[in] idx       Index of IFC that is created.
 * \return 0 on success (TRAP_E_OK), TRAP_E_MEMORY, TRAP_E_BADPARAMS on error
 */
int create_file_recv_ifc(trap_ctx_priv_t *ctx, const char *params, trap_input_ifc_t *ifc, uint32_t idx)
{
   file_private_t *priv;
   size_t name_length;
   wordexp_t files_exp;
   int i, j;

   if (params == NULL) {
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "FILE INPUT IFC[%"PRIu32"]: Parameter is null pointer.", idx);
   }

   /* Create structure to store private data */
   priv = calloc(1, sizeof(file_private_t));
   if (!priv) {
      return trap_error(ctx, TRAP_E_MEMORY);
   }

   priv->ctx = ctx;
   priv->ifc_idx = idx;
   /* Perform shell-like expansion of ~ */
   if (wordexp(params, &files_exp, 0) != 0) {
      VERBOSE(CL_ERROR, "FILE INPUT IFC[%"PRIu32"]: Unable to perform shell-like expansion of: %s", idx, params);
      free(priv);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "FILE INPUT IFC[%"PRIu32"]: Unable to perform shell-like expansion.", idx);
   }

   if (files_exp.we_wordc == 0) {
      VERBOSE(CL_ERROR, "FILE INPUT IFC[%"PRIu32"]: No files found for parameter: '%s'", idx, params);
      free(priv);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "FILE INPUT IFC[%"PRIu32"]: Unable to perform shell-like expansion.", idx);
   }

   priv->file_cnt = files_exp.we_wordc;
   priv->files = (char**) calloc(priv->file_cnt, sizeof(char*));
   if (!priv->files) {
      free(priv);
      wordfree(&files_exp);
      return trap_error(ctx, TRAP_E_MEMORY);
   }

   for (i = 0; i < priv->file_cnt; i++) {
      name_length = strlen(files_exp.we_wordv[i]);
      priv->files[i] = (char*) calloc(name_length + 1, sizeof(char));
      if (!priv->files[i]) {
         for (j = i - 1; j >= 0; j --) {
            free(priv->files[j]);
         }

         free(priv->files);
         free(priv);
         wordfree(&files_exp);
         return trap_error(ctx, TRAP_E_MEMORY);
      }

      strncpy(priv->files[i], files_exp.we_wordv[i], name_length);
   }

   wordfree(&files_exp);

   /* Check if the expanded path is not longer than supported maximum path length. */
   if (strlen(priv->files[0]) > PATH_MAX - 1) {
      VERBOSE(CL_ERROR, "FILE INPUT IFC[%"PRIu32"]: Path and filename exceeds maximum size: %u.", idx, PATH_MAX - 1);
      for (i = 0; i < priv->file_cnt; i++) {
         free(priv->files[i]);
      }

      free(priv->files);
      free(priv);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "FILE INPUT IFC[%"PRIu32"]: Path and filename exceeds maximum size.", idx);
   }

   strncpy(priv->filename, priv->files[0], PATH_MAX - 1);

   /* Sets mode and filename */
   strcpy(priv->mode, "rb");

   /* Attempts to open the file */
   priv->fd = fopen(priv->filename, priv->mode);
   if (priv->fd == NULL) {
      VERBOSE(CL_ERROR, "INPUT FILE IFC[%"PRIu32"]: unable to open file \"%s\". Possible reasons: non-existing file, bad permission.", idx, priv->filename);
      for (i = 0; i < priv->file_cnt; i++) {
         free(priv->files[i]);
      }

      free(priv->files);
      free(priv);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "INPUT FILE IFC[%"PRIu32"]: Unable to open file.", idx);
   }

   /* Fills interface structure */
   ifc->recv = file_recv;
   ifc->terminate = file_terminate;
   ifc->destroy = file_destroy;
   ifc->create_dump = file_create_dump;
   ifc->priv = priv;
   ifc->get_id = file_recv_ifc_get_id;
   ifc->is_conn = file_recv_ifc_is_conn;

   return TRAP_E_OK;
}

/**
 * @}
 *//* file_receiver */

/***** Sender *****/

void switch_file_wrapper(void *priv)
{
   file_private_t *c = (file_private_t *) priv;
   if (c && !c->is_terminated && (create_next_filename(c) == TRAP_E_OK)) {
      switch_file(c);
   }
}
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
 * \return 0 on success (TRAP_E_OK), TRAP_E_IO_ERROR if error occurs during writing, TRAP_E_TERMINATED if interface was terminated.
 */
int file_write_buffer(void *priv, const void *data, uint32_t size, int timeout)
{
   int ret_val = 0;
   file_private_t *config = (file_private_t*) priv;
   size_t written;

   if (config->is_terminated) {
      return trap_error(config->ctx, TRAP_E_TERMINATED);
   }

   /* Check whether the file stream is opened */
   if (config->fd == NULL) {
      return trap_error(config->ctx, TRAP_E_NOT_INITIALIZED);
   }

#ifdef ENABLE_NEGOTIATION
   if (config->neg_initialized == 0) {
      ret_val = output_ifc_negotiation((void *) config, TRAP_IFC_TYPE_FILE, 0);
      if (ret_val == NEG_RES_OK) {
         VERBOSE(CL_VERBOSE_LIBRARY, "FILE OUTPUT IFC[%"PRIu32"] negotiation result: success.", config->ifc_idx);
         config->neg_initialized = 1;
         fflush(config->fd);
      } else if (ret_val == NEG_RES_FMT_UNKNOWN) {
         VERBOSE(CL_VERBOSE_LIBRARY, "FILE OUTPUT IFC[%"PRIu32"] negotiation result: failed (unknown data format of this output interface -> refuse client).", config->ifc_idx);
         return trap_error(config->ctx, TRAP_E_NOT_INITIALIZED);
      } else { /* ret_val == NEG_RES_FAILED */
         VERBOSE(CL_VERBOSE_LIBRARY, "FILE OUTPUT IFC[%"PRIu32"] negotiation result: failed (error while sending hello message to input interface).", config->ifc_idx);
         return trap_error(config->ctx, TRAP_E_NOT_INITIALIZED);
      }
   }
#endif

   /* Writes data_length bytes to the file */
   written = fwrite(data, 1, size, config->fd);
   if (written != size) {
      return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "FILE OUTPUT IFC[%"PRIu32"]: unable to write to file: %s", config->ifc_idx, config->filename);
   }

   if (config->file_change_time != 0) {
      time_t current_time = time(NULL);

      /* Check whether new file should be created */
      if (difftime(current_time, config->create_time) / 60 >= config->file_change_time) {
		   config->file_index = 0;
         /* Create new filename from the current timestamp */
         int status = create_next_filename(config);
         if (status != TRAP_E_OK) {
            return trap_errorf(config->ctx, status, "FILE OUTPUT IFC[%"PRIu32"]: Error during output file creation.", config->ifc_idx);
         }

         /* Open newly created file */
         status = switch_file(config);
         if (status != TRAP_E_OK) {
            return trap_errorf(config->ctx, status, "FILE OUTPUT IFC[%"PRIu32"]: Error during output file opening.", config->ifc_idx);
         }
      }
   }

   if (config->file_change_size != 0 && (uint64_t)ftell(config->fd) >= (uint64_t)(1024 * 1024 * (uint64_t)config->file_change_size)) {

      /* Create new filename from the current timestamp */
      int status = create_next_filename(config);
      if (status != TRAP_E_OK) {
         return trap_errorf(config->ctx, status, "FILE OUTPUT IFC[%"PRIu32"]: Error during output file creation.", config->ifc_idx);
      }

      /* Open newly created file */
      status = switch_file(config);
      if (status != TRAP_E_OK) {
         return trap_errorf(config->ctx, status, "FILE OUTPUT IFC[%"PRIu32"]: Error during output file opening.", config->ifc_idx);
      }
   }

   return TRAP_E_OK;
}

static inline void finish_buffer(file_buffer_t *buffer)
{
   uint32_t header = htonl(buffer->wr_index);
   memcpy(buffer->header, &header, sizeof(header));
   buffer->finished = 1;
}

static inline void insert_into_buffer(file_buffer_t *buffer, const void *data, uint16_t size)
{
   uint16_t *msize = (uint16_t *)(buffer->data + buffer->wr_index);
   (*msize) = htons(size);
   memcpy((void *)(msize + 1), data, size);
   buffer->wr_index += (size + sizeof(size));
}

void file_flush(void *priv)
{
   int result;
   file_private_t *c = (file_private_t *) priv;
   file_buffer_t *buffer = &c->buffer;

   finish_buffer(buffer);

   result = file_write_buffer(priv, buffer->header, buffer->wr_index + sizeof(buffer->wr_index), 0);

   if (result == TRAP_E_OK) {
      __sync_add_and_fetch(&c->ctx->counter_send_buffer[c->ifc_idx], 1);

      /* Reset buffer and insert the message if it was not inserted. */
      buffer->wr_index = 0;
      buffer->finished = 0;
   } else {
      VERBOSE(CL_ERROR, "File IFC flush failed (file_write_buffer returned %i)", result);
   }
}

/**
 * \brief Store message into buffer. Write buffer into file if full. If buffering is disabled, the message is sent to the output interface immediately.
 *
 * \param[in] priv      pointer to module private data
 * \param[in] data      pointer to data to write
 * \param[in] size      size of data to write
 * \param[in] timeout   NOT USED IN THIS INTERFACE
 *
 * \return TRAP_E_OK         Success.
 * \return TRAP_E_TIMEOUT    Message was not stored into buffer and the attempt should be repeated.
 * \return TRAP_E_TERMINATED Libtrap was terminated during the process.
 */
static inline int file_send(void *priv, const void *data, uint16_t size, int timeout)
{
   int result = TRAP_E_OK;
   file_private_t *c = (file_private_t *) priv;
   file_buffer_t *buffer = &c->buffer;
   uint32_t needed_size = size + sizeof(size);
   uint32_t free_bytes = c->buffer_size - c->buffer.wr_index;
   uint8_t reinsert = 0;

   /* Can we put message at least into empty buffer? In the worst case, we could end up with SEGFAULT -> rather skip with error */
   if (needed_size > c->buffer_size) {
      return trap_errorf(c->ctx, TRAP_E_MEMORY, "Buffer is too small for this message. Skipping...");
   }

   /* Check whether the message can be stored into buffer. */
   if (buffer->finished == 0) {
      if (free_bytes >= needed_size) {
         insert_into_buffer(buffer, data, size);

         /* If bufferswitch is 0, only 1 message is allowed to be stored in buffer */
         if (c->ctx->out_ifc_list[c->ifc_idx].bufferswitch == 0) {
            finish_buffer(buffer);
         }
      } else {
         /* Need to send buffer first. */
         finish_buffer(buffer);
         reinsert = 1;
      }
   }

   /* Buffer ready to be sent. */
   if (buffer->finished == 1) {

      result = file_write_buffer(priv, buffer->header, buffer->wr_index + sizeof(buffer->wr_index), timeout);

      if (result == TRAP_E_OK) {
         __sync_add_and_fetch(&c->ctx->counter_send_buffer[c->ifc_idx], 1);

         /* Reset buffer and insert the message if it was not inserted. */
         buffer->wr_index = 0;
         buffer->finished = 0;
         if (reinsert) {
            insert_into_buffer(buffer, data, size);
         }
      }
   }

   return result;
}

int32_t file_get_client_count(void *priv)
{
   return 1;
}

int8_t file_get_client_stats_json(void *priv, json_t *client_stats_arr)
{
   /* do not collect client statistics for this interface */
   return 1;
}

char *file_send_ifc_get_id(void *priv)
{
   return ((priv) ? ((file_private_t *) priv)->filename : NULL);
}

/**
 * \brief Allocate and initiate file output interface.
 * This function is called by TRAP library to initialize one output interface.
 * \param[in,out] ctx   Pointer to the private libtrap context data (trap_ctx_init()).
 * \param[in] params    Configuration string containing colon separated values of these parameters (in this exact order): *file_name*:*open_mode*,
 * where file_name is a path to a file in which data is to be written and
 * open_mode is either a - append or w - write, if no mode is specified, the file will be opened in append mode.
 * \param[in,out] ifc   IFC interface used for calling file module.
 * \param[in] idx       Index of IFC that is created.
 * \return 0 on success (TRAP_E_OK), TRAP_E_MEMORY, TRAP_E_BADPARAMS on error
 */
int create_file_send_ifc(trap_ctx_priv_t *ctx, const char *params, trap_output_ifc_t *ifc, uint32_t idx)
{
   file_private_t *priv;
   char *dest;
   const char *params_next = NULL;
   wordexp_t exp_result;
   size_t length;

   uint32_t buffer_size = TRAP_IFC_MESSAGEQ_SIZE;

   if (params == NULL) {
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "FILE OUTPUT IFC[%"PRIu32"]: Parameter is null pointer.", idx);
   }

   /* Create structure to store private data */
   priv = calloc(1, sizeof(file_private_t));
   if (!priv) {
      return trap_error(ctx, TRAP_E_MEMORY);
   }

   priv->ctx = ctx;
   priv->ifc_idx = idx;
   priv->buffer_size = buffer_size;
   priv->buffer.header = malloc(buffer_size + sizeof(buffer_size));
   if (priv->buffer.header == NULL) {
      VERBOSE(CL_ERROR, "Memory allocation failed, terminating...");
      free(priv);
      return TRAP_E_MEMORY;
   }
   priv->buffer.data = priv->buffer.header + sizeof(buffer_size);
   priv->buffer.wr_index = 0;
   priv->buffer.finished = 0;
   /* Set default mode */
   strcpy(priv->mode, "wb");

   /* Parse file name */
   length = strcspn(params, ":");
   if (params[length] == ':') {
      params_next = params + length + 1;
   }

   if (length) {
      dest = (char*) calloc(length + 1, sizeof(char));
      if (!dest) {
         free(priv->buffer.header);
         free(priv);
         return trap_error(ctx, TRAP_E_MEMORY);
      }

      strncpy(dest, params, length);
   } else {
      free(priv->buffer.header);
      free(priv);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "FILE OUTPUT IFC[%"PRIu32"]: Filename not specified.", idx);
   }

   /* Perform shell-like expansion of ~ */
   if (wordexp(dest, &exp_result, 0) != 0) {
      VERBOSE(CL_ERROR, "FILE OUTPUT IFC[%"PRIu32"]: Unable to perform shell-like expansion of: %s", idx, dest);
      free(priv->buffer.header);
      free(priv);
      free(dest);
      wordfree(&exp_result);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "FILE OUTPUT IFC[%"PRIu32"]: Unable to perform shell-like expansion.", idx);
   }

   free(dest);
   strncpy(priv->filename_tmplt, exp_result.we_wordv[0], sizeof(priv->filename_tmplt) - 1);
   wordfree(&exp_result);

   /* Set special behavior for /dev/stdout */
   if (strncmp(priv->filename_tmplt, "/dev/stdout", 11) == 0) {
      priv->mode[0] = 'w';
      priv->file_change_size = 0;
      priv->file_change_time = 0;
   } else {
      /* Parse remaining parameters */
      while (params_next) {
         length = strcspn(params_next, ":");
         if (length > TIME_PARAM_LEN && strncmp(params_next, TIME_PARAM, TIME_PARAM_LEN) == 0) {
            priv->file_change_time = atoi(params_next + TIME_PARAM_LEN);
            if (strlen(priv->filename_tmplt) + TIME_FORMAT_STRING_LEN > sizeof(priv->filename_tmplt) - 1) {
               free(priv->buffer.header);
               free(priv);
               return trap_errorf(ctx, TRAP_E_BADPARAMS, "FILE OUTPUT IFC[%"PRIu32"]: Path and filename exceeds maximum size: %u.", idx, sizeof(priv->filename_tmplt) - 1);
            }

            /* Append timestamp formate to the current template */
            strcat(priv->filename_tmplt, TIME_FORMAT_STRING);
         } else if (length > SIZE_PARAM_LEN && strncmp(params_next, SIZE_PARAM, SIZE_PARAM_LEN) == 0) {
            priv->file_change_size = atoi(params_next + SIZE_PARAM_LEN);
         } else if (length == 1 && params_next[0] == 'a') {
            priv->mode[0] = 'a';
         }

         if (params_next[length] == '\0') {
            break;
         }

         params_next = params_next + length + 1;
      }
   }

   /* Create first filename from the prepared template */
   int status = create_next_filename(priv);
   if (status != TRAP_E_OK) {
      free(priv->buffer.header);
      free(priv);
      return trap_errorf(ctx, status, "FILE OUTPUT IFC[%"PRIu32"]: Error during output file creation.", idx);
   }

   /* Open first file */
   status = switch_file(priv);
   if (status != TRAP_E_OK) {
      free(priv->buffer.header);
      free(priv);
      return trap_errorf(ctx, status, "FILE OUTPUT IFC[%"PRIu32"]: Error during output file opening.", idx);
   }

   /* Fills interface structure */
   ifc->send = file_send;
   ifc->flush = file_flush;
   ifc->disconn_clients = switch_file_wrapper;
   ifc->terminate = file_terminate;
   ifc->destroy = file_destroy;
   ifc->get_client_count = file_get_client_count;
   ifc->get_client_stats_json = file_get_client_stats_json;
   ifc->create_dump = file_create_dump;
   ifc->priv = priv;
   ifc->get_id = file_send_ifc_get_id;

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

