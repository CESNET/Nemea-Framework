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
 *  It is used to identify if the node is leaf or inner node.
 * \{ */
#define EXTEND_LEAF 1
#define EXTEND_INNER 0
 /* /} */




/*!
 * \brief B+ tree inner node structure
 * Structure is used to keep information about inner node.
 */
typedef struct bpt_nd_ext_inner_t {
   bpt_nd_t **child;   /*< pointer to descendants */

} bpt_nd_ext_inner_t;

/*!
 * \brief B+ tree leaf node structure
 * Structure is used to keep information about leaf node.
 */
typedef struct bpt_nd_ext_leaf_t {
   bpt_nd_t *left;   /*< pointer to left leaf */
   bpt_nd_t *right;  /*< pointer to right leaf */
   void **value;     /*< array of values */
} bpt_nd_ext_leaf_t;


/*!
 * \brief Copy key
 * Function copies key from key array to another key array.
 * Indexes in array have to be specified.
 * \param[in] to pointer to destination array.
 * \param[in] index_to position in destination array.
 * \param[in] from pointer of source array.
 * \param[in] index_from position in source array.
 * \param[in] size_of_key size of key to copy.
 */
void bpt_copy_key(void *to, int index_to, void *from, int index_from, int size_of_key);

/*!
 * \brief Initialization of node
 * Function creates bpt_nd_t structure and prepares
 * it for usage.
 * \param[in] size_of_key size of key.
 * \param[in] m count of keys in one node.
 * \return pointer to bpt_nd_t structure or NULL in case of error.
 */
bpt_nd_t *bpt_nd_init(int size_of_key, int m);

/*!
 * \brief Destroy bpt_nd_t structure
 * \param[in] node pointer to bpt_nd_t structure
 */
void bpt_nd_clean(bpt_nd_t *node);

/*!
 * \brief Is key in node?
 * Function tests if the node is already inserted in
 * the node.
 * \param[in] key key to test.
 * \param[in] node pointer to the node.
 * \param[in] btree pointer to B+ tree.
 * \return 1 ON SUCCESS, 0 OTHERWISE.
 */
unsigned char bpt_nd_key(void *key, bpt_nd_t *node, bpt_t *btree);

/*!
 * \brief Search key in node.
 * Function searches key in node and returns it's index.
 * \param[in] key key to search.
 * \param[in] node pointer to the node.
 * \param[in] btree pointer to B+ tree.
 * \return index or -1 if key is not in the node.
 */
int bpt_nd_index_key(void *key, bpt_nd_t *node, bpt_t *btree);

/*!
 * \brief Is the node leaf?
 * Function tests if the node is leaf.
 * \param[in] node pointer to the node.
 * \return 1 ON SUCCESS, 0 OTHERWISE.
 */
unsigned char bpt_nd_leaf(bpt_nd_t *node);

/*!
 * \brief Parent of node
 * Function returns parent of given node.
 * \param[in] node pointer to the node.
 * \return parent of node or NULL if node is root
 */
bpt_nd_t *bpt_nd_parent(bpt_nd_t *node);

/*!
 * \brief Key on index
 * Function returns key on given index in given node.
 * \param[in] node pointer to the node.
 * \param[in] index index of key.
 * \param[in] size_of_key size of key
 * \return pointer to key
 */
void *bpt_nd_key_on_index(bpt_nd_t *node, int index, int size_of_key);

/*!
 * \brief Initialization of leaf node
 * Function creates bpt_nd_t structure extended by structure bpt_nd_ext_leaf_t (leaf).
 * \param[in] m count of keys in node.
 * \param[in] size_of_value size of value.
 * \param[in] size_of_key size of key.
 * \return pointer to bpt_nd_t structure or NULL in case of error.
 */
bpt_nd_t *bpt_ndlf_init(int m, int size_of_value, int size_of_key);

/*!
 * \brief Value on index
 * Function returns value on given index in given leaf structure.
 * \param[in] node pointer to leaf structure
 * \param[in] index index in leaf
 * \return pointer to value
 */
void *bpt_ndlf_get_val(bpt_nd_ext_leaf_t *node, int index);

/*!
 * \brief Following leaf
 * Function returns following leaf that is connected to actual leaf.
 * Given node has to be leaf!
 * \param[in] node leaf
 * \return pointer to leaf or NULL if the actual leaf is last.
 */
bpt_nd_t *bpt_ndlf_next(bpt_nd_t *node);

/*!
 * \brief Delete item on index
 * Function remove item from given leaf on given index.
 * \param[in] node Pointer to leaf.
 * \param[in] index index in leaf.
 * \param[in] size_of_key size of key.
 * \return updated count of items in leaf.
 */
int bpt_ndlf_del_item(bpt_nd_t *node, int index, int size_of_key);

/*!
 * \brief Insert item to leaf.
 * Function insert key to the leaf and sets output parameter
 * to point to value.
 * \param[in] key key to insert.
 * \param[in] node pointer to leaf node.
 * \param[in] btree pointer to B+ tree
 * \param[in] return_value pointer to value
 * \return index in leaf, -1 if there is no space for new item.
 */
int bpt_ndlf_insert(void *key, bpt_nd_t *node, bpt_t *btree,
                              void **return_value);

/*!
 * \brief Initialization of inner node
 * Function creates bpt_nd_t structure extended by structure bpt_nd_ext_inner_t (inner extension).
 * \param[in] size_of_key size of key.
 * \param[in] m count of keys in node.
 * \return pointer to bpt_nd_t structure or NULL in case of error.

 */
bpt_nd_t *bpt_ndin_init(int size_of_key, int m);

/*!
 * \brief Child on index
 * Function returns child on given index in given node.
 * Node has to be inner node not leaf!
 * \param[in] node pointer to inner node.
 * \param[in] index index of child.
 * \return pointer to child node.
 */
bpt_nd_t *bpt_ndin_child(bpt_nd_t *node, int index);

/*!
 * \brief Insert key to inner node.
 * Function inserts key to right position in inner node,moves child nodes and
 * inserts given left and right child nodes.
 * This function does not solve overloading of node.
 * \param[in] add key to insert.
 * \param[in] left pointer to left descendant of inserted key.
 * \param[in] right pointer to right descendant of inserted key.
 * \param[in] node pointer to inner node where the key is inserted.
 * \param[in] btree pointer to B+ tree.
 * \return actualized count of descendants.
 */
int bpt_ndin_insert(void *add, bpt_nd_t *left, bpt_nd_t *right, bpt_nd_t *node,
                        bpt_t *btree);

/*!
 * \brief Recursive deletion of nodes
 * Recursive function to delete all descendant nodes from given node.
 * \param[in] del pointer to node.
 */
void bpt_del_all(bpt_nd_t *del);

/*!
 * \brief Search leaf and index in B+ tree
 * Function searches leaf and index of given key in B+ tree.
 * If the key is not in the tree the return value is -1 and pointer
 * to leaf is not set.
 * \param[in] key key to search.
 * \param[out] val pointer to searched leaf.
 * \param[in] btree pointer to B+ tree
 * \return index of item in leaf or -1.
 */
int bpt_search_leaf_and_index(void *key, bpt_nd_ext_leaf_t **val, bpt_t *btree);

/*!
 * \brief Index in parent node
 * Function returns position of given node in it's parent node.
 * In the case the node is root, the return value is -1.
 * \param[in] son pointer to node.
 * \return index in parent node or -1.
 */
int bpt_nd_index_in_parent(bpt_nd_t *son);

/*!
 * \brief Insert key to new node.
 * Function creates new node and inserts given key and it's
 * descendants to it. New node is connected to parent of left or right brother.
 * It is used in case of overloading the node and if the right and left
 * rotation can't be done.
 * \param[in] add key to insrt.
 * \param[in] left pointer to left brother.
 * \param[in] right pointer to right brother.
 * \param[in] btree pointer to B+ tree.
 */
void bpt_ndin_insert_to_new_node(void *key, bpt_nd_t *left, bpt_nd_t *right,
                               bpt_t *btree);

/*!
 * \brief Search leaf in B+ tree
 * Function returns appropriate leaf node where the key should be.
 * It can be use to search the value or insert new item.
 * \param[in] key to search.
 * \param[in] btree pointer to B+ tree.
 * \return pointer to leaf node.
 */
bpt_nd_t *bpt_search_leaf(void *key, bpt_t *btree);

/*!
 * \brief Search or insert item
 * Inner function of B+ tree to insert or search item.
 * Operation is specified by argument "search".
 * \param[in] key key to insert.
 * \param[in] btree pointer to B+ tree
 * \param[in] search operation: 1 - search or insert , 0 - search but not insert.
 * \return pointer to value or NULL.
 */
void *bpt_search_or_insert_inner(void *key, bpt_t *btree, int search);

/*!
 * \brief Update parent key
 * Function checks and updates key in parent node (if new key is inserted,
 * it can change the key in parent too).
 * \param[in] node pointer to node.
 * \param[in] btree pointer to B+ tree.
 */
void bpt_nd_check(bpt_nd_t *node, bpt_t *btree);

/*!
 * \brief Delete item specified by leaf and index
 * Function removes the item specified by leaf and index in leaf.
 * \param[in] btree pointer to B+ tree
 * \param[in] index index of item to delete
 * \param[in] leaf_del pointer to leaf.
 * \return 1 ON SUCCESS, 0 OTHERWISE.
 */
int bpt_ndlf_delete_from_tree(int index, bpt_nd_t *leaf_del, bpt_t *btree);

/*!
 * \brief Node items count checker
 * Function checks if the node has allowed count of items
 * (count > MIN and count < MAX). If not, it has to be modified by
 * one of the action: left rotation, right rotation, spliting node,
 * merging nodes.
 * \param[in] check pointer to node
 * \param[in] btree pointer to B+ tree
 */
void bpt_ndin_check(bpt_nd_t *check, bpt_t *btree);

/*!
 * \brief The rightmost leaf in subtree
 * Function finds the rightmost leaf and returns it.
 * \param[in] inner pointer to inner node (subtree).
 * \return pointer to leaf.
 */
bpt_nd_t *bpt_nd_rightmost_leaf(bpt_nd_t *inner);

/*!
 * \brief The leftmost leaf in subtree
 * Function finds the leftmost leaf and returns it.
 * \param[in] inner pointer to inner node (subtree).
 * \return pointer to leaf.
 */
bpt_nd_t *bpt_nd_leftmost_leaf(bpt_nd_t *inner);

#endif            /* _B_PLUS_TREE_INTERNAL_ */