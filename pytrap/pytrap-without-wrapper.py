#!/usr/bin/env python3

import sys
import pytrap


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
            recTmpl = pytrap.UnirecTemplate(ctx.getDataFmt(0)[1])
            a = e.data
            del(e)
        recTmpl.setData(a)
        if recTmpl.SRC_PORT == 22 or recTmpl.DST_PORT == 22:
            numport += 1
        if len(a) <= 1:
            break
        num = num + 1
    print("SSH src port count: {}".format(numport))
    print("Total flows processed: {}".format(num))

mainLoop()

ctx.finalize()



