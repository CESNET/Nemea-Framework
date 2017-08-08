#!/usr/bin/env python
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import pytrap
import sys

trap = pytrap.TrapCtx()
trap.init(sys.argv, 1, 0)

# Set the list of required fields in received messages.
# This list is an output of e.g. flow_meter - basic flow.
inputspec = "ipaddr DST_IP,ipaddr SRC_IP,uint64 BYTES,uint64 LINK_BIT_FIELD,time TIME_FIRST,time TIME_LAST,uint32 PACKETS,uint16 DST_PORT,uint16 SRC_PORT,uint8 DIR_BIT_FIELD,uint8 PROTOCOL,uint8 TCP_FLAGS,uint8 TOS,uint8 TTL"
trap.setRequiredFmt(0, pytrap.FMT_UNIREC, inputspec)
rec = pytrap.UnirecTemplate(inputspec)

import time

start = time.time()
duration = 5*60

protoDict = {}
it = 0

# Main loop
while True:
    try:
        data = trap.recv()
    except pytrap.FormatChanged as e:
        fmttype, inputspec = trap.getDataFmt(0)
        rec = pytrap.UnirecTemplate(inputspec)
        data = e.data
    if len(data) <= 1:
        break
    rec.setData(data)
    proto = rec.PROTOCOL
    if proto in protoDict:
        protoDict[proto] += rec.PACKETS
    else:
        protoDict[proto] = rec.PACKETS
    
    if it == 1000:
        if time.time() - start >= duration:
            break;
        it = 0
    else:
        it += 1

# Free allocated TRAP IFCs
trap.finalize()

import operator
s = sorted(protoDict.items(), key=operator.itemgetter(1))
for proto, val in s:
    print("%s %s" % (proto, val))
 

