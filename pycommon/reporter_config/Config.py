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
	rules = list()
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
		if "addressgroups" in self.conf:
			for i in self.conf["addressgroups"]:
				self.addrGroups[i["id"]] = AddressGroup(i)

		self.actions = dict()

		# Parse and instantiate all custom actions
		if "custom_actions" in self.conf:
			for i in self.conf["custom_actions"]:
				if "mark" in i:
					from .actions.Mark import MarkAction
					self.actions[i["id"]] = MarkAction(i)

				elif "mongo" in i:
					from .actions.Mongo import MongoAction
					self.actions[i["id"]] =  MongoAction(i)

				elif "email" in i:
					from .actions.Email import EmailAction
					self.actions[i["id"]] = EmailAction(i)

				elif "file" in i:
					from .actions.File import FileAction
					self.actions[i["id"]] = FileAction(i)

				elif "warden" in i:
					from .actions.Warden import WardenAction
					self.actions[i["id"]] = WardenAction(i)

				elif "trap" in i:
					from .actions.Trap import TrapAction
					self.actions[i["id"]] = TrapAction(i, trap)

				elif "drop" in i:
					logger.warning("Drop action musn't be specified in custom_actions!")
					continue

				else:
					raise Exception("undefined action: " + str(i))

		self.actions["drop"] = DropAction()

		self.rules = list()

		# Parse all rules and match them with actions and address groups
		# There must be at least one rule (mandatory field)
		for i in self.conf["rules"]:
			self.rules.append(Rule(i
					, self.actions
					, self.addrGroups
					, parser = self.parser
					, compiler = self.compiler
					))

	def match(self, msg):
		tmp_msg = msg

		for rule in self.rules:
			res = rule.filter(msg)
			#logger.debug("Filter by rule: %s \n message: %s\n\nresult: %s", self.rules[i].rule(), msg, res)

			if res:
				# Perform actions on given message
				rule.actions(tmp_msg)
				logger.info("action running")
			else:
				rule.elseactions(tmp_msg)
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
