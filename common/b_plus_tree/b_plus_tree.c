/*!
 * \file b_plus_tree.c
 * \brief B+ tree data structure for saving information about IP adresses
 * \author Zdenek Rosa <rosazden@fit.cvut.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
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

#include "../include/b_plus_tree.h"
#include "b_plus_tree_internal.h"
inline void bpt_copy_key(void *to, int index_to, void *from, int index_from, int size_of_key)
{
   memcpy((char*)to + (index_to * size_of_key), (char*)from + (index_from * size_of_key),
          size_of_key);
}

bpt_nd_t *bpt_nd_init(int size_of_key, int m)
{
   bpt_nd_t *node = NULL;
   node = (bpt_nd_t *) calloc(sizeof(bpt_nd_t), 1);
   if (node == NULL) {
      return node;
   }
   node->key = (void *) calloc(size_of_key, m);
   if (node->key == NULL) {
      free(node);
      return (NULL);
   }
   node->count = 1;
   return node;
}

void bpt_nd_clean(bpt_nd_t *node)
{
   if (node == NULL) {
      return;
   }
   if (node->key != NULL) {
      free(node->key);
      node->key = NULL;
   }

   if (node->state_extend == EXTEND_LEAF) {
      //free leaf
      bpt_nd_ext_leaf_t *leaf;
      leaf = (bpt_nd_ext_leaf_t *) node->extend;
      if (leaf != NULL) {
         if (leaf->value != NULL) {
            free(leaf->value);
            leaf->value = NULL;
         }
         free(leaf);
         leaf = NULL;
      }
   } else if (node->state_extend == EXTEND_INNER) {
      //free inner
      if (node->extend != NULL) {
         if (((bpt_nd_ext_inner_t *) node->extend)->child != NULL) {
            free(((bpt_nd_ext_inner_t *) node->extend)->child);
            ((bpt_nd_ext_inner_t *) node->extend)->child = NULL;
         }
         free(node->extend);
         node->extend = NULL;
      }
   }
   free(node);
}

inline unsigned char bpt_nd_key(void *key, bpt_nd_t *node, bpt_t *btree)
{
   return (bpt_nd_index_key(key, node, btree) != (-1) ? 1 : 0);
}

int bpt_nd_index_key(void *key, bpt_nd_t *node, bpt_t *btree)
{
   int i;
   for (i = 0; i < node->count - 1; i++) {
      if (btree->compare((char*)(node->key) + (i * btree->size_of_key), key) == 0) {
         return i;
      }
   }
   return (-1);
}

inline unsigned char bpt_nd_leaf(bpt_nd_t *node)
{
   return (node->state_extend == EXTEND_LEAF);
}

inline bpt_nd_t *bpt_nd_parent(bpt_nd_t *node)
{
   return node->parent;
}

inline void *bpt_nd_key_on_index(bpt_nd_t *node, int index, int size_of_key)
{
   return (char*)(node->key) + (index - 1) * size_of_key;
}

bpt_nd_t *bpt_ndlf_init(int m, int size_of_value, int size_of_key)
{
   bpt_nd_t *node = NULL;
   bpt_nd_ext_leaf_t *leaf = NULL;
   node = bpt_nd_init(size_of_key, m);
   if (node == NULL) {
      return (NULL);
   }
   leaf = (bpt_nd_ext_leaf_t *) calloc(sizeof(bpt_nd_ext_leaf_t), 1);
   if (leaf == NULL) {
      bpt_nd_clean(node);
      return (NULL);
   }
   leaf->value = (void *) calloc(sizeof(void *), m);
   if (leaf->value == NULL) {
      bpt_nd_clean(node);
      free(leaf);
      return (NULL);
   }
   node->extend = (void *) leaf;
   node->state_extend = EXTEND_LEAF;
   return node;
}

inline void *bpt_ndlf_get_val(bpt_nd_ext_leaf_t *node, int index)
{
   return ((bpt_nd_ext_leaf_t *) node)->value[index - 1];
}

inline bpt_nd_t *bpt_ndlf_next(bpt_nd_t *node)
{
   if (node->state_extend != EXTEND_LEAF) {
      return (NULL);
   }
   return ((bpt_nd_ext_leaf_t *) node->extend)->right;
}

int bpt_ndlf_del_item(bpt_nd_t *node, int index, int size_of_key)
{
   int i;
   bpt_nd_ext_leaf_t *leaf;
   leaf = (bpt_nd_ext_leaf_t *) node->extend;
   free(leaf->value[index]);
   leaf->value[index] = NULL;
   for (i = index; i < node->count - 2; ++i) {
      bpt_copy_key(node->key, i, node->key, i + 1, size_of_key);
      leaf->value[i] = leaf->value[i + 1];
   }
   node->count--;
   return node->count - 1;
}

int bpt_ndlf_insert(void *key, bpt_nd_t *node, bpt_t *btree,
                              void **return_value)
{
   //return value is index in leaf. If it returns -1, key is already in tree
   int i;
   bpt_nd_ext_leaf_t *leaf;
   leaf = ((bpt_nd_ext_leaf_t *) node->extend);
   //check if there is key or not
   i = bpt_nd_index_key(key, node, btree);
   if (i != -1) {
      //key is already in leaf
      *return_value = leaf->value[i];
      return -1;
   }
   //find position of new item
   i = node->count - 2; //index of last item
   while (i >= 0 && btree->compare((char*)(node->key) + (i * btree->size_of_key), key) > 0) {
      //node->key[i + 1] = node->key[i];
      memcpy((char*)(node->key) + (i + 1) * btree->size_of_key,
             (char*)(node->key) + (i) * btree->size_of_key,
             btree->size_of_key);
      leaf->value[i + 1] = leaf->value[i];
      i--;
   }
   leaf->value[++i] = (void *) calloc(btree->size_of_value, 1);
   if (leaf->value[i] == NULL) {
      return (-1);
   }
   bpt_copy_key(node->key, i, key, 0, btree->size_of_key);
   node->count++;

   (*return_value) = leaf->value[i];
   return i;
}

bpt_nd_t *bpt_ndin_init(int size_of_key, int m)
{
   bpt_nd_ext_inner_t *inner;
   bpt_nd_t *node;
   inner = (bpt_nd_ext_inner_t *) calloc(sizeof(bpt_nd_ext_inner_t), 1);
   if (inner == NULL) {
      return (NULL);
   }
   inner->child = (bpt_nd_t **) calloc(sizeof(bpt_nd_t *), m + 1);
   if (inner->child == NULL) {
      free(inner);
      inner = NULL;
      return (NULL);
   }
   node = bpt_nd_init(size_of_key, m);
   node->extend = (void *) inner;
   node->state_extend = EXTEND_INNER;
   return node;
}

inline bpt_nd_t *bpt_ndin_child(bpt_nd_t *node, int index)
{
   return ((bpt_nd_ext_inner_t *) node->extend)->child[index - 1];
}

int bpt_ndin_insert(void *add, bpt_nd_t * left, bpt_nd_t * right, bpt_nd_t * node,
                        bpt_t * btree)
{
   int i;
   bpt_nd_ext_inner_t *inner;
   if (bpt_nd_key(add, node, btree)) {
      return (-1);
   }

   inner = (bpt_nd_ext_inner_t *) node->extend;
   i = node->count - 2;
   while (i >= 0 && btree->compare((char*)(node->key) + i * btree->size_of_key, add) > 0) {
      //key[i + 1] = key[i];
      bpt_copy_key(node->key, i + 1, node->key, i, btree->size_of_key);
      inner->child[i + 2] = inner->child[i + 1];
      i--;
   }
   bpt_copy_key(node->key, i + 1, add, 0, btree->size_of_key);
   inner->child[i + 2] = right;
   inner->child[i + 1] = left;

   node->count++;
   return node->count;
}

bpt_t *bpt_init(unsigned int size_of_btree_node, int (*comp)(void *, void *),
                                    unsigned int size_of_value, unsigned int size_of_key)
{
   bpt_t *tree = NULL;
   tree = (bpt_t *) calloc(sizeof(bpt_t), 1);
   if (tree == NULL) {
      return (NULL);
   }
   tree->m = size_of_btree_node;
   tree->root = bpt_ndlf_init(size_of_btree_node, size_of_value, size_of_key);
   if (tree->root == NULL) {
      free(tree);
      return (NULL);
   }
   tree->compare = comp;
   tree->size_of_value = size_of_value;
   tree->size_of_key = size_of_key;
   return tree;
}

void bpt_clean(bpt_t *btree)
{
   bpt_del_all(btree->root);
   free(btree);
}

void bpt_del_all(bpt_nd_t *del)
{
   int i;
   if (del->state_extend == EXTEND_LEAF) {
      bpt_nd_ext_leaf_t *leaf = NULL;
      leaf = (bpt_nd_ext_leaf_t *) del->extend;
      for (i = 0; i < del->count - 1; i++) {
         free(leaf->value[i]);
         leaf->value[i] = NULL;
      }
      bpt_nd_clean(del);
      return;
   } else {
      bpt_nd_ext_inner_t *inner;
      inner = (bpt_nd_ext_inner_t *) del->extend;
      for (i = 0; i < del->count; i++) {
         bpt_del_all(inner->child[i]);
      }
      bpt_nd_clean(del);
   }
}

int bpt_search_leaf_and_index(void *key, bpt_nd_ext_leaf_t **val, bpt_t *btree)
{
   int result;
   bpt_nd_t *node = NULL;
   node = bpt_search_leaf(key, btree);
   result = bpt_nd_index_key(key, node, btree);
   if (result == (-1)) {
      *val = NULL;
      return (-1);
   }
   *val = (bpt_nd_ext_leaf_t *) node->extend;
   return result;
}

int bpt_nd_index_in_parent(bpt_nd_t *son)
{
   //TODO name constants (-1) and (-2), use some macro...
   //find index of certain child in parent
   int i;
   if (!(son->parent)) {
      return (-1);
   }

   for (i = 0; i < son->parent->count; i++) {
      if (((bpt_nd_ext_inner_t *) son->parent->extend)->child[i] == son) {
         return i;
      }
   }
   return (-2);
}

void bpt_ndin_insert_to_new_node(void *key, bpt_nd_t *left, bpt_nd_t *right,
                               bpt_t * btree)
{
   bpt_nd_t *par;
   par = left->parent;
   //parent does not exist, has to be created and added as a parent to his children
   if (par == NULL) {
      par = bpt_ndin_init(btree->size_of_key, btree->m);
      bpt_ndin_insert(key, left, right, par, btree);
      left->parent = par;
      right->parent = par;
      btree->root = par;
      return;
   }
   //parent exists. Add key and check for size
   bpt_ndin_insert(key, left, right, par, btree);
   //size is ok, end
   if (par->count <= btree->m) {
      return;
   }
   //size is to big, split to inner node and repeat recursivly
   else {
      bpt_nd_t *right_par, *righest_node_in_left_node;
      int cut, insert, i;
      right_par = bpt_ndin_init(btree->size_of_key, btree->m);
      cut = (par->count - 1) / 2;
      insert = 0;
      for (i = cut + 1; i < par->count - 1; i++) {
         bpt_copy_key(right_par->key, insert, par->key, i,
                  btree->size_of_key);
         ((bpt_nd_ext_inner_t *) right_par->extend)->child[insert++] =
             ((bpt_nd_ext_inner_t *) par->extend)->child[i];
      }
      ((bpt_nd_ext_inner_t *) right_par->extend)->child[insert++] = ((bpt_nd_ext_inner_t *) par->extend)->child[i];  //last child
      right_par->count = insert;
      par->count = cut + 1;
      right_par->parent = par->parent;
      for (i = 0; i < right_par->count; i++) {
         ((bpt_nd_ext_inner_t *) right_par->extend)->child[i]->parent = right_par;
      }
      righest_node_in_left_node = bpt_nd_rightmost_leaf(((bpt_nd_ext_inner_t *) par->extend)->child[cut]);
      bpt_ndin_insert_to_new_node((char*)(righest_node_in_left_node->key) + (righest_node_in_left_node->count - 2) * btree->size_of_key, par, right_par, btree);
   }
}

bpt_nd_t *bpt_search_leaf(void *key, bpt_t *btree)
{
   //find leaf where is key, or where to add key
   int i;
   bpt_nd_t *pos;
   unsigned char go_right;
   int result;
   go_right = 0;
   pos = btree->root;
   while (pos->state_extend == EXTEND_INNER) {
      bpt_nd_ext_inner_t *pos2 = (bpt_nd_ext_inner_t *) pos->extend;
      go_right = 0;
      for (i = 0; i < pos->count - 1; i++) {
         result = btree->compare(key, (char*)(pos->key) + i * btree->size_of_key);
         if ((result <= 0)) {
            pos = pos2->child[i];
            go_right = 1;
            break;
         }
      }
      if (!go_right) {
         pos = pos2->child[pos->count - 1];
      }
   }
   if (pos->state_extend == EXTEND_LEAF) {
      return pos;
   }
   return NULL;
}

void *bpt_search_or_insert_inner(void *key, bpt_t *btree, int search)
{
   bpt_nd_t *node_to_insert, *r_node;
   bpt_nd_ext_leaf_t *leaf_to_insert, *r_leaf;
   int size, splitVal, insert, i, index_of_new_key;
   void *added_or_found_value;
   node_to_insert = bpt_search_leaf(key, btree);
   index_of_new_key = bpt_ndlf_insert(key, node_to_insert, btree, &added_or_found_value);

   //key is already in tree
   if (index_of_new_key == -1) {
      if (search == 0) {
         return NULL;
      }
      return added_or_found_value;
   }
   btree->count_of_values++;
   leaf_to_insert = (bpt_nd_ext_leaf_t *) node_to_insert->extend;
   size = node_to_insert->count;
   //new value was added, we have to chceck size of leaf
   if (size <= btree->m) {
      //new item was added and size is OK. Just check keys in parents
      //if it is corner value, change parent key
      if (index_of_new_key == node_to_insert->count - 2) {
         bpt_nd_check(node_to_insert, btree);
      }
      return added_or_found_value;
   }
   //size is KO, we have to create new leaf and move half datas
   size--;        //real count of values, not just default size m;
   splitVal = size / 2;
   r_node = bpt_ndlf_init(btree->m, btree->size_of_value, btree->size_of_key);
   r_leaf = (bpt_nd_ext_leaf_t *) r_node->extend;
   insert = 0;
   //copy half datas to new leaf node
   for (i = splitVal; i < size; i++) {
      bpt_copy_key(r_node->key, insert, node_to_insert->key, i, btree->size_of_key);
      r_leaf->value[insert++] = leaf_to_insert->value[i];
   }
   //set poiters to left, right, parent node
   r_node->count = insert + 1;
   node_to_insert->count = splitVal + 1;
   r_node->parent = node_to_insert->parent;
   r_leaf->right = leaf_to_insert->right;
   r_leaf->left = node_to_insert;
   leaf_to_insert->right = r_node;
   bpt_ndin_insert_to_new_node((char*)(node_to_insert->key) + (node_to_insert->count - 2) * btree->size_of_key, node_to_insert, r_node, btree);
   return added_or_found_value;
}

bpt_nd_t *bpt_nd_rightmost_leaf(bpt_nd_t *inner)
{
   if (inner->state_extend == EXTEND_LEAF) {
      return inner;
   }
   return bpt_nd_rightmost_leaf(((bpt_nd_ext_inner_t *) inner->extend)->child[inner->count - 1]);
}

void bpt_nd_check(bpt_nd_t *node, bpt_t *btree)
{
   int parent_index;
   parent_index = bpt_nd_index_in_parent(node);
   if (parent_index < 0) {
      return;
   } else if (parent_index <= node->parent->count - 2) { //set highest key in node, to parent key
      bpt_copy_key(node->parent->key, parent_index, node->key, node->count - 2, btree->size_of_key);
   } else if (parent_index == node->parent->count - 1) {
      //change values in all parent till, they are not on the corner(highest value) of node
      bpt_nd_t *par = node;

      while (parent_index == par->parent->count - 1) {
         par = par->parent;
         parent_index = bpt_nd_index_in_parent(par);
         if (parent_index < 0) { //parent does not exist
            return;
         }
      }
      bpt_copy_key(par->parent->key, parent_index, node->key, node->count - 2, btree->size_of_key);
   }
}

int bpt_ndlf_delete_from_tree(int index, bpt_nd_t *leaf_del,
                                               bpt_t *btree)
{
   int parent_index, size, i;
   //key was not found
   if (index == (-1)) {
      return 0;
   }
   btree->count_of_values--;
   parent_index = bpt_nd_index_in_parent(leaf_del);
   size = bpt_ndlf_del_item(leaf_del, index, btree->size_of_key);
   if (size >= ((btree->m - 1) / 2) || btree->root->state_extend == EXTEND_LEAF) {
      //size is ok, just check parents keys;
      bpt_nd_check(leaf_del, btree);
      return 1;
   } else {
      bpt_nd_t *brother;
      bpt_nd_ext_leaf_t *brother_leaf;
      bpt_nd_ext_leaf_t *leaf_del_leaf;
      leaf_del_leaf = (bpt_nd_ext_leaf_t *) leaf_del->extend;
      //size is too small, we have to resolve this
      if (parent_index > 0 &&
            (((bpt_nd_ext_inner_t *) leaf_del->parent->extend)->child[parent_index - 1]->count - 1) > (btree->m - 1) / 2) {
         //rotation of value from left brother
         brother = ((bpt_nd_ext_inner_t *) leaf_del->parent->extend)->child[parent_index - 1];
         brother_leaf = (bpt_nd_ext_leaf_t *) brother->extend;
         for (i = leaf_del->count - 2; i >= 0; i--) {
            //leaf_del->key[i + 1] = leaf_del->key[i];
            bpt_copy_key(leaf_del->key, i + 1, leaf_del->key, i, btree->size_of_key);
            leaf_del_leaf->value[i + 1] = leaf_del_leaf->value[i];
         }
         leaf_del->count++;
         //leaf_del->key[0] = brother->key[brother->count - 2];
         bpt_copy_key(leaf_del->key, 0, brother->key, brother->count - 2, btree->size_of_key);
         leaf_del_leaf->value[0] = brother_leaf->value[brother->count - 2];
         brother->count--;
         bpt_nd_check(brother, btree);
         if (index == leaf_del->count - 2) {
            bpt_nd_check(leaf_del, btree);
         }

      } else if (parent_index < leaf_del->parent->count - 1 &&
            ((((bpt_nd_ext_inner_t *) leaf_del->parent->extend)->child[parent_index + 1]->count) - 1) > (btree->m - 1) / 2) {
         //rotation of value from rigth brother
         brother = ((bpt_nd_ext_inner_t *) leaf_del->parent->extend)->child[parent_index + 1];
         brother_leaf = (bpt_nd_ext_leaf_t *) brother->extend;

         leaf_del->count++;
         //leaf_del->key[leaf_del->count - 2] = brother->key[0];
         bpt_copy_key(leaf_del->key, leaf_del->count - 2, brother->key, 0, btree->size_of_key);
         leaf_del_leaf->value[leaf_del->count - 2] = brother_leaf->value[0];
         brother->count--;
         for (i = 0; i < brother->count - 1; i++) {
            //brother->key[i] = brother->key[i + 1];
            bpt_copy_key(brother->key, i, brother->key, i + 1, btree->size_of_key);
            brother_leaf->value[i] = brother_leaf->value[i + 1];
         }
         bpt_nd_check(leaf_del, btree);
      } else if (parent_index > 0) {
         //merge with left brother
         brother = ((bpt_nd_ext_inner_t *) leaf_del->parent->extend)->child[parent_index - 1];
         brother_leaf = (bpt_nd_ext_leaf_t *) brother->extend;

         //bpt_copy_key(brother->key, (brother->count) - 1, leaf_del->key, 0, btree->size_of_key * (leaf_del->count - 1));
         for (i = 0; i < leaf_del->count - 1; i++) {
            ++brother->count;
            bpt_copy_key(brother->key, (brother->count) - 2, leaf_del->key, i, btree->size_of_key);
            brother_leaf->value[brother->count - 2] = leaf_del_leaf->value[i];
         }
         brother_leaf->right = leaf_del_leaf->right;
         if (leaf_del_leaf->right) {
            ((bpt_nd_ext_leaf_t *) leaf_del_leaf->right->extend)->left = brother;
         }
         //move indexs in perent
         for (i = parent_index; i < leaf_del->parent->count - 2; i++) {
            bpt_copy_key(leaf_del->parent->key, i, leaf_del->parent->key, i + 1, btree->size_of_key);
         }
         for (i = parent_index; i < leaf_del->parent->count - 1; i++) {
            ((bpt_nd_ext_inner_t *) leaf_del->parent->extend)->child[i] = ((bpt_nd_ext_inner_t *) leaf_del->parent->extend)->child[i + 1];
         }
         leaf_del->parent->count--;
         bpt_nd_check(brother, btree);
         bpt_ndin_check(brother->parent, btree);
         leaf_del->count = 0;
         bpt_nd_clean(leaf_del);
      } else if (parent_index < leaf_del->parent->count - 1) {
         //merge with right brother
         brother = ((bpt_nd_ext_inner_t *) leaf_del->parent->extend)->child[parent_index + 1];
         brother_leaf = (bpt_nd_ext_leaf_t *) brother->extend;
         //copy values
         for (i = 0; i < brother->count - 1; i++) {
            ++leaf_del->count;
            bpt_copy_key(leaf_del->key, (leaf_del->count) - 2, brother->key, i, btree->size_of_key);
            leaf_del_leaf->value[leaf_del->count - 2] = brother_leaf->value[i];
         }
         leaf_del_leaf->right = brother_leaf->right;
         if (brother_leaf->right) {
            ((bpt_nd_ext_leaf_t *) brother_leaf->right->extend)->left = leaf_del;
         }
         for (i = parent_index + 1; i < leaf_del->parent->count - 2; i++) {
            bpt_copy_key(leaf_del->parent->key, i, leaf_del->parent->key, i + 1,
                     btree->size_of_key);
         }
         for (i = parent_index + 1; i < leaf_del->parent->count - 1; i++) {
            ((bpt_nd_ext_inner_t *) leaf_del->parent->extend)->child[i] = ((bpt_nd_ext_inner_t *) leaf_del->parent->extend)->child[i + 1];
         }
         leaf_del->parent->count--;
         bpt_nd_check(leaf_del, btree);
         bpt_ndin_check(leaf_del->parent, btree);
         brother->count = 0;
         bpt_nd_clean(brother);
      }
   }
   return 1;
}

void bpt_ndin_check(bpt_nd_t *check, bpt_t *btree)
{
   int parent_index, i;
   bpt_nd_t *brother;
   bpt_nd_ext_inner_t *brother_inner;
   bpt_nd_ext_inner_t *check_inner;
   if (check->count - 1 >= ((btree->m - 1) / 2)) {
      //size id ok, end;
      return;
   }
   //if just one child, let child to be root
   if (check == btree->root) {
      if (check->count <= 1) {
         btree->root = ((bpt_nd_ext_inner_t *) btree->root->extend)->child[0];
         btree->root->parent = NULL;
         check->count = 0;
         bpt_nd_clean(check);
      }
      return;
   }
   parent_index = bpt_nd_index_in_parent(check);
   check_inner = (bpt_nd_ext_inner_t *) check->extend;
   if (parent_index > 0 && (((bpt_nd_ext_inner_t *) check->parent->extend)->child[parent_index - 1]->count - 1) > (btree->m - 1) / 2) {
      //rotation from left brother
      brother = ((bpt_nd_ext_inner_t *) check->parent->extend)->child[parent_index - 1];
      brother_inner = (bpt_nd_ext_inner_t *) brother->extend;
      for (i = check->count - 1; i >= 0; i--) {
         check_inner->child[i + 1] = check_inner->child[i];
      }
      for (i = check->count - 2; i >= 0; i--) {
         bpt_copy_key(check->key, i + 1, check->key, i, btree->size_of_key);
      }
      check->count++;
      bpt_copy_key(check->key, 0, check->parent->key, parent_index - 1, btree->size_of_key);  //add
      bpt_copy_key(check->parent->key, parent_index - 1, brother->key, brother->count - 2, btree->size_of_key); //add
      check_inner->child[0] = brother_inner->child[brother->count - 1];
      check_inner->child[0]->parent = check;
      brother->count--;
   } else if (parent_index < check->parent->count - 1 &&
              (((bpt_nd_ext_inner_t *) check->parent->extend)->child[parent_index + 1]->count - 1) > (btree->m - 1) / 2) {
      //rotation from right brother
      brother = ((bpt_nd_ext_inner_t *) check->parent->extend)->child[parent_index + 1];
      brother_inner = (bpt_nd_ext_inner_t *) brother->extend;

      check->count++;
      bpt_copy_key(check->key, check->count - 2, check->parent->key, parent_index, btree->size_of_key);   //add
      bpt_copy_key(check->parent->key, parent_index, brother->key, 0, btree->size_of_key); //add
      check_inner->child[check->count - 1] = brother_inner->child[0];
      check_inner->child[check->count - 1]->parent = check;
      brother->count--;
      for (i = 0; i < brother->count - 1; i++) {
         bpt_copy_key(brother->key, i, brother->key, i + 1, btree->size_of_key);
      }
      for (i = 0; i < brother->count; i++) {
         brother_inner->child[i] = brother_inner->child[i + 1];
      }
   } else if (parent_index > 0) {
      //merge with left brother
      int previous;
      brother = ((bpt_nd_ext_inner_t *) check->parent->extend)-> child[parent_index - 1];
      brother_inner = (bpt_nd_ext_inner_t *) brother->extend;

      previous = brother->count - 1;

      for (i = 0; i < check->count; i++) {
         brother_inner->child[(++brother->count) - 1] = check_inner->child[i];
         check_inner->child[i]->parent = brother;
      }

      for (i = parent_index; i < check->parent->count - 2; i++) {
         bpt_copy_key(check->parent->key, i, check->parent->key, i + 1, btree->size_of_key);
      }

      for (i = parent_index; i < check->parent->count - 1; i++) {
         ((bpt_nd_ext_inner_t *) check->parent->extend)->child[i] = ((bpt_nd_ext_inner_t *) check->parent->extend)->child[i + 1];
      }
      check->parent->count--;
      for (i = previous; i < brother->count; i++) {
         bpt_nd_check(bpt_nd_rightmost_leaf(brother_inner->child[i]), btree);
      }
      check->count = 0;
      bpt_nd_clean(check);
      bpt_ndin_check(brother->parent, btree);
      return;
   } else if (parent_index < check->parent->count - 1) {
      int previous, i;
      //merge with right brother
      brother = ((bpt_nd_ext_inner_t *) check->parent->extend)->child[parent_index + 1];
      brother_inner = (bpt_nd_ext_inner_t *) brother->extend;
      previous = check->count - 1;

      for (i = 0; i < brother->count; i++) {
         check_inner->child[(++check->count) - 1] = brother_inner->child[i];
         brother_inner->child[i]->parent = check;
      }

      for (i = parent_index + 1; i < check->parent->count - 2; i++) {
         bpt_copy_key(check->parent->key, i, check->parent->key, i + 1, btree->size_of_key);
      }

      for (i = parent_index + 1; i < check->parent->count - 1; i++) {
         ((bpt_nd_ext_inner_t *) check->parent->extend)->child[i] =
             ((bpt_nd_ext_inner_t *) check->parent->extend)->child[i + 1];
      }

      check->parent->count--;
      for (i = previous; i < check->count; i++) {
         bpt_nd_check(bpt_nd_rightmost_leaf(check_inner->child[i]), btree);
      }
      brother->count = 0;
      bpt_nd_clean(brother);
      bpt_ndin_check(check->parent, btree);
      return;
   }
}

bpt_nd_t *bpt_nd_leftmost_leaf(bpt_nd_t * item)
{
   while (item->state_extend == EXTEND_INNER) {
      item = ((bpt_nd_ext_inner_t *) item->extend)->child[0];
   }
   return item;
}

void *bpt_search_or_insert(bpt_t *btree, void *key)
{
   return bpt_search_or_insert_inner(key, btree, 1);
}

void *bpt_insert(bpt_t *btree, void *key)
{
   return bpt_search_or_insert_inner(key, btree, 0);
}

void *bpt_search(bpt_t *btree, void *key)
{
   bpt_nd_ext_leaf_t *leaf;
   int index;
   index = bpt_search_leaf_and_index(key, &leaf, (bpt_t *) btree);
   if (index == -1) {
      return NULL;
   }
   return leaf->value[index];
}

int bpt_item_del(bpt_t *btree, void *key)
{
   int index;
   bpt_nd_t *leaf_del;
   leaf_del = bpt_search_leaf(key, btree);
   index = bpt_nd_index_key(key, leaf_del, btree);
   return bpt_ndlf_delete_from_tree(index, leaf_del, btree);
}

int bpt_list_item_del(bpt_t *btree, bpt_list_item_t *delete_item)
{
   bpt_nd_t *leaf_del;
   int is_there_next, index_of_delete_item;
   leaf_del = delete_item->leaf;
   index_of_delete_item = delete_item->index_of_value;
   //get next value
   is_there_next = bpt_list_item_next(btree, delete_item);

   bpt_ndlf_delete_from_tree(index_of_delete_item, leaf_del, (bpt_t *) btree);

   if (is_there_next == 0) {
      return is_there_next;
   }

   delete_item->leaf = bpt_search_leaf(delete_item->key, (bpt_t *) btree);
   delete_item->index_of_value = bpt_nd_index_key(delete_item->key, delete_item->leaf,
                                                       (bpt_t *) btree);
   return is_there_next;
}

inline unsigned long int bpt_item_cnt(bpt_t *btree)
{
   return btree->count_of_values;
}

int bpt_list_start(bpt_t *tree, bpt_list_item_t *item)
{
   bpt_nd_t *node;
   node = bpt_nd_leftmost_leaf(tree->root);
   if (node == NULL || node->count == 1) {
      return 0;
   }
   item->index_of_value = 0;
   item->value = ((bpt_nd_ext_leaf_t *) node->extend)->value[0];
   bpt_copy_key(item->key, 0, node->key, 0, tree->size_of_key);
   item->leaf = node;
   return 1;
}

bpt_list_item_t *bpt_list_init(bpt_t *btree)
{
   bpt_list_item_t *item = NULL;
   item = (bpt_list_item_t *) calloc(sizeof(bpt_list_item_t), 1);
   if (item == NULL) {
      return (NULL);
   }
   item->key = (void *) calloc(btree->size_of_key, 1);
   if (item->key == NULL) {
      free(item);
      return (NULL);
   }
   return item;
}

inline void bpt_list_clean(bpt_list_item_t *item)
{
   if (item != NULL) {
      if (item->key != NULL) {
         free(item->key);
         item->key = NULL;
      }
      free(item);
      item = NULL;
   }
}

int bpt_list_item_next(bpt_t *tree, bpt_list_item_t *item)
{
   bpt_nd_t *node;
   bpt_nd_ext_leaf_t *leaf;
   node = item->leaf;
   leaf = (bpt_nd_ext_leaf_t *) node->extend;
   if (item->index_of_value < node->count - 2) {
      ++item->index_of_value;
      bpt_copy_key(item->key, 0, node->key, item->index_of_value, tree->size_of_key);
      item->value = leaf->value[item->index_of_value];
      return 1;
   } else if (bpt_ndlf_next(node) != NULL) {
      node = bpt_ndlf_next(node);
      bpt_copy_key(item->key, 0, node->key, 0, tree->size_of_key);
      item->value = ((bpt_nd_ext_leaf_t *) node->extend)->value[0];
      item->leaf = node;
      item->index_of_value = 0;
      return 1;
   }
   return 0;
}

