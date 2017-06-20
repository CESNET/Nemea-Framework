import unittest
import os
import json
import pymongo

import logging

from reporter_config.Config import Config

class RCMultipleActionsTest(unittest.TestCase):

	def setUp(self):
		"""
		Example message created by a conv function in a reporter
		"""
		with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
			self.msg = json.load(f)

		self.client = pymongo.MongoClient("localhost", 27017)
		self.collection = self.client["rc_test"]["alerts"]

		# Remove the collection (just for assurance)
		self.collection.drop()

	def tearDown(self):
		# Remove created file
		os.remove("testfile.idea")

	def test_01_receive_message(self):
		"""Perform multiple elseactions on matched message

		If an action is matched perform these actions:
			* Mark
			* Mongo
			* File

		Load multiple_elseactions.yaml configuration file, parse it and analyze it

		This shouldn't rise any exceptions, if action is performed, which it shouldn't,
		it raises the DropMsg exception (uncaught here)
		"""
		self.config = Config(os.path.dirname(__file__) + '/rc_config/multiple_elseactions.yaml');

		self.assertNotEqual(self.config, None)
		self.config.match(self.msg)

		# The actions must be checked in reversed order

		# Check if file created by File Action  exists
		self.assertTrue(os.path.exists("testfile.idea"), True)

		# Find the event in DB
		rec = self.collection.find_one()
		self.assertTrue(rec["ID"], "e214d2d9-359b-443d-993d-3cc5637107a0")

		# Check if message was marked
		self.assertEqual(rec['Test'], True)
