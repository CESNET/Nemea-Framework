import unittest
import os
import simplejson as json

from reporter_config.Config import Config

class RCFileTest(unittest.TestCase):

	def setUp(self):
		"""
		Example message created by a conv function in a reporter
		"""
		with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
			self.msg = json.load(f)

	def tearDown(self):
		pass

	def test_01_file_created(self):
		"""
		Load email.yaml configuration file, parse it and analyze it

		This shouldn't rise any exceptions
		"""
		self.config = Config(os.path.dirname(__file__) + '/rc_config/file.yaml');

		self.assertNotEqual(self.config, None)
		self.config.match(self.msg)

		self.assertTrue(os.path.exists(self.config.conf["custom_actions"][0]["file"]["path"]), True)
		os.remove(self.config.conf["custom_actions"][0]["file"]["path"])

	def test_02_file_content(self):
		self.config = Config(os.path.dirname(__file__) + '/rc_config/file.yaml');

		self.assertNotEqual(self.config, None)
		self.config.match(self.msg)

		filepath = self.config.conf["custom_actions"][0]["file"]["path"]

		# Delete the Config instance in order to close file pointer
		del self.config

		with open(filepath, 'r') as f:
			self.content = json.load(f)
		os.remove(filepath)

		self.assertTrue(self.content["ID"], "e214d2d9-359b-443d-993d-3cc5637107a0")
