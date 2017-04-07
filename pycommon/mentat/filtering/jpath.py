#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
# This file is part of Mentat system (https://mentat.cesnet.cz/).
#
# Copyright (C) since 2011 CESNET, z.s.p.o (http://www.ces.net/)
# Use of this source is governed by the MIT license, see LICENSE file.
#-------------------------------------------------------------------------------

"""
This module provides tools for parsing **JPaths** and setting or retrieving values
on given **JPath** within data structures.

*JPath* is simplified version of `JSONPath <http://goessner.net/articles/JsonPath/>`__
and can be used to addressing nodes within arbitrary data structure composed
of dict-like and list-like objects. Basically it can be used for any data
structure of objects implementing Python 3 list and/or dict interface.

The motivation for implementing this module were following two use cases:

* Enable writing of simple rules in various filtering expressions, for example::

    Source.IP4 in [192.168.0.0/24, 192.168.0.0/24]

* Enable simple message modifications based on key => value rules, for example::

    "Source[1].Type[*]" = "source type tag"

The obvious first choice for a solution was the `jsonpath-rw <https://pypi.python.org/pypi/jsonpath-rw>`__
library. The full *JSONPath* however seems to be too big of a gun for our needs and
in some cases it could even enable users to cut the branch they are sitting on. For
this reason we have designed this simplified version with only required set of basic features.

*JPath* syntax uses only dot characters ``.`` as node delimiters. Each node name may
contain only one or more of the following characters::

    [a-zA-Z0-9_]+

Node delimiters implicitly work with nested dictionaries and using delimiter
results in appending new dictionary as a value of given key in parent disctionary.
Working with lists is enabled by using indices. List indices must be enclosed in
brackets '[' and ']' and may contain one of the following values:

* ``[int]`` - precise index (negative values not permitted, numbering starts with 1)
* ``[#]`` - last (because you might not know number of nodes)
* ``[*]`` - all nodes
* (index omitted)

When retrieving value(s) at given *JPath*, use of indices will have following effects:

* ``[int]`` - Return node at particular list position (starting with 1)
* ``[#]`` - Return last node
* ``[*]`` - Return all nodes (same as omitting)
* (index omitted) - Return all nodes (same as '*')

When setting value(s) to given *JPath*, use of indices will have following effects:

* ``[int]`` - Set value to particular list node (starting with 1)
* ``[#]`` - Set value to already existing last node, or append new one to an empty list
* ``[*]`` - Append new value to a list
* (index omitted) - This will result in a dictionary key instead of a list

Consider following examples::

    >>> msg = {
       'Format': 'IDEA0',
       'ID': 'MESSAGE_ID',
       'DetectTime': 'DETECT TIME',
       'Category': ['CATEGORY'],
       'ConnCount': 633,
       'Description': 'Ping scan',
       'Source': [
          {
             'IP4': ['192.168.1.1', '192.168.1.2'],
             'Proto': ['icmp']
          },
          {
             'IP4': ['192.168.2.1', '192.168.2.2'],
             'Proto': ['icmp']
          }
       ],
       'Target': [
          {
             'Proto': ['icmp'],
             'IP4': ['192.168.3.1', '192.168.3.2'],
             'Anonymised': True
          }
       ],
       'Node': [
          {
             'SW' : ['KIPPO'],
             'Name' : 'NODE_NAME'
          }
       ]
    }

    >>> jpath_value(msg, 'Format')
    'IDEA0'
    >>> jpath_value(msg, 'Category')
    'CATEGORY'
    >>> jpath_value(msg, 'Node.Name')
    'NODE_NAME'
    >>> jpath_value(msg, 'Source.IP4')
    '192.168.1.1'

    >>> jpath_values(msg, 'Format')
    ['IDEA0']
    >>> jpath_values(msg, 'Category')
    ['CATEGORY']
    >>> jpath_values(msg, 'Node.Name')
    ['NODE_NAME']
    >>> jpath_values(msg, 'Source.IP4')
    ['192.168.1.1', '192.168.1.2', '192.168.2.1', '192.168.2.2']

The current implementation has following known drawbacks:

* Toplevel element must be a dict-like object
* Nested list-like objects are not possible: ``[[1,2],[3,4]]``
* It is not possible to set value to multiple elements at once
* It is not possible to customize type of created structure, only lists and
  dicts are always created

"""

__version__ = "0.1"
__author__ = "Jan Mach <jan.mach@cesnet.cz>"
__credits__ = "Pavel Kácha <pavel.kacha@cesnet.cz>, Andrea Kropáčová <andrea.kropacova@cesnet.cz>"

import re
import collections

#
# Define global constants.
#

#: Status code for ``success``, returned by function :py:func:`jpath_set`
RC_VALUE_SET = 0

#: Status code for ``already-exists``, returned by function :py:func:`jpath_set`
RC_VALUE_EXISTS = 1

#: Status code for ``not-unique``, returned by function :py:func:`jpath_set`
RC_VALUE_DUPLICATE = 2

class JPathException(Exception):
    """
    Custom JPath specific exception.

    This exception will be thrown on module specific errors.
    """
    def __init__(self, description):
        self._description = description
    def __str__(self):
        return repr(self._description)

def jpath_parse(jpath):
    """
    Parse given JPath into chunks.

    Returns list of dictionaries describing all of the JPath chunks.

    :param str jpath: JPath to be parsed into chunks
    :return: JPath chunks as list of dicts
    :rtype: :py:class:`list`
    :raises JPathException: in case of invalid JPath syntax
    """
    result = []
    breadcrumbs = []

    # Pattern for matching chunk syntax
    ptrn = re.compile(r"^([a-zA-Z0-9_]+)(\[(#|\*|\d+)\])?$")

    # Split JPath into chunks based on '.' character
    chunks = jpath.split('.')
    for ch in chunks:
        match = ptrn.match(ch)
        if match:
            r = {}

            # Record whole match
            r['m'] = ch

            # Record breadcrumb path
            breadcrumbs.append(ch)
            r['p'] = '.'.join(breadcrumbs)

            # Handle node name
            r['n'] = match.group(1)

            # Handle node index (optional, may be omitted)
            if match.group(2):
                r['i'] = match.group(3)
                if str(r['i']) == '#':
                    r['i'] = -1
                elif str(r['i']) == '*':
                    pass
                else:
                    r['i'] = int(r['i']) - 1

            result.append(r)
        else:
            raise JPathException("Invalid JPath chunk '{}'".format(ch))
    return result

def jpath_values(structure, jpath):
    """
    Return all values at given JPath within given data structure.

    For performance reasons this method is intentionally not written as
    recursive.

    :param str structure: data structure to be searched
    :param str jpath: JPath to be evaluated
    :return: found values as a list
    :rtype: :py:class:`list`
    """
    # Current working node set
    nodes_a = [structure]

    # Next iteration working node set
    nodes_b = []

    # Process sequentially all JPath chunks
    chunks = jpath_parse(jpath)
    for ch in chunks:
        # Process all currently active nodes
        for node in nodes_a:
            key = ch['n']

            if not isinstance(node, dict) and not isinstance(node, collections.Mapping):
                continue

            # Process indexed nodes
            if 'i' in ch:
                idx = ch['i']

                # Skip the node, if the key does not exist, the value is not
                # a list-like object or the list is empty
                if not key in node or not isinstance(node[key], list) or not isinstance(node[key], collections.MutableSequence) or not len(node[key]):
                    continue
                try:
                    # Handle '*' special index - append all nodes
                    if str(idx) == '*':
                        for i in node[key]:
                            nodes_b.append(i)
                    # Append only node at particular index
                    else:
                        nodes_b.append(node[key][idx])
                except:
                    pass

            # Process unindexed nodes
            else:
                # Skip the node, if the key does not exist
                if not key in node:
                    continue

                # Handle list values - expand them
                if isinstance(node[key], list) or isinstance(node[key], collections.MutableSequence):
                    for i in node[key]:
                        nodes_b.append(i)
                # Handle scalar values
                else:
                    nodes_b.append(node[key])

        nodes_a = nodes_b
        nodes_b = []

    return nodes_a

def jpath_value(structure, jpath):
    """
    Return single value or first value from list at given JPath within
    given data structure.

    This method returns None for non-existent JPaths.

    :param str structure: data structure to be searched
    :param str jpath: JPath to be evaluated
    :return: None or found value
    """
    values = jpath_values(structure, jpath)
    if values:
        return values[0]
    return None

def jpath_exists(structure, jpath):
    """
    Check if node at given JPath within given data structure does exist.

    :param str structure: data structure to be searched
    :param str jpath: JPath to be evaluated
    :return: True or False
    :rtype: bool
    """
    result = jpath_value(structure, jpath)
    if not result is None:
        return True
    return False

def jpath_set(structure, jpath, value, overwrite = True, unique = False):
    """
    Set given JPath to given value within given structure.

    For performance reasons this method is intentionally not written as
    recursive.

    :param str structure: data structure to be searched
    :param str jpath: JPath to be evaluated
    :param any value: value of any type to be set at given path
    :param bool overwrite: enable/disable overwriting of already existing value
    :param bool unique: ensure uniqueness of value, works only for lists
    :return: numerical return code, one of the (:py:data:`RC_VALUE_SET`,
             :py:data:`RC_VALUE_EXISTS`, :py:data:`RC_VALUE_DUPLICATE`)
    :rtype: int
    """
    chunks = jpath_parse(jpath)
    size = len(chunks) - 1
    current = structure

    # Process chunks in order, enumeration is used for detection of the last JPath chunk.
    for i, ch in enumerate(chunks):
        key = ch['n']

        if not isinstance(current, dict) and not isinstance(current, collections.Mapping):
            raise JPathException("Expected dict-like structure to attach node '{}'".format(ch['p']))

        # Process indexed nodes
        if 'i' in ch:
            idx = ch['i']

            # Automatically create nodes for non-existent keys
            if not key in current:
                current[key] = []
            if not isinstance(current[key], list) and not isinstance(current[key], collections.MutableSequence):
                raise JPathException("Expected list-like object under structure key '{}'".format(key))

            # Detection of the last JPath chunk - node somewhere in the middle
            if i != size:
                # Attempt to access node at given index
                try:
                    current = current[key][idx]
                # IndexError: list index out of range
                # Node at given index does not exist, append new one. Using insert()
                # does not work, item is appended to the end of the list anyway.
                # TypeError: list indices must be integers or slices, not str
                # In the case list index was '*', we are appending to the end of
                # list.
                except (IndexError, TypeError):
                    current[key].append({})
                    current = current[key][-1]

            # Detection of the last JPath chunk - node at the end
            else:
                # Attempt to insert value at given index
                try:
                    if overwrite or not current[key][idx]:
                        current[key][idx] = value
                    else:
                        return RC_VALUE_EXISTS
                # IndexError: list index out of range
                # Node at given index does not exist, append new one. Using insert()
                # does not work, item is appended to the end of the list anyway.
                # TypeError: list indices must be integers or slices, not str
                # In the case list index was '*', we are appending to the end of
                # list.
                except (IndexError, TypeError):
                    # At this point only deal with unique, overwrite does not make
                    # sense, because we would not be here otherwise.
                    if not unique or not value in current[key]:
                        current[key].append(value)
                    else:
                        return RC_VALUE_DUPLICATE

        # Process unindexed nodes
        else:
            # Detection of the last JPath chunk - node somewhere in the middle
            if i != size:
                # Automatically create nodes for non-existent keys
                if not key in current:
                    current[key] = {}
                if not isinstance(current[key], dict) and not isinstance(current[key], collections.Mapping):
                    raise JPathException("Expected dict-like object under structure key '{}'".format(key))

                current = current[key]

            # Detection of the last JPath chunk - node at the end
            else:
                if overwrite or not key in current:
                    current[key] = value
                else:
                    return RC_VALUE_EXISTS
    return RC_VALUE_SET

if __name__ == "__main__":
    """
    Perform the demonstration.
    """
    import pprint

    print("Path parsing:")
    pprint.pprint(jpath_parse("Test"))
    pprint.pprint(jpath_parse("Test.Path"))
    pprint.pprint(jpath_parse("Long.Test.Path"))
    pprint.pprint(jpath_parse("Long[1].Test.Path"))
    pprint.pprint(jpath_parse("Long.Test[2].Path"))
    pprint.pprint(jpath_parse("Long.Test.Path[3]"))
    pprint.pprint(jpath_parse("Long[*].Test.Path"))
    pprint.pprint(jpath_parse("Long.Test[*].Path"))
    pprint.pprint(jpath_parse("Long.Test.Path[*]"))
    pprint.pprint(jpath_parse("Long[#].Test.Path"))
    pprint.pprint(jpath_parse("Long.Test[#].Path"))
    pprint.pprint(jpath_parse("Long.Test.Path[#]"))

    print("Path fetching:")
    msg = {
        'TestA': { 'ValueA1': 'A1', 'ValueA2': 'A2' },
        'TestB': { 'ValueB1': 'B1', 'ValueB2': 'B2' },
        'TestC': { 'ValueC1': 'C1', 'ValueC2': 'C2' },
        'TestD': { 'ValueD1': ['D11','D12'], 'ValueC2': 'C2' }
    }
    pprint.pprint(jpath_values(msg, 'TestD.ValueD1'))
    pprint.pprint(jpath_values(msg, 'TestD.ValueD1[1]'))
    pprint.pprint(jpath_values(msg, 'TestD.ValueD1[2]'))
    pprint.pprint(jpath_values(msg, 'TestD.ValueD1[#]'))

    print("Path seting:")
    msg = {
        'TestA': { 'ValueA1': 'A1', 'ValueA2': 'A2' },
        'TestB': { 'ValueB1': 'B1', 'ValueB2': 'B2' },
        'TestC': { 'ValueC1': 'C1', 'ValueC2': 'C2' },
        'TestD': { 'ValueD1': ['D11','D12'], 'ValueC2': 'C2' }
    }
    pprint.pprint(jpath_set(msg, 'TestE.ValueE1', "Added value"))
    pprint.pprint(jpath_set(msg, 'TestE.ValueE2[1]', "Added value 2"))
    pprint.pprint(jpath_set(msg, 'TestE.ValueE2[2]', "Added value 3"))
    pprint.pprint(jpath_set(msg, 'TestE.ValueE2[#]', "Added value 4"))
    pprint.pprint(jpath_set(msg, 'TestE.ValueE3[1].Subval1', "Added subvalue 11"))
    pprint.pprint(jpath_set(msg, 'TestE.ValueE3[1].Subval2[1]', "Added subval 21"))
    pprint.pprint(jpath_set(msg, 'TestE.ValueE3[#].Subval2[2]', "Added subval 22"))
    pprint.pprint(msg)
