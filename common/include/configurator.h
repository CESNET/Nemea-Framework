/**
 * \file configurator.h
 * \author Marek Hurta <xhurta01@stud.fit.vutbr.cz>
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

#ifndef _NEMEA_COMMON_CONFIGURATOR_H
#define _NEMEA_COMMON_CONFIGURATOR_H

#include <inttypes.h>

/**
 * Enum for pattern types, used for specifying type of pattern XML config.
 */
enum patternTypes {
    CONF_PATTERN_FILE,
    CONF_PATTERN_STRING
};

#ifdef __cplusplus
extern "C" {
#endif

int confXmlLoadConfiguration(char *patternFile, char *userFile, void *userStruct, int patternType);
void confFreeUAMBS();
unsigned int confArrElemCount(void *arr);


int confPlainCreateContext();
void confPlainClearContext();
int confPlainAddElement(const char *name, const char *type, const char *defValue, int charArraySize, int requiredFlag);
int confPlainLoadConfiguration(const char *filePath, void *userStruct);

uint8_t confPlainGetUint8(const char *name, uint8_t defValue);
int8_t confPlainGetInt8(const char *name, int8_t defValue);
uint16_t confPlainGetUint16(const char *name, uint16_t defValue);
int16_t confPlainGetInt16(const char *name, int16_t defValue);
uint32_t confPlainGetUint32(const char *name, uint32_t defValue);
int32_t confPlainGetInt32(const char *name, int32_t defValue);
uint64_t confPlainGetUint64(const char *name, uint64_t defValue);
int64_t confPlainGetInt64(const char *name, int64_t defValue);
int32_t confPlainGetBool(const char *name, int32_t defValue);
float confPlainGetFloat(const char *name, float defValue);
double confPlainGetDouble(const char *name, double defValue);
const char *confPlainGetString(const char *name, const char *defValue);

#ifdef __cplusplus
}
#endif

#endif
