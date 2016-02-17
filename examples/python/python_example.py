#!/usr/bin/env python

import pdb
import sys
import os.path
sys.path.append(os.path.join(os.path.dirname(__file__), "..", "..", "python"))
sys.path.append(os.path.join(os.path.dirname(__file__), "..", "python"))
sys.path.append(os.path.join(os.path.dirname(__file__), ".."))
import trap
import unirec

# How to add options of module
from optparse import OptionParser
parser = OptionParser(add_help_option=False)
parser.add_option("-f", "--file", dest="filename",
      help="write report to FILE", metavar="FILE")
parser.add_option("-q", "--quiet",
      action="store_false", dest="verbose", default=True,
      help="don't print status messages to stdout")
# usage of OptionParser can be found in its help


module_info = trap.CreateModuleInfo(
   "PythonExample", # Module name
   "An example TRAP module written in Python", # Description
   1, # Number of input interfaces
   1,  # Number of output interfaces
   parser # use previously defined OptionParser
)

# Initialize module
ifc_spec = trap.parseParams(sys.argv, module_info)

trap.init(module_info, ifc_spec)

trap.registerDefaultSignalHandler() # This is needed to allow module termination using s SIGINT or SIGTERM signal

# Parse remaining command-line arguments
(options, args) = parser.parse_args()

# this module accepts all UniRec fieds -> set required format:
trap.set_required_fmt(0, trap.TRAP_FMT_UNIREC, "")

# Specifier of UniRec records will be received during libtrap negotiation
UR_Flow = None

# Main loop (trap.stop is set to True when SIGINT or SIGTERM is received)
while not trap.stop:
   # Read data from input interface
   try:
      data = trap.recv(0)
   except trap.EFMTMismatch:
      print("Error: output and input interfaces data format or data specifier mismatch")
      break
   except trap.EFMTChanged as e:
      # Get data format from negotiation
      (fmttype, fmtspec) = trap.get_data_fmt(trap.IFC_INPUT, 0)
      UR_Flow = unirec.CreateTemplate("UR_Flow", fmtspec)
      print("Negotiation:", fmttype, fmtspec)
      UR_Flow2 = unirec.CreateTemplate("UR_Flow2", fmtspec)

      # Set the same format for output IFC negotiation
      trap.set_data_fmt(0, fmttype, fmtspec)
      data = e.data
   except trap.ETerminated:
      break

   # Check for "end-of-stream" record
   if len(data) <= 1:
      # Send "end-of-stream" record and exit
      trap.send(0, "0")
      break

   # Convert data to UniRec and print it
   rec = UR_Flow(data)
   print(rec)

   assert(data == rec.serialize()) # Check that serialization works fine

   # Send data to output interface
   try:
      rec2 = UR_Flow2()
      rec2 = rec
      trap.send(0, rec2.serialize())
   except trap.ETerminated:
      break


#trap.finalize() # Explicit call to finalize is not needed, it's done aumatically

