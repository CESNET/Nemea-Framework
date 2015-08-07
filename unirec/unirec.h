/**
 * \file unirec.h
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

/**
 * \mainpage Brief description of UniRec format and API
 * @{
 * \section about About UniRec
 * UniRec is the name of message format and the name of library that can work with the messages.
 * One message consists of fields that are of static or dynamic size.
 * UniRec allows users to set and get values of UniRec fields.
 *
 * UniRec API contains various functions for manipulation: \ref unirec_basic, \ref ur_time, \ref ur_ipaddr and \ref ur_links.
 *
 * \subsection message_example Example of message --- order of fields
 *
 * \code
 * +-------+-------+-------+-------+
 * |            DST_IP             |
 * +-------+-------+-------+-------+
 * |            SRC_IP             |
 * +-------+-------+-------+-------+
 * |             BYTES             |
 * +-------+-------+-------+-------+
 * |            PACKETS            |
 * +-------+-------+-------+-------+
 * |   DST_PORT    |   SRC_PORT    |
 * +-------+-------+-------+-------+
 * | PROTO | TCP_F |    URL (7)    |
 * +-------+-------+-------+-------+
 * |    DYN1 (10)  |   DYN2 (11)   |   Static fields (size of part is always the same for the template)
 * +-------+-------+-------+-------+   -----------------
 * |              URL              |   Dynamic fields
 * +-------+-------+-------+-------+
 * |          URL          | DYN1  |
 * +-------+-------+-------+-------+
 * |     DYN1      | DYN2  |
 * +-------+-------+-------+
 * \endcode
 * @}
 */

#ifndef _UNIREC_H_
#define _UNIREC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "inline.h"
#include "ipaddr.h"
#include "ur_time.h"

typedef uint16_t ur_field_id_t; ///< Type of UniRec field identifiers
#include "fields.h" // Automatically generated marcos with field numbers (identifiers) and types

// Include structures and functions for handling LINK_BIT_FIELD
#include "links.h"

/**
 * \defgroup unirec_basic General API
 * @{
 */

#define UR_INVALID_OFFSET 0xffff
#define UR_INVALID_FIELD 0xffff

/** \brief UniRec template.
 * It contains a table mapping a field to its position in an UniRec record.
 */
typedef struct {
   uint16_t offset[UR_MAX_FIELD_ID+1]; ///< Table of offsets
   uint16_t static_size; ///< Size of static part
   ur_field_id_t first_dynamic; ///< ID of the first dynamic field or UR_INVALID_FIELD if the template doesn't contain dynamic fields
} ur_template_t;

/** \brief Constants for all possible types of UniRec fields */
typedef enum {
   UR_TYPE_STRING, // dynamic fields (string where only printable characters are expected; '\0' at the end should NOT be included)
   UR_TYPE_BYTES, // dynamic fields (generic string of bytes)
   UR_TYPE_CHAR,
   UR_TYPE_UINT8,
   UR_TYPE_INT8,
   UR_TYPE_UINT16,
   UR_TYPE_INT16,
   UR_TYPE_UINT32,
   UR_TYPE_INT32,
   UR_TYPE_UINT64,
   UR_TYPE_INT64,
   UR_TYPE_FLOAT,
   UR_TYPE_DOUBLE,
   UR_TYPE_IP,
   UR_TYPE_TIME,
} ur_field_type_t;


/** \brief Create new UniRec template
 * Create new UniRec template specified by a string containing names of its
 * fields separated by semicolons. Example spec-string:
 *   "SRC_IP;DST_IP;SRC_PORT;DST_PORT;PROTOCOL;PACKETS"
 * Order of fields is not important (templates with the same set of fields are
 * equivalent).
 * Template created by this function should be destroyed by ur_free_template.
 * \param[in] fields String with names of UniRec fields.
 * \return Pointer to newly created template or NULL on error.
 */
ur_template_t *ur_create_template(const char *fields);

/** \brief Print UniRec template
 * Print static size, first dynamic and table of offsets.
 * In case template does not contain any dynamic fields, prints - instead.
 * param[in] tmplt pointer to the template.
 */

void ur_print_template(ur_template_t *tmplt);

/** \brief Destroy UniRec template
 * Free all memory allocated for a template created previously by
 * ur_create_template.
 * \param[in] tmplt Pointer to the template.
 */
void ur_free_template(ur_template_t *tmplt);

/** \brief Create new UniRec template as a union of given templates
 * Create new UniRec template whose set of fields is union of all fields
 * of given templates.
 * \param[in] templates Array of pointers to templates to union.
 * \param[in] n Number of templates in the array.
 * \return Pointer to newly created template or NULL on error.
 */
ur_template_t *ur_union_templates(ur_template_t **templates, int n);

extern const char *UR_FIELD_NAMES[]; ///< Table with field names indexed by field ID
extern const short UR_FIELD_SIZES[]; ///< Table with field sizes indexed by field ID
extern const ur_field_type_t UR_FIELD_TYPES[]; ///< Table with field types indexed by field ID
#define ur_get_name_by_id(id) UR_FIELD_NAMES[(id)] ///< Return the name of a given field
#define ur_get_size_by_id(id) UR_FIELD_SIZES[(id)] ///< Return the size of a given field (in bytes)
#define ur_get_type_by_id(id) UR_FIELD_TYPES[(id)] ///< Return the type of a given field

ur_field_id_t ur_get_id_by_name(const char *name);

/** \brief Iterate over fields of a template
 * This function can be used to iterate over all fields of a given template.
 * It returns ID of the next field present in the template after a given ID.
 * If ID is set to UR_INVALID_FIELD, it returns the first fields. If no more
 * fields are present, UR_INVALID_FIELD is returned.
 * Example usage:
 *   ur_field_id_t id = UR_INVALID_FIELD;
 *   while ((id = ur_iter_fields(&tmplt, id)) != UR_INVALID_FIELD) {
 *      ...
 *   }
 * The order of fields is given by their order in "fields" file.
 * \param[in] tmplt Template to iterate over.
 * \param[in] id Field ID returned in last iteration or UR_INVALID_FIELD to
 *               start new cycle.
 * \return ID of the next field or UR_INVALID_FIELD if no more fields are
 *         present.
 */
ur_field_id_t ur_iter_fields(const ur_template_t *tmplt, ur_field_id_t id);


/** \brief Iterator used by ur_iter_fields_tmplt. */
typedef unsigned int ur_iter_t;

#define UR_ITER_BEGIN 0

/** \brief Iterate over fields of a template
 * This function can be used to iterate over all fields of a given template
 * in the same order as they are stored in a record.
 * At the first call, iter should be initialized to UR_ITER_BEGIN. The same
 * iter should be passed to each subsequent call.
 * Upon each call the function returns ID of the next field present in the
 * template. If no more fields are present, UR_INVALID_FIELD is returned.
 * Example usage:
 *   ur_field_id_t id;
 *   ur_iter_t iter = UR_ITER_BEGIN;
 *   while ((id = ur_iter_fields_tmplt(&tmplt, &iter)) != UR_INVALID_FIELD) {
 *      ...
 *   }
 *
 * \param[in] tmplt Template to iterate over.
 * \param[in] iter An iterator. It should be initialized to UR_ITER_BEGIN before
 *                 first call of the function.
 * \return ID of the next field or UR_INVALID_FIELD if no more fields are
 *         present.
 */
ur_field_id_t ur_iter_fields_tmplt(const ur_template_t *tmplt, ur_iter_t *iter);

/** Create UniRec record.
 * Allocate memory for a record with given template. It allocates N+M bytes,
 * where N is the size of static part of the record (inferred from template),
 * and M is the size of dynamic part, which must be provided by caller.
 * \param[in] tmplt Pointer to UniRec template.
 * \param[in] dyn_size Size of dynamic part, i.e. sum of lengths of all dynamic
 *                     fields. If it is not known at the time of record
 *                     creation, use UR_MAX_SIZE, which allocates enough memory
 *                     to hold the largest possible UniRec record (65535 bytes).
 *                     Set to 0 if there are no dynamic fields in the template.
 */
void *ur_create(ur_template_t *tmplt, uint16_t dyn_size);
#define UR_MAX_SIZE 0xffff

/** Free UniRec record.
 * Free memory allocated for UniRec record. You can call system free() on the
 * record as well, this function is there just for completeness.
 * \param[in] record Pointer to the record to free.
 */
void ur_free(void *record);


/** \brief Is given field present in given template?
 * Return true (non-zero value) if given template contains field with given ID.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] field_id Identifier of a field
 * \return non-zero if field is present, zero otherwise
 */
#define ur_is_present(tmplt, field_id) \
   ((tmplt)->offset[(field_id)] != UR_INVALID_OFFSET)

/** \brief Is given field dynamic?
 * Return true (non-zero value) if given ID refers to a field with dynamic
 * length.
 * \param[in] field_id Identifier of a field
 * \return non-zero if field is dynamic, zero otherwise
 */
#define ur_is_dynamic(field_id) \
   (ur_get_size_by_id((field_id)) == -1)


/** \brief Get value of UniRec field
 * Get value of static UniRec field. For dyamic fields, use ur_get_dyn.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field. It must be a constant beginning
 *                     with UR_, not its numeric value.
 * \return Value of the field. Data type depends on the type of the field.
 */
#define ur_get(tmplt, data, field_id) \
   (*(field_id ## _T *)((char *)(data) + (tmplt)->offset[field_id]))

/** \brief Get pointer to UniRec field
 * Get pointer to static UniRec field. For dyamic fields, use ur_get_dyn.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field. It must be a constant beginning
 *                     with UR_, not its numeric value.
 * \return Pointer to the field. Pointer type depends on the type of the field.
 */
#define ur_get_ptr(tmplt, data, field_id) \
   (field_id ## _T *)((char *)(data) + (tmplt)->offset[field_id])

/** \brief Get pointer to UniRec field
 * Get pointer to static UniRec field. In contrast to ur_get_ptr, field_id may
 * be a variable (not UR_ constant).
 * Returned pointer is always void *.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field
 * \return Pointer to the field (void *).
 */
#define ur_get_ptr_by_id(tmplt, data, field_id) \
   (void *)((char *)(data) + (tmplt)->offset[(field_id)])

/** \brief Get size of UniRec field
 * Get size of static UniRec field. For dyamic fields, use ur_get_dyn_size.
 * \param[in] field_id Identifier of a field. It must be a constant beginning
 *                     with UR_, not its numeric value.
 * \return Size of the field in bytes.
 */
#define ur_get_size(field_id) \
   sizeof(field_id ## _T)


/** \brief Get offset of the first byte of a dynamic field
 * Get offset of the first byte of a dynamic UniRec field. Offset is counted
 * from the end of static part of the record.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return Offset of the start of the dynamic field.
 */
#define ur_get_dyn_offset_start(tmplt, data, field_id) \
   ( (field_id == (tmplt)->first_dynamic) ? 0 : *(uint16_t *)((char *)(data) + (tmplt)->offset[field_id] - 2) )

/** \brief Get offset after the last byte of a dynamic field
 * Get offset just after the last byte of a dynamic UniRec field, that is
 * offset of the first byte of the field plus length of the field.
 * This is the offset set by ur_get_dyn_offset.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return Offset after the end of the dynamic field.
 */
#define ur_get_dyn_offset_end(tmplt, data, field_id) \
   ( *(uint16_t *)((char *)(data) + (tmplt)->offset[field_id]) )


/** \brief Get pointer to UniRec field with dynamic size
 * Get pointer to the beginnig of a dynamic UniRec field. For static fields,
 * use ur_get.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return Pointer (char *) to the beginning of the dynamic field.
 */
#define ur_get_dyn(tmplt, data, field_id) \
   ( (char *)(data) + \
     (tmplt)->static_size + \
     ur_get_dyn_offset_start((tmplt), (data), (field_id)) \
   )

/** \brief Get size of UniRec field with dynamic size
 * Get size of dynamic UniRec field. For static fields, use ur_get_size.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return Size of the field in bytes.
 */
#define ur_get_dyn_size(tmplt, data, field_id) \
   ( *(uint16_t *)((char *)(data) + (tmplt)->offset[field_id]) - \
      ( (field_id == (tmplt)->first_dynamic) ? 0 : *(uint16_t *)((char *)(data) + (tmplt)->offset[field_id] - 2) ) \
   )


/** \brief Get dynamic-size UniRec field as a C string
 * Copy data of a dynamic field from UniRec record and append '\0' character.
 * The function allocats new memory space for the string, it must be free'd
 * using free()!
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return Requested field as a string (char *) or NULL on malloc error. It should be free'd using free().
 */
char *ur_get_dyn_as_str(const ur_template_t *tmplt, const void *data, ur_field_id_t field_id);


/** \brief Set value of UniRec field
 * Set value of static UniRec field. Dynamic fields must be filled manually.
 * \param[in] tmplt Pointer to UniRec template
 * \param[out] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field. It must be a constant beginning
 *                     with UR_, not its numeric value.
 * \param[in] value The value the field should be set to.
 * \return New value of the field.
 */
#define ur_set(tmplt, data, field_id, value) \
   (*(field_id ## _T *)((char *)(data) + (tmplt)->offset[field_id]) = (value))

/** \brief Set value of a UniRec field
 * \param[in] tmpl Pointer to UniRec template
 * \param[out] data Pointer to the beginning of a record
 * \param[in] f_id Identifier of a field. It must be a constant beginning
 *                     with UR_, not its numeric value.
 * \param[in] v The value the field should be set to.
 * \return 0 on success, non-zero otherwise.
 */
int ur_set_from_string(const ur_template_t *tmpl, void *data, ur_field_id_t f_id, const char *v);


/** \brief Set offset of dynamic UniRec field
 * Set offset of the byte right after the end of dynamic UniRec field, i.e.
 * where the next field starts (or would start if it exists).
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \param[in] off Offset of the end of the field.
 * \return The offset.
 */
#define ur_set_dyn_offset(tmplt, data, field_id, off) \
   (*(uint16_t *)((char *)(data) + (tmplt)->offset[field_id]) = (off))


/** \brief Set dynamic UniRec field
 * Copy given data into dynamic UniRec field and set its offset in a record.
 * WARNING: Dynamic fields MUST be set in the same order as they are stored
 * in the record, that is in alphabetical order by their name.
 * Function ur_iter_field_tmplt may be used for iteration over fields in correct
 * order, for example.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \param[in] value_ptr Pointer to data which should be copied into the record.
 * \param[in] size Length of the copied data in bytes.
 */

#define ur_set_dyn(tmplt, data, field_id, value_ptr, size) \
   do { \
      char *out_ptr = ur_get_dyn((tmplt), (data), (field_id)); \
      memcpy(out_ptr, (value_ptr), (size)); \
      int new_offset = ur_get_dyn_offset_start((tmplt), (data), (field_id)) + (size); \
      ur_set_dyn_offset((tmplt), (data), (field_id), new_offset); \
   } while (0)



// TODO: check for invalid offset when DEBUG is defined


/** \brief Get size of static part of UniRec record
 * Get total size of whole UniRec record except dynamic fields.
 * \param[in] tmplt Pointer to UniRec template
 * \return Size of the dynamic part of UniRec record.
 */
#define ur_rec_static_size(tmplt) \
   ((tmplt)->static_size)


/** \brief Get size of dynamic part of UniRec record
 * Get total size of all dynamic fields in an UniRec record
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \return Size of the static part of UniRec record.
 */
#define ur_rec_dynamic_size(tmplt, data) \
   (((tmplt)->first_dynamic == UR_INVALID_OFFSET) ? \
    (0) : \
    (*(uint16_t *)((char *)(data) + (tmplt)->static_size - 2)) )


/** \brief Get total size of UniRec record
 * Get total size of given UniRec record including dynamic fields.
 * If you know that the record doesn't contain dynamic fields, use
 * ur_rec_static_size as it is slightly faster.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \return Size of the static part of UniRec record.
 */
#define ur_rec_size(tmplt, data) \
   (ur_rec_static_size(tmplt) + ur_rec_dynamic_size(tmplt, data))


/**
 * \brief Copy data from one UniRec to another.
 * Procedure gets template and void pointer of source and destination.
 * Then it copies the data defined by the given template.
 * The destination pointer must have requiered space allocated.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] src Pointer to source data
 * \param[out] dst Pointer to destination of copy
 */
#define ur_cpy(tmplt, src, dst) \
    (memcpy(dst,src,ur_rec_size(tmplt,src)))
/*inline void ur_cpy(ur_template_t *tmplt, const void *src, void *dst)
{
    memcpy(dst, src, ur_rec_size(tmplt, src));
}*/

/**
 * \brief Create new UniRec and copy the source UniRec into it.
 * Function creates new UniRec record and copies fills it with
 * the data given by parameter.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] src Pointer to source data
 * \return Pointer to a new UniRec
 */
INLINE void *ur_cpy_alloc(ur_template_t *tmplt, const void *src)
{
    void *copy = ur_create(tmplt, ur_rec_dynamic_size(tmplt, src));
    ur_cpy(tmplt, src, copy);
    return copy;
}

/**
 * \brief Transfer data between UniRec records with different templates.
 * Procedure gets templates of both records and their data and transfer
 * data from one to another. Destination data must be allocated beforehand.
 * The Procedure can transfer only static fields.
 * \param[in] tmplt_src Template of the source UniRec
 * \param[in] tmplt_dst Template of the destination UniRec
 * \param[in] src Source UniRec record
 * \param[out] dst Destination UniRec record
 */
void ur_transfer_static(ur_template_t *tmplt_src, ur_template_t *tmplt_dst, const void *src, void *dst);

/**
 * @}
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif
