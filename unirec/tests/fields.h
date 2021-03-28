#ifndef _UR_FIELDS_H_
#define _UR_FIELDS_H_

/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/
#include <unirec/unirec.h>

#define F_DST_IP   0
#define F_DST_IP_T   ip_addr_t
#define F_IP   1
#define F_IP_T   ip_addr_t
#define F_SRC_IP   2
#define F_SRC_IP_T   ip_addr_t
#define F_BYTES   3
#define F_BYTES_T   uint64_t
#define F_TIMEs   4
#define F_TIMEs_T   ur_time_t*
#define F_BAR   5
#define F_BAR_T   uint32_t
#define F_FOO   6
#define F_FOO_T   uint32_t
#define F_PACKETS   7
#define F_PACKETS_T   uint32_t
#define F_DST_PORT   8
#define F_DST_PORT_T   uint16_t
#define F_SRC_PORT   9
#define F_SRC_PORT_T   uint16_t
#define F_PROTOCOL   10
#define F_PROTOCOL_T   uint8_t
#define F_MESSAGE   11
#define F_MESSAGE_T   char
#define F_MESSAGE_CMP   12
#define F_MESSAGE_CMP_T   char
#define F_STR1   13
#define F_STR1_T   char
#define F_STR2   14
#define F_STR2_T   char
#define F_URL   15
#define F_URL_T   char
#define F_ARR1   16
#define F_ARR1_T   int32_t*
#define F_MACs   17
#define F_MACs_T   mac_addr_t*
#define F_ARR2   18
#define F_ARR2_T   uint64_t*
#define F_IPs   19
#define F_IPs_T   ip_addr_t*

extern uint16_t ur_last_id;
extern ur_static_field_specs_t UR_FIELD_SPECS_STATIC;
extern ur_field_specs_t ur_field_specs;

#endif

