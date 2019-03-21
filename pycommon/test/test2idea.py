#!/usr/bin/env python

from __future__ import absolute_import

import os, sys
import argparse

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from report2idea import *

# Moudle name, description and required input data format
MODULE_NAME = "test"
MODULE_DESC = "test"
REQ_TYPE = pytrap.FMT_UNIREC
REQ_FORMAT = ""

# Main conversion function
def convert_to_idea(rec, opts):
    endTime = getIDEAtime(pytrap.UnirecTime("2019-03-18T20:18:00Z"))
    idea={
       "Format": "IDEA0",
       "ID": getRandomId(),
       'CreateTime': getIDEAtime(pytrap.UnirecTime("2019-03-18T20:19:00Z")),
       "EventTime": endTime,
       'CeaseTime': endTime,
       "DetectTime": endTime,
       "Category": ["Test"],
       "Description": "Test Message",
       "Source": [{
             "Proto": ["tcp"]
        }],
       "Target": [{
             "Proto": ["tcp"],
       }],
       'Node': [{
          'Name': 'undefined',
          'SW': ['Nemea','test'],
          'Type': ['Flow', 'Statistical'],
       }],
    }
    return idea

# Run the module
if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser()
    Run(MODULE_NAME, MODULE_DESC, REQ_TYPE, REQ_FORMAT, convert_to_idea, arg_parser)

