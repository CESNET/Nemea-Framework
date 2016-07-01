import pytrap

a = pytrap.UnirecTime(1467376043, 666)
dt = a.toDatetime()
print(dt)
print(dt.strftime('%Y-%m-%dT%H:%M:%SZ'))
import pdb
pdb.set_trace()

