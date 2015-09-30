Format of TRAP specifier (IFC_SPEC):
====================================

TRAP specifier is an argument of `-i` option of every module.
It specifies configuration of libtrap interfaces (IFC).
IFCs are separated by comma `,` e.g. `<IFC 1>,<IFC 2>,...,<IFC N>`.

Parameters of each ifc are separated by `:`, e.g. `<type>:<par1>:<par2>:...:<parN>`.

Input IFCs must be specified at first, output IFCs follow.
The order of IFCs are depended on the specific module.

Example of module startup with 1 input and 1 output IFC:
```
traffic_repeater -i t:server:1234,u:my_socket
```
The example makes the repeater to connect to server via 1234 port using
TCP, the repeater listens on UNIX socket with my_socket as identifier.

Supported interface types:
==========================

TCP interface ('t')
-------------------

Communicates through a TCP socket. Output interface listens on a given port,
input interface connects to it. There may be more than one input interfaces
connected to one output interface, every input interface will get the same data.
Parameters when used as INPUT interface:
```
<hostname or ip>,<port>
```
Hostname/IP and port to connect to must be specified.

Parameters when used as OUTPUT interface:
```
<port>,<max_num_of_clients>
```
Port to listen on and maximal number of clients (input interfaces) allowed
must be specified.

UNIX socket ('u')
-----------------

Communicates through a UNIX socket. Output interface listens on a given port,
input interface connects to it. There may be more than one input interfaces
connected to one output interface, every input interface will get the same data.
Parameters are the same as for TCP interface ('t').

Blackhole interface ('b')
-------------------------

Can be used as OUTPUT interface only. Does nothing, everything which is sent
by this interface is dropped. It has no parameters.


File interface ('f')
--------------------

Input interface reads data from given file, output interface stores data to a given file.
Parameters when used as INPUT interface:
```
<file_name>
```
Name of file (path to the file) must be specified.

Parameters when used as OUTPUT interface:
```
<file_name>:<mode>
```
Name of file (path to the file) must be specified.
Mode is optional. There are two types of mode: `a` - append, `w` - write,
mode append is set as default, if no mode is specified.
Mode append writes data at the end of specified file, mode write overwrites specified file.

Setters of IFC parameters
=========================

There are parameters of libtrap IFC that are normally set in source codes of a module. It is possible to change the parameters via IFC_SPEC by user. The available parameters are:
* timeout - time in microseconds that can IFC block waiting for message send / receive
   * possible values: number of microseconds or special constants WAIT, NO_WAIT (input IFC), WAIT, NO_WAIT, HALF_WAIT (output IFC)
* buffer - buffering of IFC increases throughput, it is enabled by default
   * possible values: on, off
* autoflush - libtrap contains a special thread for automatic sending of non-full buffers in given timeout.
   * possible values: off, number of microseconds

Example: `-i u:inputsocket:timeout=WAIT,u:outputsocket:timeout=500000:buffer=off:autoflush=off`


More examples:
==============

my_module with 1 input interface and 2 output interfaces:
```
./my_module -i "t:localhost:12345,b:,t:23456:5"
```
The input interface will connect to localhost:12345. The first output
interface is unused (all data send there will be dropped), the second output
interface will provide data on port 23456, to which another module can connect
its input interface.

nfdump_reader module that reads nfdump file and drops records via Blackhole IFC type:
```
./modules/nfreader/nfdump_reader -i b ./file.nfdump
```
