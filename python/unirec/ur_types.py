# ur_types.py - Definitions of UniRec data types and sizes
# Author: Tomas Cejka (cejkat@cesnet.cz), 2015

from collections import namedtuple
from ur_ipaddr import *
from ur_time import Timestamp

FieldSpec = namedtuple("FieldSpec", "size python_type struct_type")

size_table = {
   "char" : 1,
   "uint8" : 1,
   "int8" : 1,
   "uint16" : 2,
   "int16" : 2,
   "uint32" : 4,
   "int32" : 4,
   "uint64" : 8,
   "int64" : 8,
   "float" : 4,
   "double" : 8,
   "ipaddr" : 16,
   "time" : 8,
   "string" : -1,
   "bytes*" : -1,
}

python_types = {
   "char" : (str, "c"),
   "uint8" : (int, "B"),
   "int8" : (int, "b"),
   "uint16" : (int, "H"),
   "int16" : (int, "h"),
   "uint32" : (int, "i"),
   "int32" : (int, "I"),
   "uint64" : (int, "q"),
   "int64" : (int, "Q"),
   "float" : (float, "f"),
   "double" : (float, "d"),
   "ipaddr" : (IPAddr, "16s"),
   "time" : (Timestamp, "8s"),
   "string" : (str, "s"),
   "bytes*" : (str, "s"),
}


size_table = {
   "char" : 1,
   "uint8" : 1,
   "int8" : 1,
   "uint16" : 2,
   "int16" : 2,
   "uint32" : 4,
   "int32" : 4,
   "uint64" : 8,
   "int64" : 8,
   "float" : 4,
   "double" : 8,
   "ipaddr" : 16,
   "time" : 8,
   "string" : -1,
   "bytes*" : -1,
}

type_table = {
   "char" : "UR_TYPE_CHAR",
   "uint8" : "UR_TYPE_UINT8",
   "int8" : "UR_TYPE_INT8",
   "uint16" : "UR_TYPE_UINT16",
   "int16" : "UR_TYPE_INT16",
   "uint32" : "UR_TYPE_UINT32",
   "int32" : "UR_TYPE_INT32",
   "uint64" : "UR_TYPE_UINT64",
   "int64" : "UR_TYPE_INT64",
   "float" : "UR_TYPE_FLOAT",
   "double" : "UR_TYPE_DOUBLE",
   "ipaddr" : "UR_TYPE_IP",
   "time" : "UR_TYPE_TIME",
   "string" : "UR_TYPE_STRING",
   "bytes*" : "UR_TYPE_BYTES",
}

c_types = {
   "char" : "char",
   "uint8" : "uint8_t",
   "int8" : "int8_t",
   "uint16" : "uint16_t",
   "int16" : "int16_t",
   "uint32" : "uint32_t",
   "int32" : "int32_t",
   "uint64" : "uint64_t",
   "int64" : "int64_t",
   "float" : "float",
   "double" : "double",
   "ipaddr" : "ip_addr_t",
   "time" : "time_t",
   "string" : "char",
   "bytes*" : "char",
}

