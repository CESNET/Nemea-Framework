# IP prefix binary search

This structure is an ordered array data structure that is used to store a dynamic set,
where the keys are low and high IP addresses of prefix. Binary search compares low and high IP
with the searched IP address and returns data associated with match prefix.

The prefix array can be used for storing any data (string, int). For example, it can be used to
aggregate information from multiple blacklists. Data type is specific
to each user and their source format (source file, delimiters, data format or data length,...), so user
MUST define ```function load_networks()```, which load and parse information, and data about
networks from any of their source and fill ```'ipps_network_list'``` structure.

Before using the search, call the ```ipps_init()``` function. The function creates all
necessary structures for storing and accessing the data and return structure of IPv4 and
IPv6 array of prefixes with data (network context).

The parameters are:
* Structure with networks array and networks counter

For searching, use the function ```ipps_search()```. Function returns number of data associated with match prefix and return pointer to data as parameter:

* Structure of ip prefix context, ipps_context_t
* IP address union
* Void pointer to data array

For example, if blacklist contains:

```
    192.168.1.0/24   aaa
    192.168.1.0/25   bbb
    192.168.1.128/25 ccc
```

```init()``` creates 2 intervals:
* From 1.0 to 1.127 with data "aaa" and "bbb"
* From 1.128 to 1.255 with data "aaa" and "ccc":

```
 192.168.1.0     192.168.1.255
       ↓            ↓
       <-----aaa---->
       <-bbb-><-ccc->
```

and ```ip_prefix_search()``` is called with 192.168.1.100, search return number 2 and pointer to data "aaa" and "bbb".
For 192.168.1.200, return also number 2 but data are "aaa" and "ccc". For 192.1.1.1, search return 0 and pointer
to data is not fill.

For destruction of a whole structure and data there is ```ipps_destroy()``` function, parameter is pointer to
the ```ipps_context_t structure```, that has to be destroyed. Also, a list of networks is necessary to destroy with
function ```destroy_networks()``` (this function isn't a part of library and the user must define it).

Recommended control flow is:
1. ```load_networks()```
2. ```ipps_init()```
3. ```destroy_networks()```
4. ```ipps_search()```
5. ```ipps_destroy()```


******************************************************************

## Example file

```
/**
 * \file main.c
 * \brief Init and find prefix EXAMPLE
 * \author Erik Sabik <xsabik02@stud.fit.vutbr.cz>
 * \author Ondrej Ploteny <xplote01@stud.fit.vutbr.cz>
 * \date 2016-2020
 */
/*
 * Copyright (C) 2020 CESNET
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
/**
 * Prefix search example
 * Example of using ip_prefix library designed for binary searching IP address in multiple blacklists.
 * Blacklists information are save in a extern file.
 *
 * Function load_networks() read blacklist file by line and parse ip addresses, masks and data.
 * Parsed networks are stored in array in network_list_t struct. Function load_networks() is specific
 * for each user and his datasource. User must define function for fill ipps_network_list_t. In this
 * example is datasource in format:
 * <ip address>/<mask>,<char[4]>\n
 * 192.168.2.1/24,aaa\n
 *
 * Function ipps_init() make prefix context from loaded networks and preprocess networks for binary search in
 * multiple blacklists:
 *  - masked and sort source ip addresses
 *  - split ipv4 and ipv networks
 *  - create intervals (compute low and high ip of network), copy data
 *  - split overlapping intervals
 *  - sort intervals
 *  Intervals with its data are stored in array in ipps_context_t struct.
 *  Function ipps_init() return pointer to new ipps_context struct or NULL if init fails.
 *
 *
 * !!  If function load_networks() is call, you have to call destroy_networks() function !!
 * !!  If function ipps_init() is call, you have to call ipps_destroy() function !!
 *
 * For prefix searching, call function ipps_search() with searched ip address (ip_addr_t struct) and properly
 * initialized prefix context (ipps_context_t struct).
 * Function ipps_search() return number of match blacklists and return their data as parameter 'data',
 * if ipps_search() return 0, no match in blacklist and ip is not found in any blacklist.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <unirec/ip_prefix_search.h>


/* Length of loaded data in bytes */
#define DATALENGTH 4

/** Trim white spaces from string
 * Shift pointer to start of string `str` to point at first not whitespace character.
 * Find last character of string `str` that is not whitespace and write '\0' 1 byte after it.
 * @param[in] str String containing whitespaces.
 * @return Pointer to string containing no whitespaces.
 */
char *str_trim(char *str)
{
    char *end;

    //Trim leading space
    while (isspace(*str)) {
        str++;
    }

    // All spaces?
    if (*str == 0) {
        return str;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        end--;
    }

    // Write new null terminator
    *(end+1) = 0;

    return str;
}
/** Convert IP address from string to struct
 * Trim whitespaces from string `str` and check for network mask.
 * If network mask is not present in string `str` then convert IP address from string `str` to network struct `network`,
 * set network mask `network->mask` to 32 and for each octet from end of IP address `network->addr.ui32[2]`
 * that equals to 0 subtract 8 from network mask, if octet is not equal to 0 then end.
 * If network mask is present in string `str` then convert it to number and write it to `network->mask`
 * then remove it from string `str` and convert IP address from string `str` to network struct `network`.
 * @param[in] str String containing IP address and network mask.
 * @param[in] network Pointer to network structure.
 * return 1 on success otherwise 0.
 *
 */
int str_to_network(char *str, ipps_network_t *network)
{
    int i;
    char *pos;

    network->mask = 32;
    network->data_len = DATALENGTH;
    network->data = malloc(network->data_len * sizeof(char));
    if(network->data == NULL) {
        fprintf(stderr, "ERROR in allocating data");
        return 0;
    }
    memset(network->data, 0, network->data_len);

    if ((pos = strstr(str, ",")) != NULL)
    {
        memcpy(network->data, pos+1,3);
        *pos = 0;
    }

    // Trim whitespaces from string
    str = str_trim(str);

    // If mask not present, read ip and calculate mask from ip
    if ((pos = strstr(str, "/")) == NULL) {
        // Convert IPv4 from string, if fail return 0
        if (ip_from_str(str, &network->addr)) {
            // Calculate mask by counting zero octets from end of net addr
            for (i = 3; i >= 0; i--) {
                if ((network->addr.ui32[2] & (0xFF<<(i*8))) == 0 ) {
                    network->mask -= 8;
                } else {
                    break;
                }
            }
        } else {
            return 0;
        }
    } else { // Mask is present, read both ip and mask
        // Convert net mask from string to uint32
        network->mask = (uint32_t)atoi(pos + 1);

        // Remove net mask from string
        *pos = '\0';

        // Convert ip from string, if fail return 0
        if (!ip_from_str(str, &network->addr)) {
            return 0;
        }
    }

    return 1;
}


/** Extract network addresses from file
 * Allocate initial memory for networks list structure `ipps_network_list_t` and network structure
 * array `networks`.
 * Read file `file` line by line and save networks addresses in array of ipss_network structure `networks`.
 * If there are more networks than can fit in allocated memory then reallocate memory with 10 more spaces for networks.
 * source file format:
 * <ip_addr>/<mask>,<data>\n
 * etc.:
 * 192.168.2.0/24,aaa
 * @param[in] file Pointer to string containing file name.
 * @return Pointer to networks list structure if success else return NULL.
 */
ipps_network_list_t *load_networks(FILE *file)
{
    if(!file)
        return NULL;

    char line[64];
    uint32_t i=0;
    int struct_count = 50; // Starting v4_count of structs to alloc
    int line_n = 1;

    // ************* LOAD NETWORKS ********************** //

    // Alloc memory for networks structs, if malloc fails return NULL
    ipps_network_t *networks = malloc(struct_count * sizeof(ipps_network_t));
    if (networks == NULL) {
        fprintf(stderr, "ERROR allocating memory for network structures\n");
        return NULL;
    }

    // Alloc memory for networks list, if malloc fails return NULL
    ipps_network_list_t * networks_list = malloc(sizeof(ipps_network_list_t));
    if (networks_list == NULL) {
        fprintf(stderr, "ERROR allocating memory for network list\n");
        return NULL;
    }

    // Read file by line, convert string to struct
    while (fgets(line, 64, file) != NULL) {
        if (str_to_network(line, &networks[i]) == 1) {
            i++;

            // If limit is reached alloc new memory
            if (i >= struct_count) {
                struct_count += 10;
                // If realloc fails return NULL
                if ((networks = realloc(networks, struct_count * sizeof(ipps_network_t))) == NULL) {
                    fprintf(stderr, "ERROR in reallocating network structure\n");
                    return NULL;
                }
            }
        } else {
            // Not a valid line -> ignore it and print warning
            fprintf(stderr, "Warning: Invalid network on line %u\n", line_n);
        }
        line_n++;
    }

    networks_list->net_count = i;
    networks_list->networks = networks;

    return networks_list;
}



/** Dealloc ipps_network_list_t
 * Dealloc struct ipps_network_list_t, opposite of load_network
 * @param[in] network_list Pointer to network_list structure
 * @return void
 */
void destroy_networks(ipps_network_list_t *network_list) {
    int index;
    for (index = 0; index < network_list->net_count; index++) {
        free(network_list->networks[index].data);
    }

    free(network_list->networks);
    free(network_list);

}


int main()
{
    struct timeval tv1, tv2;

    /* Blacklist dataset file */
    FILE * dataset;

    dataset = fopen("blacklist6.txt", "r");
    if (dataset == NULL) {
        printf("blacklist6.txt not found\n");
        return 1;
    }

    /* Parse file line and create IPv4 and IPv6 network lists */
    ipps_network_list_t * network_list = load_networks(dataset);
    fclose(dataset);

    if (network_list->net_count == 0)
    {
        printf("Empty tree, nothing to do");
        return 0;
    }


    /* Context of networks => networks are transformed to intervals from low to high IP of network,
     * overlapping intervals are divided with relevant data, data are copied from 'ipps_network_list_t'
     * ipps_context_t store sorted array of IPv4 intervals and IPv6 intervals and their counters separately
     */
    ipps_context_t *prefix_context;

    prefix_context = ipps_init(network_list);


    if (prefix_context == NULL)
    {
        destroy_networks(network_list);
        return 1;
    }

    /* 'ipps_network_list_t' is no longer a need, data are copied in 'ipps_context_t' struct */
    destroy_networks(network_list);


    /* Print all ip intervals and data */
    printf("----------------------------IPv4----------------------------\n");
    int index = 0;
    int j = 0;
    char ip_string[INET6_ADDRSTRLEN];

    printf("\t%-16s \t%-16s\t%s\n", "Low IP", "High IP", "Data");

    /* Check print IPv4 */
    for (index = 0; index < prefix_context->v4_count; ++index)
    {
        ip_to_str(&prefix_context->v4_prefix_intervals[index].low_ip, &ip_string[0]);
        printf("\t%-16s", ip_string);
        ip_to_str(&prefix_context->v4_prefix_intervals[index].high_ip, &ip_string[0]);
        printf("\t%-15s", ip_string);
        printf("\t");
        for(j=0; j < prefix_context->v4_prefix_intervals[index].data_cnt; ++j)
        {
            printf("\t%s", (char *) prefix_context->v4_prefix_intervals[index].data_array[j]);
        }
        printf("\n");
    }

    printf("------------------------------------------------------------\n");

    printf("\n-------------------------IPv6-------------------------------\n");
    printf("\t%-46s \t%-46s\t\t%s\n", "Low IP", "High IP", "Data");
    /* Check print IPv6 */
    for(index = 0; index < prefix_context->v6_count; ++index)
    {
        ip_to_str(&prefix_context->v6_prefix_intervals[index].low_ip, &ip_string[0]);
        printf("\t%-46s", ip_string);
        ip_to_str(&prefix_context->v6_prefix_intervals[index].high_ip, &ip_string[0]);
        printf("\t%-46s", ip_string);
        printf("\t");
        for(j=0; j < prefix_context->v6_prefix_intervals[index].data_cnt; ++j)
        {
            printf("\t%s", (char *) prefix_context->v6_prefix_intervals[index].data_array[j]);
        }
        printf("\n");
    }
    printf("------------------------------------------------------------\n\n");




    /***************** Find some ip addresses in cycle ****************/
    char *ip_addr[] = {"251.205.178.136",
                       "255.255.186.96",
                       "0.0.0.1",
                       "fd57:9f25:3409::07",
                       "ff8d:2222:1111::44"
    };



    ip_addr_t ip;
    int search_result;      // Number of match prefixes, 0 if not match
    char ** data = NULL;    // data in blacklist is string

    gettimeofday(&tv1, NULL);
    for (index = 0; index < 5; ++index) {
        /* for each ip address in ip_addr array find in prefix interval_search_context */
        printf("%-16s\n", ip_addr[index]);

        ip_from_str(ip_addr[index], &ip);

        /* find ip address 'ip' in network prefix interval_search_context */
        search_result = ipps_search(&ip, prefix_context, (void ***) &data);

        if (search_result) {
            /* if there is any match prefix, print all data */
            for (j = 0; j < search_result; ++j) {
                printf("\t%d %s\n", j, data[j]);
            }
        }
        else {
            printf("\tIP not found in blacklist dataset\n");
        }
        printf("\n");
    }
    gettimeofday(&tv2, NULL);

    printf("search time: %ld usec", tv2.tv_usec - tv1.tv_usec);

    /* Dealloc prefix interval_search_context struct */
    ipps_destroy(prefix_context);


    return 0;
}
```
