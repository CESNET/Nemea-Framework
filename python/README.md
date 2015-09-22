# Python Wrapper for TRAP

This directory containes python wrapper that can be used for development of Nemea modules written in python.

Installation can be done using standard the ./setup.py script by this command:
```
python ./setup.py install
```
The command should be performed as root (e.g. using sudo).

## Basic usage

After successful installation, a Nemea module in python can be run.
The example of basic usage is covered in [./python_example.py](./python_example.py) that imports
`[./trap.py](./trap.py)` (wrapper for libtrap with communication interfaces) and
`[./unirec/unirec.py](./unirec/unirec.py)` (wrapper for UniRec containing data format handling).

The `./python_example.py` script works as a simple traffic repeater i.e. it receives UniRec messages via input IFC and
sends them through output IFC.

Thanks to libtrap negotiation feature, the module can be connected to any other Nemea module and negotiation of data
format is done automatically. `./python_example.py` accepts any specifier (`""`) of UniRec data format
(`trap.TRAP_FMT_UNIREC`) on input IFC (using `trap.set_required_fmt(0, trap.TRAP_FMT_UNIREC, "")`)

On input IFC, data format can be retrieved using: `(fmttype, fmtspec) = trap.get_data_fmt(trap.IFC_INPUT, 0)`.

See source code of [./python_example.py](./python_example.py) for complete running example.
