#ifndef _UR_VALUES_H_
#define _UR_VALUES_H_

/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/
/* Edit "values" file and run ur_values.sh script to add UniRec values.      */
/*****************************************************************************/

#include <stdint.h>

/** @brief Values names and descriptions
 * It contains a table mapping a value to name and description
 */
typedef struct ur_values_s {
   int32_t value;    ///< Numeric Value
   char *name;       ///< Name of Value
   char *description;///< Description of Value
} ur_values_t;

#define UR_TYPE_START_TCP_FLAGS	0
#define UR_TYPE_END_TCP_FLAGS	6
#define UR_TYPE_START_DIR_BIT_FIELD	6
#define UR_TYPE_END_DIR_BIT_FIELD	8
#define UR_TYPE_START_DIRECTION_FLAGS	8
#define UR_TYPE_END_DIRECTION_FLAGS	12
#define UR_TYPE_START_IPV6_TUN_TYPE	12
#define UR_TYPE_END_IPV6_TUN_TYPE	19
#define UR_TYPE_START_SPOOF_TYPE	19
#define UR_TYPE_END_SPOOF_TYPE	23
#define UR_TYPE_START_EVENT_TYPE	23
#define UR_TYPE_END_EVENT_TYPE	32
#define UR_TYPE_START_TUNNEL_TYPE	32
#define UR_TYPE_END_TUNNEL_TYPE	42
#define UR_TYPE_START_HB_TYPE	42
#define UR_TYPE_END_HB_TYPE	43
#define UR_TYPE_START_HB_DIR	43
#define UR_TYPE_END_HB_DIR	46
#define UR_TYPE_START_HB_ALERT_TYPE_FIELD	46
#define UR_TYPE_END_HB_ALERT_TYPE_FIELD	50
#define UR_TYPE_START_WARDEN_TYPE	50
#define UR_TYPE_END_WARDEN_TYPE	63
#define UR_TYPE_START_HTTP_SDM_REQUEST_METHOD_ID	63
#define UR_TYPE_END_HTTP_SDM_REQUEST_METHOD_ID	71

#endif
