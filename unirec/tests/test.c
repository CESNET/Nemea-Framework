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

   // ** Create a record **

   // Allocate memory for a record
   void *rec = ur_create(tmplt, 128); // allocates size of static fields + 128 bytes for URL

   ip_addr_t src_addr, dst_addr;
   ip_from_str("1.2.3.4", &src_addr);
   ip_from_str("250.240.230.220", &dst_addr);

   // Fill all fields
   ur_set(tmplt, rec, UR_SRC_IP, src_addr);
   ur_set(tmplt, rec, UR_DST_IP, dst_addr);
   ur_set(tmplt, rec, UR_SRC_PORT, 5555);
   ur_set(tmplt, rec, UR_DST_PORT, 80);
   ur_set(tmplt, rec, UR_PROTOCOL, 6);
   ur_set(tmplt, rec, UR_PACKETS, 4321);
   ur_set(tmplt, rec, UR_BYTES, 1234567);
   // setting of dynamic fieds is not solved yet, this solution works for one dynamic field only
   memcpy(ur_get_dyn(tmplt, rec, UR_URL), "something.example.com/index.html", 33);
   *(uint16_t*)ur_get_ptr(tmplt, rec, UR_URL) = 33;

   // ** Read and print the record **

   char addr[46];
   ip_to_str(ur_get_ptr(tmplt, rec, UR_SRC_IP), addr);
   printf("SRC_IP: %s\n", addr);
   ip_to_str(ur_get_ptr(tmplt, rec, UR_DST_IP), addr);
   printf("DST_IP: %s\n", addr);
   printf("SRC_PORT: %hu\n", ur_get(tmplt, rec, UR_SRC_PORT));
   printf("DST_PORT: %hu\n", ur_get(tmplt, rec, UR_DST_PORT));
   printf("PROTOCOL: %hu\n", (unsigned short)ur_get(tmplt, rec, UR_PROTOCOL));
   printf("PACKETS: %u\n", ur_get(tmplt, rec, UR_PACKETS));
   printf("BYTES: %lu\n", ur_get(tmplt, rec, UR_BYTES));
   printf("URL: %s\n", ur_get_dyn(tmplt, rec, UR_URL));
   printf("UniRec size: %hu + %hu = %hu\n",
          ur_rec_static_size(tmplt), ur_rec_dynamic_size(tmplt, rec), ur_rec_size(tmplt, rec));

   ur_free(rec);
   ur_free_template(tmplt);

   return 0;
}
