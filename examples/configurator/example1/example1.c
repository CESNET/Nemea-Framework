#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <nemea-common/configurator.h>

// Example of loading configuration from XML files.

// All C structures must be declared as __attribute__ ((__packed__))!
typedef struct __attribute__ ((__packed__)) {
   uint32_t id;
   char name[10];
} sub_struct_t;

// Items in C structure must be defined in same order as are items
// in patternxml file !!!
typedef struct __attribute__ ((__packed__)) {
   int64_t Variable1;
   double Variable2;
   uint32_t Variable_optional;
   char Variable_string[8];
   struct __attribute__ ((__packed__)){
      uint8_t Struct_variable1;
      uint32_t Struct_optional_variable;
   } first_struct;

   char Last_param[10]; // String with 9 chars and terminating 0
   sub_struct_t *arr;   // Array of structures
   int64_t Variable3;
   char *narr;          // String array
   int32_t *num_arr;    // int32 array
} main_struct_t;

int main(int argc, char **argv)
{
   // Load configuration from XML into user defined structure.
   // pattern.xml contains configuration pattern
   // userConf.xml contains actual configuration
   main_struct_t str;
   if (confXmlLoadConfiguration("pattern.xml", "userConf.xml", (void *)&str, CONF_PATTERN_FILE)) {
      printf("Unable to load xml configuration.\n");
      return EXIT_FAILURE;
   }

   printf("Configuration:\n");
   printf("Variable1: %ld\nVariable2: %lf\nVariable_optional: %u\nVariable string: %s\n",
          str.Variable1, str.Variable2, str.Variable_optional, str.Variable_string);
   printf("Struct_variable1: %u\nStruct_optional_variable: %u\nLast_param: %s\n",
          str.first_struct.Struct_variable1, str.first_struct.Struct_optional_variable, str.Last_param);

   int cnt = confArrElemCount(str.arr), i;   // Get length of created array.
   for (i = 0; i < cnt; i++) {
      printf("arr: %u %s\n", str.arr[i].id, str.arr[i].name);
   }

   printf("Variable3: %ld\nnarr: %s\n", str.Variable3, str.narr);

   cnt = confArrElemCount(str.num_arr); // Get length of created array.
   for (i = 0; i < cnt; i++) {
      printf("num_arr: %d\n", str.num_arr[i]);
   }

   // Free created arrays.
   confFreeUAMBS();

   return 0;
}
