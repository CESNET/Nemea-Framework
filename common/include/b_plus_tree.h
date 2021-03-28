/*!
 * \file b_plus_tree.h
 * \brief B+ tree data structure
 * \author Zdenek Rosa <rosazden@fit.cvut.cz>
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
#ifndef _B_PLUS_TREE_
#define _B_PLUS_TREE_

/*!
 * \name Return codes from compare function
 * Function for comparing two keys shod return one of these values.
 * Example function for int values:
 * int compare_int(void *key1, void *key2)
 * {
 *    int a = *((int*)key1), b = *((int*)key2);
 *    if (a < b) {
 *       return LESS;
 *    } else if (a > b) {
 *       return MORE;
 *    } else {
 *       return EQUAL;
 *    }
 * }
 * \{ */
#define EQUAL 0
#define LESS -1
#define MORE 1
 /* /} */

// Public Structures
// -----------------

typedef struct bpt_nd_t bpt_nd_t;

/*!
 * \brief Structure - B+ tree - main structure
 * Structure used to keep informations about tree.
 */
typedef struct bpt_t {
   unsigned long int count_of_values;  /*< count of values in tree */
   int m;                              /*< count of descendant in node */
   int size_of_value;                  /*< size of value */
   int size_of_key;                    /*< size of key */
   bpt_nd_t *root;                     /*< root node */
   int (*compare) (void *, void *);    /*< compare function for key */
} bpt_t;

/*!
 * \brief Structure - B+ tree - node structure
 * Structure used to keep information about node and pointer to inner or leaf node.
 */
struct bpt_nd_t {
   void *extend;                 /*< pointer to leaf or inner node */
   unsigned char state_extend;   /*< type of extended variable. leaf or inner node */
   bpt_nd_t *parent;             /*< pointer to parent */
   void *key;                    /*< pointer to key */
   int count;                    /*< count of descendants */
};

/*!
 * \brief Structure - B+ tree - list item structure
 * Structure used to create list of items.
 */
typedef struct bpt_list_item_t {
   void *value;                  /*< pointer to value */
   void *key;                    /*< pointer to key */
   bpt_nd_t *leaf;               /*< pointer to actual leaf */
   unsigned int index_of_value;  /*< index of value in actual leaf */
} bpt_list_item_t;

// Main Functions
// --------------
// Functions for basic usage of the tree:
// initialization, destruction, inserting items, searching items, deleting items

/*!
 * \brief Initialization of B+ tree
 * Function creates main structure of the B+ tree.
 * \param[in] size_of_btree_node count of items in one node.
 * \param[in] comp pointer to key comparing function.
 *     Function should have two attributes (to get the keys),
 *     then it should return value EQUAL, MORE or LESS.
 * \param[in] size_of_key size of key.
 * \param[in] size_of_value size of value.
 * \return pointer to created bpt_t structure.
 */
bpt_t *bpt_init(unsigned int size_of_btree_node,
                             int (*comp)(void *, void *),
                             unsigned int size_of_value,
                             unsigned int size_of_key);

/*!
 * \brief Destroy B+ tree
 * Function removes all the keys, values and nodes in the tree.
 * The main structure is than deallocated
 * \param[in] btree pointer to B+ tree main structure
 */
void bpt_clean(bpt_t *btree);

/*!
 * \brief Insert or find item in B+ tree
 * Function tries to find key in the tree. If the key is found, the
 * appropriate value is returned. Otherwise it inserts key into the
 * tree and allocates and zeroes space for a new value which is
 * returned.
 * \param[in] btree pointer to B+ tree.
 * \param[in] key key to insert.
 * \return pointer to the value. New allocated or found in the tree.
 */
void *bpt_search_or_insert(bpt_t *btree, void *key);

/*!
 * \brief Insert item to B+ tree
 * Function inserts key to the tree and allocates and zeroes space for
 * value, which is returned. If the key is already in the tree, the
 * NULL is returned.
 * \param[in] btree pointer to B+ tree.
 * \param[in] key key to insert.
 * \return pointer to value or NULL.
 */
void *bpt_insert(bpt_t *btree, void *key);

/*!
 * \brief Search item in B+ tree
 * Function find item in the tree and returns it's value.
 * If the key is not in the tree, NULL is returned.
 * \param[in] btree pointer to B+ tree.
 * \param[in] key key to search.
 * \return pointer to value or NULL.
 */
void *bpt_search(bpt_t *btree, void *key);


/*!
 * \brief Remove item from B+ tree
 * Function searches key in B+ tree and removes
 * the item from it.
 * \param[in] btree pointer to B+ tree.
 * \param[in] key key to delete.
 * \return 1 ON SUCCESS, OTHERWISE 0.
 */
int bpt_item_del(bpt_t *btree, void *key);

/*!
 * \brief Items count in B+ tree
 * Function returns count of item in B+ tree.
 * (Getter of btree->count_of_values)
 * \param[in] btree pointer to B+ tree.
 * \return count of items.
 */
unsigned long int bpt_item_cnt(bpt_t *btree);

// LIST
// ----
// Iterating through all the items in the tree.
// Items are sorted by their key.

/*!
 * \brief Initialization of iteration structure.
 * Function allocates structure used for iteration.
 * \param[in] btree pointer to B+ tree.
 * \return pointer to the allocated structure.
 */
bpt_list_item_t *bpt_list_init(bpt_t *btree);

/*!
 * \brief Reset iteration
 * Function reset the iteration structure to point to the first
 * item in the sorted list.
 * Key is than stored in item->key and value in item->value.
 * \param[in] tree pointer to B+ tree.
 * \param[out] item pointer to iteration structure.
 * \return 1 ON SUCCESS,  0 tree is empty.
 */
int bpt_list_start(bpt_t *tree, bpt_list_item_t *item);


/*!
 * \brief Get next item from list
 * Function sets the iteration structure to next item in the list.
 * Actual key is than stored in item->key and value in item->value.
 * \param[in] tree pointer to B+ tree.
 * \param[inout] item pointer to iteration structure.
 * \return 1 ON SUCCESS,  0 END OF LIST.
 */
int bpt_list_item_next(bpt_t *tree, bpt_list_item_t *item);

/*!
 * \brief Remove item
 * Function removes actual item in the iteration structure and sets iteration to next item
 * in the list (if it was not the last one).
 * (The item will be removed in B+ tree. Do not use function bpt_item_del()
 * while iterating the list.)
 * \param[in] btree pointer to tree.
 * \param[inout] delete_item structure to list item.
 * \return 1 ON SUCCESS, 0 END OF LIST.
 */
int bpt_list_item_del(bpt_t *btree, bpt_list_item_t *delete_item);

/*!
 * \brief Destroy b+ tree
 * Function deallocates the iteration structure.
 * \param[in] item pointer to iteration structure.
 */
void bpt_list_clean(bpt_list_item_t *item);

#endif            /* _B_PLUS_TREE_ */

