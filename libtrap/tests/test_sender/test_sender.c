/**
 * \file
 * \brief Testing sender for libtrap tests.
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
#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include "fields.h"


#define DEFAULT_OUTPUT_FORMAT "ipaddr SRC_IP,ipaddr DST_IP,uint16 SRC_PORT,uint16 DST_PORT,uint8 PROTOCOL,\
   uint32 PACKETS,uint64 BYTES,time TIME_FIRST,time TIME_LAST,string SOME_OTHER_MIRACLE_OF_VARIABLE_SIZE"

/// IMPORTANT: reaching of maximal UniRec size is not check, thus, don't use size too big or many variable size fields.
#define VARIABLE_SIZE_FIELD_SIZE 19

const unsigned DEFAULT_RECORD_COUNT = 10;


trap_module_info_t *module_info = NULL;


/**
 * Definition of basic module information - module name, module description, number of input and output interfaces
 */
#define MODULE_BASIC_INFO(BASIC) \
  BASIC("Test sender", \
        "This module serves as a test sender module. It sends (unbuffered) user specified number of UniRec messages in \
        an user specified format. All fields are filled by a memset with \"6\".", 0, 1)


/**
 * Definition of module parameters - every parameter has short_opt, long_opt, description,
 * flag whether an argument is required or it is optional and argument type which is NULL
 * in case the parameter does not need argument.
 * Module parameter argument types: int8, int16, int32, int64, uint8, uint16, uint32, uint64, float, string
 */
#define MODULE_PARAMS(PARAM) \
  PARAM('c', "count", "Count of records to send.", required_argument, "uint32")\
  PARAM('u', "unirec-format", "Output format of the messages.", required_argument, "string") \
  PARAM('N', "no-eof-message", "Do not send ending empty (eof) message.", no_argument, NULL)


static int stop = 0;

/**
 * Function to handle SIGTERM and SIGINT signals (used to stop the module)
 */
TRAP_DEFAULT_SIGNAL_HANDLER(stop = 1)

/// TODO: more sophisticated record creation
void fill_record (void *rec, ur_template_t *tmplt)
{
   ur_field_id_t fld_id = UR_ITER_BEGIN;

   /* Iterate over all fields in record. */
   fld_id = ur_iter_fields(tmplt, fld_id);
   while (fld_id != UR_ITER_END) {
      if (ur_is_static(fld_id)) {
         if (ur_get_type(fld_id) == UR_TYPE_IP) {
            ip_addr_t data;
            char *ip = "6.6.6.6";
            ip_from_str(ip, &data);
            memcpy(ur_get_ptr_by_id(tmplt, rec, fld_id), &data, ur_get_size(fld_id));
         } else {
            uint64_t data = 6;
            memcpy(ur_get_ptr_by_id(tmplt, rec, fld_id), &data, ur_get_size(fld_id));
         }
      } else {
         char data[VARIABLE_SIZE_FIELD_SIZE];
         uint16_t data_size;
         if (ur_get_type(fld_id) == UR_TYPE_STRING) {
            char *header = "I am ";
            char *tail = "...";
            size_t usable_fld_name_length = (VARIABLE_SIZE_FIELD_SIZE - strlen(header) - 1);
            strncpy(data, header, strlen(header));
            if (strlen(ur_get_name(fld_id)) <= usable_fld_name_length) {
               strncpy(data + strlen(header), ur_get_name(fld_id), strlen(ur_get_name(fld_id)));
               data_size = strlen(header)+strlen(ur_get_name(fld_id));
               data[data_size] = '\0';
               data_size++;
            } else {
               usable_fld_name_length -= strlen(tail);
               strncpy(data + strlen(header), ur_get_name(fld_id), usable_fld_name_length);
               strncpy(data + strlen(header) + usable_fld_name_length, tail, strlen(tail));
               data[VARIABLE_SIZE_FIELD_SIZE-1] = '\0';
               data_size = VARIABLE_SIZE_FIELD_SIZE;
            }
         } else {
            data_size = VARIABLE_SIZE_FIELD_SIZE;
            memset(data, 6, data_size);
         }

         ur_set_var(tmplt, rec, fld_id, data, data_size);
      }

      // Get next field
      fld_id = ur_iter_fields(tmplt, fld_id);
   }
}

int main(int argc, char **argv)
{
   int ret;
   char *out_tmplt_str = DEFAULT_OUTPUT_FORMAT;
   unsigned send_limit = DEFAULT_RECORD_COUNT;
   unsigned send_rec_cnt = 0;
   int no_eof_message = 0;

   void *out_rec = NULL;
   ur_template_t *out_tmplt = NULL;

   /* **** TRAP initialization **** */

   /*
    * Macro allocates and initializes module_info structure according to MODULE_BASIC_INFO and MODULE_PARAMS
    * definitions on the lines 71 and 84 of this file. It also creates a string with short_opt letters for getopt
    * function called "module_getopt_string" and long_options field for getopt_long function in variable "long_options"
    */
   INIT_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)

   /*
    * Let TRAP library parse program arguments, extract its parameters and initialize module interfaces
    */
   TRAP_DEFAULT_INITIALIZATION(argc, argv, *module_info);

   // Turn off buffering on statistics output interface
   ret = trap_ifcctl(TRAPIFC_OUTPUT, 0, TRAPCTL_BUFFERSWITCH, 0);
   if (ret != TRAP_E_OK) {
      if (trap_last_error_msg)
         fprintf(stderr, "Unable to turn off a buffer on the output interface: %s\n", trap_last_error_msg);
         ret = 1;
         goto exit;
   }

   /*
    * Register signal handler.
    */
   TRAP_REGISTER_DEFAULT_SIGNAL_HANDLER();

   /*
    * Parse program arguments defined by MODULE_PARAMS macro with getopt() function (getopt_long() if available)
    * This macro is defined in config.h file generated by configure script
    */
   signed char opt;
   while ((opt = TRAP_GETOPT(argc, argv, module_getopt_string, long_options)) != -1) {
      switch (opt) {
      case 'c':
         send_limit = strtoul(optarg, NULL, 0);
         break;
      case 'u':
         out_tmplt_str = optarg;
         break;
      case 'N':
         no_eof_message = 1;
         break;
      default:
         fprintf(stderr, "Invalid arguments.\n");
         FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS);
         TRAP_DEFAULT_FINALIZATION();
         return -1;
      }
   }

   out_tmplt = NULL;

   if (out_tmplt_str != NULL) {
      if (ur_define_set_of_fields(out_tmplt_str) != UR_OK) {
         fprintf(stderr, "Error: output template format is not accurate.\n "
            "It should be: \"type1 name1,type2 name2,...\".\n "
            "Name of field may be any string matching the reqular expression [A-Za-z][A-Za-z0-9_]*\n"
            "Available types are: int8, int16, int32, int64, uint8, uint16, uint32, uint64, char,"
            "float, double, ipaddr, time, string, bytes\n");
         ret = 2;
         goto exit;
      }
      char *f_names = ur_ifc_data_fmt_to_field_names(out_tmplt_str);
      out_tmplt = ur_create_output_template(0, f_names, NULL);
      free(f_names);
   } else {
      fprintf(stderr, "No template specified, terminating.\n");
      goto exit;
   }

   if (out_tmplt == NULL) {
      fprintf(stderr, "Output UniRec template was not created.\n");
      ret = 1;
      goto exit;
   }


   unsigned dynamic_fld_cnt = out_tmplt->count - out_tmplt->first_dynamic;
   out_rec = ur_create_record(out_tmplt, dynamic_fld_cnt * VARIABLE_SIZE_FIELD_SIZE);


   /* **** Main processing loop **** */
   while (!stop && send_rec_cnt < send_limit) {

      ur_clear_varlen(out_tmplt, out_rec);

      fill_record(out_rec, out_tmplt);

      printf("Send %u ...", ur_rec_size(out_tmplt, out_rec));
      // Send record to interface 0. Block if ifc is not ready (unless a timeout is set using trap_ifcctl).
      ret = trap_send(0, out_rec, ur_rec_size(out_tmplt, out_rec));

      // Handle possible errors
      TRAP_DEFAULT_SEND_ERROR_HANDLING(ret, continue, break);
      printf("done.\n");
      ++send_rec_cnt;
   }

   if (!no_eof_message) {
      // Send EOF messge
      ret = trap_send(0, NULL, 0);
      // Handle possible errors
      TRAP_DEFAULT_SEND_ERROR_HANDLING(ret, goto exit, goto exit);
   }

   ret = 0;

   /* **** Cleanup **** */
exit:
   // Do all necessary cleanup in libtrap before exiting
   TRAP_DEFAULT_FINALIZATION();

   // Release allocated memory for module_info structure
   FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)

   // Free unirec templates and output record
   ur_free_template(out_tmplt);
   ur_free_record(out_rec);
   ur_finalize();

   return ret;
}

