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
#include <wordexp.h>
#include <unistd.h>

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
   file_private_t *config = (file_private_t*) priv;

   if (config) {
      if (config->fd) {
         fclose(config->fd);
      }
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

int open_next_file(file_private_t *c)
{
   char *buffer = NULL;

   if (c == NULL) {
      VERBOSE(CL_ERROR, "FILE IFC: pointer to inner data structure is NULL.");
      return -1;
   }

   if (c->fd != NULL) {
      fclose(c->fd);
      c->fd = NULL;
   }

   c->neg_initialized = 0;

   if (asprintf(&buffer, "%s%d", c->filename, c->file_cnt) < 0) {
      VERBOSE(CL_ERROR, "FILE IFC: asprintf failed.");
      return -1;
   }

   c->file_cnt++;
   c->fd = fopen(buffer, c->mode);
   if (c->fd == NULL) {
      if (c->mode[0] == 'r' && access(buffer, F_OK) == -1) {
         free(buffer);
         return -2;
      }

      VERBOSE(CL_ERROR, "FILE IFC %d: could not open a new file: %s after changing data format.", c->ifc_idx, buffer);
      free(buffer);
      return -1;
   }

   free(buffer);
   return 0;
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
   long int current_position, remaining_bytes;

   /* header of message inside the buffer */
   uint16_t *m_head = data;

   file_private_t *config = (file_private_t*) priv;

   if (config->is_terminated) {
      return trap_error(config->ctx, TRAP_E_TERMINATED);
   }

   /* Check whether the file stream is opened */
   if (config->fd == NULL) {
      return trap_error(config->ctx, TRAP_E_NOT_INITIALIZED);
   }

#ifdef ENABLE_NEGOTIATION
neg_start:
   if (config->neg_initialized == 0) {
      switch(input_ifc_negotiation((void *) config, TRAP_IFC_TYPE_FILE)) {
      case NEG_RES_FMT_UNKNOWN:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: failed (unknown data format of the output interface).");
         return TRAP_E_FORMAT_MISMATCH;

      case NEG_RES_CONT:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success.");
         config->neg_initialized = 1;
         break;

      case NEG_RES_RECEIVER_FMT_SUBSET:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success (data specifier of the input interface is subset of the output interface data specifier).");
         config->neg_initialized = 1;
         break;

      case NEG_RES_SENDER_FMT_SUBSET:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: success (new data specifier of the output interface is subset of the old one; it was not first negotiation).");
         config->neg_initialized = 1;
         break;

      case NEG_RES_FAILED:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: failed (error while receiving hello message from output interface).");
         // If the negotiation fails, try a next file.. If it fails again, return from receive
         if (feof(config->fd)) {
            if (open_next_file(config) == 0) {
               goto neg_start;
            }
         }
         return TRAP_E_FORMAT_MISMATCH;

      case NEG_RES_FMT_MISMATCH:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: failed (data format or data specifier mismatch).");
         return TRAP_E_FORMAT_MISMATCH;

      default:
         VERBOSE(CL_VERBOSE_LIBRARY, "Input_ifc_negotiation result: default case");
         break;
      }
   }
#endif

   /* Reads 4 bytes from the file, determining the length of bytes to be read to @param[out] data */
   loaded = fread(size, sizeof(uint32_t), 1, config->fd);

   if (loaded != 1) {
      if (feof(config->fd)) {
#ifdef ENABLE_NEGOTIATION
         if (open_next_file(config) == 0) {
            goto neg_start;
         } else {
            VERBOSE(CL_VERBOSE_LIBRARY, "File input ifc negotiation: eof, could not open next input file.")
         }
#endif
         /* set size of buffer to the size of 1 message (including its header) */
         (*size) = 2;
         /* set the header of message to 0B */
         *m_head = 0;

         return TRAP_E_OK;
      }

      VERBOSE(CL_ERROR, "INPUT FILE IFC: read error occurred in file: %s", config->filename);
      return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "INPUT FILE IFC: unable to read");
   }

   current_position = ftell(config->fd);
   if (current_position < 0) {
      VERBOSE(CL_ERROR, "INPUT FILE IFC: ftell failed in file: %s", config->filename);
      return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "INPUT FILE IFC: ftell failed");
   }

   if (fseek(config->fd, 0L, SEEK_END) < 0) {
      VERBOSE(CL_ERROR, "INPUT FILE IFC: fseek failed in file: %s", config->filename);
      return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "INPUT FILE IFC: fseek failed");
   }

   remaining_bytes = ftell(config->fd);
   if (remaining_bytes < 0) {
      VERBOSE(CL_ERROR, "INPUT FILE IFC: ftell failed in file: %s", config->filename);
      return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "INPUT FILE IFC: ftell failed");
   }

   remaining_bytes -= current_position;
   if (fseek(config->fd, current_position, SEEK_SET) < 0) {
      VERBOSE(CL_ERROR, "INPUT FILE IFC: fseek failed in file: %s", config->filename);
      return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "INPUT FILE IFC: fseek failed");
   }

   /* Reads (*size) bytes from the file */
   if (remaining_bytes < (*size)) {
      VERBOSE(CL_ERROR, "INPUT FILE IFC: Attempting to read %"PRIu32" bytes from file: %s, but there are only %ld bytes remaining. Read %ld bytes instead.", (*size), config->filename, remaining_bytes, remaining_bytes);
      loaded = fread(data, 1, remaining_bytes, config->fd);

      if (loaded != remaining_bytes) {
         VERBOSE(CL_ERROR, "INPUT FILE IFC: loaded incorrect number of bytes from file: %s", config->filename);
         return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "INPUT FILE IFC: fread failed");
      }
   } else {
      loaded = fread(data, 1, (*size), config->fd);

      if (loaded != (*size)) {
         VERBOSE(CL_ERROR, "INPUT FILE IFC: loaded incorrect number of bytes from file: %s", config->filename);
         return trap_errorf(config->ctx, TRAP_E_IO_ERROR, "INPUT FILE IFC: fread failed");         
      }
   }


   return TRAP_E_OK;
}

char *file_recv_ifc_get_id(void *priv)
{
   if (priv == NULL) {
      return NULL;
   }

   file_private_t *config = (file_private_t *) priv;
   if (config->filename == NULL) {
      return NULL;
   }
   return config->filename;
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
   wordexp_t exp_result;

   if (params == NULL) {
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "parameter is null pointer");
   }

   /* Perform shell-like expansion of ~ */
   if (wordexp(params, &exp_result, 0) != 0) {
      VERBOSE(CL_ERROR, "CREATE INPUT FILE IFC: unable to perform shell-like expand of: %s", params);
      wordfree(&exp_result);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "CREATE INPUT FILE IFC: unable to perform shell-like expand");
   }

   name_length = strlen(exp_result.we_wordv[0]);

   /* Create structure to store private data */
   priv = calloc(1, sizeof(file_private_t));
   if (!priv) {
      wordfree(&exp_result);
      return trap_error(ctx, TRAP_E_MEMORY);
   }

   priv->ctx = ctx;
   priv->ifc_idx = idx;
   priv->filename = (char *) calloc(name_length + 1, sizeof(char));
   if (!priv->filename) {
      free(priv);
      wordfree(&exp_result);
      return trap_error(ctx, TRAP_E_MEMORY);
   }

   /* Sets mode and filename */
   priv->mode[0] = 'r';
   priv->mode[1] = 'b';
   priv->mode[2] = '\0';
   strncpy(priv->filename, exp_result.we_wordv[0], name_length + 1);
   wordfree(&exp_result);

   /* Attempts to open the file */
   priv->fd = fopen(priv->filename, priv->mode);
   if (priv->fd == NULL) {
      VERBOSE(CL_ERROR, "CREATE INPUT FILE IFC: unable to open file \"%s\". Possible reasons: non-existing file, bad permission.", priv->filename);
      free(priv->filename);
      free(priv);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "unable to open file");
   }

   priv->is_terminated = 0;

   /* Fills interface structure */
   ifc->recv = file_recv;
   ifc->terminate = file_terminate;
   ifc->destroy = file_destroy;
   ifc->create_dump = file_create_dump;
   ifc->priv = priv;
   ifc->get_id = file_recv_ifc_get_id;

   return TRAP_E_OK;
}

/**
 * @}
 *//* file_receiver */

/***** Sender *****/

void create_next_file(void *priv)
{
   file_private_t *c = (file_private_t *) priv;
   if (open_next_file(c) != 0) {
      VERBOSE(CL_ERROR, "OUTPUT FILE IFC %d: creating and opening of a new file failed.", c->ifc_idx);
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
 * \return 0 on success (TRAP_E_OK), TTRAP_E_IO_ERROR if error occurs during writing, TRAP_E_TERMINATED if interface was terminated.
 */
int file_send(void *priv, const void *data, uint32_t size, int timeout)
{
   int ret_val = 0;
   file_private_t *config = (file_private_t*) priv;
   const trap_buffer_header_t *data_struct = (trap_buffer_header_t *) data;
   size_t written;
   uint32_t size_little_e;

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
         VERBOSE(CL_VERBOSE_LIBRARY, "File output_ifc_negotiation result: success.");
         config->neg_initialized = 1;
         fflush(config->fd);
      } else if (ret_val == NEG_RES_FMT_UNKNOWN) {
         VERBOSE(CL_VERBOSE_LIBRARY, "File output_ifc_negotiation result: failed (unknown data format of this output interface -> refuse client).");
         return trap_error(config->ctx, TRAP_E_NOT_INITIALIZED);
      } else { // ret_val == NEG_RES_FAILED
         VERBOSE(CL_VERBOSE_LIBRARY, "File output_ifc_negotiation result: failed (error while sending hello message to input interface).");
         return trap_error(config->ctx, TRAP_E_NOT_INITIALIZED);
      }
   }
#endif

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

char *file_send_ifc_get_id(void *priv)
{
   if (priv == NULL) {
      return NULL;
   }

   file_private_t *config = (file_private_t *) priv;
   if (config->filename == NULL) {
      return NULL;
   }
   return config->filename;
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
   char *dest, *dest2, *ret;
   wordexp_t exp_result;
   size_t name_length;

   if (params == NULL) {
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "parameter is null pointer");
   }

   /* Create structure to store private data */
   priv = calloc(1, sizeof(file_private_t));
   if (!priv) {
      return trap_error(ctx, TRAP_E_MEMORY);
   }

   priv->ctx = ctx;
   priv->ifc_idx = idx;
   /* Parses and sets filename and mode */
   priv->filename = dest = dest2 = NULL;
   ret = trap_get_param_by_delimiter(params, &dest, ':');

   if (!ret) {
      if (dest) {
         priv->mode[0] = 'a';
      } else {
         free(priv);
         return trap_error(ctx, TRAP_E_MEMORY);
      }
   } else {
      trap_get_param_by_delimiter(ret, &dest2, ':');
      if (!dest2) {
         free(priv);
         free(dest);
         return trap_error(ctx, TRAP_E_MEMORY);
      }

      if (dest2[0] != 'a' && dest2[0] != 'w') {
         VERBOSE(CL_ERROR, "OUTPUT FILE IFC: bad mode: %c", dest2[0]);
         free(priv);
         free(dest);
         free(dest2);
         return trap_errorf(ctx, TRAP_E_BADPARAMS, "OUTPUT FILE IFC: bad mode");
      }

      priv->mode[0] = dest2[0];
   }

   priv->mode[1] = 'b';
   priv->mode[2] = '\0';

   free(dest2);

   /* Perform shell-like expansion of ~ */
   if (wordexp(dest, &exp_result, 0) != 0) {
      VERBOSE(CL_ERROR, "CREATE OUTPUT FILE IFC: unable to perform shell-like expand of: %s", dest);
      free(priv);
      free(dest);
      wordfree(&exp_result);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "CREATE OUTPUT FILE IFC: unable to perform shell-like expand");
   }

   free(dest);
   name_length = strlen(exp_result.we_wordv[0]);

   priv->filename = (char *) calloc(name_length + 1, sizeof(char));
   if (!priv->filename) {
      free(priv);
      wordfree(&exp_result);
      return trap_error(ctx, TRAP_E_MEMORY);
   }

   strncpy(priv->filename, exp_result.we_wordv[0], name_length + 1);
   wordfree(&exp_result);

   if (priv->mode[0] == 'a' && access(priv->filename, F_OK) != -1) {
      char *buffer = NULL;
      do{
         if (buffer != NULL) {
            free(buffer);
            buffer = NULL;
         }

         if (asprintf(&buffer, "%s%d", priv->filename, priv->file_cnt) < 0) {
            VERBOSE(CL_ERROR, "CREATE OUTPUT FILE IFC: asprintf failed.");
            free(priv->filename);
            free(priv);
            return trap_error(ctx, TRAP_E_MEMORY);
         }

         priv->file_cnt++;
      } while (access(buffer, F_OK) != -1);

      priv->fd = fopen(buffer, priv->mode);
      free(buffer);
   } else {
      /* Attempts to open the file */
      priv->fd = fopen(priv->filename, priv->mode);
   }

   if (priv->fd == NULL) {
      VERBOSE(CL_ERROR, "CREATE OUTPUT FILE IFC : unable to open file \"%s\" in mode \"%c\". Possible reasons: non-existing file, bad permission, file can not be opened in this mode.", priv->filename, priv->mode[0]);
      free(priv->filename);
      free(priv);
      return trap_errorf(ctx, TRAP_E_BADPARAMS, "unable to open file");
   }

   priv->is_terminated = 0;

   /* Fills interface structure */
   ifc->send = file_send;
   ifc->disconn_clients = create_next_file;
   ifc->terminate = file_terminate;
   ifc->destroy = file_destroy;
   ifc->get_client_count = file_get_client_count;
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

