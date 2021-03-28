/**
 * \file unirec2csv.h
 * \brief Definition of UniRec API to create CSV-like representation of UniRec data
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2019
 */
/*
 * Copyright (C) 2019 CESNET
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _UNIREC2CSV_H_
#define _UNIREC2CSV_H_

#include "unirec.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup unirec2csv CSV representation
 *
 * Functions to convert UniRec template and data into CSV-like representation
 *
 * \code{.c}
 * urcsv_t *csv = urcsv_init(tmplt, ',');
 *
 * char *str = urcsv_header(csv);
 * fprintf(stderr, "%s\n", str);
 * free(str);
 *
 * str = urcsv_record(csv, rec);
 * fprintf(stderr, "%s\n", str);
 * free(str);
 *
 * urcsv_free(&csv);
 * \endcode
 * @{
 */

/**
 * Internal structure used by urcsv_init(), urcsv_free(), urcsv_header(), urcsv_record()
 */
typedef struct urcsv_s {
   /**
    * UniRec template associated with this conversion
    */
   ur_template_t *tmplt;

   /**
    * Internal string buffer, allocated to buffer_size bytes
    */
   char *buffer;

   /**
    * Internal position in the buffer to write next string
    */
   char *curpos;

   /**
    * Current size of allocated memory for buffer
    */
   uint32_t buffer_size;

   /**
    * Current free bytes in the buffer
    */
   uint32_t free_space;

   /**
    * Delimiter that is put between columns
    */
   char delimiter;
} urcsv_t;

/**
 * Constructor for #urcsv_t
 *
 * The function initializes struct for urcsv_header() and urcsv_record().
 *
 * \param[in] tmplt  UniRec template that will be used to access fields of the records
 * \param[in] delimiter    Delimiter that will be used to separate columns of output
 * \return Pointer to newly allocated and initialized structure
 */
urcsv_t *urcsv_init(ur_template_t *tmplt, char delimiter);

/**
 * Destructor for #urcsv_t
 *
 * The funtion deallocates internal memory and urcsv, the pointer is set to NULL.
 * \param[in,out] urcsv Address of pointer to structure allocated by urcsv_init(), it will be set to NULL.
 */
void urcsv_free(urcsv_t **urcsv);

/**
 * Create a header line
 *
 * The funtion creates a text representation of header according to template
 * \param[in,out] urcsv Pointer to structure allocated by urcsv_init().
 * \return Pointer to string, caller must free it
 */
char *urcsv_header(urcsv_t *urcsv);

/**
 * Create a record line
 *
 * The funtion creates a text representation of UniRec record
 * \param[in,out] urcsv Pointer to structure allocated by urcsv_init().
 * \param[in] rec    Pointer to data - UniRec message
 * \return Pointer to string, caller must free it
 */
char *urcsv_record(urcsv_t *urcsv, const void *rec);

/**
 * Convert value of UniRec field to its string representation.
 *
 * \param[out] dst   Pointer to memory where to store result (pointer is not moved)
 * \param[in] size   Size of available memory for result
 * \param[in] rec    UniRec record - value of the field is taken
 * \param[in] id     UniRec field id
 * \param[in] tmplt  UniRec template
 *
 * \return Number of written bytes. If 0, there was not enough space and caller must increase the memory size.
 */
int urcsv_field(char *dst, uint32_t size, const void *rec, ur_field_type_t id, ur_template_t *tmplt);

/**
 * @}
 *//* unirec2csv */

#ifdef __cplusplus
} // extern "C"
#endif

#endif

