# Python Wrapper for TRAP

This directory containes python wrapper that can be used for development of Nemea modules written in python.

Installation can be done using standard the ./setup.py script by this command:
```
python ./setup.py install
```
The command should be performed as root (e.g. using sudo).

## Basic usage

After successful installation, a Nemea module in python can be run.
The example of basic usage is covered in [../examples/python/python_example.py](../examples/python/python_example.py) that imports
[./trap.py](./trap.py) (wrapper for libtrap with communication interfaces) and
[./unirec/unirec.py](./unirec/unirec.py) (wrapper for UniRec containing data format handling).