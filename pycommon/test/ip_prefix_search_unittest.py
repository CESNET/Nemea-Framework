import unittest
import doctest

class DeviceTest(unittest.TestCase):
    def runTest(self):
        try:
            import pytrap
            import ip_prefix_search
        except ImportError as e:
            self.fail(str(e))


class DataTypesIPPSNetwork(unittest.TestCase):
    def runTest(self):
        import ip_prefix_search
        net1 = ip_prefix_search.IPPSNetwork("192.168.1.1/24", "aaa")
        self.assertEqual(net1, net1)
        self.assertEqual(type(net1), ip_prefix_search.IPPSNetwork, "Bad type of Network object.")
        self.assertEqual(str(net1), "'192.168.1.1/24', 'aaa'", "Network is not equal to its str().")
        self.assertEqual(repr(net1), "IPPSNetwork('192.168.1.1/24', 'aaa')",
                         "Network is not equal to its repr().")

        net1 = ip_prefix_search.IPPSNetwork("192.168.1.1/24", "aaa")
        self.assertTrue(isinstance(net1, ip_prefix_search.IPPSNetwork))
        self.assertEqual(net1.addr, "192.168.1.1/24")
        self.assertEqual(net1.data, "aaa")
        try:
            ip_prefix_search.IPPSNetwork(19216811, "aaa")
            self.fail("TypeError exception expected.")
        except:
            pass

        net1 = ip_prefix_search.IPPSNetwork("0::1/24", "aaa")
        self.assertTrue(isinstance(net1, ip_prefix_search.IPPSNetwork))
        self.assertEqual(net1.addr, "0::1/24")
        self.assertEqual(net1.data, "aaa")
        try:
            ip_prefix_search.IPPSNetwork(19216811, "aaa")
            self.fail("TypeError exception expected.")
        except:
            pass

class DataTypesIPPSInterval(unittest.TestCase):
    def runTest(self):
        import ip_prefix_search
        import pytrap
        intex = ip_prefix_search.IPPSInterval("192.168.1.0/24")
        self.assertTrue(isinstance(intex, ip_prefix_search.IPPSInterval))
        self.assertTrue(isinstance(intex.start, pytrap.UnirecIPAddr))
        self.assertTrue(isinstance(intex.end, pytrap.UnirecIPAddr))
        self.assertTrue(intex.start.isIPv4)
        self.assertTrue(intex.end.isIPv4)

        self.assertEqual(ip_prefix_search.IPPSInterval("192.168.1.0/24").get_data(), [], "get_data() test: empty data - fail")

        self.assertEqual(ip_prefix_search.IPPSInterval("192.168.1.0/24", data="aaa").get_data(), ['aaa'], "get_data() test: init data - fail")

        intex = ip_prefix_search.IPPSInterval("192.168.1.0", "192.168.1.255", "aaa")
        intex.add_data("bbb")
        self.assertEqual(intex.get_data(), ['aaa', 'bbb'], "add_data() test: simple data - fail")
        self.assertEqual(len(intex), 2, "add_data() test: simple data length - fail")

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

        intex.add_data(["ccc", "ddd"])
        self.assertEqual(intex.get_data(), ['aaa', 'bbb', 'ccc', 'ddd'], "add_data() test: multiple data - fail")
        self.assertEqual(len(intex), 4, "add_data() test: multiple data length - fail")

        intex2 = ip_prefix_search.IPPSInterval("192.168.1.128", "192.168.2.172")
        result = intex.isOverlap(intex2)
        self.assertTrue(result, "isOverlap - match IPAddr in interval - fail")

        intex2 = ip_prefix_search.IPPSInterval("192.168.2.0", "192.168.2.128")
        result = intex.isOverlap(intex2)
        self.assertFalse(result, "isOverlap - no match IPAddr in interval -fail")

        intex2 = ip_prefix_search.IPPSInterval("192.168.2.0", "192.168.2.128")
        result = intex == intex2
        self.assertFalse(result, "Equal operator - not eq - fail")

        intex2 = ip_prefix_search.IPPSInterval(pytrap.UnirecIPAddr("192.168.1.0"), "192.168.2.128")
        result = intex == intex2
        self.assertFalse(result, "Equal operator - not eq - fail")

        intex2 = ip_prefix_search.IPPSInterval("192.168.1.128/25")
        result = intex == intex2
        self.assertFalse(result,  "Equal operator - not eq - fail")

        intex2 = ip_prefix_search.IPPSInterval("192.168.1.0", pytrap.UnirecIPAddr("192.168.1.255"))
        result = intex == intex2
        self.assertTrue(result, "Equal operator - eq - fail")
        self.assertEqual(len(intex2), 0, "init data test: simple data length - fail")

        first = ip_prefix_search.IPPSInterval("fd71:5693:e769:fc3f::/64")
        self.assertTrue(isinstance(first, ip_prefix_search.IPPSInterval))
        self.assertTrue(isinstance(first.start, pytrap.UnirecIPAddr))
        self.assertTrue(isinstance(first.end, pytrap.UnirecIPAddr))
        self.assertTrue(first.start.isIPv6)
        self.assertTrue(first.end.isIPv6)

        self.assertEqual(first.get_data(), [], "get_data() test: empty data - fail")
        self.assertEqual(ip_prefix_search.IPPSInterval("fd71:5693:e769:fc3f::/64", data="aaa").get_data(), ['aaa'],
                         "get_data() test: init data - fail")

        first = ip_prefix_search.IPPSInterval("fd71:5693:e769:fc3f:0:0:0:0", "fd71:5693:e769:fc3f:ffff:ffff:ffff:ffff", "aaa")
        first.add_data("bbb")
        self.assertEqual(first.get_data(), ['aaa', 'bbb'], "add_data() test: simple data - fail")
        self.assertEqual(len(first), 2, "add_data() test: simple data length - fail")

        self.assertTrue(pytrap.UnirecIPAddr("fd71:5693:e769:fc3f:0:0:0:0") in first,
                        "ip_match() test: first ip - fail")
        self.assertTrue(pytrap.UnirecIPAddr("fd71:5693:e769:fc3f:ffff:ffff:ffff:ffff") in first,
                        "ip_match() test: last ip - fail")
        self.assertTrue(pytrap.UnirecIPAddr("fd71:5693:e769:fc3f:1:ffff:a:abab") in first,
                        "ip_match() test: mid ip - fail")
        self.assertFalse(pytrap.UnirecIPAddr("fd71:5693:e769:fc3e:ffff:ffff:ffff:ffff") in first,
                         "ip_match() test: low match - fail")
        self.assertFalse(pytrap.UnirecIPAddr("10:10:10::0") in first,
                         "ip_match() test: low match - fail")
        self.assertFalse(pytrap.UnirecIPAddr("fd71:5693:e769:fc40:0:0:0:0") in first,
                         "ip_match() test: great match - fail")
        self.assertFalse(pytrap.UnirecIPAddr("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff") in first,
                         "ip_match() test: great match - fail")

        self.assertEqual(first.isIn(pytrap.UnirecIPAddr("fd71:5693:e769:fc3f:0:0:0:0")), 0,
                         "ip_match() test: first ip - fail")
        self.assertEqual(first.isIn(pytrap.UnirecIPAddr("fd71:5693:e769:fc3f:ffff:ffff:ffff:ffff")), 0,
                         "ip_match() test: last ip - fail")
        self.assertEqual(first.isIn(pytrap.UnirecIPAddr("fd71:5693:e769:fc3f:1:ffff:a:abab")), 0,
                         "ip_match() test: mid ip - fail")
        self.assertEqual(first.isIn(pytrap.UnirecIPAddr("fd71:5693:e769:fc3e:ffff:ffff:ffff:ffff")), -1,
                         "ip_match() test: low match - fail")
        self.assertEqual(first.isIn(pytrap.UnirecIPAddr("10:10:10::0")), -1,
                         "ip_match() test: low match - fail")
        self.assertEqual(first.isIn(pytrap.UnirecIPAddr("fd71:5693:e769:fc40:0:0:0:0")), 1,
                         "ip_match() test: great match - fail")
        self.assertEqual(first.isIn(pytrap.UnirecIPAddr("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")), 1,
                         "ip_match() test: great match - fail")

        first.add_data(["ccc", "ddd"])
        self.assertEqual(first.get_data(), ['aaa', 'bbb', 'ccc', 'ddd'], "add_data() test: multiple data - fail")
        self.assertEqual(len(first), 4, "add_data() test: multiple data length - fail")

        intex2 = ip_prefix_search.IPPSInterval("fd71:5693:e769:fc3f:0000:0000:0000:0000",
                              "fd71:5693:e769:fc3f:7fff:ffff:ffff:ffff",
                              "bbb")

        result = intex2.isOverlap(first)
        self.assertTrue(result, "Modulo operator - match IPAddr in interval - fail")
        self.assertEqual(len(intex2), 1, "init data test: simple data length - fail")

        intex2 = ip_prefix_search.IPPSInterval("fd71:5693:e769:fc3f:7000:0000:0000:0000", "fd71:5693:e769:fc50:0000:0000:0000:0000")
        result = first.isOverlap(intex2)
        self.assertTrue(result, "isOverlap - match IPAddr in interval - fail")

        intex2 = ip_prefix_search.IPPSInterval("fd71:5693:e769:fc3f:7000:0000:0000:0000", "fd71:5693:e769:fc3f:ffff:ffff:ffff:ffff")
        result = first.isOverlap(intex2)
        self.assertTrue(result, "isOverlap - match IPAddr in interval - fail")

        intex2 = ip_prefix_search.IPPSInterval(pytrap.UnirecIPAddr("fd71:5693:e769:fc40:0000:0000:0000:0000"),
                              "fd71:5693:e769:fc40:7000:0000:0000:0000")
        result = first.isOverlap(intex2)
        self.assertFalse(result, "Modulo operator - no match IPAddr in interval -fail")

        result = first == intex2
        self.assertFalse(result, "Equal operator - not eq - fail")

        intex2 = ip_prefix_search.IPPSInterval("fd71:5693:e769:fc3f:0000:0000:0000:0000", "fd71:5693:e769:fc40:7000:0000:0000:0000")
        result = first == intex2
        self.assertFalse(result, "Equal operator - not eq - fail")

        intex2 = ip_prefix_search.IPPSInterval("fd71:5693:e769:fc3f:0000:0000:0000:0000/66")
        result = first == intex2
        self.assertFalse(result, "Equal operator - not eq - fail")

        intex2 = ip_prefix_search.IPPSInterval("fd71:5693:e769:fc3f:0000:0000:0000:0000/64")
        result = first == intex2
        self.assertTrue(result, "Equal operator - eq - fail")
        self.assertEqual(len(intex2), 0, "init data test: simple data length - fail")


class DataTypesIPPPContext(unittest.TestCase):
        def runTest(self):
            import ip_prefix_search
            import pytrap

            input_data_v4 = [
                ["192.168.1.5/25", "a"],
                ["192.168.1.2/24", "b"],
                ["192.168.1.130/25", "c"],
                ["192.168.1.7/26", "d"],
                ["192.168.1.250/26", "e"],
                ["192.168.2.0/24", "f"],
                ["192.168.1.150/28", "g"],
                ["192.255.255.255/32", "h"],
                ["0.0.0.0/1", "i"],
                ["255.255.255.255/2", "j"],
                ["0.0.0.0/32 ", "k"],
                ["10.10.10.10/32", "l"]
            ]
            result_ip_v4 = [
                ["0.0.0.0", "0.0.0.0", ['i', 'k']],
                ["0.0.0.1", "10.10.10.9", ['i']],
                ["10.10.10.10", "10.10.10.10", ['i', 'l']],
                ["10.10.10.11", "127.255.255.255", ['i']],
                ["192.0.0.0", "192.168.0.255", ['j']],
                ["192.168.1.0", "192.168.1.63", ['j', 'b', 'a', 'd']],
                ["192.168.1.64", "192.168.1.127", ['j', 'b', 'a']],
                ["192.168.1.128", "192.168.1.143", ['j', 'b', 'c']],
                ["192.168.1.144", "192.168.1.159", ['j', 'b', 'c', 'g']],
                ["192.168.1.160", "192.168.1.191", ['j', 'b', 'c']],
                ["192.168.1.192", "192.168.1.255", ['j', 'b', 'c', 'e']],
                ["192.168.2.0", "192.168.2.255", ['j', 'f']],
                ["192.168.3.0", "192.255.255.254", ['j']],
                ["192.255.255.255", "192.255.255.255", ['j', 'h']],
                ["193.0.0.0", "255.255.255.255", ['j']]
            ]

            input_data_v6 = [
                ["ff37:3b22:507d:a4f9:0:0:a:a/64", "a"],
                ["fd37:3b22:507d:a4f9:0:0:1:1/64", "b"],
                ["fd37:3b22:507d:a4f9:7000:0:0:0/65", "c"],
                ["fd37:3b22:507d:a4f9::0:1:20/67", "d"],
                ["fd37:3b22:507d:a4f9:ffff:ffff:ff:0/67", "e"],
                ["fd37:3b22:507d:a500:0:0:0:0/64", "f"],
                ["fd37:3b22:507d:a4f9:ffff:fe00:abab:0/70", "g"],
                ["fd37:3b22:ffff:ffff:ffff:ffff:ffff:ffff/128", "h"],
                ["0::0/1", "i"],
                ["ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/2", "j"],
                ["0::0/128", "k"],
                ["10:10:10::10/128", "l"]
            ]
            result_ip_v6 = [
                   ["::", "0000:0000:0000:0000:0000:0000:0000:0000", ['i', 'k']],
                   ["::0001", "0010:0010:0010:0000:0000:0000:0000:000f", ['i']],
                   ["0010:0010:0010:0000:0000:0000:0000:0010", "0010:0010:0010:0000:0000:0000:0000:0010", ['i', 'l']],
                   ["0010:0010:0010:0000:0000:0000:0000:0011", "7fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", ['i']],
                   ["c000:0000:0000:0000:0000:0000:0000:0000", "fd37:3b22:507d:a4f8:ffff:ffff:ffff:ffff", ['j']],
                   ["fd37:3b22:507d:a4f9:0000:0000:0000:0000", "fd37:3b22:507d:a4f9:1fff:ffff:ffff:ffff", ['j', 'b', 'c', 'd']],
                   ["fd37:3b22:507d:a4f9:2000:0000:0000:0000", "fd37:3b22:507d:a4f9:7fff:ffff:ffff:ffff", ['j', 'b', 'c']],
                   ["fd37:3b22:507d:a4f9:8000:0000:0000:0000", "fd37:3b22:507d:a4f9:dfff:ffff:ffff:ffff", ['j', 'b']],
                   ["fd37:3b22:507d:a4f9:e000:0000:0000:0000", "fd37:3b22:507d:a4f9:fbff:ffff:ffff:ffff", ['j', 'b', 'e']],
                   ["fd37:3b22:507d:a4f9:fc00:0000:0000:0000", "fd37:3b22:507d:a4f9:ffff:ffff:ffff:ffff", ['j', 'b', 'e', 'g']],
                   ["fd37:3b22:507d:a4fa:0000:0000:0000:0000", "fd37:3b22:507d:a4ff:ffff:ffff:ffff:ffff", ['j']],
                   ["fd37:3b22:507d:a500:0000:0000:0000:0000", "fd37:3b22:507d:a500:ffff:ffff:ffff:ffff", ['j', 'f']],
                   ["fd37:3b22:507d:a501:0000:0000:0000:0000", "fd37:3b22:ffff:ffff:ffff:ffff:ffff:fffe", ['j']],
                   ["fd37:3b22:ffff:ffff:ffff:ffff:ffff:ffff", "fd37:3b22:ffff:ffff:ffff:ffff:ffff:ffff", ['j', 'h']],
                   ["fd37:3b23:0000:0000:0000:0000:0000:0000", "ff37:3b22:507d:a4f8:ffff:ffff:ffff:ffff", ['j']],
                   ["ff37:3b22:507d:a4f9:0000:0000:0000:0000", "ff37:3b22:507d:a4f9:ffff:ffff:ffff:ffff", ['j', 'a']],
                   ["ff37:3b22:507d:a4fa:0000:0000:0000:0000", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", ['j']]
            ]

            context = ip_prefix_search.IPPSContext([ip_prefix_search.IPPSNetwork(line[0], line[1]) for line in input_data_v4])

            for index, interval in enumerate(context.interval_list_v4):
                intex = ip_prefix_search.IPPSInterval(result_ip_v4[index][0],
                                     result_ip_v4[index][1],
                                     data=result_ip_v4[index][2])

                self.assertEqual(interval.start, intex.start, "IPv4 init low ip fail")
                self.assertEqual(interval.end, intex.end, "IPv4 init high ip fail")
                self.assertEqual(interval.get_data(), intex.get_data(), "IPv4 init data fail")

            context = ip_prefix_search.IPPSContext(
                [ip_prefix_search.IPPSNetwork(line[0], line[1]) for line in input_data_v6])

            for index, interval in enumerate(context.interval_list_v6):
                intex = ip_prefix_search.IPPSInterval(result_ip_v6[index][0],
                                     result_ip_v6[index][1],
                                     data=result_ip_v6[index][2])

                self.assertEqual(interval.start, intex.start,
                                 "IPv6 init low ip fail {0}  {1}".format(interval.start, intex.start))
                self.assertEqual(interval.end, intex.end, "IPv6 init high ip fail {0}   {1}".format(interval.end, intex.end))
                self.assertEqual(interval.get_data(), intex.get_data(), "IPv6 init data fail")

            networks = []

            for line_v4, line_v6 in zip(input_data_v4, input_data_v6):
                networks.append(ip_prefix_search.IPPSNetwork(line_v4[0], line_v4[1]))
                networks.append(ip_prefix_search.IPPSNetwork(line_v6[0], line_v6[1]))

            context = ip_prefix_search.IPPSContext(networks)

            for index, interval in enumerate(context.interval_list_v4):
                intex = ip_prefix_search.IPPSInterval(result_ip_v4[index][0],
                                     result_ip_v4[index][1],
                                     data=result_ip_v4[index][2])

                self.assertEqual(interval.start, intex.start, "IPv4 in mix init, low ip fail")
                self.assertEqual(interval.end, intex.end, "IPv4 in mix init, high ip fail")
                self.assertEqual(interval.get_data(), intex.get_data(), "IPv4 in mix init, data fail")

            for index, interval in enumerate(context.interval_list_v6):
                intex = ip_prefix_search.IPPSInterval(result_ip_v6[index][0],
                                     result_ip_v6[index][1],
                                     data=result_ip_v6[index][2])

                self.assertEqual(interval.start, intex.start, "IPv6 in mix init, low ip fail")
                self.assertEqual(interval.end, intex.end, "IPv6 in mix init, high ip fail")
                self.assertEqual(interval.get_data(), intex.get_data(), "IPv6 in mix init, data fail")

            networks = []

            for line_v4, line_v6 in zip(input_data_v4, input_data_v6):
                networks.append(ip_prefix_search.IPPSNetwork(line_v4[0], line_v4[1]))
                networks.append(ip_prefix_search.IPPSNetwork(line_v6[0], line_v6[1]))

            context = ip_prefix_search.IPPSContext(networks)

            self.assertEqual(context.ip_search(pytrap.UnirecIPAddr("0.0.0.0")), ['i', 'k'],
                             "IP search - lowest ip address search - fail")
            self.assertEqual(context.ip_search(pytrap.UnirecIPAddr("255.255.255.255")), ['j'],
                             "IP search - highest ip address search - fail")
            self.assertEqual(context.ip_search(pytrap.UnirecIPAddr("192.168.1.165")), ['j', 'b', 'c'],
                             "IP search - middle ip address search - fail")
            self.assertEqual(context.ip_search(pytrap.UnirecIPAddr("192.168.3.0")), ['j'],
                             "IP search - middle ip address search - fail")
            self.assertFalse(context.ip_search(pytrap.UnirecIPAddr("130.255.255.255")),
                             "IP search - no match ip address search - fail")
            self.assertFalse(context.ip_search(pytrap.UnirecIPAddr("191.255.255.255")),
                             "IP search - no match ip address search - fail")
            self.assertFalse(context.ip_search(pytrap.UnirecIPAddr("128.0.0.0")),
                             "IP search - no match ip address search - fail")

            self.assertEqual(context.ip_search(pytrap.UnirecIPAddr("0::0")), ['i', 'k'],
                             "IP search - lowest ip address search - fail")
            self.assertEqual(context.ip_search(pytrap.UnirecIPAddr("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")), ['j'],
                             "IP search - highest ip address search - fail")
            self.assertEqual(context.ip_search(pytrap.UnirecIPAddr("fd37:3b22:507d:a4f9:8000:00aa:0000:aaa")), ['j', 'b'],
                             "IP search - middle ip address search - fail")
            self.assertEqual(context.ip_search(pytrap.UnirecIPAddr("fd37:3b22:507d:a501:0000:0000:0000:0000")), ['j'],
                             "IP search - middle ip address search - fail")
            self.assertEqual(context.ip_search(pytrap.UnirecIPAddr("7fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")), ['i'],
                             "IP search - middle ip address search - fail")
            self.assertFalse(context.ip_search(pytrap.UnirecIPAddr("8000:0000:0000:0000:0000:0000:0000:0000")),
                             "IP search - no match ip address search - fail")
            self.assertFalse(context.ip_search(pytrap.UnirecIPAddr("9000:abcd:123:5678:2456:abce:eee:ffee")),
                             "IP search - no match ip address search - fail")
            self.assertFalse(context.ip_search(pytrap.UnirecIPAddr("bfff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")),
                             "IP search - no match ip address search - fail")
