/**
 * \file trap_ring_buffer.c
 * \brief TRAP ring buffer
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

#ifndef TRAP_RING_BUFFER_H_
#define TRAP_RING_BUFFER_H_

struct trap_ring_buffer_s {
    void **buff;
    size_t size;

    uint64_t tail_;
    uint64_t head_;
};


static inline int
t_rb_init(struct trap_ring_buffer_s *t_rb, size_t size)
{
    t_rb->tail_ = 0;
    t_rb->head_ = 0;
    t_rb->size = size;

    t_rb->buff = calloc(1, size * sizeof *t_rb->buff);
    return t_rb->buff ? 0 : 1; 
}

static inline void
t_rb_destroy(struct trap_ring_buffer_s *t_rb)
{
    free(t_rb->buff);
}

/**
 * @brief Returns whether the ring buffer is full.
 *
 * @param t_rb Ring buffer
 *
 * @return true if the ring buffer is full, false otherwise.
 */
static inline bool
t_rb_is_full(struct trap_ring_buffer_s *t_rb)
{
    return t_rb->head_ - t_rb->tail_ == t_rb->size;
}

/**
 * @brief Write new element data to the head and return previous element on replace possition.
 *
 * @param t_rb Ring buffer
 * @param data New element
 *
 * @return Replaced element
 */
static inline void *
t_rb_get_old_write_new(struct trap_ring_buffer_s *t_rb, void *data)
{
    uint64_t head_moduled = t_rb->head_ % t_rb->size;
    
    void *old_data = t_rb->buff[head_moduled];
    t_rb->buff[head_moduled] = data;

    __sync_add_and_fetch(&t_rb->head_, 1);

    if (t_rb_is_full(t_rb)) {  
        __sync_add_and_fetch(&t_rb->tail_, 1);
    }
    return old_data;  
}


/**
 * @brief Returns id of latest element in ring buffer.
 *
 * @param t_rb Ring buffer
 */
static inline uint64_t
t_rb_tail_id(struct trap_ring_buffer_s *t_rb)
{
    return t_rb->tail_;
}



/**
 * @brief Returns whether the ring buffer is empty.
 *
 * @param t_rb Ring buffer
 *
 * @return true if the ring buffer is empty, false otherwise.
 */
static inline bool
t_rb_is_empty(struct trap_ring_buffer_s *t_rb)
{
    return t_rb->head_ == t_rb->tail_;
}

/**
 * Returns a pointer to the element at position n in the ring buffer.
 *
 * @param t_rb Ring buffer
 * @param n    Position of an element in the ring buffer.
 *
 * @return The element at the specified position in the ring buffer.
 */
static inline void *
t_rb_at(struct trap_ring_buffer_s *t_rb, uint64_t n)
{
    return t_rb->buff[n % t_rb->size];
}

#endif /* TRAP_RING_BUFFER_H_ */