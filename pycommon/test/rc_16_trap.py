import unittest
import os
import simplejson as json

import pytrap

from reporter_config.Config import Config

class RCTrapTest(unittest.TestCase):

	def setUp(self):
		"""
		Example message created by a conv function in a reporter
		"""
		with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
			self.msg = json.load(f)

	def tearDown(self):
		pass

	@unittest.skip("skipping TRAP test")
	def test_01_receive_message(self):
		"""
		Load trap.yaml configuration file, parse it and analyze it

		This shouldn't rise any exceptions
		"""
		self.config = Config(os.path.dirname(__file__) + '/rc_config/trap.yaml');

		self.assertNotEqual(self.config, None)
		self.config.match(self.msg)

		# Initialize TRAP interface
		trap = pytrap.TrapCtx()

		trap.setVerboseLevel(8)

		print(self.config.conf["custom_actions"][0]["trap"]["config"])

		trap.init(['-i', self.config.conf["custom_actions"][0]["trap"]["config"]], 1, 1)

		# Receive message from trap interface specified in config file
		data = self.trap.recv()

		idea = json.loads(data)

		self.assertTrue(self.content["ID"], "e214d2d9-359b-443d-993d-3cc5637107a0")
