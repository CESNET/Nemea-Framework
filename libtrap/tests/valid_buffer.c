#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <error.h>
#include <stdint.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "../include/libtrap/trap.h"

int read_check_buffer(FILE *f)
{
   static uint64_t file_offset = 0;
   uint32_t offset, buf_size = 0, check_mess_counter = 0;
   uint16_t *check_mess_header;
   void *check_mess_pointer;
   int errors = 0;
   size_t rb;
   uint8_t buffer[TRAP_IFC_MESSAGEQ_SIZE];

   rb = fread(&buf_size, sizeof(buf_size), 1, f);
   if (rb != 1) {
      warnx("Could not read buffer header at offset %"PRIu64, file_offset);
      return 1;
   }
   file_offset += sizeof(buf_size);
   buf_size = ntohl(buf_size);
   rb = fread(buffer, buf_size, 1, f);
   if (rb != 1) {
      warnx("Read buffer content (%"PRIu32" B) at offset %"PRIu64, htonl(buf_size), file_offset);
      return 1;
   }
   file_offset += ntohl(buf_size);


   for (offset = 0, check_mess_header = check_mess_pointer = buffer;
         ((offset < buf_size) && (offset < TRAP_IFC_MESSAGEQ_SIZE));) {
      check_mess_counter++;
      // go to next size
      offset += sizeof(*check_mess_header) + (*check_mess_header); // skip header + payload
      check_mess_pointer += sizeof(*check_mess_header) + (*check_mess_header);
      check_mess_header = (uint16_t *) check_mess_pointer;
   }
   if (offset != buf_size) {
      warnx("Not enough data or some headers are malformed.");
      errors++;
      return errors;
   } else {
      printf("%"PRIu64": buffer size %"PRIu32" B with %"PRIu32" messages.\n",
             file_offset, buf_size, check_mess_counter);
   }
   return 0;
}

int main(int argc, char **argv)
{
   FILE *f;
   int errors = 0;

   if (argc != 2) {
      puts("Usage: valid_buffer <file>");
      puts("For reading from standard input use /dev/stdin as a file.");
      return 1;
   }

   f = fopen(argv[1], "rb");
   while (f && !feof(f)) {
      errors = read_check_buffer(f);
      if (errors != 0) {
         break;
      }
   }
   fclose(f);

   return 0;
}

