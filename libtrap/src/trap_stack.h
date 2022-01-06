/**
 * \file trap_stack.c
 * \brief TRAP stack
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

#ifndef TRAP_STACK_H_
#define TRAP_STACK_H_

#include <assert.h>
#include <stdint.h>

struct trap_stack_s {
    size_t top;
    size_t max_size;
    void **data;
};

static inline int
t_stack_init(struct trap_stack_s *t_stack, size_t size)
{
    t_stack->top = 0;
    t_stack->data = malloc(size * sizeof *t_stack->data);
    t_stack->max_size = size;
    if (t_stack->data == NULL)
        return 1;
    return 0;
}

static inline void
t_stack_destroy(struct trap_stack_s *t_stack)
{
    free(t_stack->data);
}

static inline bool 
t_stack_is_empty(struct trap_stack_s *t_stack)
{
    return !t_stack->top;
}

static inline bool 
t_stack_is_full(struct trap_stack_s *t_stack)
{
    return t_stack->top == t_stack->max_size;
}

static inline bool
t_stack_push(struct trap_stack_s *t_stack, void *value)
{
    if (!t_stack_is_full(t_stack)) {
        t_stack->data[t_stack->top] = value;
        t_stack->top++;
        return true;
    }
    
    return false;
}

static inline void *
t_stack_swap_and_pop(struct trap_stack_s *t_stack, size_t idx)
{
    assert(idx < t_stack->top);

    t_stack->top--;
    void *tmp = t_stack->data[t_stack->top];
    t_stack->data[t_stack->top] = t_stack->data[idx];
    t_stack->data[idx] = tmp;
    return t_stack->data[t_stack->top];
}

/**
 * @brief 
 * 
 * @warning You have to check if stack is not empty before calling this function
 *
 * @param t_stack 
 * @return uint8_t Popped value
 */
static inline void * 
t_stack_pop(struct trap_stack_s *t_stack)
{
    assert(t_stack->top);
    t_stack->top--;

    return t_stack->data[t_stack->top];
}

#endif /* TRAP_STACK_H_ */
