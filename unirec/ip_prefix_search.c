/**
 * \file ip_prefix_search.c
 * \brief Init and prefix search
 * \author Erik Sabik <xsabik02@stud.fit.vutbr.cz>
 * \author Ondrej Ploteny <xplote01@stud.fit.vutbr.cz>
 * \date 2013
 * \date 2014
 * \date 2016
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
#include <stdint.h>
#include <stdlib.h>
#include "ip_prefix_search.h"
#include "ipps_internal.h"

/**
 * Function for swapping bits in byte.
 * \param[in] in Input byte.
 * \return Byte with reversed bits.
 */
uint8_t bit_endian_swap(uint8_t in)
{
   in = (in & 0xF0) >> 4 | (in & 0x0F) << 4;
   in = (in & 0xCC) >> 2 | (in & 0x33) << 2;
   in = (in & 0xAA) >> 1 | (in & 0x55) << 1;
   return in;
}

/**
 * Create 2D array for IPv6 networks mask
 * Create 2D array `net_mask_array` with 129 rows and 4 columns and
 * fill it with every possible IPv6 network mask.
 * \return Pointer to 2D array.
 */
uint32_t **create_ip_v6_net_mask_array()
{
   int i, j;
   uint32_t **net_mask_array = malloc(129 * sizeof(uint32_t *));
   if (net_mask_array == NULL) {
      return NULL;
   }

   for (i = 0; i < 129; i++) {
      net_mask_array[i] = malloc(4 * sizeof(uint32_t));
      if (net_mask_array[i] == NULL) {
         return NULL;
      }
      // Fill every word of IPv6 address
      net_mask_array[i][0] = 0xFFFFFFFF>>(i > 31 ? 0 : 32 - i);
      net_mask_array[i][1] = 0xFFFFFFFF>>(i > 63 ? 0 : (i > 32 ? 64 - i: 32));
      net_mask_array[i][2] = 0xFFFFFFFF>>(i > 95 ? 0 : (i > 64 ? 96 - i: 32));
      net_mask_array[i][3] = 0xFFFFFFFF>>(i > 127 ? 0 : (i > 96 ? 128 - i : 32));

      // Swap bits in every byte for compatibility with ip_addr_t stucture
      for (j = 0; j < 4; ++j) {
         net_mask_array[i][j] = (bit_endian_swap((net_mask_array[i][j] & 0x000000FF) >>  0) <<  0) |
                                (bit_endian_swap((net_mask_array[i][j] & 0x0000FF00) >>  8) <<  8) |
                                (bit_endian_swap((net_mask_array[i][j] & 0x00FF0000) >> 16) << 16) |
                                (bit_endian_swap((net_mask_array[i][j] & 0xFF000000) >> 24) << 24);
      }
   }

   return net_mask_array;
}

/**
 * Destroy 2D array for IPv6 networks mask
 * Dealloc 2D array `net_mask_array` with 129 rows and 4 columns and
 * free every possible IPv6 network mask.
 * \param[in] net_mask_array Pointer to 2D array
 * \return void
 */
void destroy_ip_v6_net_mask_array(uint32_t **net_mask_array)
{
   int index;
   for (index = 0; index < 129; index++) {
      free(net_mask_array[index]);
   }
   free(net_mask_array);
}

/**
 * Compare 2 IPv4 network addresses and mask
 * Compare byte by byte 2 IPv4 addresses.  If they are equal, compare mask
 * \param[in] v1 Pointer to network structure.
 * \param[in] v2 Pointer to network structure.
 * \return >0 if v1 IP address is greater than v2 IP address, 0 if equal, <0 if v1 is lower than v2
 */
int cmp_net_v4(const void *v1, const void *v2)
{
   const ipps_network_t *n1 = *(ipps_network_t **)v1;
   const ipps_network_t *n2 = *(ipps_network_t **)v2;

   int ip_cmp_result;

   ip_cmp_result = memcmp(&n1->addr.ui32[2], &n2->addr.ui32[2], 4);
   /* if they are equal - lower mask (greater network) first */
   if (ip_cmp_result == 0) {
      return memcmp(&n1->mask, &n2->mask, 4);
   } else {
      return ip_cmp_result;
   }
}

/**
 * Compare 2 IPv6 network addresses and mask
 * Compare byte by byte 2 IPv6 addresses.  If they are equal, compare mask
 * \param[in] v1 Pointer to network structure.
 * \param[in] v2 Pointer to network structure.
 * \return >0 if v1 IP address is greater than v2 IP address, 0 if equal, <0 if v1 is lower than v2
 */
int cmp_net_v6(const void *v1, const void *v2)
{
   const ipps_network_t *n1 = *(ipps_network_t **)v1;
   const ipps_network_t *n2 = *(ipps_network_t **)v2;

   int ip_cmp_result;

   ip_cmp_result = memcmp(&n1->addr.ui8, &n2->addr.ui8, 16);
   /* if they are equal - lower mask (greater network) first */
   if (ip_cmp_result == 0) {
      return memcmp(&n1->mask, &n2->mask, 4);
   } else {
      return ip_cmp_result;
   }
}

/**
 * Mask IPv6 address
 * Mask IPv6 address `ip` with network mask `mask` and save result to `masked_ipv6`.
 * \param[in] ip Pointer to IP union.
 * \param[in] mask Network mask.
 * \param[out] masked_ipv6 Pointer to IP union.
 * \param[in] net_list Pointer to networks list structure.
 * \return void.
 */
void mask_ipv6(ip_addr_t *ip, uint32_t mask, ip_addr_t *masked_ipv6, uint32_t **net_mask_array)
{
   int i;
   for (i = 0; i < 4; i++) {
      masked_ipv6->ui32[i] = ip->ui32[i] & net_mask_array[mask][i];
   }
}

/**
 * Interval from network and mask
 * Compute low and high IP address (net and broadcast address) from network 'net' using mask,
 * save result to 'inter'
 * \param[in] net Pointer to network structure
 * \param[out] inter Pointer to result interval
 * \param[in] net_mask_array Pointer to 2D array with every possible net mask
 * \return void
 */
void fill_interval_by_network(const ipps_network_t *net, ipps_interval_t *inter,
                              uint32_t **net_mask_array)
{
   inter->data_cnt = 0;
   inter->data_array = NULL;

   /* Fill network address */
   memcpy(&inter->low_ip, &net->addr, 16);

   /* Fill broadcast address */
   if (!ip_is6(&net->addr)) {
      // IPv4
      inter->high_ip.ui64[0] = 0;
      inter->high_ip.ui32[2] = net->addr.ui32[2] | ( ~ net_mask_array[net->mask][0]);
      inter->high_ip.ui32[3] = 0xffffffff;
   } else {
      // IPv6
      int i;
      for (i = 0; i < 4; i++) {
         inter->high_ip.ui32[i] = net->addr.ui32[i] | ( ~ net_mask_array[net->mask][i]);
      }
   }
}

/**
 * Create new interval node
 * Alloc and initialize new node to interval list
 * \param[in] low_ip Pointer to network structure
 * \param[in] high_ip Pointer to result interval
 * \return Pointer to new interval node, NULL if malloc fails
 */
 ipps_interval_node_t *new_interval(const ip_addr_t *low_ip, const ip_addr_t *high_ip)
{

   ipps_interval_node_t * new_node = malloc(sizeof(ipps_interval_node_t));
   if (new_node == NULL) {
      fprintf(stderr, "ERROR allocating memory for network interval node\n");
      return NULL;
   }

   new_node->interval = malloc(sizeof(ipps_interval_t));
   if (new_node->interval == NULL) {
      fprintf(stderr, "ERROR allocating memory for network interval\n");
      free(new_node);
      return NULL;
   }
   new_node->next = NULL;

   /* Initialize struct members */
   memcpy(&new_node->interval->low_ip,  low_ip, 16);
   memcpy(&new_node->interval->high_ip, high_ip, 16);
   new_node->interval->data_cnt = 0;
   new_node->interval->array_len = DATASLOTS;
   new_node->interval->data_array =  malloc(DATASLOTS * sizeof(void *));
   if (new_node->interval->data_array == NULL) {
      fprintf(stderr, "ERROR allocating memory for data pointers\n");
      free(new_node->interval);
      free(new_node);
      return NULL;
   }

   return  new_node;
}

/**
 * Post Insert to interval list
 * Create new interval node and insert them for 'position' node.
 * New created node is initialized and fill by 'low_ip' and 'high_ip' values.
 * \param[in] position Pointer to interval node structure, for post insert
 * \param[in] low_ip Pointer to IP address structure
 * \param[in] high_ip Pointer to IP address structure
 * \return Pointer to new inserted interval node in list, NULL if malloc fails
 */
 ipps_interval_node_t *insert_new_interval(ipps_interval_node_t *position,
                                          const ip_addr_t *low_ip, const ip_addr_t *high_ip)
{
   ipps_interval_node_t *new_interval_node = new_interval(low_ip, high_ip);
   if (new_interval_node == NULL) {
      return NULL;
   }

   /* Post Insert */
   new_interval_node->next = position->next;
   position->next = new_interval_node;

   return new_interval_node;
}

/**
 * Decrement IP address
 * Decrement IPv4 or IPv6 address 'ip' and save result to 'ip_dec'
 * \param[in] ip Pointer to input ip address structure
 * \param[out] ip_dec Pointer to output ip address structure
 * \return void
 */
void ip_dec(const ip_addr_t *ip, ip_addr_t *ip_dec)
{
   if (ip_is6(ip)) {
      memcpy(ip_dec, ip, 16);

      uint32_t tmp = 0xffffffff;
      int i;
      for (i = 3; i >=0; i--) {
         ip_dec->ui32[i] = htonl(ntohl(ip->ui32[i]) - 1);
         if (ip_dec->ui32[i] != tmp) {
            break;
         }
      }
   } else {
      ip_dec->ui64[0] = 0;
      ip_dec->ui32[2] = htonl(ntohl(ip->ui32[2]) - 1);
      ip_dec->ui32[3] = 0xffffffff;
   }
}

/**
 * Increment IP address
 * Increment IPv4 or Ipv6 address 'ip' and save result to 'ip_inc'
 * \param[in] ip Pointer to input ip address structure
 * \param[out] ip_inc Pointer to output ip address structure
 * \return void
 */
void ip_inc(const ip_addr_t *ip, ip_addr_t *ip_inc)
{
   if (ip_is6(ip)) {
      memcpy(ip_inc, ip, 16);

      uint32_t tmp = 0xffffffff;
      int i;
      for (i = 3; i >= 0; i--) {
         ip_inc->ui32[i] = htonl(ntohl(ip->ui32[i]) + 1);
         if (ip->ui32[i] < tmp) {
            break;
         }
      }
   } else {
      ip_inc->ui64[0] = 0;
      ip_inc->ui32[2] = htonl(ntohl(ip->ui32[2]) + 1);
      ip_inc->ui32[3] = 0xffffffff;
   }
}

/**
 * Deinitialize interval_search_context structure
 * Dealloc all memory, garbage collector
 * \param[in] prefix_context Pointer to interval_search_context struct
 * return 0 if dealloc is OK, 1 if free fails
 */
int ipps_destroy(ipps_context_t *prefix_context)
{
   int i;
   void **data_collector;                // Array with freed memory pointers
   uint32_t data_collector_cnt = 0;       // Number of pointers in 'data_collector'

   if (prefix_context == NULL) {
      fprintf(stderr, "ERROR NULL pointer passed to ipps_destroy\n");
      return 1;
   }

   data_collector = malloc(COLLECTORSLOTS * sizeof(void *));
   if (data_collector == NULL) {
      fprintf(stderr, "ERROR allocating memory for freed data collector\n");
      return 1;
   }

   // Dealloc all IPv4 data and intervals
   for (i = 0; i < prefix_context->v4_count; ++i) {
      if (free_data(&prefix_context->v4_prefix_intervals[i], &data_collector, &data_collector_cnt)) {
         return 1;
      }
   }

   // Dealloc all IPv6 data and intervals
   data_collector_cnt = 0;
   for (i = 0; i < prefix_context->v6_count; ++i) {
      if (free_data(&prefix_context->v6_prefix_intervals[i], &data_collector, &data_collector_cnt)) {
         return 1;
      }
   }

   free(data_collector);
   free(prefix_context->v4_prefix_intervals);
   free(prefix_context->v6_prefix_intervals);
   free(prefix_context);
   return 0;
}

/**
 * Create and initialize new interval_search_context structure
 * \return Pointer to interval_search_context struct, NULL if alloc fails
 */
ipps_context_t *new_context()
{
   ipps_context_t *prefix_context = malloc(sizeof(ipps_context_t));
   if (prefix_context == NULL) {
      fprintf(stderr, "ERROR allocating memory for network mask array\n");
      return NULL;
   }

   prefix_context->v4_count = 0;
   prefix_context->v6_count = 0;
   prefix_context->v4_prefix_intervals = NULL;
   prefix_context->v6_prefix_intervals = NULL;

   return prefix_context;
}

/**
 * Initialize ipps_context_t structure, fill IPv4 and IPv6 ipps_interval arrays
 * Function compute for all networks in 'network_list' appropriate intervals and copied network data
 * Overlapping intervals are split.  Array is sorted by low IP addr of interval.
 * Networks in network list is not necessary sorted, 'ipps_init' mask and sort each network itself
 * \param[in] network_list Pointer to network list structure
 * \return NULL if memory alloc fails, Pointer to ipps_context structure
 */
ipps_context_t *ipps_init(ipps_network_list_t *network_list)
{
   int index;
   ip_addr_t *masked_ip;

   if (network_list == NULL) {
      fprintf(stderr, "ERROR Network list is not initialized");
      return NULL;
   }

   if (network_list->net_count <= 0) {
      fprintf(stderr, "ERROR Network lists are empty, nothing to do");
      return NULL;
   }

   // Create new interval_search_context
   ipps_context_t *prefix_context = new_context();
   if (prefix_context == NULL) {
      return NULL;
   }

   // Create and fill net mask array - for masking IP
   uint32_t **net_mask_array = create_ip_v6_net_mask_array();
   if (net_mask_array == NULL) {
      fprintf(stderr, "ERROR allocating memory for network mask array\n");
      return NULL;
   }

   ipps_network_t *current_net;             // Currently precessed network
   void *tmp;

   ipps_network_t  **networks_v6;           // Pointers to ipv6 networks
   ipps_network_t  **networks_v4;           // Pointers to ipv4 networks


   uint32_t i_v6 = 0;         // Number of ipv6 networks
   uint32_t i_v4 = 0;         // Number of ipv4 networks

   size_t i_v6_alloc = NETWORKSLOTS;      // Number of available network pointers
   size_t i_v4_alloc = NETWORKSLOTS;      // Number of available network pointers

   // Allocate memory for network array
   if ((networks_v4 = malloc(i_v4_alloc * sizeof(ipps_network_t *))) == NULL ||
       (networks_v6 = malloc(i_v6_alloc * sizeof(ipps_network_t *))) == NULL) {
      fprintf(stderr, "ERROR allocating sorted network structures\n");
      return NULL;
   }

   // For each network in array - mask ip address and split to ipv4 or ipv6 network
   for (index = 0; index < network_list->net_count; ++index)
   {
      current_net = &network_list->networks[index];
      if (ip_is6(&current_net->addr)) {
         masked_ip = &current_net->addr;
         mask_ipv6(&current_net->addr, current_net->mask, masked_ip, net_mask_array);

         i_v6++;
         if (i_v6_alloc < i_v6) {
            i_v6_alloc *= 2;
            tmp = realloc(networks_v6, i_v6_alloc * sizeof(ipps_network_t *));
            if (tmp  == NULL) {
               fprintf(stderr, "ERROR allocating memory for ipv6 network collector\n");
               return NULL;
            }
            networks_v6 = tmp;

         }
         networks_v6[i_v6-1] = current_net;
      } else {
         masked_ip = &current_net->addr;
         masked_ip->ui32[2] = masked_ip->ui32[2] & net_mask_array[current_net->mask][0];

         i_v4++;
         if (i_v4_alloc < i_v4) {
            i_v4_alloc *= 2;
            tmp = realloc(networks_v4, i_v4_alloc * sizeof(ipps_network_t *));
            if (tmp == NULL) {
               fprintf(stderr, "ERROR allocating memory for ipv6 network collector\n");
               return NULL;
            }
            networks_v4 = tmp;
         }
         networks_v4[i_v4 - 1] = current_net;
      }
   }

   if (i_v4 > 0 && networks_v4[0] != NULL) {
      qsort(networks_v4, i_v4, sizeof(ipps_network_t *), cmp_net_v4);

      // Fill intervals for IPv4 to interval_search_context array, if fail, dealloc and rtrn NULL
      prefix_context->v4_prefix_intervals = init_context(networks_v4, i_v4,
                                                         &prefix_context->v4_count, net_mask_array);
      if (prefix_context->v4_prefix_intervals == NULL) {
         destroy_ip_v6_net_mask_array(net_mask_array);
         free(prefix_context);
         return NULL;
      }
      /************************/
   }
   free(networks_v4);

   if (i_v6 > 0 && networks_v6[0] != NULL) {
      qsort(networks_v6, i_v6, sizeof(ipps_network_t *), cmp_net_v6);

      // Fill intervals for IPv6 to interval_search_context array, if fail, dealloc and return NULL
      prefix_context->v6_prefix_intervals = init_context(networks_v6, i_v6,
                                                         &prefix_context->v6_count, net_mask_array);
      if (prefix_context->v6_prefix_intervals == NULL) {
         destroy_ip_v6_net_mask_array(net_mask_array);
         ipps_destroy(prefix_context);
         free(prefix_context);
         return NULL;
      }

      /************************/
   }
   free(networks_v6);

   destroy_ip_v6_net_mask_array(net_mask_array);
   return  prefix_context;
}

/**
 * Append data in 'dest' with all data from 'src' interval
 * Concat 'dest' and 'src' data_arrays: if necessary realloc destination data array.
 * Copy all data pointers from src, behind last 'dest' data pointer
 * \param[out] dest Pointer to destination interval
 * \param[in] src Pointer to source interval
 * \return 0 if OK, 1 if data array realloc fails
 */
int copy_all_data(ipps_interval_t *dest, ipps_interval_t *src)
{

   if (dest->data_cnt + src->data_cnt > dest->array_len) {
      // no free pointer slots for src data in dest data_array
      void **tmp;
      dest->array_len += src->array_len;
      tmp = realloc (dest->data_array, dest->array_len * sizeof(void *));
      if (tmp  == NULL) {
         fprintf(stderr, "ERROR allocating memory for network mask array\n");
         return 1;
      }
      dest->data_array = tmp;
   }

   memcpy(dest->data_array + dest->data_cnt, src->data_array, src->data_cnt * sizeof(void *));
   dest->data_cnt += src->data_cnt;
   return 0;
}

/**
 * Add data to interval data array
 * Alloc memory size of 'data_len' and hard copy 'data'.
 * Pointer to new data is insert to 'data_array' in interval structure 'interval', increment
 * data counter. Realloc 'data_array' in 'interval' if there is not enough unused pointers
 * \param[in] interval Pointer to interval structure with data array and data counter
 * \param[in] data Pointer to same data
 * \param[in] data_len Number of bytes allocated in 'data'
 * return 0 if OK, 1 if alloc fails
 */
int add_data(ipps_interval_t *interval, void *data, size_t data_len)
{
   // Alloc new data and copy
   void *new_data = malloc(data_len);
   if (new_data == NULL) {
      fprintf(stderr, "ERROR allocating memory for network mask array\n");
      return 1;
   }

   memcpy(new_data, data, data_len);

   // Insert new data to interval's data array - first do some space ...
   interval->data_cnt++;
   if (interval->data_cnt > interval->array_len) {
      void **tmp;
      interval->array_len *= 2;
      tmp = realloc (interval->data_array, interval->array_len * sizeof(void *));
      if (tmp  == NULL) {
         fprintf(stderr, "ERROR allocating memory for network mask array\n");
         free(new_data);
         return 1;
      }
      interval->data_array = tmp;
   }

   // ... then push new data to array
   interval->data_array[interval->data_cnt - 1] = new_data;

   return 0;
}

/**
 * Initialize array of intervals
 * For each network in 'networks' array compute IP interval and insert it into
 * interval list.  All network data are hard copied.  Overlapping intervals are
 * split and sorted.  If intervals are overlaps, data are allocated only once
 * and pointer to data is duplicated.  Overlapping is detected by comparing low
 * and high IP addresses. At the end the sorted list is copied to an array for
 * better access.
 * Returns pointer to sorted array of intervals and fill 'context_counter' by
 * number of intervals.
 * \param[in] networks Pointer to array of network structures
 * \param[in] network_count Number of networks in array
 * \param[out] context_counter Pointer to integer, fill by number of intervals in result
 * \param[in] net_mask_array Pointer to 2D array of network mask
 * \return Pointer to array of interval structures, NULL if memory alloc fails
 */
ipps_interval_t *init_context( ipps_network_t **networks, uint32_t network_count,
                               uint32_t *context_counter, uint32_t **net_mask_array)
{
   uint32_t interval_counter = 0;

   ipps_interval_t current_interval;
   ipps_interval_node_t *interval_list = NULL;

   int index = 0;

   // push first interval from network tree to interval_search_context interval list
   // compute ip interval - low and high ip addresses
   fill_interval_by_network(networks[0], &current_interval, net_mask_array);
   // insert root interval node to list
   interval_list = new_interval(&current_interval.low_ip, &current_interval.high_ip);
   if (interval_list == NULL) {
      return NULL;
   }

   if (add_data(interval_list->interval, networks[0]->data, networks[0]->data_len)) {
      // add data to root node
      return NULL;
   }
   interval_counter++;                             // number of intervals

   ipps_interval_node_t *conductor = NULL;               // list iterator
   ipps_interval_node_t *end_of_list = interval_list;    // last element in the list

   int ip_cmp_result;       // result of ip comparison
   ip_addr_t tmp_ip ;      // temporary ip addr union for compute first or last IP address of interval

   conductor = interval_list;

   // traverse rest of networks tree
   for (index = 1; index < network_count; ++index) {
      // fill temporary interval variable by current interval
      fill_interval_by_network(networks[index], &current_interval, net_mask_array);

      while (conductor != NULL) {
         // compare low IP of actual processed interval with high IP all intervals in list
         ip_cmp_result = ip_cmp( &conductor->interval->high_ip, &current_interval.low_ip);

         if (ip_cmp_result >= 0) {
            ip_cmp_result = ip_cmp( &conductor->interval->low_ip, &current_interval.low_ip);
            if (ip_cmp_result > 0) {
               // LowI > LowT
               ip_cmp_result = ip_cmp( &conductor->interval->high_ip, &current_interval.high_ip);
               if (ip_cmp_result > 0) {
                  // Con   <---------->
                  // Cur <---------->

                  fprintf(stderr, "ERROR Inserting to list");
                  destroy_list(interval_list);
                  return NULL;
               } else if (ip_cmp_result < 0) {
                  // Con   <-------->
                  // Cur <------------>

                  fprintf(stderr, "ERROR Inserting to list");
                  destroy_list(interval_list);
                  return NULL;
               } else {
                  // Con   <-------->
                  // Cur <---------->

                  fprintf(stderr, "ERROR Inserting to list");
                  destroy_list(interval_list);
                  return NULL;
               }
            } else if (ip_cmp_result < 0) {
               // LowI < LowT
               ip_cmp_result = ip_cmp( &conductor->interval->high_ip, &current_interval.high_ip);
               if (ip_cmp_result > 0) {
                  // Con <---------->
                  // Cur   <----->

                  /***********************/
                  /*    | |   ↓  |  |
                   *    <----------->
                   *     <----->
                   */

                  // Insert new interval to interval tree, conductor post insert
                  if (insert_new_interval(conductor, &current_interval.low_ip,
                                          &current_interval.high_ip) == NULL) {
                     destroy_list(interval_list);
                     return NULL;
                  }
                  interval_counter++;

                  // Fill data array in new interval node
                  // Copy original data and add new one from current network
                  if (copy_all_data(conductor->next->interval, conductor->interval)) {
                     destroy_list(interval_list);
                     return NULL;
                  }
                  if (add_data(conductor->next->interval, networks[index]->data, networks[index]->data_len)) {
                     destroy_list(interval_list);
                     return NULL;
                  }
                  /***********************/

                  /***********************/
                  /*
                   *   | |      |↓ |
                   * Con <----------->
                   * Cur   <----->
                   */

                  // Insert new interval to interval list
                  ip_inc(&current_interval.high_ip, &tmp_ip);
                  if (insert_new_interval(conductor->next, &tmp_ip, &conductor->interval->high_ip) == NULL) {
                     destroy_list(interval_list);
                     return NULL;
                  }
                  interval_counter++;

                  if (copy_all_data(conductor->next->next->interval, conductor->interval)) {
                     destroy_list(interval_list);
                     return  NULL;
                  }

                  /***********************/

                  /***********************/
                  /*      |↓|     |   |
                   *  Con <----------->
                   *  Cur   <----->
                   */

                  // Modify original interval
                  ip_dec(&current_interval.low_ip, &tmp_ip);
                  memcpy(&conductor->interval->high_ip,  &tmp_ip, 16);
                  /***********************/

                  // Modify end of interval list, 2 follower inserted
                  if (end_of_list == conductor) {
                     end_of_list = conductor->next->next;
                  }
                  break;
               }
               else if (ip_cmp_result < 0) {
                  // Con <---------->
                  // Cur   <----------->

                  fprintf(stderr, "ERROR Inserting to list");
                  destroy_list(interval_list);
                  return NULL;
               } else {
                  // Con <---------->
                  // Cur   <-------->

                  if (insert_new_interval(conductor, &current_interval.low_ip,
                                          &conductor->interval->high_ip) == NULL) {
                     destroy_list(interval_list);
                     return NULL;
                  }
                  interval_counter++;

                  if (copy_all_data(conductor->next->interval, conductor->interval)) {
                     destroy_list(interval_list);
                     return NULL;
                  }
                  if (add_data(conductor->next->interval, networks[index]->data,
                               networks[index]->data_len)) {
                     destroy_list(interval_list);
                     return  NULL;
                  }

                  if (end_of_list == conductor) {
                     end_of_list = conductor->next;
                  }

                  ip_dec(&current_interval.low_ip, &tmp_ip);
                  memcpy(&conductor->interval->high_ip,  &tmp_ip, 16);
               }
            } else {
               ip_cmp_result = ip_cmp( &conductor->interval->high_ip, &current_interval.high_ip);
               if (ip_cmp_result > 0) {
                  // Con <---------->
                  // Cur <-------->

                  ip_inc(&current_interval.high_ip, &tmp_ip);

                  insert_new_interval(conductor, &tmp_ip, &conductor->interval->high_ip);
                  interval_counter++;

                  copy_all_data(conductor->next->interval, conductor->interval);

                  if (end_of_list == conductor) {
                     end_of_list = conductor->next;
                  }
                  memcpy(&conductor->interval->high_ip,  &current_interval.high_ip, 16);
                  add_data(conductor->interval, networks[index]->data, networks[index]->data_len);

                  break;
               } else if (ip_cmp_result < 0) {
                  // Con <-------->
                  // Cur <---------->

                  fprintf(stderr, "ERROR Inserting to list");
                  destroy_list(interval_list);
                  return NULL;
               } else {
                  //  Con <-------->
                  //  Cur <-------->

                  if (add_data(conductor->interval, networks[index]->data,
                               networks[index]->data_len)) {
                     destroy_list(interval_list);
                     return NULL;
                  }
               }
            }

            break;
         } else if (ip_cmp_result < 0) {
            // Low ip address of current is higher then high ip of interval list member,
            // take next interval in list
            conductor = conductor->next;
         } else {
            // Useless branch, memcmp error
            fprintf(stderr, "ERROR Inserting to list");
            destroy_list(interval_list);
            return NULL;
         }
      }

      if (conductor == NULL) {
         // New interval at the end of list
         if (insert_new_interval(end_of_list, &current_interval.low_ip, &current_interval.high_ip) == NULL) {
            destroy_list(interval_list);
            return NULL;
         }

         // Add new data to new end of list
         end_of_list = end_of_list->next;

         if (add_data(end_of_list->interval, networks[index]->data, networks[index]->data_len)) {
            destroy_list(interval_list);
            return NULL;
         }
         interval_counter++;

         conductor = end_of_list;
      }
   }

   // Alloc new interval array
   ipps_interval_t *prefix_context = (ipps_interval_t *)malloc(interval_counter * sizeof(ipps_interval_t));
   if (prefix_context == NULL) {
      fprintf(stderr, "ERROR allocating memory for prefix interval_search_context\n");
      return NULL;
   }

   // Fill interval array
   // Hard copy intervals from list to array, duplicate data array pointer, free all list node
   conductor = interval_list;
   ipps_interval_t *array_iterator = prefix_context;
   size_t size_of_pref_interval = sizeof(ipps_interval_t);
   *context_counter = interval_counter;

   // Iterate whole interval list
   while (conductor != NULL) {
      // Copy from list to array
      memcpy(array_iterator, conductor->interval, size_of_pref_interval);
      array_iterator++;    // next interval in array

      // Destroy list node, Don't destroy data!
      interval_list = conductor->next;
      free(conductor->interval);
      free(conductor);
      conductor = interval_list;
   }

   return prefix_context;
}

/**
 * Binary search in interval_search_context
 * Binary search 'ip' in intervals interval_search_context, 'ip' is compare with 'low_ip'
 * and 'high_ip' of interval.
 * if match, fill 'data' pointer by pointer to data_array of interval and return number of data members
 * if no match return 0 and 'data' pointer not fill
 * Optimized for IPv4 or IPv6 search
 * \param[in] ip Pointer to ip address structure
 * \param[in] prefix_context Pointer to interval_search_context structure
 * \param[out] data Pointer to array of void pointers - pointers to data
 * \return int 0 if no match, >0 Number of data members in matched interval
 */
int ipps_search(ip_addr_t *ip, ipps_context_t *prefix_context, void ***data)
{
   int first, last, middle;               // Array indexes

   ipps_interval_t *interval_array;      // Pointer to IPv4 or IPv6 interval array
   uint8_t *middle_interval;             // Pointer to first byte of current middle interval

   int ip_high_cmp_result;                // Result of comparing high IP of 2 intervals
   int ip_low_cmp_result;                 // Result of comparing low IP of 2 intervals

   void *ip_addr_start;  // Ptr to first useful byte in ip_addr_t union, etc offset ui32[2] in IPv4

   size_t ip_addr_len = sizeof(ip_addr_t);
   size_t addr_cmp_len;           // Number of compare bytes, 4 for IPv4, 16 for IPv6 (IP addr size)

   size_t low_ip_offset;          // Offset of 'low_ip' in 'ipps_interval_t' structure
   size_t high_ip_offset;         // Offset of 'high' in 'ipps_interval_t' structure

   if (ip_is6(ip)) {
      if (prefix_context->v6_count == 0) {
         return 0;
      }
      first = 0;
      last = prefix_context->v6_count - 1;
      middle = (first + last)>>1;

      interval_array = prefix_context->v6_prefix_intervals;

      low_ip_offset = 0;                     // interval->low_ip is first member of struct
      high_ip_offset = ip_addr_len;          // interval->high_ip is second member of struct
      addr_cmp_len = 16;                     // Compare length, 16 bytes for IPv6
      ip_addr_start = (uint8_t *)ip;

   } else {
      if (prefix_context->v4_count == 0) {
         return 0;
      }
      first = 0;
      last   = prefix_context->v4_count - 1;
      middle = (first + last)>>1;

      interval_array = prefix_context->v4_prefix_intervals;

      low_ip_offset =  8;                          // low_ip.ui32[2]
      high_ip_offset = ip_addr_len + 8;            // high.ui32[2]
      addr_cmp_len = 4;                            // Compare length, 4 bytes for IPv4

      ip_addr_start = ((uint8_t *)ip) + 8;         // ip.ui32[2]
   }

   while (first <= last ) {
      middle_interval = (uint8_t *)(interval_array + middle);

      ip_low_cmp_result = memcmp(middle_interval + low_ip_offset, ip_addr_start, addr_cmp_len);
      ip_high_cmp_result = memcmp(middle_interval + high_ip_offset, ip_addr_start, addr_cmp_len);

      if (ip_low_cmp_result <= 0 && ip_high_cmp_result >= 0) {
         *data = ((ipps_interval_t *)middle_interval)->data_array;
         return ((ipps_interval_t *)middle_interval)->data_cnt;
      } else if (ip_high_cmp_result > 0) {
         last = middle - 1;
      } else {
         first = middle + 1;
      }
      middle = (first + last) >> 1;
   }
   return 0;
}

/**
 * Dealloc 'data_array' in 'interval'
 * \param[in] interval Pointer to prefix interval structure
 * \param[out] data_collector Pointer to 2D array with freed data pointers
 * \param[out] data_collector_cnt Number of Pointers in 'data_collector'
 * \return 0 if realloc is OK, 1 if realloc fail
 */
int free_data(ipps_interval_t *interval, void ***data_collector, uint32_t *data_coll_cnt )
{
   int j,k;
   void **tmp;

   for (j = 0; j < interval->data_cnt; ++j) {
      for (k = 0; k < *data_coll_cnt; ++k) {
         if (interval->data_array[j] == (*data_collector)[k]) {
            interval->data_array[j] = NULL;  // pointer has been freed
            break;
         }
      }

      if (k == *data_coll_cnt) {
         // Data not match in data_collector, add data pointer to collector and free memory
         if (*data_coll_cnt >= COLLECTORSLOTS && *data_coll_cnt % COLLECTORSLOTS  == 0) {
            tmp = realloc(*data_collector, ((*data_coll_cnt) + COLLECTORSLOTS) * sizeof(void *));
            if (tmp == NULL) {
               fprintf(stderr, "ERROR allocating memory for network mask array\n");
               return 1;
            }
            *data_collector = tmp;
         }

         (*data_collector)[*data_coll_cnt] = interval->data_array[j];   // Add pointer to collector
         (*data_coll_cnt)++;

         free(interval->data_array[j]);      // free data
      }
   }
   free(interval->data_array);               // free pointers to data
   return 0;
}

/**
 * Dealloc interval list
 * Dealloc all node in 'interval_list', dealloc data
 * Function is call if something get wrong and program need clean garbage in the middle of run
 * \param[in] interval_list Pointer to first node in list
 * \return 0 if OK, 1 if data collector realloc fails
 */
int destroy_list(ipps_interval_node_t *interval_list)
{
   void **data_collector;                   // pointers to freed memory
   uint32_t data_collector_cnt = 0;
   ipps_interval_node_t *tmp_interval;

   data_collector = malloc(COLLECTORSLOTS * sizeof(void *));
   if (data_collector == NULL) {
      fprintf(stderr, "ERROR allocating memory for freed data collector\n");
      return 1;
   }

   while (interval_list != NULL) {
      tmp_interval = interval_list;
      interval_list = interval_list->next;

      if (free_data(tmp_interval->interval, &data_collector, &data_collector_cnt)) {
         return 1;
      }
      free(tmp_interval->interval);
      free(tmp_interval);
   }
   free(data_collector);

   return 0;
}
