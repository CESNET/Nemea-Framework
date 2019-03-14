/**
 * \file trap_error.h
 * \brief Error handling for TRAP.
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
 * \author Tomas Cejka <cejkato2@fit.cvut.cz>
 * \author Tomas Jansky <janskto1@fit.cvut.cz>
 * \date 2013 - 2018
 */
/*
 * Copyright (C) 2013 - 2018 CESNET
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
#ifndef _TRAP_ERROR_H_
#define _TRAP_ERROR_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "trap_internal.h"
#include "../include/libtrap/trap.h"

extern const char* default_err_msg[256]; // default messages

/** Set error with default message.
 *
 *  @param[in] ctx libtrap context
 *  @param[in] err_num Error number as defined in trap.h
 *  @return err_num
 */
static inline int trap_error(trap_ctx_priv_t *ctx, int err_num)
{
   if (ctx != NULL && ctx->trap_last_error != err_num) {
      pthread_mutex_lock(&ctx->error_mtx);
      ctx->trap_last_error = err_num;
      if (err_num >= 0 &&
            err_num < sizeof(default_err_msg)/sizeof(char*) &&
            default_err_msg[err_num] != 0) {
         ctx->trap_last_error_msg = default_err_msg[err_num];
      }
      else {
         snprintf(ctx->error_msg_buffer, MAX_ERROR_MSG_BUFF_SIZE, "Unknown error (%i).", err_num);
         ctx->trap_last_error_msg = ctx->error_msg_buffer;
      }
      pthread_mutex_unlock(&ctx->error_mtx);
   }

   return err_num;
}


/** Set error with custom message (printf-like formatting).
 *
 *  @param[in] ctx libtrap context
 *  @param[in] err_num Error number as defined in trap.h
 *  @param[in] msg Human-readable string describing error, supports printf formatting.
 *  @param[in] ... Additional parameters for printf-like formatting of msg.
 *  @return err_num
 */
static inline int trap_errorf(trap_ctx_priv_t *ctx, int err_num, const char *msg, ...)
{
   if (ctx != NULL) {
      pthread_mutex_lock(&ctx->error_mtx);
      ctx->trap_last_error = err_num;
      va_list args;
      va_start(args, msg);
      vsnprintf(ctx->error_msg_buffer, MAX_ERROR_MSG_BUFF_SIZE, msg, args);
      va_end(args);
      ctx->trap_last_error_msg = ctx->error_msg_buffer;
      pthread_mutex_unlock(&ctx->error_mtx);
   }

   return err_num;
}

/** Prepend given string before current ctx->trap_last_error_msg.
 *  This function is useful when a call of some function fails and you want
 *  to print a message about it but keep the original message about the error
 *  inside the function.
 *  Expected usage:
 *     return errorp("Call of myFunc failed: ");
 *
 *  @param[in] ctx libtrap context
 *  @param[in] msg String to prepend current message
 *  @param[in] ... Additional parameters for printf-like formatting of msg.
 *  @return Current value of ctx->trap_last_error
 */
static inline int trap_errorp(trap_ctx_priv_t *ctx, const char *msg, ...)
{
   int retval = TRAP_E_BAD_FPARAMS;
   if (ctx != NULL) {
      pthread_mutex_lock(&ctx->error_mtx);
      char tmp_str[MAX_ERROR_MSG_BUFF_SIZE];
      strcpy(tmp_str, ctx->trap_last_error_msg);
      va_list args;
      va_start(args, msg);
      int n = vsnprintf(ctx->error_msg_buffer, MAX_ERROR_MSG_BUFF_SIZE, msg, args);
      va_end(args);
      if (n >= 0 && n < MAX_ERROR_MSG_BUFF_SIZE) {
         snprintf(ctx->error_msg_buffer + n, MAX_ERROR_MSG_BUFF_SIZE - n, tmp_str, args);
      }
      ctx->trap_last_error_msg = ctx->error_msg_buffer;
      retval = ctx->trap_last_error;
      pthread_mutex_unlock(&ctx->error_mtx);
   }

   return retval;
}

#endif
