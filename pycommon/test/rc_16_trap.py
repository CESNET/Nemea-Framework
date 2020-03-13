# Temporary comment out test that was not revisited yet.
#
#import unittest
#import os
#import json
#
## Skip pytrap test if we can't import pytrap itself
#pytrap_missing = False
#try:
#    import pytrap
#except:
#    pytrap_missing = True
#
#from reporter_config.Config import Config
#
#class RCTrapTest(unittest.TestCase):
#
#    def setUp(self):
#        """
#        Example message created by a conv function in a reporter
#        """
#        with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
#            self.msg = json.load(f)
#
#    def tearDown(self):
#        os.remove("/tmp/rc_testfile")
#
#    @unittest.skipIf(pytrap_missing, "missing pytrap module, skipping TRAP test")
#    def test_01_receive_message(self):
#        """
#        Load trap.yaml configuration file, parse it and analyze it
#
#        TRAP is configured to store messages to file, the test "sends" the alert
#        and checks if the file exists.
#        """
#        trap = pytrap.TrapCtx()
#
#        self.config = Config(os.path.dirname(__file__) + '/rc_config/trap.yaml', trap = trap);
#        self.assertNotEqual(self.config, None)
#
#        # Initialize TRAP interface
#        trap.init(['-i', self.config.conf["custom_actions"][0]["trap"]["config"]], 0, 1)
#
#        trap.setDataFmt(0, pytrap.FMT_JSON, "IDEA")
#
#        self.config.match(self.msg)
#
#        # Check if file exists
#        self.assertTrue(os.path.exists("/tmp/rc_testfile"), True)

