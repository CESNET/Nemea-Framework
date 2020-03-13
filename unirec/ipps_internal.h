/**
 * \file ipps_internal.h
 * \brief Init context and prefix search - Internal functions and structures
 * \author Erik Sabik <xsabik02@stud.fit.vutbr.cz>
 * \author Ondrej Ploteny <xplote01@stud.fit.vutbr.cz>
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

#ifndef IPPS_INTERNAL_H
#define IPPS_INTERNAL_H

/**
 * Structure of temporary list of intervals
 * Used for sort and merge overlaps intervals
 */
typedef struct ipps_interval_node {
    ipps_interval_t *interval;                 ///< Pointer to interval structure
    struct ipps_interval_node *next;           ///< Next node in list, NULL if last node in list
}ipps_interval_node_t;

/**
 * Create 2D array for IPv6 networks mask
 * Create 2D array `net_mask_array` with 129 rows and 4 columns and
 * fill it with every possible IPv6 network mask.
 * @return Pointer to 2D array.
 */
uint32_t **create_ip_v6_net_mask_array(void);

/**
 * Mask IPv6 address
 * Mask IPv6 address `ip` with network mask `in_mask` and save result to `masked_ipv6`.
 * \param[in] ip Pointer to IP union.
 * \param[in] mask Network mask.
 * \param[out] masked_ipv6 Pointer to IP union.
 * \param[in] net_list Pointer to networks list structure.
 * \return void.
 */
void mask_ipv6(ip_addr_t *ip, uint32_t mask, ip_addr_t *masked_ipv6, uint32_t **net_mask_array);

/**
 * Compare 2 IPv4 network addresses
 * Compare byte by byte 2 IPv4 addresses
 * \param[in] v1 Pointer to network structure.
 * \param[in] v2 Pointer to network structure.
 * \return >0 if v1 IP address is greater than v2 IP address, 0 if equal, <0 if v1 is lower than v2
 */
int cmp_net_v4(const void *v1, const void *v2);

/**
 * Compare 2 IPv6 network addresses
 * Compare byte by byte 2 IPv6 addresses.
 * \param[in] v1 Pointer to network structure.
 * \param[in] v2 Pointer to network structure.
 * \return >0 if v1 IP address is greater than v2 IP address, 0 if equal, <0 if v1 is lower than v2
 */
int cmp_net_v6(const void *v1, const void *v2);

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
                                     uint32_t **net_mask_array);

/**
 * Create new interval node
 * Alloc and initialize new node to interval list
 * \param[in] low_ip Pointer to network structure
 * \param[in] high_ip Pointer to result interval
 * \return NULL if malloc fail, pointer to new interval node
 */
ipps_interval_node_t *new_interval(const ip_addr_t *low_ip, const ip_addr_t *high_ip);

/**
 * Post Insert to interval list
 * Create new interval node and insert them for 'position' node.
 * New created node is initialized and fill by 'low_ip' and 'high_ip' values.
 * \param[in] position Pointer to interval node structure, for post insert
 * \param[in] low_ip Pointer to IP address structure
 * \param[in] high_ip Pointer to IP address structure
 * \return NULL if malloc fails, else pointer to new inserted interval node in list
 */
ipps_interval_node_t *insert_new_interval(ipps_interval_node_t *position,
                                                 const ip_addr_t *low_ip, const ip_addr_t *high_ip);

/**
 * Decrement IP address
 * Decrement IPv4 or IPv6 address 'ip' and save result to 'ip_dec'
 * \param[in] ip Pointer to ip address structure
 * \param[out] ip_dec Pointer to ip address structure
 * \return void
 */
void ip_dec(const ip_addr_t *ip, ip_addr_t *ip_dec);

/**
 * Increment IP address
 * Increment IPv4 or Ipv6 address 'ip' and save result to 'ip_inc'
 * \param[in] ip Pointer to network structure
 * \param[out] ip_inc Pointer to result interval
 * \return void
 */
void ip_inc(const ip_addr_t *ip, ip_addr_t *ip_inc);

/**
 * Create and initialize new interval_search_context structure
 * \return Pointer to interval_search_context struct, NULL if alloc fails
 */
ipps_context_t *new_context(void);


/**
 * Initialize array of intervals
 * Function for each network in 'networks' array compute IP interval and insert interval
 * to interval list.  Each network data are hard copied. Overlapping intervals are splited
 * and sorted.  If intervals are overlaps, data are alloc only once and pointer to data
 * is duplicated.  Overlapping is detected by compare lows and highs IP addresses.
 * End the end is sorted list copied to array for better access
 * Function return pointer to sorted array of intervals and fill 'context_counter' by numbers
 * of intervals.
 * \param[in] networks Pointer to array of network structures
 * \param[in] network_count Number of networks in array
 * \param[out] context_counter Pointer to integer, fill by number of intervals in result
 * \param[in] net_mask_array Pointer to 2D array of network mask
 * \return NULL if memory alloc fails, Pointer to array of interval structure
 */
ipps_interval_t *init_context(ipps_network_t **networks, uint32_t network_count,
                                     uint32_t *context_counter, uint32_t **net_mask_array);

/**
 * Add data to interval data array
 * Alloc memory size of 'data_len' and hard copy 'data'.
 * Pointer to new data is insert to 'data_array' in interval structure 'interval', increment
 * data counter.  Realloc 'data_array' in 'interval' if there is not enough unused pointers
 * \param[in] interval Pointer to interval structure with data array and data counter
 * \param[in] data Pointer to same data
 * \param[in] data_len Number of bytes allocated in 'data'
 * \return 0 if OK, 1 if alloc fails
 */
int add_data(ipps_interval_t *interval, void *data, size_t data_len);

/**
 * Append data in 'dest' with all data from 'src' interval
 * Concat 'dest' and 'src' data_arrays: if necessary, realloc destination data array.
 * Copy all data pointers from src, behind last 'dest' data pointer
 * \param[out] dest Pointer to destination interval
 * \param[in] src Pointer to source interval
 * \return 0 if OK, 1 if data array realloc fails
 */
int copy_all_data(ipps_interval_t *dest, ipps_interval_t *src);

/**
 * Dealloc network mask array
 * Dealloc array with every possible IPv6 mask
 * \param[in] net_mask_array Pointer to 2D array
 * \return void
 */
void destroy_ip_v6_net_mask_array(uint32_t **net_mask_array);


/**
 * Dealloc 'data_array' in 'interval'
 * \param[in] interval Pointer to prefix interval structure
 * \param[out] data_collector Pointer to 2D array with freed data pointers
 * \param[out] data_collector_cnt Number of Pointers in 'data_collector'
 * \return 0 if realloc is OK, 1 if realloc fail
 */
int free_data(ipps_interval_t *interval, void ***data_collector, uint32_t *data_coll_cnt);

/**
 * Dealloc interval list
 * Dealloc all node in 'interval_list', dealloc data
 * Function is call if something get wrong and program need clean garbage in the middle of run
 * \param[in] iterval_list Pointer to first node in list
 * \return 0 if OK, 1 if data collector realloc fails
 */
int destroy_list(ipps_interval_node_t *interval_list);

#endif /* ip_prefix_search.h */
