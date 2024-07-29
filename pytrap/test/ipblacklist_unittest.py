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

class BasicList(unittest.TestCase):
    def runTest(self):
        import pytrap

        ip1 = pytrap.UnirecIPAddrRange("192.168.1.0/24")
        ip2 = pytrap.UnirecIPAddrRange("192.168.2.0/24")
        ip3 = pytrap.UnirecIPAddrRange("192.168.3.0/24")
        ip4 = pytrap.UnirecIPAddrRange("10.168.3.0/8")

        iplist = pytrap.UnirecIPList({ip1: "ip1", ip2: {1: "abc", 2: "cde"}, ip3: "ip3", ip4: 123})
        # print(iplist)
        # print(repr(iplist))

        res = iplist.find(pytrap.UnirecIPAddr("1.0.0.1"))
        self.assertEqual(res, None)

        res = iplist.find(pytrap.UnirecIPAddr("192.168.1.123"))
        self.assertEqual(res, "ip1")

        res2 = iplist.find(pytrap.UnirecIPAddr("10.0.0.0"))
        self.assertEqual(res2, 123)

        res3 = iplist.find(pytrap.UnirecIPAddr("192.168.2.0"))
        self.assertEqual(res3, {1: "abc", 2: "cde"})
        # test if accessing results works after iplist deletion:
        del(iplist)
        print(res)
        print(res2)
        print(res3)
        print(res3)

class NoneDataCase(unittest.TestCase):
    def runTest(self):
        import pytrap
        iplist = pytrap.UnirecIPList({pytrap.UnirecIPAddrRange("192.168.1.0/24"): None})

        res = iplist.find(pytrap.UnirecIPAddr("192.168.1.15"))
        self.assertEqual(res, None)

class ListContains(unittest.TestCase):
    def runTest(self):
        import pytrap
        iplist = pytrap.UnirecIPList({
            pytrap.UnirecIPAddrRange("192.168.1.0/24"): "ip4",
            pytrap.UnirecIPAddrRange("2001::1/48"): "ipv6",
            pytrap.UnirecIPAddrRange(pytrap.UnirecIPAddr("10.0.0.1"),pytrap.UnirecIPAddr("10.0.0.1")): "host" })
        self.assertTrue(pytrap.UnirecIPAddr("192.168.1.255") in iplist)
        self.assertTrue(pytrap.UnirecIPAddr("2001::123") in iplist)
        self.assertTrue(pytrap.UnirecIPAddr("10.0.0.1") in iplist)
        self.assertFalse(pytrap.UnirecIPAddr("10.0.0.0") in iplist)
        self.assertFalse(pytrap.UnirecIPAddr("10.0.0.2") in iplist)
        self.assertFalse(pytrap.UnirecIPAddr("::1") in iplist)
        self.assertFalse(pytrap.UnirecIPAddr("192.168.0.0") in iplist)

class EmptyConstructorCase(unittest.TestCase):
    def runTest(self):
        import pytrap
        with self.assertRaises(TypeError):
            iplist = pytrap.UnirecIPList()

        with self.assertRaises(ValueError):
            iplist = pytrap.UnirecIPList({})

class DeletedList(unittest.TestCase):
    def runTest(self):
        import pytrap
        iplist = pytrap.UnirecIPList({pytrap.UnirecIPAddrRange("0.0.0.0/0"): 1001})
        res = iplist.find(pytrap.UnirecIPAddr("1.0.0.1"))
        self.assertEqual(res, 1001)
        del(iplist)

        # already deleted iplist:
        with self.assertRaises(UnboundLocalError):
            res = iplist.find(pytrap.UnirecIPAddr("1.0.0.1"))

class AppendUpdateDataCase(unittest.TestCase):
    def runTest(self):
        import pytrap
        iplist = pytrap.UnirecIPList({pytrap.UnirecIPAddrRange("192.168.1.0/24"): {"data":[1,2,3]}})

        res = iplist.find(pytrap.UnirecIPAddr("192.168.1.15"))
        self.assertEqual(res, {"data":[1,2,3]})
        res["data"].append(4)
        del(res)

        res2 = iplist.find(pytrap.UnirecIPAddr("192.168.1.15"))
        self.assertEqual(res2, {"data":[1,2,3,4]})
        res2 = {"data": [4,5]}
        del(res2)
        res3 = iplist.find(pytrap.UnirecIPAddr("192.168.1.15"))
        self.assertEqual(res3, {"data": [1,2,3,4]})

class AnyIPCase(unittest.TestCase):
    def runTest(self):
        import pytrap
        ir = pytrap.UnirecIPAddrRange("0.0.0.0/0")
        print(ir)
        print(ir.mask)
        iplist = pytrap.UnirecIPList({ir: "abc"})
        print(iplist)
        res = iplist.find(pytrap.UnirecIPAddr("1.0.0.1"))
        self.assertEqual(res, "abc")
        del(iplist)

        ir = pytrap.UnirecIPAddrRange("::/0")
        print(ir)
        print(ir.mask)
        iplist = pytrap.UnirecIPList({ir: "abc"})
        print(iplist)
        res = iplist.find(pytrap.UnirecIPAddr("1.0.0.1"))
        self.assertEqual(res, None)
        res = iplist.find(pytrap.UnirecIPAddr("1::1"))
        self.assertEqual(res, "abc")

