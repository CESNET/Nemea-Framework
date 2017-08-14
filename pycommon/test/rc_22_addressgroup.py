import unittest
import os
import json
import copy

from reporter_config.Config import Config

class RCAddressGroupTest(unittest.TestCase):

        def setUp(self):
                """
                Example message created by a conv function in a reporter
                """
                with open("/tmp/testwhitelist", 'w') as f:
                        f.write("192.168.0.0/24\n10.0.1.1\n")
                with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
                        self.msg = json.load(f)
                self.config = Config(os.path.dirname(__file__) + '/rc_config/mongo.yaml');

                # format from IDEA message: "Source": [{"IP4": ["1.2.3.4"]}]
                self.messages_pass = []
                for ip in ["10.0.0.9", "10.0.0.10", "192.168.0.254", "10.0.1.1"]:
                    m = copy.deepcopy(self.msg)
                    m["Source"][0]["IP4"][0] = ip
                    self.messages_pass.append(m)

                self.messages_notpass = []
                for ip in ["1.2.3.5", "10.10.0.1", "192.168.1.1"]:
                    m = copy.deepcopy(self.msg)
                    m["Source"][0]["IP4"][0] = ip
                    self.messages_notpass.append(m)

        def test_01_list(self):
                """
                Load mongo.yaml configuration file, parse it and analyze it

                This shouldn't rise any exceptions

                Should store message in DB and test if there is one record
                """
                self.config = Config(os.path.dirname(__file__) + '/rc_config/addressgroup.yaml');

                self.assertNotEqual(self.config, None)
                for idea in self.messages_pass:
                    results, actions = self.config.match(idea)
                    if True in results:
                        pass
                    else:
                        print("passing rules")
                        print("Test FAILED!!!")

                for idea in self.messages_notpass:
                    results, actions = self.config.match(idea)
                    if True not in results:
                        pass
                    else:
                        print("NOT passing rules")
                        print("Test FAILED!!!")


