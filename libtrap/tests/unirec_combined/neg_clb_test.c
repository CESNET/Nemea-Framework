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
#include "libtrap/trap.h"
#include "unirec.h"
#include "fields.h"

/// TODO remove after solving in configure.ac
#define TRAP_GETOPT(argc, argv, optstr, longopts) getopt_long(argc, argv, optstr, longopts, NULL)

trap_module_info_t *module_info = NULL;


#define MODULE_BASIC_INFO(BASIC) \
  BASIC("Test negotiation callbacks", \
  "This module test negotiation callback. If called with no arguments, no callback with no data is set \
  (i.e. NULL values for both). If argument \"-c <field_name>\" is used, field of <field_name> name is\
  searched in a received template.",\
  1, 0)


#define MODULE_PARAMS(PARAM) \
  PARAM('c', "check-field", "Check field.", required_argument, "string")


static int stop = 0;


TRAP_DEFAULT_SIGNAL_HANDLER(stop = 1)

enum clbt_mode {
   CLBT_MODE_NO_CLB = 0,
   CLBT_MODE_FLD_SEARCH
};


typedef enum clbt_search_result {
   CLBT_SEARCH_FOUND = 0,
   CLBT_SEARCH_FOUND_PARTIAL,
   CLBT_SEARCH_NOT_FOUND,
   CLBT_SEARCH_NO_SEARCH
}clbt_search_result_t;


struct clb_test_data {
   char *check_field;
   clbt_search_result_t found;
   int neg_triggered;
};


/* Check if field name is present in the received template (type of field is not checked) */
int check_fld_presence(int negotiation_result, uint8_t req_data_type, const char *req_data_fmt,
   uint8_t recv_data_type, const char *recv_data_fmt, void *my_data)
{
   struct clb_test_data *test_data = (struct clb_test_data *)my_data;
   test_data->neg_triggered = 1;

   if (recv_data_type != TRAP_FMT_UNIREC) {
      test_data->found = CLBT_SEARCH_NO_SEARCH;
      return TRAP_E_FORMAT_MISMATCH;
   }

   char *match = strstr(recv_data_fmt, test_data->check_field);
   char *match_tail = match + strlen(test_data->check_field);
   if (match == NULL) {
      test_data->found = CLBT_SEARCH_NOT_FOUND;
   } else if ((match == recv_data_fmt || match[-1] == ' ') && (match_tail[0] == 0 || match_tail[0] == ' ' || match_tail[0] == ',')) {
      test_data->found = CLBT_SEARCH_FOUND;
   } else {
      test_data->found = CLBT_SEARCH_FOUND_PARTIAL;
   }

   return TRAP_E_OK;
}


int main(int argc, char **argv)
{
   int ret;
   int test_ret;

   ur_template_t *in_tmplt = NULL;

   struct clb_test_data test_data;
   uint16_t test_mode = CLBT_MODE_NO_CLB;
   unsigned rcvd_rec_cnt = 0;

   test_data.check_field = NULL;
   test_data.neg_triggered = 0;

   /* **** TRAP initialization **** */
   INIT_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)

   TRAP_DEFAULT_INITIALIZATION(argc, argv, *module_info);

   TRAP_REGISTER_DEFAULT_SIGNAL_HANDLER();

   signed char opt;
   while ((opt = TRAP_GETOPT(argc, argv, module_getopt_string, long_options)) != -1) {
      switch (opt) {
      case 'c':
         test_data.check_field = optarg;
         test_mode = CLBT_MODE_FLD_SEARCH;
         break;
      default:
         fprintf(stderr, "Error: Invalid argument %c.\n", opt);
         FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS);
         TRAP_DEFAULT_FINALIZATION();
         return -1;
      }
   }

   char* err;
   in_tmplt = ur_create_input_template(0, NULL, &err);
   if (in_tmplt == NULL) {
      fprintf(stderr, "Error: Input UniRec template was not created (%s).\n", err);
      ret = 1;
      goto exit;
   }


   if (test_mode == CLBT_MODE_FLD_SEARCH) {
      if (!test_data.check_field) {
         fprintf(stderr, "Error: requested field search mode but no field is set.\n");
         ret = 2;
         goto exit;
      }
      trap_clb_in_negotiation(check_fld_presence, (void *)&test_data);
   } else {
      trap_clb_in_negotiation(NULL, NULL);
   }

   test_ret = 200;
   int trap_ret = TRAP_E_OK;

   /* **** Main processing loop **** */
   while (!stop) {
      /* Receive record from input interface (block until data are available) */
      const void *rec;
      uint16_t rec_size;

      trap_ret = TRAP_RECEIVE(0, rec, rec_size, in_tmplt);
      TRAP_DEFAULT_RECV_ERROR_HANDLING(trap_ret, continue, break);

      if (test_mode == CLBT_MODE_FLD_SEARCH) {
         if (test_data.neg_triggered != 0) {
            if (test_data.found == CLBT_SEARCH_FOUND) {
               test_ret = 0;
            } else if (test_data.found == CLBT_SEARCH_FOUND_PARTIAL) {
               test_ret = 101;
            } else if (test_data.found == CLBT_SEARCH_NOT_FOUND) {
               test_ret = 102;
            } else {
               test_ret = 211;
            }
         } else {
            test_ret = 221;
         }
      } else if (test_mode == CLBT_MODE_NO_CLB) {
         if (test_data.neg_triggered == 0) {
            test_ret = 0;
         } else {
            test_ret = 222;
         }
      } else {
         test_ret = 231;
      }

      // Check size of received data
      if (rec_size < ur_rec_fixlen_size(in_tmplt)) {
         if (rec_size <= 1) {
//            printf("Received ending record, terminating.\n");
            break; // End of data (used for testing purposes)
         } else {
            fprintf(stderr, "Error: data with wrong size received.");
            break;
         }
      }
      ++rcvd_rec_cnt;
   }

   if (trap_ret != 0) {
      ret = trap_ret;
   } else {
      ret = test_ret;
   }

//   printf("Received %i records.\n", rcvd_rec_cnt);

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

