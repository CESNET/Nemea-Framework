import unittest
import os
import json

from reporter_config.Config import Config

class RCAddressGroupTest(unittest.TestCase):

        def setUp(self):
                """
                Example message created by a conv function in a reporter
                """
                with open("/tmp/testwhitelist", 'w') as f:
                        f.write("192.168.0.0/24\n10.0.1.1\n")
                with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
                        self.msg = f.read()
                self.config = Config(os.path.dirname(__file__) + '/rc_config/mongo.yaml');

                # format from IDEA message: "Source": [{"IP4": ["1.2.3.4"]}]
                self.messages_pass = []
                msg = json.loads(self.msg)
                self.messages_pass.append(msg)
                msg = json.loads(self.msg)
                msg["Source"][0]["IP4"][0] = "10.0.0.10"
                self.messages_pass.append(msg)
                msg = json.loads(self.msg)
                msg["Source"][0]["IP4"][0] = "192.168.0.254"
                self.messages_pass.append(msg)
                msg = json.loads(self.msg)
                msg["Source"][0]["IP4"][0] = "10.0.1.1"
                self.messages_pass.append(msg)

                self.messages_notpass = []
                msg = json.loads(self.msg)
                msg["Source"][0]["IP4"][0] = "1.2.3.5"
                self.messages_notpass.append(msg)
                msg = json.loads(self.msg)
                msg["Source"][0]["IP4"][0] = "10.10.0.1"
                self.messages_notpass.append(msg)
                msg = json.loads(self.msg)
                msg["Source"][0]["IP4"][0] = "192.168.1.1"
                self.messages_notpass.append(msg)

        def test_01_list(self):
                """
                Load mongo.yaml configuration file, parse it and analyze it

                This shouldn't rise any exceptions

                Should store message in DB and test if there is one record
                """
                self.config = Config(os.path.dirname(__file__) + '/rc_config/addressgroup.yaml');

                self.assertNotEqual(self.config, None)
                print("passing rules")
                for idea in self.messages_pass:
                    print(self.config.match(idea))

                print("NOT passing rules")
                for idea in self.messages_notpass:
                    print(self.config.match(idea))




