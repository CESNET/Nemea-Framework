#!/usr/bin/env python

import pytrap
import sys

trap = pytrap.TrapCtx()
trap.init(sys.argv, 1, 1)

# Set the list of required fields in received messages.
# This list is an output of e.g. flow_meter - basic flow.
inputspec = "ipaddr DST_IP,ipaddr SRC_IP,uint64 BYTES,uint64 LINK_BIT_FIELD,time TIME_FIRST,time TIME_LAST,uint32 PACKETS,uint16 DST_PORT,uint16 SRC_PORT,uint8 DIR_BIT_FIELD,uint8 PROTOCOL,uint8 TCP_FLAGS,uint8 TOS,uint8 TTL"
trap.setRequiredFmt(0, pytrap.FMT_UNIREC, inputspec)
rec = pytrap.UnirecTemplate(inputspec)

# Define a template of alert (can be extended by any other field)
alertspec = "ipaddr SRC_IP,ipaddr DST_IP,uint16 DST_PORT,uint16 SRC_PORT,time EVENT_TIME"
alert = pytrap.UnirecTemplate(alertspec)
# set the data format to the output IFC
trap.setDataFmt(0, pytrap.FMT_UNIREC, alertspec)

# Allocate memory for the alert, we do not have any variable fields
# so no argument is needed.
alert.createMessage()

def do_detection(rec):
    global alert, c
    # TODO write your algorithm into this method

    # TODO and send an alert
    if rec.DST_PORT == 6666 or rec.SRC_IP == pytrap.UnirecIPAddr("192.168.1.1"):
        print(rec.strRecord())
        # fill-in alert:
        alert.SRC_IP = rec.SRC_IP
        alert.DST_IP = rec.DST_IP
        alert.SRC_PORT = rec.SRC_PORT
        alert.DST_PORT = rec.DST_PORT
        alert.EVENT_TIME = rec.TIME_FIRST
        # send alert
        trap.send(alert.getData(), 0)

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

    do_detection(rec)

# Free allocated TRAP IFCs
trap.finalize()

