import unittest
import os
import json

from reporter_config.Config import Config

class RCBaseTest(unittest.TestCase):

    def setUp(self):
        """
        Example message created by a conv function in a reporter
        """
        with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
            self.msg = json.load(f)

    def tearDown(self):
        pass

    def test_01_load_basic_config(self):
        """
        Load basic configuration file, parse it and analyze it

        This shouldn't rise any exceptions
        """
        self.config = Config(os.path.dirname(__file__) + '/rc_config/basic.yaml');

        self.assertNotEqual(self.config, None)

        # There should be only one address group
        self.assertEqual(len(self.config.addrGroups), 1)

        # There should be only one custom action and DROP action
        self.assertEqual(len(self.config.actions), 2)

        # There should be only one rule
        self.assertEqual(len(self.config.rules), 1)

    def test_02_basic_match(self):
        self.config = Config(os.path.dirname(__file__) + '/rc_config/basic.yaml');

        try:
            results, actions = self.config.match(self.msg)
        except Exception:
            pass

        self.assertEqual(results[0], True)

