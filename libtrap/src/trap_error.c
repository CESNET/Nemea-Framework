/**
 * \file trap_error.c
 * \brief Error handling for TRAP.
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \date 2013
 * \date 2014
 */
/*
 * Copyright (C) 2013,2014 CESNET
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

#include "trap_error.h"

/** Default error message for each error code */
const char* default_err_msg[256] = {
   "No error",                          // 0 TRAP_E_OK
   "Read or write operation timeout",   // 1 TRAP_E_TIMEOUT
   0,0,0,0,0,0,0,0,                     // 2-9 unused
   "TRAP library already initialized.", // 10 TRAP_E_INITIALIZED
   "Bad parameters passed to interface initializer.", // 11 TRAP_E_BADPARAMS
   "Interface index out of range.",     // 12 TRAP_E_BAD_IFC_INDEX
   "Bad parameters of function",        // 13 TRAP_E_BAD_FPARAMS
   "Input/Output error",                // 14 TRAP_E_IO_ERROR
   "Interface was terminated during read/write", // 15 TRAP_E_TERMINATED
   "Interface was not selected for read/write",  // 16 TRAP_E_NOT_SELECTED
   "TLS certificate verification failed",  // 17 TRAP_E_BAD_CERT
   0,0,                                 // 18-19 unused
   0,0,0,0,0,0,0,0,0,0,                 // 20-29 unused
   0,0,0,0,0,0,0,0,0,0,                 // 30-39 unused
   0,0,0,0,0,0,0,0,0,0,                 // 40-49 unused
   0,0,0,0,0,0,0,0,0,0,                 // 50-59 unused
   0,0,0,0,0,0,0,0,0,0,                 // 60-69 unused
   0,0,0,0,0,0,0,0,0,0,                 // 70-79 unused
   0,0,0,0,0,0,0,0,0,0,                 // 80-89 unused
   0,0,0,0,0,0,0,0,0,0,                 // 90-99 unused
   0,0,0,0,0,0,0,0,0,0,                 // 100-109 unused
   0,0,0,0,0,0,0,0,0,0,                 // 110-119 unused
   0,0,0,0,0,0,0,0,0,0,                 // 120-129 unused
   0,0,0,0,0,0,0,0,0,0,                 // 130-139 unused
   0,0,0,0,0,0,0,0,0,0,                 // 140-149 unused
   0,0,0,0,0,0,0,0,0,0,                 // 150-159 unused
   0,0,0,0,0,0,0,0,0,0,                 // 160-169 unused
   0,0,0,0,0,0,0,0,0,0,                 // 170-179 unused
   0,0,0,0,0,0,0,0,0,0,                 // 180-189 unused
   0,0,0,0,0,0,0,0,0,0,                 // 190-199 unused
   0,0,0,0,0,0,0,0,0,0,                 // 200-209 unused
   0,0,0,0,0,0,0,0,0,0,                 // 210-219 unused
   0,0,0,0,0,0,0,0,0,0,                 // 220-229 unused
   0,0,0,0,0,0,0,0,0,0,                 // 230-239 unused
   0,0,0,0,0,0,0,0,0,0,                 // 240-249 unused
   0,0,0,0,                             // 250-253 unused
   "TRAP library not initialized",      // 254 TRAP_E_NOT_INITIALIZED
   "Memory allocation error",           // 255 TRAP_E_MEMORY
};

