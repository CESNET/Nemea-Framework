/**
 * \file test_basic.c
 * \brief Test for b_plus_tree data structure
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
#include "../include/prefix_tree.h"

#define MAX_LENGTH 50
#define MIN_LENGTH 2
#define RANGE (MAX_LENGTH - MIN_LENGTH)
#define SEPARATOR "."

#define RAND_NUM_IN_RANGE() \
   ((rand() % RANGE) + MIN_LENGTH);

#define TEST_SIZE_ARR_SIZE 2
static uint32_t test_size_arr[] = {999, 9999};

#define difftime_ms(end, start) \
 (((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec));

typedef struct value_t{
   uint32_t value;
} value_t;

double time_one_set_of_test = 0;


value_t *value_from_string(const char *str, value_t *value)
{
   uint32_t v = 0;
   uint32_t len = 0, numbers = 0, non_numbers = 0;
   while (*str != 0) {
      len++;
      if (*str >= '0' && *str <= '9') {
         numbers++;
      } else {
         non_numbers++;
      }
      if (*str >= 'a' && *str <= 'h') {
         v |= len;
      } else if (*str >= 'i' && *str <= 'z') {
         v += numbers;
      } else if (*str >= 'A' && *str <= 'R') {
         v *= numbers;
      } else if (*str >= 'S') {
         v += non_numbers;
      } else {
         v *= non_numbers;
      }
      str++;
   }
   value->value = v;
   return value;

}

char *gen_random_str(const int len) {
      int i = 0;
      static const char alphanum[] =
         "0123456789"
         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
         "abcdefghijklmnopqrstuvwxyz"
         SEPARATOR;
      char *s;
      s = (char*) malloc (sizeof(char) * (len + 1));
      if (s == NULL) {
         return NULL;
      }
      //generate first letter without separator
      for (i = 0; i < len; i++) {
         if (i == 0 || i == len - 1 || s[i-1] == SEPARATOR[0]) {
            s[i] = alphanum[rand() % (sizeof(alphanum) - 2)];
         } else {
            s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
         }
      }
      s[len] = 0;
      return s;
}
int test_prefix_sufix(int toward, const char *skip_str, int len_skip_str, const char *str, int len_str)
{
   int i;
   if (len_str < len_skip_str) {
      return 0;
   }
   if (toward == PREFIX) {
      for (i = 0; i < len_skip_str; i++) {
         if (skip_str[i] != str[i]) {
            return 0;
         }
      }
      return 1;
   } else {
      for (i = len_skip_str - 1; i >= 0; i--) {
         if (skip_str[i] != str[--len_str]) {
            return 0;
         }
      }
      return 1;
   }
}


int test_tree_content(prefix_tree_t *tree, int test_count,  int toward, const char *delete_str,  char **array_of_strings, int *array_of_lengths)
{
   struct timespec start_time = {0,0}, end_time = {0,0};
   double time_diff;
   char test_str[MAX_LENGTH + 1];
   int delete_str_len = 0, degree, i, j;
   prefix_tree_domain_t * domain = NULL;
   value_t value;
   clock_gettime(CLOCK_MONOTONIC, &start_time);
   if (delete_str != NULL) {
      delete_str_len = strlen(delete_str);
   }
   for (i = 0; i < test_count; i++) {
      //skip deleted strings
      if (delete_str != NULL && test_prefix_sufix(toward, delete_str, delete_str_len, array_of_strings[i], array_of_lengths[i]) == 1) {
         continue;
      }
      domain = prefix_tree_search(tree, array_of_strings[i], array_of_lengths[i]);
      if (domain == NULL) {
         fprintf(stderr, "ERROR: String \"%s\" was not found in the tree\n", array_of_strings[i]);
         return -2;
      }
      value_from_string(array_of_strings[i], &value);
      if (memcmp(domain->value, &value, sizeof(value_t)) != 0) {
         fprintf(stderr, "ERROR: String \"%s\" was found but testing value is different.\n", array_of_strings[i]);
         return -2;
      }
      //test count of domain nodes
      degree = 1;
      for (j = 0; j < array_of_lengths[i]; j++) {
         if (array_of_strings[i][j] == SEPARATOR[0]) {
            degree++;
         }
      }
      if (degree != domain->degree) {
         fprintf(stderr, "ERROR: String \"%s\" have deegree %d, but the tree says %d\n", array_of_strings[i], degree, domain->degree);
         return -2;
      }
      //test if string is readable
      prefix_tree_read_string(tree, domain, test_str);
      if (strcmp(test_str, array_of_strings[i]) != 0) {
         fprintf(stderr, "ERROR: String \"%s\" is not readable from the domain node. It returns %s\n", array_of_strings[i], test_str);
         return -2;
      }
   }
   clock_gettime(CLOCK_MONOTONIC, &end_time);
   time_diff = difftime_ms(end_time, start_time);
   time_one_set_of_test += time_diff;
   printf("OK. Time: %fs\n", time_diff);
   return 0;
}

int run_tests(int test_count, int toward, int domain_extension, int relaxation_after_delete)
{
   int ret_val = 0;
   prefix_tree_t * tree;
   int i, ret;
   prefix_tree_domain_t * domain = NULL;
   char **array_of_strings = NULL;
   int *array_of_lengths = NULL;
   char delete_str[MAX_LENGTH+1];
   value_t value;
   double time_diff;
   struct timespec start_time = {0,0}, end_time = {0,0};
   prefix_tree_inner_node_t * inner_node;
   time_one_set_of_test= 0;
   tree = prefix_tree_initialize(toward, sizeof(value_t), SEPARATOR[0], domain_extension, relaxation_after_delete);
   domain = prefix_tree_insert(tree, "domain2.com",11);
   //alocate testing structures
   printf("Creating testing structures\n");
   array_of_strings = (char**) malloc (sizeof(char*) * test_count);
   if (array_of_strings == NULL) {
      fprintf(stderr, "ERROR: There are not enaugh memmory for this test. Please decrease the test_count\n Actual test_count = %u\n", test_count);
      ret_val = -1;
      goto exit_label;
   }
   array_of_lengths = (int*) malloc (sizeof(int) * test_count);
   if (array_of_lengths == NULL) {
      fprintf(stderr, "ERROR: There are not enaugh memmory for this test. Please decrease the test_count\n Actual test_count = %u\n", test_count);
      ret_val = -1;
      goto exit_label;
   }
   //generate random stings
   printf("Generation of random strings\n");
   for (i = 0; i < test_count; i++) {
      array_of_lengths[i] = RAND_NUM_IN_RANGE();
      array_of_strings[i] = gen_random_str(array_of_lengths[i]);
      if (array_of_strings[i] == NULL) {
         fprintf(stderr, "ERROR: There are not enaugh memmory for this test. Please decrease the test_count\n Actual test_count = %u\n", test_count);
         ret_val = -1;
         goto exit_label;
      }
   }

   //Fill the tree
   printf("TEST - Inserting strings to the tree\n");
   clock_gettime(CLOCK_MONOTONIC, &start_time);
   for (i = 0; i < test_count; i++) {
      domain = prefix_tree_insert(tree, array_of_strings[i], array_of_lengths[i]);
      if (domain == NULL) {
         fprintf(stderr, "ERROR: Inserting string \"%s\" to the tree\n", array_of_strings[i]);
         ret_val = -2;
         goto exit_label;
      }
      value_from_string(array_of_strings[i], &value);
      memcpy(domain->value, &value, sizeof(value_t));
   }
   clock_gettime(CLOCK_MONOTONIC, &end_time);
   time_diff = difftime_ms(end_time, start_time);
   time_one_set_of_test += time_diff;
   printf("OK, %u values insterted. Time: %fs\n", test_count, time_diff);

   //Test if all values are in the tree and test their value and tree parameters
   printf("TEST - Checking keys, values and tree parameters.\n");
   ret = test_tree_content(tree, test_count, toward, NULL, array_of_strings, array_of_lengths);
   if (ret < 0) {
      ret_val = ret;
      goto exit_label;
   }

   //delete the most used doamin node
   printf("TEST - Delete the most used inner node and it's descendants.\n");
   clock_gettime(CLOCK_MONOTONIC, &start_time);
   inner_node = prefix_tree_most_substring(tree->root);
   if (inner_node == NULL) {
      fprintf(stderr, "ERROR: most used inner node was not found\n");
      ret_val = -3;
      goto exit_label;
   }
   prefix_tree_read_inner_node(tree, inner_node, delete_str);
   prefix_tree_delete_inner_node(tree, inner_node);
   clock_gettime(CLOCK_MONOTONIC, &end_time);
   time_diff = difftime_ms(end_time, start_time);
   time_one_set_of_test += time_diff;
   printf("OK. Time: %fs\n", time_diff);

   //Test if all values are in the tree and test their value and tree parameters
   printf("TEST - Checking keys, values and tree parameters after delete\n");
   ret = test_tree_content(tree, test_count, toward, delete_str, array_of_strings, array_of_lengths);
   if (ret < 0) {
      ret_val = ret;
      goto exit_label;
   }

exit_label:
   printf("Dealocation of memmory\n");
   if (tree != NULL) {
      prefix_tree_destroy(tree);
      tree = NULL;
   }
   if (array_of_strings != NULL) {
      for (i = 0; i <  test_count; i++) {
         if (array_of_strings[i] != NULL) {
            free(array_of_strings[i]);
            array_of_strings[i] = NULL;
         }
      }
      free(array_of_strings);
      array_of_strings = NULL;
   }
   if (array_of_lengths != NULL) {
      free(array_of_lengths);
      array_of_lengths = NULL;
   }
   if (ret_val >= 0) {
      printf("Time of all tests: %fs\n", time_one_set_of_test);
   }
   return ret_val;
}

int main(int argc, char **argv)
{
   int i, test = 1, ret;
   for (i = 0; i  < TEST_SIZE_ARR_SIZE; i++) {
      printf("%d.TEST - count of items = %u, PREFIX, DOMAIN_EXTENSION_YES, RELAXATION_AFTER_DELETE_YES\n"\
             "---------------------------------------------------\n", test++, test_size_arr[i]);
      ret = run_tests(test_size_arr[i], PREFIX, DOMAIN_EXTENSION_YES, RELAXATION_AFTER_DELETE_YES);
      if (ret < 0) {
         return ret;
      }
      printf("\n");

      printf("%d.TEST - count of items = %u, PREFIX, DOMAIN_EXTENSION_YES, RELAXATION_AFTER_DELETE_NO\n"\
             "---------------------------------------------------\n", test++, test_size_arr[i]);
      ret = run_tests(test_size_arr[i], PREFIX, DOMAIN_EXTENSION_YES, RELAXATION_AFTER_DELETE_NO);
      if (ret < 0) {
         return ret;
      }
      printf("\n");

      printf("%d.TEST - count of items = %u, PREFIX, DOMAIN_EXTENSION_NO, RELAXATION_AFTER_DELETE_YES\n"\
             "---------------------------------------------------\n", test++, test_size_arr[i]);
      ret = run_tests(test_size_arr[i], PREFIX, DOMAIN_EXTENSION_NO, RELAXATION_AFTER_DELETE_YES);
      if (ret < 0) {
         return ret;
      }
      printf("\n");

      printf("%d.TEST - count of items = %u, PREFIX, DOMAIN_EXTENSION_NO, RELAXATION_AFTER_DELETE_NO\n"\
             "---------------------------------------------------\n", test++, test_size_arr[i]);
      ret = run_tests(test_size_arr[i], PREFIX, DOMAIN_EXTENSION_NO, RELAXATION_AFTER_DELETE_NO);
      if (ret < 0) {
         return ret;
      }
      printf("\n");

      printf("%d.TEST - count of items = %u, SUFFIX, DOMAIN_EXTENSION_YES, RELAXATION_AFTER_DELETE_YES\n"\
             "---------------------------------------------------\n", test++, test_size_arr[i]);
      ret = run_tests(test_size_arr[i], SUFFIX, DOMAIN_EXTENSION_YES, RELAXATION_AFTER_DELETE_YES);
      if (ret < 0) {
         return ret;
      }
      printf("\n");

      printf("%d.TEST - count of items = %u, SUFFIX, DOMAIN_EXTENSION_YES, RELAXATION_AFTER_DELETE_NO\n"\
             "---------------------------------------------------\n", test++, test_size_arr[i]);
      ret = run_tests(test_size_arr[i], SUFFIX, DOMAIN_EXTENSION_YES, RELAXATION_AFTER_DELETE_NO);
      if (ret < 0) {
         return ret;
      }
      printf("\n");

      printf("%d.TEST - count of items = %u, SUFFIX, DOMAIN_EXTENSION_NO, RELAXATION_AFTER_DELETE_YES\n"\
             "---------------------------------------------------\n", test++, test_size_arr[i]);
      ret = run_tests(test_size_arr[i], SUFFIX, DOMAIN_EXTENSION_NO, RELAXATION_AFTER_DELETE_YES);
      if (ret < 0) {
         return ret;
      }
      printf("\n");

      printf("%d.TEST - count of items = %u, SUFFIX, DOMAIN_EXTENSION_NO, RELAXATION_AFTER_DELETE_NO\n"\
             "---------------------------------------------------\n", test++, test_size_arr[i]);
      ret = run_tests(test_size_arr[i], SUFFIX, DOMAIN_EXTENSION_NO, RELAXATION_AFTER_DELETE_NO);
      if (ret < 0) {
         return ret;
      }
      printf("\n");
   }
   printf("OK - ALL TESTS WERE SUCCESSFUL\n");
   return 0;
}