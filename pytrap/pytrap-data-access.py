# coding: utf-8
import sys
import pdb
import pytrap

ctx = pytrap.TrapCtx()
ctx.init(sys.argv)
ctx.setRequiredFmt(0)

class URWrapper:
    def __init__(self, tmpl, spec):
        self._tmpl = tmpl
        d = [v[(v.find(" ")+1):] for v in spec.split(",")]
        self._urdict = dict(zip(d, range(len(d))))
        self._numfields = len(self._urdict)

    def setData(self, data):
        self._data = data

    def __iter__(self):
        for i in range(self._numfields):
            yield self._tmpl.get(i, self._data)

    def __getattr__(self, a):
        return self._tmpl.get(self._urdict[a], self._data)

    def __len__(self):
        return self._numfields

print("\nReceiving one UniRec message")
try:
    a = ctx.recv(0)
except pytrap.FormatChanged as e:
    fmt = ctx.getDataFmt(0)
    u = pytrap.UnirecTemplate(fmt[1])
    rec = URWrapper(u, fmt[1])
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

ctx.finalize()

