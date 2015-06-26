/**
 * \file real_time_sending.h
 * \brief Delaying sending of records according to their original timestamps
 * \author Pavel Krobot <xkrobo01@cesnet.cz>
 * \date 2014
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
#ifndef _REAL_TIME_SENDING_
#define _REAL_TIME_SENDING_

#include <sys/time.h>

#define RT_PAR_SET_DEFAULT         0

#define DEFAULT_POOL_SIZE           10
#define DEFAULT_INIT_TS_CNT         1000
#define DEFAULT_SAMPLE_RATE         100
#define DEFAULT_TS_DIFF_THRESHOLD   3.5

/** \brief State of real-time delaying (for real-time sending).
 */
typedef struct rt_state_s {
   uint32_t *mins; // pool for n minimal timestamps to compute average minimum
	uint16_t min_pool_size; // size of pool for minimal timestamps (n from above)
	uint16_t act_min_cnt;
	uint16_t sample_rate;

   uint64_t init_ts_count; // from how many first flows should be minimal timestamp determined

   struct timeval start;
   struct timeval end;
	uint32_t init_timestamp;
	uint32_t ts_diff_cnt;
   long ts_diff_sum;
   float ts_diff_total;

   float ts_diff_threshold; // holds "magic" value which represents change in timestamps of records in real traffic
} rt_state_t;

/** \brief Initialization of real-time delaying (sending) state.
 * If some initialization value is equal to RT_PAR_SET_DEFAULT, according
 * parameter is then set to default value. First and last parameter is mandatory.
 * \param[in] rt_state Name of identifier, which holds real-time delaying state structure.
 * \param[in] pool_size Size of pool for minimal timestamps to compute average minimal timestamp.
 * \param[in] init_timestamp_count From how many first flows should be minimal timestamp determined.
 * \param[in] par_sample_rate Sets initial sample rate - says how offten should be timestamp difference checked.
 * \param[in] timestamp_diff_threshold  "magic" value which represents change in timestamps of records in real traffic.
 * \param[in] err_command Command, which should be executed if malloc fails.
 */
#define RT_INIT(rt_state, pool_size, init_timestamp_count, par_sample_rate, timestamp_diff_threshold, err_command) \
   do { \
      if (pool_size == RT_PAR_SET_DEFAULT){ \
         rt_state.min_pool_size = DEFAULT_POOL_SIZE;\
      } else { \
         rt_state.min_pool_size = pool_size;\
      } \
      rt_state.mins = (uint32_t *) malloc (rt_state.min_pool_size * sizeof(uint32_t)); \
      if (rt_state.mins == NULL){ \
         err_command; \
      } \
      rt_state.act_min_cnt = 0; \
      if (init_timestamp_count == RT_PAR_SET_DEFAULT){ \
         rt_state.init_ts_count = DEFAULT_INIT_TS_CNT; \
      } else { \
         rt_state.init_ts_count = init_timestamp_count; \
      } \
      if (par_sample_rate == RT_PAR_SET_DEFAULT){ \
         rt_state.sample_rate = DEFAULT_SAMPLE_RATE; \
      } else { \
         rt_state.sample_rate = par_sample_rate; \
      } \
      if (timestamp_diff_threshold == RT_PAR_SET_DEFAULT){ \
         rt_state.ts_diff_threshold = DEFAULT_TS_DIFF_THRESHOLD; \
      } else { \
         rt_state.ts_diff_threshold = timestamp_diff_threshold; \
      } \
      rt_state.ts_diff_cnt = 0; \
      rt_state.ts_diff_sum = 0; \
      rt_state.ts_diff_total = 0; \
   } while(0)

/** \brief Just frees allocated memory.
 */
#define RT_DESTROY(rt_state) \
   free(rt_state.mins);

/** \brief Main delaying functionality
 * At first it determines initial timestamp from n first flows. Then it checks
 * after each sample if one second of data was send. If it was, then real-time
 * is checked and sending is corrected by sleep (or not to sleep).
 * \param[in] record_counter Count of actually send records.
 * \param[in] actual_timestamp Timestamp of actual record.
 * \param[in] Name of identifier, which holds real-time delaying state structure.
 */
#define RT_CHECK_DELAY(record_counter, actual_timestamp, rt_state) \
do{ \
   /* Get minimal timestamp >> */ \
   if (record_counter < rt_state.init_ts_count){ /* check if we have initial timestamp; initial timestamp is average of <min_pool_size> minimal timestamps */\
      /* first fill pool with first <min_pool_size> timestamp and sort them */ \
      if (rt_state.act_min_cnt < rt_state.min_pool_size){ \
         rt_state.mins[rt_state.act_min_cnt] = actual_timestamp; \
         if (++rt_state.act_min_cnt == rt_state.min_pool_size){ /* array of first timestamps were filled, sort it */ \
            /* simple bubble-sort */ \
            for (int i = 0; i < (rt_state.act_min_cnt - 1); ++i){ \
               for (int j = 0; j < (rt_state.act_min_cnt - i - 1); ++j){ \
                  if (rt_state.mins[j] > rt_state.mins[j+1]){ \
                     uint32_t tmp = rt_state.mins[j+1]; \
                     rt_state.mins[j+1] = rt_state.mins[j]; \
                     rt_state.mins[j] = tmp; \
                  } \
               } \
            } \
            /* array of minimal timestamps is now sorted, minimal at 0 index */ \
         } \
      /* pool with minimal timestamps is full, now add only smaller timestamps */ \
      } else { \
         if (actual_timestamp < rt_state.mins[rt_state.act_min_cnt - 1]){ \
            for (int i = (rt_state.act_min_cnt - 2); i >= 0; --i){ \
               if (actual_timestamp < rt_state.mins[i]){ \
                  rt_state.mins[i + 1] = rt_state.mins[i]; \
                  if (i == 0){ \
                     rt_state.mins[i] = actual_timestamp; \
                  } \
               } else { \
                  rt_state.mins[i + 1] = actual_timestamp; \
                  break; \
               } \
            } \
         } \
      } \
   } else if (record_counter == rt_state.init_ts_count){ \
      /* get average minimal timestamp */ \
      uint64_t tmp_sum = 0; \
      for (int i = 0; i < rt_state.act_min_cnt; ++i){ \
         tmp_sum += rt_state.mins[i]; \
      } \
      rt_state.init_timestamp = tmp_sum / rt_state.act_min_cnt; \
      /* adjust samplre rate if it is same as initial timestamp count - to do not to consider end of sample in next step */ \
      if (!(record_counter % rt_state.sample_rate)){ \
         --rt_state.sample_rate; \
      } \
      /*get actual (real) time */ \
      gettimeofday(&rt_state.start, NULL); \
      /* << Got minimal timestamp now << */ \
      } else { \
         long ts_diff = (long) actual_timestamp - (long) rt_state.init_timestamp; \
         if (ts_diff > 400 || ts_diff < -400){ /* some huge change in input data (like another file) */ \
            rt_state.init_timestamp = actual_timestamp; \
            ts_diff = 0; \
            rt_state.ts_diff_sum = 0; \
            rt_state.ts_diff_cnt = 0; \
         } \
         rt_state.ts_diff_sum += ts_diff; \
         rt_state.ts_diff_cnt++; \
         if (!(record_counter % rt_state.sample_rate)){ /* check real-time sending each <sample_rate>-record */ \
            if (rt_state.ts_diff_cnt){ /* it should be ... */ \
               rt_state.ts_diff_total = ((float) rt_state.ts_diff_sum / (float) rt_state.ts_diff_cnt); \
            } \
            if (rt_state.ts_diff_total > rt_state.ts_diff_threshold){ \
                  /* check if records was sendend in proper time, wait (sleep) if was send too fast */ \
                  gettimeofday(&rt_state.end, NULL); \
                  long rt_diff = ((rt_state.end.tv_sec * 1000000 + rt_state.end.tv_usec) - (rt_state.start.tv_sec * 1000000 + rt_state.start.tv_usec)); \
                  if (rt_diff < 1000000){ \
                     usleep(1000000 - rt_diff); \
                  } \
                  /* clear & update counters */ \
                  rt_state.ts_diff_sum = 0; \
                  rt_state.ts_diff_cnt = 0; \
                  rt_state.init_timestamp++; \
                  /* adjust and correct sample rate */ \
                  float inc_index = (rt_state.ts_diff_total - rt_state.ts_diff_threshold)  / rt_state.ts_diff_threshold; \
                  if (inc_index > 5){ \
                     inc_index = 5; \
                  } else if (inc_index < 0.2){ \
                     inc_index = 0.2; \
                  } \
                  rt_state.sample_rate /= inc_index; \
                  if (rt_state.sample_rate < 100){ \
                     rt_state.sample_rate = 100; \
                  }else if (rt_state.sample_rate > 10000){ \
                     rt_state.sample_rate = 10000; \
                  } \
                  rt_state.ts_diff_total = 0; \
                  gettimeofday(&rt_state.start, NULL); /* get actual (real) time */ \
               } \
            } \
         } \
}while (0)

#endif // _REAL_TIME_SENDING_
