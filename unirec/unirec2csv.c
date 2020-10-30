#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <time.h>
#include "unirec2csv.h"

urcsv_t *urcsv_init(ur_template_t *tmplt, char delimiter)
{
   if (tmplt == NULL) {
      return NULL;
   }
   urcsv_t *s = malloc(sizeof(urcsv_t));
   if (s != NULL) {
      s->tmplt = tmplt;
      s->delimiter = delimiter;
      /* default size of buffer */
      s->buffer_size = 4096;
      s->free_space = s->buffer_size;
      s->buffer = calloc(s->buffer_size, sizeof(s->buffer[0]));
      if (s->buffer == NULL) {
         free(s);
         return NULL;
      }
   }
   return s;
}

void urcsv_free(urcsv_t **urcsv)
{
   if (urcsv != NULL && *urcsv != NULL) {
      free((*urcsv)->buffer);
      free(*urcsv);
      (*urcsv) = NULL;
   }
}

char *urcsv_header(urcsv_t *urcsv)
{
   if (urcsv == NULL) {
      return NULL;
   }
   return ur_template_string_delimiter(urcsv->tmplt, urcsv->delimiter);
}

int urcsv_value(char *dst, uint32_t size, void *ptr, int type, int field_len)
{
   int written = 0;
   char *p = dst;

   // Static field - check what type is it and use appropriate format
   switch (type) {
   case UR_TYPE_UINT8:
      written = snprintf(p, size, "%" PRIu8, *(uint8_t*) ptr);
      break;
   case UR_TYPE_UINT16:
      written = snprintf(p, size, "%" PRIu16, *(uint16_t*) ptr);
      break;
   case UR_TYPE_UINT32:
      written = snprintf(p, size, "%" PRIu32, *(uint32_t*) ptr);
      break;
   case UR_TYPE_UINT64:
      written = snprintf(p, size, "%" PRIu64, *(uint64_t*) ptr);
      break;
   case UR_TYPE_INT8:
      written = snprintf(p, size, "%" PRIi8, *(int8_t*) ptr);
      break;
   case UR_TYPE_INT16:
      written = snprintf(p, size, "%" PRIi16, *(int16_t*) ptr);
      break;
   case UR_TYPE_INT32:
      written = snprintf(p, size, "%" PRIi32, *(int32_t*) ptr);
      break;
   case UR_TYPE_INT64:
      written = snprintf(p, size, "%" PRIi64, *(int64_t*) ptr);
      break;
   case UR_TYPE_CHAR:
      written = snprintf(p, size, "%c", *(char*) ptr);
      break;
   case UR_TYPE_FLOAT:
      written = snprintf(p, size, "%f", *(float*) ptr);
      break;
   case UR_TYPE_DOUBLE:
      written = snprintf(p, size, "%f", *(double*) ptr);
      break;
   case UR_TYPE_IP:
      {
         // IP address - convert to human-readable format and print
         char str[46];
         ip_to_str((ip_addr_t*) ptr, str);
         written = snprintf(p, size, "%s", str);
      }
      break;
   case UR_TYPE_MAC:
      {
         // MAC address - convert to human-readable format and print
         char str[MAC_STR_LEN];
         mac_to_str((mac_addr_t *) ptr, str);
         written = snprintf(p, size, "%s", str);
      }
      break;
   case UR_TYPE_TIME:
      {
         // Timestamp - convert to human-readable format and print
         time_t sec = ur_time_get_sec(*(ur_time_t*) ptr);
         int usec = ur_time_get_usec(*(ur_time_t*) ptr);
         char str[40];
         strftime(str, 39, "%FT%T", gmtime(&sec));
         written = snprintf(p, size, "%s.%06i", str, usec);
      }
      break;
   default:
      {
         // Unknown type - print the value in hex
         strncpy(p, "0x", size);
         written += 2;
         p += 2;
         for (int i = 0; i < field_len && written < size; i++) {
            int w = snprintf(p, size - written, "%02x", ((unsigned char *) ptr)[i]);
            written += w;
            p += w;
         }
      }
      break;
   } // case (field type)

   return written;
}

int urcsv_field(char *dst, uint32_t size, const void *rec, ur_field_type_t id, ur_template_t *tmplt)
{
   int written = 0;
   char *p = dst;

   // Get pointer to the field (valid for static fields only)
   void *ptr = ur_get_ptr_by_id(tmplt, rec, id);
   int type = ur_get_type(id);

   if (type == UR_TYPE_STRING) {
      // Printable string - print it as it is
      int fs = ur_get_var_len(tmplt, rec, id);
      char *data = ur_get_ptr_by_id(tmplt, rec, id);
      *(p++) = '"';
      written++;

      while (fs-- && (written < size)) {
         switch (*data) {
            case '\n': // Replace newline with space
               /* TODO / FIXME possible bug - info lost */
               *(p++) = ' ';
               written++;
               break;
            case '"' : // Double quotes in string
               *(p++) = '"';
               written++;
               *(p++) = '"';
               written++;
               break;
            default: // Check if character is printable
               if (isprint(*data)) {
                  *(p++) = *data;
                  written++;
               }
         }
         data++;
      }
      *(p++) = '"';
      written++;
   } else if (type == UR_TYPE_BYTES) {
      // Generic string of bytes - print each byte as two hex digits
      int fs = ur_get_var_len(tmplt, rec, id);
      unsigned char *data = ur_get_ptr_by_id(tmplt, rec, id);
      while (fs-- && (written < size)) {
         int w = snprintf(p, size, "%02x", *data++);
         written += w;
         p += w;
      }
   } else if (ur_is_varlen(id)) {
      int elem_type = ur_array_get_elem_type(id);
      int elem_size = ur_array_get_elem_size(id);
      int elem_cnt = ur_array_get_elem_cnt(tmplt, rec, id);
      int array_len = ur_get_len(tmplt, rec, id);
      int i;

      if (written + 2 < size) {
         written += 2;
         *(p++) = '[';
         for (i = 0; i < elem_cnt; i++) {
            int w = urcsv_value(p, size - written, ((char *) ptr) + i * elem_size, elem_type, array_len);
            written += w;
            p += w;
            if (written + 1 >= size) {
               /* not enough space */
               return 0;
            }
            if (i + 1 != elem_cnt) {
               *(p++) = '|';
               written++;
            }
         }
         *(p++) = ']';
      }
   } else  {
      return urcsv_value(p, size, ptr, type, ur_get_len(tmplt, rec, id));
   }

   return written;
}

char *urcsv_record(urcsv_t *urcsv, const void *rec)
{
   int delim = 0;
   int i = 0, written = 0;
   ur_field_id_t id = 0;

   if (urcsv == NULL || rec == NULL) {
      return NULL;
   }

   urcsv->curpos = urcsv->buffer;
   urcsv->free_space = urcsv->buffer_size;

   // Iterate over all output fields
   while ((id = ur_iter_fields_record_order(urcsv->tmplt, i++)) != UR_ITER_END) {
      if (delim != 0) {
         *(urcsv->curpos++) = urcsv->delimiter;
         urcsv->free_space -= 1;
      }

      delim = 1;
reallocated:
      if (ur_is_present(urcsv->tmplt, id)) {
         // Static field - check what type is it and use appropriate format
         written = urcsv_field(urcsv->curpos, urcsv->free_space, rec, id, urcsv->tmplt);

         urcsv->free_space -= written;
         urcsv->curpos += written;
      } else {
         continue;
      }

      if (urcsv->free_space < 100 || (written == 0 && ur_get_var_len(urcsv->tmplt, rec, id) != 0)) {
         urcsv->free_space += urcsv->buffer_size / 2;
         urcsv->buffer_size += urcsv->buffer_size / 2;
         uint32_t offset = urcsv->curpos - urcsv->buffer;
         void *temp = realloc(urcsv->buffer, urcsv->buffer_size);
         if (temp != NULL) {
            urcsv->buffer = temp;
            urcsv->curpos = urcsv->buffer + offset;
         } else {
            return NULL;
         }
         if (written == 0) {
            goto reallocated;
         }
      }
   } // loop over fields

   *urcsv->curpos = 0;
   return strdup(urcsv->buffer);
}

