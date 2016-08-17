#!/usr/bin/env python

"""
IP prefix search (IPPS)

    Library for searching any ip address in network prefixes.

    Class IPPSContext is an ordered list structure that is used to store a dynamic set,
where the keys are low and high IP addresses of prefix. Binary search compare low and high IP
with searched IP address and return data associated with match prefix.

    The prefix array can be used for storing any data (string, int). It can be use for example for
the aggregation of information from multiple blacklists.

"""
import sys
import os.path
from unirec import IPAddr, IPRange, IP4Addr, IP6Addr
from collections import namedtuple as _nt


sys.path.append(os.path.join(os.path.dirname(__file__), "..", "..", "python"))
sys.path.append(os.path.join(os.path.dirname(__file__), "..", "python"))
sys.path.append(os.path.join(os.path.dirname(__file__), ".."))


class IPPSNetwork(object):
    """
    Network class
    Represent network with associated data.
        - Network is set as simple string in prefix format e.g. '192.168.1.0/24'
        - Data type is arbitrary
    """
    def __init__(self, addr, data=None):

        if isinstance(addr, str):
            self.addr = addr  # ip address string with prefix in format: 192.168.1.1/24
        else:
            raise TypeError("Can't convert object of type to IP4Addr or IP6Addr.")


        self.data = data    # any data

    def __str__(self):
        return '{} {}'.format(self.addr, self.data)

    def __repr__(self):
        return "IPPSNetwork(" + str(self) + ")"


IPIntParentType = _nt("IPPSInterval", ["start", "end", "data"])


class IPPSInterval(IPIntParentType):
    """
    IP prefix search Interval class
    Represent IP interval range (from start IP to end IP) and store all assoc data
    Params may be:
        - string represent network in prefix format e.g. '192.168.1.0/25'
        - 2 strings or IPAddrs struct represent start and end ip addresses of interval
        - data in arbitrary type
    e.g.    IPPSInterval("192.168.1.0/24", data="xyz")
            IPPSInterval("192.168.1.0", "192.168.1.255", "xyz")
            IPPSInterval(IPAddr("192.168.1.0"), IPAddr("192.168.1.255"), "xyz")

    """
    def __new__(cls, param1, param2=None, data=None):
        tmp = IPRange.__new__(IPRange, param1, param2)
        return IPIntParentType.__new__(cls, tmp.start, tmp.end, data)

    def __init__(self, param1, param2=None, data=None):
        super(IPPSInterval, self).__init__(param1, param2)
        self._data = []
        if data is not None:
            self.add_data(data)

    def __str__(self):
        return '{} - {} {}'.format(self.start, self.end, self._data)

    def __repr__(self):
        return "IPPSInterval("+str(self)+")"

    def __contains__(self, item):
        return item in self._data

    def __lt__(self, other):
        if isinstance(other, IPPSInterval):
            if self.start == other.start:
                return self.end > other.end
            else:
                return self.start < other.start
        else:
            raise TypeError("Can't compare object with IPPSInterval.")

    def __eq__(self, other):
        if isinstance(other, IPPSInterval):
            return self.start == other.start and self.end == other.end
        else:
            raise TypeError("Can't compare object with IPPSInterval.")

    def __mod__(self, other):
        """
        Compare if 2 sorted intervals are overlaps
        :param other: instance of IPPSInterval
        :return: True if intervals overlaps, False if not overlaps
        """
        if isinstance(other, IPPSInterval):
            return self.end > other.start
        else:
            raise TypeError("Can't compare object with IPPSInterval")

    def __len__(self):
        return len(self._data)

    def get_data(self):
        return self._data

    def add_data(self, new_data):
        """
        Add data to private data array _data
        :param new_data: any inserted data
        :return: void
        """
        if isinstance(new_data, list):
            self._data.extend(new_data)
        else:
            self._data.append(new_data)

    def ip_match(self, ip):
        """
        Compare ip address with interval star and end addresses
        :param ip: IPAddr object with searched ip address
        :return: 0 if ip match in interval range,
                >0 if ip is grater then interval,
                <0 if ip is smaller then interval
        """
        if isinstance(ip, type(self.start)):
            if self.start <= ip:
                if self.end >= ip:
                    return 0
                else:
                    return 1
            else:
                return -1
        else:
            raise TypeError("Object isn't IPAddr")


class IPPSContext(object):
    """
    IP prefix search Context class
    Represent context of networks (overlaps intervals), processed for prefix search

    Params:
        - list of IPPSNetwork objects
    """

    def __init__(self, val):
        self.interval_list_v4 = []
        self.interval_list_v6 = []
        self.list_len_v4 = 0
        self.list_len_v6 = 0

        self.list_init(val)

    def __repr__(self):
        return "IPPSContext("+str(self)+")"

    def __str__(self):
        return '{}\n{}'.format([str(item) for item in self.interval_list_v4], [str(item) for item in self.interval_list_v6])

    @classmethod
    def fromFile(cls, val):
        if isinstance(val, str):
            network_list = []
            with open(val, "r") as f:
                for line in f:
                    parse = line.rstrip().split(",")
                    if '/' not in parse[0]:
                        parse[0] += "/32"

                    if len(parse) == 1:
                        network_list.append(IPPSNetwork(parse[0]))
                    else:
                        network_list.append(IPPSNetwork(parse[0], parse[1:]))
            f.close()
            return cls(network_list)
        else:
            return None

    @staticmethod
    def split_overlaps_intervals(sort_intvl_list):
        interval_list = []

        if len(sort_intvl_list) < 1:
            return

        interval_list.append(sort_intvl_list.pop(0))

        hop = 0     # optimization variable, index of last Interval inserted to interval_list

        # Insert all intervals from sort_intvl_list to interval_list.
        # If interval overlaps, split intervals (compute outer IP addrs, merge data) and delete old interval
        for interval in sort_intvl_list:
            for idx, inte in enumerate(interval_list[hop:]):
                if inte % interval:
                    if interval.start == inte.start:
                        if interval.end == inte.end:
                            # <-------> Inte
                            # <-------> Interval
                            inte.add_data(interval.get_data())
                            break
                        elif interval.end > inte.end:
                            return TypeError("Forbidden network interval compare. init fail")
                        else:
                            # <-------> Inte
                            # <----->   Interval
                            index = idx + hop
                            interval_list.insert(index,
                                                 IPPSInterval(interval.end + 1,
                                                              inte.end,
                                                              inte.get_data())
                                                 )

                            interval_list.insert(index,
                                                 IPPSInterval(interval.start,
                                                              interval.end,
                                                              inte.get_data() + interval.get_data())
                                                 )

                            interval_list.pop(index + 2)
                            break

                    elif interval.start > inte.start:
                        if interval.end == inte.end:
                            # <-------> Inte
                            #   <-----> Interval
                            index = idx + hop
                            interval_list.insert(index,
                                                 IPPSInterval(interval.start,
                                                              inte.end,
                                                              inte.get_data() + interval.get_data())
                                                 )

                            interval_list.insert(index,
                                                 IPPSInterval(inte.start,
                                                              interval.start - 1,
                                                              inte.get_data())
                                                 )

                            interval_list.pop(index + 2)
                            hop += 1
                            break
                        elif interval.end > inte.end:
                            return TypeError("Forbidden network interval compare. init fail")
                        else:
                            # <-------> Inte
                            #   <--->   Interval
                            index = idx + hop

                            interval_list.insert(index, IPPSInterval(interval.end + 1,
                                                                         inte.end,
                                                                         inte.get_data())
                                                 )

                            interval_list.insert(index,
                                                 IPPSInterval(interval.start,
                                                              interval.end,
                                                              inte.get_data() + interval.get_data())
                                                 )

                            interval_list.insert(index,
                                                 IPPSInterval(inte.start,
                                                              interval.start - 1,
                                                              inte.get_data())
                                                 )
                            interval_list.pop(index + 3)
                            hop += 1
                            break
                    else:
                        return TypeError("Forbidden network interval compare. init fail")
            else:
                interval_list.append(interval)
                hop += 1
        return interval_list

    def list_init(self, network_list):
        """ Initialization of interval list from networks in network_list
        Function create IPPSNetwork from IPPSNetwork, sort and split the overlaps intervals

        Args:
            network_list: list of IPPSNetwork objects
        Returns:
            the list of IPPSInterval objects
        Raises:
            TypeError: if network_list is not a list or node in list is not IPPSNetwork
        """

        if not isinstance(network_list, list):
            return TypeError("Init need list of IPPSNetworks")

        sort_intvl_list_v4 = []    # temporary list for convert  and sort all IPPSNetworks to IPPSIntervals
        sort_intvl_list_v6 = []    # temporary list for convert  and sort all IPPSNetworks to IPPSIntervals

        # Convert all Networks to Intervals
        for net in network_list:
            if not isinstance(net, IPPSNetwork):
                return TypeError("Object isn't IPPSNetworks")

            new_interval = IPPSInterval(net.addr, data=net.data)
            if isinstance(new_interval.start, IP4Addr) and isinstance(new_interval.end, IP4Addr):
                sort_intvl_list_v4.append(new_interval)
            elif isinstance(new_interval.start, IP6Addr) and isinstance(new_interval.end, IP6Addr):
                sort_intvl_list_v6.append(new_interval)
            else:
                raise TypeError("Object isn't IP4Addr or IP6Addr")

        if len(sort_intvl_list_v4) > 0:
            # Sort list by start ip addresses. If IP are equal, greater network first
            sort_intvl_list_v4.sort()
            self.interval_list_v4 = self.split_overlaps_intervals(sort_intvl_list_v4[:])
            self.list_len_v4 = len(self.interval_list_v4)

        if len(sort_intvl_list_v6) > 0:
            # Sort list by start ip addresses. If IP are equal, greater network first
            sort_intvl_list_v6.sort()
            self.interval_list_v6 = self.split_overlaps_intervals(sort_intvl_list_v6[:])
            self.list_len_v6 = len(self.interval_list_v6)

    def ip_search(self, ip):
        """ Binary search ip address in context interval_list
        Args:
            ip: IPAddr object, searched ip address
        Return:
            False, if ip address not match
            list with data, if ip address match
        Raises:
            TypeError: if ip is not a IPAddr object
        """
        if isinstance(ip, IP4Addr):
            interval_list = self.interval_list_v4
            last = self.list_len_v4 - 1

        elif isinstance(ip, IP6Addr):
            interval_list = self.interval_list_v6
            last = self.list_len_v6 - 1
        else:
            raise TypeError("Can't search object. Required type is IP4Addr or IP6Addr.")

        first = 0

        while first <= last:
            midpoint = (first + last) >> 1
            match = interval_list[midpoint].ip_match(ip)
            if match == 0:
                return interval_list[midpoint].get_data()
            elif match < 0:
                last = midpoint - 1
            else:
                first = midpoint + 1
        return False


# def main(argv):
    # # create new network context from IPPSNetworks
    # search_context = IPPSContext.fromFile("blacklist.txt")
    #
    # # print new context and all his interval lists
    # print search_context
    #
    # # find some IP addresses
    # result = search_context.ip_search(IPAddr.fromString("1.2.3.4"))
    # if result:
    #     print result
    # else:
    #     print "There is no match in this network context"
    #
    # result = search_context.ip_search(IPAddr.fromString("aab::bba"))
    # if result:
    #     print result
    # else:
    #     print "There is no match in this network context"


if __name__ == '__main__':
    # main(sys.argv)
    import unittest


    class IPPSTest(unittest.TestCase):
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

        def test_network(self):
            netx = IPPSNetwork("192.168.1.1/24", "aaa")
            self.assertIsInstance(netx, IPPSNetwork)
            self.assertEqual(netx.addr, "192.168.1.1/24")
            self.assertEqual(netx.data, "aaa")
            with self.assertRaises(TypeError):
                IPPSNetwork(19216811, "aaa")

            netx = IPPSNetwork("0::1/24", "aaa")
            self.assertIsInstance(netx, IPPSNetwork)
            self.assertEqual(netx.addr, "0::1/24")
            self.assertEqual(netx.data, "aaa")
            with self.assertRaises(TypeError):
                IPPSNetwork(19216811, "aaa")

        def test_interval_v4(self):
            with self.assertRaises(ValueError):
                IPPSInterval("192.168.1.0")

            intex = IPPSInterval("192.168.1.0/24")
            self.assertIsInstance(intex, IPPSInterval)
            self.assertIsInstance(intex.start, IP4Addr)
            self.assertIsInstance(intex.end, IP4Addr)
            self.assertEqual(IPPSInterval("192.168.1.0/24").get_data(), [], "get_data() test: empty data - fail")

            self.assertEqual(IPPSInterval("192.168.1.0/24", data="aaa").get_data(), ['aaa'], "get_data() test: init data - fail")

            intex = IPPSInterval("192.168.1.0", "192.168.1.255", "aaa")
            intex.add_data("bbb")
            self.assertEqual(intex.get_data(), ['aaa', 'bbb'], "add_data() test: simple data - fail")
            self.assertEqual(len(intex), 2, "add_data() test: simple data length - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("192.168.1.0")), 0, "ip_match() test: first ip - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("192.168.1.255")), 0, "ip_match() test: last ip - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("192.168.1.125")), 0, "ip_match() test: mid ip - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("192.168.0.255")), -1, "ip_match() test: low match - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("10.10.10.0")), -1, "ip_match() test: low match - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("192.168.2.0")), 1, "ip_match() test: great match - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("255.255.255.255")), 1, "ip_match() test: great match - fail")

            intex.add_data(["ccc", "ddd"])
            self.assertEqual(intex.get_data(), ['aaa', 'bbb', 'ccc', 'ddd'], "add_data() test: multiple data - fail")
            self.assertEqual(len(intex), 4, "add_data() test: multiple data length - fail")

            intex2 = IPPSInterval(IPAddr.fromString("192.168.1.0"), IPAddr.fromString("192.168.1.128"), "bbb")
            result = intex % intex2
            self.assertTrue(result, "Modulo operator - match IPAddr in interval - fail")
            self.assertEqual(len(intex2), 1, "init data test: simple data length - fail")

            intex2 = IPPSInterval("192.168.1.128", "192.168.2.255")
            result = intex % intex2
            self.assertTrue(result, "Modulo operator - match IPAddr in interval - fail")

            intex2 = IPPSInterval("192.168.1.128", "192.168.2.172")
            result = intex % intex2
            self.assertTrue(result, "Modulo operator - match IPAddr in interval - fail")

            intex2 = IPPSInterval("192.168.2.0", "192.168.2.128")
            result = intex % intex2
            self.assertFalse(result, "Modulo operator - no match IPAddr in interval -fail")

            intex2 = IPPSInterval("192.168.2.0", "192.168.2.128")
            result = intex == intex2
            self.assertFalse(result, "Equal operator - not eq - fail")

            intex2 = IPPSInterval(IPAddr.fromString("192.168.1.0"), "192.168.2.128")
            result = intex == intex2
            self.assertFalse(result, "Equal operator - not eq - fail")

            intex2 = IPPSInterval("192.168.1.128/25")
            result = intex == intex2
            self.assertFalse(result,  "Equal operator - not eq - fail")

            intex2 = IPPSInterval("192.168.1.0", IPAddr.fromString("192.168.1.255"))
            result = intex == intex2
            self.assertTrue(result, "Equal operator - eq - fail")
            self.assertEqual(len(intex2), 0, "init data test: simple data length - fail")

        def test_interval_v6(self):
            with self.assertRaises(ValueError):
                IPPSInterval("fd71:5693:e769:fc3f::")

            intex = IPPSInterval("fd71:5693:e769:fc3f::/64")
            self.assertIsInstance(intex, IPPSInterval)
            self.assertIsInstance(intex.start, IP6Addr)
            self.assertIsInstance(intex.end, IP6Addr)

            self.assertEqual(intex.get_data(), [], "get_data() test: empty data - fail")
            self.assertEqual(IPPSInterval("fd71:5693:e769:fc3f::/64", data="aaa").get_data(), ['aaa'], "get_data() test: init data - fail")

            intex = IPPSInterval("fd71:5693:e769:fc3f:0:0:0:0", "fd71:5693:e769:fc3f:ffff:ffff:ffff:ffff", "aaa")
            intex.add_data("bbb")
            self.assertEqual(intex.get_data(), ['aaa', 'bbb'], "add_data() test: simple data - fail")
            self.assertEqual(len(intex), 2, "add_data() test: simple data length - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("fd71:5693:e769:fc3f:0:0:0:0")), 0, "ip_match() test: first ip - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("fd71:5693:e769:fc3f:ffff:ffff:ffff:ffff")), 0, "ip_match() test: last ip - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("fd71:5693:e769:fc3f:1:ffff:a:abab")), 0, "ip_match() test: mid ip - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("fd71:5693:e769:fc3e:ffff:ffff:ffff:ffff")), -1, "ip_match() test: low match - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("10:10:10::0")), -1, "ip_match() test: low match - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("fd71:5693:e769:fc40:0:0:0:0")), 1, "ip_match() test: great match - fail")
            self.assertEqual(intex.ip_match(IPAddr.fromString("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")), 1, "ip_match() test: great match - fail")

            intex.add_data(["ccc", "ddd"])
            self.assertEqual(intex.get_data(), ['aaa', 'bbb', 'ccc', 'ddd'], "add_data() test: multiple data - fail")
            self.assertEqual(len(intex), 4, "add_data() test: multiple data length - fail")

            intex2 = IPPSInterval(IPAddr.fromString("fd71:5693:e769:fc3f:0000:0000:0000:0000"),
                                  IPAddr.fromString("fd71:5693:e769:fc3f:7fff:ffff:ffff:ffff"),
                                  "bbb")
            result = intex % intex2
            self.assertTrue(result, "Modulo operator - match IPAddr in interval - fail")
            self.assertEqual(len(intex2), 1, "init data test: simple data length - fail")

            intex2 = IPPSInterval("fd71:5693:e769:fc3f:7000:0000:0000:0000", "fd71:5693:e769:fc50:0000:0000:0000:0000")
            result = intex % intex2
            self.assertTrue(result, "Modulo operator - match IPAddr in interval - fail")

            intex2 = IPPSInterval("fd71:5693:e769:fc3f:7000:0000:0000:0000", "fd71:5693:e769:fc3f:ffff:ffff:ffff:ffff")
            result = intex % intex2
            self.assertTrue(result, "Modulo operator - match IPAddr in interval - fail")

            intex2 = IPPSInterval("fd71:5693:e769:fc40:0000:0000:0000:0000", "fd71:5693:e769:fc40:7000:0000:0000:0000")
            result = intex % intex2
            self.assertFalse(result, "Modulo operator - no match IPAddr in interval -fail")

            result = intex == intex2
            self.assertFalse(result, "Equal operator - not eq - fail")

            intex2 = IPPSInterval(IPAddr.fromString("fd71:5693:e769:fc3f:0000:0000:0000:0000"), "fd71:5693:e769:fc40:7000:0000:0000:0000")
            result = intex == intex2
            self.assertFalse(result, "Equal operator - not eq - fail")

            intex2 = IPPSInterval("fd71:5693:e769:fc3f:0000:0000:0000:0000/66")
            result = intex == intex2
            self.assertFalse(result, "Equal operator - not eq - fail")

            intex2 = IPPSInterval("fd71:5693:e769:fc3f:0000:0000:0000:0000/64")
            result = intex == intex2
            self.assertTrue(result, "Equal operator - eq - fail")
            self.assertEqual(len(intex2), 0, "init data test: simple data length - fail")

        def test_init_v4(self):

            context = IPPSContext([IPPSNetwork(line[0], line[1]) for line in self.input_data_v4])

            for index, interval in enumerate(context.interval_list_v4):
                intex = IPPSInterval(self.result_ip_v4[index][0],
                                     self.result_ip_v4[index][1],
                                     data=self.result_ip_v4[index][2])

                self.assertEqual(interval.start, intex.start,   "IPv4 init low ip fail")
                self.assertEqual(interval.end, intex.end,       "IPv4 init high ip fail")
                self.assertEqual(interval.get_data(), intex.get_data(), "IPv4 init data fail")

        def test_init_v6(self):

            context = IPPSContext([IPPSNetwork(line[0], line[1]) for line in self.input_data_v6])

            for index, interval in enumerate(context.interval_list_v6):
                intex = IPPSInterval(self.result_ip_v6[index][0],
                                     self.result_ip_v6[index][1],
                                     data=self.result_ip_v6[index][2])

                self.assertEqual(interval.start, intex.start,   "IPv6 init low ip fail {}  {}".format(interval.start, intex.start))
                self.assertEqual(interval.end, intex.end,       "IPv6 init high ip fail {}   {}".format(interval.end, intex. end))
                self.assertEqual(interval.get_data(), intex.get_data(), "IPv6 init data fail")

        def test_init_v4v6(self):
            networks = []

            for line_v4, line_v6 in zip(self.input_data_v4, self.input_data_v6):
                networks.append(IPPSNetwork(line_v4[0], line_v4[1]))
                networks.append(IPPSNetwork(line_v6[0], line_v6[1]))

            context = IPPSContext(networks)

            for index, interval in enumerate(context.interval_list_v4):
                intex = IPPSInterval(self.result_ip_v4[index][0],
                                     self.result_ip_v4[index][1],
                                     data=self.result_ip_v4[index][2])

                self.assertEqual(interval.start, intex.start,   "IPv4 in mix init, low ip fail")
                self.assertEqual(interval.end, intex.end,       "IPv4 in mix init, high ip fail")
                self.assertEqual(interval.get_data(), intex.get_data(), "IPv4 in mix init, data fail")

            for index, interval in enumerate(context.interval_list_v6):
                intex = IPPSInterval(self.result_ip_v6[index][0],
                                     self.result_ip_v6[index][1],
                                     data=self.result_ip_v6[index][2])

                self.assertEqual(interval.start, intex.start,   "IPv6 in mix init, low ip fail")
                self.assertEqual(interval.end, intex.end,       "IPv6 in mix init, high ip fail")
                self.assertEqual(interval.get_data(), intex.get_data(), "IPv6 in mix init, data fail")

        def test_ip_binary_search(self):
            networks = []

            for line_v4, line_v6 in zip(self.input_data_v4, self.input_data_v6):
                networks.append(IPPSNetwork(line_v4[0], line_v4[1]))
                networks.append(IPPSNetwork(line_v6[0], line_v6[1]))

            context = IPPSContext(networks)

            self.assertEqual(context.ip_search(IP4Addr("0.0.0.0")), ['i', 'k'], "IP search - lowest ip address search - fail")
            self.assertEqual(context.ip_search(IP4Addr("255.255.255.255")), ['j'], "IP search - highest ip address search - fail")
            self.assertEqual(context.ip_search(IP4Addr("192.168.1.165")), ['j', 'b', 'c'], "IP search - middle ip address search - fail")
            self.assertEqual(context.ip_search(IP4Addr("192.168.3.0")), ['j'], "IP search - middle ip address search - fail")
            self.assertFalse(context.ip_search(IP4Addr("130.255.255.255")),  "IP search - no match ip address search - fail")
            self.assertFalse(context.ip_search(IP4Addr("191.255.255.255")),  "IP search - no match ip address search - fail")
            self.assertFalse(context.ip_search(IP4Addr("128.0.0.0")),  "IP search - no match ip address search - fail")

            self.assertEqual(context.ip_search(IP6Addr("0::0")), ['i', 'k'], "IP search - lowest ip address search - fail")
            self.assertEqual(context.ip_search(IP6Addr("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")), ['j'], "IP search - highest ip address search - fail")
            self.assertEqual(context.ip_search(IP6Addr("fd37:3b22:507d:a4f9:8000:00aa:0000:aaa")),  ['j', 'b'], "IP search - middle ip address search - fail")
            self.assertEqual(context.ip_search(IP6Addr("fd37:3b22:507d:a501:0000:0000:0000:0000")), ['j'], "IP search - middle ip address search - fail")
            self.assertEqual(context.ip_search(IP6Addr("7fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")), ['i'], "IP search - middle ip address search - fail")
            self.assertFalse(context.ip_search(IP6Addr("8000:0000:0000:0000:0000:0000:0000:0000")),  "IP search - no match ip address search - fail")
            self.assertFalse(context.ip_search(IP6Addr("9000:abcd:123:5678:2456:abce:eee:ffee")),  "IP search - no match ip address search - fail")
            self.assertFalse(context.ip_search(IP6Addr("bfff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")),  "IP search - no match ip address search - fail")

    unittest.main()
