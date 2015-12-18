# trap.py - python wrapper for TRAP library
# Author: Vaclav Bartos (ibartosv@fit.vutbr.cz), 2013
# Author: Tomas Cejka (cejkat@cesnet.cz), 2015

"""
TRAP library python wrapper

This module provides access to libtrap library.

"""

import ctypes
from ctypes import *
import signal
import os.path
from optparse import OptionParser
import sys

if sys.version_info > (3,):
   long = int
   str = str
   unicode = str
   bytes = bytes
   basestring = (str,bytes)
else:
   str = str
   unicode = unicode
   basestring = basestring
   def bytes(string, encoding):
       return str(string)

# ***** Load libtrap library *****

try:
    lib = CDLL(os.path.join(os.path.dirname(__file__), "..", "libtrap", "src", ".libs", "libtrap.so"))
    #print "Loaded libtrap from git repository"
except OSError:
    lib = CDLL("libtrap.so")
    #print "Loaded libtrap from system path"

# Load libc library (needed for "signal" function, see at the bottom of the file)
libc = CDLL("libc.so.6")  # note: if libtrap will ever work on Windows, here should be "msvcrt" then


# ***** Error handling and exceptions *****

# Definition of error codes
E_OK = 0 # Success, no error
E_TIMEOUT = 1 # Read or write operation timeout
E_INITIALIZED = 10 # TRAP library already initilized
E_BADPARAMS = 11 # Bad parameters passed to interface initializer
E_BAD_IFC_INDEX = 12 # Interface index out of range
E_BAD_FPARAMS = 13 # Bad parameters of function
E_IO_ERROR = 14 # IO Error
E_TERMINATED = 15 # Interface was terminated during reading/writing
E_NOT_SELECTED = 16 # Interface was not selected reading/writing
E_HELP = 20 # Returned by parse_parameters when help is requested

E_NOT_INITIALIZED = 254 # TRAP library not initilized
E_MEMORY = 255 # Memory allocation error

# Negotiation-related return values
TRAP_E_FIELDS_MISMATCH = 21 # Returned when receiver fields are not subset of sender fields
TRAP_E_FIELDS_SUBSET = 22 # Returned when receivers fields are subset of senders fields and both sets are not identical
TRAP_E_FORMAT_CHANGED = 23 # Returned by trap_recv when format or format spec of the receivers interface has been changed
TRAP_E_FORMAT_MISMATCH = 24 # Returned by trap_recv when data format or data specifier of the output and input interfaces doesn't match

# Input/output ifc (enum trap_ifc_type)
IFC_INPUT = 1
IFC_OUTPUT = 2

# Definition of data format types (trap_data_format_t)
TRAP_FMT_UNKNOWN = 0 # unknown - message format was not specified yet
TRAP_FMT_RAW = 1 # raw data, no format specified
TRAP_FMT_UNIREC = 2 # UniRec records
TRAP_FMT_JSON = 3 # structured data serialized using JSON

# Definition of input interface states (trap_in_ifc_state_t)
STATE_FMT_WAITING = 0
STATE_FMT_OK = 1
STATE_FMT_MISMATCH = 2
STATE_FMT_CHANGED = 3

# Definition of exceptions
class TRAPException (Exception):
   def __init__(self, code, msg = None):
      self.code = int(code)
      if (msg is None):
         self.msg = c_char_p.in_dll(lib,'trap_last_error_msg').value
      else:
         self.msg = msg
   def __str__(self):
      return "TRAP ERROR %i: %s" % (self.code, self.msg)
class ETimeout (TRAPException): pass
class EInitialized (TRAPException): pass
class ENotInitialized (TRAPException): pass
class ETerminated (TRAPException): pass
class EBadParams (TRAPException): pass
class EIOError (TRAPException): pass
class EHelp (TRAPException): pass
class EMemory (TRAPException): pass
class EFMTMismatch (TRAPException): pass
class EFMTChanged(TRAPException):
    def __init__(self, code, data = None):
        self.code = code
        self.data = data

# Error handling function
def errorCodeChecker(code):
   if code == E_OK:
      return E_OK
   elif code == E_TIMEOUT:
      raise ETimeout(code)
   elif code == E_IO_ERROR:
      raise EIOError(code)
   elif code == E_TERMINATED:
      raise ETerminated(code)
   elif code in [E_BADPARAMS, E_BAD_IFC_INDEX, E_BAD_FPARAMS, E_NOT_SELECTED]:
      raise EBadParams(code)
   elif code == E_INITIALIZED:
      raise EInitialized(code)
   elif code == E_NOT_INITIALIZED:
      raise ENotInitialized(code)
   elif code == E_MEMORY:
      raise EMemory(code)
   elif code in [TRAP_E_FIELDS_SUBSET, TRAP_E_FIELDS_MISMATCH, TRAP_E_FORMAT_CHANGED]:
      raise EFMTChanged(code)
   elif code == TRAP_E_FORMAT_MISMATCH:
      raise EFMTMismatch(code)
   else:
      raise TRAPException(code, "Unknown error")


# ***** Other constants *****

MASK_ALL = 0xffffffff

NO_WAIT = 0
WAIT = -1
HALFWAIT = -2

# Direction of IFC (trap_ifc_type)
IFC_INPUT = 1  # interface acts as source of data for module
IFC_OUTPUT = 2 # interface is used for sending data out of module

CTL_AUTOFLUSH_TIMEOUT = 1
CTL_BUFFERSWITCH = 2
CTL_SETTIMEOUT = 3

# ***** Structure definitions *****

class ModuleInfoParameter(Structure):
   _fields_ = [('short_opt', c_char),
               ('long_opt', c_char_p),
               ('description', c_char_p),
               ('param_required_argument', c_int),
               ('arg_type', c_char_p)
              ]
   def __str__(self):
      return "{}, {}, {}, {}, {}".format(self.short_opt, self.long_opt, self.description, self.param_required_argument, self.arg_type)

class ModuleInfo(Structure):
   _fields_ = [('name', c_char_p),
               ('description', c_char_p),
               ('num_ifc_in', c_int),
               ('num_ifc_out', c_int),
               ('params', c_void_p)
              ]

   def __str__(self):
      return """name: {}
description: {}
num_ifc_in: {}
num_ifc_out: {}
Parameters:
{}""".format(self.name,
             self.description,
             self.num_ifc_in,
             self.num_ifc_out,
             self.params)

class MultiResult(Structure):
   _fields_ = [('message_size', c_uint16),
               ('message', c_void_p),
               ('result_code', c_int),
              ]

class IfcSpec(Structure):
   _fields_ = [('types', c_char_p),
               ('params', POINTER(c_char_p)),
              ]

# Functions for creating structures

def CreateModuleInfo(name, description, num_ifc_in, num_ifc_out, opts = None):
   """Create module_info structure for libtrap.

   name - name of Nemea module
   description - module's description
   num_ifc_in - number of module's input IFCs (-1 for variable number)
   num_ifc_out - number of module's output IFCs (-1 for variable number)
   opts - python optparse.OptionParser for specification of module's parameters"""

   # put all options into the list
   params = []
   if opts and isinstance(opts, OptionParser):
     for o in opts.option_list:
        arg_type = None if o.nargs == 0 else "string"
        arg_req = 1 if o.nargs and o.nargs >= 1 else 0
        shortopt = c_char(bytes(o._short_opts[0][-1],'ascii'))
        longopt = bytes(o._long_opts[0][2:],'ascii')
        m = ModuleInfoParameter(shortopt, longopt, bytes(o.help,'ascii'), arg_req, bytes(arg_type,'ascii'))
        params.append(m)

   moduleInfo = lib.trap_create_module_info(bytes(name,'ascii'), bytes(description, 'ascii'), num_ifc_in, num_ifc_out, len(params))
   pn = 0
   for p in params:
      lib.trap_update_module_param(moduleInfo, pn, p.short_opt, p.long_opt, p.description, p.param_required_argument, p.arg_type)
      pn = pn + 1
   return moduleInfo

def CreateIfcSpec(types, params):
   """Create ifc_spec structure.

   types - string containing list of IFC types
   params - list of IFC parameters"""
   return IfcSpec(types, (c_char_p*len(params))(*params))


# **** Functions *****

# Set function parameters
lib.trap_init.argtypes = (POINTER(ModuleInfo), IfcSpec)
lib.trap_init.restype = errorCodeChecker
lib.trap_terminate.argtypes = None
lib.trap_terminate.restype = errorCodeChecker
lib.trap_finalize.argtypes = None
lib.trap_finalize.restype = errorCodeChecker
lib.trap_get_data.argtypes = (c_uint32, POINTER(c_void_p), POINTER(c_uint16), c_int)
lib.trap_get_data.restype = errorCodeChecker
lib.trap_send_data.argtypes = (c_uint, c_void_p, c_uint16, c_int)
lib.trap_send_data.restype = errorCodeChecker
lib.trap_recv.argtypes = (c_uint32, POINTER(c_void_p), POINTER(c_uint16))
lib.trap_recv.restype = errorCodeChecker
lib.trap_send.argtypes = (c_uint32, c_void_p, c_uint16)
lib.trap_send.restype = errorCodeChecker
lib.trap_send_flush.argtypes = (c_int,)
lib.trap_send_flush.restype = errorCodeChecker
lib.trap_set_verbose_level.argtypes = (c_int,)
lib.trap_set_verbose_level.restype = None
lib.trap_get_verbose_level.argtypes = None
lib.trap_get_verbose_level.restype = c_int
lib.trap_print_help.argtypes = (POINTER(ModuleInfo),)
lib.trap_print_help.restype = None
lib.trap_print_ifc_spec_help.argtypes = None
lib.trap_print_ifc_spec_help.restype = None
lib.trap_ifcctl.argtypes = (c_uint, c_uint, c_uint) # in fact, some of c_units are enums, but they are probably implemented as uint
lib.trap_ifcctl.restype = errorCodeChecker

lib.trap_create_module_info.argtypes = (c_char_p, c_char_p, c_int, c_int, c_uint16)
lib.trap_create_module_info.restype = POINTER(ModuleInfo)
lib.trap_update_module_param.argtypes = (POINTER(ModuleInfo), c_uint16, c_char, c_char_p, c_char_p, c_int, c_char_p);
lib.trap_update_module_param.restype = c_int
lib.trap_set_help_section.argtypes = (c_int,)
lib.trap_set_help_section.restype = None

# Data format handling functions
lib.trap_set_data_fmt.argtypes = (c_uint32, c_ubyte, c_char_p)
lib.trap_set_data_fmt.restype = None
lib.trap_set_required_fmt.argtypes = (c_uint32, c_ubyte, c_char_p)
lib.trap_set_required_fmt.restype = None
lib.trap_get_data_fmt.argtypes = (c_ubyte, c_uint32, POINTER(c_ubyte), POINTER(c_char_p))
lib.trap_get_data_fmt.restype = errorCodeChecker
lib.trap_get_in_ifc_state.argtypes = (c_uint32,)
lib.trap_get_in_ifc_state.restype = c_int # check for errors is made specially later


def init(module_info, ifc_spec):
   """Initialize libtrap.

   module_info - info about module, structure created by CreateModuleInfo()
   ifc_spec - info about IFCs, structure created by parseParams() that analyzes process arguments
   """
   lib.trap_init(module_info, ifc_spec)

terminate = lib.trap_terminate
finalize = lib.trap_finalize

def getData(ifc_mask, timeout):
   """Receive and return a message.

   ifc_mask - selection of IFC
   timeout - libtrap timeout
   Obsoleted function!
   """
   data_ptr = c_void_p()
   data_size = c_uint16()
   lib.trap_get_data(ifc_mask, byref(data_ptr), byref(data_size), timeout)
   data = string_at(data_ptr, data_size.value)
   return data

def recv(ifc):
   """Receive and return a message.

   ifc - input IFC index
   Recommended function.
   """
   data_ptr = c_void_p()
   data_size = c_uint16()
   try:
      lib.trap_recv(ifc, byref(data_ptr), byref(data_size))
   except EFMTChanged as e:
      data = string_at(data_ptr, data_size.value)
      raise EFMTChanged(e.code, data)

   data = string_at(data_ptr, data_size.value)
   return data

def sendData(ifc, data, timeout):
   """Send a message.

   ifc - index of IFC
   data - message to send (array of bytes, casted to c_char_p)
   timeout - libtrap timeout
   Obsoleted function!
   """
   data_ptr = c_char_p(data)
   data_size = c_uint16(len(data))
   lib.trap_send_data(ifc, data_ptr, data_size, timeout)

def send(ifc, data):
   """Send a message.

   ifc - output IFC index
   data - array of bytes (string); data argument is casted to c_char_p and passed into libtrap
   Recommended function.
   """
   data_ptr = c_char_p(data)
   data_size = c_uint16(len(data))
   lib.trap_send(ifc, data_ptr, data_size)

sendFlush = lib.trap_send_flush

setVerboseLevel = lib.trap_set_verbose_level
getVerboseLevel = lib.trap_get_verbose_level

printHelp = lib.trap_print_help
printIfcSpecHelp = lib.trap_print_ifc_spec_help

def ifcctl(type, ifcidx, request, *args):
   """Set libtrap IFC's option.

   type - IFC_INPUT or IFC_OUTPUT
   ifcidx - index of IFC
   request - type of option to set (see libtrap doc trap_ifcctl())
   args - value to set (see libtrap doc trap_ifcctl())
   """
   if type not in [IFC_INPUT, IFC_OUTPUT]:
      raise ValueError("ifcctl: type must be IFC_INPUT or IFC_OUTPUT")
   if request == CTL_AUTOFLUSH_TIMEOUT:
      param = c_uint64(*args)
      lib.trap_ifcctl(type, ifcidx, request, param)
   elif request == CTL_BUFFERSWITCH:
      param = c_uint8(*args)
      lib.trap_ifcctl(type, ifcidx, request, param)
   elif request == CTL_SETTIMEOUT:
      param = c_uint32(*args)
      lib.trap_ifcctl(type, ifcidx, request, param)
   else:
      raise ValueError("ifcctl: request must one of: [CTL_AUTOFLUSH_TIMEOUT, CTL_BUFFERSWITCH, CTL_SETTIMEOUT]")


def parseParams(argv, module_info=None):
   """
   Parse process arguments argv and prepare IFC specification.

   If -h or --help is passed, raise EHelp to print libtrap's help.

   argv - process argument
   module_info - structure of libtrap's module_info
   """
   if "-h" in argv or "--help" in argv:
      if module_info is not None:
         try:
            index = argv.index("-h")
         except IndexError:
            index = argv.index("--help")
         try:
            help_arg = argv[index + 1]
            if help_arg in ["1", "trap"]:
               lib.trap_set_help_section(1)
            else:
               lib.trap_set_help_section(0)
         except:
            pass
         printHelp(module_info)
         exit(0)
      else:
         raise EHelp(E_HELP, "Help was requested but it cannot be shown. Author of the module should pass a module_info into the parseParams() or catch this exception and call printHelp().")

   if module_info is not None:
      module_info = module_info.contents
      try:
         index = argv.index("-i")
         ifc_spec = argv[index+1]
         del argv[index+1]
         del argv[index]
         del index
      except (ValueError, IndexError):
         raise EBadParams(E_BADPARAMS, "Interface specifier (option -i) not found.")

      if "-v" in argv:
         setVerboseLevel(0)
         argv.remove("-v")
      if "-vv" in argv:
         setVerboseLevel(1)
         argv.remove("-vv")
      if "-vvv" in argv:
         setVerboseLevel(2)
         argv.remove("-vvv")

      ifc_types = b""
      ifc_params= []
      try:
         ifcs = ifc_spec.split(',')

         if len(ifcs) < (max(module_info.num_ifc_in, 0) + max(module_info.num_ifc_out, 0)):
            raise EBadParams(E_BADPARAMS, "Number of IFC parameters doesn't match number of IFCs in module_info.")
         for i in ifcs:
            (ifcType, ifcParams) = (bytes(x,'ascii') for x in i.split(":", 1))
            ifc_types = ifc_types + ifcType
            ifc_params.append(ifcParams)

      except ValueError:
         raise EBadParams(E_BADPARAMS, "Wrong format of interface specifier.")
      if len(ifc_types) != len(ifc_params):
         raise EBadParams(E_BADPARAMS, "Wrong format of interface specifier.")

   return CreateIfcSpec(ifc_types, ifc_params)

def freeIfcSpec(ifc_spec):
   raise NotImplementedError("Wrapper for trap_free_ifc_spec is not implemented.")

def get_data_fmt(direction, ifcidx):
   """Get (fmttype, fmtspec) from IFC of direction and ifcidx.

   direction - IFC_INPUT | IFC_OUTPUT
   ifcidx - index of IFC
   Returns tuple of format type (TRAP_FMT_RAW, TRAP_FMT_UNIREC, TRAP_FMT_JSON) and format specifier."""
   fmttype = c_ubyte()
   fmtspec = c_char_p()
   lib.trap_get_data_fmt(direction, ifcidx, byref(fmttype), byref(fmtspec))
   return (fmttype.value, string_at(fmtspec) if fmtspec else None)

def set_required_fmt(ifcidx, fmttype, fmtspec):
   """Set required data format type and specifier of the input IFC (for negotiation).

   ifcidx - index of output IFC
   fmttype - format data type (TRAP_FMT_RAW, TRAP_FMT_UNIREC, TRAP_FMT_JSON)
   fmtspec - specifier of format type (e.g. "ipaddr DST_IP,ipaddr SRC_IP,uint16 DST_PORT,uint16 SRC_PORT" for TRAP_FMT_UNIREC)"""
   lib.trap_set_required_fmt(ifcidx, fmttype, bytes(fmtspec,'ascii'))

def set_data_fmt(ifcidx, fmttype, fmtspec):
   """Set data format type and specifier of the output IFC (for negotiation).

   ifcidx - index of output IFC
   fmttype - format data type (TRAP_FMT_RAW, TRAP_FMT_UNIREC, TRAP_FMT_JSON)
   fmtspec - specifier of format type (e.g. "ipaddr DST_IP,ipaddr SRC_IP,uint16 DST_PORT,uint16 SRC_PORT" for TRAP_FMT_UNIREC)"""
   lib.trap_set_data_fmt(ifcidx, fmttype, fmtspec)


def get_in_ifc_state(ifcidx):
   """Returns current state of an input interface on specified index."""
   # trap_get_in_ifc_state returns either the state or an error code
   ret = lib.trap_get_in_ifc_state
   if ret in [STATE_FMT_WAITING, STATE_FMT_OK, STATE_FMT_MISMATCH, STATE_FMT_CHANGED]:
      return ret
   return errorCodeChecker(ret)

# ***** Set up automatic cleanup when the interpreter exits *****

def autofinalize():
   try:
      finalize()
   except ENotInitialized:
      pass

import atexit
atexit.register(autofinalize)

# ***** Set signal handler *****


# We are hiding SIGINT from the interpreter but there is no other way to make
# this work. The problem is as follows:
# * When the Python interpreter recieves a signal, it plans to handle it after
#   the next interpreter "tick" (something like Python instruction). In the case
#   of library function call, the next tick can't happen until the function
#   returns.
# * TRAP library uses blocking calls to "select". These calls are interrupted by
#   any signal, but unless the interface is interrupted, the "select" is called
#   again immediately.
# * Therefore Python signal handlig can never start (if the signal arrives
#   during a blocking read/write operation in the library) so we need to
#   terminate the interafces right in the "native" signal handler.
# * Therefore we need to register a handler function using libc function instead
#   a Python function from "signal" module.


stop = False

def py_signal_handler(signum):
   global stop
   #print "Signal", signum, "received."
   stop = True
   try:
      terminate()
   except ENotInitialized:
      pass

SIGNAL_HANDLER_FUNC = CFUNCTYPE(None, c_int)
signal_handler = SIGNAL_HANDLER_FUNC(py_signal_handler)


def registerDefaultSignalHandler():
   global signal_handler
   libc.signal(2, signal_handler) # SIGINT
   libc.signal(15, signal_handler) # SIGTERM

