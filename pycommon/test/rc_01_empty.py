import unittest
import os
import json

from reporter_config.actions.Drop import DropMsg
from reporter_config.Config import Config

class RCEmptyFileTest(unittest.TestCase):

    def setUp(self):
        """
        Example message created by a conv function in a reporter
        """
        with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
            self.msg = json.load(f)

    def tearDown(self):
        pass

    def test_00_load_basic_config(self):
        try:
            self.config = Config(os.path.dirname(__file__) + '/rc_config/empty.yaml');
            self.fail("Empty configuration file shouldn't be loaded! It must contain rules.")
        except Exception as e:
            self.assertEqual(str(e), "YAML file must contain `rules`.")

    def test_01_load_basic_config(self):
        try:
            self.config = Config(os.path.dirname(__file__) + '/rc_config/incompleterules.yaml');
            self.fail("Rules must contain at least one rule.")
        except Exception as e:
            self.assertEqual(str(e), "YAML file should contain at least one `rule` in `rules`.")

    def test_02_onerule(self):
        self.config = Config(os.path.dirname(__file__) + '/rc_config/minimal.yaml');
        self.assertNotEqual(self.config, None)

    def test_03_oneruledrop(self):
        self.config = Config(os.path.dirname(__file__) + '/rc_config/minimaldrop.yaml');
        self.assertNotEqual(self.config, None)

        results, actions = self.config.match(self.msg)
        self.assertEqual(results, [True])

    def test_04_secondruleterminated(self):
        of = "/tmp/output1.idea"
        if os.path.isfile(of):
            os.unlink(of)
        self.config = Config(os.path.dirname(__file__) + '/rc_config/minimalfirstdrop.yaml');
        self.assertNotEqual(self.config, None)

        results, actions = self.config.match(self.msg)
        # only one rule should have been processed since drop was in the first rule
        self.assertEqual(results, [True])

        if os.path.isfile(of) and os.stat(of).st_size:
            os.unlink(of)
            self.fail("Drop action was the first one, file should not be existing.")
        else:
            try:
                os.unlink(of)
            except Exception:
                pass

    def test_05_malformedyaml(self):
        try:
            self.config = Config(os.path.dirname(__file__) + '/rc_config/malformedyaml.yaml');
            self.fail("Malformed YAML must raise exception with error.")
        except Exception:
            pass

