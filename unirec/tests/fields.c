/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/
// Tables are indexed by field ID
#include "fields.h"

char *ur_field_names_static[] = {
   "DST_IPn",
   "IPn",
   "SRC_IPn",
   "BYTESn",
   "BARn",
   "FOOn",
   "PACKETSn",
   "DST_PORTn",
   "SRC_PORTn",
   "PROTOCOLn",
   "STR1n",
   "STR2n",
   "URLn",
};
short ur_field_sizes_static[] = {
   16, /* DST_IPn */
   16, /* IPn */
   16, /* SRC_IPn */
   8, /* BYTESn */
   4, /* BARn */
   4, /* FOOn */
   4, /* PACKETSn */
   2, /* DST_PORTn */
   2, /* SRC_PORTn */
   1, /* PROTOCOLn */
   , /* STR1n */
   , /* STR2n */
   , /* URLn */
};
ur_field_type_t ur_field_types_static[] = {
   UR_TYPE_IP, /* DST_IPn */
   UR_TYPE_IP, /* IPn */
   UR_TYPE_IP, /* SRC_IPn */
   UR_TYPE_UINT64, /* BYTESn */
   UR_TYPE_UINT32, /* BARn */
   UR_TYPE_UINT32, /* FOOn */
   UR_TYPE_UINT32, /* PACKETSn */
   UR_TYPE_UINT16, /* DST_PORTn */
   UR_TYPE_UINT16, /* SRC_PORTn */
   UR_TYPE_UINT8, /* PROTOCOLn */
   , /* STR1n */
   , /* STR2n */
   , /* URLn */
};
ur_static_field_specs_t UR_FIELD_SPECS_STATIC = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, 13};
ur_field_specs_t ur_field_specs = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, 13, 13, 13, NULL, UR_UNINITIALIZED};
