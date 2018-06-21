/*!
 * \file counting_sort_test.c
 * \brief Counting sort test suit
 * \author Tomas Jansky <jansky@cesnet.cz>
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

#include "../include/counting_sort.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define ITEM_CNT 150

typedef struct test_record {
   uint32_t dummy_padding;
   uint8_t data;
} test_record_t;

uint32_t get_key(const void * arg)
{
   return ((test_record_t *)arg)->data;
}

int asc_cmp(const void* p1, const void* p2)
{
   return ((test_record_t *)p1)->data - ((test_record_t *)p2)->data;
}

int dsc_cmp(const void* p1, const void* p2)
{
   return ((test_record_t *)p2)->data - ((test_record_t *)p1)->data;
}

int main(void)
{
   int result = 0;
   cs_ret_code ret;
   test_record_t input[ITEM_CNT];
   test_record_t output[ITEM_CNT];

   /* Clear all bytes so that valgrind does not have problems
      during memcmp with random padding bytes values. */
   memset(input, 0, sizeof(input));
   memset(output, 0, sizeof(input));

   /* Fill input array with values between 2 - 102 (including both) */
   for (uint32_t i = 0; i < ITEM_CNT; i++) {
      input[i].data = i % 101 + 2;
   }

   /* ******************** */
   printf("TEST 1: NULL INPUT ARRAY...");
   ret = counting_sort(NULL, output, ITEM_CNT, sizeof(test_record_t), 2, 102, CS_ORDER_DSC, get_key);
   if (ret != CS_BAD_PARAM) {
      result = 1;
      printf(" failed - returned exit code %d, expected %d.\n", ret, CS_BAD_PARAM);
   } else {
      printf(" ok.\n");
   }

   /* ******************** */
   printf("TEST 2: NULL OUTPUT ARRAY...");
   ret = counting_sort(input, NULL, ITEM_CNT, sizeof(test_record_t), 2, 102, CS_ORDER_DSC, get_key);
   if (ret != CS_BAD_PARAM) {
      result = 1;
      printf(" failed - returned exit code %d, expected %d.\n", ret, CS_BAD_PARAM);
   } else {
      printf(" ok\n");
   }

   /* ******************** */
   printf("TEST 3: ZERO ITEM COUNT...");
   ret = counting_sort(input, output, 0, sizeof(test_record_t), 2, 102, CS_ORDER_DSC, get_key);
   if (ret != CS_BAD_PARAM) {
      result = 1;
      printf(" failed - returned exit code %d, expected %d.\n", ret, CS_BAD_PARAM);
   } else {
      printf(" ok\n");
   }

   /* ******************** */
   printf("TEST 4: ZERO ITEM SIZE...");
   ret = counting_sort(input, output, ITEM_CNT, 0, 2, 102, CS_ORDER_DSC, get_key);
   if (ret != CS_BAD_PARAM) {
      result = 1;
      printf(" failed - returned exit code %d, expected %d.\n", ret, CS_BAD_PARAM);
   } else {
      printf(" ok\n");
   }

   /* ******************** */
   printf("TEST 5: BAD KEY RANGE...");
   ret = counting_sort(input, output, ITEM_CNT, sizeof(test_record_t), 15, 10, CS_ORDER_DSC, get_key);
   if (ret != CS_BAD_PARAM) {
      result = 1;
      printf(" failed - returned exit code %d, expected %d.\n", ret, CS_BAD_PARAM);
   } else {
      printf(" ok\n");
   }

   /* ******************** */
   printf("TEST 5: ASC ORDER...");
   ret = counting_sort(input, output, ITEM_CNT, sizeof(test_record_t), 2, 102, CS_ORDER_ASC, get_key);
   qsort(input, ITEM_CNT, sizeof(test_record_t), asc_cmp);
   if (ret != CS_SUCCESS || memcmp(input, output, ITEM_CNT * sizeof(test_record_t)) != 0) {
      result = 1;
      printf(" failed - returned exit code %d, expected %d.\n", ret, CS_SUCCESS);
      printf("EXPECTED ARRAY:\n");
      for (uint32_t i = 0; i < ITEM_CNT; i++) {
         printf("%" PRIu8 " ", input[i].data);
      }

      printf("\nRESULT ARRAY:\n");
      for (uint32_t i = 0; i < ITEM_CNT; i++) {
         printf("%" PRIu8 " ", output[i].data);
      }

      printf("\n");
   } else {
      printf(" ok\n");
   }

   /* ******************** */
   printf("TEST 6: DSC ORDER...");
   ret = counting_sort(input, output, ITEM_CNT, sizeof(test_record_t), 2, 102, CS_ORDER_DSC, get_key);
   qsort(input, ITEM_CNT, sizeof(test_record_t), dsc_cmp);
   if (ret != CS_SUCCESS || memcmp(input, output, ITEM_CNT * sizeof(test_record_t)) != 0) {
      result = 1;
      printf(" failed - returned exit code %d, expected %d.\n", ret, CS_SUCCESS);
      printf("EXPECTED ARRAY:\n");
      for (uint32_t i = 0; i < ITEM_CNT; i++) {
         printf("%" PRIu8 " ", input[i].data);
      }

      printf("\nRESULT ARRAY:\n");
      for (uint32_t i = 0; i < ITEM_CNT; i++) {
         printf("%" PRIu8 " ", output[i].data);
      }

      printf("\n");
   } else {
      printf(" ok\n");
   }

   /* ******************** */
   printf("TEST 7: KEY VALUE BELOW SPECIFIED RANGE...");
   input[0].data = 1;
   ret = counting_sort(input, output, ITEM_CNT, sizeof(test_record_t), 2, 102, CS_ORDER_ASC, get_key);
   if (ret != CS_BAD_INDEX) {
      result = 1;
      printf(" failed - returned exit code %d, expected %d.\n", ret, CS_BAD_INDEX);
   } else {
      printf(" ok\n");
   }

   /* ******************** */
   printf("TEST 8: KEY VALUE OVER SPECIFIED RANGE...");
   input[0].data = 103;
   ret = counting_sort(input, output, ITEM_CNT, sizeof(test_record_t), 2, 102, CS_ORDER_ASC, get_key);
   if (ret != CS_BAD_INDEX) {
      result = 1;
      printf(" failed - returned exit code %d, expected %d.\n", ret, CS_BAD_INDEX);
   } else {
      printf(" ok\n");
   }

   return result;
}
