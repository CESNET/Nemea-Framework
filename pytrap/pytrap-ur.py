# coding: utf-8
import pytrap
a = pytrap.UnirecTemplate("ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,string TEXT")
data = b'\x00\x00\x00\x00\x00\x00\x00\x00\x0A\x00\x00\x01\xff\xff\xff\xff\x01\x00\x00\x00\xe3\x2b\x6c\x57\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x06\x00abcdef'

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


print("UnirecTime")
t = pytrap.UnirecTime(1466701316, 123)
print(t)

print("UnirecIPAddr")
t = pytrap.UnirecIPAddr("192.168.0.1")
print(t)

