/*!
 * \file b_plus_tree_internal.h
 * \brief B+ tree data structure - Internal functions and structures
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
#ifndef _B_PLUS_TREE_INTERNAL_
#define _B_PLUS_TREE_INTERNAL_

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

/*!
 * \name Type of node
 *  Used to identify if the node is leaf or inner node.
 * \{ */
#define EXTEND_LEAF 1
#define EXTEND_INNER 0
 /* /} */




/*!
 * \brief Structure - B+ tree - inner node structure
 * Structure used to keep information about inner node.
 */
typedef struct bpt_nd_ext_inner_t {
   bpt_nd_t **child;      /*< pointer to descendats */

} bpt_nd_ext_inner_t;

/*!
 * \brief Structure - B+ tree - leaf node structure
 * Structure used to keep information about leaf node.
 */
typedef struct bpt_nd_ext_leaf_t {
   bpt_nd_t *left;     /*< linked list, left value */
   bpt_nd_t *right;    /*< linked list, right value */
   void **value;     /*< array of values */
} bpt_nd_ext_leaf_t;


/*!
 * \brief Copy key
 * Function which copy key from certain poineter to another.
 * \param[in] to poineter to destination.
 * \param[in] index_to position in array of destination.
 * \param[in] from poineter of source.
 * \param[in] index_from position in array of source.
 * \param[in] size_of_key size of key to copy.
 */
void copy_key(void *to, int index_to, void *from, int index_from, int size_of_key);

/*!
 * \brief Creates bpt_nd_t structure
 * Function which allocs memory for bpt_nd_t structure.
 * \param[in] size_of_key size of key to allocate memory.
 * \param[in] m count of keys to allocate.
 * \return pointer to created stucture
 */
bpt_nd_t *bpt_nd_init(int size_of_key, int m);

/*!
 * \brief Destroy bpt_nd_t structure
 * \param[in] node pointer to node
 */
void bpt_nd_clean(bpt_nd_t *node);

/*!
 * \brief is key in node
 * \param[in] key
 * \param[in] node
 * \param[in] btree pointer to b tree
 * \return 1 ON SUCCESS, OTHERWISE 0
 */
unsigned char bpt_nd_key(void *key, bpt_nd_t *node, bpt_t *btree);

/*!
 * \brief Find index in node
 * \param[in] key
 * \param[in] node
 * \param[in] btree pointer to b tree
 * \return index or -1 if key is not in node
 */
int bpt_nd_index_key(void *key, bpt_nd_t *node, bpt_t *btree);

/*!
 * \brief If node if leaf
 * \param[in] node
 * \return 1 ON SUCCESS, OTHERWISE 0
 */
unsigned char bpt_nd_leaf(bpt_nd_t *node);

/*!
 * \brief Get parent of node
 * \param[in] node
 * \return parent of node or NULL if does not have
 */
bpt_nd_t *bpt_nd_parent(bpt_nd_t *node);

/*!
 * \brief Return key from item
 * \param[in] node
 * \param[in] index index in leaf
 * \param[in] size_of_key size of key in B
 * \return poiter to key
 */
void *bpt_nd_key_on_index(bpt_nd_t *node, int index, int size_of_key);

/*!
 * \brief Creates bpt_nd_ext_leaf_t structure
 * Function which allocs memory for bpt_nd_ext_leaf_t which extends bpt_nd_t structure.
 * \param[in] m count of keys and values to allocate.
 * \param[in] size_of_value size of value to allocate memory.
 * \param[in] size_of_key size of key to allocate memory.
 * \return pointer to created bpt_nd_t stucture
 */
bpt_nd_t *bpt_ndlf_init(int m, int size_of_value, int size_of_key);

/*!
 * \brief Return value from item
 * \param[in] node
 * \param[in] index index in leaf
 * \return poiter to key
 */
void *bpt_ndlf_get_val(bpt_nd_ext_leaf_t *node, int index);

/*!
 * \brief Next leaf
 * \param[in] node leaf
 * \return poiter to next leaf or NULL if not exists
 */
bpt_nd_t *bpt_ndlf_next(bpt_nd_t *node);

/*!
 * \brief Delete item from node on specific index
 * \param[in] node leaf
 * \param[in] index item to delete
 * \param[in] size_of_key size of key in B
 * \return count of items in leaf
 */
int bpt_ndlf_del_item(bpt_nd_t *node, int index, int size_of_key);

/*!
 * \brief Add key and value to the leaf.
 * \param[in] key key
 * \param[in] node leaf
 * \param[in] btree pointer to tree
 * \param[in] pointer to memory of item
 * \return index in leaf, -1 on error
 */
int bpt_ndlf_insert(void *key, bpt_nd_t *node, bpt_t *btree,
                              void **return_value);

/*!
 * \brief Creates bpt_nd_ext_inner_t structure
 * Function which allocs memory for bpt_nd_ext_inner_t which extends bpt_nd_t structure.
 * \param[in] size_of_key size of key to allocate memory.
 * \param[in] m count of keys and values to allocate.
 * \return pointer to created bpt_nd_t stucture
 */
bpt_nd_t *bpt_ndin_init(int size_of_key, int m);

/*!
 * \brief Get child on index
 * \param[in] node
 * \param[in] index index of child
 * \return pointer to child
 */
bpt_nd_t *bpt_ndin_child(bpt_nd_t *node, int index);

/*!
 * \brief Add key to inner node
 * \param[in] add key to add
 * \param[in] left left brother
 * \param[in] right right brother
 * \param[in] node node to insert key
 * \param[in] btree pointer to tree
 * \return count of descendants
 */
int bpt_ndin_insert(void *add, bpt_nd_t *left, bpt_nd_t *right, bpt_nd_t *node,
                        bpt_t *btree);

/*!
 * \brief Recursive function to delete all nodes
 * \param[in] del node to delete
 */
void bpt_del_all(bpt_nd_t *del);

/*!
 * \brief Search item in tree
 * \param[in] key to search
 * \param[in] val pointer to leaf where will be the key
 * \param[in] btree pointer to tree
 * \return index of item in leaf
 */
int bpt_search_leaf_and_index(void *key, bpt_nd_ext_leaf_t **val, bpt_t *btree);

/*!
 * \brief Find index of child in parent
 * \param[in] son node to find index of
 * \return index of nide in parent
 */
int bpt_nd_index_in_parent(bpt_nd_t *son);

/*!
 * \brief Add key to inner node. For spliting node
 * \param[in] add key to add
 * \param[in] left left brother
 * \param[in] right right brother
 * \param[in] btree pointer to tree
 */
void bpt_ndin_insert_to_new_node(void *key, bpt_nd_t *left, bpt_nd_t *right,
                               bpt_t *btree);

/*!
 * \brief Search leaf where shuld be item.
 * \param[in] key to search
 * \param[in] btree pointer to tree
 * \return leaf where should be item
 */
bpt_nd_t *bpt_search_leaf(void *key, bpt_t *btree);

/*!
 * \brief Find or Insert key, return pointer to item
 * \param[in] key to insert
 * \param[in] btree pointer to tree
 * \param[in] search 1 - return fouded or inserted value, 0 - return value just when the key was inserted, otherwise NULL
 * \return pointer to value
 */
void *bpt_search_or_insert_inner(void *key, bpt_t *btree, int search);

/*!
 * \brief Found the most right leaf and return it
 * \param[in] inner node where to seach
 * \return pointer to leaf
 */
bpt_nd_t *bpt_nd_rightmost_leaf(bpt_nd_t *inner);

/*!
 * \brief check key and key in parent, it there is problem, repair it
 * \param[in] node to check
 * \param[in] btree pointer to tree
 */
void bpt_nd_check(bpt_nd_t *node, bpt_t *btree);

/*!
 * \brief Delete item from tree, know leaf
 * \param[in] btree pointer to tree
 * \param[in] index item to delete
 * \param[in] leaf_del leaf where is item
 * \return 1 ON SUCCESS, OTHERWISE 0
 */
int bpt_ndlf_delete_from_tree(int index, bpt_nd_t *leaf_del, bpt_t *btree);

/*!
 * \brief check node if has the rigth value, not small and not big
 * \param[in] check node to check
 * \param[in] btree pointer to tree
 */
void bpt_ndin_check(bpt_nd_t *check, bpt_t *btree);

/*!
 * \brief Found the rightest leaf and return it
 * \param[in] inner node where to seach
 * \return pointer to leaf
 */
bpt_nd_t *bpt_nd_leftmost_leaf(bpt_nd_t *item);

#endif            /* _B_PLUS_TREE_INTERNAL_ */