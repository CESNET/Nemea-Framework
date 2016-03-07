#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <nemea-common/configurator.h>

// First way of loading plain text configuration.
// Configuration in this example is placed into structure.

// All C structures must be declared as __attribute__ ((__packed__))!
// Order of items in defined C structure must correspond order of items
// specified by confPlainAddElement function!
typedef struct __attribute__ ((__packed__)) {
   int32_t optional_var1;
   uint32_t optional_var2;
   double var3;
   char string1[32];
   int64_t var4;
   uint16_t optional_var5;
} main_struct_t;

int main(int argc, char **argv)
{
   // Initialize configurator for loading plain text file.
   // Must be called before confPlainAddElement and confPlainLoadConfiguration functions.
   confPlainCreateContext();

   // Specify configuration items (confPlainCreateContext must be called first!).
   // Remember, order of added items must respect the order of items in defined C structure!

   // confPlainAddElement(name, type, default_value, char_array_length, required_item)
   if (confPlainAddElement("optional_var1", "int32_t", "-666", 0, 0) ||
       confPlainAddElement("optional_var2", "uint32_t", "0", 0, 0) ||
       confPlainAddElement("var3", "double", "0.123", 0, 1) ||
       confPlainAddElement("string1", "string", "abcXYZ 123", 32, 1) ||
       confPlainAddElement("var4", "int64_t", "123456", 0, 1) ||
       confPlainAddElement("optional_var5", "uint16_t", "65000", 0, 0)) {
      printf("Unable to add element.\n");
      return EXIT_FAILURE;
   }

   // Load configuration from plain text file into C structure.
   // Parsing is based on configuration items added by confPlainAddElement function.
   main_struct_t mystr;
   if (confPlainLoadConfiguration("conf.txt", (void *)&mystr)) {
      printf("Unable to load plaintext configuration.\n");
      confPlainClearContext();
      return EXIT_FAILURE;
   }

   // Free configurator resources.
   confPlainClearContext();

   printf("Configuration:\n");
   printf("optional_var1 = %d\noptional_var2 = %u\nvar3 = %lf\nstring1 = %s\nvar4 = %ld\noptional_var5 = %u\n",
           mystr.optional_var1, mystr.optional_var2, mystr.var3, mystr.string1, mystr.var4, mystr.optional_var5);

   return 0;
}
