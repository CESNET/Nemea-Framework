import unittest
import os

from reporter_config.Config import Config

class RCBaseTest(unittest.TestCase):

	def setUp(self):
		"""
		Example message created by a conv function in a reporter
		"""
		self.msg = {
			"ID" : "e214d2d9-359b-443d-993d-3cc5637107a0",
			"WinEndTime" : "2016-06-21T11:25:01Z",
			"ConnCount" : 2,
			"Source" : [
				{
					"IP4" : [
						"1.2.3.4"
					]
				}
			],
			"Format" : "IDEA0",
			"WinStartTime" : "2016-06-21T11:20:01Z",
			"_CESNET" : {
				"StorageTime" : 1466508305
			},
			"Target" : [
				{
					"IP4" : [
						"195.113.165.128/25"
					],
					"Port" : [
						"22"
					],
					"Proto" : [
						"tcp",
						"ssh"
					],
					"Anonymised" : True
				}
			],
			"Note" : "SSH login attempt",
			"DetectTime" : "2016-06-21T13:08:27Z",
			"Node" : [
				{
					"Name" : "cz.cesnet.mentat.warden_filer",
					"Type" : [
						"Relay"
					]
				},
				{
					"AggrWin" : "00:05:00",
					"Type" : [
						"Connection",
						"Honeypot",
						"Recon"
					],
					"SW" : [
						"Kippo"
					],
					"Name" : "cz.uhk.apate.cowrie"
				}
			],
			"Category" : [
				"Attempt.Login"
			]
		}

	def tearDown(self):
		pass

	def test_02_basic_match(self):
		self.config = Config(os.path.dirname(__file__) + '/rc_config/tautology.yaml');

		self.config.match(self.msg)

		self.assertEqual(self.msg['Test'], True)

