#!/usr/bin/python

import pdb
import sys
import os.path
sys.path.append(os.path.join(os.path.dirname(__file__), "..", "..", "python"))
import trap
import unirec
from optparse import OptionParser
parser = OptionParser()
parser.add_option("-f", "--file", dest="filename",
      help="write report to FILE", metavar="FILE")
parser.add_option("-q", "--quiet",
      action="store_false", dest="verbose", default=True,
      help="don't print status messages to stdout")


module_info = trap.CreateModuleInfo(
   "PythonExample", # Module name
   "An example TRAP module written in Python", # Description
   1, # Number of input interfaces
   1,  # Number of output interfaces
   parser
)

# Initialize module
ifc_spec = trap.parseParams(sys.argv, module_info)

trap.init(module_info, ifc_spec)

trap.registerDefaultSignalHandler() # This is needed to allow module termination using s SIGINT or SIGTERM signal

trap.lib.trap_set_required_fmt(0, trap.TRAP_FMT_UNIREC, "")
# Create class for incoming UniRec records

UR_Flow = None

# Main loop (trap.stop is set to True when SIGINT or SIGTERM is received)
while not trap.stop:
   # Read data from input interface
   try:
      data = trap.recv(0)
   except trap.EFMTChanged:
      (fmttype, fmtspec) = trap.trap_get_data_fmt(trap.IFC_INPUT, 0)
      UR_Flow = unirec.CreateTemplate("UR_Flow", fmtspec)
      print(fmtspec)
      continue
   except trap.ETerminated:
      break

   # Check for "end-of-stream" record
   if len(data) <= 1:
      # Send "end-of-stream" record and exit
      trap.send(0, "0")
      break

   # Convert data to UniRec and print it
   rec = UR_Flow(data)
   print rec

   assert(data == rec.serialize()) # Check that serialization works fine

   # Send data to output interface
   try:
      trap.send(0, rec.serialize())
   except trap.ETerminated:
      break


#trap.finalize() # Explicit call to finalize is not needed, it's done aumatically

