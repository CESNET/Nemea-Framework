import unittest
import os
import json



from reporter_config.Config import Config


class RCMarkTest(unittest.TestCase):
    def setUp(self):
        """
        Example message created by a conv function in a reporter
        """
        with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
            self.msg = json.load(f)

    def tearDown(self):
        pass

    def test_01_mark_simple(self):
        """
        Load mark.yaml configuration file, parse it and analyze it

        This shouldn't rise any exceptions
        """
        self.config = Config(os.path.dirname(__file__) + '/rc_config/mark.yaml');

        self.assertNotEqual(self.config, None)
        self.config.match(self.msg)

        of1 = "/tmp/output1.idea"
        of2 = "/tmp/output2.idea"

        with open(of1, "r") as f:
            stored = json.load(f)
        self.assertEqual(stored['Test'], True)

        with open(of2, "r") as f:
            stored = json.load(f)
        self.assertEqual(stored["_CESNET"]["Status"]["Processed"], True)

        try:
            os.unlink(of1)
        except Exception:
            pass
        try:
            os.unlink(of2)
        except Exception:
            pass

