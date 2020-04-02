/**
 * \file test_iter.c
 * \brief Test of iteration functions
 * \author Zdenek Rosa <rosazden@fit.cvut.cz>
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <string.h>
#include "../unirec.h"
#include "fields.h"
//Example of usage Unirec library
#define FIELDS_COUNT 99
#define FIELDS_COUNT_HALF (FIELDS_COUNT/2 + 1)
#define NAME_LEN 10
#define FORMAT_SPEC_LEN (FIELDS_COUNT * NAME_LEN)
#define FORMAT_SPEC_HALF_LEN (FIELDS_COUNT_HALF * NAME_LEN)
UR_FIELDS()


int main(int argc, char **argv)
{
   char *buffer, *buffer_half;
   int define_ret_val, i = 0, j = 0, half_fields_count = 0;
   char name[NAME_LEN];
   char format[FORMAT_SPEC_LEN], format_half[FORMAT_SPEC_HALF_LEN];
   int id[FIELDS_COUNT];
   int value[FIELDS_COUNT];
   int id_half[FIELDS_COUNT_HALF];
   int value_half[FIELDS_COUNT_HALF];
   format[0] = 0;
   format_half[0] = 0;

   // define set of dynamic fields (..., FOO3, FOO2, FOO1)
   for (i = FIELDS_COUNT-1; i >= 0; i--) {
      snprintf(name, NAME_LEN, "FOO%d", i);
      define_ret_val = ur_define_field(name, UR_TYPE_INT32);
      if (define_ret_val < 0) {
         fprintf(stderr, "ERROR: Can't define new unirec field %s", name);
         exit(1);
      }
      // store all the fields
      snprintf(format + strlen(format), FORMAT_SPEC_LEN - strlen(format), "%s,", name);
      id[j++] = define_ret_val;
      // store just odd fields
      if (i % 2 == 0) {
         snprintf(format_half + strlen(format_half), FORMAT_SPEC_HALF_LEN - strlen(format_half), "%s,", name);
         id_half[half_fields_count++] = define_ret_val;
      }
   }
   format[strlen(format)-1] = 0;
   format_half[strlen(format_half)-1] = 0;
   // Create template of all defined fields
   ur_template_t *tmplt = ur_create_template(format, NULL);
   if (tmplt == NULL) {
      fprintf(stderr, "Error during creating template 1\n");
      return 1;
   }
   // Create template of half defined fields
   ur_template_t *tmplt_half = ur_create_template(format_half, NULL);
   if (tmplt_half == NULL) {
      ur_free_template(tmplt);
      fprintf(stderr, "Error during creating template 2\n");
      return 1;
   }
    // Allocate buffer for the new record (all fields)
   buffer = ur_create_record(tmplt, 0);
   if (buffer == NULL) {
      ur_free_template(tmplt);
      ur_free_template(tmplt_half);
      fprintf(stderr, "Error during allocating buffer\n");
      return 1;
   }
   // Allocate buffer for the new record (half of fields)
   buffer_half = ur_create_record(tmplt_half, 0);
   if (buffer_half == NULL) {
      free(buffer);
      ur_free_template(tmplt);
      ur_free_template(tmplt_half);
      fprintf(stderr, "Error during allocating buffer\n");
      return 1;
   }
   // store rand number in each FIELD and remember this values
   j = 0;
   for (i = 0; i < FIELDS_COUNT; i++) {
      value[i] = rand();
      *((int32_t*)ur_get_ptr_by_id(tmplt, buffer, id[i])) = value[i];
      if (i % 2 == 0) {
         value_half[j] = value[i];
         *((int32_t*)ur_get_ptr_by_id(tmplt_half, buffer_half, id_half[j])) = value_half[j];
        j++;
      }
   }

   // TEST - All defined fields

   // iterate through all the fields. Order has to be the same like the fields were defined
   i = 0;
   ur_field_id_t id_iter = UR_ITER_BEGIN;
   while ((id_iter = ur_iter_fields(tmplt, id_iter)) != UR_ITER_END) {
      if (value[i] != *((int32_t*)(ur_get_ptr_by_id(tmplt, buffer, id_iter)))) {
         fprintf(stderr, "Error during iterating through template 1 (definition order)\n");
         return 1;
      }
      i++;
   }
   // Iterate through all the fields. Order has to be same like in record
   i = 0;
   while ((id_iter = ur_iter_fields_record_order(tmplt, i++)) != UR_ITER_END) {
      for (j = 0; j < FIELDS_COUNT; j++) {
         if (id[j] == id_iter) {
            break;
         }
      }
      if (value[j] != *((int32_t*)(ur_get_ptr_by_id(tmplt, buffer, id_iter)))) {
         fprintf(stderr, "Error during iterating through template (record order)\n");
         return 1;
      }
   }

   // TEST - Half of defined fields

   // iterate through all the fields in the template. Order has to be the same like the fields were defined
   i = 0;
   id_iter = UR_ITER_BEGIN;
   while ((id_iter = ur_iter_fields(tmplt_half, id_iter)) != UR_ITER_END) {
         if (value_half[i] != *((int32_t*)(ur_get_ptr_by_id(tmplt_half, buffer_half, id_iter)))) {
         fprintf(stderr, "Error during iterating through template 2 (definition order)\n");
         return 1;
      }
      i++;
   }
   // Iterate through all the fields. Order has to be same like in record
   i = 0;
   while ((id_iter = ur_iter_fields_record_order(tmplt_half, i++)) != UR_ITER_END) {
      for (j = 0; j < FIELDS_COUNT_HALF; j++) {
         if (id_half[j] == id_iter) {
            break;
         }
      }
      if (value_half[j] != *((int32_t*)(ur_get_ptr_by_id(tmplt_half, buffer_half, id_iter)))) {
         fprintf(stderr, "Error during iterating through template (record order)\n");
         return 1;
      }
   }

   ur_free_template(tmplt);
   ur_free_template(tmplt_half);
   free(buffer);
   free(buffer_half);
   ur_finalize();
   printf("Test OK\n");
   return 0;
}
