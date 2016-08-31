/**
 * \file trap_buffer.c
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

#include "trap_buffer.h"

#ifdef UNIT_TESTING
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#endif
#include <stdint.h>
#include <inttypes.h>

/**
 * Change to 1 to enable debug printing.
 */
#define DEBUG_PRINTS 0

#if DEBUG_PRINTS
#include <stdio.h>
#define DBG_PRINT(...) printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

#define BLOCKSIZE_TOTAL(tb) (tb->blocksize + sizeof(tb_block_t))

static inline void _tb_block_clear(tb_block_t *bl)
{
   bl->write_data = (char *) bl->data->data;
   bl->read_data = bl->write_data;
   bl->data->size = 0;
}

static int _tb_block_init(tb_block_t *b)
{
   _tb_block_clear(b);
   b->refcount = 0;
   int res = pthread_mutex_init(&b->lock, NULL);
   if (res == 0) {
      return TB_SUCCESS;
   } else {
      return TB_ERROR;
   }
}

static int _tb_block_destroy(tb_block_t *b)
{
   if (pthread_mutex_destroy(&b->lock) == 0) {
      return TB_SUCCESS;
   } else {
      return TB_ERROR;
   }
}

static inline tb_block_t *tb_computenext_blockp(trap_buffer_t *tb, uint16_t curidx)
{
   uint16_t bi = (curidx + 1) % tb->nblocks;
   return (tb_block_t *) (tb->mem + (BLOCKSIZE_TOTAL(tb) * bi));
}

void tb_next_wr_block(trap_buffer_t *tb)
{
   uint16_t bi = (tb->cur_wr_block_idx + 1) % tb->nblocks;
   tb_block_t *bp = tb->blocks[bi];
   DBG_PRINT("move block from %p to idx %" PRIu32 " %p\n", tb->cur_wr_block, bi, bp);

   tb->cur_wr_block_idx = bi;
   tb->cur_wr_block = bp;
}

void tb_next_rd_block(trap_buffer_t *tb)
{
   uint16_t bi = (tb->cur_rd_block_idx + 1) % tb->nblocks;
   tb_block_t *bp = tb->blocks[bi];
   DBG_PRINT("move block from %p to idx %" PRIu32 " %p\n", tb->cur_rd_block, bi, bp);

   tb->cur_rd_block_idx = bi;
   tb->cur_rd_block = bp;
}

void tb_first_rd_block(trap_buffer_t *b)
{
   b->cur_rd_block = (tb_block_t *) b->mem;
   b->cur_rd_block_idx = 0;
   DBG_PRINT("move block %p\n", b->cur_rd_block);
}

void tb_first_wr_block(trap_buffer_t *b)
{
   b->cur_wr_block = (tb_block_t *) b->mem;
   b->cur_wr_block_idx = 0;
   DBG_PRINT("move block %p\n", b->cur_wr_block);
}

int tb_isblockfree(tb_block_t *b)
{
   if (b->data->size == 0) {
      return TB_SUCCESS;
   } else {
      return TB_FULL;
   }
}

trap_buffer_t *tb_init(uint16_t nblocks, uint32_t blocksize)
{
   uint16_t i = 0;
   trap_buffer_t *n = NULL;
   if (nblocks == 0 || blocksize == 0) {
      return NULL;
   }
   n = malloc(sizeof(trap_buffer_t));
   if (n == NULL) {
      return NULL;
   }

   n->blocksize = blocksize;
   n->nblocks = nblocks;
   n->mem = malloc(nblocks * BLOCKSIZE_TOTAL(n));
   if (n->mem == NULL) {
      free(n);
      return NULL;
   }

   n->blocks = malloc(nblocks * sizeof(tb_block_t *));
   if (n->blocks == NULL) {
      free(n->mem);
      free(n);
      return NULL;
   }

   if (pthread_mutex_init(&n->lock, NULL) != 0) {
      free(n->mem);
      free(n);
      return NULL;
   }
   tb_first_wr_block(n);
   tb_first_rd_block(n);

   tb_block_t *bl = n->cur_wr_block;
   for (i = 0; i < nblocks; i++) {
      _tb_block_init(bl);
      n->blocks[i] = bl;
      bl = tb_computenext_blockp(n, i);
   }

   return n;
}

void tb_destroy(trap_buffer_t **b)
{
   int i;
   if (b == NULL || (*b) == NULL) {
      return;
   }

   trap_buffer_t *p = (*b);

   tb_first_wr_block(p);
   for (i = 0; i < p->nblocks; i++) {
      _tb_block_destroy(p->cur_wr_block);
      tb_next_wr_block(p);
   }

   free(p->mem);
   free(p->blocks);
   free(p);
   (*b) = NULL;
}

int tb_lock(trap_buffer_t *b)
{
   return pthread_mutex_lock(&b->lock);
}

int tb_unlock(trap_buffer_t *b)
{
   return pthread_mutex_unlock(&b->lock);
}

int tb_block_lock(tb_block_t *bl)
{
   return pthread_mutex_lock(&bl->lock);
}

int tb_block_unlock(tb_block_t *bl)
{
   return pthread_mutex_unlock(&bl->lock);
}

static inline int _tb_pushmess_checksize(trap_buffer_t *b, tb_block_t **bl, uint32_t ts)
{
   uint32_t maxsize = (b->blocksize - sizeof(struct tb_block_data_s));
   tb_block_t *blp = *bl;

   if (((*bl)->data->size + ts) > maxsize) {
      DBG_PRINT("No memory for %" PRIu16 " msg, total size: %" PRIu32 ", hdr: %" PRIu32 "\n",
             size, ((*bl)->data->size + ts), (*bl)->data->size);
      if (ts > maxsize) {
         DBG_PRINT("Message is bigger than block.\n");
         return TB_ERROR;
      }

      /* move to the next block in locked time */
      tb_block_unlock(blp);
      tb_next_wr_block(b);
      blp = b->cur_wr_block;
      tb_block_lock(blp);
      (*bl) = blp;

      if (tb_isblockfree(*bl) == TB_FULL) {
         return TB_FULL;
      } else {
         DBG_PRINT("Moved to the next block.\n");
         return TB_USED_NEWBLOCK;
      }
   }
   return TB_SUCCESS;
}

int tb_pushmess(trap_buffer_t *b, const void *data, uint16_t size)
{
   int res = TB_SUCCESS;

   tb_lock(b);
   tb_block_t *bl = b->cur_wr_block;
   tb_block_lock(bl);
   uint32_t ts = size + sizeof(size);

   res = _tb_pushmess_checksize(b, &bl, ts);
   tb_unlock(b);

   if (res == TB_ERROR || res == TB_FULL) {
      goto exit;
   }

   uint16_t *msize = (uint16_t *) bl->write_data;
   char *p = (char *) (msize + 1);
   (*msize) = htons(size);
   memcpy(p, data, size);
   bl->data->size += size + sizeof(size);
   bl->write_data += size + sizeof(size);
   DBG_PRINT("Saved %" PRIu16 " B, total: %" PRIu32 " B\n", size, (bl->data->size + ts));

exit:
   tb_block_unlock(bl);
   return res;
}

int tb_pushmess2(trap_buffer_t *b, const void *d1, uint16_t s1, const void *d2, uint16_t s2)
{
   int res = TB_SUCCESS;
   uint32_t ts = s1 + s2 + sizeof(ts);

   tb_lock(b);
   tb_block_t *bl = b->cur_wr_block;
   tb_block_lock(bl);

   res = _tb_pushmess_checksize(b, &bl, ts);
   tb_unlock(b);

   if (res == TB_ERROR || res == TB_FULL) {
      goto exit;
   }

   uint16_t *msize = (uint16_t *) bl->write_data;
   char *p = (char *) (msize + 1);

   (*msize) = htons(ts);
   memcpy(p, d1, s1);
   p += s1;
   memcpy(p, d2, s2);
   bl->data->size += ts + sizeof(ts);
   bl->write_data += ts + sizeof(ts);
   DBG_PRINT("Saved 2-part-msg %" PRIu16 " B, total: %" PRIu32 " B\n", ts, (bl->data->size + ts));

exit:
   tb_block_unlock(bl);
   return res;
}

int tb_getmess(trap_buffer_t *b, const void **data, uint16_t *size)
{
   int res = TB_SUCCESS;
   tb_lock(b);
   tb_block_t *bl = b->cur_rd_block;
   tb_block_lock(bl);

   if (bl->read_data == bl->write_data) {
      DBG_PRINT("Not enough memory in %" PRIu32 " block for %" PRIu16 " message, total size: %" PRIu32 ", header: %" PRIu32 "\n",
             b->blocksize, size, (bl->data->size + size + sizeof(size)), bl->data->size);
      tb_block_unlock(bl);
      tb_next_rd_block(b);
      bl = b->cur_rd_block;
      tb_block_lock(bl);
      if (bl->read_data >= bl->write_data) {
         tb_block_unlock(bl);
         tb_unlock(b);
         return TB_EMPTY;
      } else {
         res = TB_USED_NEWBLOCK;
         DBG_PRINT("Moved to the next block.\n");
      }
   }
   tb_unlock(b);

   uint16_t *msize = (uint16_t *) bl->read_data;
   (*data) = (const void *) (msize + 1);
   (*size) = ntohs(*msize);
   bl->read_data += sizeof(uint16_t) + (*size);

   tb_block_unlock(bl);
   return res;
}

void tb_clear_unused(trap_buffer_t *tb)
{
   uint16_t i;
   tb_block_t *bl;
   tb_lock(tb);
   for (i = 0; i < tb->nblocks; i++) {
      bl = tb->blocks[i];
      tb_block_lock(bl);
      if (bl->refcount == 0) {
         _tb_block_clear(bl);
      }
      tb_block_unlock(bl);
   }
   tb_unlock(tb);
}

