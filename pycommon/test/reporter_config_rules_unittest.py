import unittest
import os
import json

from reporter_config.Config import Config

class RCBaseTest(unittest.TestCase):

	def setUp(self):
		"""
		Example message created by a conv function in a reporter
		"""
		with open(os.path.dirname(__file__) + '/reporter_config_msg.json', 'r') as f:
			self.msg = json.load(f)

		self.config = Config(os.path.dirname(__file__) + '/rc_config/all_actions.yaml')

	def tearDown(self):
		"""
		Should:
			* delete created file
			* remove database test_reporter_config from mongoDB
		"""
		pass

	def test_01_drop(self):
		pass
	def test_02_email(self):
		pass
	def test_03_file(self):
		pass
	def test_04_mark(self):
		pass
	def test_05_mongo(self):
		pass
	def test_06_trap(self):
		pass
	def test_07_warden(self):
		pass

	#self.config.match(self.msg)

		#self.assertEqual(self.msg['Test'], True)

