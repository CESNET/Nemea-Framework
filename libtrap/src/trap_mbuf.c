/**
 * \file trap_mbuf.c
 * \brief TRAP mbuf
 * \author Pavel Siska <siska@cesnet.cz>
 * \date 2021
 */
/*
 * Copyright (C) 2021 CESNET
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

#include <string.h>

#include "trap_mbuf.h"
#include "ifc_tcpip.h"

int
t_mbuf_init(struct trap_mbuf_s *t_mbuf, size_t active_containers, size_t max_clients)
{
    memset(t_mbuf, 0, sizeof *t_mbuf);

    t_mbuf->total_size = active_containers + max_clients + 1;

    // allocate array of containers
    t_mbuf->containers = calloc(t_mbuf->total_size, sizeof(struct trap_container_s));
    if (t_mbuf->containers == NULL) {
        goto failure;
    }

    // initialize containers
    for (size_t i = 0; i < t_mbuf->total_size; i++) {
        if (t_cont_init(&t_mbuf->containers[i]) != 0) {
            goto failure;
        }
    }

    // initialize ring buffer
    if (t_rb_init(&t_mbuf->to_send, active_containers)) {
        goto failure;
    }

    if (t_stack_init(&t_mbuf->empty, t_mbuf->total_size)) {
        goto failure;
    }

    if (t_stack_init(&t_mbuf->deferred, t_mbuf->total_size)) {
        goto failure;
    }

    // push all containers to empty stack
    for (size_t i = 0; i < t_mbuf->total_size; i++) {
        t_stack_push(&t_mbuf->empty, &t_mbuf->containers[i]);
    }

    // set active container
    t_mbuf->active = t_stack_pop(&t_mbuf->empty);

    return 0;

failure:
    t_mbuf_clear(t_mbuf);
    return 1;
}


void 
t_mbuf_clear(struct trap_mbuf_s *t_mbuf)
{
    if (t_mbuf->containers) {
        for (size_t i = 0; i < t_mbuf->total_size; i++) {
            t_cont_destroy(&t_mbuf->containers[i]);
        }
    }
    free(t_mbuf->containers);
    t_rb_destroy(&t_mbuf->to_send);
    t_stack_destroy(&t_mbuf->empty);
    t_stack_destroy(&t_mbuf->deferred);
}


struct trap_container_s *
t_mbuf_get_empty_container(struct trap_mbuf_s *t_mbuf)
{
    if (!t_stack_is_empty(&t_mbuf->empty)) {
        t_mbuf->active = t_stack_pop(&t_mbuf->empty);
        return t_mbuf->active;
    }

again:
    for (size_t i = 0; i < t_mbuf->deferred.top; i++) {
        struct trap_container_s *t_cont;
        t_cont = t_mbuf->deferred.data[i];
        uint8_t ref_counter = __sync_add_and_fetch(&t_cont->ref_counter, 0);
        if (ref_counter == 0) {
            t_cont = t_stack_swap_and_pop(&t_mbuf->deferred, i);
            t_cont_clear(t_cont);
            t_stack_push(&t_mbuf->empty, t_cont);
            goto again;
        }
    }

    t_mbuf->active = t_stack_pop(&t_mbuf->empty);
    return t_mbuf->active;
}

static inline struct trap_container_s *
get_next_container(struct trap_mbuf_s *t_mbuf)
{
    if (!t_stack_is_empty(&t_mbuf->empty)) {
        t_mbuf->active = t_stack_pop(&t_mbuf->empty);
        return t_mbuf->active;
    }

  //  printf("STACK E: %u D: %u\n", t_mbuf->empty.top, t_mbuf->deferred.top);

again:
    for (size_t i = 0; i < t_mbuf->deferred.top; i++) {
        struct trap_container_s *t_cont;
        t_cont = t_mbuf->deferred.data[i];
      //  printf("D: %u %u\n", i, t_cont->ref_counter);
        uint8_t ref_counter = __sync_add_and_fetch(&t_cont->ref_counter, 0);
       // printf("DEF: %u %u\n", i, ref_counter);
        if (ref_counter == 0) {
            t_cont = t_stack_swap_and_pop(&t_mbuf->deferred, i);
            t_cont_clear(t_cont);
            t_stack_push(&t_mbuf->empty, t_cont);
            goto again;
        }
    }

  //  printf("STACK A E: %u D: %u\n", t_mbuf->empty.top, t_mbuf->deferred.top);
/*
    for (size_t i = 0; i < t_mbuf->deferred.top; i++) {
        struct trap_container_s *t_cont;
        t_cont = t_mbuf->deferred.data[i];
        uint8_t ref_counter = __sync_add_and_fetch(&t_cont->ref_counter, 0);
        printf("DEF1: %u %u\n", i, ref_counter);
    }

    if (t_stack_is_empty(&t_mbuf->empty)) {
        printf("STACK IS EMPTY %u %u %u\n", t_mbuf->empty.top, t_mbuf->deferred.top);
        usleep(1000);
        goto again;
    }
*/
    t_mbuf->active = t_stack_pop(&t_mbuf->empty);

    return t_mbuf->active;
}