# trap.py - python wrapper for TRAP library
# Author: Vaclav Bartos (ibartosv@fit.vutbr.cz), 2013

"""
TRAP library python wrapper

This module provides access to libtrap library.

TODO: Help strings of functions.
"""

from ctypes import *
import signal

# ***** Load libtrap library *****

lib = CDLL("libtrap.so")

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
   else:
      raise TRAPException(-1, "Unknown error")


# ***** Other constants *****

MASK_ALL = 0xffffffff

NO_WAIT = 0
WAIT = -1
HALFWAIT = -2

IFC_INPUT = 1
IFC_OUTPUT = 2

CTL_AUTOFLUSH_TIMEOUT = 1
CTL_BUFFERSWITCH = 2
CTL_SETTIMEOUT = 3

# ***** Structure definitions *****

class ModuleInfo(Structure):
   _fields_ = [('name', c_char_p),
               ('description', c_char_p),
               ('num_ifc_in', c_uint),
               ('num_ifc_out', c_uint),
              ]

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

def CreateModuleInfo(name, description, num_ifc_in, num_ifc_out):
   return ModuleInfo(name, description, num_ifc_in, num_ifc_out)

def CreateIfcSpec(types, params):
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


def init(module_info, ifc_spec):
   lib.trap_init(byref(module_info), ifc_spec)

terminate = lib.trap_terminate
finalize = lib.trap_finalize

def getData(ifc_mask, timeout):
   data_ptr = c_void_p()
   data_size = c_uint16()
   lib.trap_get_data(ifc_mask, byref(data_ptr), byref(data_size), timeout)
   data = string_at(data_ptr, data_size.value)
   return data

def recv(ifc):
   data_ptr = c_void_p()
   data_size = c_uint16()
   lib.trap_recv(ifc, byref(data_ptr), byref(data_size))
   data = string_at(data_ptr, data_size.value)
   return data

def sendData(ifc, data, timeout):
   data_ptr = c_char_p(data)
   data_size = c_uint16(len(data))
   lib.trap_send_data(ifc, data_ptr, data_size, timeout)

def send(ifc, data):
   data_ptr = c_char_p(data)
   data_size = c_uint16(len(data))
   lib.trap_send(ifc, data_ptr, data_size)

setVerboseLevel = lib.trap_set_verbose_level
getVerboseLevel = lib.trap_get_verbose_level

printHelp = lib.trap_print_help
printIfcSpecHelp = lib.trap_print_ifc_spec_help

def ifcctl(type, ifcidx, request, *args):
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
   if "-h" in argv or "--help" in argv:
      if module_info is not None:
         printHelp(module_info)
         exit(0)
      else:
         raise EHelp(E_HELP, "Help was requested but it cannot be shown. Author of the module should pass a module_info into the parseParams() or catch this exception and call printHelp().")

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

   try:
      ifc_types, ifc_params = ifc_spec.split(';', 1)
      ifc_params = ifc_params.split(';')
   except ValueError:
      raise EBadParams(E_BADPARAMS, "Wrong format of interface specifier.")
   if len(ifc_types) != len(ifc_params):
      raise EBadParams(E_BADPARAMS, "Wrong format of interface specifier.")

   return CreateIfcSpec(ifc_types, ifc_params)

def freeIfcSpec(ifc_spec):
   raise NotImplementedError("Wrapper for trap_free_ifc_spec is not implemented.")


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
   #print "Singal", signum, "received."
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

