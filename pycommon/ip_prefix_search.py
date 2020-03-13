#!/usr/bin/env python

"""
IP Prefix search (IPPS) module
-----------
    Library for binary searching any ip address in network prefixes.

    Class IPPSContext is an ordered list structure that is used to store a dynamic set,
where the keys are low and high IP addresses of prefix. Binary search compare low and high IP
with searched IP address and return data associated with match prefix.

    The prefix array can be used for storing any data (string, int). It can be use for example for
the aggregation of information from multiple blacklists.

    Before using the search create IPPSContext object. The init function creates all
necessary structures for storing and accessing the data and return object with IPv4 and
IPv6 array of prefixes with data (network context).
The parameters are:
    Array of IPPSNetwork objects.

    IPPSNetwork object represent IP prefix and data input. User can define his own input source parser and create IPPSarray
by yourself, or user can call built-in parser .fromFile():
For simple creation of IPPSContext without the IPPSNetwork array, call IPPSContext.fromFile(<path to whitelist file>).
Class method fromFile() parse given file and construct IPPSContext object.

Input file must be in format:
<ip address>/<mask>,<data>\n

    For searching, there is a IPPSContext method ip_search(<IPAddr>). Function return list of data
associated with match prefix. If there is no match return False.

For example, if blacklist contains :

    192.168.1.0/24,aaa
    192.168.1.0/25,bbb
    192.168.1.128/25,ccc

init() creates 2 intervals
    - from 1.0 to 1.127 with data "aaa" and "bbb"
    - from 1.128 to 1.255 with data "aaa" and "ccc":

 192.168.1.0     192.168.1.255
       |            |
       <-----aaa---->
       <-bbb-><-ccc->


and ip_search() is call with 192.168.1.100, return array with "aaa" and "bbb".
For 192.168.1.200, return array with "aaa" and "ccc".
For 192.1.1.1, search return False

**************************************
EXAMPLE

def main(argv):
    # create new network blacklist context from file
    search_context = IPPSContext.fromFile("blacklist.txt")

    # print new context and all his interval lists
    print search_context

    # find some IP addresses
    result = search_context.ip_search(pytrap.UnirecIPAddr("1.2.3.4"))
    if result:
        print(result)
    else:
        print("There is no match in this network context")

    result = search_context.ip_search(pytrap.UnirecIPAddr("aab::bba"))
    if result:
        print(result)
    else:
        print("There is no match in this network context")

**************************************
"""
import sys, os.path
import pytrap

class IPPSNetwork(object):
    """ Network class
    Represent network with associated data.

    Args:
       addr : String represent prefix ip address
       data (optional): data assoc with ip prefix

    Attributes:
	addr (string) : prefix ip address. Set as simple string in prefix format e.g. '192.168.1.0/24'
        data : Data type is arbitrary
    """
    def __init__(self, addr, data=None):

        if isinstance(addr, str):
            self.addr = addr  # ip address string
        else:
            raise TypeError("Can't convert object of type to UnirecIPAddr, object isn't string")

        self.data = data    # any data

    def __str__(self):
        return "'{0}', '{1}'".format(self.addr, self.data)

    def __repr__(self):
        return "IPPSNetwork(" + str(self) + ")"


class IPPSInterval(pytrap.UnirecIPAddrRange):
    """
    IP prefix search Interval class
    Represent IP interval range (from start IP to end IP) and store all assoc data
    Args:
        - string represent network in prefix format e.g. '192.168.1.0/25'
        - 2 strings or UnirecIPAddr structs represent start and end ip addresses of interval
        - data in arbitrary type
    e.g.    IPPSInterval("192.168.1.0/24", data="xyz")
            IPPSInterval("192.168.1.0", "192.168.1.255", "xyz")
            IPPSInterval(pytrap.UnirecIPAddr("192.168.1.0"), pytrap.UnirecIPAddr("192.168.1.255"), "xyz")

    Attributes:
        start: low IP address of interval
        end  : high IP address of Interval
        _data: list of data object
    """
    def __new__(cls, param1, param2=None, data=None):
        if param2 is None:
            return super(IPPSInterval, cls).__new__(cls, param1)
        else:
            return super(IPPSInterval, cls).__new__(cls, param1, param2)

    def __init__(self, param1, param2=None, data=None):
        if param2 is None:
            super(IPPSInterval, self).__init__(param1)
        else:
            super(IPPSInterval, self).__init__(param1, param2)

        self._data = []
        if data is not None:
            self.add_data(data)

    def __str__(self):
        return '{0} - {1}, {2}'.format(self.start, self.end, self._data)

    def __repr__(self):
        return "IPPSInterval("+str(self)+")"

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

    def __len__(self):
        return len(self._data)

    def get_data(self):
        return self._data

    def add_data(self, new_data):
        """
        Add data to private data array _data
        Args:
            new_data: inserted data object (int, string...)
        Return: void
        """
        if isinstance(new_data, list):
            self._data.extend(new_data)
        else:
            self._data.append(new_data)


class IPPSContext(object):
    """
    IP prefix search Context class
    Represent context of networks (overlaps intervals), processed for prefix search

    Args:
        val: list of IPPSNetwork objects
    """

    def __init__(self, val):
        self.interval_list_v4 = []
        self.interval_list_v6 = []
        self.list_len_v4 = 0
        self.list_len_v6 = 0

        self.list_init(val)

    def __repr__(self):
        return "IPPSContext(" + str(self) + ")"

    def __str__(self):
        return 'IPv4{0}\nIPv6{1}'.format([str(item) for item in self.interval_list_v4],
                                       [str(item) for item in self.interval_list_v6])

    def __len__(self):
        return len(self.interval_list_v4) + len(self.interval_list_v6)

    @classmethod
    def fromFile(cls, path):
        """ Initialize IPPSContext from blacklist data file. Function parse source file
        and create IPPSNetwork structs from each line.
        Blacklist file must be in format:
	<ip address>/<mask>,<data>\n

        Args:
            path: path to source file
        Return:
            new IPPSContext or None if path isn't string
        """
        if isinstance(path, str):
            network_list = []
            with open(path, "r") as f:
                for line in f:
                    if line.isspace():
                        continue
                    parse = line.rstrip().split(",")
                    parse[0] = parse[0].strip()
                    if '/' not in parse[0]:
                        if ':' in parse[0]:
                            parse[0] += "/128"
                        else:
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
        """ Function split intervals and appropriate merge assoc data, if 2 intervals are overlap

        Args:
            sort_intvl_list: list of IPPSIntervals sorted by low IP address and IP mask
        Return:
            new list of nonoverlaping IPPSIntervals, ready to search
        Raises:
            TypeError: if sort_intvl_list is poorly sorted
        """
        interval_list = []

        if len(sort_intvl_list) < 1:
            return

        interval_list.append(sort_intvl_list.pop(0))

        hop = 0     # optimization variable, index of last Interval inserted to interval_list

        # Insert all intervals from sort_intvl_list to interval_list.
        # If interval overlaps, split intervals (compute outer IP addrs, merge data) and delete old interval
        for interval in sort_intvl_list:
            for idx, inte in enumerate(interval_list[hop:]):
                if inte.isOverlap(interval):
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
                                                 IPPSInterval(interval.end.inc(),
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
                                                              interval.start.dec(),
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

                            interval_list.insert(index, IPPSInterval(interval.end.inc(),
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
                                                              interval.start.dec(),
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
            if isinstance(new_interval.start, pytrap.UnirecIPAddr) and isinstance(new_interval.end, pytrap.UnirecIPAddr):
                if new_interval.start.isIPv4() and new_interval.end.isIPv4():
                    sort_intvl_list_v4.append(new_interval)
                elif new_interval.start.isIPv6() and new_interval.end.isIPv6():
                    sort_intvl_list_v6.append(new_interval)
                else:
                    raise TypeError("Object isn't IP4Addr or IP6Addr")
            else:
                raise TypeError("Object isn't IP4Addr or IP6Addr")

        if len(sort_intvl_list_v4) > 0:
            # Sort list by start ip addresses. If IP are equal, greater network first
            # Sort function call IPPSInterval.__lt__ for sorting
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
            True, if ip address is match, but list is empty (no data are set)
            list with data, if ip address is match and data list is not empty
        Raises:
            TypeError: if ip is not a UnriecIPAddr object
        """
        if isinstance(ip, pytrap.UnirecIPAddr):
            if ip.isIPv4():
                interval_list = self.interval_list_v4
                last = self.list_len_v4 - 1

            elif ip.isIPv6():
                interval_list = self.interval_list_v6
                last = self.list_len_v6 - 1
            else:
                raise TypeError("Can't search object. Required type is IP4Addr or IP6Addr.")
        else:
            raise TypeError("Can't search object. Required type is IP4Addr or IP6Addr.")

        first = 0

        while first <= last:
            middleindex = (first + last) >> 1
            midpoint = interval_list[middleindex]
            position = midpoint.isIn(ip)
            if position == 0:
                # ip address match
                if len(midpoint):
                    return midpoint.get_data()
                else:
                    return True
            elif position < 0:
                # ip address is lower then current midpoint
                last = middleindex - 1
            else:
                # ip address is higher then current midpoint
                first = middleindex + 1
        return False
