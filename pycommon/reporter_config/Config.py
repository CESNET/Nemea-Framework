from idea import lite
from pynspect.rules import *
from pynspect.filters import IDEAFilterCompiler, DataObjectFilter
from pynspect.gparser import MentatFilterParser

from .actions.Drop import DropAction
from .Parser import Parser
from .AddressGroup import AddressGroup
from .Rule import Rule

import logging

logger = logging.getLogger(__name__)

class Config():

	addrGroups = dict()
	actions = dict()
	rules = dict()
	parser = None
	compiler = None

	def __init__(self, path, trap = None):
		# Parse given config gile
		with open(path, 'r') as f:
			self.conf = Parser(f)

		# Build parser
		self.parser = MentatFilterParser()
		self.parser.build()

		self.compiler = IDEAFilterCompiler()

		self.addrGroups = dict()

		# Create all address groups
		for i in self.conf["addressgroups"]:
			self.addrGroups[i["id"]] = AddressGroup(i)

		self.actions = dict()

		# Parse all custom actions
		for i in self.conf["custom_actions"]:
			if i["type"] == "mark":
				from .actions.Mark import MarkAction
				self.actions[i["id"]] = MarkAction(i)

			elif i["type"] == "mongo":
				from .actions.Mongo import MongoAction
				self.actions[i["id"]] =  MongoAction(i)

			elif i["type"] == "email":
				from .actions.Email import EmailAction
				self.actions[i["id"]] = EmailAction(i)

			elif i["type"] == "file":
				from .actions.File import FileAction
				self.actions[i["id"]] = FileAction(i)

			elif i["type"] == "warden":
				from actions.Warden import WardenAction
				self.actions[i["id"]] = WardenAction(i)

			elif i["type"] == "trap":
				from .actions.Trap import TrapAction
				self.actions[i["id"]] = TrapAction(i, trap)

			else:
				raise Exception("undefined action: " + str(i))

		self.actions["drop"] = DropAction()

		self.rules = dict()

		# Parse all rules and match them with actions and address groups
		for i in self.conf["rules"]:
			self.rules[i["id"]] = Rule(i
					, self.actions
					, self.addrGroups
					, parser = self.parser
					, compiler = self.compiler
					)

	def match(self, msg):
		tmp_msg = msg

		for i in self.rules:
			res = self.rules[i].filter(msg)
			#logger.debug("Filter by rule: %s \n message: %s\n\nresult: %s", self.rules[i].rule(), msg, res)

			if res:
				# Perform actions on given message
				tmp_msg = self.rules[i].actions(tmp_msg)
				logger.info("action running")
			else:
				tmp_msg = self.rules[i].elseactions(tmp_msg)
				logger.info("else action running")

	def loglevel(self):
		"""Get logging level

		CRITICAL	50
		ERROR		40
		WARNING		30
		INFO		20
		DEBUG		10
		NOTSET		 0
		"""
		try:
			return (self.conf["reporter"]["loglevel"]) * 10
		except:
			return 30

if __name__ == "__main__":
	"""Run basic tests
	"""
