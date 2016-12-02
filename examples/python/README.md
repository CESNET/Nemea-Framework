# Example of NEMEA modules in Python

## Repeater

The [./python_example.py](./python_example.py) script works as a simple traffic repeater i.e. it receives UniRec messages via input IFC and
sends them through output IFC.

Thanks to libtrap negotiation feature, the module can be connected to any other Nemea module and negotiation of data
format is done automatically. `./python_example.py` accepts any specifier (`""`) of UniRec data format
(`pytrap.TRAP_FMT_UNIREC`) on input IFC (using `trap.set_required_fmt(0, pytrap.TRAP_FMT_UNIREC, "")`)

On input IFC, data format can be retrieved using: `(fmttype, fmtspec) = trap.get_data_fmt(pytrap.IFC_INPUT, 0)`.

See source code of [./python_example.py](./python_example.py) for complete running example.

### How to start

Try to execute:

```
./python_example.py -h
```

and

```
./python_example.py -h trap
```

to get basic information about the Nemea module and about libtrap IFC parameters (-i).

To start the module, use e.g.:

```
./python_example.py -i u:input-socket,u:output-socket
```

This will make the module listening on UNIX socket IFC with `input-socket` identifier and incomming messages will be resent via UNIX socket IFC with `output-socket` identifier.

It is needed to start any Nemea module as a data source and any module that will receive messages from `./python_example.py`.  The modules from [Nemea-Modules](https://github.com/CESNET/Nemea-Modules) can be used.

**NOTE:** `./python_example.py` claims that it has additional parameters `-f` and `-q`. However, these parameters
have no effect.  They are listed just for a demonstration of the `optparse.OptionParser` class that can be used
in python Nemea modules for unified help output.

## Example of Detection module

[./detection_example.py](./detection_example.py) is a good starting point for writing a detection module.  It contains one input IFC for receiving flow records (same UniRec templates as the [flow_meter](https://github.com/CESNET/Nemea-Modules/tree/master/flow_meter) exporter sends).

The example also shows how to fill in and send an alert - detected IP.

### How to start

To try the module, it is possible to use:

1) `flow_meter` with PCAP file:

```
./flow_meter -i u:flow_source:timeout=WAIT -r /path/to/pcap
```

Note: timeout=WAIT sets the output IFC into a blocking mode, this is useful for offline testing

or live capture from the network adapter:

```
sudo ./flow_meter -i u:flow_source:timeout=WAIT -I eth0
```

2) `logger` to visualize alert:

```
./logger -t  -i u:alerts
```

and finally 3) `detection_example.py`:

```
./detection_example.py -i u:flow_source,u:alerts:buffer=off
```

Note: `buffer=off` sets the IFC to send messages one by one without using buffer. This is useful for the offline testing.

## Troubleshooting

In case the example script fails with:

```
OSError: libtrap.so: cannot open shared object file: No such file or directory
```

please, make sure that libtrap is installed on the system.
It is also possible to use libtrap that is not installed yet -- in this case, use:

```
export LD_LIBRARY_PATH=../libtrap/src/.libs/
```

where `../libtrap/src/.libs/` is the relative path from the `python/` directory in the downloaded and compiled
Nemea-Framework repository.

