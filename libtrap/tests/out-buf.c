
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <error.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <err.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "../include/libtrap/trap.h"
#include "../src/trap_internal.h"
#include "../src/trap_error.h"
#include "../src/trap_ifc.h"
#include "../src/trap_ifc_dummy.h"
#include "../src/trap_ifc_tcpip.h"
#include "../src/trap_ifc_shmem.h"
#include "../src/trap_ifc_tcpip_internal.h"



static int my_send_all_data(tcpip_sender_private_t *c, int sd, void **data, uint32_t *size, struct timeval *tm)
{
   char buffer[TRAP_IFC_MESSAGEQ_SIZE];
   void *p = (*data);
   ssize_t numbytes = (*size), sent_b;
   int retval;
   fd_set set, disset;
   struct timeval sectm;
   int res = TRAP_E_TERMINATED;

   while (c->is_terminated == 0) {
      sectm.tv_sec = 1;
      sectm.tv_usec = 0;
      FD_ZERO(&disset);
      FD_SET(sd, &disset);
      FD_ZERO(&set);
      FD_SET(sd, &set);

      retval = select(sd + 1, &disset, &set, NULL, (tm != NULL?tm:&sectm));
      if (retval == 0) {
         if (tm != NULL) {
            /* non-blocking mode */
            goto exit;
         }
      } else if (retval < 0) {
         if ((errno == EINTR) && (c->is_terminated != 0)) {
            goto failure;
         } else if (errno == EBADF) {
            res = TRAP_E_IO_ERROR;
            goto failure;
         }
      } else if (retval > 0) {
         if (FD_ISSET(sd, &disset)) {
            /* client disconnects */
            retval = recv(sd, buffer, TRAP_IFC_MESSAGEQ_SIZE, MSG_NOSIGNAL | MSG_DONTWAIT);
            if (retval < 1) {
               //VERBOSE(CL_VERBOSE_LIBRARY, "Disconnected client.");
               res = TRAP_E_IO_ERROR;
               goto failure;
            }
         }
         if (FD_ISSET(sd, &set)) {
            char nexttry = NONBLOCKING_ATTEMPTS;
            do {
               if (tm != NULL) {
                  sent_b = send(sd, p, numbytes, MSG_NOSIGNAL | MSG_DONTWAIT);
               } else {
                  sent_b = send(sd, p, numbytes, MSG_NOSIGNAL);
               }
               if (sent_b == -1) {
                  switch (errno) {
                  case EINTR:
                  case EBADF:
                  case EPIPE:
                  case EFAULT:
                     //VERBOSE(CL_VERBOSE_LIBRARY, "Disconnected client (%i)", errno);
                     res = TRAP_E_IO_ERROR;
                     goto failure;
                  case EAGAIN:
                     if (tm != NULL) {
                        /* blocking mode does not decrement attempts */
                        nexttry--;
                     }
                     usleep(NONBLOCKING_MINWAIT);
                     break;
                  }
                  if (c->is_terminated == 1) {
                     goto failure;
                  }
               } else {
                  numbytes -= sent_b;
                  p += sent_b;
                  //DEBUG_IFC(VERBOSE(CL_VERBOSE_LIBRARY, "send sent: %"PRId64" B remaining: "
                  //          "%"PRId64" B from %p (errno %"PRId32")",
                  //          sent_b, numbytes, p, errno));
               }
            } while ((nexttry > 0) && (numbytes > 0));
exit:
            (*size) = numbytes;
            if (numbytes > 0) {
               (*data) = p;
               return TRAP_E_TIMEOUT;
            } else {
               (*data) = NULL;
               return TRAP_E_OK;
            }
         }
      }
   }
failure:
   (*data) = NULL;
   (*size) = 0;
   return res;
}

static void add_into_buffer(trap_output_ifc_t *priv, const void *data, const uint16_t size)
{
   if (priv->buffer_index < (TRAP_IFC_MESSAGEQ_SIZE - 4)) {
      uint16_t *msize = (uint16_t *) &priv->buffer[priv->buffer_index];
      (*msize) = size;
      memcpy((void *) (msize + 1), data, size);
      priv->buffer_index += size + sizeof size;
   }
}

static inline int my_trap_store_into_buffer(trap_ctx_priv_t *ctx, unsigned int ifc, const void *data, uint16_t size, int timeout, char flush)
{
   /* timeout for blocking mode */
   struct timeval tm;
   /* timout for nonblocking mode */
   struct timespec tmnblk;
   /* pointer to timeout for select() */
   struct timeval *temptm;
   /* pointer to timeout for select() */
   struct timespec *temptmblk;
   trap_set_timeouts(timeout, &tm, &tmnblk);
   temptm = (((timeout==TRAP_WAIT) || (timeout==TRAP_HALFWAIT))?NULL:&tm);
   temptmblk = ((timeout==TRAP_WAIT)?NULL:&tmnblk);
   tcpip_sender_private_t *c = ctx->out_ifc_list[ifc].priv;
   struct client_s *cl;
   uint32_t i;

   /* Declaration of variables, we can have small buffer, initialization after checking the condition. */
   uint32_t freespace, needed_size = size + sizeof(size);
   uint16_t *msize;
   void *bp;
   int result;

   /* Can we put message at least into empty buffer? In the worst case, we could end up with SEGFAULT -> rather skip with error */
   if (needed_size > (TRAP_IFC_MESSAGEQ_SIZE-4)) {
      return trap_errorf(ctx, TRAP_E_MEMORY, "Buffer is too small for this message. Skipping...");
   }

   if (flush != 0) {
      /* Autoflush call, trying to lock section, maybe interface is waiting for clients -> rather skip than block the whole thread. */
      if (pthread_mutex_trylock(&ctx->out_ifc_list[ifc].ifc_mtx) != 0) {
         return TRAP_E_OK;
      }
   } else {
      /* Lock this section at first before sending whole buffer. */
      pthread_mutex_lock(&ctx->out_ifc_list[ifc].ifc_mtx);
   }
   /* initialization in locked section, otherwise autoflush can send buffer which has been already sent */
   if (ctx->out_ifc_list[ifc].buffer_index <= TRAP_IFC_MESSAGEQ_SIZE) {
      freespace = TRAP_IFC_MESSAGEQ_SIZE - ctx->out_ifc_list[ifc].buffer_index;
   } else {
      freespace = 0;
   }
   result = TRAP_E_TIMEOUT;

   /* Is this a autoflush call? If we have empty buffer, we do not send anything. */
   if (flush != 0) {
      if (ctx->out_ifc_list[ifc].buffer_index != 0) {
         for (i = 0; i < c->clients_arr_size; ++i) {
            cl = &c->clients[i];
            if ((cl->sd <= 0) || (cl->client_state == BACKUP_BUFFER)) {
               /* not connected client */
               continue;
            }

            if (cl->sending_pointer == NULL && cl->pending_bytes == 0) {
               tcpip_tdu_header_t *h = c->message_buffer;
               memcpy((h + 1), ctx->out_ifc_list[ifc].buffer, ctx->out_ifc_list[ifc].buffer_index);
               h->data_length = htonl(ctx->out_ifc_list[ifc].buffer_index);
               cl->sending_pointer = h;
               cl->pending_bytes = ctx->out_ifc_list[ifc].buffer_index + 4;
            }
            result = my_send_all_data(c, cl->sd, &cl->sending_pointer, &cl->pending_bytes, temptm);
            break; /* one client only - debug only */
         }
         if (result == TRAP_E_OK) {
            ctx->counter_send_buffer[ifc]++;
            ctx->out_ifc_list[ifc].buffer_index = 0;
            DEBUG_BUF(VERBOSE(CL_VERBOSE_LIBRARY, "Sending partial buffer invoked by autoflush timeout on iterface %d", ifc));
         } else {
            VERBOSE(CL_VERBOSE_LIBRARY, "Autoflush was not successful.");
         }
      }
      goto fn_exit;
   }
   /* we send buffer before timeout, no need to flush it */
   ctx->out_ifc_list[ifc].bufferflush = 1;

   if ((freespace >= needed_size) && (ctx->out_ifc_list[ifc].bufferswitch == 1) && (size != 1)) {
      /* we have enough space, buffering is enabled and size is not "flush" */

      add_into_buffer(&ctx->out_ifc_list[ifc], data, size);

      result = TRAP_E_OK;

   } else {
      /* not enough space */

      for (i = 0; i < c->clients_arr_size; ++i) {
         cl = &c->clients[i];
         if ((cl->sd <= 0) || (cl->client_state == BACKUP_BUFFER)) {
            /* not connected client */
            continue;
         }

         if (cl->sending_pointer == NULL && cl->pending_bytes == 0) {
            tcpip_tdu_header_t *h = c->message_buffer;
            memcpy((h + 1), ctx->out_ifc_list[ifc].buffer, ctx->out_ifc_list[ifc].buffer_index);
            h->data_length = htonl(ctx->out_ifc_list[ifc].buffer_index);
            cl->sending_pointer = h;
            cl->pending_bytes = ctx->out_ifc_list[ifc].buffer_index + 4;
         }
         result = my_send_all_data(c, cl->sd, &cl->sending_pointer, &cl->pending_bytes, temptm);
         break; /* one client only - debug only */
      }
      /* if the buffer was successfuly sent OR we have no client: */
      if (result == TRAP_E_OK || result == TRAP_E_IO_ERROR) {
         if (result == TRAP_E_OK) {
            ctx->counter_send_buffer[ifc]++;
         } else {
            /* we had no client but we can propagate either OK or TIMEOUT: */
            result = TRAP_E_TIMEOUT;
         }
         /* buffer will be cleaned */
         ctx->out_ifc_list[ifc].buffer_index = 0;
         /* buffer was successfuly send but we still have current message pending-unstored
          * it will be the first message in buffer */
         add_into_buffer(&ctx->out_ifc_list[ifc], data, size);
      }
   }
fn_exit:
   pthread_mutex_unlock(&ctx->out_ifc_list[ifc].ifc_mtx);
   return result;
}


int main(int argc, char **argv)
{
   uint8_t buffer[TRAP_IFC_MESSAGEQ_SIZE];
   memset(buffer, 0, TRAP_IFC_MESSAGEQ_SIZE);
   uint32_t size;
   void *bp = NULL;
   int sd;
   //struct sockaddr_un unix_addr;
   struct sockaddr_un unix_addr;
   struct addrinfo *ai, *p = NULL;
   trap_module_info_t module_info = { "abc", "", 0, 1, NULL };
   char *params = "8899";
   char **params_a = {&params};
   trap_ifc_spec_t ifc_spec = {"u", params_a};
   trap_ctx_t *ctx = trap_ctx_init(&module_info, ifc_spec);
   trap_ctx_priv_t *cp = ctx;
   if (ctx == NULL || trap_ctx_get_last_error(ctx) != TRAP_E_OK) {
      errx(trap_last_error, trap_last_error_msg);
   }

   uint32_t pending_bytes;
   uint32_t iter = 10000000;
   while (iter--) {
      /* generate new message */
      uint16_t messsize = ((double) random() / RAND_MAX) * UINT16_MAX;
      messsize %= 90;
      messsize += 88;
      my_trap_store_into_buffer(ctx, 0, buffer, messsize, TRAP_NO_WAIT, 0);
      cp->counter_send_message[0]++;
   }
   my_trap_store_into_buffer(ctx, 0, buffer, 1, TRAP_NO_WAIT, 1);
   printf("Stored messages: %"PRIu64"\nSent buffers: %"PRIu64"\n",
          cp->counter_send_message[0], cp->counter_send_buffer[0]);

  return 0;
}

