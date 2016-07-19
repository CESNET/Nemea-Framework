Format of TRAP interface specifier (IFC_SPEC):
==============================================

TRAP interface specifier is an argument of `-i` option used by every NEMEA module.
It specifies a configuration of module's TRAP interfaces (IFC), i.e. what kind of IFC to use and where to connect.

Configurations of individual IFCs are separated by comma `,`, e.g. `<IFC 1>,<IFC 2>,...,<IFC N>`.
Input IFCs must be specified first, output IFCs follow.
The number of input and output IFCs depends on the specific module (you should be able to find it in the module's help or README).

Parameters of each IFC are separated by colon `:`, e.g. `<type>:<par1>:<par2>:...:<parN>`.
The first parameter is always one character specifying the type of the IFC to use, other parameters differ for individual types (see below).

Example of startup of a module with 1 input and 1 output IFC:
```
traffic_repeater -i t:example.org:1234,u:my_socket
```
The example makes the repeater to use a TCP socket as its input IFC and connect it to 'example.org', port 1234; and to create an UNIX domain socket identified as 'my_socket' as its output IFC.

Supported interface types:
==========================

TCP interface ('t')
-------------------

Communicates through a TCP socket. Output interface listens on a given port,
input interface connects to it. There may be more than one input interfaces
connected to one output interface, every input interface will get the same data.

Parameters when used as INPUT interface:
```
<hostname or ip>:<port>
```

Parameters when used as OUTPUT interface:
```
<port>:<max_num_of_clients>
```
Maximal number of connected clients (input interfaces) is optional (64 by default).

UNIX domain socket ('u')
------------------------

Communicates through a UNIX socket. Output interface creates a socket and listens,
input interface connects to it. There may be more than one input interfaces
connected to one output interface, every input interface will get the same data.

Parameters when used as INPUT interface:
```
<socket_name>
```
Socket name can be any string usable as a file name.

Parameters when used as OUTPUT interface:
```
<socket_name>:<max_num_of_clients>
```
Socket name can be any string usable as a file name.
Maximal number of connected clients (input interfaces) is optional (64 by default).


Blackhole interface ('b')
-------------------------

Can be used as OUTPUT interface only. Does nothing, everything sent
to this interface is dropped. It has no parameters.


File interface ('f')
--------------------

Input interface reads data from given file, output interface stores data to a given file.
Parameters when used as INPUT interface:
```
<file_name>
```

Parameters when used as OUTPUT interface:
```
<file_name>:<mode>
```
Mode is optional. There are two possible modes: `a` - append (default), `w` - write.
Mode 'append' writes data at the end of the specified file, mode write overwrites the specified file.


Common IFC parameters
=====================

The following parameters can be used with any type of IFC.
There are parameters of libtrap IFC that are normally let default or set in source codes of a module. It is possible to override them by user via IFC_SPEC. The available parameters are:
* timeout - time in microseconds that an IFC can block waiting for message send / receive
   * possible values: number of microseconds or one of the special values:
     * "WAIT" - block indefinitely
     * "NO_WAIT" - don't block 
     * "HALF_WAIT" (output IFC only) - block only if some client (input IFC) is connected
   * default: WAIT
* buffer (OUTPUT only) - buffering of data and sending in larger bulks (increases throughput)
   * possible values: on, off
   * default: on
* autoflush - normally data are not sent until the buffer is full. When autoflush is enabled, even non-full buffers are sent every X microseconds.
   * possible values: off, number of microseconds
   * default: 500000 (0.5s)

Example: `-i u:inputsocket:timeout=WAIT,u:outputsocket:timeout=500000:buffer=off:autoflush=off`


More examples:
==============

my_module with 1 input interface and 2 output interfaces:
```
./my_module -i "t:localhost:12345,b:,t:23456:5"
```
The input interface will connect to localhost:12345 (TCP). The first output
interface is unused (all data send there will be dropped), the second output
interface will provide data on TCP port 23456, to which another module can connect
its input interface.

nfdump_reader module that reads nfdump file and drops records via Blackhole IFC type:
```
./modules/nfreader/nfdump_reader -i b: ./file.nfdump
```
