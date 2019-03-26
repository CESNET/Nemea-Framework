import unittest
import subprocess
import os
import re
import sys
import json

# Expected output:
EXPSTRING = """{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "a7c9bff3-92d1-4069-a7fd-d296836c7001", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "df7e2659-3ba4-4aaa-8d05-25b4fde617c6", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "1f46747c-2010-48c3-9d42-cc5a2bd83706", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "8524f27a-f438-4f49-8434-070c5a6fb14b", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "ac918377-9cc0-4eca-8409-617211db02ae", "Description": "Test Message"}
{"Category": ["Test"], "Node": [{"SW": ["Nemea", "test"], "Type": ["Flow", "Statistical"], "Name": "com.example.nemea.test"}], "EventTime": "2019-03-18T20:18:00Z", "Target": [{"Proto": ["tcp"]}], "Format": "IDEA0", "CeaseTime": "2019-03-18T20:18:00Z", "CreateTime": "2019-03-18T20:19:00Z", "Source": [{"Proto": ["tcp"]}], "DetectTime": "2019-03-18T20:18:00Z", "ID": "215b9dff-e1bc-48ba-a2bc-ba37f624685c", "Description": "Test Message"}
"""

idre = r'"ID": "[^"]*",?'

class RCReporterTest(unittest.TestCase):
    def test_run_reporter(self):
        d = os.path.dirname(__file__)
        script = d + "/test2idea.py"
        data = d + "/test_data.trapcap"
        config = d + "/rc_config/stdout.yaml"
        output = subprocess.check_output(["python2" if sys.version_info[0] < 3 else "python3", script, "-D", "-i", "f:" + data, "-c", config], env={"PYTHONPATH": d + "/.."})
        output = re.sub(idre, "", output.decode("utf-8")).split("\n")
        expect = re.sub(idre, "", EXPSTRING).split("\n")

        for i in range(len(expect)):
            if expect[i] and output[i]:
                o = json.loads(output[i].replace("'", '"'))
                e = json.loads(expect[i])
                self.assertEqual(o, e)
            
