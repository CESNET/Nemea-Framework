# coding: utf-8
import pytrap
a = pytrap.UnirecTemplate("ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,string TEXT,bytes STREAMBYTES")
data = b'\x00\x00\x00\x00\x00\x00\x00\x00\x0A\x00\x00\x01\xff\xff\xff\xff\x01\x00\x00\x00\xe3\x2b\x6c\x57\x00\x00\x00\x01\x00\x00\x00\x02\x06\x00\x04\x00\x00\x00\x06\x00abcdef\xde\xad\xfe\xed'

a.setData(data)

print(len(a))
print(a)
print(a.getFieldsDict())

print("GET 0")
print(a.get(0, data))

print("GET 1")
print(a.get(1, data))

print("GET 2")
print(a.get(2, data))

print("GET 3")
print(a.get(3, data))

print("GET 4")
print(a.get(4, data))
print(a.TEXT)

print("GET 5")
print(type(a.get(5, data)))
stream = a.get(5, data)
print(" ".join([hex(i) for i in stream]))
print(a.STREAMBYTES)

print("UnirecTime")
t = pytrap.UnirecTime(1466701316, 123)
print("t")
print(t)
print(t.getSeconds())
print(t.getMiliSeconds())
print(t.getTimeAsFloat())
print(type(t.getTimeAsFloat()))
t2 = pytrap.UnirecTime(10, 100)
print("t + t2")
print(t + t2)
print("t + 1000000000")
constint = 1000000000
print(type(constint))
print( t + constint)
print("t + -1000000000")
print(t + -constint)
fl = float(t)
print(type(fl))
print(fl)


print("UnirecIPAddr")
t = pytrap.UnirecIPAddr("192.168.0.1")
print(t)


import pdb
#pdb.set_trace()
print("\n\nSetters")

a.set(2, data, int(1234))
print("GET 2")
print(a.get(2, data))
a.set(2, data, int(1))
print("GET 2")
print(a.get(2, data))
a.set(2, data, int(222))
print("GET 2")
print(a.get(2, data))

newtime = pytrap.UnirecTime(1234, 0)
a.set(1, data, newtime)
print("GET 1")
print(a.get(1, data))

newip = pytrap.UnirecIPAddr("192.168.3.1")
a.set(0, data, newip)
print("GET 0")
print(a.get(0, data))

print(a.set(4, data, "ahoj"))
print("GET 4")
print(a.get(4, data))

