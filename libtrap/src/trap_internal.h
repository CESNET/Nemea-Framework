/**
 * \file trap_internal.h
 * \brief Internal functions and macros for libtrap
 * Verbose and debug macros from libcommlbr
 * \author Tomas Konir <Tomas.Konir@liberouter.org>
 * \author Milan Kovacik <xkovaci1@fi.muni.cz>
 * \author Vojtech Krmicek <xkrmicek@fi.muni.cz>
 * \author Juraj Blaho <xblaho00@stud.fit.vutbr.cz>
 * \author Tomas Cejka <cejkat@cesnet.cz>
 * \author Tomas Jansky <janskto1@fit.cvut.cz>
 * \date 2006-2018
 *
 * Copyright (C) 2006-2018 CESNET
 *
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 * may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
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
#ifndef _TRAP_INTERNAL_H
#define _TRAP_INTERNAL_H
#include <config.h>
#include <stdio.h>
#include <pthread.h>
#include "../include/libtrap/trap.h"
#include "trap_ifc.h"

#define MAX_ERROR_MSG_BUFF_SIZE 1024

/**
 * Max length of line printed in help (used for line-breaks).
 */
#define DEFAULT_MAX_TERMINAL_WIDTH 85

/* Values of commands that supervisor wants module to perform. These values are sent in header via service interface. */
#define SERVICE_GET_COM 10  ///< Signaling a request for module statistics (interfaces stats - received messages and buffers, sent messages and buffers, autoflushes counter)
#define SERVICE_SET_COM 11  ///< Signaling a request to set some interface parameters (timeouts etc.)
#define SERVICE_OK_REPLY 12  ///< A value used as a reply signaling success

/**
 * \defgroup negotiationretvals Negotiation return values
 * @{*/

/* Input interface negotiation return values */
#define NEG_RES_CONT 111  ///< If the data format and data specifier of input and output interface are the same (input interface can receive the data for module right after the negotiation)
#define NEG_RES_RECEIVER_FMT_SUBSET 112  ///< If the data format of input and output interfaces is the same and data specifier of the input interface is subset of the output interface data specifier
#define NEG_RES_SENDER_FMT_SUBSET 116  ///< If the data format of input and output interfaces is the same and new data specifier of the output interface is subset of the old one (it is not first negotiation)
#define NEG_RES_FMT_MISMATCH 113  ///< If the data format or data specifier of input and output interfaces does not match
#define NEG_RES_FMT_CHANGED 117 ///< If the data format has changed (for JSON type, UNIREC type uses *SUBSET variants)

/* Output interface negotiation return values */
#define NEG_RES_OK 116  ///< Signaling success (hello message successfully sent to input interface)

/* Return values of input and output interface negotiations */
#define NEG_RES_FAILED 114  ///< If receiving the data from output interface fails or sending the data to input interface fails
#define NEG_RES_FMT_UNKNOWN 115  ///< If the output interface has not specified data format
/**@}*/

/** @defgroup debug Macros for verbose and debug listings
 * @{
 */

/*! control debug level */
extern int trap_debug;
/*! control verbose level */
extern int trap_verbose;
/*! buffer for verbose and debug messages */
char trap_err_msg[4096];

typedef struct trap_ctx_priv_s trap_ctx_priv_t;

extern trap_ctx_priv_t * trap_glob_ctx;

/**
 * Hello message header structure (used during the output and input interface negotiation).
 * Contains data format and data specifier size of the output interface which is making the negotiation.
 */
typedef struct hello_msg_header_s {
   uint8_t data_type;
   uint32_t data_fmt_spec_size;
} hello_msg_header_t;


/*!
\brief VERBOSE/MSG levels
*/
typedef enum trap_verbose_level {
   CL_ERROR = -3, /*!< Inforamtion about error*/
   CL_WARNING = -2, /*< Warining message */
   CL_VERBOSE_OFF = -1,/*!< Verbose off / print even if VERBOSE is off*/
   CL_VERBOSE_BASIC,/*!< Basic verbose information*/
   CL_VERBOSE_ADVANCED,/*!< Advanced verbose information*/
   CL_VERBOSE_LIBRARY/*!< Used for library functions verbose*/
} trap_verbose_level_t;

/**
 * \name Timeouts handling
 * @{*/
#define TRAP_NO_IFC_SLEEP 4 ///< seconds to sleep, when autoflushing is not active
#define TRAP_IFC_TIMEOUT 2000000 ///< size of default timeout on output interfaces in microseconds
/**@}*/

#ifndef NDEBUG
   /*! \brief Debug message macro if DEBUG macro is defined
    *
    * now 3 known DEBUG LEVELS
    *    - 0 normal debug messages
    *    - 1 extended debug (messages on enter and before exit important functions)
    *    - 2 flood developer with debug messages :-) (don't use)
    *
    * BE VERBOSE AND DEBUG ARE DIFFERENT OPTIONS !!!
   */
#  define MSG(level,format,args...) if (trap_debug >= level) {snprintf(trap_err_msg, 4095, format, ##args); debug_msg(level,trap_err_msg);}

   /*! \brief Debug message macro if DEBUG macro is defined - without new
    * line */
#  define MSG_NONL(level,format,args...) if (trap_debug >= level) {snprintf(trap_err_msg, 4095, format, ##args); debug_msg_nonl(trap_err_msg);}
   /*! Prints line in source file if DEBUG macro is defined */
#  define LINE() {fprintf(stderr, "file: %s, line: %i\n", __FILE__, __LINE__); fflush(stderr);}

#define INLINE
#else

# ifdef __GNUC__
static inline int __attribute__ ((format (printf, 2, 3))) MSG(int l, const char *fmt, ...)
{
   (void)(l + fmt);
   return 0;
}
# else
#  define MSG(level,string,args...)
# endif

# define LINE()
#define INLINE inline
#endif

void trap_verbose_msg(int level, char *string);

#ifndef NDEBUG
/*! Macro for verbose message */
#define VERBOSE(level,format,args...) if (trap_verbose>=level) { \
   snprintf(trap_err_msg,4095,"%s:%d "format,__FILE__, __LINE__, ##args); \
   trap_verbose_msg(level,trap_err_msg); \
}
#else
#define VERBOSE(level,format,args...) if (trap_verbose>=level) { \
   snprintf(trap_err_msg,4095,format, ##args); \
   trap_verbose_msg(level,trap_err_msg); \
}
#endif


#define DEBUG_IFC(X) if (TRAP_DEBUG_IFC) { \
   X; \
}

#define DEBUG_BUF(X) if (TRAP_DEBUG_BUFFERING) { \
   X; \
}


/** @} */

/**
 * List of threads and their semaphores.
 *
 * It is used for multi-result reading when _get_data() is called
 * with ifc_mask including more than one ifc.
 */
struct reader_threads_s {
   pthread_t thr;    /**< thread of reader */
   sem_t sem;        /**< semaphore used when thread is ought to sleep */
};

/**
 * \brief List of autoflush timeouts of output interfaces.
 */
typedef struct autoflush_timeouts {
   int idx;            /**< Index of output interface. */
   int64_t tm;         /**< Autoflush timeout to be elapsed. */
   int64_t tm_backup;  /**< Backup value of the autoflush timeout. */
} ifc_autoflush_t;

/**
 * Libtrap context structure.
 *
 * It contains the whole context of one instance of libtrap.  The context
 * contains arrays of communication interfaces (IFC), buffers, locks/mutexes.
 */
struct trap_ctx_priv_s {
   /**
    * Is libtrap initialized correctly? (0 ~ false)
    */
   int initialized;
   /**
    * Is libtrap terminated? (0 ~ false, should run)
    */
   volatile int terminated;

   /**
    * Number of interface changes waiting to be applied.
    */
   volatile int ifc_change;

   /**
    * Code of last error (one of the codes above)
    */
   int trap_last_error;

   /**
    * Human-readable message about last error
    */
   const char *trap_last_error_msg;

   /**
    * Buffer for dynamically generated messages
    */
   char error_msg_buffer[MAX_ERROR_MSG_BUFF_SIZE];

   /**
    * Array of input interfaces
    */
   trap_input_ifc_t  *in_ifc_list;

   /**
    * Arrays of output interfaces
    */
   trap_output_ifc_t *out_ifc_list;

   /**
    * Number of input interfaces
    */
   uint32_t num_ifc_in;

   /**
    * Number of output interfaces
    */
   uint32_t num_ifc_out;

   /**
    * Timeout common to all readers for multiread feature
    */
   int get_data_timeout;

   /**
    * Lock setting last error code and last error message.
    */
   pthread_mutex_t error_mtx;

   /**
    * Timeouts for autoflush thread.
    */
   ifc_autoflush_t *ifc_autoflush_timeout;

   /**
    * Service thread that enables communication with module
    */
   pthread_t service_thread;

   /**
    * Name of the service IFC socket, it is disabled when NULL.
    */
   char *service_ifc_name;

   /**
    * Indicator of initialized service thread
    */
   int service_thread_initialized;

   /**
    * \defgroup ctxifccounters IFC counters
    *
    * The counters are sent by service_thread_routine() via service IFC (e.g. to supervisor).
    * @{
    */
   /**
    * counter_send_message is incremented within trap_ctx_send().
    */
   uint64_t *counter_send_message;
   /**
    * counter_dropped_message is incremented within trap_ctx_send().
    */
   uint64_t *counter_dropped_message;
   /**
    * counter_recv_message is incremented within trap_ctx_recv().
    */
   uint64_t *counter_recv_message;
   /**
    * counter_send_buffer is incremented within trap_store_into_buffer() after sending buffer.
    */
   uint64_t *counter_send_buffer;
   /**
    * counter_autoflush is incremented within trap_automatic_flush_thr() after flushing buffer.
    */
   uint64_t *counter_autoflush;
   /**
    * counter_recv_buffer is incremented within trap_read_from_buffer() after successful receiving buffer.
    */
   uint64_t *counter_recv_buffer;
   /**
    * counter_recv_delay_last represents time interval between last two recv() calls (in microseconds).
    */
   uint64_t *counter_recv_delay_last;
   /**
    * counter_recv_delay_total represents total time spent outside of recv() function (in microseconds).
    */
   uint64_t *counter_recv_delay_total;
   /**
    * recv_delay_timestamp is used to determine the time that has elapsed between last two recv() calls
    */
   uint64_t *recv_delay_timestamp;
   /**
    * @}
    */
};

/**
 * Number of counter types sf IN IFC stored #trap_ctx_priv_s used in service_thread_routine()
 */
#define TRAP_IN_IFC_COUNTERS   1
/**
 * Number of counter types sf OUT IFC stored #trap_ctx_priv_s used in service_thread_routine()
 */
#define TRAP_OUT_IFC_COUNTERS   3

trap_ctx_priv_t *trap_create_ctx_t();

struct trap_buffer_header_s {
   uint32_t data_length;  /**< size of data in the data unit */
#ifdef ENABLE_HEADER_TIMESTAMP
   uint64_t timestamp;
#endif
   uint8_t data[0];
} __attribute__ ((__packed__));
typedef struct trap_buffer_header_s trap_buffer_header_t;

#ifndef ATOMICOPS
_Bool __sync_bool_compare_and_swap_8(int64_t *ptr, int64_t oldvar, int64_t newval);

uint64_t __sync_fetch_and_add_8(uint64_t *ptr, uint64_t value);

uint64_t __sync_add_and_fetch_8(uint64_t *ptr, uint64_t value);

uint64_t __sync_or_and_fetch_8(uint64_t *ptr, uint64_t value);

uint64_t __sync_and_and_fetch_8(uint64_t *ptr, uint64_t value);

#endif

#endif

