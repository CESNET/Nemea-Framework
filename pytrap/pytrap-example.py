#!/usr/bin/env python3

import sys
import pytrap
from URWrapper import URWrapper


ctx = pytrap.TrapCtx()
ctx.init(sys.argv)
ctx.setRequiredFmt(0, pytrap.FMT_UNIREC, "ipaddr SRC_IP")

def mainLoop():
    num = 0
    numport = 0
    while True:
        try:
            a = ctx.recv(0)
        except pytrap.FormatChanged as e:
            UR_Flow = pytrap.UnirecTemplate(ctx.getDataFmt(0)[1])
            rec = URWrapper(UR_Flow)
            a = e.data
            del(e)
        rec.setData(a)
        if rec.SRC_PORT == 22 or rec.DST_PORT == 22:
            numport += 1
        if len(a) <= 1:
            break
        num = num + 1
    print("SSH src port count: {}".format(numport))
    print("Total flows processed: {}".format(num))

mainLoop()

ctx.finalize()


