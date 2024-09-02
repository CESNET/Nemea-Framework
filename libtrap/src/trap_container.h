/**
 * \file trap_container.h
 * \brief TRAP container
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

#ifndef TRAP_CONTAINER_H_
#define TRAP_CONTAINER_H_

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "trap_error.h"

// default container buffer length
#define CONTAINER_DEFAULT_LEN 100000

#define TRAP_HEADER_SIZE 14

static size_t container_buff_len = CONTAINER_DEFAULT_LEN;

struct trap_container_s {
    uint8_t ref_counter __attribute__((aligned(64))); // number of clients referencing this container
    uint64_t seq_num; // initial sequence number
    uint64_t idx; // TODO comment
    size_t size; // number of inserted elements
    size_t used_bytes; // number of used bytes in buffer
    char* buffer;
};

static inline void
t_cont_set_len(size_t len)
{
    container_buff_len = len;
}

/**
 * @brief Check if buffer in container has a sufficient capacity.
 *
 * If container length is not initialized return false.
 *
 * @param capacity Required capacity.
 */
static inline bool
t_cont_has_capacity(size_t capacity)
{
    return capacity <= (container_buff_len - TRAP_HEADER_SIZE);
}

/**
 * @brief Clear the container to the default state.
 *
 * @param t_cont Container
 */
static inline void
t_cont_clear(struct trap_container_s* t_cont)
{
    t_cont->ref_counter = 1;
    t_cont->seq_num = 0;
    t_cont->idx = 0;
    t_cont->size = 0;
    t_cont->used_bytes = TRAP_HEADER_SIZE;
}

/**
 * @brief Initialize and set container to the default state container .
 *
 * @param t_cont Container
 *
 * @return 0 success
 * @return 1 error
 */
static inline int
t_cont_init(struct trap_container_s* t_cont)
{
    t_cont_clear(t_cont);
    t_cont->buffer = malloc(container_buff_len * sizeof t_cont->buffer);
    return t_cont->buffer ? 0 : 1;
}

/**
 * @brief Destriy container.
 *
 * @param t_cont Container
 */
static inline void
t_cont_destroy(struct trap_container_s* t_cont)
{
    free(t_cont->buffer);
}

/**
 * @brief Increment reference counter by 1 and acquiere container.
 *
 * @param t_cont Container
 *
 * @return Reference counter value before acquiring.
 */
static inline uint8_t
t_cont_acquiere(struct trap_container_s* t_cont)
{
    return __sync_fetch_and_add(&t_cont->ref_counter, 1);
}

/**
 * @brief Decrement reference counter by 1 and release container.
 *
 * @param t_cont Container
 *
 * @return Reference counter value after releasing.
 */
static inline uint8_t
t_cont_release(struct trap_container_s* t_cont)
{
    return __sync_sub_and_fetch(&t_cont->ref_counter, 1);
}

/**
 * @brief Insert new element into container.
 *
 * @param t_cont Container
 * @param data   Data to insert
 * @param size   Size of data
 */
static inline void
t_cont_insert(struct trap_container_s* t_cont, const char* data, size_t size)
{
    uint16_t size_16b = size;

    uint16_t* ptr = (uint16_t*)&t_cont->buffer[t_cont->used_bytes];
    *ptr = htons(size_16b);
    memcpy((void*)(ptr + 1), data, size_16b);

    t_cont->used_bytes += (size_16b + sizeof(size_16b));
    t_cont->size++;
}

/**
 * @brief Check if current container has enough space to insert element with size @p size.
 *
 * @param t_cont Container
 * @param size   Size of element
 * @return true  Size is sufficent
 * @return false Size is unsufficent
 */
static inline bool
t_cont_has_space(struct trap_container_s* t_cont, size_t size)
{
    return container_buff_len - t_cont->used_bytes >= size;
}

/**
 * @brief Set sequence number of container.
 *
 * @param t_cont Container
 * @param value  Sequence number.
 */
static inline void
t_cont_set_seq_num(struct trap_container_s* t_cont, uint64_t value)
{
    t_cont->seq_num = value;
}

/**
 * @brief Write trap header to the buffer.
 *
 * @param t_cont Container
 */
static inline void
t_cont_write_header(struct trap_container_s* t_cont, uint64_t idx)
{
    uint32_t* buffer = (uint32_t*)t_cont->buffer;
    uint64_t* seq = (uint64_t*)&t_cont->buffer[4];
    uint16_t* size = (uint16_t*)&t_cont->buffer[12];
    uint32_t header = htonl(t_cont->used_bytes - TRAP_HEADER_SIZE);
    t_cont->idx = idx;

    *buffer = header;
    *seq = t_cont->seq_num;
    *size = t_cont->size;
}

#endif /* TRAP_CONTAINER_H_ */