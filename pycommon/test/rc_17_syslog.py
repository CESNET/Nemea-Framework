#!/usr/bin/python3
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
        with self.assertRaises(Exception):
            self.config = Config(os.path.dirname(__file__) + '/rc_config/syslog-malformed-facility.yaml');

    def test_02_syslogerror_priority(self):
        """
        Load malformed syslog configuration file, parse it and analyze it
        This should rise exception
        """
        with self.assertRaises(Exception):
            self.config = Config(os.path.dirname(__file__) + '/rc_config/syslog-malformed-priority.yaml');

    def test_03_syslogerror_logoption(self):
        """
        Load malformed syslog configuration file, parse it and analyze it
        This should rise exception
        """
        with self.assertRaises(Exception):
            self.config = Config(os.path.dirname(__file__) + '/rc_config/syslog-malformed-logoption.yaml');

    def test_04_syslog(self):
        """
        Load syslog.yaml configuration file, parse it and analyze it
        This shouldn't rise any exceptions
        """
        self.config = Config(os.path.dirname(__file__) + '/rc_config/syslog.yaml');

        self.assertNotEqual(self.config, None)
        self.config.match(self.msg)

if __name__ == '__main__':
    unittest.main()

