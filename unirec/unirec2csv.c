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

char *urcsv_record(urcsv_t *urcsv, const void *rec)
{
   int delim = 0;
   int i = 0, written = 0;
   ur_field_id_t id = 0;

   if (urcsv == NULL || rec == NULL) {
      return NULL;
   }

   urcsv->curpos = urcsv->buffer;

   // Iterate over all output fields
   while ((id = ur_iter_fields_record_order(urcsv->tmplt, i++)) != UR_ITER_END) {
      if (delim != 0) {
         *(urcsv->curpos++) = urcsv->delimiter;
         urcsv->free_space -= written;
      }

      delim = 1;
      if (ur_is_present(urcsv->tmplt, id)) {
         // Get pointer to the field (valid for static fields only)
         void *ptr = ur_get_ptr_by_id(urcsv->tmplt, rec, id);
         // Static field - check what type is it and use appropriate format
         switch (ur_get_type(id)) {
         case UR_TYPE_UINT8:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%" PRIu8, *(uint8_t*) ptr);
            break;
         case UR_TYPE_UINT16:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%" PRIu16, *(uint16_t*) ptr);
            break;
         case UR_TYPE_UINT32:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%" PRIu32, *(uint32_t*) ptr);
            break;
         case UR_TYPE_UINT64:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%" PRIu64, *(uint64_t*) ptr);
            break;
         case UR_TYPE_INT8:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%" PRIi8, *(int8_t*) ptr);
            break;
         case UR_TYPE_INT16:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%" PRIi16, *(int16_t*) ptr);
            break;
         case UR_TYPE_INT32:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%" PRIi32, *(int32_t*) ptr);
            break;
         case UR_TYPE_INT64:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%" PRIi64, *(int64_t*) ptr);
            break;
         case UR_TYPE_CHAR:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%c", *(char*) ptr);
            break;
         case UR_TYPE_FLOAT:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%f", *(float*) ptr);
            break;
         case UR_TYPE_DOUBLE:
            written = snprintf(urcsv->curpos, urcsv->free_space, "%f", *(double*) ptr);
            break;
         case UR_TYPE_IP:
            {
               // IP address - convert to human-readable format and print
               char str[46];
               ip_to_str((ip_addr_t*) ptr, str);
               written = snprintf(urcsv->curpos, urcsv->free_space, "%s", str);
            }
            break;
         case UR_TYPE_MAC:
            {
               // MAC address - convert to human-readable format and print
               char str[MAC_STR_LEN];
               mac_to_str((mac_addr_t *) ptr, str);
               written = snprintf(urcsv->curpos, urcsv->free_space, "%s", str);
            }
            break;
         case UR_TYPE_TIME:
            {
               // Timestamp - convert to human-readable format and print
               time_t sec = ur_time_get_sec(*(ur_time_t*)ptr);
               int msec = ur_time_get_msec(*(ur_time_t*)ptr);
               char str[32];
               strftime(str, 31, "%FT%T", gmtime(&sec));
               written = snprintf(urcsv->curpos, urcsv->free_space, "%s.%03i", str, msec);
            }
            break;
         case UR_TYPE_STRING:
            {
               // Printable string - print it as it is
               int size = ur_get_var_len(urcsv->tmplt, rec, id);
               char *data = ur_get_ptr_by_id(urcsv->tmplt, rec, id);
               *(urcsv->curpos++) = '"';
               urcsv->free_space--;

               while (size--) {
                  switch (*data) {
                  case '\n': // Replace newline with space
                     /* TODO / FIXME possible bug - info lost */
                     *(urcsv->curpos++) = ' ';
                     urcsv->free_space--;
                     break;
                  case '"' : // Double quotes in string
                     *(urcsv->curpos++) = '"';
                     urcsv->free_space--;
                     *(urcsv->curpos++) = '"';
                     urcsv->free_space--;
                     break;
                  default: // Check if character is printable
                     if (isprint(*data)) {
                        *(urcsv->curpos++) = *data;
                        urcsv->free_space--;
                     }
                  }
                  data++;
               }
               *(urcsv->curpos++) = '"';
               urcsv->free_space--;
            }
            break;
         case UR_TYPE_BYTES:
            {
               // Generic string of bytes - print each byte as two hex digits
               int size = ur_get_var_len(urcsv->tmplt, rec, id);
               unsigned char *data = ur_get_ptr_by_id(urcsv->tmplt, rec, id);
               while (size--) {
                  written = snprintf(urcsv->curpos, urcsv->free_space, "%02x", *data++);
                  urcsv->free_space -= written;
                  urcsv->curpos += written;
               }
               goto skipped_move;
            }
            break;
         default:
            {
               // Unknown type - print the value in hex
               int size = ur_get_len(urcsv->tmplt, rec, id);
               strncpy(urcsv->curpos, "0x", urcsv->free_space);
               urcsv->free_space -= 2;
               urcsv->curpos += 2;
               for (int i = 0; i < size; i++) {
                  written = snprintf(urcsv->curpos, urcsv->free_space, "%02x", ((unsigned char*)ptr)[i]);
                  urcsv->free_space -= written;
                  urcsv->curpos += written;
               }
               goto skipped_move;
            }
            break;
         } // case (field type)
         urcsv->free_space -= written;
         urcsv->curpos += written;
      } // if present

skipped_move:
      if (urcsv->free_space < 100) {
         urcsv->free_space += urcsv->buffer_size / 2;
         urcsv->buffer_size += urcsv->buffer_size / 2;
         uint32_t offset = urcsv->curpos - urcsv->buffer;
         void *temp = realloc(urcsv->buffer, urcsv->buffer_size);
         if (temp != NULL) {
            urcsv->buffer = temp;
            urcsv->curpos = urcsv->buffer + offset;
         } else {
            /* TODO handle allocation failure */
         }
      }
   } // loop over fields

   return strdup(urcsv->buffer);
}

