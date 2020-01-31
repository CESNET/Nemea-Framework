import unittest
import subprocess
import os
import re
import sys
import json

# Expected output:
EXPSTRING = """{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "79fb01dd-135c-4f36-a88d-23f7dff697aa", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "79fb01dd-135c-4f36-a88d-23f7dff697aa", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "91301388-b2a0-44c1-9b98-9cd741d9dfa9", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "91301388-b2a0-44c1-9b98-9cd741d9dfa9", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "67d34bbd-ae90-4395-87be-2c74ca32f0d3", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "67d34bbd-ae90-4395-87be-2c74ca32f0d3", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "0ba775ff-420d-4714-a21b-b465278b1ccb", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "0ba775ff-420d-4714-a21b-b465278b1ccb", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "4735b74c-e65c-42f9-ae4c-6bec446ad500", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "4735b74c-e65c-42f9-ae4c-6bec446ad500", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "aeecc5a8-dedc-4bbf-96b7-fd64ab376863", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"IP4": ["192.168.0.1", "192.168.1.1", "10.0.0.1"], "Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "aeecc5a8-dedc-4bbf-96b7-fd64ab376863", "Description": "Test Message"}
"""

idre = r'"ID": "[^"]*",?'

class RCReporterTest(unittest.TestCase):
    def test_run_reporter(self):
        d = os.path.dirname(__file__)
        script = d + "/testiprange2idea.py"
        data = d + "/test_data.trapcap"
        config = d + "/rc_config/iprange_stdout.yaml"
        output = subprocess.check_output(["python2" if sys.version_info[0] < 3 else "python3", script, "-D", "-i", "f:" + data, "-c", config], env={"PYTHONPATH": d + "/.."})
        output = re.sub(idre, "", output.decode("utf-8")).split("\n")
        expect = re.sub(idre, "", EXPSTRING).split("\n")

        output = list(filter(lambda l: l != "", output))
        self.assertEqual(len(output), 18)

        for i in range(len(expect)):
            if expect[i] and output[i]:
                o = json.loads(output[i].replace("'", '"'))
                e = json.loads(expect[i])
                self.assertEqual(o, e)

