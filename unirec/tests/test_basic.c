/**
 * \file test_basic.c
 * \brief Test for basic UniRec functions
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
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
#define FOO_TEST_VALUE 12345
#define BAR_TEST_VALUE 54321
#define IP_TEST_VALUE 12345678
#define STR_TEST_VALUE "Hello World!"
#define NEW_TEST_VALUE 124
#define STR2_TEST_VALUE "Hello Unirec!"

UR_FIELDS(
   uint32 FOO,
   uint32 BAR,
   ipaddr IP,
   string STR1,
   string STR2,
   int32* ARR1,
   ipaddr* IPs,
   macaddr* MACs,
   uint64* ARR2,
   time *TIMEs
)


int main(int argc, char **argv)
{
   char *buffer, *buffer2, *template_string1, *template_string2;

   // Create a record and store it to the buffer
   {
      // Create template
      ur_template_t *tmplt = ur_create_template("STR1,FOO,BAR,IP,STR1,STR1,IP,ARR1,ARR2,IPs,MACs,TIMEs", NULL);
      if (tmplt == NULL) {
         fprintf(stderr, "Error during creating template\n");
         return 1;
      }
      template_string1 = ur_template_string(tmplt);
      if (template_string1 == NULL) {
         fprintf(stderr, "Error during creating template string\n");
         return 1;
      }
      // Allocate memory for a record
      void *rec = ur_create_record(tmplt, 512); // Pre-allocate 512 bytes for strings
      if(rec == NULL) {
         fprintf(stderr, "Error during creating record\n");
         return 1;
      }
      // Fill values into the record
      ur_set(tmplt, rec, F_FOO, FOO_TEST_VALUE);
      ur_set(tmplt, rec, F_BAR, BAR_TEST_VALUE);
      ur_set(tmplt, rec, F_IP, ip_from_int(IP_TEST_VALUE));
      ur_set_string(tmplt, rec, F_STR1, STR_TEST_VALUE);

      if (ur_set_array_from_string(tmplt, rec, F_TIMEs, "    2018-06-27T16:52:54 2018-06-27T16:52:54.500  2018-06-27T16:52:54Z  ") != 0) {
         fprintf(stderr, "Error set time array from string\n");
         return 1;
      }
      if (ur_set_array_from_string(tmplt, rec, F_ARR2, "             10   11 12 13  14 15 16 17         18 19") != 0) {
         fprintf(stderr, "Error set uint array from string failed\n");
         return 1;
      }
      if (ur_set_from_string(tmplt, rec, F_IPs, "    10.0.0.1     10.0.0.2 ::1   127.0.0.1     ") != 0) {
         fprintf(stderr, "Error when setting IP adresses from string\n");
         return 1;
      }
      ur_array_append(tmplt, rec, F_IPs, ip_from_int(IP_TEST_VALUE));
      if (ur_set_from_string(tmplt, rec, F_MACs, "00:00:00:00:00:00      11:11:11:11:11:11     22:22:22:22:22:22        ") != 0) {
         fprintf(stderr, "Error when setting MAC adresses from string\n");
         return 1;
      }

      if (ur_array_get_elem_cnt(tmplt, rec, F_ARR1) != 0) {
         fprintf(stderr, "Error, array element count should be %d and is %d\n", 0, ur_array_get_elem_cnt(tmplt, rec, F_ARR1));
         return 1;
      }

      ur_array_allocate(tmplt, rec, F_ARR1, 10);
      ur_array_allocate(tmplt, rec, F_ARR1, 20);
      ur_array_allocate(tmplt, rec, F_ARR1, 30);
      ur_array_allocate(tmplt, rec, F_ARR1, 0);

      ur_array_append(tmplt, rec, F_ARR1, -9);
      ur_array_allocate(tmplt, rec, F_ARR1, 5);
      if (ur_array_get_elem_cnt(tmplt, rec, F_ARR1) != 5) {
         fprintf(stderr, "Error, array element count should be %d and is %d\n", 5, ur_array_get_elem_cnt(tmplt, rec, F_ARR1));
         return 1;
      }

      // Fill the array (indexes 1-9) with values from -8 to 0
      for (int i = 2;  i < 9; ++i) {
         ur_array_set(tmplt, rec, F_ARR1, 9-i, -i);
      }
      *(int32_t *) ur_array_append_get_ptr(tmplt, rec, F_ARR1) = -1;
      *(int32_t *) ur_array_append_get_ptr(tmplt, rec, F_ARR1) = 0;

      if (ur_array_get_elem_cnt(tmplt, rec, F_ARR1) != 10) {
         fprintf(stderr, "Error, array element count should be %d and is %d\n", 10, ur_array_get_elem_cnt(tmplt, rec, F_ARR1));
         return 1;
      }

      // Store record into a buffer
      buffer = malloc(ur_rec_size(tmplt, rec));
      if(buffer == NULL) {
         fprintf(stderr, "Error, memory allocation\n");
         return 1;
      }
      memcpy(buffer, rec, ur_rec_size(tmplt, rec));
      // Free memory
      ur_free_record(rec);
      ur_free_template(tmplt);
   }

   // -----

   // Read data from the record in the buffer
   {
      // Create another template with the same set of fields (the set of fields MUST be the same, even if we don't need to work with all fields)
      ur_template_t *tmplt = ur_create_template("FOO   ,  BAR\n,IP,STR1,   ARR1, ARR2 ,MACs, IPs, TIMEs", NULL);
      if(tmplt == NULL){
         fprintf(stderr, "Error during creating template\n");
         return 1;
      }
      template_string2 = ur_template_string(tmplt);
      if (template_string2 == NULL) {
         fprintf(stderr, "Error during creating template string\n");
         return 1;
      }
      //check templates
      if (strcmp(template_string1, template_string2) != 0) {
         fprintf(stderr, "templates string does not match.\nTemplate1: %s\nTemplate2: %s\n", template_string1, template_string2);
         return 1;
      }
      //test values
      if (ur_get(tmplt, buffer, F_FOO) != FOO_TEST_VALUE) {
         fprintf(stderr, "FOO value does not match. It is %d and should be %d\n", ur_get(tmplt, buffer, F_FOO), FOO_TEST_VALUE);
         return 1;
      }
      if (ip_get_v4_as_int(&(ur_get(tmplt, buffer, F_IP))) != IP_TEST_VALUE) {
         fprintf(stderr, "IP value does not match. It is %d and should be %d\n", ip_get_v4_as_int(&(ur_get(tmplt, buffer, F_IP))), IP_TEST_VALUE);
         return 1;
      }
      if (ur_get(tmplt, buffer, F_BAR) != BAR_TEST_VALUE) {
         fprintf(stderr, "BAR value does not match. It is %d and should be %d\n", ur_get(tmplt, buffer, F_BAR), BAR_TEST_VALUE);
         return 1;
      }
      char str_cmp [] = STR_TEST_VALUE;
      if (strlen(str_cmp) != ur_get_var_len(tmplt, buffer, F_STR1) || memcmp(ur_get_ptr(tmplt, buffer, F_STR1), str_cmp, ur_get_var_len(tmplt, buffer, F_STR1)) != 0) {
         fprintf(stderr, "STR1 value does not match. It is %.*s and should be %s\n", ur_get_var_len(tmplt, buffer, F_STR1), ur_get_ptr(tmplt, buffer, F_STR1), STR_TEST_VALUE);
         return 1;
      }

      if (ur_array_get_elem_cnt(tmplt, buffer, F_IPs) != 5) {
         fprintf(stderr, "Error when setting IP addresses\n");
         return 1;
      }
      if (ur_array_get_elem_cnt(tmplt, buffer, F_MACs) != 3) {
         fprintf(stderr, "Error when setting MAC addresses\n");
         return 1;
      }
      if (ur_array_get_elem_cnt(tmplt, buffer, F_TIMEs) != 3) {
         fprintf(stderr, "Error when setting times\n");
         return 1;
      }

      ip_addr_t ip_tmp;
      if (!ip_from_str("10.0.0.1", &ip_tmp)   || ip_cmp(&ip_tmp, &ur_array_get(tmplt, buffer, F_IPs, 0)) ||
          !ip_from_str("10.0.0.2", &ip_tmp)   || ip_cmp(&ip_tmp, &ur_array_get(tmplt, buffer, F_IPs, 1)) ||
          !ip_from_str("::1", &ip_tmp)        || ip_cmp(&ip_tmp, &ur_array_get(tmplt, buffer, F_IPs, 2)) ||
          !ip_from_str("127.0.0.1", &ip_tmp)  || ip_cmp(&ip_tmp, &ur_array_get(tmplt, buffer, F_IPs, 3)) ||
          ip_cmp(&ur_get(tmplt, buffer, F_IP), &ur_array_get(tmplt, buffer, F_IPs, 4))) {
         fprintf(stderr, "Error IP address mismatch\n");
         return 1;
      }

      mac_addr_t mac_tmp;
      if (!mac_from_str("0:0:0:0:0:0", &mac_tmp)         || mac_cmp(&mac_tmp, &ur_array_get(tmplt, buffer, F_MACs, 0)) ||
          !mac_from_str("11:11:11:11:11:11", &mac_tmp)   || mac_cmp(&mac_tmp, &ur_array_get(tmplt, buffer, F_MACs, 1)) ||
          !mac_from_str("22:22:22:22:22:22", &mac_tmp)   || mac_cmp(&mac_tmp, &ur_array_get(tmplt, buffer, F_MACs, 2))) {
         fprintf(stderr, "Error MAC address mismatch\n");
         return 1;
      }


      if (ur_array_get_elem_size(F_ARR1) != sizeof(uint32_t)) {
         fprintf(stderr, "Error, array element size should be %lu and is %d\n", sizeof(uint32_t), ur_array_get_elem_size(F_ARR1));
         return 1;
      }

      for (int i = 0;  i < 10; ++i) {
         int32_t val = ur_array_get(tmplt, buffer, F_ARR1, i);
         if (val != i - 9) {
            fprintf(stderr, "1# ARR1 value mismatch at %d index, read %d, expected %d\n", i, val, i - 9);
            return 1;
         }
      }

      for (uint64_t i = 0;  i < 10; ++i) {
         uint64_t val = ur_array_get(tmplt, buffer, F_ARR2, i);
         if (val != i + 10) {
            fprintf(stderr, "2# ARR2 value mismatch at %lu index, read %lu, expected %lu\n", i, val, i + 10);
            return 1;
         }
      }


      ur_free_template(tmplt);
   }

   // -----

   // Test ur_clone_record() function
   {
      ur_template_t *tmplt = ur_create_template("STR1,FOO,BAR,IP", NULL);
      if (tmplt == NULL){
         fprintf(stderr, "Error during creating template\n");
         return 1;
      }
      // Create a clone of the record in the buffer
      void *newrec = ur_clone_record(tmplt, buffer);
      if (tmplt == NULL){
         fprintf(stderr, "Error during cloning a record\n");
         return 1;
      }
      // Check if the records are the same
      if (ur_rec_size(tmplt, buffer) != ur_rec_size(tmplt, newrec)) {
        fprintf(stderr, "Cloned record has different size than original\n");
        return 1;
      }
      if (memcmp(buffer, newrec, ur_rec_size(tmplt, buffer))) {
        fprintf(stderr, "Cloned record is different from original\n");
        return 1;
      }
      // Free memory
      ur_free_record(newrec);
      ur_free_template(tmplt);
   }


   // -----

   // Copy selected fields of the record into a new one and add two new fields,
   // one is known before (STR2), one is newly defined (NEW)
   {
      // Define the field NEW
      int define_ret_val = ur_define_field("NEW", UR_TYPE_UINT16);
      if (define_ret_val < 0) {
         fprintf(stderr, "ERROR: Can't define new unirec field 'NEW'");
         exit(1);
      }
      ur_field_id_t new_id = define_ret_val;
      // Create templates matching the old and the new record
      ur_template_t *tmplt1 = ur_create_template("FOO,BAR,IP,STR1,ARR1,TIMEs,ARR2,MACs,IPs", NULL);
      if (tmplt1 == NULL) {
         fprintf(stderr, "Error during creating template\n");
         return 1;
      }
      ur_template_t *tmplt2 = ur_create_template("BAR,STR1,STR2,NEW", NULL);
      if (tmplt2 == NULL) {
         fprintf(stderr, "Error during creating template\n");
         return 1;
      }

      // Allocate buffer for the new record
      buffer2 = ur_create_record(tmplt2, ur_get_var_len(tmplt1, buffer, F_STR1) + 64);
      // This function copies all fields present in both templates from buffer1 to buffer2
      ur_copy_fields(tmplt2, buffer2, tmplt1, buffer);

      // Record in buffer2 now contains fields BAR and STR1 with the same values as in buffer1.
      // Values of STR2 and NEW are undefined.

      // Set value of str2
      static const char str2 [] = STR2_TEST_VALUE;
      ur_set_string(tmplt2, buffer2, F_STR2, str2);
      // Set value of NEW
      // (we can't use ur_set because type of the field is not known at compile time)
      *(uint16_t*)ur_get_ptr_by_id(tmplt2, buffer2, new_id) = NEW_TEST_VALUE;
      ur_free_template(tmplt1);
      ur_free_template(tmplt2);
   }

   // -----

   // Read data from the record in the second buffer
   {
      // Create template with the second set of fields (we can use different order of fields, it doesn't matter)
      ur_template_t *tmplt = ur_create_template("BAR,NEW,STR2,STR1", NULL);

      // The NEW field is already defined (it is stored globally) but we don't know its ID here
      int new_id = ur_get_id_by_name("NEW");
      if (new_id < 0) {
         fprintf(stderr, "field NEW was not found.\n");
      }
      //test values
      if (*((int*)(ur_get_ptr_by_id(tmplt, buffer2, new_id))) != NEW_TEST_VALUE) {
         fprintf(stderr, "NEW value does not match. It is %d and should be %d\n", *((int*)(ur_get_ptr_by_id(tmplt, buffer2, new_id))), NEW_TEST_VALUE);
         return 1;
      }
      if (ur_get(tmplt, buffer2, F_BAR) != BAR_TEST_VALUE) {
         fprintf(stderr, "BAR value does not match. It is %d and should be %d\n", ur_get(tmplt, buffer, F_BAR), BAR_TEST_VALUE);
         return 1;
      }
      char str_cmp [] = STR_TEST_VALUE;
      if (strlen(str_cmp) != ur_get_var_len(tmplt, buffer2, F_STR1) || memcmp(ur_get_ptr(tmplt, buffer2, F_STR1), str_cmp, ur_get_var_len(tmplt, buffer2, F_STR1)) != 0) {
         fprintf(stderr, "STR1 value does not match. It is %.*s and should be %s\n", ur_get_var_len(tmplt, buffer2, F_STR1), ur_get_ptr(tmplt, buffer2, F_STR1), STR_TEST_VALUE);
         return 1;
      }
      char str_cmp2 [] = STR2_TEST_VALUE;
      if (strlen(str_cmp2) != ur_get_var_len(tmplt, buffer2, F_STR2) || memcmp(ur_get_ptr(tmplt, buffer2, F_STR2), str_cmp2, ur_get_var_len(tmplt, buffer2, F_STR2)) != 0) {
         fprintf(stderr, "STR2 value does not match. It is %.*s and should be %s\n", ur_get_var_len(tmplt, buffer2, F_STR2), ur_get_ptr(tmplt, buffer2, F_STR2), STR2_TEST_VALUE);
         return 1;
      }

      ur_free_template(tmplt);
   }
   {
      ur_template_t *tmplt;
      int ret;
      const char *spec = "ipaddr ADDRA,   ipaddr ADDRB  , ipaddr  ADDRC ";
      tmplt = ur_create_template_from_ifc_spec(spec);
      if (tmplt != NULL) {
         fprintf(stderr, "Allocation should have failed because of undefined fields.\n");
         return 1;
      }
      if ((ret = ur_define_set_of_fields(spec)) != UR_OK) {
         fprintf(stderr, "ur_define_set_of_fields() failed.");
         return 1;
      }
      tmplt = ur_create_template_from_ifc_spec(spec);
      if (tmplt == NULL) {
         fprintf(stderr, "Allocation of template failed.\n");
         return 1;
      }

      // The NEW field is already defined (it is stored globally) but we don't know its ID here
      int new_id = ur_get_id_by_name("ADDRA");
      if (new_id == UR_E_INVALID_NAME) {
         fprintf(stderr, "field ADDRA was not found.\n");
         return 1;
      }
      new_id = ur_get_id_by_name("ADDRB");
      if (new_id == UR_E_INVALID_NAME) {
         fprintf(stderr, "field ADDRB was not found.\n");
         return 1;
      }
      new_id = ur_get_id_by_name("ADDRC");
      if (new_id == UR_E_INVALID_NAME) {
         fprintf(stderr, "field ADDRC was not found.\n");
         return 1;
      }
      ur_free_template(tmplt);
   }
   //deallocation
   free(buffer);
   free(buffer2);
   ur_finalize();
   free(template_string1);
   free(template_string2);
   printf("Test OK\n");
   return 0;
}
