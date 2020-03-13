/**
 * \file unirec.c
 * \brief Definition of UniRec structures and functions
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
 * \author Zdenek Rosa <rosazden@fit.cvut.cz>
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

#define _BSD_SOURCE
#define _XOPEN_SOURCE
#define __USE_XOPEN
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <regex.h>
#include <assert.h>
#include <ctype.h>
#include "unirec.h"
#include <libtrap/trap.h>
#include <inttypes.h>
#include "ur_values.c"
#include "inline.h"

// All inline functions from ipaddr.h must be declared again with "extern"
// in exactly one translation unit (it generates externally linkable code of
// these function)
// See this for explanation (Nemo's answer):
// http://stackoverflow.com/questions/6312597/is-inline-without-static-or-extern-ever-useful-in-c99
INLINE_IMPL int ip_is4(const ip_addr_t *addr);
INLINE_IMPL int ip_is6(const ip_addr_t *addr);
INLINE_IMPL uint32_t ip_get_v4_as_int(ip_addr_t *addr);
INLINE_IMPL char *ip_get_v4_as_bytes(const ip_addr_t *addr);
INLINE_IMPL ip_addr_t ip_from_int(uint32_t i);
INLINE_IMPL ip_addr_t ip_from_4_bytes_be(char b[4]);
INLINE_IMPL ip_addr_t ip_from_4_bytes_le(char b[4]);
INLINE_IMPL ip_addr_t ip_from_16_bytes_be(char b[16]);
INLINE_IMPL ip_addr_t ip_from_16_bytes_le(char b[16]);
INLINE_IMPL int ip_cmp(const ip_addr_t *addr1, const ip_addr_t *addr2);
INLINE_IMPL int ip_from_str(const char *str, ip_addr_t *addr);
INLINE_IMPL void ip_to_str(const ip_addr_t *addr, char *str);

INLINE_IMPL mac_addr_t mac_from_bytes(uint8_t *array);
INLINE_IMPL int mac_from_str(const char *str, mac_addr_t *addr);
INLINE_IMPL int mac_cmp(const mac_addr_t *addr1, const mac_addr_t *addr2);
INLINE_IMPL void mac_to_str(const mac_addr_t *addr, char *str);
INLINE_IMPL void mac_to_bytes(const mac_addr_t *addr, uint8_t *array);


/**
 * \brief Sizes of UniRec data types.
 *
 * Data types are defined in the #ur_field_type_str array.
 */
const int ur_field_type_size[] = {
   -1, /*UR_TYPE_STRING*/
   -1, /*UR_TYPE_BYTES*/
   1, /*UR_TYPE_CHAR*/
   1, /*UR_TYPE_UINT8*/
   1, /*UR_TYPE_INT8*/
   2, /*UR_TYPE_UINT16*/
   2, /*UR_TYPE_INT16*/
   4, /*UR_TYPE_UINT32*/
   4, /*UR_TYPE_INT32*/
   8, /*UR_TYPE_UINT64*/
   8, /*UR_TYPE_INT64*/
   4, /*UR_TYPE_FLOAT*/
   8, /*UR_TYPE_DOUBLE*/
   16, /*UR_TYPE_IP*/
   6, /*UR_TYPE_MAC*/
   8, /*UR_TYPE_TIME*/

   // arrays
   -1, /*UR_TYPE_A_UINT8*/
   -1, /*UR_TYPE_A_INT8*/
   -2, /*UR_TYPE_A_UINT16*/
   -2, /*UR_TYPE_A_INT16*/
   -4, /*UR_TYPE_A_UINT32*/
   -4, /*UR_TYPE_A_INT32*/
   -8, /*UR_TYPE_A_UINT64*/
   -8, /*UR_TYPE_A_INT64*/
   -4, /*UR_TYPE_A_FLOAT*/
   -8, /*UR_TYPE_A_DOUBLE*/
   -16, /*UR_TYPE_A_IP*/
   -6, /*UR_TYPE_A_MAC*/
   -8, /*UR_TYPE_A_TIME*/
};

/**
 * \brief UniRec data types.
 *
 * Sizes of data types are defined in the #ur_field_type_size array.
 */
const char *ur_field_type_str[] = {
   "string", /*UR_TYPE_STRING*/
   "bytes", /*UR_TYPE_BYTES*/
   "char", /*UR_TYPE_CHAR*/
   "uint8", /*UR_TYPE_UINT8*/
   "int8", /*UR_TYPE_INT8*/
   "uint16", /*UR_TYPE_UINT16*/
   "int16", /*UR_TYPE_INT16*/
   "uint32", /*UR_TYPE_UINT32*/
   "int32", /*UR_TYPE_INT32*/
   "uint64", /*UR_TYPE_UINT64*/
   "int64", /*UR_TYPE_INT64*/
   "float", /*UR_TYPE_FLOAT*/
   "double", /*UR_TYPE_DOUBLE*/
   "ipaddr", /*UR_TYPE_IP*/
   "macaddr", /*UR_TYPE_MAC*/
   "time", /*UR_TYPE_TIME*/
   "uint8*", /*UR_TYPE_A_UINT8*/
   "int8*", /*UR_TYPE_A_INT8*/
   "uint16*", /*UR_TYPE_A_UINT16*/
   "int16*", /*UR_TYPE_A_INT16*/
   "uint32*", /*UR_TYPE_A_UINT32*/
   "int32*", /*UR_TYPE_A_INT32*/
   "uint64*", /*UR_TYPE_A_UINT64*/
   "int64*", /*UR_TYPE_A_INT64*/
   "float*", /*UR_TYPE_A_FLOAT*/
   "double*", /*UR_TYPE_A_DOUBLE*/
   "ipaddr*", /*UR_TYPE_A_IP*/
   "macaddr*", /*UR_TYPE_A_MAC*/
   "time*", /*UR_TYPE_A_TIME*/
};

/**
 * \brief UniRec array element data types.
 */
int ur_field_array_elem_type[] = {
   UR_TYPE_STRING, /* UR_TYPE_STRING */
   UR_TYPE_BYTES, /* UR_TYPE_BYTES */
   UR_TYPE_CHAR, /* UR_TYPE_CHAR */
   UR_TYPE_UINT8, /* UR_TYPE_UINT8 */
   UR_TYPE_INT8, /* UR_TYPE_INT8 */
   UR_TYPE_UINT16, /* UR_TYPE_UINT16 */
   UR_TYPE_INT16, /* UR_TYPE_INT16 */
   UR_TYPE_UINT32, /* UR_TYPE_UINT32 */
   UR_TYPE_INT32, /* UR_TYPE_INT32 */
   UR_TYPE_UINT64, /* UR_TYPE_UINT64 */
   UR_TYPE_INT64, /* UR_TYPE_INT64 */
   UR_TYPE_FLOAT, /* UR_TYPE_FLOAT */
   UR_TYPE_DOUBLE, /* UR_TYPE_DOUBLE */
   UR_TYPE_IP, /* UR_TYPE_IP */
   UR_TYPE_MAC, /* UR_TYPE_MAC */
   UR_TYPE_TIME, /* UR_TYPE_TIME */
   UR_TYPE_UINT8, /* UR_TYPE_A_UINT8 */
   UR_TYPE_INT8, /* UR_TYPE_A_INT8 */
   UR_TYPE_UINT16, /* UR_TYPE_A_UINT16 */
   UR_TYPE_INT16, /* UR_TYPE_A_INT16 */
   UR_TYPE_UINT32, /* UR_TYPE_A_UINT32 */
   UR_TYPE_INT32, /* UR_TYPE_A_INT32 */
   UR_TYPE_UINT64, /* UR_TYPE_A_UINT64 */
   UR_TYPE_INT64, /* UR_TYPE_A_INT64 */
   UR_TYPE_FLOAT, /* UR_TYPE_A_FLOAT */
   UR_TYPE_DOUBLE, /* UR_TYPE_A_DOUBLE */
   UR_TYPE_IP, /* UR_TYPE_A_IP */
   UR_TYPE_MAC, /* UR_TYPE_A_MAC */
   UR_TYPE_TIME, /* UR_TYPE_A_TIME */
};


ur_field_specs_t ur_field_specs;
ur_static_field_specs_t UR_FIELD_SPECS_STATIC;

const char UR_MEMORY_ERROR[] = "Memory allocation error";

int ur_init(ur_static_field_specs_t field_specs_static)
{
   int i, j;
   if (ur_field_specs.intialized == UR_INITIALIZED) {
      return UR_OK;
   }
   //copy size
   ur_field_specs.ur_last_statically_defined_id = field_specs_static.ur_last_id;
   ur_field_specs.ur_last_id = field_specs_static.ur_last_id;
   ur_field_specs.ur_allocated_fields = field_specs_static.ur_last_id + UR_INITIAL_SIZE_FIELDS_TABLE;
   //copy field type
   ur_field_specs.ur_field_types = (ur_field_type_t *) calloc(sizeof(ur_field_type_t), ur_field_specs.ur_allocated_fields);
   if (ur_field_specs.ur_field_types == NULL) {
      return UR_E_MEMORY;
   }
   memcpy(ur_field_specs.ur_field_types, field_specs_static.ur_field_types, sizeof(ur_field_type_t) * field_specs_static.ur_last_id);
   //copy field sizes
   ur_field_specs.ur_field_sizes = (short *) calloc(sizeof(short), ur_field_specs.ur_allocated_fields);
   if (ur_field_specs.ur_field_sizes == NULL) {
      free(ur_field_specs.ur_field_types);
      return UR_E_MEMORY;
   }
   memcpy(ur_field_specs.ur_field_sizes, field_specs_static.ur_field_sizes, sizeof(short) * field_specs_static.ur_last_id);
   //copy field names
   ur_field_specs.ur_field_names = (char **) calloc(sizeof(char *), ur_field_specs.ur_allocated_fields);
   if (ur_field_specs.ur_field_names == NULL) {
      free(ur_field_specs.ur_field_types);
      free(ur_field_specs.ur_field_sizes);
      return UR_E_MEMORY;
   }
   for (i = 0; i < field_specs_static.ur_last_id; i++) {
      ur_field_specs.ur_field_names[i] = (char *) calloc(sizeof(char), strlen(field_specs_static.ur_field_names[i]) + 1);
      if (ur_field_specs.ur_field_names[i] == NULL) {
         free(ur_field_specs.ur_field_types);
         free(ur_field_specs.ur_field_sizes);
         for (j = 0; j < i; j++) {
            free(ur_field_specs.ur_field_names[j]);
         }
         free(ur_field_specs.ur_field_names);
         return UR_E_MEMORY;
      }
      strcpy(ur_field_specs.ur_field_names[i], field_specs_static.ur_field_names[i]);
   }
   ur_field_specs.intialized = UR_INITIALIZED;
   return UR_OK;
}

char *ur_template_string_delimiter(const ur_template_t *tmplt, int delimiter)
{
   char *str = NULL, *strmove = NULL, *str_new = NULL;
   int len = UR_DEFAULT_LENGTH_OF_TEMPLATE, act_len = 0;

   if (tmplt == NULL) {
      return NULL;
   }

   str = (char *) calloc(sizeof(char), len);
   if (str == NULL) {
      return NULL;
   }
   strmove = str;
   for (int i = 0; i < tmplt->count; i++) {
      act_len += strlen(ur_field_type_str[ur_get_type(tmplt->ids[i])]) + strlen(ur_get_name(tmplt->ids[i])) + 2;
      if (act_len >= len) {
         len *= 2;
         str_new = (char *) realloc(str, sizeof(char) * len);
         if (str_new == NULL) {
            free(str);
            return NULL;
         }
         strmove = str_new + (strmove - str);
         str = str_new;
      }
      sprintf(strmove, "%s %s%c", ur_field_type_str[ur_get_type(tmplt->ids[i])], ur_get_name(tmplt->ids[i]), delimiter);
      strmove += strlen(strmove);
   }
   if (tmplt->count != 0) {
      strmove[-1] = '\0';
   }
   return str;
}

int ur_get_empty_id()
{
   ur_field_id_linked_list_t * first;
   //check if UniRec is initialized, if not initialize it
   if (ur_field_specs.intialized != UR_INITIALIZED) {
      int init_val = ur_init(UR_FIELD_SPECS_STATIC);
      if (init_val != UR_OK) {
         return init_val;
      }
   }
   //check undefined fields
   if (ur_field_specs.ur_undefine_fields != NULL) {
      //resuse old undefined fields
      int id;
      first = ur_field_specs.ur_undefine_fields;
      ur_field_specs.ur_undefine_fields = ur_field_specs.ur_undefine_fields->next;
      id = first->id;
      free(first);
      return id;
   } else {
      //take new id
      if (ur_field_specs.ur_last_id < ur_field_specs.ur_allocated_fields) {
         //take value from remaining space
         return ur_field_specs.ur_last_id++;
      } else if (ur_field_specs.ur_last_id < UR_FIELD_ID_MAX) {
         //increase space for fields
         int new_size;

         char **ur_field_names_new;
         short *ur_field_sizes_new;
         ur_field_type_t *ur_field_types_new;

         new_size = ur_field_specs.ur_allocated_fields * 2 < UR_FIELD_ID_MAX ? ur_field_specs.ur_allocated_fields * 2 : UR_FIELD_ID_MAX;

         //copy field type
         ur_field_types_new = (ur_field_type_t *) realloc(ur_field_specs.ur_field_types, sizeof(ur_field_type_t) * new_size);
         if (ur_field_types_new == NULL) {
            return UR_E_MEMORY;
         }

         //copy field sizes
         ur_field_sizes_new = (short *) realloc(ur_field_specs.ur_field_sizes, sizeof(short) * new_size);
         if (ur_field_sizes_new == NULL) {
            free(ur_field_types_new);
            return UR_E_MEMORY;
         }

         //copy field names
         ur_field_names_new = (char **) realloc(ur_field_specs.ur_field_names, sizeof(char *) * new_size);
         if (ur_field_names_new == NULL) {
            free(ur_field_types_new);
            free(ur_field_sizes_new);
            return UR_E_MEMORY;
         }
         //replace for new values
         ur_field_specs.ur_field_names = ur_field_names_new;
         ur_field_specs.ur_field_sizes = ur_field_sizes_new;
         ur_field_specs.ur_field_types = ur_field_types_new;
         ur_field_specs.ur_allocated_fields = new_size;
         return ur_field_specs.ur_last_id++;
      } else {
         //no more space for new fields
         return UR_E_MEMORY;
      }
   }
}

int ur_get_field_type_from_str(const char *type)
{
   if (type == NULL) {
      return UR_E_INVALID_TYPE;
   }
   for (int i = 0; i < UR_COUNT_OF_TYPES; i++) {
      if (strcmp(type, ur_field_type_str[i]) == 0) {
         return i;
      }
   }
   return UR_E_INVALID_TYPE;
}

const char *ur_get_type_and_name_from_string(const char *source, char **name, char **type, int *length_name, int *length_type)
{
   int length_type_2 = 0, length_name_2 = 0;
   const char *source_cpy;
   /* skip white spaces */
   while (*source != 0 && isspace(*source)) {
      source++;
   }
   /* start of type */
   source_cpy = source;
   while (*source != 0 && !isspace(*source)) {
      length_type_2++;
      source++;
   }
   /* end of type */

   /* copy "type" string (realloc destination if needed) */
   if (length_type_2 >= *length_type) {
      if (*type != NULL) {
         free(*type);
      }
      *type = (char *) malloc(sizeof(char) * (length_type_2 + 1));
      if (*type == NULL) {
         return NULL;
      }
      *length_type = length_type_2 + 1;
   }
   memcpy(*type, source_cpy, length_type_2);
   (*type)[length_type_2] = 0;

   /* skip white spaces */
   while (*source != 0 && isspace(*source)) {
      source++;
   }
   /* start of name */
   source_cpy = source;
   while (*source != 0 && !isspace(*source) && *source != ',') {
      length_name_2++;
      source++;
   }
   /* end of name */

   /* copy "name" string (realloc destination if needed) */
   if (length_name_2 >= *length_name) {
      if (*name != NULL) {
         free(*name);
      }
      *name = (char *) malloc(sizeof(char) * (length_name_2 + 1));
      if (*name == NULL) {
         return NULL;
      }
      *length_name = length_name_2 + 1;
   }
   memcpy(*name, source_cpy, length_name_2);
   (*name)[length_name_2] = 0;
   /* skip white spaces */
   while (*source != 0 && isspace(*source)) {
      source++;
   }
   /* skip comma */
   if (*source == ',') {
      source++;
   }
   return source;
}

char *ur_ifc_data_fmt_to_field_names(const char *ifc_data_fmt)
{
   const char *source_cpy = NULL, *p = ifc_data_fmt;
   char *out_str;
   int name_len = 0, act_len = 0, str_len;
   str_len = strlen(ifc_data_fmt);
   out_str = (char *) calloc(str_len + 1, sizeof(char));
   if (out_str == NULL) {
      return NULL;
   }
   while (*p != 0) {
      /* skip white spaces */
      while (*p != 0 && isspace(*p)) {
         p++;
      }
      /* field type */
      while (*p != 0 && *p != ' ') {
         p++;
      }
      /* skip white spaces */
      while (*p != 0 && isspace(*p)) {
         p++;
      }

      //copy name
      source_cpy = p;
      name_len = 0;
      while (*p != 0 && *p != ',' && !isspace(*p)) {
         name_len++;
         p++;
      }
      assert(name_len + act_len + 1 <= str_len);
      memcpy(out_str + act_len, source_cpy, name_len);
      act_len += name_len;
      /* skip white spaces */
      while (*p != 0 && isspace(*p)) {
         p++;
      }
      if (*p == ',') {
         p++;
      } else if (*p == 0) {
         break;
      } else {
         free(out_str);
         return NULL; /* name must be followed by a comma or end of string */
      }
      out_str[act_len] = ',';
      act_len++;
   }
   return out_str;
}

ur_template_t *ur_expand_template(const char *ifc_data_fmt, ur_template_t *tmplt)
{
   int name_len = 0, act_len = 0, concat_str_len = strlen(ifc_data_fmt);
   char *concat_str;
   const char *source_cpy, *p = ifc_data_fmt;
   ur_tmplt_direction direction = UR_TMPLT_DIRECTION_NO;
   uint32_t ifc_out = 0;
   concat_str = (char *) malloc(sizeof(char) * concat_str_len);
   if (concat_str == NULL) {
      return NULL;
   }
   while (*p != 0) {
      while (*p != 0 && !isspace(*p)) {
         p++;
      }
      p++;
      //copy name
      source_cpy = p;
      name_len = 0;
      while (*p != 0 && *p != ',') {
         name_len++;
         p++;
      }
      if (name_len + act_len + 1 > concat_str_len) {
         char *str_new;
         str_new = (char *) realloc(concat_str, sizeof(char) * (concat_str_len * 2));
         if (str_new == NULL) {
            /* XXX memory leak original concat_str? */
            return NULL;
         }
         concat_str_len *= 2;
         concat_str = str_new;
      }
      memcpy(concat_str + act_len, source_cpy, name_len);
      act_len += name_len;
      concat_str[act_len] = ',';
      act_len++;
   }
   if (tmplt != NULL) {
      direction = tmplt->direction;
      ifc_out = tmplt->ifc_out;
      for (int i = 0; i < tmplt->count; i++) {
         const char *f_name = ur_get_name(tmplt->ids[i]);
         name_len = strlen(f_name);
         if (name_len + act_len + 1 > concat_str_len) {
            char *str_new;
            str_new = (char *) realloc(concat_str, sizeof(char) * (concat_str_len * 2));
            if (str_new == NULL) {
               /* XXX memory leak original concat_str? */
               return NULL;
            }
            concat_str_len *= 2;
            concat_str = str_new;
         }
         memcpy(concat_str + act_len, f_name, name_len);
         act_len += name_len;
         *(concat_str + act_len) = ',';
         act_len++;
      }
      ur_free_template(tmplt);
   }
   if (act_len > 0) {
      act_len--;
      concat_str[act_len] = 0;
   }
   tmplt = ur_create_template(concat_str, NULL);
   tmplt->direction = direction;
   tmplt->ifc_out = ifc_out;
   free(concat_str);
   return tmplt;
}

int ur_define_set_of_fields(const char *ifc_data_fmt)
{
   const char *new_fields_move;
   new_fields_move = ifc_data_fmt;
   char *field_name, *field_type;
   int field_name_length = UR_DEFAULT_LENGTH_OF_FIELD_NAME, field_type_length = UR_DEFAULT_LENGTH_OF_FIELD_TYPE;
   int field_id = 0, field_type_id = 0;
   field_name = (char *) malloc(sizeof(char) * field_name_length);
   if (field_name == NULL) {
      return UR_E_MEMORY;
   }
   field_type = (char *) malloc(sizeof(char) * field_type_length);
   if (field_type == NULL) {
      free(field_name);
      return UR_E_MEMORY;
   }
   while (*new_fields_move != 0) {
      new_fields_move = ur_get_type_and_name_from_string(new_fields_move, &field_name, &field_type, &field_name_length, &field_type_length);
      if (new_fields_move == NULL) {
         if (field_name != NULL) {
            free(field_name);
         }
         if (field_type != NULL) {
            free(field_type);
         }
         return UR_E_MEMORY;
      }
      //through all fields of receiver
      field_type_id = ur_get_field_type_from_str(field_type);
      if (field_type_id < 0) {
         if (field_name != NULL) {
            free(field_name);
         }
         free(field_type);
         return field_type_id;
      }
      field_id = ur_define_field(field_name, field_type_id);
      if (field_id < 0) {
         if (field_name != NULL) {
            free(field_name);
         }
         free(field_type);
         return field_id;
      }
   }
   if (field_name != NULL) {
      free(field_name);
   }
   free(field_type);
   return UR_OK;
}

ur_template_t *ur_define_fields_and_update_template(const char *ifc_data_fmt, ur_template_t *tmplt)
{
   ur_template_t *new_tmplt;
   if (ur_define_set_of_fields(ifc_data_fmt) < 0) {
      return NULL;
   }
   new_tmplt = ur_create_template_from_ifc_spec(ifc_data_fmt);
   if (new_tmplt != NULL && tmplt != NULL) {
      new_tmplt->ifc_out = tmplt->ifc_out;
      new_tmplt->direction = tmplt->direction;
      ur_free_template(tmplt);
   }
   return new_tmplt;
}

ur_template_t *ur_create_template_from_ifc_spec(const char *ifc_data_fmt)
{
   char *field_names = ur_ifc_data_fmt_to_field_names(ifc_data_fmt);
   if (field_names == NULL) {
      return NULL;
   }
   ur_template_t *new_tmplt = ur_create_template(field_names, NULL);
   free(field_names);
   return new_tmplt;
}

int ur_define_field(const char *name, ur_field_type_t type)
{
   int insert_id;
   char * name_copy;
   int name_len;
   if (name == NULL) {
      return UR_E_INVALID_NAME;
   }
   //check the regural expression of a name
   name_len = strlen(name);
   if (name_len == 0) {
      return UR_E_INVALID_NAME;
   }
   if (!((name[0] >= 'A' && name[0] <= 'Z') || (name[0] >= 'a' && name[0] <= 'z'))) {
      return UR_E_INVALID_NAME;
   }
   for (int i = 1; i < name_len; i++) {
      if (!((name[i] >= 'A' && name[i] <= 'Z') || (name[i] >= 'a' && name[i] <= 'z') || (name[i] >= '0' && name[i] <= '9') || name[i] == '_')) {
         return UR_E_INVALID_NAME;
      }
   }
   // If this is the first dynamically allocated field, call ur_init
   if (ur_field_specs.ur_allocated_fields == ur_field_specs.ur_last_statically_defined_id) {
      int init_val = ur_init(UR_FIELD_SPECS_STATIC);
      if (init_val != 0) {
         return init_val;
      }
   }
   //check if the field is already defined
   for (int i = 0; i < ur_field_specs.ur_last_id; i++) {
      if (ur_field_specs.ur_field_names[i] != NULL && strcmp(name, ur_field_specs.ur_field_names[i]) == 0) {
         if (type == ur_field_specs.ur_field_types[i]) {
            //name exists and type is equal
            return i;
         } else {
            //name exists, but type is different
            return UR_E_TYPE_MISMATCH;
         }
      }
   }
   //create new field
   name_copy = (char *) calloc(sizeof(char), strlen(name) + 1);
   if (name_copy == NULL) {
      //error during allocation
      return UR_E_MEMORY;
   }
   strcpy(name_copy, name);
   insert_id = ur_get_empty_id();
   if (insert_id < 0) {
      //error
      free(name_copy);
      return insert_id;
   }
   ur_field_specs.ur_field_names[insert_id] = name_copy;
   ur_field_specs.ur_field_sizes[insert_id] = ur_size_of(type);
   ur_field_specs.ur_field_types[insert_id] = type;
   return insert_id;
}

int ur_undefine_field_by_id(ur_field_id_t field_id)
{
   if (field_id < ur_field_specs.ur_last_statically_defined_id || field_id >= ur_field_specs.ur_last_id) {
      //id is invalid
      return UR_E_INVALID_PARAMETER;
   } else if (ur_field_specs.ur_field_names[field_id] == NULL) {
      //ID is already undefined
      return UR_E_INVALID_PARAMETER;
   } else {
      //undefine field
      ur_field_id_linked_list_t *undefined_item;
      undefined_item = (ur_field_id_linked_list_t *) calloc(sizeof(ur_field_id_linked_list_t), 1);
      if (undefined_item == NULL) {
         //error during allocation
         return UR_E_MEMORY;
      }
      free(ur_field_specs.ur_field_names[field_id]);
      ur_field_specs.ur_field_names[field_id] = NULL;
      undefined_item->id = field_id;
      undefined_item->next = ur_field_specs.ur_undefine_fields;
      ur_field_specs.ur_undefine_fields = undefined_item;
   }
   return UR_OK;
}

int ur_undefine_field(const char *name)
{
   int i;
   //find id of field
   for (i = ur_field_specs.ur_last_statically_defined_id; i < ur_field_specs.ur_last_id; i++) {
      if (ur_field_specs.ur_field_names[i] != NULL && strcmp(name, ur_field_specs.ur_field_names[i]) == 0) {
         return ur_undefine_field_by_id(i);
      }
   }
   //field with given name was not found
   return  UR_E_INVALID_NAME;
}


void ur_finalize()
{
   if (ur_field_specs.intialized != UR_INITIALIZED) {
      //there is no need for deallocation, because nothing has been allocated.
      return;
   }
   if (ur_field_specs.ur_field_names != NULL) {
      for (int i=0; i < ur_field_specs.ur_last_id; i++) {
         if (ur_field_specs.ur_field_names[i] != NULL) {
            free(ur_field_specs.ur_field_names[i]);
         }
      }
      free(ur_field_specs.ur_field_names);
   }
   if (ur_field_specs.ur_undefine_fields != NULL) {
      ur_field_id_linked_list_t *next, * act_del;
      act_del = ur_field_specs.ur_undefine_fields;
      while (act_del != NULL) {
         next = act_del->next;
         free(act_del);
         act_del = next;
      }
   }
   if (ur_field_specs.ur_field_sizes != NULL) {
      free(ur_field_specs.ur_field_sizes);
   }
   if (ur_field_specs.ur_field_types != NULL) {
      free(ur_field_specs.ur_field_types);
   }
   ur_field_specs.ur_field_names = UR_FIELD_SPECS_STATIC.ur_field_names;
   ur_field_specs.ur_field_sizes = UR_FIELD_SPECS_STATIC.ur_field_sizes;
   ur_field_specs.ur_field_types = UR_FIELD_SPECS_STATIC.ur_field_types;
   ur_field_specs.ur_last_statically_defined_id = UR_FIELD_SPECS_STATIC.ur_last_id;
   ur_field_specs.ur_last_id = UR_FIELD_SPECS_STATIC.ur_last_id;
   ur_field_specs.ur_allocated_fields = UR_FIELD_SPECS_STATIC.ur_last_id;
   ur_field_specs.ur_undefine_fields = NULL;
   ur_field_specs.intialized = UR_UNINITIALIZED;
}

// Find field ID given its name
int ur_get_id_by_name(const char *name)
{
   for (int id = 0; id < ur_field_specs.ur_last_id; id++) {
      if (ur_field_specs.ur_field_names[id] != NULL && strcmp(name, ur_field_specs.ur_field_names[id]) == 0) {
         return id;
      }
   }
   return UR_E_INVALID_NAME;
}

// Return -1 if f1 should go before f2, 0 if f1 is the same as f2, 1 otherwise
int compare_fields(const void *field1, const void *field2)
{
   const field_spec_t *f1 = field1;
   const field_spec_t *f2 = field2;
   if (f1->size > f2->size) {
      return -1;
   } else if (f1->size < f2->size) {
      return 1;
   } else {
      return strcmp(f1->name, f2->name);
   }
}

ur_template_t *ur_ctx_create_input_template(trap_ctx_t *ctx, int ifc, const char *fields, char **errstr)
{
   ur_template_t *tmplt = ur_create_template(fields, errstr);
   if (tmplt == NULL) {
      return NULL;
   }
   if (ur_ctx_set_input_template(ctx, ifc, tmplt) != UR_OK) {
      if (errstr != NULL) {
         *errstr = (char *) malloc(strlen(UR_MEMORY_ERROR) + 1);
         if (*errstr != NULL) {
            strcpy(*errstr, UR_MEMORY_ERROR);
         }
      }
      ur_free_template(tmplt);
      return NULL;
   }
   return tmplt;
}

ur_template_t *ur_ctx_create_output_template(trap_ctx_t *ctx, int ifc, const char *fields, char **errstr)
{
   ur_template_t *tmplt = ur_create_template(fields, errstr);
   if (tmplt == NULL) {
      return NULL;
   }
   if (ur_ctx_set_output_template(ctx, ifc, tmplt) != UR_OK) {
      if (errstr != NULL) {
         *errstr = (char *) malloc(strlen(UR_MEMORY_ERROR) + 1);
         if (*errstr != NULL) {
            strcpy(*errstr, UR_MEMORY_ERROR);
         }
      }
      ur_free_template(tmplt);
      return NULL;
   }
   return tmplt;
}

int ur_ctx_set_output_template(trap_ctx_t *ctx, int ifc, ur_template_t *tmplt)
{
   if (tmplt == NULL) {
      return UR_OK;
   }
   if (tmplt->direction == UR_TMPLT_DIRECTION_IN) {
      tmplt->direction = UR_TMPLT_DIRECTION_BI;
   } else {
      tmplt->direction = UR_TMPLT_DIRECTION_OUT;
   }
   tmplt->ifc_out = ifc;
   char * tmplt_str = ur_template_string(tmplt);
   if (tmplt_str == NULL) {
      return UR_E_MEMORY;
   }
   trap_ctx_set_data_fmt(ctx, ifc, TRAP_FMT_UNIREC, tmplt_str);
   free(tmplt_str);
   return UR_OK;
}

int ur_ctx_set_input_template(trap_ctx_t *ctx, int ifc, ur_template_t *tmplt)
{
   if (tmplt == NULL) {
      return UR_OK;
   }
   if (tmplt->direction == UR_TMPLT_DIRECTION_OUT) {
      tmplt->direction = UR_TMPLT_DIRECTION_BI;
   } else {
      tmplt->direction = UR_TMPLT_DIRECTION_IN;
   }
   char * tmplt_str = ur_template_string(tmplt);
   if (tmplt_str == NULL) {
      return UR_E_MEMORY;
   }
   trap_ctx_set_required_fmt(ctx, ifc, TRAP_FMT_UNIREC, tmplt_str);
   free(tmplt_str);
   return UR_OK;
}

ur_template_t *ur_ctx_create_bidirectional_template(trap_ctx_t *ctx, int ifc_in, int ifc_out, const char *fields, char **errstr)
{
   ur_template_t *tmplt = ur_create_template(fields, errstr);
   if (tmplt == NULL) {
      return NULL;
   }
   tmplt->direction = UR_TMPLT_DIRECTION_BI;
   tmplt->ifc_out = ifc_out;
   char * tmplt_str = ur_template_string(tmplt);
   if (tmplt_str == NULL) {
      if (errstr != NULL) {
         *errstr = (char *) malloc(strlen(UR_MEMORY_ERROR) + 1);
         if (*errstr != NULL) {
            strcpy(*errstr, UR_MEMORY_ERROR);
         }
      }
      ur_free_template(tmplt);
      return NULL;
   }
   trap_ctx_set_required_fmt(ctx, ifc_in, TRAP_FMT_UNIREC, tmplt_str);
   trap_ctx_set_data_fmt(ctx, ifc_out, TRAP_FMT_UNIREC, tmplt_str);
   free(tmplt_str);
   return tmplt;
}

ur_template_t *ur_create_template(const char *fields, char **errstr)
{
   // Count number of fields
   int n_fields = 0, written_fields = 0;
   if (fields) {
      /* skip leading spaces */
      while (*fields != '\0' && isspace(*fields)) {
         fields++;
      }
      /* Count number of fields */
      if (*fields != '\0') {
         n_fields = 1;
         const char *tmp = fields;
         while (*tmp != '\0') {
            if (*(tmp++) == ',') {
               n_fields++;
            }
         }
      }
   }
   // Allocate array of field_spec structs
   field_spec_t *fields_spec = malloc(n_fields * sizeof(field_spec_t));
   if (fields_spec == NULL && n_fields > 0) {
      if (errstr != NULL) {
         *errstr = (char *) malloc(strlen(UR_MEMORY_ERROR) + 1);
         if (*errstr != NULL) {
            strcpy(*errstr, UR_MEMORY_ERROR);
         }
      }
      return NULL;
   }
   // Parse fields and fill the array
   const char *start_ptr = fields;
   const char *end_ptr;
   for (int i = 0; i < n_fields; i++) {
      // Get field name
      end_ptr = start_ptr;
      /* go to the first space / comma / end-of-string */
      while (!isspace(*end_ptr) && *end_ptr != ',' && *end_ptr != '\0') {
         end_ptr++;
      }
      int len = end_ptr - start_ptr;
      fields_spec[written_fields].name = malloc(len + 1);
      if (fields_spec[written_fields].name == NULL) {
         if (errstr != NULL) {
            *errstr = (char *) malloc(strlen(UR_MEMORY_ERROR) + 1);
            if (*errstr != NULL) {
               strcpy(*errstr, UR_MEMORY_ERROR);
            }
         }
         for (int j = 0; j < i; j++) {
            free(fields_spec[j].name);
         }
         free(fields_spec);
         return NULL;
      }
      memcpy(fields_spec[written_fields].name, start_ptr, len);
      fields_spec[written_fields].name[len] = 0;
      start_ptr = end_ptr;
      while ((isspace(*start_ptr) || *start_ptr == ',') && *start_ptr != '\0') {
         start_ptr++;
      }
      // Get field ID
      int id_by_name = ur_get_id_by_name(fields_spec[written_fields].name);
      if (id_by_name == UR_E_INVALID_NAME) {
         // Unknown field name
         if (errstr != NULL) {
            *errstr = (char *) malloc(100);
            if (*errstr != NULL) {
               int n;
               n = snprintf(*errstr, 100, "field: %s is not defined.", fields_spec[written_fields].name);
               if (n >= 100) {
                  strcpy(*errstr, "given field is not defined");
               }
            }
         }
         for (int j = 0; j <= written_fields; j++) {
            free(fields_spec[j].name);
         }
         free(fields_spec);
         return NULL;
      }
      //check if the field is not in the template.
      int in_the_template = 0;
      for (int j = 0; j < written_fields; j++) {
         if (fields_spec[j].id == id_by_name) {
            in_the_template = 1;
            break;
         }
      }
      //if the field is not already int the template, copy values and move the index, otherwise just free the string with name.
      if (in_the_template == 0) {
         fields_spec[written_fields].id = id_by_name;
         // Get field size
         fields_spec[written_fields].size = ur_get_size(fields_spec[written_fields].id);
         written_fields++;
      } else {
         free(fields_spec[written_fields].name);
         fields_spec[written_fields].name = NULL;
      }
   }
   // Sort fields according to UniRec specification (by size and names)
   if (n_fields > 0) {
      qsort(fields_spec, written_fields, sizeof(field_spec_t), compare_fields);
   }
   // Allocate memory for the template
   ur_template_t *tmplt = (ur_template_t *) calloc(sizeof(ur_template_t), 1);
   if (tmplt == NULL) {
      for (int i = 0; i < written_fields; i++) {
         free(fields_spec[i].name);
      }
      free(fields_spec);
      if (errstr != NULL) {
         *errstr = (char *) malloc(strlen(UR_MEMORY_ERROR) + 1);
         if (*errstr != NULL) {
            strcpy(*errstr, UR_MEMORY_ERROR);
         }
      }
      return NULL;
   }
   //set no direction to the template
   tmplt->direction = UR_TMPLT_DIRECTION_NO;
   //allocate memory for offset table
   tmplt->offset_size = ur_field_specs.ur_last_id;
   tmplt->offset = malloc(ur_field_specs.ur_last_id * sizeof(uint16_t));
   if (tmplt->offset == NULL) {
      for (int i = 0; i < written_fields; i++) {
         free(fields_spec[i].name);
      }
      free(fields_spec);
      free(tmplt);
      if (errstr != NULL) {
         *errstr = (char *) malloc(strlen(UR_MEMORY_ERROR) + 1);
         if (*errstr != NULL) {
            strcpy(*errstr, UR_MEMORY_ERROR);
         }
      }
      return NULL;
   }
   // Set all fields to invalid offset
   memset(tmplt->offset, 0xff, ur_field_specs.ur_last_id * sizeof(uint16_t));
   // Fill offsets of all fields into the table
   uint16_t offset = 0;
   uint16_t first_dynamic = UR_NO_DYNAMIC_VALUES;
   for (int i = 0; i < written_fields; i++) {
      // Set offset
      if (fields_spec[i].size < 0) { // dynamic field
         tmplt->offset[fields_spec[i].id] = offset;
         offset += 4;
         if (first_dynamic == UR_NO_DYNAMIC_VALUES) {
            first_dynamic = i;
         }
      } else { // static field
         tmplt->offset[fields_spec[i].id] = offset;
         offset += fields_spec[i].size;
      }
   }
   tmplt->first_dynamic = first_dynamic;
   tmplt->static_size = offset;

   //save ids to template
   tmplt->ids = (ur_field_id_t *) malloc(sizeof(ur_field_id_t) * written_fields);
   if (tmplt->ids == NULL) {
      for (int i = 0; i < written_fields; i++) {
         free(fields_spec[i].name);
      }
      free(fields_spec);
      free(tmplt);
      if (errstr != NULL) {
         *errstr = (char *) malloc(strlen(UR_MEMORY_ERROR) + 1);
         if (*errstr != NULL) {
            strcpy(*errstr, UR_MEMORY_ERROR);
         }
      }
      return NULL;
   }
   tmplt->count = written_fields;
   for (int i = 0; i < written_fields; i++) {
      tmplt->ids[i] = fields_spec[i].id;
   }
   // Free array of field specs
   for (int i = 0; i < written_fields; i++) {
      free(fields_spec[i].name);
   }
   free(fields_spec);
   return tmplt;
}

void ur_free_template(ur_template_t *tmplt) {
   if (tmplt == NULL) {
      return;
   }
   //free offset table
   if (tmplt->offset != NULL) {
      free(tmplt->offset);
   }
   //free ids
   if (tmplt->ids != NULL) {
      free(tmplt->ids);
   }
   free(tmplt);
}

// Compare fields of two templates
int ur_template_compare(const ur_template_t *tmpltA, const ur_template_t *tmpltB)
{
   if (tmpltA->count == tmpltB->count) {
      return memcmp(tmpltA->ids, tmpltB->ids, sizeof(uint16_t) * tmpltA->count) == 0;
   } else {
      return 0;
   }
}


// Print template
void ur_print_template(ur_template_t *tmplt)
{
   printf("static_size: %hu, first_dynamic: ", tmplt->static_size);
   (tmplt->first_dynamic == UR_NO_DYNAMIC_VALUES) ? (printf("-")) : (printf("%d", tmplt->ids[tmplt->first_dynamic]));
   printf(", offsets:\n"
          "ID\t%-30s\toffset\n","name");
   for (int i = 0; i < tmplt->count; i++) {
      printf("%d\t%-30s\t%6hu\n", tmplt->ids[i], ur_field_specs.ur_field_names[tmplt->ids[i]], tmplt->offset[tmplt->ids[i]]);
   }
}

void ur_var_change_size(const ur_template_t *tmplt, void *rec, int field_id, int new_val_len)
{
   // pointer to field and size of a field
   char *out_ptr = ur_get_ptr_by_id(tmplt, rec, field_id);
   int old_size_of_field = ur_get_len(tmplt, rec, field_id);
   //if the size is different, move following fields
   if (old_size_of_field != new_val_len) {
      uint16_t size = new_val_len;
      uint16_t offset_static = ur_get_var_offset(tmplt, rec, field_id);
      int index = 0;
      //find index of changed field in record array
      for (int i = 0; i< tmplt->count; i++) {
         if (field_id == tmplt->ids[i]) {
            index = i;
         }
      }
      //set new offset for dynamic fields which are situated behind changed field
      for (int i = index + 1; i < tmplt->count; i++) {
         ur_set_var_offset(tmplt, rec, tmplt->ids[i], offset_static + size);
         size += ur_get_len(tmplt, rec, tmplt->ids[i]);
      }
      memmove(out_ptr + new_val_len, out_ptr + old_size_of_field, size - new_val_len);
      ur_set_var_len(tmplt, rec, field_id, new_val_len);
   }
}

int ur_set_var(const ur_template_t *tmplt, void *rec, int field_id, const void *val_ptr, int val_len)
{
   if (tmplt->offset[field_id] == UR_INVALID_OFFSET) {
      return UR_E_INVALID_FIELD_ID;
   }
   // wrong parameters or template does not contain dynamic fields
   if (tmplt->first_dynamic == UR_NO_DYNAMIC_VALUES || ur_is_static(field_id)) {
      return UR_E_INVALID_FIELD_ID;
   }
   // pointer to field and size of a field
   char * out_ptr = ur_get_ptr_by_id(tmplt, rec, field_id);
   //change size of a variable length field
   ur_var_change_size(tmplt, rec, field_id, val_len);
   //copy new value
   memcpy(out_ptr, val_ptr, val_len);
   return UR_OK;
}

int ur_array_resize(const ur_template_t *tmplt, void *rec, int field_id, int len)
{
   if (tmplt->offset[field_id] == UR_INVALID_OFFSET) {
      return UR_E_INVALID_FIELD_ID;
   }
   // wrong parameters or template does not contain dynamic fields
   if (tmplt->first_dynamic == UR_NO_DYNAMIC_VALUES || ur_is_static(field_id)) {
      return UR_E_INVALID_FIELD_ID;
   }
   //change size of a variable length field
   ur_var_change_size(tmplt, rec, field_id, len);
   return UR_OK;
}

char *ur_array_append_get_ptr(const ur_template_t *tmplt, void *rec, int field_id)
{
   int elem_cnt = ur_array_get_elem_cnt(tmplt, rec, field_id);
   int elem_size = ur_array_get_elem_size(field_id);
   if (ur_array_resize(tmplt, rec, field_id, (elem_cnt + 1) * elem_size) == UR_OK) {
      return (char *) ur_get_ptr_by_id(tmplt, rec, field_id) + elem_cnt * elem_size;
   } else {
      return NULL;
   }
}

void ur_clear_varlen(const ur_template_t * tmplt, void *rec)
{
   //set null offset and length for all dynamic fields
   for (int i = tmplt->first_dynamic; i < tmplt->count; i++) {
      ur_set_var_offset(tmplt, rec, tmplt->ids[i], 0);
      ur_set_var_len(tmplt, rec, tmplt->ids[i], 0);
   }
}

uint16_t ur_rec_varlen_size(const ur_template_t *tmplt, const void *rec)
{
   int size = 0;
   for (int i = tmplt->first_dynamic; i < tmplt->count; i++) {
      size += ur_get_var_len(tmplt, rec, tmplt->ids[i]);
   }
   return size;
}

// Allocate memory for UniRec record
void *ur_create_record(const ur_template_t *tmplt, uint16_t max_var_size)
{
   unsigned int size = (unsigned int)tmplt->static_size + max_var_size;
   if (size > UR_MAX_SIZE)
      size = UR_MAX_SIZE;
   return (void *) calloc(size, 1);
}

// Free UniRec record
void ur_free_record(void *record)
{
   free(record);
}

// Get dynamic field as C string (allocate, copy and append '\0')
char *ur_get_var_as_str(const ur_template_t *tmplt, const void *rec, ur_field_id_t field_id)
{
   uint16_t size = ur_get_var_len(tmplt, rec, field_id);
   char *str = malloc(size + 1);
   if (str == NULL)
      return NULL;
   if (size > 0) {
      const char *p = ur_get_ptr_by_id(tmplt, rec, field_id);
      memcpy(str, p, size);
   }
   str[size] = '\0';
   return str;
}

inline void *ur_clone_record(const ur_template_t *tmplt, const void *src)
{
    uint16_t varsize = ur_rec_varlen_size(tmplt, src);
    void *copy = ur_create_record(tmplt, varsize);
    if (copy) {
       memcpy(copy, src, ur_rec_fixlen_size(tmplt) + varsize);
    }
    return copy;
}

void ur_copy_fields(const ur_template_t *dst_tmplt, void *dst, const ur_template_t *src_tmplt, const void *src)
{
   int size_of_field = 0;
   void * ptr_dst = NULL;
   void * ptr_src = NULL;
   uint16_t size = src_tmplt->offset_size < dst_tmplt->offset_size ? src_tmplt->offset_size : dst_tmplt->offset_size;
   //Fields with same template can be copied by fully by memcpy
   if (src_tmplt == dst_tmplt) {
      memcpy(dst, src, ur_rec_size(src_tmplt, src));
      return;

   }
   // minimal value from offset table size
   for (int i = 0; i < size; i++) {
      // if two templates have the same field
      if (src_tmplt->offset[i] != UR_INVALID_OFFSET && dst_tmplt->offset[i] != UR_INVALID_OFFSET) {
         size_of_field = ur_get_size(i);
         if (size_of_field > 0) {
            // static fields
            ptr_dst = ur_get_ptr_by_id(dst_tmplt, dst, i);
            ptr_src = ur_get_ptr_by_id(src_tmplt, src, i);
            memcpy(ptr_dst, ptr_src, size_of_field);
         } else {
            // variable-size fields
            ptr_src = ur_get_ptr_by_id(src_tmplt, src, i);
            size_of_field = ur_get_var_len(src_tmplt, src, i);
            ur_set_var(dst_tmplt, dst, i, ptr_src, size_of_field);
         }
      }
   }
}

// Function for iterating over all fields in a given template
ur_iter_t ur_iter_fields(const ur_template_t *tmplt, ur_iter_t id)
{
   // Set first ID to check
   if (id == UR_ITER_BEGIN) {
      id = 0;
   } else {
      id++;
   }
   // Find first ID which is present in the template
   while (id < tmplt->offset_size) {
      if (tmplt->offset[id] != UR_INVALID_OFFSET) {
         return id;
      }
      id++;
   }
   return UR_ITER_END;
}

// Function for iterating over all fields in a given template. Fields are in the same
// order like in record
ur_iter_t ur_iter_fields_record_order(const ur_template_t *tmplt, int index)
{
   // Set first ID to check
   if (index >= tmplt->count || index < 0) {
      return UR_ITER_END;
   }
   return tmplt->ids[index];
}

int ur_set_array_from_string(const ur_template_t *tmpl, void *data, ur_field_id_t f_id, const char *v)
{
   ip_addr_t addr;
   mac_addr_t macaddr;
   int rv = 0;
   ur_time_t urtime = 0;
   void *ptr = ur_get_ptr_by_id(tmpl, data, f_id);
   int elems_parsed = 0;
   int elems_allocated = UR_ARRAY_ALLOC;
   const char *scan_format = NULL;
   const int element_size = ur_array_get_elem_size(f_id);

   if (ur_is_present(tmpl, f_id) == 0 || !ur_is_array(f_id)) {
      return 1;
   }
   if (ur_array_allocate(tmpl, data, f_id, elems_allocated) != UR_OK) {
      return 1;
   }
   while (v && *v == UR_ARRAY_DELIMITER) {
      v++; // Skip the delimiter, move to beginning of the next value
   }
   switch (ur_get_type(f_id)) {
   case UR_TYPE_A_UINT8:
      scan_format = "%" SCNu8;
      break;
   case UR_TYPE_A_UINT16:
      scan_format = "%" SCNu16;
      break;
   case UR_TYPE_A_UINT32:
      scan_format = "%" SCNu32;
      break;
   case UR_TYPE_A_UINT64:
      scan_format = "%" SCNu64;
      break;
   case UR_TYPE_A_INT8:
      scan_format = "%" SCNi8;
      break;
   case UR_TYPE_A_INT16:
      scan_format = "%" SCNi16;
      break;
   case UR_TYPE_A_INT32:
      scan_format = "%" SCNi32;
      break;
   case UR_TYPE_A_INT64:
      scan_format = "%" SCNi64;
      break;
   case UR_TYPE_A_FLOAT:
      scan_format = "%f";
      break;
   case UR_TYPE_A_DOUBLE:
      scan_format = "%lf";
      break;
   case UR_TYPE_A_IP:
      // IP address - convert to human-readable format
      while (v && *v) {
         char tmp[64];
         const char *ip = tmp;
         char *end;
         end = strchr(v, UR_ARRAY_DELIMITER);
         if (end == NULL) {
            ip = v;
            if (*v == 0) {
               break;
            }
         } else {
            memcpy(tmp, v, end - v);
            tmp[end - v] = 0;
         }
         v = end;
         if (ip_from_str(ip, &addr) == 0) {
            rv = 1;
            break;
         }
         ((ip_addr_t *) ptr)[elems_parsed] = addr;
         elems_parsed++;
         if (elems_parsed >= elems_allocated) {
            elems_allocated += UR_ARRAY_ALLOC;
            if (ur_array_allocate(tmpl, data, f_id, elems_allocated) != UR_OK) {
               return 1;
            }
         }
         while (v && *v == UR_ARRAY_DELIMITER) {
            v++; // Skip the delimiter, move to beginning of the next value
         }
      }
      break;
   case UR_TYPE_A_MAC:
      // MAC address - convert to human-readable format
      while (v && *v) {
         if (mac_from_str(v, &macaddr) == 0) {
            rv = 1;
            break;
         }
         ((mac_addr_t *) ptr)[elems_parsed] = macaddr;
         elems_parsed++;
         if (elems_parsed >= elems_allocated) {
            elems_allocated += UR_ARRAY_ALLOC;
            if (ur_array_allocate(tmpl, data, f_id, elems_allocated) != UR_OK) {
               return 1;
            }
         }
         v = strchr(v, UR_ARRAY_DELIMITER);
         while (v && *v == UR_ARRAY_DELIMITER) {
            v++; // Skip the delimiter, move to beginning of the next value
         }
      }
      break;
   case UR_TYPE_A_TIME:
      // Timestamp - convert from human-readable format
      while (v && *v) {
         char tmp[64];
         const char *time = tmp;
         char *end;
         end = strchr(v, UR_ARRAY_DELIMITER);
         if (end == NULL) {
            time = v;
            if (*v == 0) {
               break;
            }
         } else {
            memcpy(tmp, v, end - v);
            tmp[end - v] = 0;
         }
         if (ur_time_from_string(&urtime, time) != 0) {
            rv =  1;
            break;
         }
         ((ur_time_t *) ptr)[elems_parsed] = urtime;
         elems_parsed++;
         if (elems_parsed >= elems_allocated) {
            elems_allocated += UR_ARRAY_ALLOC;
            if (ur_array_allocate(tmpl, data, f_id, elems_allocated) != UR_OK) {
               return 1;
            }
         }
         v = strchr(v, UR_ARRAY_DELIMITER);
         while (v && *v == UR_ARRAY_DELIMITER) {
            v++; // Skip the delimiter, move to beginning of the next value
         }
      }
      break;
   default:
      fprintf(stderr, "Unsupported UniRec field type, skipping.\n");
      ur_array_allocate(tmpl, data, f_id, 0);
      break;
   }

   if (scan_format != NULL) {
      while (v && *v) {
         if (sscanf(v, scan_format, (void *) ((char*) ptr + elems_parsed * element_size)) != 1) {
            rv = 1;
            break;
         }
         elems_parsed++;
         if (elems_parsed >= elems_allocated) {
            elems_allocated += UR_ARRAY_ALLOC;
            if (ur_array_allocate(tmpl, data, f_id, elems_allocated) != UR_OK) {
               return 1;
            }
         }
         v = strchr(v, UR_ARRAY_DELIMITER);
         while (v && *v == UR_ARRAY_DELIMITER) {
            v++; // Skip the delimiter, move to beginning of the next value
         }
      }
   }

   if (elems_allocated > elems_parsed) {
      ur_array_allocate(tmpl, data, f_id, elems_parsed);
   }
   return rv;
}
int ur_set_from_string(const ur_template_t *tmpl, void *data, ur_field_id_t f_id, const char *v)
{
   ip_addr_t *addr_p = NULL, addr;
   mac_addr_t *macaddr_p = NULL, macaddr;
   int rv = 0;
   ur_time_t urtime = 0;
   void *ptr = ur_get_ptr_by_id(tmpl, data, f_id);

   if (ur_is_present(tmpl, f_id) == 0) {
      return 1;
   }
   switch (ur_get_type(f_id)) {
   case UR_TYPE_UINT8:
      if (sscanf(v, "%" SCNu8, (uint8_t *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_UINT16:
      if (sscanf(v, "%" SCNu16 , (uint16_t *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_UINT32:
      if (sscanf(v, "%" SCNu32, (uint32_t *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_UINT64:
      if (sscanf(v, "%" SCNu64, (uint64_t *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_INT8:
      if (sscanf(v, "%" SCNi8, (int8_t *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_INT16:
      if (sscanf(v, "%" SCNi16, (int16_t *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_INT32:
      if (sscanf(v, "%" SCNi32, (int32_t *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_INT64:
      if (sscanf(v, "%" SCNi64, (int64_t *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_CHAR:
      if (sscanf(v, "%c", (char *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_FLOAT:
      if (sscanf(v, "%f", (float *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_DOUBLE:
      if (sscanf(v, "%lf", (double *) ptr) != 1) {
         rv = 1;
      }
      break;
   case UR_TYPE_IP:
      // IP address - convert to human-readable format
      if (ip_from_str(v, &addr) == 0) {
         rv = 1;
         break;
      }
      addr_p = (ip_addr_t *) ptr;
      (*addr_p) = addr;
      break;
   case UR_TYPE_MAC:
      // MAC address - convert to human-readable format
      if (mac_from_str(v, &macaddr) == 0) {
         rv = 1;
         break;
      }
      macaddr_p = (mac_addr_t *) ptr;
      (*macaddr_p) = macaddr;
      break;
   case UR_TYPE_TIME:
      // Timestamp - convert from human-readable format
      if (ur_time_from_string(&urtime, v) != 0) {
         fprintf(stderr, "Failed to parse time.\n");
      }
      (*(ur_time_t *) ptr) = urtime;
      break;
   case UR_TYPE_STRING:
      // Printable string
      ur_set_var(tmpl, data, f_id, v, strlen(v));
      break;
   case UR_TYPE_BYTES:
      {
         // Generic string of bytes
         int size = strlen(v)/2;
         ur_var_change_size(tmpl, data, f_id, size);
         unsigned char *data_ptr = ur_get_ptr_by_id(tmpl, data, f_id);
         for ( ; size > 0; --size, v += 2, ++data_ptr) {
            sscanf(v, "%2hhx", data_ptr);
         }
      }
      break;
   default:
      if (ur_is_array(f_id)) {
         return ur_set_array_from_string(tmpl, data, f_id, v);
      }
      fprintf(stderr, "Unsupported UniRec field type, skipping.\n");
      break;
   }
   return rv;
}

uint8_t ur_time_from_string(ur_time_t *ur, const char *str)
{
   struct tm t;
   time_t sec = -1;
   uint64_t nsec = 0;
   char *res = NULL;

   if (ur == NULL || str == NULL) {
      return 2;
   }

   res = strptime(str, "%Y-%m-%dT%T", &t);
   /* parsed to sec - msec delimiter */
   if ((res != NULL) && ((*res == '.') || (*res == 0) || (*res == 'z') || (*res == 'Z'))) {
      sec = timegm(&t);
      if (sec != -1) {
         if (*res != 0 && *++res != 0) {
            char frac_buffer[10];
            memset(frac_buffer, '0', 9);
            frac_buffer[9] = 0;

            // now "res" points to the beginning of the fractional part or 'Z' for UTC timezone,
            // which have at least one char.
            // Expand the number by zeros to the right to get it in ns
            // (if there are more than 9 digits, truncate the rest)
            size_t frac_len = strlen(res);
            if (frac_len > 0 && (res[frac_len - 1] == 'z' || res[frac_len - 1] == 'Z')) {
                frac_len--;
            }
            if (frac_len > 9) {
                frac_len = 9;
            }
            memcpy(frac_buffer, res, frac_len);
            nsec = strtoul(frac_buffer, NULL, 10); // returns 0 on error - that's OK
         }
         *ur = ur_time_from_sec_nsec((uint64_t) sec, nsec);
      } else {
         goto failed_time_parsing;
      }
      /* success */
      return 0;
   } else {
failed_time_parsing:
      *ur = (ur_time_t) 0;
      /* parsing error */
      return 1;
   }
}

char *ur_cpy_string(const char *str)
{
   int str_len = strlen(str) + 1;
   char *new_str = malloc(sizeof(char) * str_len);
   if (new_str == NULL) {
      return NULL;
   }
   memcpy(new_str, str, str_len);
   return new_str;
}

const char *ur_values_get_name_start_end(uint32_t start, uint32_t end, int32_t value)
{
   for (int i = start; i < end; i++) {
      if (ur_values[i].value == value) {
         return ur_values[i].name;
      }
   }
   return NULL;
}

const char *ur_values_get_description_start_end(uint32_t start, uint32_t end, int32_t value)
{
   for (int i = start; i < end; i++) {
      if (ur_values[i].value == value) {
         return ur_values[i].description;
      }
   }
   return NULL;
}
// *****************************************************************************
// ** "Links" part - set of functions for handling LINK_BIT_FIELD

// Create and initialize links structure.
ur_links_t *ur_create_links(const char *mask)
{
   uint64_t checker;
   unsigned int indexer;
   ur_links_t *lm;

   // Allocate memory for structure.
   lm = (ur_links_t *) malloc(sizeof(ur_links_t));
   if (lm == NULL) {
      return NULL;
   }

   // Try to convert passed mask in string to uint64_t.
   if (sscanf(mask, "%"SCNx64, &lm->link_mask) < 1) {
      free(lm);
      return NULL;
   }

   // Get link count.
   lm->link_count = 0;
   checker = 1;
   for (int i = 0; i < MAX_LINK_COUNT; ++i) {
      if (lm->link_mask & checker) {
         lm->link_count++;
      }
      checker <<= 1;
   }
   if (lm->link_count == 0) {
      free(lm);
      return NULL;
   }
   // Allocate array for link indexes
   lm->link_indexes = (uint64_t *) malloc(lm->link_count * sizeof(uint64_t));
   if (lm->link_indexes == NULL) {
      free(lm);
      return NULL;
   }

   // Fill link indexes
   indexer = 0;
   checker = 1;
   for (int i = 0; i < MAX_LINK_COUNT; ++i) {
      if (lm->link_mask & checker) {
         lm->link_indexes[indexer++] = i;
      }
      checker <<= 1;
   }

   return lm;
}

// Destroy links structure.
void ur_free_links(ur_links_t *links)
{
   if (links != NULL) {
      free(links->link_indexes);
      free(links);
   }
}
// Following functions are defined in links.h
// (their headers are repeated here with INLINE_IMPL to generate externally
// linkable code)

// Get index of link (0 - (link_count-1))
INLINE_IMPL int ur_get_link_index(ur_links_t *links, uint64_t link_bit_field);

// Get position in link_bit_field of link
INLINE_IMPL uint64_t ur_get_link_bit_field_position(ur_links_t *links, unsigned int index);

// Get link mask.
INLINE_IMPL uint64_t ur_get_link_mask(ur_links_t *links);

// Get link count.
INLINE_IMPL unsigned int ur_get_link_count(ur_links_t *links);

// END OF "Links" part *********************************************************
// *****************************************************************************

