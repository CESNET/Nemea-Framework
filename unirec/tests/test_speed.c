#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "../unirec.h"

struct flow_rec_s {
   ip_addr_t dst_ip;
   ip_addr_t src_ip;
   uint64_t  bytes;
   uint32_t  packets;
   uint16_t  dst_port;
   uint16_t  src_port;
   uint8_t   protocol;
   uint16_t  url_len;
   char      url[20];
} __attribute__((packed));
typedef struct flow_rec_s flow_rec_t;


int main(int argc, char **argv)
{
#ifdef UNIREC
   ur_template_t *tmplt = ur_create_template("SRC_IP,DST_IP,SRC_PORT,DST_PORT,PROTOCOL,PACKETS,BYTES,URL");
   if (tmplt == NULL) {
      fprintf(stderr, "Error when creating UniRec template.\n");
      return 1;
   }

   char *rec =
#else
   flow_rec_t *rec = (flow_rec_t*)
#endif
      "asdfqwerASDFQWER"
      "asdfqwerASDFQWER"
      "bytesxyz"
      "pkts"
      "dp"
      "sp"
      "p"
      "\x13\x00"
      "http://example.com/";

   int x = 0;
   uint16_t y = 0;
   uint64_t z = 0;
   char tmp_str[20];

   time_t start_time = time(NULL);

   for (int i = 0; i < 1000000000; i++) {
#ifdef UNIREC
      if (ip_is4(ur_get_ptr(tmplt, rec, UR_SRC_IP)))
         x += 1;
      if (ip_is4(ur_get_ptr(tmplt, rec, UR_DST_IP)))
         x += 1;
      y += ur_get(tmplt, rec, UR_SRC_PORT);
      y += ur_get(tmplt, rec, UR_DST_PORT);
      y += ur_get(tmplt, rec, UR_PROTOCOL);
      z += ur_get(tmplt, rec, UR_PACKETS);
      z += ur_get(tmplt, rec, UR_BYTES);
      memcpy(tmp_str, ur_get_dyn(tmplt, rec, UR_URL), ur_get_dyn_size(tmplt, rec, UR_URL));
#else
      if (ip_is4(&rec->src_ip))
         x += 1;
      if (ip_is4(&rec->dst_ip))
         x += 1;
      y += rec->src_port;
      y += rec->dst_port;
      y += rec->protocol;
      z += rec->packets;
      z += rec->bytes;
      memcpy(tmp_str, rec->url, rec->url_len);
#endif
   }

   printf("%i %hu %lu\n", x, y, z);

   printf("Time: %us\n", time(NULL) - start_time);

#ifdef UNIREC
   ur_free_template(tmplt);
#endif

   return 0;
}
