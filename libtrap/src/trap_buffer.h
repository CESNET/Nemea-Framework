/**
 * \file trap_buffer.h
 * \brief TRAP ring buffer data structure
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2016
 */
/*
 * Copyright (C) 2016 CESNET
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

#ifndef TRAP_BUFFER_H
#define TRAP_BUFFER_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define TB_SUCCESS	0
#define TB_ERROR	1
#define TB_FULL         2
#define TB_USED_NEWBLOCK      3
#define TB_EMPTY        4


struct tb_block_data_s {
   /**
    * Size of stored data, it must be always <= (blocksize - sizeof(size)).
    *
    * If the size is 0, the block is free.
    */
   uint32_t size;
   /**
    * Pointer to the beginning of data stored in this block
    */
   char data[0];
};

typedef struct tb_block_s {
   /**
    * Pointer to the space for adding new data (to the header of message)
    */
   char *write_data;

   /**
    * Pointer to the space for adding new data (to the header of message)
    */
   char *read_data;

   /**
    * Reference counter with non-zero value means that the block is used and cannot be freed.
    */
   uint16_t refcount;

   /**
    * Lock the block
    */
   pthread_mutex_t lock;

   /**
    * RAW size without changing byte-order
    */
   uint32_t rsize;

   /**
    * Pointer to data in the block (to the header of the first message)
    */
   struct tb_block_data_s data[0];
} tb_block_t;

typedef struct trap_buffer_s {
   /**
    * Pointer to internal memory containing the whole ring buffer.
    */
   char *mem;

   /**
    * Pointer to current block
    */
   tb_block_t *cur_wr_block;

   /**
    * Array of pointers to blocks.
    */
   tb_block_t **blocks;
   /**
    * Index of current block to spare computation when moving to next block
    */
   uint16_t cur_wr_block_idx;

   /**
    * Pointer to current block
    */
   tb_block_t *cur_rd_block;

   /**
    * Index of current block to spare computation when moving to next block
    */
   uint16_t cur_rd_block_idx;

   /**
    * Maximal size of tb_block_data_s element (containing data + 32b header).
    */
   uint32_t blocksize;

   /**
    * Number of blocks in the buffer.
    */
   uint16_t nblocks;

   /**
    * Number of blocks in the buffer.
    */
   uint16_t freeblocks;

   /**
    * Lock the buffer
    */
   pthread_mutex_t lock;

   /**
    * Lock the isbusy condvar
    */
   pthread_mutex_t isbusy_lock;

   pthread_cond_t isbusy;
} trap_buffer_t;

/**
 * \defgroup trap_buffer_general General API
 * @{
 */

/**
 * Create a new buffer that will work wit nblocks of block_size.
 * \param[in] nblocks   Number of blocks that will be stored in the ring buffer.
 * \param[in] blocksize Maximal size of each block.
 * \return Pointer to the buffer struct, NULL on error.
 */
trap_buffer_t *tb_init(uint16_t nblocks, uint32_t blocksize);

/**
 * Free memory and set the pointer to NULL.
 * \param[in] Pointer to buffer.
 */
void tb_destroy(trap_buffer_t **tb);

/**
 * Lock buffer before manipulation.
 */
int tb_lock(trap_buffer_t *tb);

/**
 * Unlock buffer after manipulation.
 */
int tb_unlock(trap_buffer_t *tb);

/**
 * Lock block before manipulation.
 */
int tb_block_lock(tb_block_t *bl);

/**
 * Unlock block after manipulation.
 */
int tb_block_unlock(tb_block_t *bl);

/**
 * Check if the current block is free.
 *
 * \param[in] bl  Pointer to the block.
 * \return TB_SUCCESS when the current block is free, TB_FULL if it contains at least one message.
 */
int tb_isblockfree(tb_block_t *bl);

/**
 * @}
 */

/**
 * \defgroup trap_buffer_send    Output IFC
 * A set of functions used for output IFC (module sends messages).
 * @{
 */
/**
 * Push message into the current block
 * \param[in] tb     Pointer to the buffer.
 * \param[in] data   Pointer to the message to send.
 * \param[in] size   Size of the message to send.
 * \return TB_SUCCESS or TB_USED_NEWBLOCK when the message was stored
 *         successfuly, TB_FULL when there is no place for the message.
 *         TB_USED_NEWBLOCK indicates that the current block was changed.
 *         TB_ERROR means that size is bigger than block size.
 */
int tb_pushmess(trap_buffer_t *tb, const void *data, uint16_t size);

/**
 * Push message (consisting of d1 and d2 parts) into the current block
 * \param[in] tb   Pointer to the buffer.
 * \param[in] d1   Pointer to the 1st part of the message to send.
 * \param[in] s1   Size of the 1st part of the message to send.
 * \param[in] d2   Pointer to the 2nd part of the message to send.
 * \param[in] s2   Size of the 2nd part of the message to send.
 * \return TB_SUCCESS or TB_USED_NEWBLOCK when the message was stored
 *         successfuly, TB_FULL when there is no place for the message.
 *         TB_USED_NEWBLOCK indicates that the current block was changed.
 *         TB_ERROR means that size is bigger than block size.
 */
int tb_pushmess2(trap_buffer_t *tb, const void *d1, uint16_t s1, const void *d2, uint16_t s2);

void tb_wait_for_push(trap_buffer_t *tb);

/**
 * Go through all blocks and those which are not used (refcount) mark as free.
 *
 * \param[in] tb  Pointer to the buffer.
 */
void tb_clear_unused(trap_buffer_t *tb);

/**
 * Move to the next block for writing.
 *
 * This function moves cur_block pointer to the next block (it overflows after nblocks).
 * \return TB_FULL if the next block is full, cur_wr_block was not moved,
 *  TB_SUCCESS if the next block is empty, the cur_wr_block was moved.
 */
int tb_next_wr_block(trap_buffer_t *tb);

/**
 * Move to the first block for writing.
 *
 * This function moves cur_block pointer to the first block.
 */
void tb_first_wr_block(trap_buffer_t *tb);

/**
 * Lock the current free block for getting its content.
 *
 * After this code, it is possible to read size and data from bl.
 * See TB_FLUSH_START() for unlocking the block.
 *
 * Pseudocode:
 * trap_buffer_t *b = tb_init(10, 100000);
 * tb_block_t *bl;
 * TB_FLUSH_START(b, &bl, res);
 * if (res == TB_SUCCESS) {
 *    s = send(...);
 *    TB_FLUSH_END(b, bl, 0);
 * }
 *
 * \param[in] rdb  Pointer to the buffer.
 * \param[out] bl  Pointer to block (tb_block_t **bl).
 * \param[out] res  Result of TB_FLUSH_START(), it is set to TB_EMPTY (no data to send) or TB_FULL (we can continue sending).
 */
#define TB_FLUSH_START(rdb, bl, res) do { \
      tb_lock(rdb); \
      (*bl) = rdb->cur_rd_block; \
      tb_block_lock(*bl); \
      if (tb_iswr_ready(rdb) == TB_SUCCESS) { \
         res = TB_FULL; \
      } else { \
         /* current block is not free, we must unlock and wait */ \
         tb_block_unlock(*bl); \
         res = TB_EMPTY; \
      } \
      tb_unlock(rdb); \
   } while (0)

/**
 * Unlock the current free block after writing its content.
 *
 * It MUST NOT be called when TB_FLUSH_START() returned TB_FULL.
 *
 * \param[in] rdb  Pointer to the buffer.
 * \param[in] bl  Pointer to block (tb_block_t *bl).
 * \param[in] ref   Reference count, if 0, mark the block as free.
 */
#define TB_FLUSH_END(rdb, bl, ref) do { \
      bl->refcount = ref; \
      tb_block_unlock(bl); \
      tb_unlock(rdb); \
      tb_lock(rdb); \
      tb_block_lock(bl); \
      if (bl->refcount == 0 && bl->rsize != 0) { \
         /* block can be marked as empty for next pushmess() */ \
         bl->rsize = 0; \
         bl->data->size = 0; \
         /* we have processed a block -> it is free now */ \
         tb_next_rd_block(rdb); \
         tb_busy_lock(rdb); \
         rdb->freeblocks++; \
         pthread_cond_signal(&rdb->isbusy); \
         tb_busy_unlock(rdb); \
      } \
      tb_block_unlock(bl); \
      tb_unlock(rdb); \
   } while (0)

/**
 * @}
 */

/**
 * \defgroup trap_buffer_recv    Input IFC
 * A set of functions used for input IFC (module receives messages).
 * @{
 */

/**
 * Get message from buffer.
 * \param[in] tb   Pointer to the buffer.
 * \param[out] data Pointer to read message.
 * \param[out] size Size of read message.
 * \return TB_SUCCESS or TB_USED_NEWBLOCK when the message was read
 *         successfuly, TB_EMPTY when there is no message in the buffer to read.
 *         TB_USED_NEWBLOCK indicates that the current block was changed
 */
int tb_getmess(trap_buffer_t *tb, const void **data, uint16_t *size);

/**
 * Move to the first block for reading.
 *
 * This function moves cur_block pointer to the first block.
 * \param[in] tb   Pointer to the buffer.
 */
void tb_first_rd_block(trap_buffer_t *tb);


/**
 * Move to the next block for reading.
 *
 * This function moves cur_block pointer to the next block (it overflows after nblocks).
 * \param[in] tb   Pointer to the buffer.
 */
int tb_next_rd_block(trap_buffer_t *tb);

int tb_iswr_ready(trap_buffer_t *tb);

/**
 * Lock the current free block for writing its content.
 *
 * After this code, it is possible to write size and data into bl.
 * See TB_FILL_END() for unlocking the block.
 *
 * Pseudocode:
 * trap_buffer_t *b = tb_init(10, 100000);
 * tb_block_t *bl;
 * TB_FILL_START(b, &bl, res);
 * if (res == TB_SUCCESS) {
 *    s = recv(...);
 *    TB_FILL_END(b, bl, s);
 * }
 *
 * \param[in] rdb  Pointer to the buffer.
 * \param[out] bl  Pointer to block (tb_block_t **bl).
 * \param[out] res  Result of TB_FILL_START(), it is set to TB_SUCCESS or TB_FULL.
 */
#define TB_FILL_START(rdb, bl, res) do { \
      tb_lock(rdb); \
      DBG_PRINT("cur_wr_block_idx: %i\n", rdb->cur_wr_block_idx); \
      (*bl) = rdb->cur_wr_block; \
      tb_block_lock(*bl); \
      if (tb_isblockfree(*bl) == TB_SUCCESS) { \
         (res) = TB_SUCCESS; \
      } else { \
         /* current block is not free, we must unlock and wait */ \
         tb_block_unlock(*bl); \
         (res) = tb_next_wr_block(rdb); \
         if (res == TB_SUCCESS) { \
            (*bl) = rdb->cur_wr_block; \
            tb_block_lock(*bl); \
         } \
      } \
      tb_unlock(rdb); \
   } while (0)

/**
 * Unlock the current free block after writing its content.
 *
 * It MUST NOT be called when TB_FILL_START() returned TB_FULL.
 *
 * \param[in] rdb  Pointer to the buffer.
 * \param[in] bl  Pointer to block (tb_block_t *bl).
 * \param[in] s   Size of data written into the block. This will be set into header.
 */
#define TB_FILL_END(rdb, bl, s) do { \
      bl->rsize = s; \
      bl->data->size = htonl(s); \
      bl->write_data += s; \
      bl->read_data = bl->data->data; \
      tb_block_unlock(bl); \
   } while (0)

/**
 * @}
 */

#endif

