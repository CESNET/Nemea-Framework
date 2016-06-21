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

def mainLoop():
    global UR_Flow
    for i in range(9000000):
        try:
            a = ctx.recv(0)
        except pytrap.FormatChanged as e:
            UR_Flow = unirec.CreateTemplate("UR_Flow", ctx.getDataFmt(0)[1])
            a = e.data
            del(e)
        rec = UR_Flow(a)


#import cProfile
#cProfile.run('mainLoop()')
mainLoop()

ctx.finalize()


