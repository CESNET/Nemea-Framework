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

        # IP address shouldn't be in dict
        with self.assertRaises(KeyError):
            print(d[i4])

        # Only string is a valid argument of UnirecIPAddr()
        with self.assertRaises(TypeError):
            i = pytrap.UnirecIPAddr(0)

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


        # only UnirecIPAddr type supported
        with self.assertRaises(TypeError):
            i = 1 in i4

class DataTypesMACAddr(unittest.TestCase):
    def runTest(self):
        import pytrap
        mac0 = pytrap.UnirecMACAddr("0:1:11:A:AA:FF")
        self.assertEqual(str(mac0), "00:01:11:0a:aa:ff",
                         "MAC address .str() did not produce canonical representation.")
        mac1 = pytrap.UnirecMACAddr("11:22:33:44:55:66")
        self.assertEqual(mac1, mac1)
        self.assertEqual(type(mac1), pytrap.UnirecMACAddr, "Bad type of MAC address object.")
        self.assertEqual(str(mac1),
                         "11:22:33:44:55:66",
                         "MAC address .str() did not produce canonical representation.")
        self.assertEqual(repr(mac1), "UnirecMACAddr('11:22:33:44:55:66')", "MAC address is not equal to its repr().")

        mac2 = pytrap.UnirecMACAddr("10:20:30:40:50:60")
        mac3 = pytrap.UnirecMACAddr("11:22:33:44:55:66")
        self.assertFalse(mac1 == mac2, "Comparison of different MAC addresses failed.")
        self.assertFalse(mac1 <= mac2, "Comparison of the same MAC addresses failed.")
        self.assertFalse(mac2 >= mac1, "Comparison of the same MAC addresses failed.")
        self.assertFalse(mac1 != mac3, "Comparison of the same MAC addresses failed.")
        self.assertFalse(mac1  < mac3, "Comparison of the same MAC addresses failed.")
        self.assertFalse(mac1  > mac3, "Comparison of the same MAC addresses failed.")

        mac4 = mac1
        for i in range(0, 256):
            mac4 = mac4.inc();

        self.assertTrue(mac4 == pytrap.UnirecMACAddr("11:22:33:44:56:66"), "Incrementation failed.")

        for i in range(0, 256):
            mac4 = mac4.dec();

        self.assertTrue(mac4 == mac1, "Decrementation failed.")

        mac5 = pytrap.UnirecMACAddr("FF:FF:FF:FF:FF:FF").inc()
        mac6 = pytrap.UnirecMACAddr("00:00:00:00:00:00").dec()
        self.assertTrue(mac5 == pytrap.UnirecMACAddr("00:00:00:00:00:00"), "Incrementation failed.")
        self.assertTrue(mac6 == pytrap.UnirecMACAddr("FF:FF:FF:FF:FF:FF"), "Decrementation failed.")

        # test mac address bytes conversion to integer (bytes consist of uint32_t part and uint16_t part)
        mac7 = pytrap.UnirecMACAddr("12:34:FF:FF:FF:FF").inc()
        mac8 = pytrap.UnirecMACAddr("12:34:56:78:FF:FF").inc()
        self.assertTrue(mac7 == pytrap.UnirecMACAddr("12:35:00:00:00:00"), "uint32_t test failed.")
        self.assertTrue(mac8 == pytrap.UnirecMACAddr("12:34:56:79:00:00"), "uint16_t test failed.")

        d = dict()
        m1 = pytrap.UnirecMACAddr("1:2:3:4:5:6")
        m2 = pytrap.UnirecMACAddr("6:5:4:3:2:1")
        m3 = pytrap.UnirecMACAddr("FF:FF:FF:FF:FF:FF")
        d[m1] = 1
        d[m2] = 2
        d[m3] = 3
        self.assertEqual(d[m3], 3)
        self.assertEqual(d[m1], 1)
        self.assertEqual(d[m2], 2)
        m4 = pytrap.UnirecMACAddr("1:2:3:0:0:0")

        # MAC address shouldn't be in dict
        with self.assertRaises(KeyError):
            print(d[m4])

        # Only string is a valid argument of UnirecMACAddr()
        with self.assertRaises(TypeError):
            i = pytrap.UnirecMACAddr(0)

        # Only string is a valid argument of UnirecMACAddr()
        with self.assertRaises(TypeError if sys.version_info[0] >= 3 else pytrap.TrapError):
            i = pytrap.UnirecMACAddr(bytes([11, 22, 33, 44, 55, 66]))

        i = pytrap.UnirecMACAddr("00:00:00:00:00:00")
        self.assertTrue(i.isNull())
        self.assertFalse(i)
        i = pytrap.UnirecMACAddr("00:00:00:00:00:01")
        self.assertFalse(i.isNull())
        self.assertTrue(i)

        # __contains__
        self.assertFalse(m3 in m4)
        self.assertTrue(m4 in m4)
        self.assertTrue(pytrap.UnirecMACAddr("d:e:a:d:be:ef") in pytrap.UnirecMACAddr("d:e:a:d:be:ef"))

class DataTypesMACAddrRange(unittest.TestCase):
    def runTest(self):
        import pytrap
        mac = pytrap.UnirecMACAddr("d:e:a:d:be:ef")
        bl1 = pytrap.UnirecMACAddr("d:e:a:d:be:ef")
        bl2 = pytrap.UnirecMACAddrRange(
            pytrap.UnirecMACAddr("d:0:0:0:0:0"), pytrap.UnirecMACAddr("d:f:f:f:f:f"))
        bl3 = pytrap.UnirecMACAddr("b:e:a:d:de:ef")
        bl4 = pytrap.UnirecMACAddrRange(
            pytrap.UnirecMACAddr("0:0:0:FF:FF:FF"), pytrap.UnirecMACAddr("1:2:3:4:5:6"))
        bl5 = pytrap.UnirecMACAddrRange(
            pytrap.UnirecMACAddr("0:0:0:FF:FF:FF"), pytrap.UnirecMACAddr("1:0:0:0:0:0"))
        bl6 = pytrap.UnirecMACAddrRange(
            pytrap.UnirecMACAddr("1:0:0:FF:FF:FF"), pytrap.UnirecMACAddr("c:0:0:0:0:0"))

        self.assertEqual(type(bl2),
                         pytrap.UnirecMACAddrRange,
                         "Bad type of MAC address object.")
        self.assertEqual(str(bl6),
                         "01:00:00:ff:ff:ff - 0c:00:00:00:00:00",
                         "String representation of UnirecMACAddrRange not equal to expected string.")
        self.assertEqual(repr(bl6),
                         "UnirecMACAddrRange(UnirecMACAddr('01:00:00:ff:ff:ff'), UnirecMACAddr('0c:00:00:00:00:00'))",
                         "Representation of UnirecMACAddrRange not equal to expected string.")

        # both are True:
        self.assertTrue(mac in bl1)
        self.assertTrue(mac in bl2)
        # both are False
        self.assertFalse(mac in bl3)
        self.assertFalse(mac in bl4)

        self.assertTrue(bl2.isIn(mac) == 0)
        self.assertTrue(bl4.isIn(mac) == 1)
        self.assertTrue(bl4.isIn(pytrap.UnirecMACAddr("0:0:0:0:0:1")) == -1)

        self.assertTrue(bl4.isOverlap(bl4))
        self.assertTrue(bl4.isOverlap(bl5))
        self.assertTrue(bl5.isOverlap(bl4))
        self.assertTrue(bl4.isOverlap(bl6))
        self.assertTrue(not bl6.isOverlap(bl2))
        self.assertTrue(not bl2.isOverlap(bl4))

        self.assertTrue(not bl2 == bl4)
        self.assertTrue(bl2 == bl2)

        mac1 = pytrap.UnirecMACAddr("d:e:ad:be:e:f")
        mac2 = pytrap.UnirecMACAddr("b:a:d:f0:0:d")
        rangemap = dict()
        rangemap[mac1] = 1
        rangemap[mac2] = 2
        if mac1 not in rangemap:
            self.fail("mac1 should be already in dict.")
        if mac2 not in rangemap:
            self.fail("mac2 should be already in dict.")
        s = 0
        for key in rangemap:
            s += rangemap[key]
        self.assertEqual(s, 3)

def timedelta_total_seconds(timedelta):
    return (
        timedelta.microseconds + 0.0 +
        (timedelta.seconds + timedelta.days * 24 * 3600) * 10 ** 6) / 10 ** 6

class DataTypesTimeConstructors(unittest.TestCase):
    def runTest(self):
        import pytrap
        # Unsupported type
        with self.assertRaises(TypeError):
            pytrap.UnirecTime(tuple([1, 3]))

        # Float
        t = pytrap.UnirecTime(1.5)
        self.assertEqual(t.getSeconds(), 1)
        self.assertEqual(t.getMiliSeconds(), 500)

        # seconds and miliseconds
        t = pytrap.UnirecTime(1466701316, 123)
        self.assertEqual(t.getSeconds(), 1466701316, "Number of seconds differs.")
        self.assertEqual(t.getMiliSeconds(), 123, "Number of miliseconds differs.")
        self.assertEqual(t.getTimeAsFloat(), 1466701316.123, "Time as float differs.")

        t1 = pytrap.UnirecTime("2019-03-18T20:58:12.100")
        self.assertEqual(t1, pytrap.UnirecTime(1552942692, 100))

        # test conversion from string with timezone to UnirecTime
        t1 = pytrap.UnirecTime("2019-03-18T20:58:12.100Z")
        t2 = pytrap.UnirecTime("2019-03-19T20:58:12.222Z")
        self.assertEqual(t1, pytrap.UnirecTime(1552942692, 100))
        self.assertEqual(t2, pytrap.UnirecTime(1553029092, 222))
        self.assertEqual(t2.getSeconds() - t1.getSeconds(), 24 * 60 * 60)

        # malformed format
        with self.assertRaises(TypeError):
            pytrap.UnirecTime("18.3.2019 20:58:12.100Z")

class DataTypesTime(unittest.TestCase):
    def runTest(self):
        import pytrap

        t = pytrap.UnirecTime(1466701316, 123)
        self.assertEqual(type(t), pytrap.UnirecTime, "Bad type of Time object.")
        self.assertEqual(str(t),  "1466701316.123", "Time is not equal to its str().")
        self.assertEqual(repr(t), "UnirecTime(1466701316, 123)", "Time is not equal to its repr().")
        self.assertEqual(float(t), 1466701316.123, "Conversion of Time to float failed.")
        self.assertEqual(t, t)

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

class DataTypesArray(unittest.TestCase):
    def runTest(self):
        import pytrap
        a = pytrap.UnirecTemplate("uint32 FOO, uint32 BAR, ipaddr IP, string STR, int32* ARR1, ipaddr* IPs, macaddr* MACs, uint64* ARR2, time* TIMEs")
        data = bytearray(b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\xbc\x61\x4e\xff\xff\xff\xff\x31\xd4\x00\x00\x39\x30\x00\x00\x00\x00\x0c\x00\x0c\x00\x28\x00\x34\x00\x12\x00\x46\x00\x50\x00\x96\x00\x18\x00\xae\x00\x50\x00\x48\x65\x6c\x6c\x6f\x20\x57\x6f\x72\x6c\x64\x21\xf7\xff\xff\xff\xf8\xff\xff\xff\xf9\xff\xff\xff\xfa\xff\xff\xff\xfb\xff\xff\xff\xfc\xff\xff\xff\xfd\xff\xff\xff\xfe\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x11\x11\x11\x11\x11\x11\x22\x22\x22\x22\x22\x22\x0a\x00\x00\x00\x00\x00\x00\x00\x0b\x00\x00\x00\x00\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x0d\x00\x00\x00\x00\x00\x00\x00\x0e\x00\x00\x00\x00\x00\x00\x00\x0f\x00\x00\x00\x00\x00\x00\x00\x10\x00\x00\x00\x00\x00\x00\x00\x11\x00\x00\x00\x00\x00\x00\x00\x12\x00\x00\x00\x00\x00\x00\x00\x13\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xe6\xc0\x33\x5b\x00\x00\x00\x80\xe6\xc0\x33\x5b\x00\x00\x00\x00\xe6\xc0\x33\x5b\x00\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x01\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x0a\x00\x00\x02\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x7f\x00\x00\x01\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00\xbc\x61\x4e\xff\xff\xff\xff')
        a.setData(data)
        self.assertEqual(len(a), 9, "Number of fields differs, 9 expected.")
        self.assertEqual(str(a), "(ipaddr IP,uint32 BAR,uint32 FOO,string STR,int32* ARR1,macaddr* MACs,uint64* ARR2,time* TIMEs,ipaddr* IPs)")
        d = a.getFieldsDict()
        self.assertEqual(type(d), dict)
        self.assertEqual(set(d.keys()), set(['IP', 'BAR', 'FOO', 'STR', 'ARR1', 'MACs', 'ARR2', 'TIMEs', 'IPs']))

        self.assertEqual(a.getFieldType("ARR1"), list)
        self.assertEqual(a.getFieldType("ARR2"), list)
        self.assertEqual(a.getFieldType("IPs"), list)
        self.assertEqual(a.getFieldType("TIMEs"), list)

        self.assertEqual(a.get(data, "FOO"), 12345)
        self.assertEqual(a.get(data, "FOO"), a.getByID(data, 6))
        self.assertEqual(a.get(data, "FOO"), a.FOO)

        self.assertEqual(a.get(data, "BAR"), 54321)
        self.assertEqual(a.get(data, "BAR"), a.getByID(data, 7))
        self.assertEqual(a.get(data, "BAR"), a.BAR)

        self.assertEqual(a.get(data, "IP"), pytrap.UnirecIPAddr("0.188.97.78"))
        self.assertEqual(a.get(data, "IP"), a.getByID(data, 8))
        self.assertEqual(a.get(data, "IP"), a.IP)

        self.assertEqual(a.get(data, "STR"), "Hello World!")
        self.assertEqual(a.get(data, "STR"), a.getByID(data, 9))
        self.assertEqual(a.get(data, "STR"), a.STR)

        self.assertEqual(a.get(data, "ARR1"), [-9, -8, -7, -6, -5, -4, -3, -2, -1, 0])
        self.assertEqual(a.get(data, "ARR1"), a.getByID(data, 10))
        self.assertEqual(a.get(data, "ARR1"), a.ARR1)

        self.assertEqual(a.get(data, "IPs"), [pytrap.UnirecIPAddr("10.0.0.1"), pytrap.UnirecIPAddr("10.0.0.2"), pytrap.UnirecIPAddr("::1"), pytrap.UnirecIPAddr("127.0.0.1"), pytrap.UnirecIPAddr("0.188.97.78")])
        self.assertEqual(a.get(data, "IPs"), a.getByID(data, 11))
        self.assertEqual(a.get(data, "IPs"), a.IPs)

        self.assertEqual(a.get(data, "MACs"), [pytrap.UnirecMACAddr("0:0:0:0:0:0"), pytrap.UnirecMACAddr("11:11:11:11:11:11"), pytrap.UnirecMACAddr("22:22:22:22:22:22")])
        self.assertEqual(a.get(data, "MACs"), a.getByID(data, 12))
        self.assertEqual(a.get(data, "MACs"), a.MACs)

        self.assertEqual(a.get(data, "ARR2"), [10, 11, 12, 13, 14, 15, 16, 17, 18, 19])
        self.assertEqual(a.get(data, "ARR2"), a.getByID(data, 13))
        self.assertEqual(a.get(data, "ARR2"), a.ARR2)

        self.assertEqual(a.get(data, "TIMEs"), [pytrap.UnirecTime("2018-06-27T16:52:54"), pytrap.UnirecTime("2018-06-27T16:52:54.500"), pytrap.UnirecTime("2018-06-27T16:52:54Z"),])
        self.assertEqual(a.get(data, "TIMEs"), a.getByID(data, 14))
        self.assertEqual(a.get(data, "TIMEs"), a.TIMEs)

        f_str = "Hello Unirec!"
        f_arr1 = [-42, 7,11, 13, -17, 19]
        f_ips = [pytrap.UnirecIPAddr("10.200.4.1"), pytrap.UnirecIPAddr("192.168.0.200"), pytrap.UnirecIPAddr("2000::1")]
        f_macs = [pytrap.UnirecMACAddr("6:5:4:3:2:1"), pytrap.UnirecMACAddr("FF:FF:FF:FF:FF:FF"), pytrap.UnirecMACAddr("1:1:1:1:1:1"), pytrap.UnirecMACAddr("1:2:3:4:5:6")]
        f_arr2 = [123467890, 987654321]
        f_times = [pytrap.UnirecTime(1466706915, 999), pytrap.UnirecTime(146324234, 999), pytrap.UnirecTime(0, 1), pytrap.UnirecTime(1466700000, 0)]

        a.set(data, "STR", f_str)
        a.set(data, "ARR1", f_arr1)
        a.set(data, "IPs", f_ips)
        a.MACs = f_macs
        a.ARR2 = f_arr2
        a.set(data, "TIMEs", f_times)

        self.assertEqual(a.get(data, "STR"), f_str)
        self.assertEqual(a.get(data, "ARR1"), f_arr1)
        self.assertEqual(a.get(data, "IPs"), f_ips)
        self.assertEqual(a.get(data, "MACs"), f_macs)
        self.assertEqual(a.get(data, "ARR2"), f_arr2)
        self.assertEqual(a.get(data, "TIMEs"), f_times)

        f_arr2 = []
        a.ARR2 = f_arr2
        self.assertEqual(a.get(data, "ARR2"), f_arr2)

        f_arr1 = -42
        a.ARR1 = f_arr1
        self.assertEqual(a.get(data, "ARR1"), [f_arr1])

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
        self.assertEqual(set(d.keys()), set(['TIME_FIRST', 'ABC', 'BCD', 'TEXT', 'STREAMBYTES', 'SRC_IP']))

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
            getdata = a.getData()
            # size of created (allocated) message and retrieved message differs (due to non-filled variable fields)
            self.assertEqual(data[:len(getdata)], getdata)

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
        self.assertEqual(set(a.getFieldsDict().keys()), set(['SRC_IP', 'STREAMBYTES', 'TEXT', 'TIME_FIRST', 'ABC', 'BCD']))
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
        self.assertEqual(set(a.getFieldsDict().keys()), set(['DST_IP', 'TIME_FIRST', 'BCD', 'STREAMBYTES', 'TEXT2']))

        # Data was not set, this should raise exception.
        with self.assertRaises(pytrap.TrapError):
            a.DST_IP = pytrap.UnirecIPAddr("4.4.4.4")

        a.createMessage(100)

        # This template has no SRC_IP.
        with self.assertRaises(AttributeError):
            a.SRC_IP = pytrap.UnirecIPAddr("4.4.4.4")

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
        a = pytrap.UnirecTemplate("ipaddr IP, macaddr MAC,time TIME,uint64 U64,uint32 U32,uint16 U16,uint8 U8,int64 I64,int32 I32,int16 I16,int8 I8,float FL,double DB,char CHR,string TEXT,bytes STREAMBYTES")
        a.createMessage(100)
        a.IP = pytrap.UnirecIPAddr("1.2.3.4")
        a.MAC = pytrap.UnirecMACAddr("1:2:3:4:5:6")
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
        self.assertTrue(a.MAC == pytrap.UnirecMACAddr("1:2:3:4:5:6"))
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
        self.assertEqual(type(a.MAC), pytrap.UnirecMACAddr)
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
        self.assertEqual(a.getFieldType("MAC"), pytrap.UnirecMACAddr)
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

        # recSize can't be called without arguments unless data was set by setData
        with self.assertRaises(TypeError):
            a.recSize()

        with self.assertRaises(TypeError):
            a.recVarlenSize()

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

        # 2 arguments or <ip>/<netmask> are required
        with self.assertRaises(TypeError):
            pytrap.UnirecIPAddrRange("1.2.3.4")
        with self.assertRaises(TypeError):
            pytrap.UnirecIPAddrRange(pytrap.UnirecIPAddr("1.2.3.4"))

        # Integer arguments are not supported
        with self.assertRaises(TypeError):
            pytrap.UnirecIPAddrRange(1, 2)

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

class SignedUnsignedArrayFieldTest(unittest.TestCase):
    def runTest(self):
        import pytrap
        a = pytrap.UnirecTemplate("int64* ARRF1,int32* ARRF2,int16* ARRF3,int8* ARRF4,uint64* ARRF5,uint32* ARRF6,uint16* ARRF7,uint8* ARRF8")
        a.createMessage(10000)
        a.ARRF1 = [-1, -1]
        a.ARRF2 = [-1, -1]
        a.ARRF3 = [-1, -1]
        a.ARRF4 = [-1, -1]
        a.ARRF5 = [-1, -1]
        a.ARRF6 = [-1, -1]
        a.ARRF7 = [-1, -1]
        a.ARRF8 = [-1, -1]

        self.assertEqual(a.ARRF1, [-1, -1])
        self.assertEqual(a.ARRF2, [-1, -1])
        self.assertEqual(a.ARRF3, [-1, -1])
        self.assertEqual(a.ARRF4, [-1, -1])
        self.assertEqual(a.ARRF5, [18446744073709551615, 18446744073709551615])
        self.assertEqual(a.ARRF6, [4294967295, 4294967295])
        self.assertEqual(a.ARRF7, [65535, 65535])
        self.assertEqual(a.ARRF8, [255, 255])



class CopyTemplateTest(unittest.TestCase):
    def runTest(self):
        import pytrap
        a = pytrap.UnirecTemplate("ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,string TEXT,bytes STREAMBYTES")
        astr = str(a)
        b = a.copy()
        bstr = str(b)
        self.assertEqual(astr, bstr)
        self.assertEqual(astr, '(ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,bytes STREAMBYTES,string TEXT)')

class AllocateBigMessage(unittest.TestCase):
    def runTest(self):
        import pytrap
        a = pytrap.UnirecTemplate("ipaddr SRC_IP,time TIME_FIRST,uint32 ABC,uint32 BCD,string TEXT,bytes STREAMBYTES")
        with self.assertRaises(pytrap.TrapError):
            a.createMessage(100000)

