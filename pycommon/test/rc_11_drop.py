import unittest
import os
import json

from reporter_config.Config import Config
from reporter_config.actions.Drop import DropMsg

class RCDropTest(unittest.TestCase):

    def setUp(self):
        """
        Example message created by a conv function in a reporter
        """
        with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
            self.msg = json.load(f)

    def tearDown(self):
        pass

    def test_01_drop(self):
        """
        Load drop.yaml configuration file, parse it and analyze it

        """
        self.config = Config(os.path.dirname(__file__) + '/rc_config/drop.yaml');

        self.assertNotEqual(self.config, None)

        self.config.match(self.msg)

