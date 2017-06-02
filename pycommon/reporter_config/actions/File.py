from .Action import Action

import sys
import os
import json
import logging as log

logger = log.getLogger(__name__)

class FileAction(Action):
	"""Store IDEA message in a file
	Depending on the given file path the messages can be stored in:
		* FILE: a single file with new messages appended to it
		* DIR: each message in separate file where filename is the ID of the message
		  with .idea extension
		* STDOUT: message is written to STDOUT pipe
	"""
	def __init__(self, action):
		self.actionId = action["id"]
		self.actionType = "file"
		self.path = action["file"]["path"]
		self.dir = action["file"]["dir"] if "dir" in action["file"] else False

		if self.path == '-':
			self.fileHandle = sys.stdout
			self.dir = True
		elif not self.dir:
			# Open file if dir is not specified
			self.fileHandle = open(self.path, "a")
		else:
			self.fileHandle = None
			if not os.path.exists(self.path):
				os.mkdir(self.path, 777)

	def run(self, record):
		if self.fileHandle:
			self.fileHandle.write(json.dumps(record) + '\n')

		else:
			"""Store record in separate file
			"""
			with open(os.path.join(self.path, record["ID"] + ".idea"), "w") as f:
				f.write(json.dumps(record))

	def __del__(self):
		if self.fileHandle:
			if self.fileHandle != sys.stdout:
				self.fileHandle.close()

