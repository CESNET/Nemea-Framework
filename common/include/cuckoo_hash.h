/**
 * \file cuckoo_hash.h
 * \brief Generic hash table with Cuckoo hashing support -- header file.
 * \author Roman Vrana <xvrana20@stud.fit.vutbr.cz>
 * \date 2013
 * \date 2014
 */
/*
 * Copyright (C) 2013,2014 CESNET
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

#ifndef CUCKOO_HASH_H
#define CUCKOO_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Error constant returned by rehashing function when failing.
 */
#define REHASH_FAILURE -1

/**
 * Constant return by index getter when an item is not found.
 */
#define NOT_FOUND -1

/**
 * Error constant returned by insert function when failing due no memory
 */
#define INSERT_FAILURE -2

/**
 * Initialization constant for enabling rehash feature. After unsuccesful insert
 * the table will be rehashed automatically.
 */
#define REHASH_ENABLE 1

/**
 * Initialization constant for disabling rehash feature. Last item that was kicked
 * will be dropped permanently.
 */
#define REHASH_DISABLE 0

/**
 * Structure of the item of the table.
 */
typedef struct {
    /*@{*/
    char *key; /**< Key of the item (as bytes) */
    unsigned int key_length; /**< Length of the key for item */
    void *data; /**< Pointer to data (void for use with any data possible) */
    /*@}*/
} cc_item_t;

/**
 * Structure of the hash table.
 */
typedef struct {
    /*@{*/
    cc_item_t *table; /**< Array of the item representing the storage */
    unsigned int data_size; /**< Size of the data stored in every item (content of the data pointer) */
    unsigned int table_size; /**< Current size/capacity of the table */
    unsigned int key_length; /**< Length of the key used for items */
    unsigned int rehash; /**< Enabled/disabled rehash feature */
    unsigned int item_count; /**< Number of items currently stored */
    /*@}*/
} cc_hash_table_t;
/*
 * Initialization function for the table.
 */
int ht_init(cc_hash_table_t* new_table, unsigned int table_size, unsigned int data_size, unsigned int key_length, int rehash);

/*
 * Function for resizing and rehashing the table.
 */
int rehash(cc_hash_table_t* ht, cc_item_t* rest);

/*
 * Function for inserting an element.
 */
int ht_insert(cc_hash_table_t* ht, char *key, const void *new_data, unsigned int n_key_length);

/*
 * Function for checking whether the table is empty.
 */
int ht_is_empty(cc_hash_table_t* ht);

/*
 * Getters for data/index to item in table.
 */
void *ht_get(cc_hash_table_t* ht, char* key, unsigned int key_length);
int ht_get_index(cc_hash_table_t* ht, char* key, unsigned int key_length);

/*
 * Procedures for removing single item from table.
 */
void ht_remove_by_key(cc_hash_table_t* ht, char* key, unsigned int key_length);
void ht_remove_by_index(cc_hash_table_t* ht, unsigned int index);

/*
 * Procedure for removing all items from table.
 */
void ht_clear(cc_hash_table_t *ht);

/*
 * Destructor of the table.
 */
void ht_destroy(cc_hash_table_t *ht);

#ifdef __cplusplus
}
#endif

#endif
