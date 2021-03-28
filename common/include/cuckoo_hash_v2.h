/**
 * \file cuckoo_hash_v2.h
 * \brief Generic hash table with Cuckoo hashing support -- header file.
 * \author Roman Vrana, xvrana20@stud.fit.vutbr.cz
 * \date 2013
 */

#include <stdint.h>

#ifndef CUCKOO_HASH_V2_H
#define CUCKOO_HASH_V2_H

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
 * Structure of the indexing array.
 */
typedef struct {
    /*@{*/
    uint8_t valid; /**< Flag of data validity. */
    int index; /**< Index to data fields. */
    /*@}*/
} index_array_t;

/**
 * Structure of the hash table.
 */
typedef struct {
    /*@{*/
    index_array_t* ind; /**< Indexing array for data. */
    char **keys; /**< Array of keys. */
    void **data; /**< Array of data. */
    char *key_kick; /**< Key of the kicked data. */
    void *data_kick; /**< Pointer for returning kicked data. */
    unsigned int data_size; /**< Size of the data stored in every item (content of the data pointer). */
    unsigned int table_size; /**< Current size/capacity of the table. */
    unsigned int key_length; /**< Length of the key used for items. */
    /*@}*/
} cc_hash_table_v2_t;
/*
 * Initialization function for the table.
 */
int ht_init_v2(cc_hash_table_v2_t* new_table, unsigned int table_size, unsigned int data_size, unsigned int key_length);

/*
 * Function for resizing and rehashing the table.
 */
int rehash_v2(cc_hash_table_v2_t* ht);

/*
 * Function for inserting an element.
 */
void* ht_insert_v2(cc_hash_table_v2_t* ht, char *key, const void *new_data);
void* ht_lock_insert_v2(cc_hash_table_v2_t *ht, char *key, const void *new_data, void (*lock)(int), void (*unlock)(int));
/*
 * Getters for data/index to item in table.
 */
void *ht_get_v2(cc_hash_table_v2_t* ht, char* key);
int ht_get_index_v2(cc_hash_table_v2_t* ht, char* key);

/*
 * Function for checking data validity.
 */
int ht_is_valid_v2(cc_hash_table_v2_t* ht, char* key, int index);

/*
 * Procedures for removing single item from table.
 */
void ht_remove_by_key_v2(cc_hash_table_v2_t* ht, char* key);
void ht_remove_precomp_v2(cc_hash_table_v2_t* ht, char* key, unsigned int h1, unsigned int h2, unsigned int h3);

/*
 * Procedure for removing all items from table.
 */
void ht_clear_v2(cc_hash_table_v2_t *ht);

/*
 * Destructor of the table.
 */
void ht_destroy_v2(cc_hash_table_v2_t *ht);

#ifdef __cplusplus
}
#endif

#endif
