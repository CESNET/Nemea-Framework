#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <nemea-common/configurator.h>

// Second way of retrieving plain text configuration.
// Configuration items are returned using confPlainGetXXXX functions.

int main(int argc, char **argv)
{
   // Initialize configurator for loading plain text file.
   // Must be called before confPlainAddElement and confPlainLoadConfiguration functions.
   confPlainCreateContext();

   // Specify configuration items (confPlainCreateContext must be called first!).
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

   // Load configuration from plain text file. If NULL pointer is supplied to user structure parameter,
   // configuration values could be retrieved using confPlainGetXXXX functions.
   // Parsing is based on configuration items added by confPlainAddElement function.
   if (confPlainLoadConfiguration("conf.txt", NULL)) {
      printf("Unable to load plaintext configuration.\n");
      confPlainClearContext();
      return EXIT_FAILURE;
   }

   printf("Configuration:\n");
   printf("optional_var1 = %d\noptional_var2 = %u\nvar3 = %lf\nstring1 = %s\nvar4 = %ld\noptional_var5 = %u\nundefined var = %s\n",
            confPlainGetInt32("optional_var1", 987),
            confPlainGetUint32("optional_var2", 0),
            confPlainGetDouble("var3", 3434.4545),
            confPlainGetString("string1", ""),
            confPlainGetInt64("var4", 1111),
            confPlainGetUint16("optional_var5", 7),
            confPlainGetString("not-defined", "Default values are returned for undefined item names."));

   // Free configurator resources. You can still use confPlainGetXXXX functions now, but defaultValue will be always returned.
   confPlainClearContext();
   return 0;
}
