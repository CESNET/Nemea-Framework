
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

#define TRAP_IFC_MESSAGEQ_SIZE 100000
#define DEFAULT_MAX_DATA_LENGTH 100000
#define TRAP_E_OK 0
#define TRAP_E_TIMEOUT 1
#define TRAP_E_IO_ERROR 2
#define TRAP_E_TERMINATED 3
#define NONBLOCKING_ATTEMPTS 0
#define NONBLOCKING_MINWAIT 100
typedef struct tcpip_sender_private {
char is_terminated;
} tcpip_sender_private_t;


static int send_all_data(tcpip_sender_private_t *c, int sd, void **data, uint32_t *size, struct timeval *tm)
{
   char buffer[DEFAULT_MAX_DATA_LENGTH];
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
            retval = recv(sd, buffer, DEFAULT_MAX_DATA_LENGTH, MSG_NOSIGNAL | MSG_DONTWAIT);
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

int main(int argc, char **argv)
{
   uint8_t buffer[TRAP_IFC_MESSAGEQ_SIZE];
   uint32_t size;
   void *bp = NULL;
   int sd;
   //struct sockaddr_un unix_addr;
   struct sockaddr_un unix_addr;
   struct addrinfo *ai, *p = NULL;

   tcpip_sender_private_t c = {0};

   memset(&unix_addr, 0, sizeof(unix_addr));
   unix_addr.sun_family = AF_UNIX;
   snprintf(unix_addr.sun_path, sizeof(unix_addr.sun_path) - 1, "/tmp/trap-localhost-8899.sock");
   /* if socket file exists, it could be hard to create new socket and bind */
   unlink(unix_addr.sun_path); /* error when file does not exist is not a problem */
   sd = socket(AF_UNIX, SOCK_STREAM, 0);
   if (bind(sd, (struct sockaddr *) &unix_addr, sizeof(unix_addr)) == -1) {
      err(errno, "bind failed.");
   }
   if (listen(sd, 10) == -1) {
      err(errno, "listen failed.");
   }

   int csd = accept(sd, NULL, NULL);
   uint32_t pending_bytes;
   uint32_t iter = 100000;
   while (1) {
      if (bp == NULL) {
         if (!--iter) {
            break;
         }
         /* generate new message */
         uint32_t maxsize = ((double) random() / RAND_MAX) * TRAP_IFC_MESSAGEQ_SIZE;
         uint32_t *bs = (uint32_t *) buffer;
         bp = (void *) (bs + 1);
         uint32_t used_size = 0;
         uint16_t *ms = (uint16_t *) bp;
         while (used_size < maxsize) {
            uint16_t messsize = ((double) random() / RAND_MAX) * UINT16_MAX;
            messsize = maxsize-used_size-2 < messsize ? maxsize-used_size : messsize;
            messsize %= 90;
            messsize += 88;
            used_size += messsize + sizeof messsize;
            *ms = messsize;
            bp += messsize + sizeof *ms;
            ms = (uint16_t *) bp;
         }
         *bs = htonl(used_size);
         bp = buffer;
         pending_bytes = used_size + sizeof used_size;
      } else {
         /* send */
         struct timeval tm = {0, 0};
         send_all_data(&c, csd, &bp, &pending_bytes, &tm);
      }

   }

  return 0;
}

