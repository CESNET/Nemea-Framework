/**
 * \file ip_prefix_search.h
 * \brief Init context and prefix search
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

#ifndef IP_PREFIX_SEARCH_H
#define IP_PREFIX_SEARCH_H

#include "ipaddr.h"

/* Default size of data pointer array */
#define DATASLOTS 8

/* Default size of data collector */
#define COLLECTORSLOTS 16

/* Default size of ipv6 and ipv4 network collector */
#define NETWORKSLOTS 16

/**
 * Structure of network and assoc data.  Represent input information
 */
typedef struct network {
    ip_addr_t addr;                 ///< Network IP address
    uint32_t mask;                  ///< Network mask, CIDR notation, use for indexing
    void *data;                     ///< Pointer to same data
    size_t data_len;                ///< Number of bytes in 'data'
} ipps_network_t;

/**
 * Structure of all networks
 */
typedef struct network_list {
    uint32_t net_count;             ///< Number of networks in 'networks' array
    ipps_network_t *networks;       ///< Pointer to networks array
} ipps_network_list_t;

/**
 * Structure of IP interval.
 * IP interval is range of all IP addresses between low_ip and high_ip
 */
typedef struct {
    // Don't move offset is set
    ip_addr_t low_ip;               ///< Low IP of interval.
    ip_addr_t high_ip;              ///< High IP of interval.
    uint32_t data_cnt;              ///< Number of currently used data pointers in 'data_array'
    size_t array_len;               ///< Allocated size of 'data_array' => total available slots
    void **data_array;              ///< Array of pointers to data
}ipps_interval_t;

/**
 * Structure of sorted interval arrays.
 * Used for searching
 */
typedef struct {
    uint32_t v4_count;                        ///< Number of intervals in IPv4 array
    uint32_t v6_count;                        ///< Number of intervals in IPv6 array
    ipps_interval_t *v4_prefix_intervals;     ///< Pointer to IPv4 intervals array
    ipps_interval_t *v6_prefix_intervals;     ///< Pointer to IPv6 intervals array
}ipps_context_t;

/**
 * Initialize interval_search_context structure, fill IPv4 and IPv6 interval_search_context arrays
 * Function compute for all networks in 'network_list' appropriate intervals and copied network data
 * Overlapping intervals are split.  Array is sorted by low IP addr of interval.
 * Networks in network list is not necessary sorted, 'ipps_init' mask and sort each network itself
 * \param[in] network_list Pointer to network list structure
 * \return NULL if memory alloc fails, Pointer to interval_search_context structure
 */
ipps_context_t *ipps_init(ipps_network_list_t *networks);

/**
 * Deinitialize interval_search_context structure
 * Dealloc all memory, garbage collector
 * \param[in] prefix_context Pointer to interval_search_context struct
 * return 0 if deallocis OK, 1 if free fails
 */
int ipps_destroy(ipps_context_t *prefix_context);

/**
 * Binary search in interval_search_context
 * Binary search 'ip' in intervals interval_search_context, 'ip' is compare with 'low_ip' and
 * 'high_ip' of interval
 * if match, fill 'data' pointer by data_array of the interval and return number of used data slots
 * if no match return 0 and 'data' pointer not fill
 * Optimized for IPv4 or IPv6 search
 * \param[in] ip Pointer to ip address structure
 * \param[in] prefix_context Pointer to interval_search_context structure
 * \param[out] data Pointer to array of void pointers - pointers to data
 * \return int 0 if no match, >0 Number of data members in matched interval
 */
int ipps_search(ip_addr_t *ip, ipps_context_t *prefix_context, void ***data);

#endif /* ip_prefix_search.h */
