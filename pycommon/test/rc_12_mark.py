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

		self.assertEqual(self.msg['Test'], True)

	def test_02_mark_nested(self):
		self.msg["Target"][0]["IP4"].append("1.2.3.4")

		self.config = Config(os.path.dirname(__file__) + '/rc_config/mark.yaml');

		self.assertNotEqual(self.config, None)
		self.config.match(self.msg)

		self.assertEqual(self.msg["_CESNET"]["Status"]["Processed"], True)

