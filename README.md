Nemea Framework
===============

This repository contains base libraries for a [Nemea system](https://github.com/CESNET/Nemea).
The Nemea system is a modular system that consists of 
independent modules for network traffic analysis and anomaly
detection.

The framework consists of:
 * [libtrap](./libtrap) -- communication interface for messages transfer between Nemea modules
 * [UniRec](./unirec) -- flexible and efficient data format of flow-records
 * [common](./common) -- usefull common functions and data structures
 * [python](./python) -- python wrapper for libtrap and UniRec that allows development of nemea modules in python
 * [pycommon](./pycommon) -- python common modules and methods, there is currently a support of alerts creation in the [IDEA](https://idea.cesnet.cz/en/index) format that can be stored into MongoDB or sent to the [Warden](https://wardenw.cesnet.cz/) incident sharing system


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

Python parts must be installed separately when needed.
It can be done using:

```
cd python; sudo python setup.py install
```
and
```
cd pycommon; sudo python setup.py install
```

Project status:
===============

Travis CI build: [![Build Status](https://travis-ci.org/CESNET/Nemea-Framework.svg?branch=master)](https://travis-ci.org/CESNET/Nemea-Framework)

Coverity Scan: [![Coverity Scan Build Status](https://scan.coverity.com/projects/6189/badge.svg)](https://scan.coverity.com/projects/6189)

CodeCov: [![codecov.io](https://codecov.io/github/CESNET/Nemea-Framework/coverage.svg?branch=master)](https://codecov.io/github/CESNET/Nemea-Framework?branch=master)

