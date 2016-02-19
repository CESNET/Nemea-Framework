# ur_types.py - Definitions of UniRec data types and sizes
# Author: Tomas Cejka (cejkat@cesnet.cz), 2015

from __future__ import absolute_import
from collections import namedtuple
from unirec.ur_ipaddr import *
from unirec.ur_time import Timestamp

FieldSpec = namedtuple("FieldSpec", "size python_type struct_type")

size_table = {
   b"char" : 1,
   b"uint8" : 1,
   b"int8" : 1,
   b"uint16" : 2,
   b"int16" : 2,
   b"uint32" : 4,
   b"int32" : 4,
   b"uint64" : 8,
   b"int64" : 8,
   b"float" : 4,
   b"double" : 8,
   b"ipaddr" : 16,
   b"time" : 8,
   b"string" : -1,
   b"bytes" : -1,
}

python_types = {
   b"char" : (str, "c"),
   b"uint8" : (int, "B"),
   b"int8" : (int, "b"),
   b"uint16" : (int, "H"),
   b"int16" : (int, "h"),
   b"uint32" : (int, "i"),
   b"int32" : (int, "I"),
   b"uint64" : (int, "q"),
   b"int64" : (int, "Q"),
   b"float" : (float, "f"),
   b"double" : (float, "d"),
   b"ipaddr" : (IPAddr, "16s"),
   b"time" : (Timestamp, "8s"),
   b"string" : (str, "s"),
   b"bytes" : (str, "s"),
}

type_table = {
   b"char" : b"UR_TYPE_CHAR",
   b"uint8" : b"UR_TYPE_UINT8",
   b"int8" : b"UR_TYPE_INT8",
   b"uint16" : b"UR_TYPE_UINT16",
   b"int16" : b"UR_TYPE_INT16",
   b"uint32" : b"UR_TYPE_UINT32",
   b"int32" : b"UR_TYPE_INT32",
   b"uint64" : b"UR_TYPE_UINT64",
   b"int64" : b"UR_TYPE_INT64",
   b"float" : b"UR_TYPE_FLOAT",
   b"double" : b"UR_TYPE_DOUBLE",
   b"ipaddr" : b"UR_TYPE_IP",
   b"time" : b"UR_TYPE_TIME",
   b"string" : b"UR_TYPE_STRING",
   b"bytes" : b"UR_TYPE_BYTES",
}

c_types = {
   b"char" : b"char",
   b"uint8" : b"uint8_t",
   b"int8" : b"int8_t",
   b"uint16" : b"uint16_t",
   b"int16" : b"int16_t",
   b"uint32" : b"uint32_t",
   b"int32" : b"int32_t",
   b"uint64" : b"uint64_t",
   b"int64" : b"int64_t",
   b"float" : b"float",
   b"double" : b"double",
   b"ipaddr" : b"ip_addr_t",
   b"time" : b"time_t",
   b"string" : b"char",
   b"bytes" : b"char",
}

