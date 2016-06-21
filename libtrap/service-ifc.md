SERVICE INTERFACE
=================

Description:
------------

Service interface is used for getting module's statistics per interface (e.g. counters).
Every interface has the following counters:

- input interface: number of received messages, number of received buffers
- output interface: number of sent messages, number of sent buffers, number of dropped messages, number of auto-flushes

These stats are periodically fetched by supervisor from every module via service interface.
It is also possible to obtain these stats with special program [trap_stats](https://github.com/CESNET/Nemea-Framework/blob/master/libtrap/tools/trap_stats.c). 


Socket creation:
----------------

The communication socket for service interface is automatically created by libtrap during context initialization. It means every module has implicit service interface.
Path to the socket is the same as for unix-socket type of interface */tmp/trap-localhost-SPEC.sock*, where SPEC is string *service_PID*. PID is process ID of the module.
For example, communication socket of the service interface of a module with PID 12345 has the following path: */tmp/trap-localhost-service_12345.sock*


Structure of the data:
----------------------

Communication on the service interface socket uses following structure:

```c
struct msg_header_s {
   uint8_t com; ///< Code of the command
   uint32_t data_size; ///< Data size of the buffer sent after the msg_header
}
```

Values used in *com* variable (10 based numbers):
- *10* a request for module statistics
- *11* a request to set some interface parameters (timeouts etc.)
- *12* a reply signaling success

To get interface statistics, a client (for example supervisor) sends this structure with values {com = 10, data_size = 0} and module replies by sending the structure back with values {com = 12, data_size > 0} followed by buffer with statistics in JSON format.
Structure of the data in the buffer is described below.

First two pairs (keys *in_cnt* and *out_cnt*) define number of input and output interfaces of the module.
After these two pairs there are two objects *in* and *out* describing input and output interfaces, each of them followed by array of records.
A record of the object (*in* or *out*) contains interface type, interface ID and interface counters. Interface type can be one of {t, u, f, g, b} values which corresponds to {tcpip, unixsocket, file, generator, blackhole}. Character values are sent as integers (t = 116, u = 117 etc.). Interface ID has a string value and corresponds to port number (tcpip), name of socket (unixsocket), name of file (file) or "none" value (blackhole, generator). Names of the attributes are shown in the example below. It shows JSON data for a module with 1 input interface and 2 output interfaces.
Note: all counters are set to 0.

```json
{
   "in_cnt":1,
   "out_cnt":2,
   "in":[
      {
         "ifc_id":"flow_data_source",
         "ifc_type":117,
         "messages":0,
         "buffers":0
      }
   ],
   "out":[
      {
         "sent-messages":0,
         "ifc_id":"12001",
         "dropped-messages":0,
         "ifc_type":116,
         "autoflushes":0,
         "buffers":0
      },
      {
         "sent-messages":0,
         "ifc_id":"12002",
         "dropped-messages":0,
         "ifc_type":116,
         "autoflushes":0,
         "buffers":0
      }
   ]
}
```