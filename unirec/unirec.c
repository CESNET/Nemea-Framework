/**
 * \file unirec.c
 * \brief Definition of UniRec structures and functions
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2013
 * \date 2014
 * \date 2015
 */
/*
 * Copyright (C) 2013-2015 CESNET
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
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
/* for string to unirec conversion: */
#include <inttypes.h>

#define IP_IMPLEMENT_HERE // put externally-linkable code of inline functions
                          // defined in ip_addr.h into this translation unit
#include "unirec.h"
#undef IP_IMPLEMENT_HERE

// Include tables and functions generated automatically from "fields" file
#include "field_func.c"

// Include structures and functions for handling LINK_BIT_FIELD
#include "links.h"

typedef struct field_spec_s {
   char *name;
   int size;
   ur_field_id_t id;
} field_spec_t;

// Return -1 if f1 should go before f2, 0 if f1 is the same as f2, 1 otherwise
static int compare_fields(const void *field1, const void *field2)
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

// Return set of fields (string) which should substitute for given group name
const char *ur_get_group(const char *name)
{
   for (int i = 0; i < UR_FIELD_GROUPS_NUM; i++) {
      if (strcmp(name, UR_FIELD_GROUPS[i][0]) == 0) {
         return UR_FIELD_GROUPS[i][1];
      }
   }
   return NULL;
}


// Substitute groups in fields-spec string
static char *ur_substitute_groups(const char *fields_str)
{
   // Copy fields_str to editable string
   int f_str_len = strlen(fields_str);
   char *f_str = malloc(f_str_len + 1);
   if (f_str == NULL) {
      fprintf(stderr, "Memory allocation error.\n");
      return NULL;
   }
   strcpy(f_str, fields_str);

   // Repeat while there is some group to substitute
   int iter = 0;
   while (1) {
      // Iteration limit to avoid replacement loops
      if (++iter > 20) {
         free(f_str);
//          fprintf(stderr, "%s: Group replacement loop (iteration limit reached).\n", __FUNCTION__);
         return NULL; // Group replacement loop (iteration limit reached)
      }

      // Find start of group label ('<')
      char *start = strchr(f_str, '<');
      if (start == NULL) {
         break; // No group label found - everything substituted
      }
      // Find end of group label ('>')
      char *end = strchr(start+1, '>');
      if (end == NULL) {
         free(f_str);
//          fprintf(stderr, "%s: No matching '>'.\n", __FUNCTION__);
         return NULL; // No matching '>'
      }
      int group_label_len = end - start + 1; // Length of group label including '<' and '>'

      // Find the group specification
      *end = '\0';
      const char *group_str = ur_get_group(start + 1);
      *end = '>';
      if (group_str == NULL) {
         free(f_str);
//          fprintf(stderr, "%s: Unknown group.\n", __FUNCTION__);
         return NULL; // Unknown group
      }
      int group_str_len = strlen(group_str); // Length of group replacement string

      // Allocate new f_str
      int new_f_str_len = f_str_len - group_label_len + group_str_len;
      char *new_f_str = malloc(new_f_str_len + 1);

      // Copy contents of old f_str into the new one and substitute the group label
      char *tmp = new_f_str;
      strncpy(tmp, f_str, (start - f_str)); // Copy beginning of the f_str
      tmp += (start - f_str);
      strcpy(tmp, group_str); // Copy the group replacement string
      tmp += group_str_len;
      strcpy(tmp, end + 1); // Copy the rest of the f_str

      free(f_str);
      f_str = new_f_str;
      f_str_len = new_f_str_len;
   }

   return f_str;
}

// Find field ID given its name
ur_field_id_t ur_get_id_by_name(const char *name)
{
   for (int id = 0; id < UR_FIELDS_NUM; id++) {
      if (strcmp(name, UR_FIELD_NAMES[id]) == 0) {
         return id;
      }
   }
   return UR_INVALID_FIELD;
}

// Create UniRec template
ur_template_t *ur_create_template(const char *fields_str)
{
   // ** Substitute groups of fields **
   char *f_str = ur_substitute_groups(fields_str);
   if (f_str == NULL) {
      return NULL;
   }

   // ** Parse field specification string and create array of field_specs **

   // Count number of fields
   int n_fields = 1;
   const char *tmp = f_str;
   while (*tmp != '\0') {
      if (*(tmp++) == ',') {
         n_fields++;
      }
   }
   //fprintf(stderr, "n_fields: %i\n", n_fields);

   // Allocate array of field_spec structs
   field_spec_t *fields = malloc(n_fields * sizeof(field_spec_t));
   if (fields == NULL) {
      free(f_str);
      return NULL;
   }

   // Parse f_str and fill the array
   const char *start_ptr = f_str;
   const char *end_ptr;
   for (int i = 0; i < n_fields; i++) {
      // Get field name
      end_ptr = strchr(start_ptr, (i < n_fields-1) ? ',' : '\0'); // find next semicolon or the end of string
      int len = end_ptr - start_ptr;
      fields[i].name = malloc(len + 1);
      memcpy(fields[i].name, start_ptr, len);
      fields[i].name[len] = 0;
      start_ptr = end_ptr + 1;

      // Get field ID
      fields[i].id = ur_get_id_by_name(fields[i].name);
      if (fields[i].id == UR_INVALID_FIELD) {
         // Unknown field name
         do {
            free(fields[i].name);
         } while (i--);
         free(fields);
         free(f_str);
         return NULL;
      }

      // Get field size
      fields[i].size = ur_get_size_by_id(fields[i].id);
   }

   free(f_str);

   // Sort fields according to UniRec specification (by size and names)
   qsort(fields, n_fields, sizeof(field_spec_t), compare_fields);

//    fprintf(stderr, "\n");
//    for (int i = 0; i < n_fields; i++) {
//       fprintf(stderr, "%s %hu %i\n", fields[i].name, fields[i].id, fields[i].size);
//    }

   // ** Create a template **

   // Allocate memory for the template
   ur_template_t *tmplt = malloc(sizeof(ur_template_t));
   if (tmplt == NULL) {
      for (int i = 0; i < n_fields; i++) {
         free(fields[i].name);
      }
      free(fields);
      return NULL;
   }

   // Set all fields to invalid offset
   memset(tmplt, 0xff, sizeof(ur_template_t));
   tmplt->first_dynamic = UR_INVALID_FIELD;

   // Fill offsets of all fields into the table
   uint16_t offset = 0;
   for (int i = 0; i < n_fields; i++) {
      // Check whether offset is not already set
      if (tmplt->offset[fields[i].id] != 0xffff) {
         // Offset is already set - duplicated field
         for (int i = 0; i < n_fields; i++) {
            free(fields[i].name);
         }
         free(fields);
         free(tmplt);
         return NULL;
      }
      // Set offset
      if (fields[i].size < 0) { // dynamic field
         if (tmplt->first_dynamic == UR_INVALID_FIELD) {
            tmplt->first_dynamic = fields[i].id;
         }
         tmplt->offset[fields[i].id] = offset;
         offset += 2;
      }
      else { // static field
         tmplt->offset[fields[i].id] = offset;
         offset += fields[i].size;
      }
   }

   tmplt->static_size = offset;

   // Free array of field specs
   for (int i = 0; i < n_fields; i++) {
      free(fields[i].name);
   }
   free(fields);

   return tmplt;
}

// Print template
void ur_print_template(ur_template_t *tmplt) {
   printf("static_size: %hu, first_dynamic: ", tmplt->static_size);
   // if template does not contain any dynamic fields, prints "-"
   (tmplt->first_dynamic == UR_INVALID_FIELD) ? (printf("-")) : (printf("%hu", tmplt->first_dynamic));
   printf(", offsets:\n"
          "ID\t%-30s\toffset\n","name");
   for (int i = 0; i <= UR_MAX_FIELD_ID; i++) {
      printf("%d\t%-30s\t%6hu\n", i, UR_FIELD_NAMES[i], tmplt->offset[i]);
   }
}

// Destroy UniRec template
void ur_free_template(ur_template_t *tmplt)
{
   free(tmplt);
}

// Create a template as a union of given templates
ur_template_t *ur_union_templates(ur_template_t **templates, int n)
{
   // Allocate memory for the template
   ur_template_t *tmplt = malloc(sizeof(ur_template_t));
   if (tmplt == NULL) {
      return NULL;
   }

   // Set all fields to invalid offset
   memset(tmplt, 0xff, sizeof(ur_template_t));
   tmplt->first_dynamic = UR_INVALID_FIELD;

   // Fill offsets of all fields into the table
   uint16_t offset = 0;
   // Check for all possible fields in the order in which they are in templates
   for (int i = 0; i < UR_ALL_FIELDS_NUM; i++) {
      // Try to find this field in all templates
      int found = 0;
      for (int t = 0; t < n; t++) {
         if (templates[t]->offset[UR_ALL_FIELDS[i]] != UR_INVALID_FIELD) {
            found = 1;
            break;
         }
      }
      if (!found) {
         continue;
      }

      // Field is present in at least one template - include it in the new one

      // Set offset
      ur_field_id_t id = UR_ALL_FIELDS[i];
      int size = ur_get_size_by_id(id);
      if (size < 0) { // dynamic field
         if (tmplt->first_dynamic == UR_INVALID_FIELD) {
            tmplt->first_dynamic = id;
         }
         tmplt->offset[id] = offset;
         offset += 2;
      }
      else { // static field
         tmplt->offset[id] = offset;
         offset += size;
      }
   }

   tmplt->static_size = offset;

   return tmplt;
}


// Function for iterating over all fields in a given template
ur_field_id_t ur_iter_fields(const ur_template_t *tmplt, ur_field_id_t id)
{
   // Set first ID to check
   if (id == UR_INVALID_FIELD) {
      id = 0;
   } else {
      id++;
   }
   // Find first ID which is present in the template
   while (id <= UR_MAX_FIELD_ID) {
      if (tmplt->offset[id] != UR_INVALID_OFFSET) {
         return id;
      }
      id++;
   }
   return UR_INVALID_FIELD;
}

// Function for iterating over all fields in a given template
ur_field_id_t ur_iter_fields_tmplt(const ur_template_t *tmplt, ur_iter_t *iter)
{
   // Go through IDs in UR_ALL_FIELDS (these are sorted by the same order as in records)
   while (*iter < UR_ALL_FIELDS_NUM) {
      // find next field vith valid offset
      if (tmplt->offset[UR_ALL_FIELDS[*iter]] != UR_INVALID_OFFSET) {
         return UR_ALL_FIELDS[(*iter)++];
      }
      (*iter)++;
   }
   return UR_INVALID_FIELD;
}

// Allocate memory for UniRec record
void *ur_create(ur_template_t *tmplt, uint16_t dyn_size)
{
   unsigned int size = (unsigned int)tmplt->static_size + dyn_size;
   if (size > 0xffff)
      size = 0xffff;
   return malloc(size);
}

// Free UniRec record
void ur_free(void *record)
{
   free(record);
}

// Transfer static data between UniRec records with different templates
void ur_transfer_static(ur_template_t *tmplt_src, ur_template_t *tmplt_dst, const void *src, void *dst)
{
   ur_field_id_t id = UR_INVALID_FIELD;
   while ((id = ur_iter_fields(tmplt_src, id)) != UR_INVALID_FIELD) {
      if (ur_is_present(tmplt_dst, id) && !ur_is_dynamic(id)) {
         memcpy(ur_get_ptr_by_id(tmplt_dst, dst, id), ur_get_ptr_by_id(tmplt_src, src, id), ur_get_size_by_id(id));
      }
   }
}


// Get dynamic field as C string (allocate, copy and append '\0')
char *ur_get_dyn_as_str(const ur_template_t *tmplt, const void *data, ur_field_id_t field_id)
{
   uint16_t size = ur_get_dyn_size(tmplt, data, field_id);
   char *str = malloc(size + 1);
   if (str == NULL)
      return NULL;
   if (size > 0) {
      const char *p = ur_get_dyn(tmplt, data, field_id);
      memcpy(str, p, size);
   }
   str[size] = '\0';
   return str;
}

/**
 * Check if the year is leaping.
 * \param[in] year  Year to check.
 * \return 0 if the year is not leap, 1 if the year is leap.
 * \note This function was taken http://stackoverflow.com/questions/16647819/timegm-cross-platform
 * and modified.
 */
static char is_leap(uint16_t year)
{
   if (year % 400 == 0) {
      return 1;
   }
   if (year % 100 == 0) {
      return 0;
   }
   if (year % 4 == 0) {
      return 1;
   }
   return 0;
}

/**
 * Get the number of days from the year 0.
 * \param[in] year  Ending year for computation.
 * \return Number of days till the year given by parameter.
 * \note This function was taken http://stackoverflow.com/questions/16647819/timegm-cross-platform
 * and modified.
 */
static uint32_t days_from_0(int16_t year)
{
   year--;
   return 365 * year + (year / 400) - (year / 100) + (year / 4);
}

/**
 * Get the number of days from the year 1970.
 * \param[in] year  Ending year for computation.
 * \return Number of days from 1970 till the year given by parameter.
 * \note This function was taken http://stackoverflow.com/questions/16647819/timegm-cross-platform
 * and modified.
 */
static uint32_t days_from_1970(uint16_t year)
{
   uint32_t days_from_0_to_1970 = days_from_0(1970);
   return days_from_0(year) - days_from_0_to_1970;
}

/**
 * Get the number of days since the 1st of January.
 * \param[in] year  Year for computation.
 * \param[in] month  Month for computation.
 * \param[in] day  Day for computation.
 * \return Number of days since the 1st of January till the gived date.
 * \note This function was taken http://stackoverflow.com/questions/16647819/timegm-cross-platform
 * and modified.
 */
static int16_t days_from_1jan(int16_t year, int16_t month, int16_t day)
{
   static const uint16_t days[2][12] = {
      { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
      { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
   };
   return days[(int32_t) is_leap(year)][month - 1] + day - 1;
}

/**
 * Convert timestamp into seconds.
 * \param[in] t  Date and time structure.
 * \return Converted date and time.
 * \note This function was taken http://stackoverflow.com/questions/16647819/timegm-cross-platform
 * and modified.
 */
static time_t ur_timegm(const struct tm *t)
{
   int day, day_of_year, days_since_epoch;
   time_t seconds_in_day, result;
   int years_diff;
   int year = t->tm_year + 1900;
   int month = t->tm_mon;

   if (month > 11) {
      year += month / 12;
      month %= 12;
   } else if (month < 0) {
      years_diff = (-month + 11) / 12;
      year -= years_diff;
      month += 12 * years_diff;
   }

   month++;
   day = t->tm_mday;
   day_of_year = days_from_1jan(year, month, day);
   days_since_epoch = days_from_1970(year) + day_of_year;

   seconds_in_day = 3600 * 24;
   result = seconds_in_day * days_since_epoch + 3600 * t->tm_hour + 60 * t->tm_min + t->tm_sec;

   return result;
}

int ur_set_from_string(const ur_template_t *tmpl, void *data, ur_field_id_t f_id, const char *v)
{
   ip_addr_t *addr_p = NULL, addr;
   int rv = 0;
   struct tm t;
   char *res = NULL;
   time_t sec = -1;
   uint64_t msec = 0;
   void *ptr = ur_get_ptr_by_id(tmpl, data, f_id);

   if (ur_is_present(tmpl, f_id) == 0) {
      return 1;
   }
   switch (ur_get_type_by_id(f_id)) {
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
      // IP address - convert to human-readable format and print
      if (ip_from_str(v, &addr) == 0) {
         rv = 1;
         break;
      }
      addr_p = (ip_addr_t *) ptr;
      (*addr_p) = addr;
      break;
   case UR_TYPE_TIME:
      // Timestamp - convert from human-readable format
      res = strptime(v, "%FT%T", &t);
      /* parsed to sec - msec delimiter */
      if ((res != NULL) && (*res == '.')) {
         sec = ur_timegm(&t);
         if (sec != -1) {
            msec = atoi(res + 1);
            (*(ur_time_t *) ptr) = ur_time_from_sec_msec((uint64_t) sec, msec);
         } else {
            goto failed_time_parsing;
         }
      } else {
failed_time_parsing:
         (*(ur_time_t *) ptr) = (ur_time_t) 0;
         fprintf(stderr, "Failed to parse time.\n");
      }
      break;
   case UR_TYPE_STRING:
      // Printable string
      ur_set_dyn(tmpl, data, f_id, v, strlen(v));
      break;
   case UR_TYPE_BYTES:
      {
         // Generic string of bytes
         int size = strlen(v)/2;
         char *data_ptr = ur_get_dyn(tmpl, data, f_id);
         ur_set_dyn_offset(tmpl, data, f_id, (ur_get_dyn_offset_start(tmpl, data, f_id) + size));
         for ( ; size > 0; --size, v += 2, ++data_ptr) {
            sscanf(v, "%2hhx", data_ptr);
         }
      }
      break;
   default:
      fprintf(stderr, "Unsupported UniRec field type, skipping.\n");
      break;
   }
   return rv;
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
   if ((links) != NULL) {
      free(links->link_indexes);

      free(links);
      links = NULL;
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
