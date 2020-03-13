/**
 * \file configurator.h
 * \author Marek Hurta <xhurta01@stud.fit.vutbr.cz>
 * \author Erik Sabik <xsabik02@stud.fit.vutbr.cz>
 * \date 2015
 */

/*
 * Copyright (C) 2014 CESNET
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

#ifndef _NEMEA_COMMON_CONFIGURATOR
#define _NEMEA_COMMON_CONFIGURATOR


/**
 * Default string maximum size (with 0 terminating byte included)
 */
#define DEFAULT_STRING_MAX_SIZE "32"

/**
 * Enum of supported types.
 * WARNING: Order of elements must correspond to `varTypeSize` array!
 */
typedef enum {
    UINT8_T,
    INT8_T,
    UINT16_T,
    INT16_T,
    UINT32_T,
    INT32_T,
    UINT64_T,
    INT64_T,
    FLOAT,
    DOUBLE,
    STRING,
    STRUCT,
    ARRAY
} varType;

/**
 * Array with supported types sizes.
 * WARNING: Order of elements must correspond to `varType` enum!
 */
static const char varTypeSize[] = {
    sizeof(uint8_t),
    sizeof(int8_t),
    sizeof(uint16_t),
    sizeof(int16_t),
    sizeof(uint32_t),
    sizeof(int32_t),
    sizeof(uint64_t),
    sizeof(int64_t),
    sizeof(float),
    sizeof(double),
    (char) -1, // String, size is computed based on specified max size.
    (char) -1, // Struct, size is computed based on specified elements.
    (char) -1  // Array, size is computed based on specified elements.
};

/**
 * Enum to distinguish user types.
 */
typedef enum {
    VARIABLE,
    USTRUCT,
    UARRAY
} userType;

/**
 * Enum to distinguish array types.
 */
typedef enum {
    AR_ELEMENT,
    AR_STRUCT
} arrayType;

/**
 * Item structure of configuration map.
 */
typedef struct configStructItemStruct {
    std::string name;
    varType type;
    arrayType arrType;
    configStructItemStruct *arrElement;
    void *defaultValue;
    std::string defaultStringValue;
    void *map;
    bool isRequired;
    int offset;
    int stringMaxSize;
    void *elementAr;
    unsigned int arrayElementSize;
} configStrucItem;

/**
 * Item structure of user configuration map.
 */
typedef struct {
    userType type;
    arrayType arrType;
    std::string name;
    std::string value;
    void *map;
    void *elementAr;
} userConfigItem;

/**
 * Structure for modul information.
 */
typedef struct {
    std::string patternModuleName;
    std::string patternModuleAuthor;
    std::string userModuleName;
    std::string userModuleAuthor;
} infoStruct;

/**
 * Structure containing information about allocated memory for arrays.
 */
typedef struct {
    std::vector<void *> memBlockArr;
    std::vector<unsigned int> memBlockElCount;
} userAllocatedMemoryBlockStructure;

/**
 * Item structure for array elements.
 */
typedef struct {
    std::string name;
    std::string value;
} userArrElemStruct;

#endif
