#!/usr/bin/env python

import pytrap
import sys 

trap = pytrap.TrapCtx()
trap.init(sys.argv, 1, 1)
trap.setRequiredFmt(0)
while True:
    try:
        data = trap.recv()
    except pytrap.FormatChanged as e:
        fmttype, fmtspec = trap.getDataFmt(0)
        trap.setDataFmt(0, fmttype, fmtspec)
        rec = pytrap.UnirecTemplate(fmtspec)
        data = e.data
    if len(data) <= 1:
        break
    rec.setData(data)
    print(rec.strRecord())
    trap.send(data)
trap.finalize()
