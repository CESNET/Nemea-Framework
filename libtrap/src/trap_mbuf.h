/**
 * \file trap_mbuf.h
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

#ifndef TRAP_MBUF_H
#define TRAP_MBUF_H

#include "trap_container.h"
#include "trap_stack.h"
#include "trap_ring_buffer.h"

struct trap_mbuf_s {
    // counter of finished containers
    uint64_t finished_containers;

    // counter of total processed messages
    uint64_t processed_messages;

    // array of allacated containers
    struct trap_container_s *containers;

    // pointer to currently active container
    struct trap_container_s *active;

    // ring buffer with containers to send
    struct trap_ring_buffer_s to_send;

    // empty containers
    struct trap_stack_s empty;

    // deffered containers
    struct trap_stack_s deferred;

    // 
    uint64_t lowest_cont_idx;

    // private
    size_t total_size;
};

/**
 * @brief Initialize mbuf structure.
 *
 * @param active_containers Maximal number of active containers at one time.
 * @param max_clients Maximal number of connected clients at one time.
 */
int 
t_mbuf_init(struct trap_mbuf_s *t_mbuf, size_t active_containers, size_t max_clients);

/**
 * @brief Deallocates memory needed by mbuf structure.
 */
void 
t_mbuf_clear(struct trap_mbuf_s *t_mbuf);

/**
 * @brief Get empty container.
 */
struct trap_container_s *
t_mbuf_get_empty_container(struct trap_mbuf_s *t_mbuf);

#endif /* TRAP_MBUF_H */
