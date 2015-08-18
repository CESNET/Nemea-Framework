/**
 * \file module_info_example.c
 * \brief Example of module_info structure usage.
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
#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <getopt.h>
#include <unistd.h>

#include <libtrap/trap.h>

// Variable containing  module information in a form of trap_module_info_t structure (IMPORTANT - name of the variable must be "module_info" because of libtrap macros)
trap_module_info_t * module_info = NULL;

// Definition of basic module information - module name, module description, number of input and output interfaces
#define MODULE_BASIC_INFO(BASIC) \
  BASIC("module_name","module_description",1,1)

//BASIC(char *, char *, int, int)

// Definition of module parameters - every parameter has short_opt, long_opt, description, flag whether argument is required or it is optional
// and argument type which is NULL in case the parameter does not need argument.
// Module parameter argument types: int8, int16, int32, int64, uint8, uint16, uint32, uint64, float, string
#define MODULE_PARAMS(PARAM) \
  PARAM('s', "sampling", "param_description", no_argument, "none") \
  PARAM('p', "port", "param_description2", required_argument, "int8") \
  PARAM('L', "logs", "param_description3", required_argument, "string") \
  PARAM('h', "help", "param_description4", no_argument, "none") \
  PARAM('r', "records_num", "param_description5", required_argument, "uint16") \
  PARAM('f', "file", "param_description6", required_argument, "string")

//PARAM(char, char *, char *, no_argument  or  required_argument, char *)



int main(int argc, char *argv[])
{
	uint x = 0;
	int opt = 0;

	// IMPORTANT
	// Macro allocates and initializes module_info structure and all its members according to the MODULE_BASIC_INFO and MODULE_PARAMS definitions on the lines 14 and 21 of this file
	// It also creates a string with short_opt letters for getopt function called "module_getopt_string" and long_options field for getopt_long function in variable "long_options"
	INIT_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)


	// Printing out whole module_info structure
	printf("--- Module_info structure after initialization ---\n");
	printf("Basic info:\n%s %s %d %d\n\nParams:\n", module_info->name, module_info->description, module_info->num_ifc_in, module_info->num_ifc_out);
	while (module_info->params[x] != NULL) {
		printf("-%c --%s %s %d %s\n", module_info->params[x]->short_opt, module_info->params[x]->long_opt, module_info->params[x]->description, module_info->params[x]->param_required_argument, module_info->params[x]->argument_type);
		x++;
	}

	// Printing out static structure long_options used in getopt_long function
	x = 0;
	printf("\n--- Long_options structure after initialization ---\n");
	while (long_options[x].name != 0) {
		printf("{%s, %d, 0, '%c'}\n", long_options[x].name, long_options[x].has_arg, (char)long_options[x].val);
		x++;
	}

	// Printing out string variable module_getopt_string containing short_opt letters for getopt function
	printf("\n--- Getopt string ---\n\"%s\"\n", module_getopt_string);


	// Usage of the created getopt string
	printf("\n--- Params parsing ---\n");
	while ((opt = TRAP_GETOPT(argc, argv, module_getopt_string, long_options)) != -1) {
		switch (opt) {
		case 's':
			printf("opt: \"%c\"\n", opt);
			break;
		case 'p':
			printf("opt: \"%c\"\n", opt);
			break;
		case 'L':
			printf("opt: \"%c\"\n", opt);
			break;
		case 'h':
			printf("opt: \"%c\"\n", opt);
			break;
		case 'r':
			printf("opt: \"%c\"\n", opt);
			break;
		case 'f':
			printf("opt: \"%c\"\n", opt);
			break;

		default:
			fprintf(stderr, "Invalid arguments.\n");
			goto exit;
		}
	}


	/*
	*
	* Modules body
	*
	*
	*/

exit:
	// IMPORTANT
	// Release allocated memory for module_info structure and the rest of the used variables
	FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)
	return 0;
}
