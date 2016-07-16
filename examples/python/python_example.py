#!/usr/bin/python

import pytrap
import sys 

c = pytrap.TrapCtx()
c.init(sys.argv, 1, 1)
c.setRequiredFmt(0)
while True:
    try:
        data = c.recv()
    except pytrap.FormatChanged as e:
        fmttype, fmtspec = c.getDataFmt(0)
        c.setDataFmt(0, fmttype, fmtspec)
        rec = pytrap.UnirecTemplate(fmtspec)
        data = e.data
    if len(data) <= 1:
        break
    rec.setData(data)
    print(rec.strRecord())
    c.send(data)
c.finalize()
