/**
 * \file trap.h
 * \brief Interface of TRAP library.
 * \author Vaclav Bartos <ibartosv@fit.vutbr.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \author Tomas Jansky <janskto1@fit.cvut.cz>
 * \date 2013 - 2018
 */
/*
 * Copyright (C) 2013 - 2018 CESNET
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

#ifndef _TRAP_H_
#define _TRAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdarg.h>

#include "trap_module_info.h"
#include "jansson.h"

#define trap_ctx_t void

/**
 * \defgroup commonapi Common libtrap API
 *
 * This module declares basic public constants, structures and functions of libtrap.
 * @{
 */

/**
 * Text string with libtrap version.
 */
extern const char trap_version[];

/**
 * Text string with Git revision of libtrap.
 */
extern const char trap_git_version[];

/**
 * Text string with default path format to sockets (UNIX IFC and service IFC).
 * Assigned in ifc_tcpip.h
 */
extern char *trap_default_socket_path_format;

/**
 * \defgroup errorcodes Error codes
 * @{*/
#define TRAP_E_OK 0 ///< Success, no error
#define TRAP_E_TIMEOUT 1 ///< Read or write operation timeout
#define TRAP_E_INITIALIZED 10 ///< TRAP library already initilized
#define TRAP_E_BADPARAMS 11 ///< Bad parameters passed to interface initializer
#define TRAP_E_BAD_IFC_INDEX 12 ///< Interface index out of range
#define TRAP_E_BAD_FPARAMS 13 ///< Bad parameters of function
#define TRAP_E_IO_ERROR 14 ///< IO Error
#define TRAP_E_TERMINATED 15 ///< Interface was terminated during reading/writing
#define TRAP_E_NOT_SELECTED 16 ///< Interface was not selected reading/writing
#define TRAP_E_BAD_CERT 17 ///< Wrong certificate given to TLS interface
#define TRAP_E_HELP 20 ///< Returned by parse_parameters when help is requested
#define TRAP_E_FIELDS_MISMATCH 21 ///< Returned when receiver fields are not subset of sender fields
#define TRAP_E_FIELDS_SUBSET 22 ///< Returned when receivers fields are subset of senders fields and both sets are not identical
#define TRAP_E_FORMAT_CHANGED 23 ///< Returned by trap_recv when format or format spec of the receivers interface has been changed
#define TRAP_E_FORMAT_MISMATCH 24 ///< Returned by trap_recv when data format or data specifier of the output and input interfaces doesn't match
#define TRAP_E_NOT_INITIALIZED 254 ///< TRAP library not initilized
#define TRAP_E_MEMORY 255 ///< Memory allocation error
/**@}*/

/**
 * \defgroup trap_timeout TRAP Timeout
 *
 * TRAP IFC works with timeout to decide wether it should block or just wait
 * for some time.
 * The timeout is usually in microseconds.
 * This section lists some special timeout values.
 * @{
 */
/**
 * Non-Blocking mode, do not wait ever.
 */
#define TRAP_NO_WAIT 0
/**
 * Blocking mode, wait for client's connection, for message transport to/from
 * internal system buffer.
 */
#define TRAP_WAIT -1
/**
 * Blocking mode, do not wait for client's connection, clients do not try to
 * reconnect, there is no recovering of clients during get_data/send_data in this mode.
 * For input ifc it is the same as TRAP_NO_WAIT.
 */
#define TRAP_HALFWAIT -2

#define TRAP_TIMEOUT_STR(t) (t==TRAP_WAIT?"TRAP_WAIT":(t==TRAP_NO_WAIT?"TRAP_NO_WAIT":(t==TRAP_HALFWAIT?"TRAP_HALFWAIT":"")))

#define TRAP_NO_AUTO_FLUSH (-1l) ///< value to disable autoflushing on output interface

/**@}*/

/**
 * \defgroup trapifcspec Specifier of TRAP interfaces
 * The format of -i parameter of modules sets up TRAP interfaces.
 * #TRAP_IFC_DELIMITER divides specifier into interfaces.
 * #TRAP_IFC_PARAM_DELIMITER divides parameters of one interface.
 *
 * Each TRAP interface must have the type (\ref ifctypes) as the first parameter.
 * The following parameters are passed to the interface and they are interface-dependent.
 *
 * The format example, let's assume the module has 1 input IFC and 1 output IFC:
 *
 *  <BLOCKQUOTE>-i t:localhost:7600,u:my_socket</BLOCKQUOTE>
 *
 * This sets the input IFC to TCP type and it will connect to localhost, port 7600.
 * The output IFC will listen on UNIX socket with identifier my_socket.
 *
 * @{
 */

/**
 * Delimiter of TRAP interfaces in IFC specifier
 */
#define TRAP_IFC_DELIMITER ','

/**
 * Delimiter of TRAP interface's parameters in IFC specifier
 */
#define TRAP_IFC_PARAM_DELIMITER ':'

/**
 * \defgroup ifctypes Types of IFC
 * @{
 */
#define TRAP_IFC_TYPE_GENERATOR 'g' ///< trap_ifc_dummy generator (input)
#define TRAP_IFC_TYPE_BLACKHOLE 'b' ///< trap_ifc_dummy blackhole (output)
#define TRAP_IFC_TYPE_TCPIP     't' ///< trap_ifc_tcpip (input&output part)
#define TRAP_IFC_TYPE_TLS       'T' ///< trap_ifc_tls (input&output part)
#define TRAP_IFC_TYPE_UNIX      'u' ///< trap_ifc_tcpip via UNIX socket(input&output part)
#define TRAP_IFC_TYPE_SERVICE   's' ///< service ifc
#define TRAP_IFC_TYPE_FILE      'f' ///< trap_ifc_file (input&output part)
extern char trap_ifc_type_supported[];

/**
 * Type of interface (direction)
 */
enum trap_ifc_type {
   TRAPIFC_INPUT = 1, ///< interface acts as source of data for module
   TRAPIFC_OUTPUT = 2 ///< interface is used for sending data out of module
};

/**
 * @}
 *//* ifctypes */
/**
 * @}
 *//* trapifcspec */

/**
 * \name TRAP interface control request
 * @{*/
enum trap_ifcctl_request {
   TRAPCTL_AUTOFLUSH_TIMEOUT = 1,  ///< Set timeout of automatic buffer flushing for interface, expects uint64_t argument with number of microseconds. It can be set to #TRAP_NO_AUTO_FLUSH to disable autoflush.
   TRAPCTL_BUFFERSWITCH = 2,       ///< Enable/disable buffering - could be dangerous on input interface!!! expects char argument with value 1 (default value after libtrap initialization - enabled) or 0 (for disabling buffering on interface).
   TRAPCTL_SETTIMEOUT = 3          ///< Set interface timeout (int32_t): in microseconds for non-blocking mode; timeout can be also: TRAP_WAIT, TRAP_HALFWAIT, or TRAP_NO_WAIT.
};
/**@}*/

#ifndef TRAP_IFC_MESSAGEQ_SIZE
#define TRAP_IFC_MESSAGEQ_SIZE 100000 ///< size of message queue used for buffering
#endif

/** Structure with specification of interface types and their parameters.
 *  This can be filled by command-line parameters using trap_parse_params
 *  function.
 */
typedef struct trap_ifc_spec_s {
   char *types;
   char **params;
} trap_ifc_spec_t;

/**
 * \defgroup trap_mess_fmt Message format
 * @{
 */
/**
 * Type of messages that are sent via IFC
 */
typedef enum {
   /** unknown - message format was not specified yet  */
   TRAP_FMT_UNKNOWN = 0,

   /** raw data, no format specified */
   TRAP_FMT_RAW = 1,

   /** UniRec records */
   TRAP_FMT_UNIREC = 2,

   /** structured data serialized using JSON */
   TRAP_FMT_JSON = 3
} trap_data_format_t;

/**
 * Possible states of an IFC during data format negotiation.
 */
typedef enum {
   /** Negotiation is not completed */
   FMT_WAITING = 0,

   /** Negotiation was successful */
   FMT_OK = 1,

   /** Negotiation failed, format mismatch */
   FMT_MISMATCH = 2,

   /** Negotiation was successful, but receivers (input ifc) template is subset of senders (output ifc) template and missing fields has to be defined */
   FMT_CHANGED = 3
} trap_in_ifc_state_t;

/**
 * Set format of messages on output IFC.
 *
 * \param[in] out_ifc_idx   index of output IFC
 * \param[in] data_type     format of messages defined by #trap_data_format_t
 * \param[in] ...   if data_type is TRAP_FMT_UNIREC or TRAP_FMT_JSON, additional parameter
 * that specifies template is expected
 */
void trap_set_data_fmt(uint32_t out_ifc_idx, uint8_t data_type, ...);

/**
 * Set format of messages expected on input IFC.
 *
 * \param[in] in_ifc_idx   index of input IFC
 * \param[in] data_type     format of messages defined by #trap_data_format_t
 * \param[in] ...   if data_type is TRAP_FMT_UNIREC or TRAP_FMT_JSON, additional parameter
 * that specifies template is expected
 * \return TRAP_E_OK on success
 */
int trap_set_required_fmt(uint32_t in_ifc_idx, uint8_t data_type, ...);

/**
 * Get message format and template that is set on IFC.
 *
 * On output IFC it should return the values that were set.  On input IFC
 * it should return format and template that was received.
 *
 * \param[in] ifc_dir     #trap_ifc_type direction of interface
 * \param[in] ifc_idx   index of IFC
 * \param[out] data_type     format of messages defined by #trap_data_format_t
 * \param[out] spec   Template specifier - UniRec specifier in case of TRAP_FMT_UNIREC data_type, otherwise, it can be any string.
 * \return TRAP_E_OK on success, on error see trap_ctx_get_data_fmt().
 */
int trap_get_data_fmt(uint8_t ifc_dir, uint32_t ifc_idx, uint8_t *data_type, const char **spec);

/**
 * Set format of messages on output IFC.
 *
 * This function is thread safe.
 *
 * \param[in,out] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] out_ifc_idx   Index of output IFC.
 * \param[in] data_type     Format of messages defined by #trap_data_format_t.
 * \param[in] ...   If data_type is TRAP_FMT_UNIREC or TRAP_FMT_JSON, additional parameter
 * that specifies template (char *) is expected.
 */
void trap_ctx_set_data_fmt(trap_ctx_t *ctx, uint32_t out_ifc_idx, uint8_t data_type, ...);

/**
 * Set format of messages on output IFC.
 *
 * This function is thread safe.
 *
 * \param[in,out] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] out_ifc_idx   Index of output IFC.
 * \param[in] data_type     Format of messages defined by #trap_data_format_t.
 * \param[in] ap   If data_type is TRAP_FMT_UNIREC or TRAP_FMT_JSON, additional parameter
 * that specifies template (char *) is expected.
 */
void trap_ctx_vset_data_fmt(trap_ctx_t *ctx, uint32_t out_ifc_idx, uint8_t data_type, va_list ap);

/**
 * Returns current state of an input interface on specified index.
 *
 * This function is thread safe.
 *
 * \param[in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifc_idx   Index of the input interface
 * \return Value of #trap_in_ifc_state_t on success, otherwise TRAP_E_NOT_INITIALIZED when libtrap context is not initialized or
 * TRAP_E_BAD_IFC_INDEX (ifc_idx >= number of input ifcs).
 */
int trap_ctx_get_in_ifc_state(trap_ctx_t *ctx, uint32_t ifc_idx);

/**
 * Set format of messages expected on input IFC.
 *
 * This function is thread safe.
 *
 * \param[in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] in_ifc_idx   Index of input IFC.
 * \param[in] data_type     Format of messages defined by #trap_data_format_t.
 * \param[in] ...   If data_type is TRAP_FMT_UNIREC or TRAP_FMT_JSON, additional parameter
 * that specifies template (char *) is expected.
 * \return TRAP_E_OK on success, on error see #trap_ctx_vset_required_fmt().
 */
int trap_ctx_set_required_fmt(trap_ctx_t *ctx, uint32_t in_ifc_idx, uint8_t data_type, ...);

/**
 * Set format of messages expected on input IFC.
 *
 * This function is thread safe.
 *
 * \param[in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] in_ifc_idx   Index of input IFC.
 * \param[in] data_type     Format of messages defined by #trap_data_format_t.
 * \param[in] ap   If data_type is TRAP_FMT_UNIREC or TRAP_FMT_JSON, additional parameter
 * that specifies template (char *) is expected.
 * \return TRAP_E_OK on success, TRAP_E_NOT_INITIALIZED when libtrap context is not initialized, TRAP_E_BAD_IFC_INDEX or TRAP_E_BADPARAMS on error.
 */
int trap_ctx_vset_required_fmt(trap_ctx_t *ctx, uint32_t in_ifc_idx, uint8_t data_type, va_list ap);

/**
 * Get message format and template that is set on IFC.
 *
 * On output IFC it should return the values that were set.  On input IFC
 * it should return format and template that was received.
 * This function is thread safe.
 *
 * \param[in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifc_dir     #trap_ifc_type direction of interface
 * \param[in] ifc_idx     Index of IFC.
 * \param[out] data_type   Format of messages defined by #trap_data_format_t.
 * \param[out] spec   Specifier of data format specifies the template (char *) is expected.
 * \return TRAP_E_OK on success, TRAP_E_NOT_INITIALIZED if libtrap context is not initialized or negotiation is not successful yet for input IFC, TRAP_E_BAD_IFC_INDEX or TRAP_E_BADPARAMS on error.
 */
int trap_ctx_get_data_fmt(trap_ctx_t *ctx, uint8_t ifc_dir, uint32_t ifc_idx, uint8_t *data_type, const char **spec);

/**
 * @}
 *//* trap_mess_fmt */

/**
 * Parse Fields name and types from string.
 *
 * Function parses the source string and sets the given pointers (pointers to source string). Than it sets length of name and type
 *
 * \param[in] source Source string to parse.
 * \param[in] name ouput parameter, where will be set the pointer to name of a field (pointer to source string).
 * \param[in] type ouput parameter, where will be set the pointer to type of a field (pointer to source string).
 * \param[in] length_name ouput parameter, where will be set the length of a name.
 * \param[in] length_type ouput parameter, where will be set the length of a type.
 * \return pointer to source string, moved to next field
 */
const char *trap_get_type_and_name_from_string(const char *source, const char **name, const char **type, int *length_name, int *length_type);

/**
 * Compares sender_ifc template and receiver_ifc template
 * and returns whether receivers template is subset of the senders template.
 *
 * \param[in] sender_ifc_data_fmt   sender_ifc template (char *)
 * \param[in] receiver_ifc_data_fmt   receiver_ifc template (char *)
 * \return TRAP_E_OK on success (receivers template is subset of the senders template),
 * TRAP_E_FIELDS_MISMATCH (receivers template has field which is not in senders template).
 */
int trap_ctx_cmp_data_fmt(const char *sender_ifc_data_fmt, const char *receiver_ifc_data_fmt);

/**
 * Returns global context.
 *
 * \return pointer to global context.
 */
void *trap_get_global_ctx();

/**
 * Returns current state of an input interface on specified index.
 *
 * \param[in] ifc_idx   Index of the input interface
 * \return See #trap_ctx_get_in_ifc_state().
 */
int trap_get_in_ifc_state(uint32_t ifc_idx);

/** Parse command-line arguments.
 * Extract arguments needed by TRAP to set up interfaces (-i params), verbosity
 * level (-v/-vv/-vvv) and return the rest (argc and argv are modified, i.e.
 * processed parameter is removed). Extracted information is stored into
 * ifc_spec. These variables should be passed to trap_init. Data in ifc_spec
 * must be freed by trap_free_ifc_spec. If help is requested (-h/--help)
 * TRAP_E_HELP is returned (argc and argv are modified also).
 * @param[in,out] argc Pointer to number of command-line arguments.
 * @param[in,out] argv Command-line arguments.
 * @param[out] ifc_spec Structure with specification of interface types and
 *                      their parameters.
 * @return Error code (0 on success)
 */
int trap_parse_params(int *argc, char **argv, trap_ifc_spec_t *ifc_spec);

/**
 * \brief Splitter of *params* string.
 * Cut the first param, copy it into *dest* and returns pointer to the start of following
 * parameter.
 * \param[in] source  source string, typically *params*
 * \param[out] dest  destination string, target of first paramater copying
 * \param[in] delimiter  separator of values in *params*
 * \return Pointer to the start of following parameter (char after delimiter). \note If NULL, no other parameter is present or error during allocation occured.
 */
char *trap_get_param_by_delimiter(const char *source, char **dest, const char delimiter);

/**
 * \brief Check content of buffer, iterate over message headers
 * \param [in] buffer	start of buffer
 * \param [in] buffer_size	size of buffer
 * \return 0 on success, number of errors otherwise
 */
int trap_check_buffer_content(void *buffer, uint32_t buffer_size);

/**
 * @}
 *//* commonapi */

/*****************************************************************************/
/***************************** Library interface *****************************/

/**
 * \defgroup simpleapi Simple API
 * @{
 */

extern int trap_last_error; ///< Code of last error (one of the codes above)
extern const char *trap_last_error_msg; ///< Human-readable message about last error

/** Destructor of trap_ifc_spec_t structure.
 * @param[in] ifc_spec trap_ifc_spec_t structure to clear.
 * @return  Error code (0 on success)
 */
int trap_free_ifc_spec(trap_ifc_spec_t ifc_spec);

/** Initialization function.
 * Create and initialize all interfaces.
 * @param[in] module_info Pointer to struct containing info about the module.
 * @param[in] ifc_spec Structure with specification of interface types and
 *                      their parameters.
 * @return Error code (0 on success)
 */
int trap_init(trap_module_info_t *module_info, trap_ifc_spec_t ifc_spec);

/** Function to terminate module's operation.
 * This function stops all read/write operations on all interfaces.
 * Any waiting in trap_recv() and trap_send()_data is interrupted and these
 * functions return immediately with TRAP_E_TERMINATED.
 * Any call of trap_recv() or trap_send() after call of this function
 * returns TRAP_E_TERMINATED.
 *
 * This function is used to terminate module's operation (asynchronously), e.g.
 * in SIGTERM handler.
 * @return Always TRAP_E_OK (0).
 */
int trap_terminate();

/** Cleanup function.
 * Disconnect all interfaces and do all necessary cleanup.
 * @return Error code
 */
int trap_finalize();

/** Send data to output interface.
 * Write data of size `size` given by `data` pointer into interface `ifc`.
 * If data cannot be written immediately (e.g. because of full buffer or
 * lost connection), wait until write is possible or `timeout` microseconds
 * elapses. If `timeout` < 0, wait indefinitely.
 * @param[in] ifcidx Index of interface to write into.
 * @param[in] data Pointer to data.
 * @param[in] size Number of bytes of data.
 * @param[in] timeout Timeout in microseconds for non-blocking mode; timeout
 * can be also: TRAP_WAIT, TRAP_HALFWAIT, or TRAP_NO_WAIT.
 * @return Error code - 0 on success, TRAP_E_TIMEOUT if timeout elapses.
 * \deprecated This function should be replaced by trap_send().
 */
int trap_send_data(unsigned int ifcidx, const void *data, uint16_t size, int timeout);

/**
 * \brief Receive data from input interface.
 *
 * Receive a message from interface specified by `ifcidx` and set
 * pointer to the `data`.
 * When function returns due to timeout, contents of `data` and `size` are undefined.
 *
 * @param[in] ifcidx    Index of input IFC.
 * @param[out] data     Pointer to received data.
 * @param[out] size     Size of received data in bytes of data.
 * @return Error code - #TRAP_E_OK on success, #TRAP_E_TIMEOUT if timeout elapses.
 *
 * \note Data must not be freed! Library stores incomming data into static array and rewrites it during every trap_recv() call.
 * \see trap_ifcctl() to set timeout (#TRAPCTL_SETTIMEOUT)
 */
int trap_recv(uint32_t ifcidx, const void **data, uint16_t *size);

/**
 * \brief Send data via output interface.
 *
 * Send a message given by `data` pointer of `size` message size via interface specified by `ifcidx`
 * that is the index of output interfaces (counted from 0).
 *
 * @param[in] ifcidx    Index of input IFC.
 * @param[out] data     Pointer to message to send.
 * @param[out] size     Size of message in bytes.
 * @return Error code - #TRAP_E_OK on success, #TRAP_E_TIMEOUT if timeout elapses.
 *
 * \see trap_ifcctl() to set timeout (#TRAPCTL_SETTIMEOUT)
 */
int trap_send(uint32_t ifcidx, const void *data, uint16_t size);

/** Set verbosity level of library functions.
 * Verbosity levels may be:
 *   - -3 - errors
 *   - -2 - warnings
 *   - -1 - notices (default)
 *   -  0 - verbose
 *   -  1 - more verbose
 *   -  2 - even more verbose
 *
 * @param[in] level Desired level of verbosity.
 */
void trap_set_verbose_level(int level);

/** Get verbosity level.
 * See trap_set_verbose_level for list of levels.
 * @return Verbosity level currently set in the library.
 */
int trap_get_verbose_level();

/** Set section for trap_print_help()
 *
 * \param [in] level  0 for default info about module, 1 for info about IFC specifier
 */
void trap_set_help_section(int level);

/** Print common TRAP help message.
 * The help message contains information from module_info and describes common
 * TRAP command-line parameters.
 * @param[in] module_info Pointer to struct containing info about the module.
 */
void trap_print_help(const trap_module_info_t *module_info);

/** Print help about interface specifier.
 * Prints help message about format of interface specifier and description of
 * all available interface types.
 * This message is normally a part of help printed by trap_print_help, this
 * function is useful when you don't use standard TRAP command-line parameters
 * but you still use the same format of interface specifier.
 */
void trap_print_ifc_spec_help();

/**
 * \brief Control TRAP interface.
 *
 * \note Type and request types were changed from enum because of python wrapper.
 * \param [in] type     #trap_ifc_type direction of interface
 * \param [in] ifcidx   index of TRAP interface
 * \param [in] request  #trap_ifcctl_request type of operation
 * \param [in,out] ...  arguments of request, see #trap_ifcctl_request for more details on requests and their arguments.
 *
 * Examples:
 * \code{C}
 * // set 4 seconds timeout
 * trap_ifcctl(TRAPIFC_INPUT, 0, TRAPCTL_SETTIMEOUT, 4000000);
 * // disable auto-flush
 * trap_ifcctl(TRAPIFC_OUTPUT, 0, TRAPCTL_AUTOFLUSH_TIMEOUT, TRAP_NO_AUTO_FLUSH);
 * // disable buffering
 * trap_ifcctl(TRAPIFC_OUTPUT, 0, TRAPCTL_BUFFERSWITCH, 0);
 * \endcode
 *
 * \return TRAP_E_OK on success
 */
int trap_ifcctl(int8_t type, uint32_t ifcidx, int32_t request, ... /* arg */);

/**
 * \brief Force flush of buffer.
 *
 * \param[in] ifc    IFC Index of interface to write into.
 */
void trap_send_flush(uint32_t ifc);

/**
 * @}
 *//* basic API */

/********** Macros generating pieces of common code **********/

/**
 * \addtogroup contextapi Context API
 *
 * This API allows user to use multiple instances of libtrap in the same process.
 *
 * The API is similar to \ref simpleapi. The difference is in the private context memory
 * that is returned by trap_ctx_init() and is freed by trap_ctx_finalize().
 * Obtained context pointer must be passed to all functions from context API.
 * @{
 */

/**
 * \brief Initialize and return the context of libtrap.
 *
 * This function is thread safe.
 *
 * \param[in] module_info     Pointer to struct containing info about the module.
 * \param[in] ifc_spec        Structure with specification of interface types and
 *                      their parameters.
 * \return Pointer to context (context needs to be checked for error value by trap_ctx_get_last_error() function), NULL on memory error.
 */
trap_ctx_t *trap_ctx_init(trap_module_info_t *module_info, trap_ifc_spec_t ifc_spec);

/**
 * \brief Initialize and return the context of libtrap.
 *
 * This function is thread safe.
 *
 * \param[in] module_info      Pointer to struct containing info about the module.
 * \param[in] ifc_spec         Structure with specification of interface types and their parameters.
 * \param[in] service_ifcname  Identifier of the service IFC (used as a part of path to the UNIX socket). When NULL is used, no service IFC will be opened.
 *
 * \return Pointer to context (context needs to be checked for error value by trap_ctx_get_last_error() function), NULL on memory error.
 */
trap_ctx_t *trap_ctx_init2(trap_module_info_t *module_info, trap_ifc_spec_t ifc_spec, const char *service_ifcname);

/**
 * \brief Initialize and return the context of libtrap.
 *
 * This function is thread safe.
 *
 * \param[in] name   Name of the NEMEA module (libtrap context).
 * \param[in] description - Detailed description of the module, can be NULL ("" will be used in such case)
 * \param[in] i_ifcs Number of input IFCs, it can be -1 if o_ifcs > -1 (-1 means variable number of IFCs, it is then computed from ifc_spec).
 * \param[in] o_ifcs Number of output IFCs, it can be -1 if i_ifcs > -1 (-1 means variable number of IFCs, it is then computed from ifc_spec).
 * \param[in] ifc_spec  IFC_SPEC stringdescribed in README.ifcspec.md
 * \param[in] service_ifcname Identifier of the service IFC (used as a part of path to the UNIX socket). When NULL is used, no service IFC will be opened.
 *
 * \return Pointer to context (context needs to be checked for error value by trap_ctx_get_last_error() function), NULL on memory error.
 */
trap_ctx_t *trap_ctx_init3(const char *name, const char *description, int8_t i_ifcs, int8_t o_ifcs, const char *ifc_spec, const char *service_ifcname);

/**
 * \brief Terminate libtrap context and free resources.
 *
 * This function is thread safe.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (trap_ctx_init()).
 * \return TRAP_E_OK on success.
 */
int trap_ctx_finalize(trap_ctx_t **ctx);

/**
 * \brief Terminate libtrap context.
 *
 * This function is thread safe.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \return TRAP_E_OK on success.
 */
int trap_ctx_terminate(trap_ctx_t *ctx);

/**
 * \brief Read data from input interface.
 *
 * This function is thread safe.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifc    Index of input interface (counted from 0).
 * \param[out] data  Pointer to received data.
 * \param[out] size  Size of received data in bytes.
 *
 * \return Error code - TRAP_E_OK on success, TRAP_E_TIMEOUT if timeout elapses.
 * \see #trap_ctx_ifcctl
 */
int trap_ctx_recv(trap_ctx_t *ctx, uint32_t ifc, const void **data, uint16_t *size);

/**
 * \brief Send data via output interface.
 *
 * Write data of size `size` given by `data` pointer into interface `ifc`.
 * If data cannot be written immediately (e.g. because of full buffer or
 * lost connection), wait until write is possible or `timeout` microseconds
 * elapses. If `timeout` < 0, wait indefinitely.
 * This function is thread safe.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifc    Index of interface to write into.
 * \param[in] data   Pointer to data.
 * \param[in] size   Number of bytes of data.
 * \return Error code - 0 on success, TRAP_E_TIMEOUT if timeout elapses.
 * \see #trap_ctx_ifcctl
 */
int trap_ctx_send(trap_ctx_t *ctx, unsigned int ifc, const void *data, uint16_t size);

/**
 * \brief Set verbosity level of library functions.
 *
 * Verbosity levels may be:
 *   - -3 - errors
 *   - -2 - warnings
 *   - -1 - notices (default)
 *   -  0 - verbose
 *   -  1 - more verbose
 *   -  2 - even more verbose
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] level  Desired level of verbosity.
 */
void trap_ctx_set_verbose_level(trap_ctx_t *ctx, int level);

/**
 * \brief Get verbosity level.
 *
 * \see #trap_set_verbose_level for the list of levels.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \return Verbosity level currently set in the library.
 */
int trap_ctx_get_verbose_level(trap_ctx_t *ctx);

/**
 * \brief Control TRAP interface.
 *
 * This function is thread safe.
 *
 * \note Type and request types were changed from enum because of python wrapper.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param [in] type     #trap_ifc_type direction of interface
 * \param [in] ifcidx   index of TRAP interface
 * \param [in] request  #trap_ifcctl_request type of operation
 * \param [in,out] ...  arguments of request, see #trap_ifcctl_request for more details on requests and their arguments.
 *
 * Examples:
 * \code{C}
 * // set 4 seconds timeout
 * trap_ctx_ifcctl(ctx, TRAPIFC_INPUT, 0, TRAPCTL_SETTIMEOUT, 4000000);
 * // disable auto-flush
 * trap_ctx_ifcctl(ctx, TRAPIFC_OUTPUT, 0, TRAPCTL_AUTOFLUSH_TIMEOUT, TRAP_NO_AUTO_FLUSH);
 * // disable buffering
 * trap_ctx_ifcctl(ctx, TRAPIFC_OUTPUT, 0, TRAPCTL_BUFFERSWITCH, 0);
 * \endcode
 *
 * \return TRAP_E_OK on success
 */
int trap_ctx_ifcctl(trap_ctx_t *ctx, int8_t type, uint32_t ifcidx, int32_t request, ... /* arg */);

/**
 * \brief Control TRAP interface.
 *
 * This function is thread safe.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param [in] type     #trap_ifc_type direction of interface
 * \param [in] ifcidx   index of TRAP interface
 * \param [in] request  #trap_ifcctl_request type of operation
 * \param [in,out] ap  arguments of request.
 * \return TRAP_E_OK on success
 *
 * \see trap_ctx_ifcctl().
 */
int trap_ctx_vifcctl(trap_ctx_t *ctx, int8_t type, uint32_t ifcidx, int32_t request, va_list ap);

/**
 * \brief Get last result code from libtrap context.
 *
 * This function is thread safe.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \return \ref errorcodes
 */
int trap_ctx_get_last_error(trap_ctx_t *ctx);

/**
 * \brief Get last (error) message from libtrap context.
 *
 * This function is thread safe.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \return Text string with last (error) message from libtrap context.
 */
const char *trap_ctx_get_last_error_msg(trap_ctx_t *ctx);

/**
 * \brief Force flush of buffer.
 *
 * This function is thread safe.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifc    IFC Index of interface to write into.
 */
void trap_ctx_send_flush(trap_ctx_t *ctx, uint32_t ifc);

/**
 * \brief Get number of connected clients.
 *
 * Output interface (TCP or UNIX) allows to send messages to multiple clients.  This
 * function reads number of connected clients from internal interface
 * structure.
 *
 * \param[in] ctx    Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] ifcidx IFC Index of output interface.
 * \return Number of connected clients. -1 on error.
 */
int trap_ctx_get_client_count(trap_ctx_t *ctx, uint32_t ifcidx);

/**
 * \brief Create dump files.
 *
 * Create dump files for debug as follows:
 *  trap-i[number]-config.txt   Output interface configuration.
 *  trap-i[number]-buffer.dat    Output interface buffer
 *  trap-o[number]-config.txt  Input interface configuration.
 *  trap-o[number]-buffer.dat   Input interface buffer
 *
 * \param[in] ctx   Pointer to the private libtrap context data (#trap_ctx_init()).
 * \param[in] path  Output directory, if NULL use current working directory.
 */
void trap_ctx_create_ifc_dump(trap_ctx_t *ctx, const char *path);

/**
 * @}
 *//* contextapi */

/**
 * \defgroup modulemacros Nemea module macros
 *
 * Set of preprocessor macros for rapid NEMEA module development.
 * @{
 */
/** \brief Define default signal handler function
 * Defines function to handle SIGTERM and SIGINT signals. When a signal is
 * received, it runs the specified command and calls trap_terminate().
 * Place this macro before your main function.
 * \param[in] stop_cmd Command which stops operation of a module. Usually
 *                     setting a variable which is tested in module's main loop.
 */
#define TRAP_DEFAULT_SIGNAL_HANDLER(stop_cmd) \
   void trap_default_signal_handler(int signal)\
   {\
      if (signal == SIGTERM || signal == SIGINT) { \
         stop_cmd;\
         trap_terminate();\
      }\
   }

/** \brief Register default signal handler.
 * Register function defined by TRAP_DEFAULT_SIGNAL_HANDLER as handler of
 * SIGTERM and SIGINT signals.
 * Place this macro between TRAP initialization and the main loop.
 */
#ifdef HAVE_SIGACTION
#define TRAP_REGISTER_DEFAULT_SIGNAL_HANDLER() \
   do {\
      if (trap_get_verbose_level() >= 1)\
         printf("Setting signal handler for SIGINT and SIGTERM using 'sigaction' function.\n");\
      struct sigaction act;\
      /* Set default signal handler function*/\
      act.sa_handler = trap_default_signal_handler;\
      act.sa_flags = 0;\
      /* Prevent interruption of signal handler by another SIGTERM or SIGINT */\
      sigemptyset(&act.sa_mask);\
      sigaddset(&act.sa_mask, SIGTERM);\
      sigaddset(&act.sa_mask, SIGINT);\
      /* Register signal hander */\
      sigaction(SIGTERM, &act, NULL);\
      sigaction(SIGINT, &act, NULL);\
   } while(0)
#else
#define TRAP_REGISTER_DEFAULT_SIGNAL_HANDLER() \
   do {\
      if (trap_get_verbose_level() >= 1)\
         printf("Setting signal handler for SIGINT and SIGTERM using 'signal' function.\n");\
      signal(SIGTERM, trap_default_signal_handler);\
      signal(SIGINT, trap_default_signal_handler);\
   } while(0)
#endif



/** \brief Initialize TRAP using command-line parameters and handle errors.
 * Generates code that parses command-line parameters using trap_parse_params,
 * intializes TRAP library using trap_init and handle possible errors.
 * It calls exit(1) when an error has occured.
 * Place this macro at the beginning of your main function.
 * \param[in,out] argc Number of command-line parameters.
 * \param[in,out] argv List of command-line parameters.
 * \param[in] module_info trap_module_info_t structure containing information
 *                        about the module.
 */
#define TRAP_DEFAULT_INITIALIZATION(argc, argv, module_info) \
   {\
      trap_ifc_spec_t ifc_spec;\
      int ret = trap_parse_params(&argc, argv, &ifc_spec);\
      if (ret != TRAP_E_OK) {\
         if (ret == TRAP_E_HELP) {\
            trap_print_help(&module_info);\
            FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS) \
            return 0;\
         }\
         trap_free_ifc_spec(ifc_spec);\
         fprintf(stderr, "ERROR in parsing of parameters for TRAP: %s\n", trap_last_error_msg);\
         FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS) \
         return 1;\
      }\
      ret = trap_init(&module_info, ifc_spec);\
      if (ret != TRAP_E_OK) {\
         trap_free_ifc_spec(ifc_spec);\
         fprintf(stderr, "ERROR in TRAP initialization: %s\n", trap_last_error_msg);\
         FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS) \
         return 1;\
      }\
      trap_free_ifc_spec(ifc_spec);\
   }

/** \brief Generate TRAP cleanup code.
 * Only calls trap_finalize function.
 * Place this macro at the end of your main function.
 */
#define TRAP_DEFAULT_FINALIZATION() \
   trap_finalize();

/** \brief Handle possible errors after call to trap_recv().
 * \param[in] ret_code Return code of trap_recv().
 * \param timeout_cmd Command to run when a timeout has occured, e.g. "continue".
 * \param error_cmd Command to run when an error has occured or interface was
 *                  terminated, e.g. "break".
 * \deprecated This macro should be replaced by TRAP_DEFAULT_RECV_ERROR_HANDLING.
 */
#define TRAP_DEFAULT_GET_DATA_ERROR_HANDLING(ret_code, timeout_cmd, error_cmd) \
   if ((ret_code) != TRAP_E_OK) {\
      if ((ret_code) == TRAP_E_TIMEOUT) {\
         timeout_cmd;\
      } else if ((ret_code) == TRAP_E_TERMINATED) {\
         error_cmd;\
      } else if (ret_code == TRAP_E_FORMAT_CHANGED) { \
         /* Nothing to do here, TRAP_E_FORMAT_CHANGED has to be skipped by this macro */ \
         /* (module can perform some special operations with templates after trap_recv() signals format change) */ \
      } else if (ret_code == TRAP_E_FORMAT_MISMATCH) { \
         fprintf(stderr, "trap_recv() error: output and input interfaces data formats or data specifiers mismatch.\n"); \
         error_cmd; \
      } else {\
         fprintf(stderr, "Error: trap_recv() returned %i (%s)\n", (ret_code), trap_last_error_msg);\
         error_cmd;\
      }\
   }

/** \brief Handle possible errors after call to trap_recv.
 * \param[in] ret_code Return code of trap_recv.
 * \param timeout_cmd Command to run when a timeout has occured, e.g. "continue".
 * \param error_cmd Command to run when an error has occured or interface was
 *                  terminated, e.g. "break".
 */
#define TRAP_DEFAULT_RECV_ERROR_HANDLING(ret_code, timeout_cmd, error_cmd) \
   if ((ret_code) != TRAP_E_OK) {\
      if ((ret_code) == TRAP_E_TIMEOUT) {\
         timeout_cmd;\
      } else if ((ret_code) == TRAP_E_TERMINATED) {\
         error_cmd;\
      } else if (ret_code == TRAP_E_FORMAT_CHANGED) { \
         /* Nothing to do here, TRAP_E_FORMAT_CHANGED has to be skipped by this macro */ \
         /* (module can perform some special operations with templates after trap_recv() signals format change) */ \
      } else if (ret_code == TRAP_E_FORMAT_MISMATCH) { \
         fprintf(stderr, "trap_recv() error: output and input interfaces data formats or data specifiers mismatch.\n"); \
         error_cmd; \
      } else {\
         fprintf(stderr, "Error: trap_recv() returned %i (%s)\n", (ret_code), trap_last_error_msg);\
         error_cmd;\
      }\
   }


/** \brief Handle possible errors after call to trap_send_data.
 * \param[in] ret_code Return code of trap_send_data.
 * \param timeout_cmd Command to run when a timeout has occured, e.g. "0" to
 *                    do nothing.
 * \param error_cmd Command to run when an error has occured or interface was
 *                  terminated, e.g. "break".
 * \deprecated This macro should be replaced by TRAP_DEFAULT_SEND_ERROR_HANDLING.
 */
#define TRAP_DEFAULT_SEND_DATA_ERROR_HANDLING(ret_code, timeout_cmd, error_cmd) \
   if ((ret_code) != TRAP_E_OK) {\
      if ((ret_code) == TRAP_E_TIMEOUT) {\
         timeout_cmd;\
      } else if ((ret_code) == TRAP_E_TERMINATED) {\
         error_cmd;\
      } else {\
         fprintf(stderr, "Error: trap_send_data() returned %i (%s)\n", (ret_code), trap_last_error_msg);\
         error_cmd;\
      }\
   }

/** \brief Handle possible errors after call to trap_send.
 * \param[in] ret_code Return code of trap_send.
 * \param timeout_cmd Command to run when a timeout has occured, e.g. "0" to
 *                    do nothing.
 * \param error_cmd Command to run when an error has occured or interface was
 *                  terminated, e.g. "break".
 */
#define TRAP_DEFAULT_SEND_ERROR_HANDLING(ret_code, timeout_cmd, error_cmd) \
   if ((ret_code) != TRAP_E_OK) {\
      if ((ret_code) == TRAP_E_TIMEOUT) {\
         timeout_cmd;\
      } else if ((ret_code) == TRAP_E_TERMINATED) {\
         error_cmd;\
      } else {\
         fprintf(stderr, "Error: trap_send() returned %i (%s)\n", (ret_code), trap_last_error_msg);\
         error_cmd;\
      }\
   }
/**
 * @}
 *//* modulemacros */



/**
 * Function handles output interface negotiation (sends hello message to input interface with its data format
 * and data specifier). Hello message contains message header (data format and data specifier size) and data specifier.
 *
 * \param[in] ifc_priv_data  Pointer to output interface private structure.
 * \param[in] ifc_type  Type of IFC, e.g. TRAP_IFC_TYPE_FILE, TRAP_IFC_TYPE_TCPIP, or TRAP_IFC_TYPE_UNIX.
 * \param[in] client_idx  Index of new connected client.
 *
 * \return NEG_RES_FAILED if sending the data to input interface fails,
 *             NEG_RES_FMT_UNKNOWN if the output interface has not specified data format,
 *             NEG_RES_OK signaling success (hello message successfully sent to input interface).
 */
int output_ifc_negotiation(void *ifc_priv_data, char ifc_type, uint32_t client_idx);


/**
 * Function handles input interface negotiation (receives hello message from output interface with its data format
 * and data specifier and compares it with its own data format and data specifier).
 * Hello message contains message header (data format and data specifier size) and data specifier.
 *
 * \param[in,out] ifc_priv_data Pointer to input interface private structure.
 * \param[in] ifc_type  Type of IFC, e.g. TRAP_IFC_TYPE_FILE, TRAP_IFC_TYPE_TCPIP, or TRAP_IFC_TYPE_UNIX.
 *
 * \return NEG_RES_FAILED if receiving the data from output interface fails,
 *             NEG_RES_FMT_UNKNOWN if the output interface has not specified data format,
 *             NEG_RES_FMT_SUBSET if the data format of input and output interfaces is the same and data specifier of the input interface is subset of the output interface data specifier,
 *             NEG_RES_CONT if the data format and data specifier of input and output interface are the same (input interface can receive the data for module right after the negotiation),
 *             NEG_RES_FMT_MISMATCH if the data format or data specifier of input and output interfaces does not match.
 */
int input_ifc_negotiation(void *ifc_priv_data, char ifc_type);


#ifdef __cplusplus
} // extern "C"
#endif

#endif
