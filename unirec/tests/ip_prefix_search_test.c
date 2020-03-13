/**
 * \file ip_prefix_search_test.c
 * \brief Test for IP prefich search data structure
 * \author Ondrej Ploteny <xplote01@stud.fit.vutbr.cz>
 * \date 2016
 */
/*
 * Copyright (C) 2015 CESNET
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
#include "../ip_prefix_search.h"
#include "../ipps_internal.h"

//void print_networks( ipps_network_list_t *networks)
//{
//    int index;
//    char string[INET6_ADDRSTRLEN];
//
//    for (index = 0; index < networks->net_count; ++index) {
//        ip_to_str(&networks->networks[index].addr, &string[0]);
//        printf("%d %-32s/%-3d\t%c\n", index, string, networks->networks[index].mask,
//               *(char *) networks->networks[index].data);
//    }
//    printf("\n\n");
//}

ipps_network_list_t *new_list(int use_net, char *ip_addr[], uint32_t *masks)
{
    ipps_network_list_t *new_list = malloc(sizeof(ipps_network_list_t));
    if (new_list == NULL) {
        fprintf(stderr, "ERROR alloc new list memory");
        return NULL;
    }

    new_list->net_count = use_net;
    new_list->networks = malloc(use_net * sizeof(ipps_network_t));
    ipps_network_t *network = new_list->networks;

    int i;
    for (i = 0; i < use_net; ++i) {
        network->data = malloc(sizeof(char));
        network->data_len = 1;
        ((char *) network->data)[0] = 'a' + (char) i;
        ip_from_str(ip_addr[i], &network->addr);
        network->mask = masks[i];
        network++;
    }
    return new_list;
}

void deinit_list(ipps_network_list_t *list)
{
    int i;
    for (i = 0; i < list->net_count; ++i) {
        free(list->networks[i].data);
    }
    free(list->networks);
    free(list);
}


void print_context(ipps_context_t *prefix_context)
{
    char ip_string[INET6_ADDRSTRLEN];
    int j, index;

    // Check print IPv4
    for (index = 0; index < prefix_context->v4_count; ++index) {
        ip_to_str(&prefix_context->v4_prefix_intervals[index].low_ip, &ip_string[0]);
        printf("\t%-16s", ip_string);
        ip_to_str(&prefix_context->v4_prefix_intervals[index].high_ip, &ip_string[0]);
        printf("\t%-15s", ip_string);
        printf("\t");
        for (j=0; j < prefix_context->v4_prefix_intervals[index].data_cnt; ++j) {
            printf("\t%c", *(char *) prefix_context->v4_prefix_intervals[index].data_array[j]);
        }
        printf("\n");
    }

    printf("------------------------------------------------------------\n");

    printf("\n-------------------------IPv6-------------------------------\n");
    printf("\t%-46s \t%-46s\t\t%s\n", "Low IP", "High IP", "Data");
    // Check print IPv6
    for (index = 0; index < prefix_context->v6_count; ++index) {
        ip_to_str(&prefix_context->v6_prefix_intervals[index].low_ip, &ip_string[0]);
        printf("\t%-46s", ip_string);
        ip_to_str(&prefix_context->v6_prefix_intervals[index].high_ip, &ip_string[0]);
        printf("\t%-46s", ip_string);
        printf("\t");
        for (j=0; j < prefix_context->v6_prefix_intervals[index].data_cnt; ++j) {
            printf("\t%c", *(char *) prefix_context->v6_prefix_intervals[index].data_array[j]);
        }
        printf("\n");
    }
    printf("------------------------------------------------------------\n\n");
}

int main(void)
{
    int index;
    int search_result;
    int cmp_result;

    ip_addr_t ip;
    ip_addr_t ip2;

    ipps_network_list_t *networks;
    ipps_context_t *prefix_context;
//    char string[INET6_ADDRSTRLEN];


    /**********************************************************************/
    char *ip_addr[] = { "192.168.2.255",
                        "1.2.3.0",
                        "192.168.1.1",
                        "200.200.200.210",
                        "f002:c316:69c3::ff:ffff",
                        "f001:c316:69c3::a:0",
                        "ff29:c316:69c3::a:a1",
                        "0001:ffff::a1",
                        "efff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
                        "f000:0000:0000:0000:0000:0000:0000:0000"

    };

    char *ip_addr_inc[] = { "192.168.3.0",
                            "1.2.3.1",
                            "192.168.1.2",
                            "200.200.200.211",
                            "f002:c316:69c3::100:0000",
                            "f001:c316:69c3::a:1",
                            "ff29:c316:69c3::a:a2",
                            "0001:ffff::a2",
                            "f000:0000:0000:0000:0000:0000:0000:0000",
                            "f000:0000:0000:0000:0000:0000:0000:0001"

    };

    char *ip_addr_dec[] = { "192.168.2.254",
                            "1.2.2.255",
                            "192.168.1.0",
                            "200.200.200.209",
                            "f002:c316:69c3::ff:fffe",
                            "f001:c316:69c3::9:ffff",
                            "ff29:c316:69c3::a:a0",
                            "0001:ffff::a0",
                            "efff:ffff:ffff:ffff:ffff:ffff:ffff:fffe",
                            "efff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"


    };

    printf("TEST 1 - IP increment\n");
    for (index = 0; index < 10; ++index) {
        if (index == 4) {
            printf("\tIPv4 increment function OK\n");
        }

        ip_from_str(ip_addr[index], &ip);
        ip_inc(&ip, &ip2);
        ip_from_str(ip_addr_inc[index], &ip);

        cmp_result = memcmp(&ip2, &ip, 16);
        if (cmp_result) {
            return 1;
        }
    }
    printf("\tIPv6 increment function OK\n");


    printf("TEST 2 - IP decrement\n");
    for (index = 0; index < 10; ++index) {
        if (index == 4) {
            printf("\tIPv4 decrement function OK\n");
        }

        ip_from_str(ip_addr[index], &ip);
        ip_dec(&ip, &ip2);
        ip_from_str(ip_addr_dec[index], &ip);

        cmp_result = memcmp(&ip2, &ip, 16);
        if (cmp_result) {
            return 1;
        }
    }
    printf("\tIPv6 decrement function OK\n");

    /**********************************************************************/
    char *ip_addr2[] = { "192.168.2.255",
                         "1.2.3.0",
                         "192.168.1.1",
                         "200.200.200.210",
                         "200.200.200.210",
                         "f002:c316:69c3::ff:ffff",
                         "f001:c316:69c3::a:0",
                         "ff29:c316:69c3::a:a1",
                         "0001:ffff::a1",
                         "0001:ffff::a1"

    };

    uint32_t masks[] = {1, 24, 25, 32, 31, 1, 64, 47, 128, 120};
    uint32_t masks_srt[] = {24, 25, 1, 31, 32, 120, 128, 64, 1, 47};
    char data_srt[] = {'b', 'c', 'a', 'e', 'd', 'j', 'i', 'g', 'f', 'h'};


    printf("TEST 3 - SORT\n");
    char *ip_addr_srt[] = {"1.2.3.0",
                           "192.168.1.1",
                           "192.168.2.255",
                           "200.200.200.210",
                           "200.200.200.210",
                           "0001:ffff::a1",
                           "0001:ffff::a1",
                           "f001:c316:69c3::a:0",
                           "f002:c316:69c3::ff:ffff",
                           "ff29:c316:69c3::a:a1",

    };

    networks = new_list(10, ip_addr2, masks);
    ipps_network_t **networks_v4 = malloc(5 * sizeof(ipps_network_t *));
    ipps_network_t **networks_v6 = malloc(5 * sizeof(ipps_network_t *));

    for (index = 0; index < 5; ++index) {
        networks_v4[index] = &networks->networks[index];
    }

    for (index = 0; index < 5; ++index) {
        networks_v6[index] = &networks->networks[index + 5];
    }

    qsort(networks_v4, 5, sizeof(ipps_network_t *), cmp_net_v4);

    for (index = 0; index < 5; ++index) {
        ip_from_str(ip_addr_srt[index], &ip);
        cmp_result = memcmp(networks_v4[index], &ip, 16);
        if (cmp_result) {
            return 1;
        } else {
            if (masks_srt[index] != networks_v4[index]->mask) {
                return 1;
            }
            if (data_srt[index] != *(char *) networks_v4[index]->data) {
                return 1;
            }
        }
    }
    printf("\tIPv4 sort function OK\n");

    qsort(networks_v6, 5, sizeof(ipps_network_t *), cmp_net_v6);

    for (index = 0; index < 5; ++index) {
        ip_from_str(ip_addr_srt[index+5], &ip);
        cmp_result = memcmp(networks_v6[index], &ip, 16);
        if (cmp_result) {
            return 1;
        } else {
            if(masks_srt[index+5] != networks_v6[index]->mask) {
                return 1;
            }
            if(data_srt[index+5] != *(char*) networks_v6[index]->data) {
                return 1;
            }
        }
    }
    printf("\tIPv6 sort function OK\n");
    free(networks_v4);
    free(networks_v6);
    deinit_list(networks);


    /**********************************************************************/
    /**********************************************************************/
    printf("TEST 4 - Create Interval\n");
    ipps_interval_node_t *inter_node;
    ip_from_str(ip_addr2[0], &ip);
    ip_from_str(ip_addr2[1], &ip2);

    inter_node = new_interval( &ip, &ip2);
    if (inter_node == NULL
        || inter_node->interval == NULL
        || inter_node->interval->data_array == NULL) {
        return 1;
    }
    cmp_result = memcmp(&inter_node->interval->low_ip, &ip, 16);
    if (cmp_result != 0) {
        return 1;
    }
    cmp_result = memcmp(&inter_node->interval->high_ip, &ip2, 16);
    if (cmp_result != 0) {
        return 1;
    }
    free(inter_node->interval->data_array);
    free(inter_node->interval);
    free(inter_node);
    printf("\tIPv4 create interval function OK\n");

    ip_from_str(ip_addr2[5], &ip);
    ip_from_str(ip_addr2[7], &ip2);

    inter_node = new_interval(&ip, &ip2);
    if (inter_node == NULL
        || inter_node->interval == NULL
        || inter_node->interval->data_array == NULL) {
        return 1;
    }
    cmp_result = memcmp(&inter_node->interval->low_ip, &ip, 16);
    if (cmp_result != 0) {
        return 1;
    }
    cmp_result = memcmp(&inter_node->interval->high_ip, &ip2, 16);
    if (cmp_result != 0) {
        return 1;
    }

    printf("\tIPv6 create interval function OK\n");
    /**********************************************************************/
    /**********************************************************************/

    printf("TEST 5 - add data\n");

    void *tmp_ptr = malloc(sizeof(char));
    if (tmp_ptr == NULL) {
        return 1;
    }

    *(char *)tmp_ptr = 'a';

    if (add_data(inter_node->interval, tmp_ptr, sizeof(char))) {
        destroy_list(inter_node);
        return  1;
    }
    free(tmp_ptr);

    tmp_ptr = malloc(2 * sizeof(char));
    ((char *)tmp_ptr)[0] = 'b';
    ((char *)tmp_ptr)[1] = 'c';

    if (add_data(inter_node->interval, tmp_ptr, 2 * sizeof(char))) {
        destroy_list(inter_node);
        return  1;
    }
    free(tmp_ptr);

    tmp_ptr = malloc(sizeof(int));
    *(int *)tmp_ptr = 42;
    if(add_data(inter_node->interval, tmp_ptr, sizeof(int))) {
        destroy_list(inter_node);
        return  1;
    }
    free(tmp_ptr);

    if(inter_node->interval->data_array == NULL
       || inter_node->interval->data_array[0] == NULL
       || inter_node->interval->data_array[1] == NULL
       || inter_node->interval->data_array[2] == NULL) {
        return 1;
    }

    if( *(char *)inter_node->interval->data_array[0] != 'a') {
        return 1;
    }
    if( ((char *) inter_node->interval->data_array[1])[0] != 'b') {
        return 1;
    }
    if( ((char *) inter_node->interval->data_array[1])[1] != 'c') {
        return 1;
    }
    if( *(int *)inter_node->interval->data_array[2] != 42) {
        return 1;
    }

    printf("\tAdd multiple data OK\n");

    /**********************************************************************/
    /**********************************************************************/
    printf("TEST 6 - insert interval\n");
    ip_from_str(ip_addr2[2], &ip);
    ip_from_str(ip_addr2[3], &ip2);

//    insert_new_interval(inter_node, &ip, &ip2);
    if (insert_new_interval(inter_node, &ip, &ip2) == NULL) {
        destroy_list(inter_node);
        return 1;
    }

    if (inter_node->next == NULL
        || inter_node->next->interval == NULL
        || inter_node->next->interval->data_array == NULL) {
        return 1;
    }

    cmp_result = memcmp(&inter_node->next->interval->low_ip, &ip, 16);
    if (cmp_result != 0) {
        return 1;
    }
    cmp_result = memcmp(&inter_node->next->interval->high_ip, &ip2, 16);
    if (cmp_result != 0) {
        return 1;
    }


    printf("\tPost insert OK\n");

    /**********************************************************************/
    /**********************************************************************/
    printf("TEST 7 - Copy data\n");

    for (index = 0; index < 4; ++index) {
        if (copy_all_data(inter_node->next->interval, inter_node->interval)) {
            destroy_list(inter_node);
            return 1;
        }
    }

     if (inter_node->next->interval->data_array == NULL) {
         return 1;
     }
    int offset = 0;
    for (index = 0; index < 4; ++index) {
        offset = 3*index;
        if (inter_node->next->interval->data_array[0 + offset] == NULL
            || inter_node->next->interval->data_array[1 + offset] == NULL
            || inter_node->next->interval->data_array[2 + offset] == NULL) {
            return 1;
        }

        if (*(char *)inter_node->next->interval->data_array[0+offset] != 'a') {
            return 1;
        }
        if (((char *) inter_node->next->interval->data_array[1+offset])[0] != 'b') {
            return 1;
        }
        if (((char *) inter_node->next->interval->data_array[1+offset])[1] != 'c') {
            return 1;
        }
        if (*(int *)inter_node->next->interval->data_array[2+offset] != 42) {
            return 1;
        }
    }

    if (inter_node->next->interval->data_cnt != 12) {
        return 1;
    }

    if(inter_node->next->interval->array_len <= DATASLOTS) {
        return 1;
    }

    destroy_list(inter_node);
    printf("\tCopy function (overflow array, data copy) OK\n");


    /**********************************************************************/
    /**********************************************************************/


    char *ip_addr3[] = { "1.2.3.4",
                        "192.168.1.1",
                        "192.168.2.255",
                        "200.200.200.200",
                        "0001:ffff::a1",
                        "f001:c316:69c3::a:a1",
                        "f002:c316:69c3::ff:ffff",
                        "ff29:c316:69c3::a:a1",

    };

    uint32_t masks3[] = {1, 24, 25, 32, 1, 64, 47, 128};

    networks = new_list(8, ip_addr3, masks3);

//    print_networks(networks);

    prefix_context = ipps_init(networks);
    deinit_list(networks);
    printf("TEST 8 - Context alloc\n");
    if (prefix_context == NULL) {
        return 1;
    }
    if (prefix_context->v4_count > 0 && prefix_context->v4_prefix_intervals == NULL) {
        return 1;
    }
    if (prefix_context->v6_count > 0 && prefix_context->v6_prefix_intervals == NULL) {
        return 1;
    }

    for (index = 0; index < prefix_context->v4_count; ++index) {
        if (prefix_context->v4_prefix_intervals[index].data_array[0] == NULL) {
            return 1;
        } else {
            if (*(char *)prefix_context->v4_prefix_intervals[index].data_array[0] != 'a' + index) {
                return 1;
            }
        }

    }

    for (index = 0; index < prefix_context->v6_count; ++index) {
        if(prefix_context->v6_prefix_intervals[index].data_array[0] == NULL) {
            return 1;
        } else {
            if (*(char *)prefix_context->v6_prefix_intervals[index].data_array[0] != 'a' + index + 4) {
                return 1;
            }
        }

    }

    printf("\tAlloc test OK\n");

    /**********************************************************************/
    /**********************************************************************/

    printf("TEST 9 - MASK IP\n");
    char *ip_addr_l[] = { "0.0.0.0",
                          "192.168.1.0",
                          "192.168.2.128",
                          "200.200.200.200",
                          "0000:0000:0000:0000:0000:0000:0000:0000",
                          "f001:c316:69c3:0000:0000:0000:0000:0000",
                          "f002:c316:69c2:0000:0000:0000:0000:0000",
                          "ff29:c316:69c3:0000:0000:0000:000a:00a1"

    };
    char *ip_addr_h[] = {"127.255.255.255",
                         "192.168.1.255",
                         "192.168.2.255",
                         "200.200.200.200",
                         "7fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
                         "f001:c316:69c3:0000:ffff:ffff:ffff:ffff",
                         "f002:c316:69c3:ffff:ffff:ffff:ffff:ffff",
                         "ff29:c316:69c3:0000:0000:0000:000a:00a1"
    };

    int result;
    int i;
    for (i = 0; i < 4; ++i) {
        ip_from_str(ip_addr_l[i], &ip);

        result = memcmp(&prefix_context->v4_prefix_intervals[i].low_ip, &ip, 16 );
        if (result) {
            return 1;
        }

    }
    printf("\tIPv4 masking OK\n");

    for (i=0; i < 4; ++i) {
        ip_from_str(ip_addr_l[4+i], &ip);

        result = memcmp(&prefix_context->v6_prefix_intervals[i].low_ip, &ip, 16 );
        if(result) {
            return 1;
        }
    }
    printf("\tIPv6 masking OK\n");
    /**********************************************************************/

    /**********************************************************************/

    printf("TEST 10 - BASIC INTERVALS\n");
    for (i = 0; i < 4; ++i) {
        ip_from_str(ip_addr_l[i], &ip);

        result = memcmp(&prefix_context->v4_prefix_intervals[i].low_ip, &ip, 16);
        if (result) {
            return 1;
        }

        ip_from_str(ip_addr_h[i], &ip);
        result = memcmp(&prefix_context->v4_prefix_intervals[i].high_ip, &ip, 16);
        if (result) {
            return 1;
        }
    }
    printf("\tIPv4 basic intervals OK\n");

    for (i = 0; i < 4; ++i) {
        ip_from_str(ip_addr_l[4+i], &ip);

        result = memcmp(&prefix_context->v6_prefix_intervals[i].low_ip, &ip, 16);
        if (result) {
            return 1;
        }

        ip_from_str(ip_addr_h[4+i], &ip);
        result = memcmp(&prefix_context->v6_prefix_intervals[i].high_ip, &ip, 16);
        if (result) {
            return 1;
        }
    }
    printf("\tIPv6 basic intervals OK\n");


//    print_context(prefix_context);



    /**********************************************************************/

    /**********************************************************************/
    printf("TEST 12 - BASIC INTERVALS search\n");
    char *match_search[] = {"0.0.0.0", "192.168.1.100", "192.168.2.255"};
    char *no_match_search[] = {"192.168.0.255", "192.168.3.0", "255.255.255.255"};
    char *match6_search[] = {"0::0", "f001:c316:69c3::a0:a0", "f002:c316:69c3:ffff:ffff:ffff:ffff:ffff"};
    char *no_match6_search[] = {"f001:c316:69c1::ffff", "f002:c316:69c4::", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff "};

    char **data;

    printf("\tIPv4 basic interval search\n");
    for (index = 0; index < 3; ++index) {


        ip_from_str(match_search[index], &ip);
        /* find ip address 'ip' in network prefix interval_search_context */
        search_result = ipps_search(&ip, prefix_context, (void ***)&data);

        if (search_result != 1) {
            return 1;
        }
        if (*(char*)prefix_context->v4_prefix_intervals[index].data_array[0] != data[0][0]) {
            return 1;
        }
        printf("\t\tmatch test %d OK\n", index);
    }

    for (index = 0; index < 3; ++index) {
        ip_from_str(no_match_search[index], &ip);
        /* find ip address 'ip' in network prefix interval_search_context */
        search_result = ipps_search(&ip, prefix_context, (void ***)&data);

        if (search_result != 0) {
            return 1;
        }

        printf("\t\tno match test %d OK\n", index);
    }

    printf("\tIPv6 basic interval search\n");
    for (index = 0; index < 3; ++index) {

        ip_from_str(match6_search[index], &ip);
        /* find ip address 'ip' in network prefix interval_search_context */
        search_result = ipps_search(&ip, prefix_context, (void ***)&data);

        if (search_result != 1) {
            return 1;
        }
        if (*(char*)prefix_context->v6_prefix_intervals[index].data_array[0] != data[0][0]) {
            return 1;
        }
        printf("\t\tmatch test %d OK\n", index);
    }

    for (index = 0; index < 3; ++index) {
        ip_from_str(no_match6_search[index], &ip);
        /* find ip address 'ip' in network prefix interval_search_context */
        search_result = ipps_search(&ip, prefix_context, (void ***)&data);

        if (search_result != 0) {
            return 1;
        }

        printf("\t\tno match test %d OK\n", index);
    }
    ipps_destroy(prefix_context);


    /**********************************************************************/

    /**********************************************************************/
    printf("TEST 13 - Overlaps Intervals\n");
    char *ip_addr4[]= {"192.168.1.5",
                       "192.168.1.2",
                       "192.168.1.130",
                       "192.168.1.7",
                       "192.168.1.250",
                       "192.168.2.0",
                       "192.168.1.150",
                       "192.255.255.255",
                       "0.0.0.0",
                       "255.255.255.255",
                       "0.0.0.0",
                       "10.10.10.10",
                        "ff37:3b22:507d:a4f9:0:0:a:a",
                        "fd37:3b22:507d:a4f9:0:0:1:1",
                        "fd37:3b22:507d:a4f9:7000:0:0:0",
                        "fd37:3b22:507d:a4f9::0:1:20",
                        "fd37:3b22:507d:a4f9:ffff:ffff:ff:0",
                        "fd37:3b22:507d:a500:0:0:0:0",
                        "fd37:3b22:507d:a4f9:ffff:fe00:abab:0",
                        "fd37:3b22:ffff:ffff:ffff:ffff:ffff:ffff",
                        "0::0",
                        "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
                        "0::0",
                        "10:10:10::10"
    };

    uint32_t masks4[] = {25, 24, 25, 26, 26, 24, 28, 32, 1, 2, 32, 32,
                         64, 64, 65, 67, 67, 64, 70, 128, 1, 2, 128, 128 };

    char *result_ip_addr4_l[] ={
            "0.0.0.0",
            "0.0.0.1",
            "10.10.10.10",
            "10.10.10.11",
            "192.0.0.0",
            "192.168.1.0",
            "192.168.1.64",
            "192.168.1.128",
            "192.168.1.144",
            "192.168.1.160",
            "192.168.1.192",
            "192.168.2.0",
            "192.168.3.0",
            "192.255.255.255",
            "193.0.0.0"
    };

    char *result_ip_addr4_h[] ={
            "0.0.0.0",
            "10.10.10.9",
            "10.10.10.10",
            "127.255.255.255",
            "192.168.0.255",
            "192.168.1.63",
            "192.168.1.127",
            "192.168.1.143",
            "192.168.1.159",
            "192.168.1.191",
            "192.168.1.255",
            "192.168.2.255",
            "192.255.255.254",
            "192.255.255.255",
            "255.255.255.255"
    };


    char *result_data_4[] = {"ik",
                       "i",
                       "il",
                       "i",
                       "j",
                       "jbad",
                       "jba",
                       "jbc",
                       "jbcg",
                       "jbc",
                       "jbce",
                       "jf",
                       "j",
                       "jh",
                       "j"
    };
    
    char * result_ip_addr6_l[] = {
                        "::",                                      
                        "::0001",                                  
                        "0010:0010:0010:0000:0000:0000:0000:0010", 
                        "0010:0010:0010:0000:0000:0000:0000:0011", 
                        "c000:0000:0000:0000:0000:0000:0000:0000", 
                        "fd37:3b22:507d:a4f9:0000:0000:0000:0000", 
                        "fd37:3b22:507d:a4f9:2000:0000:0000:0000", 
                        "fd37:3b22:507d:a4f9:8000:0000:0000:0000", 
                        "fd37:3b22:507d:a4f9:e000:0000:0000:0000", 
                        "fd37:3b22:507d:a4f9:fc00:0000:0000:0000", 
                        "fd37:3b22:507d:a4fa:0000:0000:0000:0000", 
                        "fd37:3b22:507d:a500:0000:0000:0000:0000", 
                        "fd37:3b22:507d:a501:0000:0000:0000:0000", 
                        "fd37:3b22:ffff:ffff:ffff:ffff:ffff:ffff", 
                        "fd37:3b23:0000:0000:0000:0000:0000:0000", 
                        "ff37:3b22:507d:a4f9:0000:0000:0000:0000", 
                        "ff37:3b22:507d:a4fa:0000:0000:0000:0000"
    };
    char * result_ip_addr6_h[] = {
                        "0000:0000:0000:0000:0000:0000:0000:0000",
                        "0010:0010:0010:0000:0000:0000:0000:000f",
                        "0010:0010:0010:0000:0000:0000:0000:0010",
                        "7fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
                        "fd37:3b22:507d:a4f8:ffff:ffff:ffff:ffff",
                        "fd37:3b22:507d:a4f9:1fff:ffff:ffff:ffff",
                        "fd37:3b22:507d:a4f9:7fff:ffff:ffff:ffff",
                        "fd37:3b22:507d:a4f9:dfff:ffff:ffff:ffff",
                        "fd37:3b22:507d:a4f9:fbff:ffff:ffff:ffff",
                        "fd37:3b22:507d:a4f9:ffff:ffff:ffff:ffff",
                        "fd37:3b22:507d:a4ff:ffff:ffff:ffff:ffff",
                        "fd37:3b22:507d:a500:ffff:ffff:ffff:ffff",
                        "fd37:3b22:ffff:ffff:ffff:ffff:ffff:fffe",
                        "fd37:3b22:ffff:ffff:ffff:ffff:ffff:ffff",
                        "ff37:3b22:507d:a4f8:ffff:ffff:ffff:ffff",
                        "ff37:3b22:507d:a4f9:ffff:ffff:ffff:ffff",
                        "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" };
    
    
                             
                             
    char *result_data_6[] = {
            "uw",
            "u",
            "ux",
            "u",
            "v",
            "vnop",
            "vno",
            "vn",
            "vnq",
            "vnqs",
            "v",
            "vr",
            "v",
            "vt",
            "v",
            "vm",
            "v"
    };


    networks = new_list(24, ip_addr4, masks4);

//    print_networks(networks);

    prefix_context = ipps_init(networks);
    deinit_list(networks);

//    print_context(prefix_context);

    if (prefix_context == NULL) {
        return 1;
    }

    if (prefix_context->v4_prefix_intervals == NULL) {
        return 1;
    }

    if (prefix_context->v4_count != 15) {
        return 1;
    }

    size_t size;

    for (index = 0; index < prefix_context->v4_count; ++index) {
        ip_from_str(result_ip_addr4_l[index], &ip);
        ip_from_str(result_ip_addr4_h[index], &ip2);
        size = strlen(result_data_4[index]);


        cmp_result = memcmp(&ip, &prefix_context->v4_prefix_intervals[index].low_ip, 16);
        if (cmp_result != 0) {
            return 1;
        }
        cmp_result = memcmp(&ip2, &prefix_context->v4_prefix_intervals[index].high_ip, 16);
        if (cmp_result != 0) {
            return 1;
        }
        if (size != prefix_context->v4_prefix_intervals[index].data_cnt) {
            return 1;
        }
        for (int j = 0; j < size; ++j) {
            if (((char *) prefix_context->v4_prefix_intervals[index].data_array[j])[0] != result_data_4[index][j]) {
                return 1;
            }
        }
    }
    printf("\tIPv4 intervals OK\n");

//    print_context(prefix_context);

    for (index = 0; index < prefix_context->v6_count; ++index) {
        ip_from_str(result_ip_addr6_l[index], &ip);
        ip_from_str(result_ip_addr6_h[index], &ip2);
        size = strlen(result_data_6[index]);

        cmp_result = memcmp(&ip, &prefix_context->v6_prefix_intervals[index].low_ip, 16);
        if (cmp_result != 0) {
            return 1;
        }
        cmp_result = memcmp(&ip2, &prefix_context->v6_prefix_intervals[index].high_ip, 16);
        if (cmp_result != 0) {
            return 1;
        }
        if (size != prefix_context->v6_prefix_intervals[index].data_cnt) {
            return 1;
        }
        for (int j = 0; j < size; ++j) {
            if (((char *) prefix_context->v6_prefix_intervals[index].data_array[j])[0] != result_data_6[index][j]) {
                return 1;
            }
        }
    }
    printf("\tIPv6 intervals OK\n");

    /**********************************************************************/

    /**********************************************************************/

    printf("TEST 14 - Overlaps Intervals search\n");

    char *match_search2[] = {"0.0.0.0", "192.168.1.100", "192.168.2.255"};
    char *no_match_search2[] = {"128.0.0.0", "172.168.3.0", "191.255.255.255"};
    char *match6_search2[] = {"0::0",
                              "fd37:3b22:507d:a4f9:e000::",
                              "fd37:3b22:507d:a500:ffff:ffff:ffff:ffff",
                              "ff37:3b22:ffff:a4f8:ffff:ffff:abab:abcd",
                              "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" };
    char *no_match6_search2[] = {"8001:0:0::0",
                                 "bfff:ffff:ffff:ffff:ffff:ffff:ffff:ffff",
                                 "8fff:ab:1da:ffff:ffff:00aa:ffff:0010"};

    int data_index4[]={0, 6, 11};
    int data_index6[]={0, 8, 11, 14, 16};

    printf("\tIPv4 overlaps interval search\n");
    for (index = 0; index < 3; ++index) {

        size = strlen(result_data_4[data_index4[index]]);
        ip_from_str(match_search2[index], &ip);
        /* find ip address 'ip' in network prefix interval_search_context */
        search_result = ipps_search(&ip, prefix_context, (void ***)&data);

        if (search_result != size) {
            return 1;
        }
        for (int j = 0; j < size; ++j) {
//            printf("%c", result_data_4[data_index4[index]][j]);
//            printf("%c", data[j][0]);

            if(result_data_4[data_index4[index]][j] != data[j][0]) {
                return 1;
            }
        }
        printf("\t\tmatch test %d OK\n", index);
    }

    for (index = 0; index < 3; ++index) {
        ip_from_str(no_match_search2[index], &ip);
        /* find ip address 'ip' in network prefix interval_search_context */
        search_result = ipps_search(&ip, prefix_context, (void ***) &data);

        if (search_result != 0) {
            return 1;
        }

        printf("\t\tno match test %d OK\n", index);
    }
    printf("\tIPv6 overlaps interval search\n");
    for (index = 0; index < 5; ++index) {

        size = strlen(result_data_6[data_index6[index]]);
        ip_from_str(match6_search2[index], &ip);
        /* find ip address 'ip' in network prefix interval_search_context */
        search_result = ipps_search(&ip, prefix_context, (void ***) &data);

        if (search_result != size) {
            return 1;
        }
        for (int j = 0; j < size; ++j) {
//            printf("%c", result_data_4[data_index4[index]][j]);
//            printf("%c", data[j][0]);

            if(result_data_6[data_index6[index]][j] != data[j][0]) {
                return 1;
            }
        }
        printf("\t\tmatch test %d OK\n", index);
    }

    for (index = 0; index < 3; ++index) {
        ip_from_str(no_match6_search2[index], &ip);
        /* find ip address 'ip' in network prefix interval_search_context */
        search_result = ipps_search(&ip, prefix_context, (void ***) &data);

        if (search_result != 0) {
            return 1;
        }

        printf("\t\tno match test %d OK\n", index);
    }
    ipps_destroy(prefix_context);
    return 0;
}
