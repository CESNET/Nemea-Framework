import unittest
import doctest
import sys
if sys.version_info > (3,):
    long = int

class DeviceTest(unittest.TestCase):
    def runTest(self):
        try:
            import pytrap
        except ImportError as e:
            self.fail(str(e))

class DataTypesIPAddr(unittest.TestCase):
    def runTest(self):
        import pytrap
        ip1 = pytrap.UnirecIPAddr("192.168.3.1")
        self.assertEqual(ip1, ip1)
        self.assertEqual(type(ip1), pytrap.UnirecIPAddr, "Bad type of IP address object.")
        self.assertEqual(str(ip1),  "192.168.3.1", "IP address is not equal to its str().")
        self.assertEqual(repr(ip1), "UnirecIPAddr('192.168.3.1')", "IP address is not equal to its repr().")

        self.assertTrue(ip1.isIPv4(),  "IPv4 was not recognized.")
        self.assertFalse(ip1.isIPv6(), "IPv4 was recognized as IPv6.")

        ip2 = pytrap.UnirecIPAddr("192.168.0.1")
        ip3 = pytrap.UnirecIPAddr("192.168.3.1")
        self.assertFalse(ip1 == ip2, "Comparison of different IP addresses failed.")
        self.assertFalse(ip1 <= ip2, "Comparison of the same IP addresses failed.")
        self.assertFalse(ip2 >= ip1, "Comparison of the same IP addresses failed.")
        self.assertFalse(ip1 != ip3, "Comparison of the same IP addresses failed.")
        self.assertFalse(ip1  < ip3, "Comparison of the same IP addresses failed.")
        self.assertFalse(ip1  > ip3, "Comparison of the same IP addresses failed.")

        ip1 = pytrap.UnirecIPAddr("fd7c:e770:9b8a::465")
        self.assertEqual(type(ip1), pytrap.UnirecIPAddr, "Bad type of IP address object.")
        self.assertEqual(str(ip1), "fd7c:e770:9b8a::465", "IP address is not equal to its str().")
        self.assertEqual(repr(ip1), "UnirecIPAddr('fd7c:e770:9b8a::465')", "IP address is not equal to its repr().")
        self.assertFalse(ip1.isIPv4(), "IPv6 was not recognized.")
        self.assertTrue(ip1.isIPv6(), "IPv6 was recognized as IPv4.")

        d = dict()
        i1 = pytrap.UnirecIPAddr("0:0:0:1::")
        i2 = pytrap.UnirecIPAddr("::1")
        i3 = pytrap.UnirecIPAddr("8.8.8.8")
        d[i1] = 1
        d[i2] = 2
        d[i3] = 3
        self.assertEqual(d[i3], 3)
        self.assertEqual(d[i1], 1)
        self.assertEqual(d[i2], 2)
        i4 = pytrap.UnirecIPAddr("8.8.4.4")
        try:
            print(d[i4])
            self.fail("IP address shouldn't be in dict")
        except:
            pass
        try:
            i = pytrap.UnirecIPAddr(0)
            self.fail("Only string is a valid argument of UnirecIPAddr()")
        except:
            pass
        i = pytrap.UnirecIPAddr("::")
        self.assertTrue(i.isNull())
        self.assertFalse(i)
        i = pytrap.UnirecIPAddr("::1")
        self.assertFalse(i.isNull())
        self.assertTrue(i)
        i = pytrap.UnirecIPAddr("0.0.0.0")
        self.assertTrue(i.isNull())
        self.assertFalse(i)
        i = pytrap.UnirecIPAddr("1.2.3.4")
        self.assertFalse(i.isNull())
        self.assertTrue(i)

        # __contains__
        self.assertFalse(i3 in i4)
        self.assertTrue(i4 in i4)
        self.assertTrue(pytrap.UnirecIPAddr("1.2.3.4") in pytrap.UnirecIPAddr("1.2.3.4"))
        ip = pytrap.UnirecIPAddr("1.2.3.4")
        bl1 = pytrap.UnirecIPAddr("1.2.3.4")
        bl2 = pytrap.UnirecIPAddrRange("1.0.0.0/8")
        bl3 = pytrap.UnirecIPAddr("1.2.3.5")
        bl4 = pytrap.UnirecIPAddrRange("2.0.0.0/8")
        # both are True:
        self.assertTrue(ip in bl1)
        self.assertTrue(ip in bl2)
        # both are False
        self.assertFalse(ip in bl3)
        self.assertFalse(ip in bl4)


        try:
            i = 1 in i4
            self.fail("only UnirecIPAddr type supported.")
        except TypeError:
            # expected UnirecIPAddr type
            pass

def timedelta_total_seconds(timedelta):
    return (
        timedelta.microseconds + 0.0 +
        (timedelta.seconds + timedelta.days * 24 * 3600) * 10 ** 6) / 10 ** 6
class DataTypesTime(unittest.TestCase):
    def runTest(self):
        import pytrap
        t = pytrap.UnirecTime(1466701316, 123)
        self.assertEqual(type(t), pytrap.UnirecTime, "Bad type of Time object.")
        self.assertEqual(str(t),  "1466701316.123", "Time is not equal to its str().")
        self.assertEqual(repr(t), "UnirecTime(1466701316, 123)", "Time is not equal to its repr().")
        self.assertEqual(float(t), 1466701316.123, "Conversion of Time to float failed.")
        self.assertEqual(t, t)
        self.assertEqual(t.getSeconds(), 1466701316, "Number of seconds differs.")
        self.assertEqual(t.getMiliSeconds(), 123, "Number of miliseconds differs.")
        self.assertEqual(t.getTimeAsFloat(), 1466701316.123, "Time as float differs.")

        t2 = pytrap.UnirecTime(10, 100)
        self.assertEqual(type(t2), pytrap.UnirecTime, "Bad type of Time object.")
        self.assertEqual(str(t2),  "10.100", "Time is not equal to its str().")
        self.assertEqual(repr(t2), "UnirecTime(10, 100)", "Time is not equal to its repr().")

        self.assertFalse(t == t2)
        self.assertFalse(t <= t2)
        self.assertTrue(t  >= t2)
        self.assertTrue(t  != t2)
        self.assertFalse(t < t2)
        self.assertTrue(t  > t2)

        res1 = pytrap.UnirecTime(1466701326, 223)
        self.assertEqual(t + t2, res1)
        res2 = pytrap.UnirecTime(2466701316, 123)
        self.assertEqual(t + 1000000000, res2)
        res3 = pytrap.UnirecTime(466701316, 123)
        self.assertEqual(t + (-1000000000), res3)

        t = pytrap.UnirecTime(1466701316.5)
        print(t)
        self.assertEqual(t.getSeconds(), 1466701316)
        self.assertEqual(t.getMiliSeconds(), 500)

        for i in range(10):
            self.assertEqual(res1.format(), "2016-06-23T17:02:06Z")
        for i in range(10):
            self.assertEqual(res1.format("%d.%m.%Y"), "23.06.2016")

        from datetime import datetime
        now = pytrap.UnirecTime.now()
        now2 = datetime.utcnow()
        delta = now2 - now.toDatetime()
        self.assertTrue(timedelta_total_seconds(delta) <= 1, "Now returns delayed time {0}.".format(str(delta)))

        # convert datetime to UnirecTime and compare it
        now = pytrap.UnirecTime.fromDatetime(now2)
        self.assertTrue(abs(now.getSeconds() - int(now2.strftime("%s")) <= 1))


class DataAccessGetTest(unittest.TestCase):
    def runTest(self):
        import pytrap
        a = pytrap.UnirecTemplate("ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,string TEXT,bytes STREAMBYTES")
        data = bytearray(b'\x00\x00\x00\x00\x00\x00\x00\x00\x0A\x00\x00\x01\xff\xff\xff\xff\xc8\x76\xbe\xff\xe3\x2b\x6c\x57\x00\x00\x00\x01\x00\x00\x00\x02\x06\x00\x04\x00\x00\x00\x06\x00abcdef\xde\xad\xfe\xed')
        a.setData(data)
        self.assertEqual(len(a), 6, "Number of fields differs, 6 expected.")
        self.assertEqual(str(a), "(ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,bytes STREAMBYTES,string TEXT)")
        d = a.getFieldsDict()
        self.assertEqual(type(d), dict)
        self.assertEqual(d, {'TIME_FIRST': 1, 'ABC': 2, 'BCD': 3, 'TEXT': 4, 'STREAMBYTES': 5, 'SRC_IP': 0})

        self.assertEqual(a.get(data, "SRC_IP"), pytrap.UnirecIPAddr("10.0.0.1"))
        self.assertEqual(a.get(data, "SRC_IP"), a.getByID(data, 0))
        self.assertEqual(a.get(data, "SRC_IP"), a.SRC_IP)

        self.assertEqual(a.get(data, "TIME_FIRST"), a.getByID(data, 1))
        self.assertEqual(a.get(data, "TIME_FIRST"), pytrap.UnirecTime(1466706915, 999))
        self.assertEqual(a.get(data, "TIME_FIRST"), a.TIME_FIRST)

        self.assertEqual(a.get(data, "ABC"), 16777216)
        self.assertEqual(a.get(data, "ABC"), a.getByID(data, 2))
        self.assertEqual(a.get(data, "ABC"), a.ABC)

        self.assertEqual(a.get(data, "BCD"), 33554432)
        self.assertEqual(a.get(data, "BCD"), a.getByID(data, 3))
        self.assertEqual(a.get(data, "BCD"), a.BCD)

        self.assertEqual(a.get(data, "TEXT"), "abcdef")
        self.assertEqual(a.get(data, "TEXT"), a.getByID(data, 4))
        self.assertEqual(a.get(data, "TEXT"), a.TEXT)
        self.assertEqual(type(a.get(data, "STREAMBYTES")), bytearray)
        self.assertEqual(a.get(data, "STREAMBYTES"), bytearray(b'\xde\xad\xfe\xed'))
        self.assertEqual(a.get(data, "STREAMBYTES"), a.getByID(data, 5))
        self.assertEqual(a.get(data, "STREAMBYTES"), a.STREAMBYTES)
        stream = a.get(data, "STREAMBYTES")
        self.assertEqual(" ".join([hex(i) for i in stream]), "0xde 0xad 0xfe 0xed")


class DataAccessSetTest(unittest.TestCase):
    def runTest(self):
        import pytrap
        a = pytrap.UnirecTemplate("ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,string TEXT,bytes STREAMBYTES")
        data = a.createMessage(100)
        for i in range(100):
            self.assertEqual(data, a.getData())

        a.ABC = 666
        self.assertEqual(a.ABC, 666)
        a.SRC_IP = pytrap.UnirecIPAddr("147.32.1.1")
        self.assertEqual(a.SRC_IP, pytrap.UnirecIPAddr("147.32.1.1"))
        a.setByID(data, 0, pytrap.UnirecIPAddr("fd7c:e770:9b8a::465"))
        self.assertEqual(a.SRC_IP, pytrap.UnirecIPAddr("fd7c:e770:9b8a::465"))
        a.set(data, "SRC_IP", pytrap.UnirecIPAddr("10.0.0.1"))
        self.assertEqual(a.SRC_IP, pytrap.UnirecIPAddr("10.0.0.1"))

        a.TIME_FIRST = pytrap.UnirecTime(666, 0)
        self.assertEqual(a.TIME_FIRST, pytrap.UnirecTime(666, 0))
        a.setByID(data, 1, pytrap.UnirecTime(1234, 666))
        self.assertEqual(a.TIME_FIRST, pytrap.UnirecTime(1234, 666))
        a.set(data, "TIME_FIRST", pytrap.UnirecTime(1468962758, 166))
        self.assertEqual(a.TIME_FIRST, pytrap.UnirecTime(1468962758, 166))

        a.TEXT = "different text"
        self.assertEqual(a.TEXT, "different text")
        a.setByID(data, 4, "my long text")
        self.assertEqual(a.TEXT, "my long text")
        a.set(data, "TEXT", "long text")
        self.assertEqual(a.TEXT, "long text")

        a.STREAMBYTES = bytearray(b"he\x01\x01")
        self.assertEqual(a.STREAMBYTES, bytearray(b"he\x01\x01"))
        a.STREAMBYTES = bytes(b"\xca\xfe")
        self.assertEqual(a.STREAMBYTES, bytes(b"\xca\xfe"))
        a.setByID(data, 5, bytes(b"\xde\xad\xbe\xef"))
        self.assertEqual(a.STREAMBYTES, bytes(b"\xde\xad\xbe\xef"))

        data = a.createMessage(100)
        a.setByID(data, 2, int(1234))
        a.ABC = int(1)
        self.assertEqual(a.ABC, int(1))
        a.set(data, "ABC", int(666))
        self.assertEqual(a.ABC, int(666))
        a.ABC = int(222)
        self.assertEqual(a.ABC, int(222))
        # overflow
        a.ABC = int(4294967296)
        self.assertEqual(a.ABC, int(0))


class TemplateTest(unittest.TestCase):
    def runTest(self):
        import pytrap
        a = pytrap.UnirecTemplate("ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,string TEXT,bytes STREAMBYTES")
        data = bytearray(b'\x00\x00\x00\x00\x00\x00\x00\x00\x0A\x00\x00\x01\xff\xff\xff\xff\xc8\x76\xbe\xff\xe3\x2b\x6c\x57\x00\x00\x00\x01\x00\x00\x00\x02\x06\x00\x04\x00\x00\x00\x06\x00abcdef\xde\xad\xfe\xed')
        a.setData(data)
        tc = a.strRecord()
        for i in range(100):
            self.assertEqual(tc, a.strRecord())

class Template2Test(unittest.TestCase):
    def runTest(self):
        import pytrap
        a = pytrap.UnirecTemplate("ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,string TEXT,bytes STREAMBYTES")
        a.createMessage(100)
        self.assertEqual(a.getFieldsDict(), {'SRC_IP': 0, 'STREAMBYTES': 5, 'TEXT': 4, 'TIME_FIRST': 1, 'ABC': 2, 'BCD': 3})
        a.SRC_IP = pytrap.UnirecIPAddr("1.2.3.4")
        a.TIME_FIRST = pytrap.UnirecTime(123456)
        a.ABC = 1234
        a.BCD = 54321
        a.STREAMBYTES = bytearray(b"streamtext")
        a.TEXT = "stringmuj text"
        tc = a.strRecord()
        for i in range(100):
            self.assertEqual(tc, a.strRecord())
        a = pytrap.UnirecTemplate("ipaddr DST_IP,time TIME_FIRST,uint32 BCD,string TEXT2,bytes STREAMBYTES")
        self.assertEqual(a.getFieldsDict(), {'BCD': 3, 'DST_IP': 6, 'TIME_FIRST': 1, 'STREAMBYTES': 5, 'TEXT2': 7})
        try:
            a.DST_IP = pytrap.UnirecIPAddr("4.4.4.4")
        except:
            pass
        else:
            self.fail("Data was not set, this should raise exception.")
        a.createMessage(100)
        try:
            a.SRC_IP = pytrap.UnirecIPAddr("4.4.4.4")
        except:
            pass
        else:
            self.fail("This template has no SRC_IP.")
        a.DST_IP = pytrap.UnirecIPAddr("4.4.4.4")
        a.STREAMBYTES = bytearray(b"hello")
        valdict = {}
        for name, value in a:
            valdict[name] = value
        self.assertEqual(valdict, {'STREAMBYTES': bytearray(b'hello'), 'DST_IP': pytrap.UnirecIPAddr('4.4.4.4'),
                                    'TIME_FIRST': pytrap.UnirecTime(0), 'TEXT2': '', 'BCD': 0})


class Template3Test(unittest.TestCase):
    def runTest(self):
        import pytrap
        a = pytrap.UnirecTemplate("ipaddr IP,time TIME,uint64 U64,uint32 U32,uint16 U16,uint8 U8,int64 I64,int32 I32,int16 I16,int8 I8,float FL,double DB,char CHR,string TEXT,bytes STREAMBYTES")
        a.createMessage(100)
        a.IP = pytrap.UnirecIPAddr("1.2.3.4")
        a.TIME = pytrap.UnirecTime(123456)
        a.U64 = 0x100000000
        a.U32 = 0x10000
        a.U16 = 0x100
        a.U8 = 0x1
        a.I64 = -1
        a.I32 = -1
        a.I16 = -1
        a.I8 = -1
        a.FL = 1.234
        a.DB = 1.234
        #a.CHR = "a"
        a.TEXT = "text"
        a.STREAMBYTES = b"streambytes"

        self.assertTrue(a.IP == pytrap.UnirecIPAddr("1.2.3.4"))
        self.assertTrue(a.TIME == pytrap.UnirecTime(123456))
        self.assertTrue(a.U64 == 0x100000000)
        self.assertTrue(a.U32 == 0x10000)
        self.assertTrue(a.U16 == 0x100)
        self.assertTrue(a.U8 == 0x1)
        self.assertTrue(a.I64 == -1)
        self.assertTrue(a.I32 == -1)
        self.assertTrue(a.I16 == -1)
        self.assertTrue(a.I8 == b'\xff')
        self.assertTrue(1.234 - a.FL < 1e-7)
        self.assertTrue(a.DB == 1.234)
        #self.assertTrue(a.CHR == "a")
        self.assertTrue(a.TEXT == "text")
        self.assertTrue(a.STREAMBYTES == b"streambytes")

        # Check types
        self.assertEqual(type(a.IP), pytrap.UnirecIPAddr)
        self.assertEqual(type(a.TIME), pytrap.UnirecTime)
        self.assertEqual(type(a.U64), long)
        self.assertEqual(type(a.U32), int)
        self.assertEqual(type(a.U16), int)
        self.assertEqual(type(a.U8), int)
        self.assertEqual(type(a.I64), long)
        self.assertEqual(type(a.I32), int)
        self.assertEqual(type(a.I16), int)
        self.assertEqual(type(a.I8), bytes)
        self.assertEqual(type(a.CHR), int)
        self.assertEqual(type(a.FL), float)
        self.assertEqual(type(a.DB), float)
        self.assertEqual(type(a.TEXT), str)
        self.assertEqual(type(a.STREAMBYTES), bytearray)

        self.assertEqual(a.getFieldType("IP"), pytrap.UnirecIPAddr)
        self.assertEqual(a.getFieldType("TIME"), pytrap.UnirecTime)
        self.assertEqual(a.getFieldType("U64"), long)
        self.assertEqual(a.getFieldType("U32"), long)
        self.assertEqual(a.getFieldType("U16"), long)
        self.assertEqual(a.getFieldType("U8"), long)
        self.assertEqual(a.getFieldType("I64"), long)
        self.assertEqual(a.getFieldType("I32"), long)
        self.assertEqual(a.getFieldType("I16"), long)
        self.assertEqual(a.getFieldType("I8"), long)
        self.assertEqual(a.getFieldType("CHR"), long)
        self.assertEqual(a.getFieldType("FL"), float)
        self.assertEqual(a.getFieldType("DB"), float)
        self.assertEqual(a.getFieldType("TEXT"), str)
        self.assertEqual(a.getFieldType("STREAMBYTES"), bytearray)

class TemplateSizeTest(unittest.TestCase):
    def runTest(self):
        import pytrap
        a = pytrap.UnirecTemplate("ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,string TEXT,bytes STREAMBYTES")
        data = bytearray(b'\x00\x00\x00\x00\x00\x00\x00\x00\x0A\x00\x00\x01\xff\xff\xff\xff\xc8\x76\xbe\xff\xe3\x2b\x6c\x57\x00\x00\x00\x01\x00\x00\x00\x02\x06\x00\x04\x00\x00\x00\x06\x00abcdef\xde\xad\xfe\xed')
        self.assertEqual(a.recSize(data), 50)
        self.assertEqual(a.recFixlenSize(), 40)
        self.assertEqual(a.recVarlenSize(data), 10)
        self.assertRaises(TypeError, a.recSize)  # recSize can't by called without arguments unless data was set by setData
        self.assertRaises(TypeError, a.recVarlenSize)
        a.setData(data)
        self.assertEqual(a.recSize(), 50) # now it should be OK
        self.assertEqual(a.recVarlenSize(), 10)
        
        
        
        a = pytrap.UnirecTemplate("uint32 X,ipaddr IP")
        data = bytearray(100) # Allocate larger byte array than necessary
        a.setData(data)
        self.assertEqual(a.recSize(), 20)
        self.assertEqual(a.recFixlenSize(), 20)
        self.assertEqual(a.recVarlenSize(), 0)
        
        a = pytrap.UnirecTemplate("string STR")
        data = bytearray(65535)
        data[0:1] = b'\x00\x00\xfb\xff' # Set offset (0) and length (65531 - maximum), let data be set to zeros
        a.setData(data)
        self.assertEqual(a.recSize(), 65535)
        self.assertEqual(a.recFixlenSize(), 4)
        self.assertEqual(a.recVarlenSize(), 65531)

class DataTypesIPAddrRange(unittest.TestCase):
    def runTest(self):
        import pytrap
        try:
            ip1 = pytrap.UnirecIPAddrRange("1.2.3.4")
            self.fail("2 arguments or <ip>/<netmask> are required")
        except:
            pass
        try:
            ip1 = pytrap.UnirecIPAddrRange(pytrap.UnirecIPAddr("1.2.3.4"))
            self.fail("2 arguments or <ip>/<netmask> are required")
        except:
            pass
        try:
            ip1 = pytrap.UnirecIPAddrRange(1, 2)
            self.fail("Integer arguments are not supported.")
        except:
            pass
        ip1 = pytrap.UnirecIPAddrRange("192.168.3.1/24")
        self.assertEqual(ip1, ip1)
        self.assertEqual(type(ip1), pytrap.UnirecIPAddrRange, "Bad type of IP address Range object.")
        self.assertEqual(str(ip1), "192.168.3.0 - 192.168.3.255", "IP address is not equal to its str().")
        self.assertEqual(repr(ip1), "UnirecIPAddrRange(UnirecIPAddr('192.168.3.0'), UnirecIPAddr('192.168.3.255'))",
                         "IP address is not equal to its repr().")

        self.assertTrue(ip1.start.isIPv4(), "IPv6 was recognized as IPv4.")
        self.assertFalse(ip1.start.isIPv6(), "IPv6 was not recognized.")
        self.assertEqual(ip1.start.isIPv4(), ip1.end.isIPv4(), "IPv4 was not recognized.")
        self.assertEqual(ip1.start.isIPv6(), ip1.end.isIPv6(), "IPv4 was recognized as IPv6.")

        ip2 = pytrap.UnirecIPAddrRange("192.168.0.1/24")
        ip3 = pytrap.UnirecIPAddrRange("192.168.3.1/24")
        self.assertFalse(ip1 == ip2, "Comparison of different IP addresses failed.")
        self.assertFalse(ip1 <= ip2, "Comparison of the same IP addresses failed.")
        self.assertFalse(ip2 >= ip1, "Comparison of the same IP addresses failed.")
        self.assertFalse(ip1 != ip3, "Comparison of the same IP addresses failed.")
        self.assertFalse(ip1 < ip3, "Comparison of the same IP addresses failed.")
        self.assertFalse(ip1 > ip3, "Comparison of the same IP addresses failed.")

        ip1 = pytrap.UnirecIPAddrRange("192.168.3.1/32")
        self.assertEqual(ip1.start, ip1.end)
        ip1 = pytrap.UnirecIPAddrRange("192.168.3.1", "192.168.3.1")
        self.assertEqual(ip1.start, ip1.end)
        ip1 = pytrap.UnirecIPAddrRange(pytrap.UnirecIPAddr("192.168.4.1"), pytrap.UnirecIPAddr("192.168.4.1"))
        self.assertEqual(ip1.start, ip1.end)

        ip1 = pytrap.UnirecIPAddrRange("fd7c:e770:9b8a::465/64")
        self.assertEqual(type(ip1), pytrap.UnirecIPAddrRange, "Bad type of IP address object.")
        self.assertEqual(str(ip1), "fd7c:e770:9b8a:: - fd7c:e770:9b8a:0:ffff:ffff:ffff:ffff",
                         "IP address is not equal to its str().")
        self.assertEqual(repr(ip1), "UnirecIPAddrRange(UnirecIPAddr('fd7c:e770:9b8a::'), UnirecIPAddr('fd7c:e770:9b8a:0:ffff:ffff:ffff:ffff'))",
                         "IP address is not equal to its repr().")

        self.assertFalse(ip1.start.isIPv4(), "IPv6 was not recognized.")
        self.assertTrue(ip1.start.isIPv6(), "IPv6 was recognized as IPv4.")
        self.assertEqual(ip1.start.isIPv4(), ip1.end.isIPv4(), "IPv4 was not recognized.")
        self.assertEqual(ip1.start.isIPv6(), ip1.end.isIPv6(), "IPv4 was recognized as IPv6.")

        ip1 = pytrap.UnirecIPAddrRange("fd7c:e770:9b8a::465/128")
        self.assertEqual(ip1.start, ip1.end)
        ip1 = pytrap.UnirecIPAddrRange("fd7c:e770:9b8a::465", "fd7c:e770:9b8a::465")
        self.assertEqual(ip1.start, ip1.end)
        ip1 = pytrap.UnirecIPAddrRange(pytrap.UnirecIPAddr("fd7c:e770:9b8a::465"),
                                       pytrap.UnirecIPAddr("fd7c:e770:9b8a::465"))
        self.assertEqual(ip1.start, ip1.end)

        intex = pytrap.UnirecIPAddrRange("192.168.1.0", "192.168.1.255")

        self.assertTrue(pytrap.UnirecIPAddr("192.168.1.0") in intex, "in test: first ip - fail")
        self.assertTrue(pytrap.UnirecIPAddr("192.168.1.255") in intex, "in test: last ip - fail")
        self.assertTrue(pytrap.UnirecIPAddr("192.168.1.125") in intex, "in test: mid ip - fail")
        self.assertFalse(pytrap.UnirecIPAddr("192.168.0.255") in intex, "in test: low match - fail")
        self.assertFalse(pytrap.UnirecIPAddr("10.10.10.0") in intex, "in test: low match - fail")
        self.assertFalse(pytrap.UnirecIPAddr("192.168.2.0") in intex, "in test: great match - fail")
        self.assertFalse(pytrap.UnirecIPAddr("255.255.255.255") in intex, "in test: great match - fail")

        self.assertEqual(intex.isIn(pytrap.UnirecIPAddr("192.168.1.0")), 0, "in test: first ip - fail")
        self.assertEqual(intex.isIn(pytrap.UnirecIPAddr("192.168.1.255")), 0, "in test: last ip - fail")
        self.assertEqual(intex.isIn(pytrap.UnirecIPAddr("192.168.1.125")),  0, "in test: mid ip - fail")
        self.assertEqual(intex.isIn(pytrap.UnirecIPAddr("192.168.0.255")), -1, "in test: low match - fail")
        self.assertEqual(intex.isIn(pytrap.UnirecIPAddr("10.10.10.0")), -1, "in test: low match - fail")
        self.assertEqual(intex.isIn(pytrap.UnirecIPAddr("192.168.2.0")), 1, "in test: great match - fail")
        self.assertEqual(intex.isIn(pytrap.UnirecIPAddr("255.255.255.255")), 1, "in test: great match - fail")


        intex2 = pytrap.UnirecIPAddrRange("192.168.1.128", "192.168.2.172")
        result = intex.isOverlap(intex2)
        self.assertTrue(result, "isOverlap - match IPAddr in interval - fail")

        intex2 = pytrap.UnirecIPAddrRange("192.168.2.0", "192.168.2.128")
        result = intex.isOverlap(intex2)
        self.assertFalse(result, "isOverlap - no match IPAddr in interval -fail")

        intex2 = pytrap.UnirecIPAddrRange("192.168.2.0", "192.168.2.128")
        result = intex == intex2
        self.assertFalse(result, "Equal operator - not eq - fail")

        intex2 = pytrap.UnirecIPAddrRange(pytrap.UnirecIPAddr("192.168.1.0"), "192.168.2.128")
        result = intex == intex2
        self.assertFalse(result, "Equal operator - not eq - fail")

        intex2 = pytrap.UnirecIPAddrRange("192.168.1.128/25")
        result = intex == intex2
        self.assertFalse(result, "Equal operator - not eq - fail")

        intex2 = pytrap.UnirecIPAddrRange("192.168.1.0", pytrap.UnirecIPAddr("192.168.1.255"))
        result = intex == intex2
        self.assertTrue(result, "Equal operator - eq - fail")

        # test if UnirecIPAddrRange is hashable (can be used in dict as a key)
        rangemap = dict()
        rangemap[ip1] = 1
        rangemap[ip2] = 2
        if ip1 not in rangemap:
            self.fail("ip1 should be already in dict.")
        if ip2 not in rangemap:
            self.fail("ip2 should be already in dict.")
        s = 0
        for key in rangemap:
            s += rangemap[key]
        self.assertEqual(s, 3)


