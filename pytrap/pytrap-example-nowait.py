#!/usr/bin/env python3

import pytrap
import unirec
import sys
import pdb


#UR_Flow = unirec.CreateTemplate("UR_Flow", b"ipaddr DST_IP,ipaddr SRC_IP,double EVENT_SCALE,time TIME_FIRST,time TIME_LAST,uint16 DST_PORT,uint16 SRC_PORT,uint8 EVENT_TYPE,uint8 PROTOCOL")
UR_Flow = unirec.CreateTemplate("UR_Flow", b"double EVENT_SCALE,time TIME_FIRST,time TIME_LAST,uint16 DST_PORT,uint16 SRC_PORT,uint8 EVENT_TYPE,uint8 PROTOCOL")

ctx = pytrap.TrapCtx()

ctx.init(sys.argv)
#pytrap.setVerboseLevel(2)
print(ctx.getVerboseLevel())

ctx.setRequiredFmt(0)
ctx.ifcctl(0, True, 3, 1000000)

try:
    a = ctx.recv(0)
    raise Exception("recv should have raised Timeout exception")
except pytrap.TrapError as e:
    print("Caught Timeout exception: {0}".format(type(e)))


ctx.finalize()



