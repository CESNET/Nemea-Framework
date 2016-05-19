/*!
 * \file prefix_tree.c
 * \brief Prefix tree data structure for saving informaticons about domains.
 * \author Zdenek Rosa <rosazden@fit.cvut.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2014
 * \date 2015
 */
/*
 * Copyright (C) 2014-2015 CESNET
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
#include "../include/prefix_tree.h"

int prefix_tree_map_character_to_number(unsigned  char  letter)
{
   return letter;
   /*if (letter >= ' ' && letter <= '~') {
      //numbers are on position 0 to 9 from 48 -57
      return (letter) -' ';
   } else {
      //printf("this letter canot be used in domain: %c, in dec %d\n", letter ,letter);
      return COUNT_OF_LETTERS_IN_DOMAIN-1;
   }*/

}

prefix_tree_t * prefix_tree_initialize(unsigned char prefix_suffix, unsigned int size_of_value, int domain_separator, int domain_extension, relaxation_after_delete relaxation)
{
   prefix_tree_t *tree = NULL;
   tree = (prefix_tree_t *) calloc(sizeof(prefix_tree_t), 1);
   if (tree == NULL) {
      goto failure;
   }
   tree->size_of_value = size_of_value;
   tree->prefix_suffix = prefix_suffix;
   tree->domain_separator = domain_separator;
   tree->relaxation = relaxation;
   tree->root = (prefix_tree_inner_node_t *) calloc(sizeof(prefix_tree_inner_node_t), 1);
   if (tree->root == NULL) {
      goto failure;
   }
   tree->root->domain = (prefix_tree_domain_t *) calloc(sizeof(prefix_tree_domain_t), 1);
   if (tree->root->domain == NULL) {
      goto failure;
   }
   tree->root->domain->count_of_insert = 1;
   if (domain_extension == DOMAIN_EXTENSION_YES) {
      tree->root->domain->domain_extension = (node_domain_extension_t *) calloc(1, sizeof(node_domain_extension_t));
      tree->domain_extension = (tree_domain_extension_t *) calloc(1, sizeof(tree_domain_extension_t));
      if (tree->domain_extension == NULL) {
         goto failure;
      }
      tree->domain_extension->list_of_most_subdomains = (prefix_tree_domain_t **) calloc(sizeof(prefix_tree_domain_t *), MAX_SIZE_OF_DEGREE);
      if (tree->domain_extension->list_of_most_subdomains == NULL) {
         goto failure;
      }
      tree->domain_extension->list_of_most_subdomains_end = (prefix_tree_domain_t **) calloc(sizeof(prefix_tree_domain_t *), MAX_SIZE_OF_DEGREE);
      if (tree->domain_extension->list_of_most_subdomains_end == NULL) {
         goto failure;
      }
   } else {
      tree->domain_extension=NULL;
   }
   return tree;
failure:
   if (tree != NULL) {
      if (tree->root != NULL) {
         if (tree->root->domain != NULL) {
            free(tree->root->domain->domain_extension);
            free(tree->root->domain);
         }
         free(tree->root);
      }
      if (tree->domain_extension != NULL) {
         if (tree->domain_extension->list_of_most_subdomains != NULL) {
            free(tree->domain_extension->list_of_most_subdomains);
         }
         free(tree->domain_extension);
      }
      free(tree);
   }
   return NULL;
}



int prefix_tree_destroy_recursive(prefix_tree_t * tree, prefix_tree_inner_node_t *  node)
{
   int deleted_domains = 0;
   int i, index;

   //for all nodes
   if (node != NULL) {
      //for all children in node
      if (node->child != NULL) {
         for (i=0; i<COUNT_OF_LETTERS_IN_DOMAIN;i++) {
            if (node->child[i] != NULL) {
               deleted_domains = prefix_tree_destroy_recursive(tree, node->child[i]);
               node->count_of_children--;
            }
         }
         free(node->child);
      }
      //for domain and domain's child
      if (node->domain != NULL) {
         if (node->domain->child != NULL) {
            deleted_domains = prefix_tree_destroy_recursive(tree, node->domain->child);
         }
         if (node->domain->value != NULL) {
            free(node->domain->value);
         }
         if (tree->domain_extension != NULL) {
            if (node->domain->domain_extension->most_subdomains_more) {
               node->domain->domain_extension->most_subdomains_more->domain_extension->most_subdomains_less = node->domain->domain_extension->most_subdomains_less;
            }
            if (node->domain->domain_extension->most_subdomains_less) {
               node->domain->domain_extension->most_subdomains_less->domain_extension->most_subdomains_more = node->domain->domain_extension->most_subdomains_more;
            }
            if (node->domain->domain_extension->most_used_domain_more) {
               node->domain->domain_extension->most_used_domain_more->domain_extension->most_used_domain_less = node->domain->domain_extension->most_used_domain_less;
            }
            if (node->domain->domain_extension->most_used_domain_less) {
               node->domain->domain_extension->most_used_domain_less->domain_extension->most_used_domain_more = node->domain->domain_extension->most_used_domain_more;
            }
            if (node->domain == tree->domain_extension->list_of_most_used_domains) {
               tree->domain_extension->list_of_most_used_domains = node->domain->domain_extension->most_used_domain_less;
            }
            if (node->domain == tree->domain_extension->list_of_most_used_domains_end) {
               tree->domain_extension->list_of_most_used_domains_end = node->domain->domain_extension->most_used_domain_more;
            }
            if (node->domain == tree->domain_extension->list_of_most_unused_domains) {
               tree->domain_extension->list_of_most_unused_domains = node->domain->domain_extension->most_used_domain_less;
            }
            index = node->domain->degree;
            if (index >= MAX_SIZE_OF_DEGREE) {
               index = MAX_SIZE_OF_DEGREE - 1;
            }
            if (node->domain == tree->domain_extension->list_of_most_subdomains[index]) {
               tree->domain_extension->list_of_most_subdomains[index] = node->domain->domain_extension->most_subdomains_less;
            }
            if (node->domain == tree->domain_extension->list_of_most_subdomains_end[index]) {
               tree->domain_extension->list_of_most_subdomains_end[index] = node->domain->domain_extension->most_subdomains_more;
            }
            free(node->domain->domain_extension);
            node->domain->domain_extension = NULL;
         }
         free(node->domain);
         deleted_domains++;
      }
      if (node->value != NULL) {
         free(node->value);
      }
      //node string
      if (node->string != NULL) {
         free(node->string);
      }
      free(node);
      return deleted_domains;
   }
   return 0;
}

void prefix_tree_destroy(prefix_tree_t * tree)
{
   if (tree != NULL) {
      prefix_tree_destroy_recursive(tree, tree->root);
      if (tree->domain_extension != NULL) {
         if (tree->domain_extension->list_of_most_subdomains != NULL) {
            free(tree->domain_extension->list_of_most_subdomains);
         }
         if (tree->domain_extension->list_of_most_subdomains_end != NULL) {
            free(tree->domain_extension->list_of_most_subdomains_end);
         }
         free(tree->domain_extension);
         tree->domain_extension=NULL;
      }
      free(tree);
   }
}

void prefix_tree_decrease_counters_deleted_inner_node(prefix_tree_inner_node_t * node, int deleted_strings, int deleted_domains)
{

   if (node == NULL) {
      return;
   }
   do{
      while (node->parent != NULL) {
         node->count_of_string -= deleted_strings;
         node = node->parent;
      }
      node->count_of_string -= deleted_strings;
      if (node->parent_is_domain) {
         node->parent_is_domain->count_of_different_subdomains -= deleted_domains;
         node = node->parent;
      }
      else {
         break;
      }
   }while (node);

}

prefix_tree_inner_node_t * join_nodes(prefix_tree_inner_node_t * node)
{
   if (!node) {
      return NULL;
   } else if (node->count_of_children == 1 && node->domain == NULL && node->value == NULL) {
      int i;
      for (i = 0; i < COUNT_OF_LETTERS_IN_DOMAIN; i++) {
         if (node->child[i] != NULL) {
            char *str;
            prefix_tree_inner_node_t * child;
            child = node->child[i];
            str = (char*)calloc(sizeof(char), node->length + child->length);
            memcpy(str, node->string, node->length);
            memcpy(str + node->length, child->string, child->length);
            node->length += child->length;
            free(node->string);
            free(node->child);
            free(child->string);
            node->string = str;
            node->count_of_children = child->count_of_children;
            node->count_of_string = child->count_of_string;
            node->child = child->child;
            if (node->child != NULL && node->count_of_children != 0) {
               int i;
               for (i = 0; i < COUNT_OF_LETTERS_IN_DOMAIN; i++) {
                  if (node->child[i] != NULL) {
                     node->child[i]->parent = node;
                  }
               }
            }
            if (child->domain != NULL) {
               child->domain->parent=node;
               node->domain = child->domain;
            }
            node->value = child->value;
            free(child);
            return node;
         }
      }
   }
   return node;


}

void prefix_tree_delete_inner_node(prefix_tree_t * tree, prefix_tree_inner_node_t * node)
{
   if (!node) {
      return;
   }
   if (node == tree->root) {
      int i;
      for (i=0; i < COUNT_OF_LETTERS_IN_DOMAIN; i++) {
         if (node->child[i] != NULL) {
            prefix_tree_destroy_recursive(tree, node->child[i]);
            node->child[i] = NULL;
            node->count_of_children--;
         }
      }
      tree->count_of_different_domains = 0;
      node->count_of_string = 0;
      node->count_of_children = 0;
      if (node->child) {
         free(node->child);
      }
      node->child = NULL;

   } else if (node->parent && node->parent->child) {
      prefix_tree_inner_node_t * parent;
      int deleted_domains, deleted_strings;
      parent = node->parent;
      parent->child[prefix_tree_map_character_to_number(node->string[0])] = NULL;
      parent->count_of_children--;
      deleted_strings = node->count_of_string;
      deleted_domains = prefix_tree_destroy_recursive(tree, node);
      prefix_tree_decrease_counters_deleted_inner_node(parent, deleted_strings, deleted_domains);
      tree->count_of_different_domains -= deleted_domains;
      if (tree->relaxation == RELAXATION_AFTER_DELETE_YES && parent->count_of_children == 1 && parent->domain == NULL && parent->value == NULL) {
         join_nodes(parent);
      }
   } else {
      prefix_tree_destroy_recursive(tree, node);
      return;
   }
}

void prefix_tree_recursive_plus_domain(prefix_tree_domain_t * domain_parent, prefix_tree_t * tree)
{
   if (domain_parent != NULL) {
      int index;
      //+1 to subdomain
      domain_parent->count_of_different_subdomains++;
      if (tree->domain_extension != NULL) {
         index = domain_parent->degree;
         if (index >= MAX_SIZE_OF_DEGREE) {
            index = MAX_SIZE_OF_DEGREE - 1;
         }
         //add or sort in list count_of_insert
         if (domain_parent->count_of_different_subdomains > ADD_TO_LIST_FROM_COUNT_OF_DIFFERENT_SUBDOMAINS) {
            //on the begeing set the best, and the worse
            if (tree->domain_extension->list_of_most_subdomains[index] == NULL && tree->domain_extension->list_of_most_subdomains_end[index] == NULL) {
               tree->domain_extension->list_of_most_subdomains[index] = domain_parent;
               tree->domain_extension->list_of_most_subdomains_end[index] = domain_parent;
            } else {
               //new domain in tree, set it to the end of the list
               if (domain_parent->domain_extension->most_subdomains_more  == NULL && domain_parent->domain_extension->most_subdomains_less  == NULL) {
                  if (tree->domain_extension->list_of_most_subdomains_end[index] != domain_parent) {
                     tree->domain_extension->list_of_most_subdomains_end[index]->domain_extension->most_subdomains_less = domain_parent;
                     domain_parent->domain_extension->most_subdomains_more = tree->domain_extension->list_of_most_subdomains_end[index];
                     tree->domain_extension->list_of_most_subdomains_end[index]=domain_parent;
                  }
               }
               //if it is more used than other, than move forward
               while (domain_parent->domain_extension->most_subdomains_more != NULL && domain_parent->domain_extension->most_subdomains_more->count_of_different_subdomains < domain_parent->count_of_different_subdomains) {
                  prefix_tree_domain_t * help;
                  help = domain_parent->domain_extension->most_subdomains_more;
                  domain_parent->domain_extension->most_subdomains_more = help->domain_extension->most_subdomains_more;
                  help->domain_extension->most_subdomains_less = domain_parent->domain_extension->most_subdomains_less;
                  help->domain_extension->most_subdomains_more = domain_parent;
                  domain_parent->domain_extension->most_subdomains_less = help;
                  if (domain_parent->domain_extension->most_subdomains_more != NULL) {
                     domain_parent->domain_extension->most_subdomains_more->domain_extension->most_subdomains_less = domain_parent;
                  } else {
                     //on the top
                     tree->domain_extension->list_of_most_subdomains[index] = domain_parent;
                  }

                  if (help->domain_extension->most_subdomains_less != NULL) {
                     help->domain_extension->most_subdomains_less->domain_extension->most_subdomains_more = help;
                  }
                  if (help->domain_extension->most_subdomains_less == NULL) {
                     tree->domain_extension->list_of_most_subdomains_end[index] = help;
                  }
               }
            }
         }
      }
      //move to next item
      domain_parent = domain_parent->parent_domain;
   }
}

void prefix_tree_recursive_plus_node(prefix_tree_domain_t * domain, prefix_tree_t * tree)
{
   prefix_tree_inner_node_t * node;
   while (domain != NULL && domain->parent != NULL) {
         prefix_tree_recursive_plus_domain(domain->parent_domain, tree);
         node = domain->parent;
         while (node->parent != NULL) {
            node->count_of_string++;
            node = node->parent;
         }
         node->count_of_string++;
         domain = node->parent_is_domain;
   }
}

char *prefix_tree_read_inner_node(prefix_tree_t *tree, prefix_tree_inner_node_t *node, char *string)
{
   int i;
   if (tree->prefix_suffix == PREFIX) {
      for (i = 0; i < node->length; i++) {
         string[i] = node->string[i];
      }
      string[i] = 0;
   } else {
      string[node->length] = 0;
      for (i = 0; i < node->length; i++) {
         string[i] = node->string[node->length - 1 - i];
      }
   }
   return NULL;
}

prefix_tree_inner_node_t * prefix_tree_most_substring(prefix_tree_inner_node_t * node) {
   int i, max_index = 0;
   unsigned int max = 0;
   if (node->child != NULL) {
      for (i=0; i < COUNT_OF_LETTERS_IN_DOMAIN; i++) {
         if (node->child[i] != NULL && node->child[i]->count_of_string > max) {
            max = node->child[i]->count_of_string;
            max_index = i;
         }
      }
      return node->child[max_index];
   }
   return NULL;
}

prefix_tree_domain_t * prefix_tree_new_domain(prefix_tree_inner_node_t * node, prefix_tree_domain_t * domain_parent, prefix_tree_t * tree)
{
   node->domain = (prefix_tree_domain_t*) calloc(sizeof(prefix_tree_domain_t),1);
   if (node->domain == NULL) {
      return (NULL);
   }
   if (tree->size_of_value > 0) {
      node->domain->value = calloc(tree->size_of_value,1);
      if (node->domain->value == NULL) {
         return (NULL);
      }
   }
   node->domain->parent_domain = domain_parent;
   node->domain->parent = node;
   if (domain_parent) {
      node->domain->degree = domain_parent->degree+1;
   }
   if (tree->domain_extension != NULL) {
      node->domain->domain_extension = (node_domain_extension_t*) calloc (1,sizeof(node_domain_extension_t));
      if (node->domain->domain_extension == NULL) {
         return (NULL);
      }
   }
   //plus new domain
   //prefix_tree_recursive_plus_domain(domain_parent, tree);
   //plus to split nodes
   prefix_tree_recursive_plus_node(node->domain,tree);
   return node->domain;
}

prefix_tree_inner_node_t * prefix_tree_new_node(prefix_tree_inner_node_t * parent, int map_number)
{
   parent->count_of_children++;
   parent->child[map_number] = (prefix_tree_inner_node_t*) calloc(sizeof(prefix_tree_inner_node_t),1);
   if (parent->child[map_number] == NULL) {
      return (NULL);
   }
   parent->child[map_number]->value = NULL;
   parent->child[map_number]->parent = parent;
   return parent->child[map_number];
}

prefix_tree_inner_node_t * prefix_tree_add_children_array(prefix_tree_inner_node_t * parent)
{
   parent->child = (prefix_tree_inner_node_t **) calloc(sizeof(prefix_tree_inner_node_t*),COUNT_OF_LETTERS_IN_DOMAIN);
   if (parent->child == NULL) {
      return (NULL);
   }
   return parent;
}

prefix_tree_inner_node_t * prefix_tree_new_node_parent_is_domain(prefix_tree_domain_t * domain)
{
   domain->child = (prefix_tree_inner_node_t*) calloc(sizeof(prefix_tree_inner_node_t),1);
   if (domain->child == NULL) {
      return (NULL);
   }
   domain->child->parent_is_domain = domain;
   if (prefix_tree_add_children_array(domain->child) == NULL) {
      return (NULL);
   }
   return domain->child;
}

int prefix_tree_count_to_domain_separator(const char *string, int length, int domain_separator, char prefix)
{
   int i;
   if (prefix == PREFIX) {
      for (i=0; i < length; i++) {
         if (string[i] == domain_separator) {
            return i;
         }
      }
   } else {
      for (i=length-1; i >=0; i--) {
         if (string[i] == domain_separator) {
            return length - i - 1;
         }
      }
   }
   return length;
}



prefix_tree_domain_t * prefix_tree_add_new_item(prefix_tree_inner_node_t * node ,prefix_tree_domain_t * domain , const char *string, int length, prefix_tree_t * tree)
{
   int count, i;
   count = prefix_tree_count_to_domain_separator(string, length, tree->domain_separator, tree->prefix_suffix);
   node->string = (char*) calloc(sizeof(char),count);
   if (node->string == NULL) {
      return NULL;
   }
   if (tree->prefix_suffix == PREFIX) {
      //copy normal
      for (i=0; i<count; i++) {
         node->string[i] = string[i];
      }
      node->length = count;
      prefix_tree_new_domain(node, domain, tree);
      if (length > count) {
         return prefix_tree_add_domain_recursive_prefix(prefix_tree_new_node_parent_is_domain(node->domain), node->domain, string + count + 1, length - count - 1, tree);
      }
   } else {
      //copy invert
      for (i=0; i<count; i++) {
         node->string[i] = string[length-i-1];
      }
      node->length = count;
      prefix_tree_new_domain(node, domain, tree);
      if (length > count) {
         return prefix_tree_add_domain_recursive_suffix(prefix_tree_new_node_parent_is_domain(node->domain), node->domain, string, length - count - 1, tree);
      }
   }


   return node->domain;
}

prefix_tree_inner_node_t * prefix_tree_split_node_into_two(prefix_tree_inner_node_t * node, int index)
{
   prefix_tree_inner_node_t * first_node = NULL;
   char * second_string;
   //first node, must be created
   if (node->string == NULL) {
      return (NULL);
   }
   if (node->parent) {
      first_node = prefix_tree_new_node(node->parent, prefix_tree_map_character_to_number(*(node->string)));
      node->parent->count_of_children--;
   }
   if (first_node == NULL) {
      return (NULL);
   }
   first_node->count_of_string = node->count_of_string;
   if (prefix_tree_add_children_array(first_node) == NULL) {
      return (NULL);
   }
   first_node->string = (char*) calloc(sizeof(char), index);
   if (first_node->string == NULL) {
      return (NULL);
   }
   memcpy(first_node->string, node->string, sizeof(char) * (index));
   first_node->length = index;
   //second node must be edited
   second_string = (char*) calloc(sizeof(char), node->length - index);
   if (second_string == NULL) {
      return (NULL);
   }
   memcpy(second_string, node->string+index,sizeof(char) * (node->length - index));
   free(node->string);
   node->string = second_string;
   node->length = node->length - index;
   node->parent = first_node;
   //conect first node to second
   first_node->child[prefix_tree_map_character_to_number(*second_string)] = node;
   first_node->count_of_children++;
   return first_node;
}


int prefix_tree_length_of_string(prefix_tree_domain_t *domain)
{
   int length = 0;
   prefix_tree_inner_node_t *node;
   while (domain != NULL && domain->parent != NULL) {
      node = domain->parent;
      while (node->parent != NULL) {
         length += node->length;
         node = node->parent;
      }
      domain = node->parent_is_domain;
      length++;
   }
   length--;
   return length;
}



char * prefix_tree_read_string(prefix_tree_t * tree, prefix_tree_domain_t * domain, char * string)
{
   char  *pointer_to_string;
   prefix_tree_inner_node_t *node;
   int i;
   pointer_to_string = string;
   node = domain->parent;
   if (tree->prefix_suffix == PREFIX) {
      int length;
      length = prefix_tree_length_of_string(domain);
      pointer_to_string += length;
      *pointer_to_string = 0;
      pointer_to_string--;
      while (domain != NULL && domain->parent != NULL) {
         node = domain->parent;
         while (node->parent != NULL) {
            for (i = node->length-1; i>=0; i--) {
               *pointer_to_string = node->string[i];
               pointer_to_string--;
            }
            node = node->parent;
         }
         if (pointer_to_string > string) {
            *pointer_to_string = tree->domain_separator;
            pointer_to_string--;
         }
         domain = node->parent_is_domain;
      }
   } else {
      while (domain != NULL && domain->parent != NULL) {
         node = domain->parent;
         while (node->parent != NULL) {
            for (i = node->length-1; i>=0; i--) {
               *pointer_to_string = node->string[i];
               pointer_to_string++;
            }
            node = node->parent;
         }
         *pointer_to_string = tree->domain_separator;
         pointer_to_string++;
         domain = node->parent_is_domain;
      }
      pointer_to_string--;
      *pointer_to_string=0;
   }
   return string;
}

prefix_tree_domain_t * prefix_tree_add_domain_recursive_suffix(prefix_tree_inner_node_t * node, prefix_tree_domain_t * domain_parent, const char *string, int length, prefix_tree_t * tree)
{
   //read common part;
   int i, index;
   index = length-1;
   for (i=0; i < node->length; i++) {
      if (index >= 0 && node->string[i] == string[index]) {
         index--;
      } else {
         break;
      }
   }
   //common part does not exist at all
   if (i==0) {
      int map_number;
      map_number = prefix_tree_map_character_to_number(string[index]);
      //new record, create new nodes
      if (node->child ==NULL) {
         if (prefix_tree_add_children_array(node) == NULL) {
            return (NULL);
         }
      }
      if (node->child[map_number] == NULL) {
         if (prefix_tree_new_node(node, map_number) == NULL) {
            return (NULL);
         }
         return prefix_tree_add_new_item(node->child[map_number],domain_parent , string, length, tree);
      } else {
         //link exists
         return prefix_tree_add_domain_recursive_suffix(node->child[map_number], domain_parent, string, length, tree);
      }
   } else if (i < node->length) {
      //common part exist but is too short
      //split node into two nodes, on index where it is not common
      node = prefix_tree_split_node_into_two(node, i);
      if (node == NULL) {
         return (NULL);
      }
      //domain
      if (index == -1 || string[index] == tree->domain_separator) {
         if (node->domain == NULL) {
            if (prefix_tree_new_domain(node, domain_parent, tree) == NULL) {
               return (NULL);
            }
         }
         if (index <= 0) {
            return (node->domain);
         } else {
            return prefix_tree_add_domain_recursive_suffix(prefix_tree_new_node_parent_is_domain(node->domain), node->domain, string, index, tree);
         }
      } else {
         //continue with other nodes
         int map_number;
         map_number = prefix_tree_map_character_to_number(string[index]);
         if (node->child == NULL) {
            if (prefix_tree_add_children_array(node) == NULL) {
               return (NULL);
            }
         }
         if (node->child[map_number] == NULL) {
            if (prefix_tree_new_node(node,map_number) == NULL) {
               return (NULL);
            }
            return prefix_tree_add_new_item(node->child[map_number],domain_parent, string, index+1, tree);
         }
         return prefix_tree_add_domain_recursive_suffix(node->child[map_number], domain_parent, string, index+1, tree);
      }
   } else if (i == node->length) {
   //node is fully used and it continues to other node
      int map_number;
      if (index < 0 || string[index] == tree->domain_separator) {
         if (node->domain == NULL) {
            if (prefix_tree_new_domain(node, domain_parent, tree) == NULL) {
               return (NULL);
            }
         } else if (node->domain->exception) {
            return NULL;
         }
         if (index < 0) {
            return (node->domain);
         } else if (node->domain->child == NULL) {
            return prefix_tree_add_domain_recursive_suffix(prefix_tree_new_node_parent_is_domain(node->domain), node->domain, string, index, tree);
         } else {
            return prefix_tree_add_domain_recursive_suffix(node->domain->child, node->domain, string, index, tree);
         }
      }

      map_number = prefix_tree_map_character_to_number(string[index]);
      if (node->child == NULL) {
         if (prefix_tree_add_children_array(node) == NULL) {
            return (NULL);
         }
      }
      if (node->child[map_number] == NULL) {
         if (prefix_tree_new_node(node,map_number) == NULL) {
            return (NULL);
         }
         return prefix_tree_add_new_item(node->child[map_number],domain_parent, string, index+1, tree);
      } else {
         return prefix_tree_add_domain_recursive_suffix(node->child[map_number], domain_parent, string, index+1, tree);
      }
   } else {
      printf("error\n");
      return NULL;
   }

}

prefix_tree_domain_t * prefix_tree_add_domain_recursive_prefix(prefix_tree_inner_node_t * node, prefix_tree_domain_t * domain_parent, const char *string, int length, prefix_tree_t * tree)
{
   //read common part;
   int i;
   for (i=0; i < node->length; i++) {
      if (length > 0 && node->string[i] == *string) {
         length--;
         string++;
      } else {
         break;
      }
   }
   //common part does not exist at all
   if (i==0) {
      int map_number;
      map_number = prefix_tree_map_character_to_number(*string);
      //new record, create new nodes
      if (node->child ==NULL) {
         if (prefix_tree_add_children_array(node) == NULL) {
            return (NULL);
         }
      }
      if (node->child[map_number] == NULL) {
         if (prefix_tree_new_node(node, map_number) == NULL) {
            return (NULL);
         }
         return prefix_tree_add_new_item(node->child[map_number],domain_parent , string, length, tree);
      } else {
         //link exists
         return prefix_tree_add_domain_recursive_prefix(node->child[map_number], domain_parent, string, length, tree);
      }
   } else if (i < node->length) {
      //common part exist but is too short
      //split node into two nodes, on index where it is not common
      node = prefix_tree_split_node_into_two(node, i);
      if (node == NULL) {
         return (NULL);
      }
      //domain
      if (length <= 0 || *string == tree->domain_separator) {
         if (node->domain == NULL) {
            if (prefix_tree_new_domain(node, domain_parent, tree) == NULL) {
               return (NULL);
            }
         }
         if (length <= 0) {
            return (node->domain);
         } else {
            return prefix_tree_add_domain_recursive_prefix(prefix_tree_new_node_parent_is_domain(node->domain), node->domain, ++string, length-1, tree);
         }
      } else {
         //continue with other nodes
         int map_number;
         map_number = prefix_tree_map_character_to_number(*string);
         if (node->child == NULL) {
            if (prefix_tree_add_children_array(node) == NULL) {
               return (NULL);
            }
         }
         if (node->child[map_number] == NULL) {
            if (prefix_tree_new_node(node,map_number) == NULL) {
               return (NULL);
            }
            return prefix_tree_add_new_item(node->child[map_number],domain_parent, string, length, tree);
         }
         return prefix_tree_add_domain_recursive_prefix(node->child[map_number], domain_parent, string, length, tree);
      }
   } else if (i == node->length) {
      //node is fully used and it continues to other node
      int map_number;
      if (length <= 0 || *string == tree->domain_separator) {
         if (node->domain == NULL) {
            if (prefix_tree_new_domain(node, domain_parent, tree) == NULL) {
               return (NULL);
            }
         } else if (node->domain->exception) {
            return NULL;
         }
         if (length <= 0) {
            return (node->domain);
         } else if (node->domain->child == NULL) {
            return prefix_tree_add_domain_recursive_prefix(prefix_tree_new_node_parent_is_domain(node->domain), node->domain, ++string, length-1, tree);
         } else {
            return prefix_tree_add_domain_recursive_prefix(node->domain->child, node->domain, ++string, length-1, tree);
         }
      }

      map_number = prefix_tree_map_character_to_number(*string);
      if (node->child == NULL) {
         if (prefix_tree_add_children_array(node) == NULL) {
            return (NULL);
         }
      }
      if (node->child[map_number] == NULL) {
         if (prefix_tree_new_node(node,map_number) == NULL) {
            return (NULL);
         }
         return prefix_tree_add_new_item(node->child[map_number],domain_parent, string, length, tree);
      } else {
         return prefix_tree_add_domain_recursive_prefix(node->child[map_number], domain_parent, string, length, tree);
      }
   } else {
      printf("error\n");
      return NULL;
   }

}

int prefix_tree_is_string_in_exception(prefix_tree_t * tree, const char *string, int length)
{
   int i, index, map_number;
   prefix_tree_inner_node_t *node;
   node = tree->root;


   if (tree->prefix_suffix == PREFIX) {
      //prefix tree
      index = 0;
      while (node != NULL) {
            for (i=0; i < node->length; i++) {
               if (index < length && node->string[i] == string[index]) {
                  index++;
               } else {
                  return 0;
               }
            }
            if (index >= length || string[index] == tree->domain_separator) {
               if (node->domain == NULL) {
                  return 0;
               } else if (node->domain->exception == 1) {
                  return 1;
               } else if (index < 0) {
                  return 0;
               }
               index++;
               node = node->domain->child;
            } else {
               if (node->child == NULL) {
                  return 0;
               }
               map_number = prefix_tree_map_character_to_number(string[index]);
               node = node->child[map_number];
            }
      }
   } else {
      //suffix tree
      index = length - 1;
      while (node != NULL) {
            for (i=0; i < node->length; i++) {
               if (index >= 0 && node->string[i] == string[index]) {
                  index--;
               } else {
                  return 0;
               }
            }
            if (index < 0 || string[index] == tree->domain_separator) {
               if (node->domain == NULL) {
                  return 0;
               } else if (node->domain->exception == 1) {
                  return 1;
               } else if (index >= length) {
                  return 0;
               }
               index--;
               node = node->domain->child;
            } else {
               if (node->child == NULL) {
                  return 0;
               }
               map_number = prefix_tree_map_character_to_number(string[index]);
               node = node->child[map_number];
            }
      }
   }


   return 0;
}

prefix_tree_domain_t * prefix_tree_search(prefix_tree_t * tree,const  char *string, int length)
{
   int i, index, map_number;
   prefix_tree_inner_node_t *node;
   node = tree->root;

   if (tree->prefix_suffix == PREFIX) {
      //prefix tree
      index = 0;
      while (node != NULL) {
            for (i=0; i < node->length; i++) {
               if (index < length && node->string[i] == string[index]) {
                  index++;
               } else {
                  return NULL;
               }
            }
            if (index >= length || string[index] == tree->domain_separator) {
               if (node->domain == NULL) {
                  return NULL;
               } else if (index >= length) {
                  return node->domain;
               }
               index++;
               node = node->domain->child;
            } else {
               if (node->child == NULL) {
                  return NULL;
               }
               map_number = prefix_tree_map_character_to_number(string[index]);
               node = node->child[map_number];
            }
      }
   } else {
      //suffix tree
      index = length - 1;
      while (node != NULL) {
            for (i=0; i < node->length; i++) {
               if (index >= 0 && node->string[i] == string[index]) {
                  index--;
               } else {
                  return NULL;
               }
            }
            if (index < 0 || string[index] == tree->domain_separator) {
               if (node->domain == NULL) {
                  return NULL;
               } else if (index < 0) {
                  return node->domain;
               }
               index--;
               node = node->domain->child;
            } else {
               if (node->child == NULL) {
                  return NULL;
               }
               map_number = prefix_tree_map_character_to_number(string[index]);
               node = node->child[map_number];
            }
      }
   }
   return NULL;
}



double prefix_tree_most_used_domain_percent_of_subdomains(prefix_tree_t *tree, int depth)
{
   if (depth >= MAX_SIZE_OF_DEGREE) {
      return 0;
   }

   if (tree->domain_extension->list_of_most_subdomains[depth] != NULL) {
      return (double)tree->domain_extension->list_of_most_subdomains[depth]->count_of_different_subdomains / (double)tree->root->domain->count_of_different_subdomains;
   }
   return 0;
}



prefix_tree_domain_t *prefix_tree_insert(prefix_tree_t * tree, const char *string, int length)
{
   prefix_tree_domain_t * found, * iter;
   if (tree->prefix_suffix == PREFIX) {
      //prefix tree
      found = prefix_tree_add_domain_recursive_prefix(tree->root, tree->root->domain, string, length, tree);
   } else {
      //suffix tree
      found = prefix_tree_add_domain_recursive_suffix(tree->root, tree->root->domain, string, length, tree);
   }
   if (found == NULL) {
      //exception or error
      return NULL;
   }

   found->count_of_insert++;
   tree->count_of_inserting++;
   if (tree->domain_extension != NULL) {
      iter = found;
      if (found->count_of_insert == 1) {
         //just one search
         //Because of the speed, it is better to devide used and unused list. The unused list is not sorted, and used is sorted
         tree->count_of_domain_searched_just_ones++;
         tree->count_of_different_domains++;
         tree->count_of_inserting_for_just_ones++;
         //first candidate
         if (tree->domain_extension->list_of_most_unused_domains == NULL) {
            tree->domain_extension->list_of_most_unused_domains=iter;
         } else {
            iter->domain_extension->most_used_domain_less = tree->domain_extension->list_of_most_unused_domains;
            tree->domain_extension->list_of_most_unused_domains->domain_extension->most_used_domain_more = iter;
            tree->domain_extension->list_of_most_unused_domains=iter;
         }
      } else if (found->count_of_insert == MAX_COUNT_TO_BE_IN_JUST_ONE_SEARCHER) {
         tree->count_of_inserting_for_just_ones += MAX_COUNT_TO_BE_IN_JUST_ONE_SEARCHER-1;
         tree->count_of_domain_searched_just_ones--;
         //delete from the most unused list
         if (iter->domain_extension->most_used_domain_more != NULL) {
            iter->domain_extension->most_used_domain_more->domain_extension->most_used_domain_less = iter->domain_extension->most_used_domain_less;
         } else {
            tree->domain_extension->list_of_most_unused_domains = iter->domain_extension->most_used_domain_less;
         }
         if (iter->domain_extension->most_used_domain_less != NULL) {
            iter->domain_extension->most_used_domain_less->domain_extension->most_used_domain_more = iter->domain_extension->most_used_domain_more;
         }
         iter->domain_extension->most_used_domain_less = iter->domain_extension->most_used_domain_more = NULL;
      } else if (found->count_of_insert > MAX_COUNT_TO_BE_IN_JUST_ONE_SEARCHER) {
         tree->count_of_inserting_for_just_ones++;
      }

      if (found->count_of_insert > ADD_TO_LIST_FROM_COUNT_OF_SEARCH) {
         //add or sort in list count_of_insert
         if (tree->domain_extension->list_of_most_used_domains == NULL && tree->domain_extension->list_of_most_used_domains_end == NULL ) {
            //on the begeing set the best, and the worse
            tree->domain_extension->list_of_most_used_domains=iter;
            tree->domain_extension->list_of_most_used_domains_end=iter;
         } else {
            //new domain in tree, set it to the end of the list
            if (iter->domain_extension->most_used_domain_more  == NULL && iter->domain_extension->most_used_domain_less  == NULL && iter != tree->domain_extension->list_of_most_used_domains_end) {
               tree->domain_extension->list_of_most_used_domains_end->domain_extension->most_used_domain_less = iter;
               iter->domain_extension->most_used_domain_more = tree->domain_extension->list_of_most_used_domains_end;
               tree->domain_extension->list_of_most_used_domains_end=iter;
            }

            //if it is more used than other, than move forward
            while (iter->domain_extension->most_used_domain_more != NULL && iter->domain_extension->most_used_domain_more->count_of_insert < iter->count_of_insert ) {
               prefix_tree_domain_t * help;
               help = iter->domain_extension->most_used_domain_more;
               iter->domain_extension->most_used_domain_more = help->domain_extension->most_used_domain_more;
               help->domain_extension->most_used_domain_less = iter->domain_extension->most_used_domain_less;
               help->domain_extension->most_used_domain_more = iter;
               iter->domain_extension->most_used_domain_less = help;
               if (iter->domain_extension->most_used_domain_more != NULL) {
                  iter->domain_extension->most_used_domain_more->domain_extension->most_used_domain_less = iter;
               } else {
                  //on the top
                  tree->domain_extension->list_of_most_used_domains=iter;
               }

               if (help->domain_extension->most_used_domain_less != NULL) {
                  help->domain_extension->most_used_domain_less->domain_extension->most_used_domain_more = help;
               }
               if (help->domain_extension->most_used_domain_less == NULL) {
                  tree->domain_extension->list_of_most_used_domains_end = help;
               }
            }
         }
      }
   }
   //add or sort in list count_of_different_subdomains
   return found;
}



prefix_tree_domain_t *prefix_tree_add_string_exception(prefix_tree_t *tree, const char *string, int length)
{
   prefix_tree_domain_t *found;
   if (tree->prefix_suffix == PREFIX) {
      //prefix tree
      found = prefix_tree_add_domain_recursive_prefix(tree->root, tree->root->domain, string, length, tree);
   } else {
      //suffix tree
      found = prefix_tree_add_domain_recursive_suffix(tree->root, tree->root->domain, string, length, tree);
   }
   if (found != NULL) {
      found->exception = 1;
   }
   return found;
}
