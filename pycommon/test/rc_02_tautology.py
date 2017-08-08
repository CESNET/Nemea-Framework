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

	def test_02_basic_match(self):
		self.config = Config(os.path.dirname(__file__) + '/rc_config/tautology.yaml');

		self.config.match(self.msg)

		self.assertEqual(self.msg['Test'], True)

