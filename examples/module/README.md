# Example module - README

## Installation
Follow these steps:

1) Let Autotools process the configuration files.

    `autoreconf -i`

2) Configure the module directory.

    `./configure`

3) Build the module.

    `make`

*Important*: The nemea-framework has to be built (or installed) beforehand.

## Description
This module contains example of module implementation using TRAP platform.

## Interfaces
- Inputs: 0
- Outputs: 1

## Parameters
### Common TRAP parameters
- `-h [trap,1]`      Print help message for this module / for libtrap specific parameters.
- `-i IFC_SPEC`      Specification of interface types and their parameters.
- `-v`               Be verbose.
- `-vv`              Be more verbose.
- `-vvv`             Be even more verbose.

## Algorithm
Module recives UniRec format containing two numbers FOO and BAR. Sends UniRec format containing FOO, BAR and their sum as BAZ.

## Troubleshooting
### Loading shared libraries
`error while loading shared libraries: libtrap.so.1: cannot open shared object file: No such file or directory`

Set variable LD_LIBRARY_PATH correctly. Library libtrap.so (located in libtrap/src/.libs) is installed into /usr/local/lib by default.

### TRAP parameters
`ERROR in parsing of parameters for TRAP: Interface specifier (option -i) not found.`

You should provide the module parameters required by the TRAP library. For more information run the module with `-h trap` parameter.