import unittest
import os
import json

pymongo_missing = False
try:
    import pymongo
except:
    pymongo_missing = True

import logging

from reporter_config.Config import Config

class RCMultipleActionsTest(unittest.TestCase):

    def setUp(self):
        """
        Example message created by a conv function in a reporter
        """
        with open(os.path.dirname(__file__) + '/rc_msg.json', 'r') as f:
            self.msg = json.load(f)

        logging.basicConfig(level=50)
        self.client = pymongo.MongoClient("localhost", 27017)
        self.collection = self.client["rc_test"]["alerts"]

        # Remove the collection (just for assurance)
        self.collection.drop()

    def tearDown(self):
        # Remove created file
        os.remove("testfile.idea")

    @unittest.skipIf(pymongo_missing, "missing pymongo, skipping mongodb test with actions")
    def test_01_receive_message(self):
        """Perform multiple actions on matched message

        If an action is matched perform these actions:
            * Mark
            * Mongo
            * File

        Load multiple_actions.yaml configuration file, parse it and analyze it

        This shouldn't rise any exceptions
        """
        self.config = Config(os.path.dirname(__file__) + '/rc_config/multiple_actions.yaml');

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
