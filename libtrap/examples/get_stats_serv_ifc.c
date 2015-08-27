/**
 * \file get_stats_serv_ifc.c
 * \brief Simple program which connects to service interface of a nemea module and receives module statistics (interface counters - send, received messages of every interface etc.).
 * \author Marek Svepes <svepemar@fit.cvut.cz>
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

#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <getopt.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>

#include "../include/libtrap/trap.h"

#define MODULES_UNIXSOCKET_PATH_FILENAME_FORMAT   "/tmp/trap-localhost-%s.sock" ///< Modules output interfaces socket, to which connects service thread.

#define SERVICE_GET_COM 10
#define SERVICE_SET_COM 11
#define SERVICE_OK_REPLY 12

typedef struct service_msg_header_s {
   uint8_t com;
   uint32_t data_size;
} service_msg_header_t;

union tcpip_socket_addr {
   struct addrinfo tcpip_addr; ///< used for TCPIP socket
   struct sockaddr_un unix_addr; ///< used for path of UNIX socket
};

typedef struct in_ifc_stats_s {
   uint64_t recv_msg_cnt;
   uint64_t recv_buffer_cnt;
} in_ifc_stats_t;

typedef struct out_ifc_stats_s {
   uint64_t sent_msg_cnt;
   uint64_t sent_buffer_cnt;
   uint64_t autoflush_cnt;
} out_ifc_stats_t;

/**************************/

uint8_t prog_terminated = 0;
int sd = -1;
char * dest_port = NULL;

in_ifc_stats_t *in_ifc_stats = NULL;
out_ifc_stats_t *out_ifc_stats = NULL;

int module_num_out_ifc = 0;
int module_num_in_ifc = 0;

int connect_to_module_service_ifc()
{
   union tcpip_socket_addr addr;

   memset(&addr, 0, sizeof(addr));

   addr.unix_addr.sun_family = AF_UNIX;
   snprintf(addr.unix_addr.sun_path, sizeof(addr.unix_addr.sun_path) - 1, MODULES_UNIXSOCKET_PATH_FILENAME_FORMAT, dest_port);
   sd = socket(AF_UNIX, SOCK_STREAM, 0);
   if (sd == -1) {
         return -1;
   }

   if (connect(sd, (struct sockaddr *) &addr.unix_addr, sizeof(addr.unix_addr)) == -1) {
         close(sd);
         sd = -1;
         return -1;
   } else {
      return 0;
   }
}

int decode_cnts_from_json(char **data)
{
   size_t arr_idx = 0;

   uint32_t in_ifc_cnt = 0, out_ifc_cnt = 0;

   json_error_t error;
   json_t *json_struct = NULL;

   json_t *in_ifces_arr = NULL;
   json_t *out_ifces_arr = NULL;
   json_t *in_ifc_cnts  = NULL;
   json_t *out_ifc_cnts = NULL;
   json_t *cnt = NULL;

   /***********************************/

   // Parse received modules counters in json format
   json_struct = json_loads(*data , 0, &error);
    if (json_struct == NULL) {
        printf("[ERROR] Could not convert modules stats to json structure on line %d: %s\n", error.line, error.text);
        return -1;
    }

    // Check whether the root elem is a json object
    if (json_is_object(json_struct) == 0) {
      printf("[ERROR] Root elem is not a json object.\n");
      json_decref(json_struct);
      return -1;
    }


    if (module_num_in_ifc > 0) {
      // Get value of the key "in" from json root elem (it should be an array of json objects - every object contains counters of one input interface)
      in_ifces_arr = json_object_get(json_struct, "in");
      if (in_ifces_arr == NULL) {
         printf("[ERROR] Could not get key \"in\" from root json object while parsing modules stats.\n");
         json_decref(json_struct);
         return -1;
      }

      if (json_is_array(in_ifces_arr) == 0) {
         printf("[ERROR] Value of key \"in\" is not a json array.\n");
         json_decref(json_struct);
         return -1;
      }

      json_array_foreach(in_ifces_arr, arr_idx, in_ifc_cnts) {
         if (json_is_object(in_ifc_cnts) == 0) {
            printf("[ERROR] Counters of an input interface are not a json object in received json structure.\n");
            json_decref(json_struct);
            return -1;
         }

         cnt = json_object_get(in_ifc_cnts, "messages");
         if (cnt == NULL) {
            printf("[ERROR] Could not get key \"%s\" from an input interface json object.\n", "messages");
            json_decref(json_struct);
            return -1;
         }
         in_ifc_stats[in_ifc_cnt].recv_msg_cnt = json_integer_value(cnt);

         cnt = json_object_get(in_ifc_cnts, "buffers");
         if (cnt == NULL) {
            printf("[ERROR] Could not get key \"%s\" from an input interface json object.\n", "buffers");
            json_decref(json_struct);
            return -1;
         }
         in_ifc_stats[in_ifc_cnt].recv_buffer_cnt = json_integer_value(cnt);

         in_ifc_cnt++;
      }
   }


   if (module_num_out_ifc > 0) {
      // Get value of the key "out" from json root elem (it should be an array of json objects - every object contains counters of one output interface)
      out_ifces_arr = json_object_get(json_struct, "out");
      if (out_ifces_arr == NULL) {
         printf("[ERROR] Could not get key \"out\" from root json object while parsing modules stats.\n");
         json_decref(json_struct);
         return -1;
      }

      if (json_is_array(out_ifces_arr) == 0) {
         printf("[ERROR] Value of key \"out\" is not a json array.\n");
         json_decref(json_struct);
         return -1;
      }

      json_array_foreach(out_ifces_arr, arr_idx, out_ifc_cnts) {
         if (json_is_object(out_ifc_cnts) == 0) {
            printf("[ERROR] Counters of an output interface are not a json object in received json structure.\n");
            json_decref(json_struct);
            return -1;
         }

         cnt = json_object_get(out_ifc_cnts, "messages");
         if (cnt == NULL) {
            printf("[ERROR] Could not get key \"%s\" from an output interface json object.\n", "messages");
            json_decref(json_struct);
            return -1;
         }
         out_ifc_stats[out_ifc_cnt].sent_msg_cnt = json_integer_value(cnt);

         cnt = json_object_get(out_ifc_cnts, "buffers");
         if (cnt == NULL) {
            printf("[ERROR] Could not get key \"%s\" from an output interface json object.\n", "buffers");
            json_decref(json_struct);
            return -1;
         }
         out_ifc_stats[out_ifc_cnt].sent_buffer_cnt = json_integer_value(cnt);

         cnt = json_object_get(out_ifc_cnts, "autoflushes");
         if (cnt == NULL) {
            printf("[ERROR] Could not get key \"%s\" from an output interface json object.\n", "autoflushes");
            json_decref(json_struct);
            return -1;
         }
         out_ifc_stats[out_ifc_cnt].autoflush_cnt = json_integer_value(cnt);

         out_ifc_cnt++;
      }
   }

   json_decref(json_struct);
   return 0;
}



int service_recv_data(uint32_t size, void **data)
{
   int num_of_timeouts = 0;
   int total_receved = 0;
   int last_receved = 0;

   while (total_receved < size) {
      last_receved = recv(sd, (*data) + total_receved, size - total_receved, MSG_DONTWAIT);
      if (last_receved == 0) {
         printf("Modules service thread closed its socket, im done !\n");
         return -1;
      } else if (last_receved == -1) {
         if (errno == EAGAIN  || errno == EWOULDBLOCK) {
            num_of_timeouts++;
            if (num_of_timeouts >= 3) {
               return -1;
            } else {
               usleep(25000);
               continue;
            }
         }
         printf("[SERVICE] Error while receiving from module!\n");
         return -1;
      }
      total_receved += last_receved;
   }
   return 0;
}

int service_send_data(uint32_t size, void **data)
{
   int num_of_timeouts = 0, total_sent = 0, last_sent = 0;

   while (total_sent < size) {
      last_sent = send(sd, (*data) + total_sent, size - total_sent, MSG_DONTWAIT);
      if (last_sent == -1) {
         if (errno == EAGAIN  || errno == EWOULDBLOCK) {
            num_of_timeouts++;
            if (num_of_timeouts >= 3) {
               return -1;
            } else {
               usleep(25000);
               continue;
            }
         }
         printf("[SERVICE] Error while sending to module!\n");
         return -1;
      }
      total_sent += last_sent;
   }
   return 0;
}


void print_statistics()
{
   unsigned int x = 0;

   if (module_num_in_ifc > 0) {
      printf("Input interfaces:\n");
      for (x = 0; x < module_num_in_ifc; x++) {
         printf("\tIFC %d> RM: %" PRIu64 ", RB: %" PRIu64 "\n", x, in_ifc_stats[x].recv_msg_cnt, in_ifc_stats[x].recv_buffer_cnt);
      }
   }

   if (module_num_out_ifc > 0) {
      printf("Output interfaces:\n");
      for (x = 0; x < module_num_out_ifc; x++) {
         printf("\tIFC %d> SM: %" PRIu64 ", SB: %" PRIu64 ", AF: %" PRIu64 "\n", x, out_ifc_stats[x].sent_msg_cnt, out_ifc_stats[x].sent_buffer_cnt, out_ifc_stats[x].autoflush_cnt);
      }
   }
}

void signal_handler(int catched_signal)
{
   if (catched_signal == SIGINT) {
      printf("[SIGNAL HANDLER] SIGINT caught -> gonna terminate!\n");
      prog_terminated = 1;
   }
}

int main (int argc, char **argv)
{
   // Set up signal handler to catch SIGINT which terminates the program
   struct sigaction sig_action;
   sig_action.sa_handler = signal_handler;
   sig_action.sa_flags = 0;
   sigemptyset(&sig_action.sa_mask);

   if (sigaction(SIGINT, &sig_action, NULL) == -1) {
      printf("[ERROR] Sigaction: signal handler won't catch SIGINT !\n");
   }

   char c = 0;
   int original_argc = argc;

   // Parse program arguments
   while (1) {
      c = getopt(argc, argv, "hi:o:s:");
      if (c == -1) {
         break;
      }

      switch (c) {
      case 'h':
         printf("Usage:  ./program  -s service_socket_name  -i number_input_ifces  -o number_output_ifces\n(note: service_socket_name is the string from -i specifier of the running module, -i s:module_serv_sock)\n");
         return 0;
      case 'i':
         module_num_in_ifc = atoi(optarg);
         break;
      case 'o':
         module_num_out_ifc = atoi(optarg);
         break;
      case 's':
         dest_port = strdup(optarg);
         break;
      }
   }

   if (original_argc != 7) {
      printf("Usage:  ./program  -s service_socket_name  -i number_input_ifces  -o number_output_ifces\n(note: service_socket_name is the string from -i specifier of the running module, -i s:module_serv_sock)\n");
      return 0;
   } else {
      printf("\x1b[31;1m""Use Control+C to stop me...\n""\x1b[0m");
      printf("Legend:\n\tRM (received messages)\n\tRB (received buffers)\n\tSM (sent messages)\n\tSB (sent buffers)\n\tAF (autoflushes counter)\n- - - - - - - - - - - - - - - - -\n");
   }

   uint32_t buffer_size = 256;
   char * buffer = (char *) calloc(buffer_size, sizeof(char));
   service_msg_header_t *header = (service_msg_header_t *) calloc(1, sizeof(service_msg_header_t));

   in_ifc_stats = (in_ifc_stats_t *) calloc(module_num_in_ifc, sizeof(in_ifc_stats_t));
   out_ifc_stats = (out_ifc_stats_t *) calloc(module_num_out_ifc, sizeof(out_ifc_stats_t));

   // Connect to modules service interface
   if (connect_to_module_service_ifc() == -1) {
      printf("Could not connect to service ifc.\n");
      return 0;
   }

   while (prog_terminated == 0) {

      // Set request header
      header->com = SERVICE_GET_COM;
      header->data_size = 0;

      // Send request for modules stats
      if (service_send_data(sizeof(service_msg_header_t), (void **) &header) == -1) {
         printf("[SERVICE] Error while sending request to module.\n");
         break;
      }


      // Receive reply header
      if (service_recv_data(sizeof(service_msg_header_t), (void **) &header) == -1) {
         printf("[SERVICE] Error while receiving reply header from module.\n");
         break;
      }

      // Check if the reply is OK
      if (header->com != SERVICE_OK_REPLY) {
         printf("[SERVICE] Wrong reply from module.\n");
         break;
      }

      memset(buffer, 0, buffer_size);
      if (header->data_size > buffer_size) {
         // Reallocate buffer for incoming data
         buffer_size += buffer_size / 2;
         buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
         memset(buffer + (2 * (buffer_size / 3)), 0, (buffer_size / 3) * sizeof(char));
      }

      // Receive module stats in json format
      if (service_recv_data(header->data_size, (void **) &buffer) == -1) {
         printf( "[SERVICE] Error while receiving stats from module.\n");
         break;
      }

      // Decode json and save stats into structures
      if (decode_cnts_from_json(&buffer) == -1) {
         printf( "[SERVICE] Error while receiving stats from module.\n");
         break;
      }

      // Print stats
      print_statistics();
      printf("--- 3 seconds waiting ---\n");
      sleep(3);
   }

   if (buffer != NULL) {
      free(buffer);
      buffer = NULL;
   }

   if (in_ifc_stats != NULL) {
      free(in_ifc_stats);
      in_ifc_stats = NULL;
   }

   if (out_ifc_stats != NULL) {
      free(out_ifc_stats);
      out_ifc_stats = NULL;
   }

   if (header != NULL) {
      free(header);
      header = NULL;
   }

   if (dest_port != NULL) {
      free(dest_port);
      dest_port = NULL;
   }

   return 0;
}