/**
 * \file cuckoo_hash.c
 * \brief Generic hash table with Cuckoo hashing support.
 * \author Roman Vrana, xvrana20@stud.fit.vutbr.cz
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cuckoo_hash_v2.h"
#include "hashes_v2.h"

/**
 * Initialization function for the hash table.
 * Function gets the pointer to the structure with the table and creates a new
 * one according to table_size parameter. The data_size parameter serves in
 * table operations for data manipulation.
 *
 * @param new_table Pointer to a table structure.
 * @param table_size Size of the newly created table.
 * @param data_size Size of the data being stored in the table.
 * @param key_length Length of the key used for referencing the items.
 * @return -1 if the table wasn't created, 0 otherwise.
 */
int ht_init_v2(cc_hash_table_v2_t* new_table, unsigned int table_size, unsigned int data_size, unsigned int key_length)
{
    // allocate indexing array
    new_table->ind = (index_array_t *) calloc(table_size, sizeof(index_array_t));
    // test if the memory was allocated
    if (new_table->ind == NULL) {
        fprintf(stderr, "ERROR: Hash table couldn't be initialized.\n");
        return -1;
    }

    // prepare indexes
    for (int i = 0;  i < table_size; i++) {
        new_table->ind[i].index = i;
    }

    // allocate keys
    new_table->keys = (char **) calloc(table_size, sizeof(char *));
    if (new_table->keys) {
        for(int i = 0; i < table_size; i++) {
            new_table->keys[i] = (char *) calloc(key_length, sizeof(char));
            if (new_table->keys[i] == NULL) {
                fprintf(stderr, "ERROR: Hash table couldn't be initialized.\n");
                return -1;
            }
        }
    }

    // allocate data
    new_table->data = (void **) calloc(table_size, sizeof(void *));
    if (new_table->data) {
        for(int i = 0; i < table_size; i++) {
            new_table->data[i] = (void *) calloc(1, data_size);
            if (new_table->data[i] == NULL) {
                fprintf(stderr, "ERROR: Hash table couldn't be initialized.\n");
                return -1;
            }
        }
    }

    new_table->data_kick = calloc(1, data_size);
    new_table->key_kick = (char *) calloc(key_length, sizeof(char));

    // set data size and current table size
    new_table->data_size = data_size;
    new_table->table_size = table_size;
    new_table->key_length = key_length;

    return 0;
}

/**
 * Function for resizing/rehashing the table.
 * This function is called only when the table is unable to hold anymore.
 * items. It resizes the old table using double the capacity and rehashes
 * all items that were stored in the old table so far. Then it inserts the
 * item that would be discarded. If this function fails the table cannot be
 * used anymore and the program cannot continue due the lack of memory.
 *
 * @param ht Table to be resized and rehashed.
 * @return 0 on succes otherwise REHASH_FAILURE.
 */
int rehash_v2(cc_hash_table_v2_t* ht)
{
    cc_hash_table_v2_t new_ht;
    int ret;

    // create new table twice as large as the old one
    ret = ht_init_v2(&new_ht, ht->table_size * 2, ht->data_size, ht->key_length);
    if (ret != 0) {
        return REHASH_FAILURE;
    }

    // move all data to new table
    for (int i = 0; i < ht->table_size; i++) {
        if (ht->ind[i].valid) {
            ht_insert_v2(&new_ht, ht->keys[ht->ind[i].index], ht->data[ht->ind[i].index]);
        }
    }

    // destroy the old table
    ht_destroy_v2(ht);

    // swap the tables
    *ht = new_ht;
    return 0;
}

/**
 * Function for inserting the item into the table.
 * This function performs the insertion operation using the "Cuckoo hashing"
 * algorithm. It computes the one hash for the item and tries to insert it
 * on the retrieved position. If th position indexed by index array is empty
 * (valid bit is 0) then the item is inserted without any other opertaions.
 * If the position already occupied then the items are "swapped" until empty
 * position is found (swapping occures only with indexes, data are kept intact).
 * If no position is found then the last remaining item is returned as kicked
 * out.
 *
 * @param ht Table in which we want to insert the item.
 * @param key Key of the newly inserted data.
 * @param new_data Pointer to new data to be inserted.
 * @return NULL on success, pointer to data that have been kicked out otherwise.
 */
void* ht_insert_v2(cc_hash_table_v2_t* ht, char *key, const void *new_data)
{

    int pos_n, pos_i, pos_k = -1, pos_s;
    int swap1, swap2, swap3;
    int ttl = 15;

    // compute hash for the inserted item
    pos_i = pos_n = hash_1(key, ht->key_length, ht->table_size);


    // position is empty --> insert item
    if (ht->ind[pos_i].valid == 0) {
        memcpy(ht->keys[ht->ind[pos_i].index], key, ht->key_length);
        memcpy(ht->data[ht->ind[pos_i].index], new_data, ht->data_size);
        ht->ind[pos_i].valid = 1;
        return NULL;
    } else {
        // we need suitable position --> do the swapping magic :)
        for (; ttl > 0; ttl--) {
            // we found viable position --> finish swapping the indexes
            if (ht->ind[pos_i].valid == 0) {
                pos_s = ht->ind[pos_i].index;
                ht->ind[pos_i].index = pos_k;
                ht->ind[pos_i].valid = 1;
                pos_i = pos_s;
                break;
            }

            // swap indexes of the inserted/moved item with kicked item
            pos_s = ht->ind[pos_i].index;
            ht->ind[pos_i].index = pos_k;
            pos_k = pos_s;

            // we went out from the table -- don't try to find position anymore
            if (pos_k == -1) {
                ttl = 0;
                break;
            }
            // compute both hashes
            swap1 = hash_1(ht->keys[pos_k], ht->key_length, ht->table_size);
            swap2 = hash_2(ht->keys[pos_k], ht->key_length, ht->table_size);
            swap3 = hash_3(ht->keys[pos_k], ht->key_length, ht->table_size);

            // determine the correct hash to use
            if (swap1 == pos_i) {
                pos_i = swap2;
            } else if (swap2 == pos_i) {
                pos_i = swap3;
            } else {
                pos_i = swap1;
            }
        }
    }

    // found a viable position --> insert the item and index it
    if (ttl > 0) {
        ht->ind[pos_n].index = pos_i;
        memcpy(ht->keys[ht->ind[pos_n].index], key, ht->key_length);
        memcpy(ht->data[ht->ind[pos_n].index], new_data, ht->data_size);
        ht->ind[pos_n].valid = 1;
        return NULL;
    } else {
        // no viable position was found --> last item goes out
        if (pos_k < 0) {
            // index out of table -- kick the first item
            memcpy(ht->key_kick, ht->keys[ht->ind[pos_n].index], ht->key_length);
            memcpy(ht->data_kick, ht->data[ht->ind[pos_n].index], ht->data_size);
        } else {
            // copy the kicked item
            memcpy(ht->key_kick, ht->keys[pos_k], ht->key_length);
            memcpy(ht->data_kick, ht->data[pos_k], ht->data_size);
            ht->ind[pos_n].index = pos_k;
        }

        // insert the new item where possible
        memcpy(ht->keys[ht->ind[pos_n].index], key,  ht->key_length);
        memcpy(ht->data[ht->ind[pos_n].index], new_data, ht->data_size);
        ht->ind[pos_n].valid = 1;

        return ht->data_kick;
    }
}


/**
 * Function for inserting the item into the table with locking during insertion.
 * This function performs the insertion operation using the "Cuckoo hashing"
 * algorithm. It computes the one hash for the item and tries to insert it
 * on the retrieved position. If th position indexed by index array is empty
 * (valid bit is 0) then the item is inserted without any other opertaions.
 * If the position already occupied then the items are "swapped" until empty
 * position is found (swapping occures only with indexes, data are kept intact).
 * If no position is found then the last remaining item is returned as kicked
 * out. This function also performs locking operation before inserting new values.
 *
 * @param ht Table in which we want to insert the item.
 * @param key Key of the newly inserted data.
 * @param new_data Pointer to new data to be inserted.
 * @param lock Pointer to function performing locking operation.
 * @param unlock Pointer to function performing unlocking operation.
 * @return NULL on success, pointer to data that have been kicked out otherwise.
 */
void* ht_lock_insert_v2(cc_hash_table_v2_t *ht, char *key, const void *new_data, void (*lock)(int), void (*unlock)(int))
{

    int pos_n, pos_i, pos_k = -1, pos_s;
    int swap1, swap2, swap3;
    int ttl = 15;

    // compute hash for the inserted item
    pos_i = pos_n = hash_1(key, ht->key_length, ht->table_size);

    // position is empty --> insert item
    if (ht->ind[pos_i].valid == 0) {
        lock(ht->ind[pos_i].index); // lock before inserting
        memcpy(ht->keys[ht->ind[pos_i].index], key, ht->key_length);
        memcpy(ht->data[ht->ind[pos_i].index], new_data, ht->data_size);
        ht->ind[pos_i].valid = 1;
        unlock(ht->ind[pos_i].index); // unlock
        return NULL;
    } else {
        // we need suitable position --> do the swapping magic :)
        for (; ttl > 0; ttl--) {
            // we found viable position --> finish swapping the indexes
            if (ht->ind[pos_i].valid == 0) {
                pos_i = ht->ind[pos_i].index;
                ht->ind[pos_i].index = pos_k;
                break;
            }

            // swap indexes of the inserted/moved item with kicked item
            pos_s = ht->ind[pos_i].index;
            ht->ind[pos_i].index = pos_k;
            pos_k = pos_s;

            // we went out from the table -- don't try to find position anymore
            if (pos_k == -1) {
                ttl = 0;
                break;
            }

            // compute both hashes
            swap1 = hash_1(ht->keys[pos_k], ht->key_length, ht->table_size);
            swap2 = hash_2(ht->keys[pos_k], ht->key_length, ht->table_size);
            swap3 = hash_3(ht->keys[pos_k], ht->key_length, ht->table_size);

            // determine the correct hash to use
            if (swap1 == pos_i) {
                pos_i = swap2;
            } else if (swap2 == pos_i) {
                pos_i = swap3;
            } else {
                pos_i = swap1;
            }
        }
    }

    // found a viable position --> insert the item and index it
    if (ttl > 0) {
        ht->ind[pos_n].index = pos_i;
        lock(ht->ind[pos_i].index); // lock before inserting
        memcpy(ht->keys[ht->ind[pos_n].index], key, ht->key_length);
        memcpy(ht->data[ht->ind[pos_n].index], new_data, ht->data_size);
        ht->ind[pos_i].valid = 1;
        unlock(ht->ind[pos_i].index); // unlock
        return NULL;
    } else {
        // no viable position was found --> last item goes out

        if (pos_k < 0) {
            lock(ht->ind[pos_n].index); // lock before inserting
            // index out of table -- kick the first item
            memcpy(ht->key_kick, ht->keys[ht->ind[pos_n].index], ht->key_length);
            memcpy(ht->data_kick, ht->data[ht->ind[pos_n].index], ht->data_size);
            unlock(ht->ind[pos_n].index); // unlock
        } else {
            lock(ht->ind[pos_k].index); // lock before inserting
            // copy the kicked item
            memcpy(ht->key_kick, ht->keys[ht->ind[pos_k].index], ht->key_length);
            memcpy(ht->data_kick, ht->data[ht->ind[pos_k].index], ht->data_size);
            ht->ind[pos_n].index = pos_k;
            unlock(ht->ind[pos_k].index); // unlock
        }

        // insert the new item where possible
        lock(ht->ind[pos_n].index); // lock before inserting
        memcpy(ht->keys[ht->ind[pos_n].index], key,  ht->key_length);
        memcpy(ht->data[ht->ind[pos_n].index], new_data, ht->data_size);
        ht->ind[pos_n].index = pos_n;
        unlock(ht->ind[pos_n].index); // unlock

        // insert the new item where possible
        return ht->data_kick;
    }
}

/**
 * Function for checking data validity.
 * Function checks whether the item with given key is present in table
 * or it was already erased.
 *
 * @param ht Table to check for the item.
 * @param key Key of the checked item.
 * @param index Supposed index of the item.
 * @return 1 if item is on the given index in table 0 otherwise.
 */
int ht_is_valid_v2(cc_hash_table_v2_t* ht, char* key, int index)
{
    int h1, h2, h3;
    h1 = hash_1(key, ht->key_length, ht->table_size);

    if (ht->ind[h1].index == index && ht->ind[h1].valid == 1) {
        return 1;
    }
    h2 = hash_2(key, ht->key_length, ht->table_size);
    if (ht->ind[h2].index == index && ht->ind[h2].valid == 1) {
        return 1;
    }
    h3 = hash_3(key, ht->key_length, ht->table_size);
    if (ht->ind[h3].index == index && ht->ind[h3].valid == 1) {
        return 1;
    }
    return 0;
}

/**
 * Function for getting the data from table.
 * Function computes both hashes for the given key and checks the positions
 * for the desired data. Pointer to the data is returned when found.
 *
 * @param ht Hash table to be searched for data.
 * @param key Key of the desired item.
 * @return Pointer to the data when found otherwise NULL.
 */
void *ht_get_v2(cc_hash_table_v2_t* ht, char* key)
{
    unsigned int pos1, pos2, pos3;

    pos1 = hash_1(key, ht->key_length, ht->table_size);

    if (ht->ind[pos1].valid == 1 && memcmp(key, ht->keys[ht->ind[pos1].index], ht->key_length) == 0) {
        return ht->data[ht->ind[pos1].index];
    }

    pos2 = hash_2(key, ht->key_length, ht->table_size);
    if (ht->ind[pos2].valid == 1 && memcmp(key, ht->keys[ht->ind[pos2].index], ht->key_length) == 0) {
        return ht->data[ht->ind[pos2].index];
    }

    pos3 = hash_3(key, ht->key_length, ht->table_size);
    if (ht->ind[pos3].valid == 1 && memcmp(key, ht->keys[ht->ind[pos3].index], ht->key_length) == 0) {
        return ht->data[ht->ind[pos3].index];
    }
    return NULL;
}

/**
 * Function for getting the index of the item in table.
 * Function computes both hashes for the given key and checks the positions
 * for the desired item. Index is returned when found.
 *
 * @param ht Hash table to be searched for data.
 * @param key Key of the desired item.
 * @return Index of the item in table otherwise -1.
 */
int ht_get_index_v2(cc_hash_table_v2_t* ht, char* key)
{
    unsigned int pos1, pos2, pos3;

    pos1 = hash_1(key, ht->key_length, ht->table_size);

    if (ht->ind[pos1].valid == 1 && memcmp(key, ht->keys[ht->ind[pos1].index], ht->key_length) == 0) {
        return ht->ind[pos1].index;
    }

    pos2 = hash_2(key, ht->key_length, ht->table_size);
    if (ht->ind[pos2].valid == 1 && memcmp(key, ht->keys[ht->ind[pos2].index], ht->key_length) == 0) {
        return ht->ind[pos2].index;
    }

    pos3 = hash_3(key, ht->key_length, ht->table_size);
    if (ht->ind[pos3].valid == 1 && memcmp(key, ht->keys[ht->ind[pos3].index], ht->key_length) == 0) {
        return ht->ind[pos3].index;
    }
    return -1;
}

/**
 * Procedure for removing the item from table.
 * Procedure searches for the data using the given key and invalidates the valid
 * variable for the item. If the item is already empty the procedure does nothing.
 *
 * @param ht Hash table to be searched for data.
 * @param key Key of the desired item.
 */
void ht_remove_by_key_v2(cc_hash_table_v2_t* ht, char* key)
{
    unsigned int pos1, pos2, pos3;
    pos1 = hash_1(key, ht->key_length, ht->table_size);

    if (ht->ind[pos1].valid == 1 && memcmp(key, ht->keys[ht->ind[pos1].index], ht->key_length) == 0) {
        ht->ind[pos1].valid = 0;
        return;
    }

    pos2 = hash_2(key, ht->key_length, ht->table_size);
    if (ht->ind[pos2].valid == 1 && memcmp(key, ht->keys[ht->ind[pos2].index], ht->key_length) == 0) {
        ht->ind[pos2].valid = 0;
        return;
    }

    pos3 = hash_3(key, ht->key_length, ht->table_size);
    if (ht->ind[pos3].valid == 1 && memcmp(key, ht->keys[ht->ind[pos3].index], ht->key_length) == 0) {
        ht->ind[pos3].valid = 0;
        return;
    }
}

/**
 * Procedure for removing the item from table.
 * Procedure removes the item based on precomputed hashes. Rest of the procedure is
 * same as normal removal procedure.
 *
 * @param ht Hash table to be searched for data.
 * @param key Key of the desired iteim.
 * @param h1 Precomputed position from first hash.
 * @param h2 Precomputed position from second hash.
 * @param h3 Precomputed position from third hash.
 */
void ht_remove_precomp_v2(cc_hash_table_v2_t* ht, char* key, unsigned int h1, unsigned int h2, unsigned int h3)
{
    if (ht->ind[h1].valid == 1 && memcmp(key, ht->keys[ht->ind[h1].index], ht->key_length) == 0) {
        ht->ind[h1].valid = 0;
        return;
    }
    if (ht->ind[h2].valid == 1 && memcmp(key, ht->keys[ht->ind[h2].index], ht->key_length) == 0) {
        ht->ind[h2].valid = 0;
        return;
    }
    if (ht->ind[h3].valid == 1 && memcmp(key, ht->keys[ht->ind[h3].index], ht->key_length) == 0) {
        ht->ind[h3].valid = 0;
        return;
    }
}

/**
 * Procedure for cleaning the table
 * Procedure removes all items from the table keeping the table intact.
 * Removal is done only by invalidating indexes in index array.
 * This procedure DOES NOT free any memory.
 *
 * @param ht Table to be cleared.
 */

void ht_clear_v2(cc_hash_table_v2_t *ht)
{
    for(int i = 0; i < ht->table_size; i++) {
        ht->ind[i].valid = 0;
        ht->ind[i].index = i;
    }
}


/**
 * Clean-up procedure.
 * Procedure goes frees all the memory used by the table.
 *
 * @param ht Hash table to be destroyed.
 */
void ht_destroy_v2(cc_hash_table_v2_t *ht)
{

    for (int i = 0; i < ht->table_size; i++) {
        free(ht->data[i]);
        free(ht->keys[i]);
        ht->data[i] = NULL;
        ht->keys[i] = NULL;
    }

    free(ht->data);
    ht->data = NULL;

    free(ht->keys);
    ht->keys = NULL;

    free(ht->ind);
    ht->ind = NULL;

    free(ht->data_kick);
    ht->data_kick = NULL;

    free(ht->key_kick);
    ht->key_kick = NULL;

    ht->data_size = 0;
    ht->table_size = 0;
    ht->key_length = 0;
}
