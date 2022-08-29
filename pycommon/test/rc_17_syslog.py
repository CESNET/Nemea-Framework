import unittest
import os
import json

from reporter_config.Config import Config, Parser

class RCSyslogTest(unittest.TestCase):

    def setUp(self):
        """
        Example message created by a conv function in a reporter
        """
        with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
            self.msg = json.load(f)

    def tearDown(self):
        pass

    def test_01_syslogerror_facility(self):
        """
        Load malformed syslog configuration file, parse it and analyze it
        This should rise exception
        """
        self.parser = Parser(os.path.dirname(__file__) + '/rc_config/syslog-malformed-facility.yaml');
        with self.assertRaises(Exception):
            self.config = Config(self.parser);

    def test_02_syslogerror_priority(self):
        """
        Load malformed syslog configuration file, parse it and analyze it
        This should rise exception
        """
        self.parser = Parser(os.path.dirname(__file__) + '/rc_config/syslog-malformed-priority.yaml');
        with self.assertRaises(Exception):
            self.config = Config(self.parser);

    def test_03_syslogerror_logoption(self):
        """
        Load malformed syslog configuration file, parse it and analyze it
        This should rise exception
        """
        self.parser = Parser(os.path.dirname(__file__) + '/rc_config/syslog-malformed-logoption.yaml');
        with self.assertRaises(Exception):
            self.config = Config(self.parser);

    def test_04_syslog(self):
        """
        Load email.yaml configuration file, parse it and analyze it
        This shouldn't rise any exceptions
        """
        self.parser = Parser(os.path.dirname(__file__) + '/rc_config/syslog.yaml');
        self.config = Config(self.parser);

        self.assertNotEqual(self.config, None)
        self.config.match(self.msg)

