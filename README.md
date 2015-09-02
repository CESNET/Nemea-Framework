Nemea Framework
===============

This repository contains base libraries for a Nemea system.
The Nemea system is a modular system that consists of 
independent modules for network traffic analysis and anomaly
detection.

The framework consists of:
 * [libtrap](./libtrap) -- communication interface for messages transfer between Nemea modules
 * [UniRec](./unirec) -- flexible and efficient data format of flow-records
 * [common](./common) -- usefull common functions and data structures
 * [python](./python) -- python wrapper for libtrap and UniRec that allows development of nemea modules in python


Installation
============

This repository is usually used as a git submodule of https://github.com/CESNET/Nemea
However, it can be installed independently using:

```
./bootstrap.sh
./configure
make
sudo make install
```

For information about configuration options see:
```
./configure --help
```

