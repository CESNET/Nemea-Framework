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
#include <stdint.h>
#include <time.h>
#include "../include/b_plus_tree.h"


#define TEST_SIZE_ARR_SIZE 4
#define LEAF_SIZE_ARR_SIZE 3
static uint32_t test_size_arr[] = {999, 9999, 99999, 999999};
static uint32_t leaf_size_arr[] = {5,50,100};

#define difftime_ms(end, start) \
 (((double)end.tv_sec + 1.0e-9*end.tv_nsec) - ((double)start.tv_sec + 1.0e-9*start.tv_nsec));

typedef struct b_key_t{
    uint32_t key;
} b_key_t;

typedef struct b_value_t{
    uint64_t value;
} b_value_t;

typedef struct test_pair_t{
    uint32_t key;
    uint64_t value;
    uint64_t sort_for_delete_random_value;
    uint8_t deleted;
} test_pair_t;

double time_one_set_of_test = 0;
static int compare_value_in_pairs(const void * a, const void * b)
{
   uint64_t h1, h2;
   h1 = (*((test_pair_t**)a))->value;
   h2 = (*((test_pair_t**)b))->value;
   if (h1 == h2) {
         return EQUAL;
   }
   else if (h1 < h2) {
      return LESS;
   }
   else {
      return MORE;
   }
}

static int compare_random_delete_value_in_pairs(const void * a, const void * b)
{
   uint64_t h1, h2;
   h1 = (*((test_pair_t**)a))->sort_for_delete_random_value;
   h2 = (*((test_pair_t**)b))->sort_for_delete_random_value;
   if (h1 == h2) {
         return EQUAL;
   }
   else if (h1 < h2) {
      return LESS;
   }
   else {
      return MORE;
   }
}

int compare_key(void * a, void * b)
{
   uint32_t h1, h2;
   h1 = ((b_key_t*)a)->key;
   h2 = ((b_key_t*)b)->key;
   if (h1 == h2) {
      return EQUAL;
   }
   else if (h1 < h2) {
      return LESS;
   }
   else {
      return MORE;
   }
}

int check_search_of_items(test_pair_t *pairs, void *tree, uint32_t test_count)
{
   int ret_val = 0;
   uint32_t i;
   b_value_t *value_pt;
   double time_diff;
   struct timespec start_time = {0,0}, end_time = {0,0};
   clock_gettime(CLOCK_MONOTONIC, &start_time);
   i = 0;
   while (i < test_count && pairs[i].deleted == 1) {
      i++;
   }
   while (i < test_count) {
      //value from bplus item structure
      value_pt = bpt_search(tree, &(pairs[i].key));
      if (value_pt == NULL) {
         fprintf(stderr, "ERROR during searching item in the tree by function bpt_search.\n");
         ret_val = -3;
         goto exit_label2;
      }
      if (pairs[i].value != value_pt->value) {
         fprintf(stderr, "ERROR, during iteration through the tree. Key is %u. Expected value: %lu, value in the tree:%lu\n", pairs[i].key, pairs[i].value, value_pt->value);
         ret_val = -4;
         goto exit_label2;
      }
      i++;
      while (i < test_count && pairs[i].deleted == 1) {
         i++;
      }
   }

exit_label2:
   if (ret_val >= 0) {
      clock_gettime(CLOCK_MONOTONIC, &end_time);
      time_diff = difftime_ms(end_time, start_time);
      time_one_set_of_test += time_diff;
      printf("OK. Time: %fs\n", time_diff);
   }
   return ret_val;
}

int check_sort_of_items(test_pair_t *pairs, void *tree, uint32_t test_count)
{
   int ret_val = 0;
   bpt_list_item_t *b_item = NULL;
   int is_there_next = 0;
   uint32_t i;
   b_value_t *value_pt;
   b_key_t *key_pt;
   double time_diff;
   struct timespec start_time = {0,0}, end_time = {0,0};
   clock_gettime(CLOCK_MONOTONIC, &start_time);
   b_item = bpt_list_init(tree);
   if (b_item == NULL) {
      fprintf(stderr,"ERROR during initializing list iterator structure\n");
      ret_val = -1;
      goto exit_label;
   }
   i = 0;
   while (i < test_count && pairs[i].deleted == 1) {
      i++;
   }
   is_there_next = bpt_list_start(tree, b_item);
   while (is_there_next == 1) {
      //value from bplus item structure
      value_pt = b_item->value;
      if (value_pt == NULL) {
         fprintf(stderr, "ERROR during iteration through the tree. Value is NULL\n");
         ret_val = -3;
         goto exit_label;
      }
      key_pt = b_item->key;
      if (key_pt == NULL) {
         fprintf(stderr, "ERROR during iteration through the tree. Key is NULL\n");
         ret_val = -3;
         goto exit_label;
      }
      if (i >= test_count) {
         fprintf(stderr, "ERROR, there are more items in the tree than it was inserted.\n");
         ret_val = -3;
         goto exit_label;
      }
      //test key and value
      if (pairs[i].key != key_pt->key) {
         fprintf(stderr, "ERROR, during iteration through the tree. Expected key: %u, key in the tree:%u\n", pairs[i].key, key_pt->key);
         ret_val = -4;
         goto exit_label;
      }
      if (pairs[i].value != value_pt->value) {
         fprintf(stderr, "ERROR, during iteration through the tree. Key is %u. Expected value: %lu, value in the tree:%lu\n", pairs[i].key, pairs[i].value, value_pt->value);
         ret_val = -4;
         goto exit_label;
      }
      i++;
      while (i < test_count && pairs[i].deleted == 1) {
         i++;
      }
      //next item
      is_there_next = bpt_list_item_next(tree, b_item);
   }

   while (i < test_count && pairs[i].deleted == 1) {
      i++;
   }
   if (i < test_count) {
      fprintf(stderr,"ERROR missing items in the tree\n");
      ret_val = -5;
   }

exit_label:
   if (b_item != NULL) {
      bpt_list_clean(b_item);
      b_item = NULL;
   }
   if (ret_val >= 0) {
      clock_gettime(CLOCK_MONOTONIC, &end_time);
      time_diff = difftime_ms(end_time, start_time);
      time_one_set_of_test += time_diff;
      printf("OK. Time: %fs\n", time_diff);
   }
   return ret_val;
}

int run_tests(int test_count, int tree_size_leaf)
{
   int rand_del, is_there_next, ret;
   uint32_t i, count_of_deleted_items = 0;
   double time_diff;
   void *tree = NULL;
   test_pair_t *pairs = NULL;
   test_pair_t **pairs_value_sorted = NULL;
   test_pair_t **pairs_sorted_random_for_delete = NULL;
   struct timespec start_time = {0,0}, end_time = {0,0};
   int ret_val = 0;


   b_key_t key_st;
   b_key_t *key_pt = NULL;
   b_value_t *value_pt = NULL;
   bpt_list_item_t *b_item = NULL;
   //allocate structures for testing
   pairs = malloc(test_count * sizeof(test_pair_t));
   if (pairs == NULL) {
      fprintf(stderr,"ERROR: There are not enaugh memmory for this test. Please decrease the test_count\n Actual test_count = %u\n", test_count);
      ret_val = -1;
      goto exit_label;
   }
   pairs_value_sorted = malloc(test_count * sizeof(test_pair_t*));
   if (pairs_value_sorted == NULL) {
      fprintf(stderr,"ERROR: There are not enaugh memmory for this test. Please decrease the test_count\n Actual test_count = %u\n", test_count);
      ret_val = -1;
      goto exit_label;
   }
   pairs_sorted_random_for_delete = malloc(test_count * sizeof(test_pair_t*));
   if (pairs_sorted_random_for_delete == NULL) {
      fprintf(stderr,"ERROR: There are not enaugh memmory for this test. Please decrease the test_count\n Actual test_count = %u\n", test_count);
      ret_val = -1;
      goto exit_label;
   }

   //generate values
   printf("Generating values for test.\n");
   for (i = 0; i < test_count; i++) {
      pairs[i].key = i;
      pairs[i].value = (((uint64_t) rand() <<  0) & 0x00000000FFFFFFFFull) | (((uint64_t) rand() << 32) & 0xFFFFFFFF00000000ull);
      pairs[i].sort_for_delete_random_value = (((uint64_t) rand() <<  0) & 0x00000000FFFFFFFFull) | (((uint64_t) rand() << 32) & 0xFFFFFFFF00000000ull);
      pairs_value_sorted[i] = &(pairs[i]);
      pairs_sorted_random_for_delete[i] = &(pairs[i]);
      pairs[i].deleted = 0;
   }
   // Sort pairs on values
   qsort(pairs_value_sorted, test_count, sizeof(test_pair_t*), compare_value_in_pairs);
   // Sort pairs on random value for delete
   qsort(pairs_sorted_random_for_delete, test_count, sizeof(test_pair_t*), &compare_random_delete_value_in_pairs);
   //initialize tree
   tree = bpt_init(tree_size_leaf, &compare_key, sizeof(b_value_t), sizeof(b_key_t));
   if (tree == NULL) {
      fprintf(stderr,"ERROR during initializing b_plus_tree\n");
      ret_val = -1;
      goto exit_label;
   }
   //insert items to the tree
   printf("TEST - Item insertion\n");
   clock_gettime(CLOCK_MONOTONIC, &start_time);
   for (i = 0; i < test_count; i++) {
      key_st.key = pairs[i].key;
      value_pt = bpt_insert(tree, &key_st);
      if (value_pt == NULL) {
         fprintf(stderr,"ERROR during insertion. Key %u, value %lu \n", pairs[i].key, pairs[i].value);
         ret_val = -2;
         goto exit_label;
      }
      value_pt->value = pairs[i].value;
   }
   clock_gettime(CLOCK_MONOTONIC, &end_time);
   time_diff = difftime_ms(end_time, start_time);
   time_one_set_of_test += time_diff;
   printf("OK - %d items inserted. Time: %fs\n", test_count, time_diff);

   //test sort of items
   printf("TEST - Sort of inserted items\n");
   ret = check_sort_of_items(pairs, tree, test_count);
   if (ret < 0) {
      //error
      ret_val = ret;
      goto exit_label;
   }
   //test sort of items
   printf("TEST - Seach of inserted items\n");
   ret = check_search_of_items(pairs, tree, test_count);
   if (ret < 0) {
      //error
      ret_val = ret;
      goto exit_label;
   }

   //delete test - random delete of some items - 50% should be deleted
   printf("TEST - Deleting approximately 50%% of items\n");
   clock_gettime(CLOCK_MONOTONIC, &start_time);
   for (i = 0; i < test_count; i++) {
      rand_del = rand();
      if (rand_del % 2 == 0) {
         //delete
         key_st.key = pairs_sorted_random_for_delete[i]->key;
         if (bpt_item_del(tree, &key_st) != 1) {
            //deleting error
            fprintf(stderr,"ERROR, deleting item from tree. Key: %u\n", key_st.key);
            ret_val = -6;
            goto exit_label;
         } else {
            //item was deleted
            pairs_sorted_random_for_delete[i]->deleted = 1;
            count_of_deleted_items++;
         }
      }
   }
   clock_gettime(CLOCK_MONOTONIC, &end_time);
   time_diff = difftime_ms(end_time, start_time);
   time_one_set_of_test += time_diff;
   printf("OK - %d items were deleted. Time: %fs\n", count_of_deleted_items, time_diff);

   //test - checking the reamining items
   printf("TEST - Check remaing items after deleting. - Iteration function.\n");
   ret = check_sort_of_items(pairs, tree, test_count);
   if (ret < 0) {
      //error
      ret_val = ret;
      goto exit_label;
   }
   printf("TEST - Check remaing items after deleting. - Searchnig function.\n");
   ret = check_search_of_items(pairs, tree, test_count);
   if (ret < 0) {
      //error
      ret_val = ret;
      goto exit_label;
   }

   //delete items during iterating the list of items.
   printf("TEST - Delete approximately 50%% remaining items during iteration the sorted list of items.\n");
   clock_gettime(CLOCK_MONOTONIC, &start_time);
   b_item = bpt_list_init(tree);
   if (b_item == NULL) {
      fprintf(stderr,"ERROR during initializing list iterator structure\n");
      ret_val = -1;
      goto exit_label;
   }
   i = 0;
   count_of_deleted_items = 0;
   while (i < test_count && pairs[i].deleted == 1) {
      i++;
   }
   is_there_next = bpt_list_start(tree, b_item);
   while (is_there_next == 1) {
      //value from bplus item structure
      value_pt = b_item->value;
      if (value_pt == NULL) {
         fprintf(stderr,"ERROR during iteration through the tree. Value is NULL\n");
         ret_val = -3;
         goto exit_label;
      }
      key_pt = b_item->key;
      if (key_pt == NULL) {
         fprintf(stderr,"ERROR during iteration through the tree. Key is NULL\n");
         ret_val = -3;
         goto exit_label;
      }
      if (i >= test_count) {
         fprintf(stderr,"ERROR, there are more items in the tree than it was inserted.\n");
         ret_val = -3;
         goto exit_label;
      }
      //test key and value
      if (pairs[i].key != key_pt->key || pairs[i].value != value_pt->value) {
         fprintf(stderr,"ERROR, during iteration through the tree. Expected item (key,value): (%u,%lu),  item in the tree: (%u,%lu)\n", pairs[i].key, pairs[i].value, key_pt->key, value_pt->value);
         ret_val = -4;
         goto exit_label;
      }
      //delete item
      rand_del = rand();
      if (rand_del % 2 == 0) {
         //delete
         is_there_next = bpt_list_item_del(tree, b_item);
         pairs[i].deleted = 1;
         count_of_deleted_items++;
      } else {
         //next item in tree
         is_there_next = bpt_list_item_next(tree, b_item);
      }
      //next item in testing structure
      i++;
      while (i < test_count && pairs[i].deleted == 1) {
         i++;
      }
   }
   if (i < test_count) {
      fprintf(stderr,"ERROR missing items in the tree\n");
      ret_val = -5;
      goto exit_label;
   }
   clock_gettime(CLOCK_MONOTONIC, &end_time);
   time_diff = difftime_ms(end_time, start_time);
   time_one_set_of_test += time_diff;
   printf("OK - %d items were deleted. Time: %fs\n", count_of_deleted_items, time_diff);

   //test - checking the reamining items
   printf("TEST - Check remaing items after deleting. - Iteration function.\n");
   ret = check_sort_of_items(pairs, tree, test_count);
   if (ret < 0) {
      //error
      ret_val = ret;
      goto exit_label;
   }
   printf("TEST - Check remaing items after deleting. - Searchnig function.\n");
   ret = check_search_of_items(pairs, tree, test_count);
   if (ret < 0) {
      //error
      ret_val = ret;
      goto exit_label;
   }
   printf("Dealocation of memmory\n");

exit_label:
   if (b_item != NULL) {
      bpt_list_clean(b_item);
      b_item = NULL;
   }
   if (tree != NULL) {
      bpt_clean(tree);
      tree = NULL;
   }
   if (pairs != NULL) {
      free(pairs);
      pairs = NULL;
   }
   if (pairs_value_sorted != NULL) {
      free(pairs_value_sorted);
      pairs_value_sorted = NULL;
   }
   if (pairs_sorted_random_for_delete != NULL) {
      free(pairs_sorted_random_for_delete);
      pairs_sorted_random_for_delete = NULL;
   }

   if (ret_val < 0) {
      printf("run_tests failed!\n");
   } else {
      printf("Time of all tests: %fs\n", time_one_set_of_test);
   }
   return ret_val;
}

int main(int argc, char **argv)
{
   int test = 1, test_cnt_it, leaf_cnt_it, res;
   for (leaf_cnt_it = 0; leaf_cnt_it < LEAF_SIZE_ARR_SIZE; leaf_cnt_it++) {
      for (test_cnt_it = 0; test_cnt_it < TEST_SIZE_ARR_SIZE; test_cnt_it++) {
         printf("%d.TEST - count of items = %u, leaf size = %u\n"\
                "---------------------------------------------------\n", test++, test_size_arr[test_cnt_it], leaf_size_arr[leaf_cnt_it]);
         res = run_tests(test_size_arr[test_cnt_it], leaf_size_arr[leaf_cnt_it]);
         printf("\n");
         if (res < 0) {
            return res;
         }
      }
   }
   printf("OK - ALL TESTS WERE SUCCESSFUL\n");
   return 0;

}