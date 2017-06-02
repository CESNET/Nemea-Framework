import unittest
import os
import json
import pymongo

from reporter_config.Config import Config

class RCMongoTest(unittest.TestCase):

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
		self.collection.drop()
		self.client.close()

	def test_01_store_record(self):
		"""
		Load mongo.yaml configuration file, parse it and analyze it

		This shouldn't rise any exceptions

		Should store message in DB and test if there is one record
		"""
		self.config = Config(os.path.dirname(__file__) + '/rc_config/mongo.yaml');

		self.assertNotEqual(self.config, None)
		self.config.match(self.msg)

		self.assertEqual(self.collection.find().count(), 1)

	def test_02_check_record(self):
		"""
		Load mongo.yaml configuration file, parse it and analyze it

		This shouldn't rise any exceptions

		Should store message in DB, find it, and check contents
		"""
		self.config = Config(os.path.dirname(__file__) + '/rc_config/mongo.yaml');

		self.assertNotEqual(self.config, None)
		self.config.match(self.msg)

		rec = self.collection.find_one()

		self.assertEqual(rec["ID"], "e214d2d9-359b-443d-993d-3cc5637107a0")

