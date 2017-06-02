import unittest
import os
import json

from reporter_config.Config import Config

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
		os.remove("testfile.idea")
		pass

	def test_01_init(self):
		self.config = Config(os.path.dirname(__file__) + '/rc_config/all_actions.yaml')

