# ur_ipaddr.py - classes for representing IP addresses (supports UniRec format)
# Author: Vaclav Bartos (ibartosv@fit.vutbr.cz), 2012-2013

"""
Module with classes representing IPv4 and IPv6 addresses.

TODO: creation of IP4Range/IP6Range using address-netmask notation,
      i.e. "192.168.0.0/24" -> IP4Range(192.168.0.0, 192.168.0.255)
"""

from collections import namedtuple as _nt
import struct
import sys

if sys.version_info > (3,):
   long = int

__all__ = ['IPAddr', 'IP4Addr', 'IP6Addr', 'IPRange', 'IP4Range', 'IP6Range']

class IPAddr(long):
   """Abstract class for IPv4 and IPv6.

   You can use constructor to parse string IP addresses, it will recognize
   IP version automatically and return IP4Addr or IP6Addr class.
   To create IP addresses from int/long, you must use IP4Addr or
   IP6Addr directly.
   You can also use class method fromBytes to convert string of 4 or 16 bytes
   to IP address.
   """
   __slots__ = ()

   def __new__(cls, val = None):
      if val is None:
         return IP4Addr()
      elif isinstance(val, str):
         return cls.fromString(val)
      elif isinstance(val, long) or isinstance(val, int):
         raise TypeError("Use IP4Addr or IP6Addr to convert from a number.")
      else:
         raise TypeError("Can't convert object of type " + str(type(val)) + " to IP4Addr or IP6Addr.")

   @classmethod
   def fromBytes(cls, val):
      if len(val) == 4:
         return IP4Addr.fromBytes(val)
      elif len(val) == 16:
         return IP6Addr.fromBytes(val)
      else:
         raise ValueError("IPAddr.fromBytes takes only 4-byte or 16-byte string.")

   @classmethod
   def fromUniRec(cls, val):
      "Convert IP address in UniRec format to IP4Addr or IP6Addr"
      if len(val) != 16:
         raise ValueError("IPAddr.fromUniRec takes only a 16-byte string.")
      else:
         x = struct.unpack(">IIII", val)
         if x[0] == 0 and x[1] == 0 and x[3] == 0xffffffff:
            return IP4Addr.fromLong(x[2])
         else:
            return IP6Addr.fromBytes(val)

   @classmethod
   def fromString(cls, val):
      try:
         return IP4Addr.fromString(val)
      except ValueError:
         try:
            return IP6Addr.fromString(val)
         except ValueError:
            raise ValueError('"'+val+'" is not a valid IP Address (IPv4 nor IPv6).')




class IP4Addr(IPAddr):
   """
   Class representing IPv4 addresses.

   It is specialized long, but its string version is human-readable. It can
   also parse IP address given as a string.
   """
   __slots__ = ()

   def __new__(cls, val = long(0)):
      if isinstance(val, IP4Addr):
         return val
      elif isinstance(val, long) or isinstance(val, int):
         return cls.fromLong(val)
      elif isinstance(val, str):
         if len(val) == 4:
            return cls.fromBytes(val)
         return cls.fromString(val)
      else:
         raise TypeError("Can't convert object of type " + str(type(val)) + " to IP4Addr. Only str and int/long are valid.")

   @classmethod
   def fromLong(cls, val):
      if 0 <= val < 2**32:
         return long.__new__(cls, val)
      else:
         raise ValueError("IPv4 address must be between 0 and 2^32-1")

   @classmethod
   def fromBytes(cls, val):
      x = struct.unpack("BBBB", val)
      return long.__new__(cls, x[0]<<24 | x[1]<<16 | x[2]<<8 | x[3])

   @classmethod
   def fromString(cls, val):
      x = map(long, val.split('.'))
      if not (len(x) == 4 and all(map(lambda a:0<=a<256, x))):
         raise ValueError('String "' + val + '" is not a valid IPv4 address.')
      return long.__new__(cls, x[0]<<24 | x[1]<<16 | x[2]<<8 | x[3])


   def toBytes(self):
      return struct.pack(">I", long(self))

   def toUniRec(self):
      return struct.pack(">IIII", 0, 0, long(self), 0xffffffff)

   def __str__(self):
      x = long(self)
      return str((x >> 24) & 0xff) + "." + \
             str((x >> 16) & 0xff) + "." + \
             str((x >>  8) & 0xff) + "." + \
             str(x & 0xff)

   def __repr__(self):
      return "IP4Addr("+str(self)+")"

   def __add__(self, a):
      if isinstance(a, IP6Addr):
         raise TypeError("Can't sum IP4Addr and IP6Addr")
      return IP4Addr(long.__add__(self,a))

   def __sub__(self, a):
      if isinstance(a, IP6Addr):
         raise TypeError("Can't subtract IP6Addr from IP4Addr")
      return IP4Addr(long.__sub__(self,a))

   def __and__(self, a):
      if isinstance(a, IP6Addr):
         raise TypeError("Can't do logical 'and' of IP4Addr and IP6Addr")
      return IP4Addr(long.__and__(self,a))

   def __or__(self, a):
      if isinstance(a, IP6Addr):
         raise TypeError("Can't do logical 'or' of IP4Addr and IP6Addr")
      return IP4Addr(long.__or__(self,a))



class IP6Addr(IPAddr):
   """
   Class representing IPv6 addresses.

   It is specialized long, but its string version is human-readable. It can
   also parse IP address given as a string.
   """
   __slots__ = ()

   def __new__(cls, val = long(0)):
      if isinstance(val, IP6Addr):
         return val
      elif isinstance(val, long) or isinstance(val, int):
         return cls.fromLong(val)
      elif isinstance(val, str):
         # You must call fromBytes explicitly, 16 bytes can also be a human-readable string
         #if len(val) == 16:
         #   return fromBytes(val)
         return cls.fromString(val)
      else:
         raise TypeError("Can't convert object of type " + str(type(val)) + " to IP6Addr. Only str and int/long are valid.")

   @classmethod
   def fromLong(cls, val):
      if 0 <= val < 2**128:
         return long.__new__(cls, val)
      else:
         raise ValueError("IPv6 address must be between 0 and 2^128-1")

   @classmethod
   def fromBytes(cls, val):
      x = struct.unpack(">IIII", val)
      return long.__new__(cls, x[0]<<96 | x[1]<<64 | x[2]<<32 | x[3])

   @classmethod
   def fromString(cls, val):
      err = ValueError('String "' + val + '" is not a valid IPv6 address.')
      x = val.split("::")

      if len(x) == 1:    # there was no "::"
         x = x[0].split(":")
         if len(x) != 8:
            raise err

         # check if all values are between 0000 and ffff
         if not all(map(lambda a:0<=int(a,16)<=0xffff, x)):
            raise err

         # add leading zeros
         x = map(lambda a: "0"*(4-len(a))+a, x)

         # join it into string and convert it to long
         x = ''.join(x)
         return long.__new__(cls, x, 16)

      elif len(x) == 2:
         x1 = x[0].split(":")
         x2 = x[1].split(":")
         if x1 == ['']:
            x1 = []
         if x2 == ['']:
            x2 = []

         missing = 8 - (len(x1) + len(x2))
         if (missing < 0):
            raise err

         # check if all values are between 0000 and ffff
         if not all(map(lambda a:0<=int(a,16)<=0xffff, x1)):
            raise err
         if not all(map(lambda a:0<=int(a,16)<=0xffff, x2)):
            raise err

         # add leading zeros
         x1 = map(lambda a: "0"*(4-len(a))+a, x1)
         x2 = map(lambda a: "0"*(4-len(a))+a, x2)

         # join it into string and convert it to long
         x = ''.join(x1) + "0000"*missing + ''.join(x2)
         return long.__new__(cls, x, 16)
      else:
         raise err


   def toBytes(self):
      x = long(self)
      return struct.pack(">IIII", long((x >>  96) & 0xffffffff),
                                  long((x >>  64) & 0xffffffff),
                                  long((x >>  32) & 0xffffffff),
                                  long(x & 0xffffffff) )

   def toUniRec(self):
      return self.toBytes()

   def __str__(self):
      x = long(self)
      return "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x"%(
             int((x >> 112) & 0xffff),
             int((x >>  96) & 0xffff),
             int((x >>  80) & 0xffff),
             int((x >>  64) & 0xffff),
             int((x >>  48) & 0xffff),
             int((x >>  32) & 0xffff),
             int((x >>  16) & 0xffff),
             int(x & 0xffff) )

   def __repr__(self):
      return "IP6Addr("+str(self)+")"

   def __add__(self, a):
      if isinstance(a, IP4Addr):
         raise TypeError("Can't sum IP6Addr and IP4Addr")
      return IP6Addr(long.__add__(self,a))

   def __sub__(self, a):
      if isinstance(a, IP4Addr):
         raise TypeError("Can't subtract IP4Addr from IP6Addr")
      return IP6Addr(long.__sub__(self,a))

   def __and__(self, a):
      if isinstance(a, IP4Addr):
         raise TypeError("Can't do logical 'and' of IP6Addr and IP4Addr")
      return IP6Addr(long.__and__(self,a))

   def __or__(self, a):
      if isinstance(a, IP4Addr):
         raise TypeError("Can't do logical 'or' of IP6Addr and IP4Addr")
      return IP6Addr(long.__or__(self,a))


IPRangeParentType = _nt("IPRange",["start","end"])

class IPRange(IPRangeParentType):
   __slots__ = ()

   def __new__(cls, param1, param2=None):
      """
      Params may be:
         start, end  - first and last address of a range as IPAddr, string or number
         string      - string in "prefix" format, e.g. 1.2.3.0/24
      """
      if param2 is None:
         return cls.fromString(param1)
      else:
         return cls.fromPair(param1, param2)

   @classmethod
   def fromPair(cls, start, end):
      if isinstance(start, IP4Addr) or isinstance(start, IP6Addr):
         s = start
      else:
         s = IPAddr(start)
      if isinstance(end, IP4Addr) or isinstance(end, IP6Addr):
         e = end
      else:
         e = IPAddr(end)

      if isinstance(s, IP4Addr) and isinstance(e, IP4Addr):
         return IPRangeParentType.__new__(IP4Range, s, e)
      elif isinstance(s, IP6Addr) and isinstance(e, IP6Addr):
         return IPRangeParentType.__new__(IP6Range, s, e)
      else:
         raise TypeError("Both addresses must be of the same type (IPv4/IPv6).")

   @classmethod
   def fromString(cls, string):
      """Create IPRange from a string in "prefix" format, e.g. "1.2.3.0/24"."""
      base,prefix_len = string.split('/')
      base = IPAddr(base)
      prefix_len = int(prefix_len)
      if isinstance(base, IP4Addr):
         if not 0 <= prefix_len <= 32:
            raise ValueError("Length of IPv4 prefix must be between 0 and 32.")
         start = base &  (~long(0) << (32-prefix_len))
         end   = base | ~(~long(0) << (32-prefix_len))
      else:
         if not 0 <= prefix_len <= 128:
            raise ValueError("Length of IPv6 prefix must be between 0 and 128.")
         start = base &  (~long(0) << (128-prefix_len))
         end   = base | ~(~long(0) << (128-prefix_len))
      return cls.fromPair(start, end)


class IP4Range(IPRange):
   __slots__ = ()

   def __new__(cls, start, end):
      """Create new IP4Range instance. 'start' and 'end' must be instances of
      IP4Addr"""
      return IPRangeParentType.__new__(cls, start, end)

   def __repr__(self):
      return "IP4Range("+str(self.start)+", "+str(self.end)+")"

   __str__ = __repr__

   def __contains__(self, addr):
      if not isinstance(addr, IPAddr):
         addr = IPAddr(addr)
      if isinstance(addr, IP6Addr):
         return False
      else:
         return self.start <= addr <= self.end


class IP6Range(IPRange):
   __slots__ = ()

   def __new__(cls, start, end):
      """Create new IP6Range instance. 'start' and 'end' must be instances of
      IP6Addr"""
      return IPRangeParentType.__new__(cls, start, end)

   def __repr__(self):
      return "IP6Range("+str(self.start)+", "+str(self.end)+")"

   __str__ = __repr__

   def __contains__(self, addr):
      if not isinstance(addr, IPAddr):
         addr = IPAddr(addr)
      if isinstance(addr, IP4Addr):
         return False
      else:
         return self.start <= addr <= self.end



# If module is ran as a standalone script, run tests
# TODO tests for IPRange
if __name__ == '__main__':
   import unittest
   class ipaddrtest(unittest.TestCase):
      def test_ipv4(self):
         self.assertEqual(IP4Addr("0.0.0.0"), 0)
         self.assertEqual(IP4Addr("255.255.255.255"), 2**32-1)
         self.assertEqual(IP4Addr("1.2.3.4"), 16909060)
         self.assertEqual(str(IP4Addr("100.101.102.103")), "100.101.102.103")
         self.assertEqual(IP4Addr("\x00\x01\xff\xfe"), 131070)
         self.assertEqual(IP4Addr("ABCD"), IP4Addr("65.66.67.68"))
         self.assertEqual(IP4Addr("test").toBytes(), "test")
         self.assertEqual(IP4Addr(0), 0)
         self.assertEqual(IP4Addr(123456789), 123456789)
         self.assertEqual(IP4Addr("1.2.3.4")+IP4Addr("4.3.2.1") , IP4Addr("5.5.5.5"))
         self.assertEqual(IP4Addr("10.11.12.13")-13 , IP4Addr("10.11.12.0"))
         self.assertEqual(IP4Addr("10.11.12.13") & IP4Addr("255.255.0.0") , IP4Addr("10.11.0.0"))

         self.assertRaises(TypeError, IP4Addr, 0.5)
         self.assertRaises(ValueError, IP4Addr, "1.2.3")
         self.assertRaises(ValueError, IP4Addr, "1.2.3.4.5")
         self.assertRaises(ValueError, IP4Addr, "0")
         self.assertRaises(ValueError, IP4Addr, "0.0.0.256")
         self.assertRaises(ValueError, IP4Addr, "-1.0.1.2")
         self.assertRaises(ValueError, IP4Addr, "10.20.x.40")
         self.assertRaises(ValueError, IP4Addr, -1)
         self.assertRaises(ValueError, IP4Addr, 2**32)
         self.assertRaises(TypeError, IP4Addr(0).__add__, 0.5)
         self.assertRaises(TypeError, IP4Addr(0).__add__, "1")

      def test_ipv6(self):
         self.assertEqual(IP6Addr("::"), 0)
         self.assertEqual(IP6Addr("0::0"), 0)
         self.assertEqual(IP6Addr("::1"), 1)
         self.assertEqual(str(IP6Addr("0123:4567:89ab:cdef:0011:2233:4455:6677")), "0123:4567:89ab:cdef:0011:2233:4455:6677")
         self.assertEqual(str(IP6Addr("123:4567:89ab:cdef:11:2233:4455:6677")), "0123:4567:89ab:cdef:0011:2233:4455:6677")
         self.assertEqual(IP6Addr("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"), 2**128-1)
         self.assertEqual(IP6Addr("::ffff:ffff"), 2**32-1)
         self.assertEqual(str(IP6Addr("1111:22::abcd:f")), "1111:0022:0000:0000:0000:0000:abcd:000f")
         self.assertEqual(IP6Addr.fromBytes("\x00\x01\x02\x03\xff\xfe\xfd\xfc  \x00\x00ABCD"), IP6Addr("0001:0203:fffe:fdfc:2020:0000:4142:4344"))
         self.assertEqual(IP6Addr.fromBytes("\x00"*16), IP6Addr(0))
         self.assertEqual(IP6Addr("::").toBytes(), "\x00"*16)
         self.assertEqual(IP6Addr.fromBytes("* IPv6 address *").toBytes(), "* IPv6 address *")
         self.assertEqual(IP6Addr(0), 0)
         self.assertEqual(IP6Addr(1234567890123), 1234567890123)
         self.assertEqual(IP6Addr(2**128-1), 2**128-1)
         self.assertEqual(IP6Addr("ff::1")+IP6Addr("::1:1"), IP6Addr("ff::1:2"))
         self.assertEqual(IP6Addr("2001::ffff:0110")-16 , IP6Addr("2001::ffff:0100"))
         self.assertEqual(IP6Addr("abcd:1234:5678::1") & IP6Addr("ffff:ffff::") , IP6Addr("abcd:1234::"))
         self.assertEqual(IP6Addr("abcd:1234:5678::1111") & IP6Addr("::f") , IP6Addr("::1"))

         self.assertRaises(TypeError, IP6Addr, 0.5)
         self.assertRaises(ValueError, IP6Addr, "1:2:3:4")
         self.assertRaises(ValueError, IP6Addr, "0:0:0:0:1:2:3:4:0:0:0:0:1:2:3:4:0")
         self.assertRaises(ValueError, IP6Addr, "0000:1111::2222:3333::0000")
         self.assertRaises(ValueError, IP6Addr, "::1:1::")
         self.assertRaises(ValueError, IP6Addr, "::abcd:efgh")
         self.assertRaises(ValueError, IP6Addr, "::-1")
         self.assertRaises(ValueError, IP6Addr, "::0.5")
         self.assertRaises(ValueError, IP6Addr, -1)
         self.assertRaises(ValueError, IP6Addr, 2**128)
         self.assertRaises(TypeError, IP6Addr(0).__add__, 0.5)
         self.assertRaises(TypeError, IP6Addr(0).__add__, "1")
         # IPv6 can't be created from bytes using contructor (fromBytes must be used)
         # (it's because we can't say, if 16B string is a string notation or sequence of bytes)
         self.assertRaises(ValueError, IP6Addr, ("\x00"*16))

      def test_ipv4_ipv6_operations(self):
         # Mix IPv4 and IPv6 addresses shouldn't be possible
         self.assertRaises(TypeError, IP4Addr(0).__add__, IP6Addr(0))
         self.assertRaises(TypeError, IP4Addr(0).__sub__, IP6Addr(0))
         self.assertRaises(TypeError, IP4Addr(0).__and__, IP6Addr(0))
         self.assertRaises(TypeError, IP6Addr(0).__add__, IP4Addr(0))
         self.assertRaises(TypeError, IP6Addr(0).__sub__, IP4Addr(0))
         self.assertRaises(TypeError, IP6Addr(0).__and__, IP4Addr(0))

      def test_universal_constuctor(self):
         self.assertEqual(IPAddr("0.1.2.3"), IP4Addr("0.1.2.3"))
         self.assertEqual(IPAddr("1234::1:abcd"), IP6Addr("1234::1:abcd"))
         self.assertEqual(IPAddr.fromBytes("ABCD"), IP4Addr("ABCD"))
         self.assertEqual(IPAddr.fromBytes("this is 16 bytes"), IP6Addr.fromBytes("this is 16 bytes"))
         # Because it's not possible to determine if 16B string is string notation or bytes,
         # this should raise exception
         self.assertRaises(ValueError, IPAddr, "ABCD")
         self.assertRaises(ValueError, IPAddr, "this is 16 bytes")
         # It's not possible to derive IP version from a number
         self.assertRaises(TypeError, IPAddr, 0)
         self.assertRaises(TypeError, IPAddr, 123456789)

         self.assertRaises(TypeError, IPAddr, 0.5)
         self.assertRaises(TypeError, IPAddr, (1,2))

   print("IP Address module - self testing:")
   unittest.main()


