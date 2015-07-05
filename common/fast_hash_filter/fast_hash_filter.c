/**
 * \file fast_hash_filter.c
 * \brief Fast 8-way hash table with rehashing possibility.
 * \author Matej Vido, xvidom00@stud.fit.vutbr.cz
 * \date 2014
 */
/*
 * Copyright (C) 2014 CESNET
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

#include "../include/fast_hash_filter.h"
#include "fhf_hashes.h"

#include <stdlib.h>
#include <string.h>

/**
 * Index to the lookup table is the current value of free flag from the hash table row.
 * The value on the index is the order (column) of first free item in
 * the hash table row.
 */
uint8_t fhf_lt_free_flag[] = {
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
   0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

/**
 * Index to the lookup table is the exponent and the value on the index is
 * the power of two.
 */
uint8_t fhf_lt_pow_of_two[] = {
   1, 2, 4, 8, 16, 32, 64, 128
};

/**
 * \brief Function for initializing table.
 *
 * Parameters need to meet following requirements:
 * table_rows - non-zero, power of two
 * key_size   - non-zero
 * data_size  - non-zero
 *
 * @param table_rows    Number of rows in the table.
 * @param key_size      Size of key in bytes.
 * @param data_size     Size of data in bytes.
 *
 * @return Pointer to the hash table structure, NULL if the memory could not be allocated
 *         or parameters do not meet requirements.
 */
fhf_table_t * fhf_init(uint64_t table_rows, uint32_t key_size, uint32_t data_size)
{
   //check params
   if (!(table_rows && !(table_rows & (table_rows - 1)))) //power of two
      return NULL;
   if (!key_size)
      return NULL;
   if (!data_size)
      return NULL;

   //allocate table
   fhf_table_t *new_table = (fhf_table_t *) calloc(1, sizeof(fhf_table_t));

   if (new_table == NULL)
      return NULL;

   new_table->table_rows = table_rows;
   new_table->key_size = key_size;
   new_table->data_size = data_size;
   new_table->old_table = NULL;

   //set hash function
   if (key_size == 40)
      new_table->hash_function = &fhf_hash_40;
   else if (key_size % 8 == 0)
      new_table->hash_function = &fhf_hash_div8;
   else
      new_table->hash_function = &fhf_hash;

   //allocate field of keys
   if ((new_table->key_field = (uint8_t *) calloc(key_size * table_rows * FHF_TABLE_COLS, sizeof(uint8_t))) == NULL) {
      free(new_table);
      return NULL;
   }

   //allocate field of datas
   if ((new_table->data_field = (uint8_t *) calloc(data_size * table_rows * FHF_TABLE_COLS, sizeof(uint8_t))) == NULL) {
      free(new_table->key_field);
      free(new_table);
      return NULL;
   }

   //allocate field of free flags
   if ((new_table->free_flag_field = (uint8_t *) calloc(table_rows, sizeof(uint8_t))) == NULL) {
      free(new_table->key_field);
      free(new_table->data_field);
      free(new_table);
      return NULL;
   }

   //allocate field of locks for table
   if ((new_table->lock_table = (int8_t *) calloc(table_rows, sizeof(int8_t))) == NULL) {
      free(new_table->key_field);
      free(new_table->data_field);
      free(new_table->free_flag_field);
      free(new_table);
      return NULL;
   }

   return new_table;
}

/**
 * \brief Function for clearing table.
 *
 * Function sets free flags of all items in the table to zero.
 * Items with zero free flags are considered free. Data and keys remain in table while they
 * are replaced by new items.
 *
 * @param table   Pointer to the table structure.
 */
void fhf_clear(fhf_table_t *table)
{
   uint64_t i;

   for (i = 0; i < table->table_rows; i++) {
      //lock row
      while (__sync_lock_test_and_set(&table->lock_table[i], 1))
         ;
      table->free_flag_field[i] = 0;
      //unlock row
      __sync_lock_release(&table->lock_table[i]);
   }
}

/**
 * \brief Function for destroying table and freeing memory.
 *
 * Function frees memory of the whole table structure.
 *
 * @param table   Pointer to the table structure.
 */
void fhf_destroy(fhf_table_t *table)
{
   free(table->key_field);
   free(table->data_field);
   free(table->free_flag_field);
   free(table->lock_table);
   if (table->old_table != NULL)
      fhf_destroy(table->old_table);
   free(table);
}

/**
 * \brief Function for initializing iterator for the table.
 *
 * @param table   Pointer to the table structure.
 *
 * @return        Pointer to the iterator structure.
 *                NULL if could not allocate memory.
 */
fhf_iter_t * fhf_init_iter(fhf_table_t *table)
{
   if (table == NULL)
      return NULL;

   fhf_iter_t * new_iter = (fhf_iter_t *) calloc(1, sizeof(fhf_iter_t));

   if (new_iter == NULL)
      return NULL;

   new_iter->table = table;
   new_iter->row = FHF_ITER_START;
   new_iter->col = FHF_ITER_START;
   new_iter->key_ptr = NULL;
   new_iter->data_ptr = NULL;

   return new_iter;
}

/**
 * \brief Function for reinitializing iterator for table.
 *
 * @param iter    Pointer to the existing iterator.
 */
void fhf_reinit_iter(fhf_iter_t *iter)
{
   if (iter->row >= 0) {
      //unlock row
      __sync_lock_release(&iter->table->lock_table[iter->row]);
   }
   iter->row = FHF_ITER_START;
   iter->col = FHF_ITER_START;
   iter->key_ptr = NULL;
   iter->data_ptr = NULL;
}

/**
 * \brief Function for destroying iterator and freeing memory.
 *
 * If function is used in the middle of the table, function also unlocks row, which is locked.
 *
 * @param iter    Pointer to the existing iterator.
 */
void fhf_destroy_iter(fhf_iter_t *iter)
{
   if (iter->row >= 0) {
      //unlock row
      __sync_lock_release(&iter->table->lock_table[iter->row]);
   }
   free(iter);
}
