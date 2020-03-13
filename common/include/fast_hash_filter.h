/**
 * \file fast_hash_filter.h
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

#ifndef __FAST_HASH_FILTER_H__
#define __FAST_HASH_FILTER_H__

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Needed for some C++ modules
 */
#ifndef UINT64_MAX
#define UINT64_MAX -1ULL
#endif

/**
 * Number of columns in the hash table.
 */
#define FHF_TABLE_COLS 8

/**
 * Value of free flag when column is full.
 */
#define FHF_COL_FULL ((uint8_t) 0xFF)

/**
 * Constants used for insert functions.
 */
enum fhf_insert {
   FHF_INSERT_OK = 0,
   FHF_INSERT_FAILED = -1,
   FHF_INSERT_FULL = -2
};

/**
 * Constants used for resizing function.
 */
enum fhf_resize {
   FHF_RESIZE_OK = 0,
   FHF_RESIZE_FAILED_ALLOC = 1,
   FHF_RESIZE_FAILED_INSERT = 2
};

/**
 * Constants used for get and update data functions.
 */
enum fhf_found {
   FHF_FOUND = 0,
   FHF_NOT_FOUND = 1
};

/**
 * Constants used for removing functions.
 */
enum fhf_remove {
   FHF_REMOVED = 0,
   FHF_NOT_REMOVED = 1
};

/**
 * Constants used for iterator functions.
 */
enum fhf_iter {
   FHF_ITER_RET_OK  =  0,
   FHF_ITER_RET_END =  1,
   FHF_ITER_START   = -1,
   FHF_ITER_END     = -2
};

/**
 * Lookup tables.
 */
extern uint8_t fhf_lt_free_flag[];
extern uint8_t fhf_lt_pow_of_two[];

/**
 * Hash table structure.
 *
 * Free flag:
 * 0 - free
 * 1 - full
 *
 *                        MSB                         LSB
 * Bits                  | X | X | X | X | X | X | X | X |
 * Index of item in row    7   6   5   4   3   2   1   0
 */
typedef struct fhf_table_struct fhf_table_t;
struct fhf_table_struct {
   uint64_t    table_rows;                                           /**< Number of rows in the table. Non-zero, power of two. */
   uint32_t    key_size;                                             /**< Size of key in bytes. Non-zero value. */
   uint32_t    data_size;                                            /**< Size of data in bytes. Non-zero value. */
   uint8_t     *key_field;                                           /**< Pointer to array of keys. */
   uint8_t     *data_field;                                          /**< Pointer to array of data. */
   uint8_t     *free_flag_field;                                     /**< Pointer to array of free flags. */
   int8_t      *lock_table;                                          /**< Pointer to array of locks for rows in the table. */
   fhf_table_t *old_table;                                           /**< Pointer to old table structure which will be destroyed by next resizing. */
   uint64_t    (*hash_function)(const void *, uint32_t, uint64_t);   /**< Pointer to used hash function. */
};

/**
 * Iterator structure.
 */
typedef struct {
   fhf_table_t  *table;    /**< Pointer to the structure of table. */
   int64_t     row;        /**< Value of row where the item is located. */
   int32_t     col;        /**< Value of column where the item is located. */
   uint8_t     *key_ptr;   /**< Pointer to the key of item. */
   uint8_t     *data_ptr;  /**< Pointer to the data of item. */
} fhf_iter_t;

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
fhf_table_t * fhf_init(uint64_t table_rows, uint32_t key_size, uint32_t data_size);

/**
 * \brief Function for clearing table.
 *
 * Function sets free flags of all items in the table to zero.
 * Items with zero free flags are considered free. Data and keys remain in table while they
 * are replaced by new items.
 *
 * @param table   Pointer to the table structure.
 */
void fhf_clear(fhf_table_t *table);

/**
 * \brief Function for destroying table and freeing memory.
 *
 * Function frees memory of the whole table structure.
 *
 * @param table   Pointer to the table structure.
 */
void fhf_destroy(fhf_table_t *table);

/**
 * \brief Function for initializing iterator for the table.
 *
 * @param table   Pointer to the table structure.
 *
 * @return        Pointer to the iterator structure.
 *                NULL if could not allocate memory.
 */
fhf_iter_t * fhf_init_iter(fhf_table_t *table);

/**
 * \brief Function for reinitializing iterator for table.
 *
 * @param iter    Pointer to the existing iterator.
 */
void fhf_reinit_iter(fhf_iter_t *iter);

/**
 * \brief Function for destroying iterator and freeing memory.
 *
 * If function is used in the middle of the table, function also unlocks row, which is locked.
 *
 * @param iter    Pointer to the existing iterator.
 */
void fhf_destroy_iter(fhf_iter_t *iter);

/**
 * \brief Function for inserting the item into the table.
 *
 * Function checks whether there already is item with the key "key" in the table.
 * If not, function inserts the item in the table. If there is not any place in that row,
 * the item won't be inserted.
 *
 * @param      table    Pointer to table structure.
 * @param      key      Pointer to key of item to be inserted.
 * @param      data     Pointer to data of item to be inserted.
 *
 * @return     FHF_INSERT_OK      if the item was succesfully inserted.
 *             FHF_INSERT_FAILED  if the item already is in the table.
 *             FHF_INSERT_FULL    if the row where the item should be inserted is full
 *                                and the item could not be inserted.
 */
static inline int fhf_insert(fhf_table_t *table, const void *key, const void *data)
{
   uint64_t table_row = (table->table_rows - 1) & (table->hash_function)(key, table->key_size, (uint64_t) table);
   uint64_t table_col_row = table_row * FHF_TABLE_COLS;

   //lock row
   while (__sync_lock_test_and_set(&table->lock_table[table_row], 1))
      ;

   //looking for item
   if ((table->free_flag_field[table_row] & 0x01U) && !memcmp(&table->key_field[table_col_row * table->key_size], key, table->key_size)) {
      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x02U) && !memcmp(&table->key_field[(table_col_row + 1) * table->key_size], key, table->key_size)) {
      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x04U) && !memcmp(&table->key_field[(table_col_row + 2) * table->key_size], key, table->key_size)) {
      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x08U) && !memcmp(&table->key_field[(table_col_row + 3) * table->key_size], key, table->key_size)) {
      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x10U) && !memcmp(&table->key_field[(table_col_row + 4) * table->key_size], key, table->key_size)) {
      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x20U) && !memcmp(&table->key_field[(table_col_row + 5) * table->key_size], key, table->key_size)) {
      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x40U) && !memcmp(&table->key_field[(table_col_row + 6) * table->key_size], key, table->key_size)) {
      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x80U) && !memcmp(&table->key_field[(table_col_row + 7) * table->key_size], key, table->key_size)) {
      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_FAILED;
   }

   if (table->free_flag_field[table_row] < FHF_COL_FULL) {
      //insert item
      memcpy(&table->key_field[(table_col_row + fhf_lt_free_flag[table->free_flag_field[table_row]]) * table->key_size], key, table->key_size);
      memcpy(&table->data_field[(table_col_row + fhf_lt_free_flag[table->free_flag_field[table_row]]) * table->data_size], data, table->data_size);

      //change free flag
      table->free_flag_field[table_row] += fhf_lt_pow_of_two[fhf_lt_free_flag[table->free_flag_field[table_row]]];

      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_OK;
   }
   else {
      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_FULL;
   }
}

/**
 * \brief Function for inserting the item into the table. Function inserts only the key
 *        and sets pointer where the data can be inserted.
 *
 * Function checks whether there already is item with the key "key" in the table. If yes,
 * function sets data_ptr to point to the data and data can be updated.
 * If not, function inserts the item's key in the table and sets data_ptr to point to data
 * of new item. If there is not any place in the row where the item should be inserted,
 * the item won't be inserted.
 *
 * @param      table       Pointer to table structure.
 * @param      key         Pointer to key of item to be inserted.
 * @param      lock        Function sets "lock" to point to lock of row where is located
 *                         inserted item.
 * @param      data_ptr    Function sets "data_ptr" to point to data of inserted item.
 *
 * @return     FHF_INSERT_OK      if the item was succesfully inserted.
 *                                !!ROW IS LOCKED. USE fhf_unlock_data TO UNLOCK ROW!!
 *                                after doing something with data.
 *             FHF_INSERT_FAILED  if the item already is in the table.
 *                                !!ROW IS LOCKED. USE fhf_unlock_data TO UNLOCK ROW!!
 *                                after doing something with data.
 *             FHF_INSERT_FULL    if the row where the item should be inserted is full
 *                                and the item could not be inserted.
 *                                !!ROW IS UNLOCKED!!
 */
static inline int fhf_insert_own_or_update(fhf_table_t *table, const void *key, int8_t **lock, void ** data_ptr)
{
   uint64_t table_row = (table->table_rows - 1) & (table->hash_function)(key, table->key_size, (uint64_t) table);
   uint64_t table_col_row = table_row * FHF_TABLE_COLS;

   //lock row
   while (__sync_lock_test_and_set(&table->lock_table[table_row], 1))
      ;

   //looking for item
   if ((table->free_flag_field[table_row] & 0x01U) && !memcmp(&table->key_field[table_col_row * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = &table->data_field[table_col_row * table->data_size];
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x02U) && !memcmp(&table->key_field[(table_col_row + 1) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = &table->data_field[(table_col_row + 1) * table->data_size];
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x04U) && !memcmp(&table->key_field[(table_col_row + 2) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = &table->data_field[(table_col_row + 2) * table->data_size];
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x08U) && !memcmp(&table->key_field[(table_col_row + 3) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = &table->data_field[(table_col_row + 3) * table->data_size];
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x10U) && !memcmp(&table->key_field[(table_col_row + 4) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = &table->data_field[(table_col_row + 4) * table->data_size];
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x20U) && !memcmp(&table->key_field[(table_col_row + 5) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = &table->data_field[(table_col_row + 5) * table->data_size];
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x40U) && !memcmp(&table->key_field[(table_col_row + 6) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = &table->data_field[(table_col_row + 6) * table->data_size];
      return FHF_INSERT_FAILED;
   }

   if ((table->free_flag_field[table_row] & 0x80U) && !memcmp(&table->key_field[(table_col_row + 7) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = &table->data_field[(table_col_row + 7) * table->data_size];
      return FHF_INSERT_FAILED;
   }

   if (table->free_flag_field[table_row] < FHF_COL_FULL) {
      //insert item
      memcpy(&table->key_field[(table_col_row + fhf_lt_free_flag[table->free_flag_field[table_row]]) * table->key_size], key, table->key_size);

      *lock = &table->lock_table[table_row];
      *data_ptr = &table->data_field[(table_col_row + fhf_lt_free_flag[table->free_flag_field[table_row]]) * table->data_size];

      //change free flag
      table->free_flag_field[table_row] += fhf_lt_pow_of_two[fhf_lt_free_flag[table->free_flag_field[table_row]]];
      return FHF_INSERT_OK;
   }
   else
   {
      //unlock row
      __sync_lock_release(&table->lock_table[table_row]);
      return FHF_INSERT_FULL;
   }
}

/**
 * \brief Function for updating data in table.
 *
 * @param   table       Pointer to the table structure.
 * @param   key         Key of item to be updated.
 * @param   lock        Function sets "lock" to point to lock of row where is located
 *                      item to be updated.
 * @param   data_ptr    Function sets "data_ptr" to point to data of updated item.
 *
 * @return  FHF_FOUND         if item is found, "lock" is set, "data_ptr" is set.
 *                            !!ROW IS LOCKED. USE fhf_unlock_data TO UNLOCK ROW!!
 *                            after doing something with data.
 *          FHF_NOT_FOUND     if item is not found, "lock" is NOT SET, "data_ptr" is NOT SET.
 *                            !!ROW IS UNLOCKED!!
 */
static inline int fhf_update_data(fhf_table_t *table, const void *key, int8_t **lock, void **data_ptr)
{
   uint64_t table_row = (table->table_rows - 1) & (table->hash_function)(key, table->key_size, (uint64_t) table);
   uint64_t table_col_row = table_row * FHF_TABLE_COLS;

   //lock row
   while (__sync_lock_test_and_set(&table->lock_table[table_row], 1))
      ;

   if ((table->free_flag_field[table_row] & 0x01U) && !memcmp(&table->key_field[table_col_row * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (void *) &table->data_field[table_col_row * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x02U) && !memcmp(&table->key_field[(table_col_row + 1) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (void *) &table->data_field[(table_col_row + 1) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x04U) && !memcmp(&table->key_field[(table_col_row + 2) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (void *) &table->data_field[(table_col_row + 2) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x08U) && !memcmp(&table->key_field[(table_col_row + 3) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (void *) &table->data_field[(table_col_row + 3) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x10U) && !memcmp(&table->key_field[(table_col_row + 4) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (void *) &table->data_field[(table_col_row + 4) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x20U) && !memcmp(&table->key_field[(table_col_row + 5) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (void *) &table->data_field[(table_col_row + 5) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x40U) && !memcmp(&table->key_field[(table_col_row + 6) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (void *) &table->data_field[(table_col_row + 6) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x80U) && !memcmp(&table->key_field[(table_col_row + 7) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (void *) &table->data_field[(table_col_row + 7) * table->data_size];
      return FHF_FOUND;
   }

   //unlock row
   __sync_lock_release(&table->lock_table[table_row]);
   return FHF_NOT_FOUND;
}

/**
 * \brief Function for getting data from table.
 *        Function DOES NOT USE LOCKS.
 *
 *        !!DO NOT USE WITH MULTIPLE THREADS!!
 *
 *
 * @param   table       Pointer to the table structure.
 * @param   key         Key of item to be found.
 * @param   data_ptr    Function sets "data_ptr" to point to data of found item.
 *
 * @return  FHF_FOUND         if item is found, "data_ptr" is set.
 *          FHF_NOT_FOUND     if item is not found, "data_ptr" is NOT SET.
 */
static inline int fhf_get_data(fhf_table_t *table, const void *key, const void **data_ptr)
{
   uint64_t table_row = (table->table_rows - 1) & (table->hash_function)(key, table->key_size, (uint64_t) table);
   uint64_t table_col_row = table_row * FHF_TABLE_COLS;

   if ((table->free_flag_field[table_row] & 0x01U) && !memcmp(&table->key_field[table_col_row * table->key_size], key, table->key_size)) {
      *data_ptr = (const void *) &table->data_field[table_col_row * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x02U) && !memcmp(&table->key_field[(table_col_row + 1) * table->key_size], key, table->key_size)) {
      *data_ptr = (const void *) &table->data_field[(table_col_row + 1) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x04U) && !memcmp(&table->key_field[(table_col_row + 2) * table->key_size], key, table->key_size)) {
      *data_ptr = (const void *) &table->data_field[(table_col_row + 2) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x08U) && !memcmp(&table->key_field[(table_col_row + 3) * table->key_size], key, table->key_size)) {
      *data_ptr = (const void *) &table->data_field[(table_col_row + 3) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x10U) && !memcmp(&table->key_field[(table_col_row + 4) * table->key_size], key, table->key_size)) {
      *data_ptr = (const void *) &table->data_field[(table_col_row + 4) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x20U) && !memcmp(&table->key_field[(table_col_row + 5) * table->key_size], key, table->key_size)) {
      *data_ptr = (const void *) &table->data_field[(table_col_row + 5) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x40U) && !memcmp(&table->key_field[(table_col_row + 6) * table->key_size], key, table->key_size)) {
      *data_ptr = (const void *) &table->data_field[(table_col_row + 6) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x80U) && !memcmp(&table->key_field[(table_col_row + 7) * table->key_size], key, table->key_size)) {
      *data_ptr = (const void *) &table->data_field[(table_col_row + 7) * table->data_size];
      return FHF_FOUND;
   }

   return FHF_NOT_FOUND;
}

/**
 * \brief Function for getting data from the table.
 *        Function USES LOCKS.
 *        CAN BE USED WITH MULTIPLE THREADS.
 *
 * @param   table       Pointer to the table structure.
 * @param   key         Key of found item..
 * @param   lock        Function sets "lock" to point to lock of row where is located found item.
 * @param   data_ptr    Function sets "data_ptr" to point to data of found item.
 *
 * @return  FHF_FOUND         if item is found, "lock" is set, "data_ptr" is set.
 *                            !!ROW IS LOCKED. USE fhf_unlock_data TO UNLOCK ROW!!
 *                            after reading data.
 *          FHF_NOT_FOUND     if item is not found, "lock" is NOT SET, "data_ptr" is NOT SET.
 *                            !!ROW IS UNLOCKED!!
 */
static inline int fhf_get_data_locked(fhf_table_t *table, const void *key, int8_t **lock, const void **data_ptr)
{
   uint64_t table_row = (table->table_rows - 1) & (table->hash_function)(key, table->key_size, (uint64_t) table);
   uint64_t table_col_row = table_row * FHF_TABLE_COLS;

   //lock row
   while (__sync_lock_test_and_set(&table->lock_table[table_row], 1))
      ;

   if ((table->free_flag_field[table_row] & 0x01U) && !memcmp(&table->key_field[table_col_row * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (const void *) &table->data_field[table_col_row * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x02U) && !memcmp(&table->key_field[(table_col_row + 1) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (const void *) &table->data_field[(table_col_row + 1) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x04U) && !memcmp(&table->key_field[(table_col_row + 2) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (const void *) &table->data_field[(table_col_row + 2) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x08U) && !memcmp(&table->key_field[(table_col_row + 3) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (const void *) &table->data_field[(table_col_row + 3) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x10U) && !memcmp(&table->key_field[(table_col_row + 4) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (const void *) &table->data_field[(table_col_row + 4) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x20U) && !memcmp(&table->key_field[(table_col_row + 5) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (const void *) &table->data_field[(table_col_row + 5) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x40U) && !memcmp(&table->key_field[(table_col_row + 6) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (const void *) &table->data_field[(table_col_row + 6) * table->data_size];
      return FHF_FOUND;
   }
   if ((table->free_flag_field[table_row] & 0x80U) && !memcmp(&table->key_field[(table_col_row + 7) * table->key_size], key, table->key_size)) {
      *lock = &table->lock_table[table_row];
      *data_ptr = (const void *) &table->data_field[(table_col_row + 7) * table->data_size];
      return FHF_FOUND;
   }

   //unlock row
   __sync_lock_release(&table->lock_table[table_row]);
   return FHF_NOT_FOUND;
}

/**
 * \brief Function for unlocking table row.
 *
 * @param lock      Pointer to lock variable.
 */
static inline void fhf_unlock_data(int8_t *lock)
{
   __sync_lock_release(lock);
}

/**
 * \brief Function for removing item from the table.
 *
 * Function removes item which key is equal to "key" from the table.
 * Free flag is changed to zero - indicates free item.
 * Key and data remains until there is new item inserted.
 *
 * @param table         Pointer to the hash table structure.
 * @param key           Key of item which will be removed.
 *
 * @return  FHF_REMOVED       if item is found and removed.
 *          FHF_NOT_REMOVED   if item is not found and not removed.
 */
static inline int fhf_remove(fhf_table_t *table, const void *key)
{
   uint64_t table_row = (table->table_rows - 1) & (table->hash_function)(key, table->key_size, (uint64_t) table);
   uint64_t table_col_row = table_row * FHF_TABLE_COLS;
   unsigned int i;

   //lock row
   while (__sync_lock_test_and_set(&table->lock_table[table_row], 1))
      ;

   for (i = 0; i < FHF_TABLE_COLS; i++) {
      if ((table->free_flag_field[table_row] & (1 << i)) && !memcmp(&table->key_field[(table_col_row + i) * table->key_size], key, table->key_size)) {
         table->free_flag_field[table_row] &= ~(1 << i);
         //unlock row
         __sync_lock_release(&table->lock_table[table_row]);
         return FHF_REMOVED;
      }
   }
   //unlock row
   __sync_lock_release(&table->lock_table[table_row]);
   return FHF_NOT_REMOVED;
}

/**
 * \brief Function for removing LOCKED item from the table.
 *        Function DOES NOT LOCKS LOCK, it can be used only to remove already LOCKED ITEM.
 *
 * Function removes item which key is equal to "key" from the table.
 * Free flag is changed to zero - indicates free item.
 * Key and data remains until there is new item inserted.
 *
 * @param table         Pointer to the hash table structure.
 * @param key           Key of item which will be removed.
 * @param lock_ptr      Pointer to lock of the table, where the item is.
 *
 * @return  FHF_REMOVED       if item is found and removed.
 *                            !!ROW IS UNLOCKED!!
 *          FHF_NOT_REMOVED   if item is not found and not removed.
 *                            lock which is pointed to by lock_ptr !!REMAINS LOCKED!!
 */
static inline int fhf_remove_locked(fhf_table_t *table, const void *key, int8_t *lock_ptr)
{
   uint64_t table_row = (table->table_rows - 1) & (table->hash_function)(key, table->key_size, (uint64_t) table);
   uint64_t table_col_row = table_row * FHF_TABLE_COLS;
   unsigned int i;

   if (lock_ptr == &table->lock_table[table_row]) {
      for (i = 0; i < FHF_TABLE_COLS; i++) {
         if ((table->free_flag_field[table_row] & (1 << i)) && !memcmp(&table->key_field[(table_col_row + i) * table->key_size], key, table->key_size)) {
            table->free_flag_field[table_row] &= ~(1 << i);
            //unlock row
            __sync_lock_release(&table->lock_table[table_row]);
            return FHF_REMOVED;
         }
      }
   }
   return FHF_NOT_REMOVED;
}

/**
 * \brief Function for removing actual item from the table when using iterator.
 *
 * @param iter          Pointer to iterator structure.
 *
 * @return  FHF_REMOVED       if item is removed.
 *          FHF_NOT_REMOVED   if item is not removed.
 */
static inline int fhf_remove_iter(fhf_iter_t *iter)
{
   switch(iter->row) {
      case FHF_ITER_START:
      case FHF_ITER_END:
         return FHF_NOT_REMOVED;

      default:
         if (iter->table->free_flag_field[iter->row] & (1 << iter->col)) {
            iter->table->free_flag_field[iter->row] &= ~(1 << iter->col);
            iter->key_ptr = NULL;
            iter->data_ptr = NULL;
            return FHF_REMOVED;
         }
         return FHF_NOT_REMOVED;
   }
}

/**
 * \brief Function for getting next item in the table.
 *
 * @param iter      Pointer to the iterator structure.
 *
 * @return  FHF_ITER_RET_OK   if iterator structure contain next structure.
 *          FHF_ITER_RET_END  if iterator is in the end of the table and does not
 *                            contain any other item.
 */
static inline int fhf_get_next_iter(fhf_iter_t *iter)
{
   uint32_t i, j;

   switch (iter->row) {
      default:
         for (j = iter->col + 1; j < FHF_TABLE_COLS; j++) {
            if (iter->table->free_flag_field[iter->row] & (1 << j)) {
               iter->col = j;
               iter->key_ptr = &iter->table->key_field[(iter->row * FHF_TABLE_COLS + j) * iter->table->key_size];
               iter->data_ptr = &iter->table->data_field[(iter->row * FHF_TABLE_COLS + j) * iter->table->data_size];
               return FHF_ITER_RET_OK;
            }
         }
         //unlock row
         __sync_lock_release(&iter->table->lock_table[iter->row]);

      case FHF_ITER_START:
         for (i = iter->row + 1; i < iter->table->table_rows; i++) {
            //lock row
            while (__sync_lock_test_and_set(&iter->table->lock_table[i], 1))
               ;
            for (j = 0; j < FHF_TABLE_COLS; j++) {
               if (iter->table->free_flag_field[i] & (1 << j)) {
                  iter->row = i;
                  iter->col = j;
                  iter->key_ptr = &iter->table->key_field[(i * FHF_TABLE_COLS + j) * iter->table->key_size];
                  iter->data_ptr = &iter->table->data_field[(i * FHF_TABLE_COLS + j) * iter->table->data_size];
                  return FHF_ITER_RET_OK;
               }
            }
            //unlock row
            __sync_lock_release(&iter->table->lock_table[i]);
         }

      case FHF_ITER_END:
         iter->row = FHF_ITER_END;
         iter->col = FHF_ITER_END;
         iter->key_ptr = NULL;
         iter->data_ptr = NULL;
         return FHF_ITER_RET_END;
   }
}

/**
 * \brief Function for resizing and rehashing the table.
 *
 * Function initializes new table with double size and inserts all items from old table to new.
 * If some item cannot be inserted in new table, function tries double the size again and so on. Maximum is UINT64_MAX/2 + 1.
 * Hashing functions use as a seed pointer to table, so new table should also generates new hashes.
 * Old table is destroyed by next resizing, therefore pointer to old table is saved in new table.
 *
 * @param table   Pointer to pointer to table. Function changes pointer to table.
 *
 * @return  FHF_RESIZE_OK              if resizing is successful and all items from old table
 *                                     were inserted to new table and the pointer to table is changed.
 *          FHF_RESIZE_FAILED_ALLOC    if allocation of memory for new table or for iterator fails, pointer to table remains same.
 *          FHF_RESIZE_FAILED_INSERT   if table size was changed to maximal size and some item
 *                                     could not be inserted in new table.
 */
static inline int fhf_resize(fhf_table_t **table)
{
   int ret = 0;
   uint64_t new_table_rows;
   fhf_table_t * old_table;
   fhf_table_t * new_table;

   old_table = *table;

   if (old_table->old_table != NULL) {
      fhf_destroy(old_table->old_table);
      old_table->old_table = NULL;
   }

   new_table_rows = 2 * old_table->table_rows;

   while (new_table_rows <= UINT64_MAX/2 + 1 && new_table_rows != 0) {
      fhf_iter_t * iterator_old_table;

      new_table = fhf_init(new_table_rows, old_table->key_size, old_table->data_size);
      if (new_table == NULL)
         return FHF_RESIZE_FAILED_ALLOC;

      iterator_old_table = fhf_init_iter(old_table);
      if (iterator_old_table == NULL) {
         fhf_destroy(new_table);
         return FHF_RESIZE_FAILED_ALLOC;
      }

      while (fhf_get_next_iter(iterator_old_table) != FHF_ITER_RET_END) {
         ret = fhf_insert(new_table, (const void *) iterator_old_table->key_ptr, (const void *) iterator_old_table->data_ptr);
         if (ret != FHF_INSERT_OK) {
            break;
         }
      }
      fhf_destroy_iter(iterator_old_table);

      if (ret != FHF_INSERT_OK) {
         fhf_destroy(new_table);
         new_table_rows *= 2;
         continue;
      }
      new_table->old_table = old_table;
      *table = new_table;
      return FHF_RESIZE_OK;
   }
   return FHF_RESIZE_FAILED_INSERT;
}


#ifdef __cplusplus
}
#endif

#endif
