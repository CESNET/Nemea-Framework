# coding: utf-8
import sys
import pdb
import pytrap

ctx = pytrap.TrapCtx()
ctx.init(sys.argv)
ctx.setRequiredFmt(0)

print("\nReceiving one UniRec message")
try:
    a = ctx.recv(0)
except pytrap.FormatChanged as e:
    fmt = ctx.getDataFmt(0)
    rec = pytrap.UnirecTemplate(fmt[1])
    a = e.data
    del(e)
print(rec)
rec.setData(a)
print("\nDirect access using index")
for i in range(len(rec)):
    print(rec.get(i, a))

print("\nAttribute access")
print(rec.SRC_IP)
for i in ["SRC_IP", "DST_IP", "SRC_PORT", "DST_PORT"]:
    v = getattr(rec, i)
    print(v)

print("\nIteration over all fields")
for i in rec:
    print(i)

print("\nPrint values, ids and names of fields")
print(rec.strRecord())

print("\nDict from all fields")
d = {}
for k, v in rec:
    d[k] = str(v)
print(d)

ctx.finalize()

