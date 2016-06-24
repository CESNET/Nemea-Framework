# coding: utf-8
import sys
import pdb
import pytrap

ctx = pytrap.TrapCtx()
ctx.init(sys.argv)
ctx.setRequiredFmt(0)
u = None
numfields = 0

def mainLoop():
    global u
    for i in range(9000000):
        try:
            a = ctx.recv(0)
        except pytrap.FormatChanged as e:
            fmt = ctx.getDataFmt(0)
            u = pytrap.UnirecTemplate(fmt[1])
            numfields = len(fmt[1].split(","))
            a = e.data
            del(e)
        for i in range(numfields):
            v = u.get(i, a)


mainLoop()
print("finished")
ctx.finalize()

