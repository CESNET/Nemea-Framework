/**
 * \file trap_module_info.h
 * \brief Structures containing information about Nemea modules and macros for initialization and release of these structures.
 * \author Marek Svepes <svepemar@fit.cvut.cz>
 * \date 2015
 */
/*
 * Copyright (C) 2015 CESNET
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#ifndef _TRAP_MODULE_INFO_H_
#define _TRAP_MODULE_INFO_H_

/**
 * \addtogroup commonapi
 * @{
 */
/**
 * \defgroup module_parameters Parameters of modules
 *
 * This section contains a set of macros that should be used for definition
 * of parameters for a module.  Usage of these macros helps to generate
 * machine readable information about module that can be read e.g. by
 * supervisor.
 *
 * \section example-module-parameters Example of usage of module's parameters
 *
 * This example show the usage of macros defined in this section.
 *
 * \code{.c}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <getopt.h>

#include "module_info_test.h"

module_info_test_t *module_info = NULL;


// Definition of basic module information - module name, module description,
// number of input and output interfaces
#define MODULE_BASIC_INFO(BASIC) \
  BASIC("name", "description", 1, 1)

// Definition of module parameters - every parameter has short_opt, long_opt,
// description, flag whether argument is required or it is optional
// and argument type which is NULL in case the parameter does not need argument
#define MODULE_PARAMS(PARAM) \
  PARAM('s', "long_opt", "description", 0, NULL) \
  PARAM('b', "long_opt2", "description2", 1, "argument_type") \
  PARAM('d', "long_opt3", "description3", 1, "argument_type") \
  PARAM('u', "long_opt4", "description4", 0, NULL) \
  PARAM('i', "long_opt5", "description5", 1, "argument_type") \
  PARAM('c', "long_opt6", "description6", 1, "argument_type")


int main()
{
   uint32_t x = 0;

   // Allocate and initialize module_info structure and all its members
   // according to the MODULE_BASIC_INFO and MODULE_PARAMS definitions
   INIT_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS);

   printf("--- Module_info structure after initialization ---\n");
   printf("Basic info: %s %s %d %d\nParams:\n", module_info->name,
          module_info->description, module_info->num_in_ifc,
          module_info->num_out_ifc);

   for (x = 0; x < trap_module_params_cnt; x++) {
      printf("-%c --%s %s %d %s\n", module_info->params[x].short_opt,
             module_info->params[x].long_opt,
             module_info->params[x].description,
             module_info->params[x].param_required_argument,
             module_info->params[x].argument_type);
   }

   // Generate long_options array of structures for getopt_long function
   GEN_LONG_OPT_STRUCT(MODULE_PARAMS);

   x = 0;
   printf("\n--- Long_options structure after initialization ---\n");
   while (long_options[x].name != 0) {
      printf("{%s, %d, 0, %c}\n", long_options[x].name,
             long_options[x].has_arg, (char)long_options[x].val);
      x++;
   }

   // Release allocated memory for module_info structure
   FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS);
   return 0;
}

 * \endcode
 *
 * @{
 */
/** Structure with information about one module parameter
 *  Every parameter contains short_opt, long_opt, description,
 *  flag whether the parameter requires argument and argument type.
 */
typedef struct trap_module_info_parameter_s {
   char   short_opt;
   char  *long_opt;
   char  *description;
   int param_required_argument;
   char  *argument_type;
} trap_module_info_parameter_t;

/** Structure with information about module
 *  This struct contains basic information about the module, such as module's
 *  name, number of interfaces etc. It's supposed to be filled with static data
 *  and passed to trap_init function.
 */
typedef struct trap_module_info_s {
   char *name;           ///< Name of the module (short string)
   char *description;    /**< Detailed description of the module, can be a long
                              string with several lines or even paragraphs. */
   int num_ifc_in;  ///< Number of input interfaces
   int num_ifc_out; ///< Number of output interfaces
   trap_module_info_parameter_t **params;
} trap_module_info_t;

/** Macro generating one line of long_options field for getopt_long function
 */
#define GEN_LONG_OPT_STRUCT_LINE(p_short_opt, p_long_opt, p_description, p_required_argument, p_argument_type) {p_long_opt, p_required_argument, 0, p_short_opt},

/** Macro generating long_options field for getopt_long function according to given macro with parameters
 */
#define GEN_LONG_OPT_STRUCT(PARAMS) \
   static struct option long_options[] __attribute__((used)) = { \
      PARAMS(GEN_LONG_OPT_STRUCT_LINE) \
      {0, 0, 0, 0} \
   };

/** Macro for allocation and initialization of module basic information
 *
 * \param [in,out] module_info  pointer to module_info
 * \param [in] p_name   module's name
 * \param [in] p_description    module's description
 * \param [in] p_input  number of input IFCs
 * \param [in] p_output number of output IFCs
 */
#define ALLOCATE_BASIC_INFO_2(module_info, p_name, p_description, p_input, p_output) \
   if (module_info != NULL) { \
      if (p_name != NULL) { \
         module_info->name = strdup(p_name); \
      } else { \
         module_info->name = NULL; \
      } \
      if (p_description != NULL) { \
         module_info->description = strdup(p_description); \
      } else { \
         module_info->description = NULL; \
      } \
      module_info->num_ifc_in = p_input; \
      module_info->num_ifc_out = p_output; \
   }

/** Macro for allocation and initialization of module basic information in global module_info
 *
 * \param [in] p_name   module's name
 * \param [in] p_description    module's description
 * \param [in] p_input  number of input IFCs
 * \param [in] p_output number of output IFCs
 */
#define ALLOCATE_BASIC_INFO(p_name, p_description, p_input, p_output) \
   ALLOCATE_BASIC_INFO_2(module_info, p_name, p_description, p_input, p_output)

/** Macro for allocation and initialization of module parameters information
 *
 * \param [in] m  module_info pointer
 * \param [in] param_id   index of parameter to set
 * \param [in] p_short_opt  short option
 * \param [in] p_long_opt   long option
 * \param [in] p_description  option description
 * \param [in] p_required_argument    no_argument | required_argument | optional_argument
 * \param [in] p_argument_type  data type of option argument
 */
#define ALLOCATE_PARAM_ITEMS_2(m, param_id, p_short_opt, p_long_opt, p_description, p_required_argument, p_argument_type) \
   if (m != NULL) { \
      if (m->params[param_id] == NULL) { \
         m->params[param_id] = (trap_module_info_parameter_t *) calloc(1, sizeof(trap_module_info_parameter_t)); \
      } \
      if (m->params[param_id] != NULL) { \
         if (p_short_opt != 0) { \
            m->params[param_id]->short_opt = p_short_opt; \
         } else { \
            m->params[param_id]->short_opt = 0; \
         } \
         if (p_long_opt != NULL) { \
            m->params[param_id]->long_opt = strdup(p_long_opt); \
         } else { \
            m->params[param_id]->long_opt = strdup(""); \
         } \
         if (p_description != NULL) { \
            m->params[param_id]->description = strdup(p_description); \
         } else { \
            m->params[param_id]->description = strdup(""); \
         } \
         if (p_required_argument == 1) { \
            m->params[param_id]->param_required_argument = p_required_argument; \
         } else { \
            m->params[param_id]->param_required_argument = 0; \
         } \
         if (p_argument_type != NULL) { \
            m->params[param_id]->argument_type = strdup(p_argument_type); \
         } else { \
            m->params[param_id]->argument_type = strdup(""); \
         } \
      } \
   }

/** Macro for allocation and initialization of module parameters information in global module_info
 *
 * \param [in] p_short_opt  short option
 * \param [in] p_long_opt   long option
 * \param [in] p_description  option description
 * \param [in] p_required_argument    no_argument | required_argument | optional_argument
 * \param [in] p_argument_type  data type of option argument
 */
#define ALLOCATE_PARAM_ITEMS(p_short_opt, p_long_opt, p_description, p_required_argument, p_argument_type) \
   ALLOCATE_PARAM_ITEMS_2(module_info, trap_module_params_cnt, p_short_opt, p_long_opt, p_description, p_required_argument, p_argument_type) \
   trap_module_params_cnt++;

/** Macro releasing memory allocated for module basic information in global variable module_info
 */
#define FREE_BASIC_INFO(p_name, p_description, p_input, p_output) \
   if (module_info->name != NULL) { \
      free(module_info->name); \
      module_info->name = NULL; \
   } \
   if (module_info->description != NULL) { \
      free(module_info->description); \
      module_info->description = NULL; \
   }

/** Macro releasing memory allocated for module parameters information in global variable module_info
  */
#define FREE_PARAM_ITEMS(p_short_opt, p_long_opt, p_description, p_required_argument, p_argument_type) \
   if (module_info->params[trap_module_params_cnt] != NULL) { \
      if (module_info->params[trap_module_params_cnt]->long_opt != NULL) { \
         free(module_info->params[trap_module_params_cnt]->long_opt); \
         module_info->params[trap_module_params_cnt]->long_opt= NULL; \
      } \
      if (module_info->params[trap_module_params_cnt]->description != NULL) { \
         free(module_info->params[trap_module_params_cnt]->description); \
         module_info->params[trap_module_params_cnt]->description= NULL; \
      } \
      if (module_info->params[trap_module_params_cnt]->argument_type != NULL) { \
         free(module_info->params[trap_module_params_cnt]->argument_type); \
         module_info->params[trap_module_params_cnt]->argument_type= NULL; \
      } \
      if (module_info->params[trap_module_params_cnt] != NULL) { \
         free(module_info->params[trap_module_params_cnt]); \
      } \
      trap_module_params_cnt++; \
   }

#define GENERATE_GETOPT_STRING(p_short_opt, p_long_opt, p_description, p_required_argument, p_argument_type) \
  if (module_getopt_string_size <= (strlen(module_getopt_string) + 2)) { \
    module_getopt_string_size += module_getopt_string_size/2; \
    module_getopt_string = (char *) realloc(module_getopt_string, module_getopt_string_size * sizeof(char)); \
    memset(module_getopt_string + (2*module_getopt_string_size)/3, 0, module_getopt_string_size/3); \
  } \
  module_getopt_string[strlen(module_getopt_string)] = p_short_opt; \
  if (p_required_argument == required_argument) { \
    module_getopt_string[strlen(module_getopt_string)] = ':'; \
  }

/** Macro counting number of module parameters - memory allocation purpose
 */
#define COUNT_MODULE_PARAMS(p_short_opt, p_long_opt, p_description, p_required_argument, p_argument_type) trap_module_params_cnt++;

/** Macro initializing whole module_info structure in global variable module_info
 *  First argument is macro defining module basic information (name, description, number of input/output interfaces)
 *  Second argument is macro defining all module parameters and their values
 *  Last pointer of module info parameters array is NULL to detect end of the array without any counter
 */
#define INIT_MODULE_INFO_STRUCT(BASIC, PARAMS) \
   int trap_module_params_cnt = 0; \
   size_t module_getopt_string_size = 50; \
   char * module_getopt_string = (char *) calloc(module_getopt_string_size, sizeof(char)); \
   module_info = (trap_module_info_t *) calloc(1, sizeof(trap_module_info_t)); \
   GEN_LONG_OPT_STRUCT(PARAMS); \
   BASIC(ALLOCATE_BASIC_INFO) \
   PARAMS(COUNT_MODULE_PARAMS) \
   if (module_info != NULL) { \
      module_info->params = (trap_module_info_parameter_t **) calloc(trap_module_params_cnt + 1, sizeof(trap_module_info_parameter_t *)); \
      if (module_info->params != NULL) { \
         trap_module_params_cnt = 0; \
         PARAMS(ALLOCATE_PARAM_ITEMS) \
         PARAMS(GENERATE_GETOPT_STRING) \
      } \
   }

/**
 * Allocate module info with empty parameters array.
 *
 * \param [in] mname  name of Nemea module
 * \param [in] mdesc  description of Nemea module
 * \param [in] i_ifcs number of input IFCs
 * \param [in] o_ifcs number of output IFCs
 * \param [in] param_count  number of parameters
 * \return pointer to module_info with allocated parameters, NULL otherwise.
 */
trap_module_info_t *trap_create_module_info(const char *mname, const char *mdesc, int8_t i_ifcs, int8_t o_ifcs, uint16_t param_count);

/**
 * Set module's parameter in the allocated module_info structure.
 *
 * \param [in,out] m    pointer to allocated module_info
 * \param [in] param_id index of parameter which is set
 * \param [in] shortopt short form of the parameter e.g. 'f' for -f
 * \param [in] longopt  long form of the parameter e.g. "file" for --file
 * \param [in] desc     description of the parameter
 * \param [in] req_arg  requirement of argument, use standard required_argument, no_argument, optional_argument
 * \param [in] arg_type data type of argument
 */
int trap_update_module_param(trap_module_info_t *m, uint16_t param_id, char shortopt, const char *longopt, const char *desc, int req_arg, const char *arg_type);

/** Macro releasing whole module_info structure allocated in global variable module_info
 *  First argument is macro defining module basic information (name, description, number of input/output interfaces)
 *  Second argument is macro defining all module parameters and their values
*/
#define FREE_MODULE_INFO_STRUCT(BASIC, PARAMS) \
   if (module_info != NULL) { \
      trap_module_params_cnt = 0; \
      BASIC(FREE_BASIC_INFO) \
      if (module_info->params != NULL) { \
         PARAMS(FREE_PARAM_ITEMS) \
         free(module_info->params); \
         module_info->params = NULL; \
      } \
      free(module_info); \
      module_info = NULL; \
      if (module_getopt_string != NULL) { \
        free(module_getopt_string); \
      } \
   }


/**
 * @}
 */
/**
 * @}
 */

#endif
