/**
 * \file cuckoo_hash.c
 * \brief Generic hash table with Cuckoo hashing support.
 * \author Roman Vrana, xvrana20@stud.fit.vutbr.cz
 * \author Tomas Cejka, cejkat@cesnet.cz
 * \date 2013
 * \date 2014
 * \date 2015
 */
/*
 * Copyright (C) 2013-2015 CESNET
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
#include "../include/cuckoo_hash.h"
#include "hashes.h"

/**
 * Initialization function for the hash table.
 * Function gets the pointer to the structure with the table and creates a new
 * one according to table_size parameter. The data_size parameter serves in
 * table operations for data manipulation.
 *
 * @param new_table Pointer to a table structure.
 * @param table_size Size of the newly created table.
 * @param data_size Size of the data being stored in the table.
 * @param key_length Length of the key used for refernecing the items.
 * @param rehash Enable/disable rehash feature.
 * @return -1 if the table wasn't created, 0 otherwise.
 */
int ht_init(cc_hash_table_t *new_table, unsigned int table_size, unsigned int data_size, unsigned int key_length, int rehash)
{
    if (new_table == NULL) {
        fprintf(stderr, "ERROR: Bad parameter, initialization failed.\n");
        return -1;
    }
    // allocate the table itself
    new_table->table = (cc_item_t*) calloc(table_size, sizeof(cc_item_t));

    // test if the memory was allocated
    if (new_table->table == NULL) {
        fprintf(stderr, "ERROR: Hash table couldn't be initialized.\n");
        return -1;
    }

    // set data size and current table size
    new_table->data_size = data_size;
    new_table->table_size = table_size;
    new_table->key_length = key_length;
    new_table->rehash = rehash;
    new_table->item_count = 0;
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
 * @param rest Item to be inserted after rehashing.
 * @return 0 on succes otherwise REHASH_FAILURE.
 */
int rehash(cc_hash_table_t* ht, cc_item_t* rest)
{
    cc_item_t *old_table, *new_table;

    // allocate new table
    new_table = (cc_item_t*) calloc((ht->table_size * 2), sizeof(cc_item_t));

    if (new_table == NULL) {
        fprintf(stderr, "ERROR: Hash table cannot be extended. Unable to continue.\n");
        return -1;
    }

    unsigned int old_size = ht->table_size;

    old_table = ht->table;
    ht->table = new_table;
    ht->table_size *= 2;

    // rehash and copy items from old table to new one
    for (int i = 0; i < old_size; i++) {
        if (old_table[i].key != NULL && old_table[i].data != NULL) {
            ht_insert(ht, old_table[i].key, old_table[i].data, old_table[i].key_length);
        }
    }

    // insert the remaining item
    ht_insert(ht, rest->key, rest->data, rest->key_length);

    // destroy old table
    for(int i = 0; i < old_size; i++) {
        if (old_table[i].key != NULL) {
            free(old_table[i].key);
            old_table[i].key = NULL;
        }
        if (old_table[i].data != NULL) {
            free(old_table[i].data);
            old_table[i].data = NULL;
        }
    }
    free(old_table);

    return 0;
}

/**
 * Function for inserting the item into the table.
 * This function perform the insertion operation using the "Cuckoo hashing"
 * algorithm. It computes the one hash for the item and tries to insert it
 * on the retrieved position. If the positio is empty the item is inserted
 * without any other necessary operations. However if the position is already
 * occupied the residing item is kicked out and replaced with the currently
 * inserted item. Then the funstion computes both hashes for the kicked out
 * item and determines which one to use alternatively. Then it tries to insert
 * this item using the same method. For preventing infinite loop the "TTL" value
 * is implemented. If this is exceeded the table is resized and rehashed using
 * the rehash() function.
 *
 * @param ht Table in which we want to insert the item.
 * @param key Key of the newly inserted data.
 * @param new_data Pointer to new data to be inserted.
 * @param n_key_length Length of the key of the new item.
 * @return 0 on succes otherwise REHASH_FAILURE when the rehashing function fails.
 */
int ht_insert(cc_hash_table_t* ht, char *key, const void *new_data, unsigned int n_key_length)
{
    int t, ret = 0;
    unsigned int pos, swap1, swap2, swap3;
    pos = hash_1(key, n_key_length, ht->table_size);

    cc_item_t prev, curr;

    // prepare memory for storing "kicked" values
    prev.key = NULL;
    prev.data = NULL;

    // prepare memory for data
    if (ht->key_length == 0) {
        curr.key = calloc(n_key_length, sizeof(char));
    } else {
        curr.key = calloc(ht->key_length, sizeof(char));
    }
    curr.key_length = n_key_length;
    curr.data = calloc(ht->data_size, sizeof(char));

    if (curr.key == NULL || curr.data == NULL) {
        fprintf(stderr, "ERROR: No memory available for another data. Item will be discarded.\n");
        return INSERT_FAILURE;
    }

    // make a working copy of inserted data
    memcpy(curr.key, key, n_key_length);
    memcpy(curr.data, new_data, ht->data_size);

    for (t = 1; t <= 15; t++) {
        if (ht->table[pos].data == NULL && ht->table[pos].key == NULL) { // try empty
            // we insert a new value into the table

            // assign data and key pointers
            ht->table[pos].key = curr.key;
            ht->table[pos].key_length = curr.key_length;
            ht->table[pos].data = curr.data;
            ++ht->item_count;

            return 0;
        }

        // computed position is occupied --> we kick the residing item out
        prev.key = ht->table[pos].key;
        prev.key_length = ht->table[pos].key_length;
        prev.data = ht->table[pos].data;

        //copy new item
        ht->table[pos].key = curr.key;
        ht->table[pos].key_length = curr.key_length;
        ht->table[pos].data = curr.data;

        // compute both hashses for kicked item
        swap1 = hash_1(prev.key, prev.key_length, ht->table_size);
        swap2 = hash_2(prev.key, prev.key_length, ht->table_size);
        swap3 = hash_3(prev.key, prev.key_length, ht->table_size);

        // test which one was used
        if (swap1 == pos) {
            pos = swap2;
        } else if (swap2 == pos) {
            pos = swap3;
        } else {
            pos = swap1;
        }

        // prepare the item for insertion
        curr.key = prev.key;
        curr.key_length = prev.key_length;
        curr.data = prev.data;
    }

    // TTL for insertion exceeded, rehash is needed

    // rehash the table and return the appropriate value is succesful
    if (ht->rehash == REHASH_ENABLE) {
        ret = rehash(ht, &curr);
        if (!ret) {
            ++ht->item_count;
        }
    }
    ret = 20;
    free(curr.data);
    free(curr.key);
    return ret;

}

/**
 * Function for checking whether the table has any items.
 * @param ht Hash table to check.
 * @return 0 if false, any other value if true.
 */
int ht_is_empty(cc_hash_table_t* ht)
{
    return ht->item_count == 0;
}

/**
 * Function for getting the data from table.
 * Function computes both hashes for the given key and checks the positions
 * for the desired data. Pointer to the data is returned when found.
 *
 * @param ht Hash table to be searched for data.
 * @param key Key of the desired item.
 * @param key_length Length of the key of the searched item.
 * @return Pointer to the data when found otherwise NULL.
 */
void *ht_get(cc_hash_table_t* ht, char* key, unsigned int key_length)
{
    unsigned int pos1, pos2, pos3;

    pos1 = hash_1(key, key_length, ht->table_size);
    pos2 = hash_2(key, key_length, ht->table_size);
    pos3 = hash_3(key, key_length, ht->table_size);

    if (ht->table[pos1].data != NULL && memcmp(key, ht->table[pos1].key, key_length) == 0) {
        return ht->table[pos1].data;
    }
    if (ht->table[pos2].data != NULL && memcmp(key, ht->table[pos2].key, key_length) == 0) {
        return ht->table[pos2].data;
    }
    if (ht->table[pos3].data != NULL && memcmp(key, ht->table[pos3].key, key_length) == 0) {
        return ht->table[pos3].data;
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
 * @param key_length Length of the key of the searched item.
 * @return Index of the item in table otherwise -1.
 */
int ht_get_index(cc_hash_table_t* ht, char* key, unsigned int key_length)
{
    unsigned int pos1, pos2, pos3;

    pos1 = hash_1(key, key_length, ht->table_size);
    pos2 = hash_2(key, key_length, ht->table_size);
    pos3 = hash_3(key, key_length, ht->table_size);

    if (ht->table[pos1].data != NULL && memcmp(key, ht->table[pos1].key, key_length) == 0) {
        return pos1;
    }
    if (ht->table[pos2].data != NULL && memcmp(key, ht->table[pos2].key, key_length) == 0) {
        return pos2;
    }
    if (ht->table[pos3].data != NULL && memcmp(key, ht->table[pos3].key, key_length) == 0) {
        return pos3;
    }
    return -1;
}

/**
 * Procedure for removing the item from table.
 * Procedure searches for the data using the given key and frees any memory
 * used by the item. If the item is already empty the procedure does nothing.
 *
 * @param ht Hash table to be searched for data.
 * @param key_length Length of the key of the removed item.
 * @param key Key of the desired item.
 */
void ht_remove_by_key(cc_hash_table_t* ht, char* key, unsigned int key_length)
{
    unsigned int pos1, pos2, pos3;
    pos1 = hash_1(key, key_length, ht->table_size);
    pos2 = hash_2(key, key_length, ht->table_size);
    pos3 = hash_3(key, key_length, ht->table_size);

    if (ht->table[pos1].data != NULL && (memcmp(key, ht->table[pos1].key, key_length) == 0)) {
        free(ht->table[pos1].data);
        free(ht->table[pos1].key);
        ht->table[pos1].data = NULL;
        ht->table[pos1].key = NULL;
        --ht->item_count;
        return;
    }

    if (ht->table[pos2].data != NULL && (memcmp(key, ht->table[pos2].key, key_length) == 0)) {
        free(ht->table[pos2].data);
        free(ht->table[pos2].key);
        ht->table[pos2].data = NULL;
        ht->table[pos2].key = NULL;
        --ht->item_count;
        return;
    }
    if (ht->table[pos3].data != NULL && (memcmp(key, ht->table[pos3].key, key_length) == 0)) {
        free(ht->table[pos3].data);
        free(ht->table[pos3].key);
        ht->table[pos3].data = NULL;
        ht->table[pos3].key = NULL;
        --ht->item_count;
        return;
    }
}

/**
 * Procedure for removing the item from table (index version).
 * Procedure removes the item pointed by the index from the table.
 * If the item is already empty the procedure does nothing.
 *
 * @param ht Hash table to be searched for data.
 * @param index Index of item to be removed.
 */
void ht_remove_by_index(cc_hash_table_t* ht, unsigned int index)
{
    if (ht->table[index].data != NULL && ht->table[index].key != NULL) {
        free(ht->table[index].data);
        free(ht->table[index].key);
        ht->table[index].data = NULL;
        ht->table[index].key = NULL;
        --ht->item_count;
    }
}

/**
 * Procedure for cleaning the table
 * Procedure removes all items from the table keeping the table intact.
 *
 * @param ht Table to be cleared.
 */

void ht_clear(cc_hash_table_t *ht)
{
    for(int i = 0; i < ht->table_size; i++) {
        if (ht->table[i].key != NULL) {
            free(ht->table[i].key);
            ht->table[i].key = NULL;
        }
        if (ht->table[i].data != NULL) {
            free(ht->table[i].data);
            ht->table[i].data = NULL;
        }
        ht->table[i].key_length = 0;
        ht->item_count = 0;
    }
}


/**
 * Clean-up procedure.
 * Procedure goes frees all the memory used by the table.
 *
 * @param ht Hash table to be searched for data.
 */
void ht_destroy(cc_hash_table_t *ht)
{
    ht_clear(ht);
    free(ht->table);
}
