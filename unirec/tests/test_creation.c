#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "../unirec.h"


int main(int argc, char **argv)
{
   // Create a template
   ur_template_t *tmplt = ur_create_template("SRC_IP,DST_IP,SRC_PORT,DST_PORT,PROTOCOL,PACKETS,BYTES,URL");
   if (tmplt == NULL) {
      fprintf(stderr, "Error when creating UniRec template.\n");
      return 1;
   }

   // Allocate memory for a record
   void* rec = ur_create(tmplt, 128); // allocates size of static fields + 128 bytes for URL

   ip_addr_t src_addr, dst_addr;
   ip_from_str("1.0.0.0", &src_addr);
   ip_from_str("255.0.0.0", &dst_addr);
   int x = 0;
   uint16_t y = 0;
   uint64_t z = 0;
   const char *url1 = "www.example.com";
   const char *url2 = "something.example.com/index.html";
   size_t url1_len = strlen(url1);
   size_t url2_len = strlen(url2);


   time_t start_time = time(NULL);

   for (int i = 0; i < 1000000000; i++) {
      ur_set(tmplt, rec, UR_SRC_IP, src_addr);
      ur_set(tmplt, rec, UR_DST_IP, dst_addr);
      src_addr.ui32[2] += 1;
      dst_addr.ui32[2] += 7;
      ur_set(tmplt, rec, UR_SRC_PORT, y++);
      ur_set(tmplt, rec, UR_DST_PORT, y*=2);
      ur_set(tmplt, rec, UR_PROTOCOL, x++);
      ur_set(tmplt, rec, UR_PACKETS, x++);
      ur_set(tmplt, rec, UR_BYTES, z+=100);
      // setting of dynamic fieds is not solved yet, this solution works for one dynamic field only
      if (i & 1) {
         memcpy(ur_get_dyn(tmplt, rec, UR_URL), url1, url1_len);
         *(uint16_t*)ur_get_ptr(tmplt, rec, UR_URL) = url1_len;
      } else {
         memcpy(ur_get_dyn(tmplt, rec, UR_URL), url2, url2_len);
         *(uint16_t*)ur_get_ptr(tmplt, rec, UR_URL) = url2_len;
      }
   }

   printf("Time: %us\n", time(NULL) - start_time);

   ur_free(rec);

   ur_free_template(tmplt);

   return 0;
}