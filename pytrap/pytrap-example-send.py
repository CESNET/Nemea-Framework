#!/usr/bin/env python3

import pytrap
import unirec
import sys
import pdb


ctx = pytrap.TrapCtx()

ctx.init(sys.argv, 0, 1)

UR_Flow = unirec.CreateTemplate("UR_Flow", b"double EVENT_SCALE,uint8 EVENT_TYPE")

rec = UR_Flow()
rec.EVENT_SCALE = 1.234
rec.EVENT_TYPE = 10

ctx.setDataFmt(0, 2, "double EVENT_SCALE,uint8 EVENT_TYPE")

ctx.send(0, rec.serialize())

ctx.finalize()



