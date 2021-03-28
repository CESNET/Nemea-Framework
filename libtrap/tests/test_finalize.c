/**
 * \file test_finalize.c
 * \brief Repeat init and finalize of libtrap
 * \author Pavel Krobot <xkrobo01@stud.fit.vutbr.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <libtrap/trap.h>

#define MODULE_NAME "Init Test"
#define MODULE_DESCRIPTION "...\n"


trap_module_info_t in_module_info = {
	MODULE_NAME,// Module name
	MODULE_DESCRIPTION,// Module description
	1,// Number of input interfaces
	0,// Number of output interfaces
};

trap_module_info_t out_module_info = {
	MODULE_NAME,// Module name
	MODULE_DESCRIPTION,// Module description
	0,// Number of input interfaces
	1,// Number of output interfaces
};



/* Global components */
int stop = 0;// Signaling termination of program (by SIGINT or SIGTERM)

/**
 * \brief Function to handle SIGTERM and SIGINT signals (used to stop the program);
 * \param[in] signal Signal number.
 */
void trap_default_signal_handler(int signal)
{
   if (signal == SIGTERM || signal == SIGINT) {
      stop = 1;
      fprintf(stderr, "\nInfo: Caught signal %d, please wait for termination.\n", signal);
   }
}

/**
 * \brief Print program help and usage.
 */
void print_help (char *module_name){
   trap_print_help(&out_module_info);
   printf(
      "Usage: %s [ -d ] [ -h ] [ -v ]\n"
      " -d :\n"
      " -v :\n"
      " -h : Print this help and exit.\n",
      module_name);
}

/**
 * \brief Main function of slave node.
 * \param[in] argc
 * \param[in] argv
 * \return  Returns EXIT_SUCCESS on success, EXIT_FAILURE code otherwise.
 */
int main(int argc, char **argv)
{
   // *** Declarations ***
   int status = EXIT_SUCCESS; // Program status

   int trap_ret;
   trap_ctx_t *in_ctx;
   trap_ctx_t *out_ctx;

   ////////////////////////////////////////////////////////////////////////////////
   //// 1. Initialization and set up

   // ***** TRAP interface initialization ***** >>>
   // Prepare structure for Trap interface setup
   char *in_ifc[] = {"test", "-i", "t:localhost:6111"};
   int arg_cnt = 3;

   in_ctx = NULL;

   trap_ifc_spec_t in_ifc_spec;

   trap_ret = trap_parse_params(&arg_cnt, in_ifc, &in_ifc_spec);
   if (trap_ret != TRAP_E_OK) {// TRAP_E_HELP is not treated here since parameters to this function is passed from static structure
      trap_free_ifc_spec(in_ifc_spec);
      fprintf(stderr, "Error: Error in parsing of parameters for TRAP (input): %s\n", trap_last_error_msg);
      return EXIT_FAILURE;
   }

   // Initialize Trap input context
   in_ctx = trap_ctx_init(&in_module_info, in_ifc_spec);

   // Since we do not need this structure anymore ...
   trap_free_ifc_spec(in_ifc_spec);

   if (in_ctx == NULL || trap_ctx_get_last_error(in_ctx) != TRAP_E_OK) {
      fprintf(stderr, "Error: Error in TRAP initialization (input context): %s\n", trap_last_error_msg);
      return EXIT_FAILURE;
   }

   // ***** TRAP interface control ***** >>>
   // Set timeout on input (command) interface
   if (trap_ctx_ifcctl(in_ctx, TRAPIFC_INPUT, 0, TRAPCTL_SETTIMEOUT, TRAP_WAIT) != TRAP_E_OK) {
      fprintf(stderr, "Error: Error while setting up input interface timeout.\n");
   }

   // *** END OF TRAP interface initialization and control ***** <<<

   // *** Register signal handler ***
   TRAP_REGISTER_DEFAULT_SIGNAL_HANDLER();

   ////////////////////////////////////////////////////////////////////////////////
   ////// 2. Main loop
   int maxtry = 5;
   while (!stop && maxtry--) {
      //////      // ***** TRAP interface initialization ***** >>>
      printf("starting.\n");
      sleep(1);
      // After receiving of command, open output (data) interface
      char *out_ifc[] = {"test", "-i", "t:61000"};
      int arg_cnt = 3;

      out_ctx = NULL;

      trap_ifc_spec_t out_ifc_spec;

      trap_ret = trap_parse_params(&arg_cnt, out_ifc, &out_ifc_spec);
      if (trap_ret != TRAP_E_OK) {// TRAP_E_HELP is not treated here since parameters to this function is passed from static structure
         trap_free_ifc_spec(out_ifc_spec);
         fprintf(stderr, "Error: Error in parsing of parameters for TRAP (output): %s\n", trap_last_error_msg);
         return EXIT_FAILURE;
      }
      printf("Processing.\n");

      // Initialize Trap output context
      out_ctx = trap_ctx_init(&out_module_info, out_ifc_spec);

      // Since we do not need this structure anymore ...
      trap_free_ifc_spec(out_ifc_spec);

      if (out_ctx == NULL || trap_ctx_get_last_error(out_ctx) != TRAP_E_OK) {
         fprintf(stderr, "Error: Error in TRAP initialization (output context): %s\n", trap_last_error_msg);
         return EXIT_FAILURE;
      }
      // ***** TRAP interface control ***** >>>
      // Set timeout on output (data) interface
      if (trap_ctx_ifcctl(out_ctx, TRAPIFC_OUTPUT, 0, TRAPCTL_SETTIMEOUT, TRAP_WAIT) != TRAP_E_OK) {
         fprintf(stderr, "Error: Error while setting up output interface timeout.\n");
      }
      // *** END OF TRAP interface initialization and control ***** <<<
      // Wait for master node to connect on output interface
      sleep(5);
      printf("Finalize.\n");
      trap_ctx_finalize(&out_ctx);
      trap_ctx_finalize(&in_ctx);
      out_ctx = NULL;
   }

   return status;
}

