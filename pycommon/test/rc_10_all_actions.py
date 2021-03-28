import unittest
import os
import json

from reporter_config.Config import Config

skip_test = False

# Try to import all needed modules, if any is missing skip this test
try:
    import pymongo
    import warden_client
    import pynspect
    import pytrap
except:
    skip_test = True

class RCAllActionsTest(unittest.TestCase):
    """Instantiate all custom_actions types
    """

    def setUp(self):
        """
        Example message created by a conv function in a reporter
        """
        with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
            self.msg = json.load(f)


    def tearDown(self):
        """
        Should:
            * delete created file
            * remove database test_reporter_config from mongoDB (not needed in this case)
        """
        # Silently fail if file not found
        try:
            os.remove("testfile.idea")
        except:
            pass

    @unittest.skipIf(skip_test, "missing some modules, skipping all actions init test")
    def test_01_init(self):
        self.config = Config(os.path.dirname(__file__) + '/rc_config/all_actions.yaml')

