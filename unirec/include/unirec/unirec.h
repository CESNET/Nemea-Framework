/**
 * \file unirec.h
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

#ifndef _UNIREC2_H_
#define _UNIREC2_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include "ipaddr.h"
#include "macaddr.h"
#include "ur_time.h"
#include "links.h"
#include "ur_values.h"
#include <libtrap/trap.h>

//General setting
#define UR_DEFAULT_LENGTH_OF_TEMPLATE 1024 /// Length of string of a template
#define UR_DEFAULT_LENGTH_OF_FIELD_NAME 128 /// Length of name (string) of a field
#define UR_DEFAULT_LENGTH_OF_FIELD_TYPE 16 /// Length of type (string) of a field
#define UR_INITIAL_SIZE_FIELDS_TABLE 5 ///< Initial size of free space in fields tables
#define UR_FIELD_ID_MAX INT16_MAX       ///< Max ID of a field
#define UR_FIELDS(...)        ///<  Definition of UniRec fields
#define UR_ARRAY_DELIMITER ' ' ///< Delimiter of array elements in string
#define UR_ARRAY_ALLOC 10 ///< Default alloc size increment for ur_set_array_from_string
//Iteration constants
#define UR_ITER_BEGIN (-1)  ///< First value in iterating through the fields
#define UR_ITER_END INT16_MAX    ///< Last value in iterating through the fields
//default values
#define UR_INVALID_OFFSET 0xffff ///< Default value of all offsets (value is not in the record)
#define UR_NO_DYNAMIC_VALUES 0xffff    ///< Value of variable "first_dynamic" if no dynamic values are present
#define UR_UNINITIALIZED 0          ///< Indicator if the UniRec has not been initialized by calling function ur_init.
#define UR_INITIALIZED 1            ///< Indicator if the UniRec has been initialized by calling function ur_init.
#define UR_INVALID_FIELD ((ur_field_id_t) 0xffff)    ///< ID of invalid field
//return codes
#define UR_E_INVALID_PARAMETER -6   ///< The given parameter is wrong
#define UR_E_INVALID_FIELD_ID -5       ///< The field ID is not present in a template.
#define UR_E_TYPE_MISMATCH -4    ///< The type of a field is different
#define UR_E_INVALID_NAME -3     ///< The given name is not present in a template.
#define UR_E_INVALID_TYPE -2     ///< The type of a field is not defined
#define UR_E_MEMORY -1        ///< Problem during allocating memory
#define UR_OK 0         ///< No problem


/** \brief Constants for all possible types of UniRec fields */
#define UR_COUNT_OF_TYPES 29 ///< Count of types of UniRec fields
typedef enum {
   UR_TYPE_STRING,   ///< var-len fields (string where only printable characters are expected; '\0' at the end should NOT be included)
   UR_TYPE_BYTES,    ///< var-len fields (generic string of bytes)
   UR_TYPE_CHAR,  ///< char
   UR_TYPE_UINT8, ///< unsigned int (8b)
   UR_TYPE_INT8,  ///< int (8b)
   UR_TYPE_UINT16,   ///< unsigned int (16b)
   UR_TYPE_INT16, ///< int (8b)
   UR_TYPE_UINT32,   ///< unsigned int (32b)
   UR_TYPE_INT32, ///< int (32b)
   UR_TYPE_UINT64,   ///< unsigned int (64b)
   UR_TYPE_INT64, ///< int (64b)
   UR_TYPE_FLOAT, ///< float (32b)
   UR_TYPE_DOUBLE,   ///< double (64b)
   UR_TYPE_IP,    ///< IP address (128b)
   UR_TYPE_MAC,    ///< MAC address (48b)
   UR_TYPE_TIME,   ///< time (64b)

   // Arrays
   UR_TYPE_A_UINT8, ///< unsigned int (8b) array
   UR_TYPE_A_INT8,  ///< int (8b) array
   UR_TYPE_A_UINT16,   ///< unsigned int (16b) array
   UR_TYPE_A_INT16, ///< int (8b) array
   UR_TYPE_A_UINT32,   ///< unsigned int (32b) array
   UR_TYPE_A_INT32, ///< int (32b) array
   UR_TYPE_A_UINT64,   ///< unsigned int (64b) array
   UR_TYPE_A_INT64, ///< int (64b) array
   UR_TYPE_A_FLOAT, ///< float (32b) array
   UR_TYPE_A_DOUBLE,   ///< double (64b) array
   UR_TYPE_A_IP,    ///< IP address (128b) array
   UR_TYPE_A_MAC,    ///< MAC address (48b) array
   UR_TYPE_A_TIME   ///< time (64b) array
} ur_field_type_t;

typedef enum {
   UR_TMPLT_DIRECTION_NO,  ///< template is not used for sending data
   UR_TMPLT_DIRECTION_IN,  ///< input direction
   UR_TMPLT_DIRECTION_OUT, ///< ouput direction
   UR_TMPLT_DIRECTION_BI   ///< bidirection
} ur_tmplt_direction;

typedef int16_t ur_field_id_t;  ///< Type of UniRec field identifiers
typedef ur_field_id_t ur_iter_t; ///< Type for identifying iteration id through all fields

/** \brief Linked list for undefined field ids
 * Linked list consisting of field ids, which are freed after operation undefine.
 */
typedef struct ur_field_id_linked_list_s ur_field_id_linked_list_t;
struct ur_field_id_linked_list_s{
  ur_field_id_t id;        ///< free id
  ur_field_id_linked_list_t *next;  ///< Pointer to next item in the linked list
};

/** \brief UniRec default field list
 * It contains all fields which are specified statically in source code of a module.
 * This structure is passed to ur_init()
 */
typedef struct {
   char **ur_field_names;     ///< Array of names of fields
   short *ur_field_sizes;     ///< Array of sizes of fields
   ur_field_type_t *ur_field_types;    ///< Array of types of fields
   ur_field_id_t ur_last_id;     ///< Last specified ID
} ur_static_field_specs_t;

/** \brief UniRec fields structure
 * It contains all fields which are statically defined by UR_FIELDS(...) and run-time generated
 * fields. This structure can be modified during run-time by generating new fields and erasing
 * existing fields.
 */
typedef struct {
   char **ur_field_names;        ///< Array of names of fields
   short *ur_field_sizes;        ///< Array of sizes of fields
   ur_field_type_t *ur_field_types;    ///< Array of types of fields
   ur_field_id_t ur_last_statically_defined_id; ///< Last statically defined field by UR_FIELDS(...)
   ur_field_id_t ur_last_id;        ///< The highest ID of a field + 1
   /**
   *Count of fields which is allocated (ur_allocated_fields - ur_last_id = remaining fields count)
    */
   ur_field_id_t ur_allocated_fields;
   ur_field_id_linked_list_t * ur_undefine_fields; ///< linked list of free (undefined) IDs
   uint8_t intialized;  ///< If the UniRec is initialized by function ur_init variable is set to UR_INITIALIZED, otherwise 0
} ur_field_specs_t;

/** \brief Sorting fields structure
 * This structure is used to sort fields by their size and name. The structure
 * is passed to the sorting algorithm.
 */
typedef struct field_spec_s {
   char *name;    ///< Name of a field
   int size;      ///< Size of a field
   ur_field_id_t id; ///< ID of a field
} field_spec_t;

/** \brief UniRec template.
 * It contains a table mapping a field to its position in an UniRec record.
 */
typedef struct {
   uint16_t *offset;       ///< Table of offsets
   uint16_t offset_size;   ///< size of offset table.
   int16_t *ids;    ///< Array of ids in template
   uint16_t first_dynamic; ///< First dynamic (variable-length) field. Index to the ids array
   uint16_t count;      ///< Count of fields in template
   uint16_t static_size;   ///< Size of static part
   ur_tmplt_direction direction; ///< Direction of data input, output, bidirection, no direction
   uint32_t ifc_out;   ///< output interface number (stored only if the direction == UR_TMPLT_DIRECTION_BI)
} ur_template_t;

/**
 * \defgroup libtraphelpers Helpers for libtrap
 *
 * This module defines useful macros for handling data receive.
 * When data format is changed, these macros create new UniRec template.
 *
 * @{
 */
/** \brief Receive data from interface
 * Receive data with specified template from libtrap interface. If the receiving template is
 * subset of sending template, it will define new fields and expand receiving template.
 * \param[in] ifc_num index of libtrap interface
 * \param[in] data pointer to memory where the data will be stored
 * \param[in] data_size size of allocated space for data
 * \param[in] tmplt pointer to input template
 * \return return value of trap_recv
 */
#define TRAP_RECEIVE(ifc_num, data, data_size, tmplt) \
 TRAP_CTX_RECEIVE(trap_get_global_ctx(), ifc_num, data, data_size, tmplt);

/** \brief Receive data from interface with given context
 * Receive data with specified template from libtrap interface with specified context.
 * If the receiving template is subset of sending template, it will define new fields
 * and expand receiving template.
 * \param[in] ctx context
 * \param[in] ifc_num index of libtrap interface
 * \param[in] data pointer to memory where the data will be stored
 * \param[in] data_size size of allocated space for data
 * \param[in] tmplt pointer to input template
 * \return return value of trap_ctx_recv
 */
#define TRAP_CTX_RECEIVE(ctx, ifc_num, data, data_size, tmplt) __extension__ \
({\
   int ret = trap_ctx_recv(ctx, ifc_num, &data, &data_size);\
   if (ret == TRAP_E_FORMAT_CHANGED) {\
      const char *spec = NULL;\
      uint8_t data_fmt;\
      if (trap_ctx_get_data_fmt(ctx, TRAPIFC_INPUT, ifc_num, &data_fmt, &spec) != TRAP_E_OK) {\
         fprintf(stderr, "Data format was not loaded.\n");\
      } else {\
         tmplt = ur_define_fields_and_update_template(spec, tmplt);\
         if (tmplt == NULL) {\
            fprintf(stderr, "Template could not be edited.\n");\
         } else {\
            if (tmplt->direction == UR_TMPLT_DIRECTION_BI) {\
               char * spec_cpy = ur_cpy_string(spec);\
               if (spec_cpy == NULL) {\
                  fprintf(stderr, "Memory allocation problem.\n");\
               } else {\
                  trap_ctx_set_data_fmt(ctx, tmplt->ifc_out, TRAP_FMT_UNIREC, spec_cpy);\
               }\
            }\
         }\
      }\
   }\
   ret;\
})

/** \brief Receive data from interface with given context
 * Receive data with specified template from libtrap interface with specified context.
 * If the receiving template is subset of sending template, it will define new fields
 * and expand receiving template.
 * \param[in] ctx context
 * \param[in] ifc_num index of libtrap interface
 * \param[in] data pointer to memory where the data will be stored
 * \param[in] data_size size of allocated space for data
 * \param[in] tmplt pointer to input template
 * \return return value of trap_ctx_recv
 */
/*#define UNIREC_CTX_CHECK_TEMPLATE (ctx, ret, ifc_num, tmplt, fmt_changed_cmd, fmt_not_compatabile_cmd) __extension__ \
({\
   if (ret == TRAP_E_OK_FORMAT_CHANGED && trap_ctx_get_in_ifc_state(ctx, ifc_num) == FMT_SUBSET) {\
      const char *spec = NULL;\
      uint8_t data_fmt;\
      if (trap_ctx_get_data_fmt(ctx, TRAPIFC_INPUT, ifc_num, &data_fmt, &spec) != TRAP_E_OK) {\
         fprintf(stderr, "Data format was not loaded.");\
      } else {\
         tmplt = ur_define_fields_and_update_template(spec, tmplt);\
         if (tmplt == NULL) {\
            fprintf(stderr, "Template could not be edited");\
         } else {\
            if (tmplt->direction == UR_TMPLT_DIRECTION_BI) {\
               char * spec_cpy = ur_cpy_string(spec);\
               if (spec_cpy == NULL) {\
                  fprintf(stderr, "Memory allocation problem.");\
               } else {\
                  trap_ctx_set_data_fmt(ctx, tmplt->ifc_out, TRAP_FMT_UNIREC, spec_cpy);\
               }\
            }\
            trap_ctx_confirm_ifc_state(ctx, ifc_num);\
            fmt_changed_cmd;
         }\
      }\
   }\
   ret;\
})*/

/**
 * @}
 *//* libtraphelpers */

/**
 * \defgroup urtemplate UniRec templates and fields
 *
 * @{
 */

/**
 * \brief Sizes of UniRec data types.
 *
 * Data types are defined in the #ur_field_type_str array.
 */
extern const int ur_field_type_size[];

/**
 * \brief UniRec data types.
 *
 * Sizes of data types are defined in the #ur_field_type_size array.
 */
extern const char *ur_field_type_str[];

/**
 * \brief UniRec array element types.
 *
 * Used to get type of an array element. Can be indexed using UR_TYPE_*.
 */
extern int ur_field_array_elem_type[];

/**
 * \brief Structure that lists UniRec field specifications such as names, types, id.
 */
extern ur_field_specs_t ur_field_specs;

/**
 * \brief Structure that lists staticaly defined UniRec field specifications such as names, types, id (using UR_FIELDS()).
 */
extern ur_static_field_specs_t UR_FIELD_SPECS_STATIC;

/** \brief Get UniRec specifier of the `tmplt` template with `delimiter` between fields.
 *
 * Get names and sizes of fields separated by given delimiter.
 *
 * Example:
 * \code
 * ipaddr DST_IP,ipaddr SRC_IP
 * \endcode
 *
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] delimiter  Delimiter that is placed between the UniRec fields in the template specifier.
 * \return String containing list of fields and their types.
 * \note Caller must free the returned memory.
 */
char *ur_template_string_delimiter(const ur_template_t *tmplt, int delimiter);

/** \brief Get string of a template
 * Get names and sizes of fields separated by comma. Return string has to be freed by user.
 *
 * \see ur_template_string_delimiter()
 * \param[in] tmplt Pointer to UniRec template
 * \return String containing list of fields and their types.
 * \note Caller must free the returned memory.
 */
#define ur_template_string(tmplt) \
 ur_template_string_delimiter(tmplt, ',')

/** \brief Get size of UniRec type
 * Get size of fixed-length UniRec type. For variable-length type return value < 0.
 * \param[in] type Enum value of type.
 * \return Size of the field in bytes.
 */
#define ur_size_of(type) \
   ur_field_type_size[type]

/** \brief Get name of UniRec field
 * Get name of any UniRec defined field.
 * \param[in] field_id ID of a field.
 * \return pointer to name (char *).
 */
#define ur_get_name(field_id) \
   ur_field_specs.ur_field_names[field_id]

/** \brief Get type of UniRec field
 * Get type of any UniRec defined field.
 * \param[in] field_id ID of a field.
 * \return Type of the field (ur_field_type_t).
 */
#define ur_get_type(field_id) \
   ur_field_specs.ur_field_types[field_id]

/**
 * Lookup a field type for its textual representation.
 *
 * \param[in] type   UniRec type in string format, e.g. "ipaddr" or "uint8"
 * \returns Index into ur_field_type_str and ur_field_type_size arrays or UR_E_INVALID_TYPE if the type is not found.
 */
int ur_get_field_type_from_str(const char *type);

/** \brief Get size of UniRec field
 * Get size of a fixed-length UniRec field. When variable-length field is passed,
 * return value < 0. To get real length of a variable-length field use ur_get_var_len.
 * \param[in] field_id ID of a field.
 * \return Size of the field in bytes or -1 if the field is variable-length.
 */
#define ur_get_size(field_id) \
   ur_field_specs.ur_field_sizes[field_id]

/** \brief Get value of UniRec field
 * Get value of a fixed-length UniRec field. For variable-length fields, use ur_get_ptr
 * or ur_get_ptr_by_id.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field. It must be a token beginning with
 *                     F_, not a numeric ID (e.g. F_SRC_IP for source IP
 *                     defined in UR_FIELDS as SRC_IP).
 * \return Value of the field. Data type depends on the type of the field.
 */
#define ur_get(tmplt, data, field_id) \
   (*(field_id ## _T*)((char *)(data) + (tmplt)->offset[field_id]))

/** \brief Get pointer to UniRec field
 * Get pointer to fixed or varible length statically defined UniRec field.
 * For dynamically defined fields, use ur_get_ptr_by_id.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field. It must be a token beginning with
 *                     F_, not a numeric ID.
 * \return Pointer to the field. Pointer type depends on the type of the field.
 */
#define ur_get_ptr(tmplt, data, field_id) \
   (field_id ## _T*)((char *) (ur_is_static(field_id) ? ((char *)(data) + (tmplt)->offset[field_id]) : \
   (char *)(data) + (tmplt)->static_size + (*((uint16_t *)((char *)(data) + (tmplt)->offset[field_id])))))

/** \brief Get pointer to UniRec field
 * Get pointer to fixed or varible length UniRec field. In contrast to ur_get_ptr, field_id may
 * be a variable (not only F_ token). Returned pointer is always void *.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field. Token beginning with F_ or a numeric ID.
 * \return Pointer to the field (void *).
 */
#define ur_get_ptr_by_id(tmplt, data, field_id) \
   (void *)((char *) (ur_is_static(field_id) ? ((char *)(data) + (tmplt)->offset[field_id]) : \
      (char *)(data) + (tmplt)->static_size + (*((uint16_t *)((char *)(data) + (tmplt)->offset[field_id])))))

/** \brief Set value of UniRec (static) field
 * Set value of a fixed-length UniRec field. For variable-length fields, use ur_set_var.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] data Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field. It must be a token beginning with
 *                     F_, not a numeric ID.
 * \param[in] value The value the field should be set to.
 */
#define ur_set(tmplt, data, field_id, value) \
   (*(field_id ## _T*)((char *)(data) + (tmplt)->offset[field_id]) = (value))

/** \brief Get offset of variable-length field in the record.
 * Get offset of a specified variable-length field in the record. Given field has to be part of the record.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return Offset of the field in the record relative to the beginning of the record.
 */
#define ur_get_var_offset(tmplt, rec, field_id) \
   *((uint16_t *)((char *) rec + tmplt->offset[field_id]))

/** \brief Get size of a variable sized field in the record.
 * Get size of a variable-length field in the record. Given field has to be part of the record.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return Length of the field in the record in bytes.
 */
#define ur_get_var_len(tmplt, rec, field_id) \
   *((uint16_t *)((char *) rec + tmplt->offset[field_id] + 2))

/** \brief Set size of variable-length field in the record.
 * Set size of specified variable-length field in the record. Field has to be part of the record. It does
 * not move other variable sized records. It just sets given value to the right position in the record.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \param[in] len The value the length should be set to.
 */
#define ur_set_var_len(tmplt, rec, field_id, len) \
   *((uint16_t *)((char *) rec + tmplt->offset[field_id] + 2)) = (uint16_t) len

/** \brief Set offset of variable-length field in the record.
 * Set offset of specified variable-length field in the record. Field has to be part of the record. It does
 * not move other variable sized records. It just sets given value to the right position in the record.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \param[in] offset_val The value the offset should be set to.
 */
#define ur_set_var_offset(tmplt, rec, field_id, offset_val) \
   *((uint16_t *)((char *) rec + tmplt->offset[field_id])) = (uint16_t) offset_val

/** \brief Get length of UniRec field
 * Get actual length of fixed or variable-length UniRec field.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field Identifier of a field - e.g. F_SRC_IP for source IP defined in UR_FIELDS as SRC_IP.
 * \return Length of the field in bytes.
 */
#define ur_get_len(tmplt, rec, field) \
   ur_is_static(field) ? ur_get_size(field) : ur_get_var_len(tmplt, rec, field)

/** \brief Set variable-length field to given string.
 * Set contents of a variable-length field in the record. Field has to be part of the record.
 * It copies string terminated with \0 and safely moves other variable-length records.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \param[in] str String terminated with \0, which will be copied
 */
#define ur_set_string(tmplt, rec, field_id, str) \
   ur_set_var(tmplt, rec, field_id, str, strlen(str))

/**
 * Check if field is an array.
 *
 * \param[in] field_id Identifier of a field.
 * \return 1 if field is an array, 0 otherwise.
 */
#define ur_is_array(field_id) \
   (ur_field_type_size[ur_get_type(field_id)] < 0 ? 1 : 0)

/**
 * \brief Get size of a single element of UniRec field.
 *
 * \param[in] field_id Identifier of a field.
 * \return Size of a static field or size of a single element in an array field.
 */
#define ur_array_get_elem_size(field_id) \
   (ur_field_type_size[ur_get_type(field_id)] < 0 ? -ur_field_type_size[ur_get_type(field_id)] : ur_field_type_size[ur_get_type(field_id)])

/**
 * \brief Get number of elements stored in an UniRec array.
 *
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return Number of elements stored in an UniRec array field.
 */
#define ur_array_get_elem_cnt(tmplt, rec, field_id) \
   (ur_get_var_len(tmplt, rec, field_id) / ur_array_get_elem_size(field_id))

/**
 * \brief Get type of an element stored in an UniRec array.
 *
 * \param[in] field_id Identifier of a field.
 * \return UR_TYPE_* value.
 */
#define ur_array_get_elem_type(field_id) \
   ur_field_array_elem_type[ur_get_type(field_id)]

/**
 * \brief Set element to array at given index. Automatically resizes array when index is out of array bounds.
 * Set contents of a variable-length field in the record. Field has to be part of the record.
 *
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \param[in] index Element index in array.
 * \param[in] element Array element.
 */
#define ur_array_set(tmplt, rec, field_id, index, element) \
   if ((index) * ur_array_get_elem_size(field_id) < ur_get_var_len(tmplt, rec, field_id) || ur_array_resize(tmplt, rec, field_id, (index + 1) * ur_array_get_elem_size(field_id)) == UR_OK) { \
      (((field_id ## _T)((char *)(ur_get_ptr_by_id(tmplt, rec, field_id))))[index] = (element)); \
   }

/**
 * \brief Set element to array at last position. Automatically resizes array.
 *
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \param[in] element Array element.
 */
#define ur_array_append(tmplt, rec, field_id, element) \
   if (ur_array_resize(tmplt, rec, field_id, (ur_array_get_elem_cnt(tmplt, rec, field_id) + 1) * ur_array_get_elem_size(field_id)) == UR_OK) { \
      (((field_id ## _T)((char *)(ur_get_ptr_by_id(tmplt, rec, field_id))))[ur_array_get_elem_cnt(tmplt, rec, field_id) - 1] = (element)); \
   }

/**
 * \brief Allocate new element at the end of array and return its pointer.
 *
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return Pointer to allocated element at the end or NULL if allocation failed.
 */
char *ur_array_append_get_ptr(const ur_template_t *tmplt, void *rec, int field_id);

/**
 * \brief Clear contents of an UniRec array. Array is resized to 0 elements.
 *
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return UR_OK if there is no problem. UR_E_INVALID_FIELD_ID if the ID is not in the template.
 */
#define ur_array_clear(tmplt, rec, field_id) \
   ur_array_resize(tmplt, rec, field_id, 0)

/**
 * \brief Get element of an array field at given index
 *
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \param[in] index Element index in array.
 * \param[in] element Array element.
 * \return Specified field in an array.
 */
#define ur_array_get(tmplt, rec, field_id, index) \
   ((field_id ## _T)((char *)(ur_get_ptr_by_id(tmplt, rec, field_id))))[index]

/**
 * \brief Preallocates UniRec array field to have requested number of elements.
 *
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \param[in] elem_cnt Number of elements to be in resized array.
 * \return UR_OK if there is no problem. UR_E_INVALID_FIELD_ID if the ID is not in the template.
 */
#define ur_array_allocate(tmplt, rec, field_id, elem_cnt) \
   ur_array_resize(tmplt, rec, field_id, (elem_cnt) * ur_array_get_elem_size(field_id))

/** \brief Is given field present in given template?
 * Return true (non-zero value) if given template contains field with given ID.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] field_id Identifier of a field.
 * \return non-zero if field is present, zero otherwise
 */
#define ur_is_present(tmplt, field_id) \
   ((tmplt)->offset_size > (field_id) && (tmplt)->offset[(field_id)] != UR_INVALID_OFFSET)

/** \brief Is given field variable-length?
 * Return true (non-zero value) if given ID refers to a field with variable length.
 * \param[in] field_id Identifier of a field.
 * \return non-zero if field is variable-length, zero otherwise
 */
#define ur_is_varlen(field_id) \
   (ur_field_specs.ur_field_sizes[field_id] < 0)

/** \brief Alias for ur_is_varlen (for backwards compatibility only) */
#define ur_is_dynamic(field_id) \
   ur_is_varlen(field_id)

/** \brief Is given field fixed-length?
 * Return true (non-zero value) if given ID refers to a field with fixed length.
 * \param[in] field_id Identifier of a field.
 * \return non-zero if field is fixed-length, zero otherwise
 */
#define ur_is_fixlen(field_id) \
   (ur_field_specs.ur_field_sizes[field_id] >= 0)

/** \brief Alias for ur_is_fixlen (for backwards compatibility only) */
#define ur_is_static(field_id) \
   ur_is_fixlen(field_id)


/** \brief Get size of fixed-length part of UniRec record
 * Get total size of UniRec record except variable-length fields.
 * \param[in] tmplt Pointer to UniRec template
 * \return Size of the static part of UniRec record.
 */
#define ur_rec_fixlen_size(tmplt) \
   ((tmplt)->static_size)

/** \brief Get size of UniRec record (static and variable length)
 * Get total size of whole UniRec record.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \return Size of the UniRec record.
 */
#define ur_rec_size(tmplt, rec) \
   (ur_rec_fixlen_size(tmplt) + ur_rec_varlen_size(tmplt, rec))

/** \brief Initialize UniRec structures
 * Initialize UniRec structures. Function is called during defining first own field.
 * \param[in] field_specs_static Structure of statically-known UniRec fields.
 * \return UR_E_MEMORY if there is an allocation problem, UR_OK otherwise.
 */
int ur_init(ur_static_field_specs_t field_specs_static);

/** \brief Return first empty id for new UniRec field
 * Return first empty id for new UniRec field. If there is no space in the UniRec
 * structures, it will increase space in the existing structures.
 * \return ID for new field. UR_E_MEMORY (negative value) if there is an allocation problem.
 */
int ur_get_empty_id();

/** \brief Define new UniRec field
 * Define new UniRec field at run-time. It adds new field into existing structures.
 * If the field already exists (name and type are equal) it only returns its ID.
 * \param[in] name String with name of field.
 * \param[in] type Type of field (specified by UR_TYPE_*).
 * \return ID of created or existing field.
 * UR_E_MEMORY if there is an allocation problem.
 * UR_E_INVALID_NAME if the name value is empty.
 * UR_E_TYPE_MISMATCH if the name already exists, but the type is different.
 */
int ur_define_field(const char *name, ur_field_type_t type);

/** \brief Define set of new UniRec fields
 * Define new UniRec fields at run-time. It adds new fields into existing structures.
 * If the field already exists and type is equal nothing will happen. If the type is not equal
 * an error will be returned.
 * Example ifc_data_fmt: "uint32 FOO,uint8 BAR,float FOO2"
 * \param[in] ifc_data_fmt String containing types and names of fields delimited by comma.
 * \return UR_OK on success
 * UR_E_MEMORY if there is an allocation problem.
 * UR_E_INVALID_NAME if the name value is empty.
 * UR_E_INVALID_TYPE if the type does not exist.
 * UR_E_TYPE_MISMATCH if the name already exists, but the type is different.
 */
int ur_define_set_of_fields(const char *ifc_data_fmt);

/** \brief Undefine UniRec field by its id
 * Undefine UniRec field created at run-time. It erases given field from UniRec structures
 * and the field ID can be used for another field.
 * By undefining field, all templates which were using the undefined field, has to be recreated.
 * \param[in] field_id Identifier of a field.
 * \return UR_E_MEMORY if there is an allocation problem.
 * UR_E_INVALID_PARAMETER if the field was not created at run-time or the field_id does not exist.
 */
int ur_undefine_field_by_id(ur_field_id_t field_id);

/** \brief Undefine UniRec field by its name
 * Undefine UniRec field created at run-time. It erases given field from UniRec structures
 * and the field ID can be used for another field.
 * By undefining field, all templates which were using the undefined field, has to be recreated.
 * \param[in] name Name of a field.
 * \return UR_E_MEMORY if there is an allocation problem.
 * UR_E_INVALID_PARAMETER if the field was not created at run-time or the field_id does not exist.
 */
int ur_undefine_field(const char *name);

/** \brief Deallocate UniRec structures
 * Deallocate UniRec structures at the end of a program.
 * This function SHOULD be called after all UniRec functions and macros
 * invocations, typically during a cleanup phase before the program's end.
 * This function has to be called if some fields are defined
 * during run-time, otherwise this function is needless.
 */
void ur_finalize();

/** \brief Get ID of a field by its name
 * Get ID of a field by its name.
 * \param[in] name String with name of a field.
 * \return ID of a field. UR_E_INVALID_NAME (negative value) if the name is not known.
 */
int ur_get_id_by_name(const char *name);

/** \brief Compare fields
 * Compare two fields. This function is for sorting the fields in the right order.
 * First by sizes (larger first) and then by names.
 * \param[in] field1 Pointer to first field (field_spec_t)
 * \param[in] field2 Pointer to second field (field_spec_t)
 * \return -1 if f1 should go before f2, 0 if f1 is the same as f2, 1 otherwise
 */
int compare_fields(const void *field1, const void *field2);

/** \brief Create UniRec template
 * Create new UniRec template specified by a string containing names of its
 * fields separated by commas. Example spec-string:
 *   "SRC_IP,DST_IP,SRC_PORT,DST_PORT,PROTOCOL,PACKETS"
 * Order of fields is not important (templates with the same set of fields are
 * equivalent).
 * Template created by this function should be destroyed by ur_free_template.
 * \param[in] fields String with names of fields delimited by comma
 * \param[in] errstr Pointer to char * string where the error message will be
 * allocated and written in case of error. NULL if you don't need error message.
 * In case of error, function will allocate string, which has to be freed by user.
 * \return Pointer to newly created template or NULL in case of error.
 */
ur_template_t *ur_create_template(const char *fields, char **errstr);

/** \brief Create UniRec template and set it to input interface
 * Creates UniRec template (same like ur_create_template) and set this template
 * to input interface of a module.
 * \param[in] ifc interface number
 * \param[in] fields String with names of fields delimited by comma
 * \param[in] errstr Pointer to char * string where the error message will be
 * allocated and written in case of error. NULL if you don't need error message.
 * In case of error, function will allocate string, which has to be freed by user.
 * \return Pointer to newly created template or NULL in case of error.
 */
#define ur_create_input_template(ifc, fields, errstr) \
   ur_ctx_create_input_template(trap_get_global_ctx(), ifc, fields, errstr);

/** \brief Create UniRec template and set it to output interface
 * Creates UniRec template (same like ur_create_template) and set this template
 * to ouput interface of a module.
 * \param[in] ifc interface number
 * \param[in] fields String with names of fields delimited by comma
 * \param[in] errstr Pointer to char * string where the error message will be
 * allocated and written in case of error. NULL if you don't need error message.
 * In case of error, function will allocate string, which has to be freed by user.
 * \return Pointer to newly created template or NULL in case of error.
 */
#define ur_create_output_template(ifc, fields, errstr) \
   ur_ctx_create_output_template(trap_get_global_ctx(), ifc, fields, errstr);

/** \brief Create UniRec template and set it to output interface on specified context
 * Creates UniRec template, same like ur_create_output_template, but context is specified.
 * \param[in] ctx specified context
 * \param[in] ifc interface number
 * \param[in] fields String with names of fields delimited by comma
 * \param[in] errstr Pointer to char * string where the error message will be
 * allocated and written in case of error. NULL if you don't need error message.
 * In case of error, function will allocate string, which has to be freed by user.
 * \return Pointer to newly created template or NULL in case of error.
 */
ur_template_t *ur_ctx_create_output_template(trap_ctx_t *ctx, int ifc, const char *fields, char **errstr);

/** \brief Create UniRec template and set it to input interface on specified context
 * Creates UniRec template, same like ur_create_input_template, but context is specified.
 * \param[in] ctx specified context
 * \param[in] ifc interface number
 * \param[in] fields String with names of fields delimited by comma
 * \param[in] errstr Pointer to char * string where the error message will be
 * allocated and written in case of error. NULL if you don't need error message.
 * In case of error, function will allocate string, which has to be freed by user.
 * \return Pointer to newly created template or NULL in case of error.
 */
ur_template_t *ur_ctx_create_input_template(trap_ctx_t *ctx, int ifc, const char *fields, char **errstr);


/** \brief Set UniRec template to ouput interface on specified context
 * \param[in] ctx specified context
 * \param[in] ifc interface number
 * \param[in] tmplt pointer to a template
 * \return UR_OK if there is no problem. UR_E_MEMORY if the ID is not in the template.
 */
int ur_ctx_set_output_template(trap_ctx_t *ctx, int ifc, ur_template_t *tmplt);

/** \brief Set UniRec template to ouput interface
 * \param[in] ifc interface number
 * \param[in] tmplt pointer to a template
 * \return UR_OK if there is no problem. UR_E_MEMORY if the ID is not in the template.
 */
#define  ur_set_output_template(ifc, tmplt) \
   ur_ctx_set_output_template(trap_get_global_ctx(), ifc, tmplt)

/** \brief Set UniRec template to input interface on specified context
 * \param[in] ctx specified context
 * \param[in] ifc interface number
 * \param[in] tmplt pointer to a template
 * \return UR_OK if there is no problem. UR_E_MEMORY if the ID is not in the template.
 */
int ur_ctx_set_input_template(trap_ctx_t *ctx, int ifc, ur_template_t *tmplt);

/** \brief Set UniRec template to input interface
 * \param[in] ifc interface number
 * \param[in] tmplt pointer to a template
 * \return UR_OK if there is no problem. UR_E_MEMORY if the ID is not in the template.
 */
#define  ur_set_input_template(ifc, tmplt) \
   ur_ctx_set_input_template(trap_get_global_ctx(), ifc, tmplt)

/** \brief Create UniRec template and set it to input and output interface on specified context
 * Creates UniRec template, same like ur_create_bidirectional_template, but context is specified.
 * \param[in] ctx specified context
 * \param[in] ifc_in input interface number
 * \param[in] ifc_out output interface number
 * \param[in] fields String with names of fields delimited by comma
 * \param[in] errstr Pointer to char * string where the error message will be
 * allocated and written in case of error. NULL if you don't need error message.
 * In case of error, function will allocate string, which has to be freed by user.
 * \return Pointer to newly created template or NULL in case of error.
 */
ur_template_t *ur_ctx_create_bidirectional_template(trap_ctx_t *ctx, int ifc_in, int ifc_out, const char *fields, char **errstr);

/** \brief Create UniRec template and set it to input and output interface
 * Creates UniRec template (same like ur_create_template) and set this template
 * to input and ouput interface of a module.
 * \param[in] ifc_in input interface number
 * \param[in] ifc_out output interface number
 * \param[in] fields String with names of fields delimited by comma
 * \param[in] errstr Pointer to char * string where the error message will be
 * allocated and written in case of error. NULL if you don't need error message.
 * In case of error, function will allocate string, which has to be freed by user.
 * \return Pointer to newly created template or NULL in case of error.
 */
#define ur_create_bidirectional_template(ifc_in, ifc_out, fields, errstr) \
   ur_ctx_create_bidirectional_template(trap_get_global_ctx(), ifc_in, ifc_out, fields, errstr);

/** \brief Expand UniRec template
 * Expand existing UniRec template by a string containing types and names of its
 * fields separated by commas. Example ifc_data_fmt:
 *   "uint32 FOO,uint8 BAR,float FOO2"
 * Order of fields is not important (templates with the same set of fields are
 * equivalent).
 * In case of success the given template will be destroyed and new template will be returned.
 * \param[in] ifc_data_fmt String with types and names of fields delimited by commas
 * \param[in] tmplt Pointer to an existing template.
 * \return Pointer to the updated template or NULL in case of error.
 */
ur_template_t *ur_expand_template(const char *ifc_data_fmt, ur_template_t *tmplt);

/** \brief Defined new fields and expand an UniRec template
 * Define new fields (function ur_define_set_of_fields) and create new UniRec
 * template (function ur_create_template_from_ifc_spec).
 * The string describing fields contain types and names of fields separated by commas.
 * Example ifc_data_fmt: "uint32 FOO,uint8 BAR,float FOO2"
 * Order of fields is not important (templates with the same set of fields are
 * equivalent).
 * In case of success the given template will be destroyed and new template will be returned.
 * \param[in] ifc_data_fmt String with types and names of fields delimited by commas
 * \param[in] tmplt Pointer to an existing template.
 * \return Pointer to the updated template or NULL in case of error.
 */
ur_template_t *ur_define_fields_and_update_template(const char *ifc_data_fmt, ur_template_t *tmplt);

/** \brief Create UniRec template from data format string.
 * Creates new UniRec template (function ur_create_template_from_ifc_spec).
 * The string describing fields contain types and names of fields separated by commas.
 * Example ifc_data_fmt: "uint32 FOO,uint8 BAR,float FOO2"
 * Order of fields is not important (templates with the same set of fields are
 * equivalent)..
 * \param[in] ifc_data_fmt String with types and names of fields delimited by commas
 * \return Pointer to the new template or NULL in case of error.
 */
ur_template_t *ur_create_template_from_ifc_spec(const char *ifc_data_fmt);

/** \brief Destroy UniRec template
 * Free all memory allocated for a template created previously by
 * ur_create_template.
 * \param[in] tmplt Pointer to the template.
 */
void ur_free_template(ur_template_t *tmplt);

/** \brief Compares fields of two UniRec templates
 * Function compares only sets of UniRec fields (direction is not compared).
 * \note Function does not check if arguments are valid UniRec templates. Caller must check validity
 * before calling this function. Return value is undefined when any argument IS NOT a valid UniRec
 * template (i.e. some of its field is redefined, etc).
 * \param[in] tmpltA Pointer to the first template.
 * \param[in] tmpltB Pointer to the second template.
 * \return Returns non-zero (1) value if templates matches. Otherwise, it returns 0.
 */
int ur_template_compare(const ur_template_t *tmpltA, const ur_template_t *tmpltB);


/** \brief Print UniRec template
 * Print static_size, first_dynamic and table of offsets to stdout (for debugging).
 * If template does not contain any dynamic fields, print '-' instead.
 * param[in] tmplt pointer to the template.
 */
void ur_print_template(ur_template_t *tmplt);

/** \brief Set content of variable-length UniRec field
 * Copy given data into variable-length UniRec field, set its offset and length in a record and
 * move data which are behind this field. For better performance use function ur_clear_varlen,
 * before setting all variable fields in record.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record.
 * \param[in] field_id Identifier of a field.
 * \param[in] val_ptr Pointer to data which should be copied into the record.
 * \param[in] val_len Length of the copied data in bytes.
 * \return UR_OK if there is no problem. UR_E_INVALID_FIELD_ID if the ID is not in the template.
 */
int ur_set_var(const ur_template_t *tmplt, void *rec, int field_id, const void *val_ptr, int val_len);

/** \brief Change length of a array field.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record.
 * \param[in] field_id Identifier of a field.
 * \param[in] len Length of the copied data in bytes.
 * \return UR_OK if there is no problem. UR_E_INVALID_FIELD_ID if the ID is not in the template.
 */
int ur_array_resize(const ur_template_t *tmplt, void *rec, int field_id, int len);

/** \brief Clear variable-length part of a record.
 * For better performance of setting content to variable-length fields, use this function
 * before setting of all the variable-length fields. This function will clear all the variable-length
 * fields, so they don't have to be moved in memory during setting of them.
 * \param[in] tmplt Pointer to UniRec template.
 * \param[in] rec Pointer to the beginning of a record.
 */
void ur_clear_varlen(const ur_template_t *tmplt, void *rec);

/** \brief Get size of variable sized part of UniRec record
 * Get total size of all variable-length fields in an UniRec record
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the record
 * \return Size of the variable part of UniRec record.
 */
uint16_t ur_rec_varlen_size(const ur_template_t *tmplt, const void *rec);

/** Create UniRec record.
 * Allocate and zero memory for a record with given template. It
 * allocates N+M bytes, where N is the size of static part of the
 * record (inferred from template), and M is the size of variable part
 * (variable-length fields), which must be provided by caller.
 * No more than 65535 bytes is allocated (even if N+M is greater),
 * since this is the maximal possible size of UniRec record.
 * \param[in] tmplt Pointer to UniRec template.
 * \param[in] max_var_size Size of variable-length part, i.e. sum of
 *                     lengths of all variable-length fields. If it is
 *                     not known at the time of record creation, use
 *                     UR_MAX_SIZE, which allocates enough memory to
 *                     hold the largest possible UniRec record (65535
 *                     bytes). Set to 0 if there are no
 *                     variable-length fields in the template.
 */
void *ur_create_record(const ur_template_t *tmplt, uint16_t max_var_size);
#define UR_MAX_SIZE 0xffff

/** Free UniRec record.
 * Free memory allocated for UniRec record. You can call system free() on the
 * record as well, this function is there just for completeness.
 * \param[in] record Pointer to the record to free.
 */
void ur_free_record(void *record);

/** \brief Get variable-length UniRec field as a C string
 * Copy data of a variable-length field from UniRec record and append '\0' character.
 * The function allocates new memory space for the string, it must be freed
 * using free()!
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] rec Pointer to the beginning of a record
 * \param[in] field_id Identifier of a field.
 * \return Requested field as a string (char *) or NULL on malloc error. It should be
 * freed using free().
 */
char *ur_get_var_as_str(const ur_template_t *tmplt, const void *rec, ur_field_id_t field_id);

/**
 * \brief Copy data from one UniRec record to another.
 * Copies all fields present in both templates from src to dst. The function
 * compares src_tmplt and dst_tmplt and for each field present in both
 * templates it sets the value of field in dst to a corresponding value in src.
 * "dst" must point to a memory of enough size.
 * \param[in] dst_tmplt Pointer to destination UniRec template
 * \param[in] dst Pointer to destination record
 * \param[in] src_tmplt Pointer to source UniRec template
 * \param[in] src Pointer to source record
 */
void ur_copy_fields(const ur_template_t *dst_tmplt, void *dst, const ur_template_t *src_tmplt, const void *src);

/**
 * \brief Copy data from one UniRec to another.
 * Procedure gets template and void pointer of source and destination.
 * Then it copies the data defined by the given template.
 * The destination pointer must have required space allocated.
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
 * Function creates new UniRec record and fills it with the data given by
 * parameter.
 * \param[in] tmplt Pointer to UniRec template
 * \param[in] src Pointer to source data (UniRec record of the same template)
 * \return Pointer to a new UniRec
 */
void *ur_clone_record(const ur_template_t *tmplt, const void *src);

/** \brief Iterate over fields of a template in order of a record
 * This function can be used to iterate over all fields of a given template.
 * It returns ID of the next field present in the template after a given ID.
 * If ID is set to UR_ITER_BEGIN, it returns the first fields. If no more
 * fields are present, UR_ITER_END is returned.
 * Example usage:
 *   ur_field_id_t id = UR_ITER_BEGIN
 *   while ((id = ur_iter_fields(&tmplt, id)) != UR_ITER_END) {
 *      ...
 *   }
 * The order of fields is given by the order in which they are defined.
 * \param[in] tmplt Template to iterate over.
 * \param[in] id Field ID returned in last iteration or UR_ITER_BEGIN to
 *               get first value.
 * \return ID of the next field or UR_ITER_END if no more fields are present.
 */
ur_iter_t ur_iter_fields(const ur_template_t *tmplt, ur_iter_t id);

/** \brief Iterate over fields of a template
 * This function can be used to iterate over all fields of a given template.
 * It returns n-th ID of a record specified by index.
 * If the return value is UR_ITER_END. The index is higher than count of fields
 * in the template.
 * Example usage:
 *   int i = 0;
 *   while ((id = ur_iter_fields(&tmplt, i++)) != UR_ITER_END) {
 *      ...
 *   }
 * The order of fields is given by the order in the record
 * \param[in] tmplt Template to iterate over.
 * \param[in] index Field ID returned in last iteration or UR_ITER_BEGIN to
 *               get first value.
 * \return Field ID of the next field or UR_ITER_END if no more fields are present.
 */
ur_iter_t ur_iter_fields_record_order(const ur_template_t *tmplt, int index);

/** \brief Set value of a UniRec array field
 * \param[in] tmpl Pointer to UniRec template
 * \param[out] data Pointer to the beginning of a record
 * \param[in] f_id Identifier of a field. It must be a constant beginning
 *                     with UR_, not its numeric value.
 * \param[in] v The value the field should be set to, array of fields with ' ' space delimiter.
 * \return 0 on success, non-zero otherwise.
 */
int ur_set_array_from_string(const ur_template_t *tmpl, void *data, ur_field_id_t f_id, const char *v);

/** \brief Set value of a UniRec field
 * \param[in] tmpl Pointer to UniRec template
 * \param[out] data Pointer to the beginning of a record
 * \param[in] f_id Identifier of a field. It must be a constant beginning
 *                     with UR_, not its numeric value.
 * \param[in] v The value the field should be set to.
 * \return 0 on success, non-zero otherwise.
 */
int ur_set_from_string(const ur_template_t *tmpl, void *data, ur_field_id_t f_id, const char *v);

/** \brief Parses field names from data format
 * Function parses field names from data format and returns
 * pointer to new allocated string.
 * Example: "type1 name1,type2 name2" => "name1,name2"
 * New string has to be freed by user.
 * \param[in] ifc_data_fmt Pointer to the string containing data format
 * \return Pointer to string with names of fields
 */
char *ur_ifc_data_fmt_to_field_names(const char *ifc_data_fmt);

/** \brief Duplicates given string.
 * Helper function which returns pointer to duplicated string.
 * New string has to be freed by user.
 * \param[in] str Pointer to the string to duplicate
 * \return Pointer to duplicated string on NULL.
 */
char *ur_cpy_string(const char *str);

/** \brief Returns name of specified value (Helper function)
 * Helper function for ur_values_get_name. This function returns name of
 * specified value and field, which is defined in values file. Function
 * needs start and end index of a field.
 * \param[in] start Index of first item to search the value in ur_values array
 * \param[in] end Index of last item to search the value in ur_values array
 * \param[in] value Value of an item to find.
 * \return Pointer to string or NULL if the value was not found
 */
const char *ur_values_get_name_start_end(uint32_t start, uint32_t end, int32_t value);

/** \brief Returns description of specified value (Helper function)
 * Helper function for ur_values_get_description. This function returns description of
 * specified value and field, which is defined in values file. Function
 * needs start and end index of a field.
 * \param[in] start Index of first item to search the value in ur_values array
 * \param[in] end Index of last item to search the value in ur_values array
 * \param[in] value Value of an item to find
 * \return Pointer to string or NULL if the value was not found
 */
const char *ur_values_get_description_start_end(uint32_t start, uint32_t end, int32_t value);

/** \brief Returns name of specified value
 * This function returns name of specified value and field, which is defined in
 * values file.
 * \param[in] field Name of field to search value in
 * \param[in] value Value of an item to find
 * \return Pointer to string or NULL if the value was not found
 */
#define ur_values_get_name(field, value) \
   ur_values_get_name_start_end(UR_TYPE_START_ ## field, UR_TYPE_END_ ## field, value)

/** \brief Returns description of specified value
 * This function returns description of specified value and field, which is defined in
 * values file.
 * \param[in] field Name of field to search value in
 * \param[in] value Value of an item to find
 * \return Pointer to string or NULL if the value was not found
 */
#define ur_values_get_description(field, value) \
   ur_values_get_description_start_end(UR_TYPE_START_ ## field, UR_TYPE_END_ ## field, value)

/**
 * @}
 *//* urtemplate */

#ifdef __cplusplus
} // extern "C"
#endif

#endif
