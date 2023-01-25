/**
 * \file 
 * \brief TRAP service interfaces
 * \author Pavel Siska <siska@cesnet.cz>
 * \date 2023
 */
/*
 * Copyright (C) 2023 CESNET
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
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <assert.h>
#include <poll.h>

#include "../include/libtrap/trap.h"
#include "trap_internal.h"
#include "trap_mbuf.h"
#include "trap_ifc.h"
#include "trap_error.h"
#include "ifc_tcpip.h"
#include "ifc_service.h"
#include "ifc_service_internal.h"
#include "ifc_socket_common.h"

/**
 * Unix sockets for service IFC and UNIX IFC have default path format defined by UNIX_PATH_FILENAME_FORMAT
 */
char *trap_default_socket_path_format __attribute__((used)) = UNIX_PATH_FILENAME_FORMAT;

/**
 * \brief Set service interface state as terminated.
 * \param[in] priv  pointer to module private data
 */
static void tcpip_sender_service_terminate(void *priv)
{
   service_private_t *c = (service_private_t *) priv;

   if (c != NULL) {
      c->is_terminated = 1;
      close(c->term_pipe[1]);
      VERBOSE(CL_VERBOSE_LIBRARY, "Closed term_pipe, it should break poll()");
   } else {
      VERBOSE(CL_ERROR, "Destroying IFC that is probably not initialized.");
   }
   return;
}

/**
 * \brief Destructor of TCP sender (output ifc)
 * \param[in] priv  pointer to module private data
 */
static void tcpip_sender_service_destroy(void *priv)
{
   service_private_t *c = (service_private_t *) priv;
   char *unix_socket_path = NULL;
   int32_t i;


#define X(x) free(x); x = NULL;
   // Free private data
   if (c != NULL) {
      if (asprintf(&unix_socket_path, trap_default_socket_path_format, c->server_port) != -1) {
         if (unix_socket_path != NULL) {
            unlink(unix_socket_path);
            X(unix_socket_path);
         }
      }
      if (c->server_port != NULL) {
         X(c->server_port);
      }

      /* close server socket */
      close(c->server_sd);

      /* disconnect all clients */
      if (c->clients != NULL) {
         for (i = 0; i < c->max_clients; i++) {
            if (c->clients[i].sd != -1) {
               shutdown(c->clients[i].sd, SHUT_RDWR);
               close(c->clients[i].sd);
            }
         }
         X(c->clients);
      }

      X(c)
   }
#undef X
}

static int server_service_socket_open(void *priv)
{
   struct sockaddr_un unix_addr; 
   struct addrinfo *p = NULL;
   service_private_t *c = (service_private_t *) priv;
   if (c->server_port == NULL) {
      return TRAP_E_BAD_FPARAMS;
   }

   memset(&unix_addr, 0, sizeof(unix_addr));
   
   /* UNIX socket */
   unix_addr.sun_family = AF_UNIX;
   snprintf(unix_addr.sun_path, sizeof(unix_addr.sun_path) - 1, trap_default_socket_path_format, c->server_port);
   /* if socket file exists, it could be hard to create new socket and bind */
   unlink(unix_addr.sun_path); /* error when file does not exist is not a problem */
   c->server_sd = socket(AF_UNIX, SOCK_STREAM, 0);
   if (c->server_sd != -1) {
      if (bind(c->server_sd, (struct sockaddr *) &unix_addr, sizeof(unix_addr)) != -1) {
         p = (struct addrinfo *) &unix_addr;
         if (chmod(unix_addr.sun_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == -1) {
            VERBOSE(CL_ERROR, "Failed to set permissions to socket (%s).", unix_addr.sun_path);
         }
      } else {
         /* error bind() failed */
         p = NULL;
         VERBOSE(CL_ERROR, "Failed bind() with the following socket path: %s", unix_addr.sun_path);
      }
   } else {
      VERBOSE(CL_ERROR, "Failed to create socket.");
      p = NULL;
   }

   if (p == NULL) {
      // if we got here, it means we didn't get bound
      VERBOSE(CL_VERBOSE_LIBRARY, "selectserver: failed to bind");
      return TRAP_E_IO_ERROR;
   }

   // listen
   if (listen(c->server_sd, c->max_clients) == -1) {
      VERBOSE(CL_ERROR, "Listen failed");
      return TRAP_E_IO_ERROR;
   }

   c->initialized = 1;
   return 0;
}

int create_service_sender_ifc(const char *params, trap_output_ifc_t *ifc)
{
   int result = TRAP_E_OK;
   char *server_port = NULL;
   service_private_t *priv = NULL;
   unsigned int max_clients = DEFAULT_MAX_CLIENTS;
   uint32_t i;

#define X(pointer) free(pointer); \
   pointer = NULL;

   // Check parameter
   if (params == NULL) {
      VERBOSE(CL_ERROR, "IFC requires at least one parameter (UNIX socket name).");
      return TRAP_E_BADPARAMS;
   }

   // Create structure to store private data
   priv = (service_private_t *) calloc(1, sizeof(service_private_t));
   if (priv == NULL) {
      result = TRAP_E_MEMORY;
      goto failsafe_cleanup;
   }

   /* Parsing params */
   trap_get_param_by_delimiter(params, &server_port, TRAP_IFC_PARAM_DELIMITER);
   if ((server_port == NULL) || (strlen(server_port) == 0)) {
      VERBOSE(CL_ERROR, "Missing 'port' for UNIX socket IFC.");
      result = TRAP_E_BADPARAMS;
      goto failsafe_cleanup;
   }

   priv->clients = calloc(max_clients, sizeof(service_client_t));
   if (priv->clients == NULL) {
      /* if some memory could not have been allocated, we cannot continue */
      result = TRAP_E_MEMORY;
      goto failsafe_cleanup;
   }
   for (i = 0; i < max_clients; ++i) {
      service_client_t *client = &(priv->clients[i]);
      client->sd = -1;
   }

   priv->server_port = server_port;
   priv->max_clients = max_clients;
   priv->is_terminated = 0;

   VERBOSE(CL_VERBOSE_ADVANCED, "config service:\nserver_port:\t%s\nmax_clients:\t%u\n", priv->server_port, priv->max_clients);

   result = server_service_socket_open(priv);
   if (result != TRAP_E_OK) {
      VERBOSE(CL_ERROR, "Socket could not be opened on given port '%s'.", server_port);
      goto failsafe_cleanup;
   }

   if (pipe(priv->term_pipe) != 0) {
      VERBOSE(CL_ERROR, "Opening of pipe failed. Using stdin as a fall back.");
      priv->term_pipe[0] = 0;
   }

   // Fill struct defining the interface
   ifc->terminate = tcpip_sender_service_terminate;
   ifc->destroy = tcpip_sender_service_destroy;
   ifc->priv = priv;
   
   return result;

failsafe_cleanup:
   X(server_port);
   if (priv != NULL) {
      if (priv->clients != NULL) {
         X(priv->clients);
      }
      X(priv);
   }
#undef X
   return result;
}