/**
 * \file
 * \brief Test input interface negotiation callback.
 * \author Pavel Krobot <Pavel.Krobot@cesnet.cz>
 * \date 2018
 */
/*
 * Copyright (C) 2018 CESNET
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <getopt.h>
#include "../../include/libtrap/callbacks.h"
#include "../../include/libtrap/trap.h"
#include <unirec/unirec.h>
#include "fields.h"

UR_FIELDS(
   ipaddr SRC_IP,
   ipaddr DST_IP,
   uint16 SRC_PORT,
   uint16 DST_PORT,
   uint8 PROTOCOL,
   uint32 PACKETS,
   uint64 BYTES,
)

//const char *input_format = "ipaddr SRC_IP,ipaddr DST_IP,uint16 SRC_PORT,uint16 DST_PORT,uint8 PROTOCOL,uint32 PACKETS,uint64 BYTES";
const char *input_format = "SRC_IP,DST_IP,SRC_PORT,DST_PORT,PROTOCOL,PACKETS,BYTES";

trap_module_info_t *module_info = NULL;


#define MODULE_BASIC_INFO(BASIC) \
  BASIC("Test negotiation callbacks", "", 1, 0)


#define MODULE_PARAMS(PARAM) \
  PARAM('c', "check", "Check field.", required_argument, "string")\



static int stop = 0;

char *check_field = "NIC";
int cbk_ret = 0;

int clbk_called_test = 0;


TRAP_DEFAULT_SIGNAL_HANDLER(stop = 1)


int check_fld_presence(int negotiation_result, uint8_t req_data_type, const char *req_data_fmt,
   uint8_t recv_data_type, const char *recv_data_fmt)
{
   clbk_called_test = 666;
   printf("Result: %i\n", negotiation_result);
   printf("Req data type: %c\n", req_data_type);
   printf("Req data fmt: %s\n", req_data_fmt);
   printf("Recv data type: %c\n", recv_data_type);
   printf("Recv data fmt: %s\n", recv_data_fmt);
   if (req_data_type == TRAP_FMT_UNIREC) {
      printf("Req is OK!\n");
   } else {
      printf("Req is BAD!\n");
   }

   if (recv_data_type == TRAP_FMT_UNIREC) {
      printf("Recv is OK!\n");
   } else {
      printf("Recv is BAD!\n");
   }

   /// TODO: return value unused? - convert to void or check return value
   return TRAP_E_OK;
}


int main(int argc, char **argv)
{
   int ret;

   ur_template_t *in_tmplt = NULL;

   unsigned rcvd_rec_cnt = 0;

   /* **** TRAP initialization **** */
   INIT_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)

   TRAP_DEFAULT_INITIALIZATION(argc, argv, *module_info);

   TRAP_REGISTER_DEFAULT_SIGNAL_HANDLER();

   signed char opt;
   while ((opt = TRAP_GETOPT(argc, argv, module_getopt_string, long_options)) != -1) {
      switch (opt) {
      case 'c':
         check_field = optarg;
         break;
      default:
         fprintf(stderr, "Invalid arguments.\n");
         FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS);
         TRAP_DEFAULT_FINALIZATION();
         return -1;
      }
   }

   char* err;
   in_tmplt = ur_create_input_template(0, input_format, &err);
   if (in_tmplt == NULL) {
      fprintf(stderr, "Input UniRec template was not created (%s).\n", err);
      ret = 1;
      goto exit;
   }
   trap_clb_in_negotiation(check_fld_presence);
   trap_set_required_fmt(0, TRAP_FMT_UNIREC, "ipaddr SRC_IP");
   /* **** Main processing loop **** */
   while (!stop) {
      /* Receive record from input interface (block until data are available) */
      const void *rec;
      uint16_t rec_size;

//      ret = TRAP_RECEIVE(0, rec, rec_size, in_tmplt);
      const char *spec = NULL;
      uint8_t data_fmt;
      ret = trap_recv(0, &rec, &rec_size);
      if (ret == TRAP_E_FORMAT_CHANGED) {
         if (trap_get_data_fmt(TRAPIFC_INPUT, 0, &data_fmt, &spec) != TRAP_E_OK) {
            fprintf(stderr, "Data format was not loaded.n");
         } else {
            in_tmplt = ur_define_fields_and_update_template(spec, in_tmplt);
            if (in_tmplt == NULL) {
               fprintf(stderr, "Template could not be edited.n");
            } else {
               if (in_tmplt->direction == UR_TMPLT_DIRECTION_BI) {
                  char * spec_cpy = ur_cpy_string(spec);
                  if (spec_cpy == NULL) {
                     fprintf(stderr, "Memory allocation problem.n");
                  } else {
                     trap_set_data_fmt(in_tmplt->ifc_out, TRAP_FMT_UNIREC, spec_cpy);
                  }
               }
            }
         }
      }
      TRAP_DEFAULT_RECV_ERROR_HANDLING(ret, continue, break);

      // Check size of received data
      if (rec_size < ur_rec_fixlen_size(in_tmplt)) {
         if (rec_size <= 1) {
            printf("Received ending record, terminating.\n");
            break; // End of data (used for testing purposes)
         } else {
            fprintf(stderr, "Error: data with wrong size received.");
            break;
         }
      }

      ++rcvd_rec_cnt;
   }

   printf("Received %i records.\n", rcvd_rec_cnt);
   printf("clbk_called_test is: %i.\n", clbk_called_test);

   ret = 0;

   /* **** Cleanup **** */
exit:
   // Do all necessary cleanup in libtrap before exiting
   TRAP_DEFAULT_FINALIZATION();

   // Release allocated memory for module_info structure
   FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)

   // Free unirec templates and output record
   ur_free_template(in_tmplt);
   ur_finalize();

   return ret;
}
