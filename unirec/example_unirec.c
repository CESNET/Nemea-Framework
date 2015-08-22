// ** Put header + licence here **

/* Example of a module with a fixed set of input and output UniRec fields.
   Input requires two numbers: FOO and BAR. Output contains fields FOO, BAR
   and BAZ, where BAZ = FOO + BAR.
   If input contains some other fields besides FOO and BAR, they are NOT copied
   to the output. Output always consists of FOO, BAR, BAZ only.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <string.h>
#include "fields.c"
//Example of usage Unirec library
UR_FIELDS(
   uint32 FOO,
   uint32 BAR,
   ipaddr IP,
   string STR1,
   string STR2,
)

int main(int argc, char **argv)
{
   char *buffer, *buffer2;

   // Create a record and store it to the buffer
   {
      // Create template
      ur_template_t *tmplt = ur_create_template("FOO,BAR,IP,STR1", NULL);

      // Allocate memory for a record
      void *rec = ur_create_record(tmplt, 512); // Pre-allocate 512 bytes for strings

      // Fill values into the record
      ur_set(tmplt, rec, F_FOO, 12345);
      ur_set(tmplt, rec, F_BAR, 54321);
      ur_set(tmplt, rec, F_IP, ip_from_int(12345678));
      ur_set_string(tmplt, rec, F_STR1, "Hello World!");
      // Store record into a buffer
      buffer = malloc(ur_rec_size(tmplt, rec));
      memcpy(buffer, rec, ur_rec_size(tmplt, rec));

      // Free memory
      free(rec);
      ur_free_template(tmplt);
   }

   // -----

   // Read data from the record in the buffer
   {
      // Create another template with the same set of fields (the set of fields MUST be the same, even if we don't need to work with all fields)
      ur_template_t *tmplt = ur_create_template("FOO,BAR,IP,STR1", NULL);

      // Read FOO, IP and STR1 fields from the record in the buffer
      // (to keep it simple, ip address is printed as a number)
      printf("%u %u %u %s\n",
             ur_get(tmplt, buffer, F_FOO), // Access to plain integer is simple
             ip_get_v4_as_int(&(ur_get(tmplt, buffer, F_IP))), // IP address must be converted before print
             ur_get_var_len(tmplt, buffer, F_STR1), // Returns length of the string (it is neccessary to use "%*s" since string in UniRec don't contain terminating '\0')
             ur_get_ptr(tmplt, buffer, F_STR1) // Returns pointer to the beginning of the string
            );
      // -> should print "12345 12345678 13 Hello World!"

      ur_free_template(tmplt);
   }

   // -----

   // Copy selected fields of the record into a new one and add two new fields,
   // one is known before (STR2), one is newly defined (NEW)
   {
      // Define the field NEW
      ur_field_id_t new_id = ur_define_field("NEW", UR_TYPE_UINT16);
      if (new_id < 0) {
         fprintf(stderr, "ERROR: Can't define new unirec field 'NEW'");
         exit(1);
      }

      // Create templates matching the old and the new record
      ur_template_t *tmplt1 = ur_create_template("FOO,BAR,IP,STR1", NULL);
      ur_template_t *tmplt2 = ur_create_template("BAR,STR1,STR2,NEW", NULL);

      // Allocate buffer for the new record
      buffer2 = ur_create_record(tmplt2, ur_get_var_len(tmplt1, buffer, F_STR1) + 64); // Allocate 64B for STR2

      // This function copies all fields present in both templates from buffer to buffer2
      ur_copy_fields(tmplt2, buffer2, tmplt1, buffer);
      // TODO speciální funkce pro kopírovaní celé tmplt1, pokud víme, že tmplt2 je nadmonožinou tmplt1 (a pokud to tak bude efektivnìjší)

      // Record in buffer2 now contains fields BAR and STR1 with the same values as in buffer.
      // Values of STR2 and NEW are undefined.

      // Set value of str2
      ur_set_string(tmplt2, buffer2, F_STR2, "The second string");

      // Set value of NEW
      // (we can't use ur_set because type of the field is not known at compile time)
      *(uint16_t*)(ur_get_ptr_by_id(tmplt2, buffer2, new_id)) = 1;
      ur_free_template(tmplt1);
      ur_free_template(tmplt2);
   }

   // -----

   // Read data from the record in the second buffer
   {
      // Create template with the second set of fields (we can use different order of fields, it doesn't matter)
      ur_template_t *tmplt = ur_create_template("BAR,NEW,STR2,STR1", NULL);

      // The NEW field is already defined (it is stored globally) but we don't know its ID here
      ur_field_id_t new_id = ur_get_id_by_name("NEW");
      printf(" new field %d\n", new_id);

      // Read FOO, IP, STR1 and NEW fields from the record in the buffer
      printf("%u %u %s %u %s %u\n",
             ur_get(tmplt, buffer2, F_BAR),
             ur_get_var_len(tmplt, buffer2, F_STR1),
             ur_get_ptr(tmplt, buffer2, F_STR1),
             ur_get_var_len(tmplt, buffer2, F_STR2),
             ur_get_ptr(tmplt, buffer2, F_STR2),
             *(uint16_t*)ur_get_ptr_by_id(tmplt, buffer2, new_id) // Access to dynamically defined fields is a little more complicated
            );
      // -> should print "54321 13 Hello World! 18 The second string 1"

      ur_free_template(tmplt);
   }

   free(buffer);
   free(buffer2);
   ur_finalize();
   return 0;
}