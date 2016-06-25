# coding: utf-8
import sys
import pdb
import pytrap
from URWrapper import URWrapper

ctx = pytrap.TrapCtx()
ctx.init(sys.argv)
ctx.setRequiredFmt(0)

print("\nReceiving one UniRec message")
try:
    a = ctx.recv(0)
except pytrap.FormatChanged as e:
    fmt = ctx.getDataFmt(0)
    u = pytrap.UnirecTemplate(fmt[1])
    rec = URWrapper(u)
    a = e.data
    del(e)

rec.setData(a)
print("\nDirect access using index")
for i in range(len(rec)):
    print(u.get(i, a))

print("\nAttribute access")
print(rec.SRC_IP)
for i in ["SRC_IP", "DST_IP", "SRC_PORT", "DST_PORT"]:
    v = rec.__getattr__(i)

print("\nIteration over all fields")
for i in rec:
    print(i)

print("\nPrint values, ids and names of fields")
print(rec.strRecord())


ctx.finalize()

