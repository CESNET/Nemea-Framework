# ur_time.py - Class for handling timestamps in UniRec format
# Author: Vaclav Bartos (ibartosv@fit.vutbr.cz), 2013

import struct
from datetime import datetime
import time
import calendar
import re
import sys

if sys.version_info > (3,):
    long = int

MSEC_TO_FRAC = 0b01000001100010010011011101001011110001101010011111101111  # 2**32 / 1000 in fixed-point
FRAC_TO_MSEC = 0b1111101001  # 1000 / 2**32 in fixed-point

__all__ = ("Timestamp",)


class Timestamp(long):
    "Class for handling timestamps in UniRec format (32b.32b fixed-point integer)"

    @classmethod
    def fromSec(cls, sec):
        """
        Create instance of Timestamp from a number of seconds since the epoch.
        The number of seconds may be integer or float.
        """
        if isinstance(sec, float):
            return cls.fromSecMsec(int(sec), int((sec-int(sec))*1000))
        else:
            return long.__new__(cls, sec << 32)

    @classmethod
    def fromSecMsec(cls, sec, msec):
        """
        Create instance of Timestamp from a number of seconds and miliseconds since the epoch.
        Both sec and msec should be integers.
        """
        return long.__new__(cls, (sec << 32) | ((msec * MSEC_TO_FRAC) >> 32))

    @classmethod
    def fromUniRec(cls, bytes):
        "Create instance of Timestamp from a time in UniRec format (64bit integer as string of bytes)."
        return long.__new__(cls, struct.unpack("=Q", bytes)[0])

    @classmethod
    def fromString(cls, string):
        """
        Create instance of Timestamp from a string in one of these formats:
            %Y-%m-%dT%H:%M:%S.%f
            %Y-%m-%dT%H:%M:%S
            %Y-%m-%dT%H:%M
            %Y%m%d%H%M%S.%f
            %Y%m%d%H%M%S
            %Y%m%d%H%M
        """
        match = re.match(r'^\s*(\d{4})-?(\d\d)-?(\d\d)[T ]?(\d\d):?(\d\d):?(\d\d)?(\.\d+)?\s*Z?\s*$', string)
        if not match:
            if string.lower() == "now":
                return cls.now()
            raise ValueError("Can't parse given string.")
        dt = datetime(*map(int, match.groups(0)[:-1]))
        sec = calendar.timegm(dt.utctimetuple())
        msec = 0
        if match.group(7) is not None:
            msec = long(float(match.group(7)) * 1000)
        val = (long(sec) << 32) | ((long(msec) * MSEC_TO_FRAC) >> 32)
        return long.__new__(cls, val)

    @classmethod
    def now(cls):
        "Create instance of Timestamp containing current time."
        return cls.fromSec(time.time())

    def toUniRec(self):
        "Return time in UniRec format (64bit integer as string of bytes)."
        return struct.pack("=Q", self)

    def __repr__(self):
        return "Time("+long.__repr__(self)+")"

    def __str__(self):
        "Return time as a string in ISO 8601 format."
        sec = self >> 32
        msec = ((self & 0xffffffff) * FRAC_TO_MSEC) >> 32
        return datetime.utcfromtimestamp(sec).strftime("%Y-%m-%dT%H:%M:%S") + ".%03d" % msec + "Z"

    def toString(self, format="%Y-%m-%dT%H:%M:%S"):
        """Return time as a string in given format (see datetime.strftime for format specification).
        TODO: Miliseconds are not supported yet."""
        sec = self >> 32
        return datetime.utcfromtimestamp(sec).strftime(format)

    def __float__(self):
        "Return time as a float number of seconds since the Epoch."
        return float(self >> 32) + float((self & 0xffffffff) * FRAC_TO_MSEC)/1000  # nefunguje

    def __int__(self):
        "Return time as an integer number of seconds since the Epoch (fractional part truncated)."
        return (self >> 32)

    def getSec(self):
        return self >> 32

    def setSec(self, value):
        self = (value << 32) | (self & 0xffffffff)

    def getMsec(self):
        return (self & 0xffffffff) * 1000/2**32  # FRAC_TO_MSEC     # TODO neni presne, s FRAC_TO_MSEC to nefunguje vubec

    def setMsec(self, value):
        self = (self & ~0xffffffff) | ((value * MSEC_TO_FRAC) >> 32)

    # set funkce nefunguji (nic nezmeni)

    sec = property(getSec, setSec)
    msec = property(getMsec, setMsec)

    def __add__(self, x):
        """Add x to Timestamp. If x is Timestamp, simply sum them up, otherwise
        suppose x is number of seconds sicne epoch (as int or float)."""
        if isinstance(x, Timestamp):
            return Timestamp(long(self) + long(x))
        elif isinstance(x, float):
            sec, msec = self.sec, self.msec
            sec += int(x)
            msec += int((x-int(x)+0.0005)*1000)
            if msec > 1000:
                msec -= 1000
                sec += 1
            return Timestamp.fromSecMsec(sec, msec)
        else:
            sec, msec = self.sec, self.msec
            return Timestamp.fromSecMsec(sec + x, msec)

    def __sub__(self, x):
        """Subtract x from Timestamp. If x is Timestamp, simply return their difference,
        otherwise suppose x is number of seconds sicne epoch (as int or float)."""
        if isinstance(x, Timestamp):
            return Timestamp(long(self) - long(x))
        elif isinstance(x, float):
            sec, msec = self.sec, self.msec
            sec -= int(x)
            msec -= int((x-int(x)+0.0005)*1000)
            if msec < 0:
                msec += 1000
                sec -= 1
            return Timestamp.fromSecMsec(sec, msec)
        else:
            sec, msec = self.sec, self.msec
            return Timestamp.fromSecMsec(sec - x, msec)

    # ADD a SUB funguje spravne!

    def __lt__(self, x):
        if isinstance(x, Timestamp):
            return long(self) < long(x)
        elif isinstance(x, float):
            return (self.sec < int(x) or (self.sec == int(x) and self.msec < int((x-int(x)+0.0005)*1000)))
        else:
            return (self.sec < x)

    def __le__(self, x):
        if isinstance(x, Timestamp):
            return long(self) <= long(x)
        elif isinstance(x, float):
            return (self.sec < int(x) or (self.sec == int(x) and self.msec <= int((x-int(x)+0.0005)*1000)))
        else:
            return (self.sec < x or (self.sec == x and self.msec == 0))

    def __gt__(self, x):
        return not self.__le__(x)

    def __ge__(self, x):
        return not self.__lt__(x)

    # LE, LT i GE, GT s intem funguji OK (i kdyz je msec != 0)

    # porovnavani s floatem je nejake divne
