from pynspect.rules import *
from pynspect.filters import IDEAFilterCompiler, DataObjectFilter
from pynspect.gparser import MentatFilterParser
from idea import lite

import logging

logger = logging.getLogger(__name__)

class Rule():
	def __init__(self, rule, actions, addrGroups, parser=None, compiler=None):
		# Check is we got parser instance
		if parser is None:
			self.parser = MentatFilterParser()
			self.parser.build()
		else:
			self.parser = parser

		# Check is we got compiler instance
		if compiler is None:
			self.compiler = IDEAFilterCompiler()
		else:
			self.compiler = compiler

		# Instantiate filter
		self.__filter = DataObjectFilter()

		# Store rule condition in raw form
		self.__conditionRaw = rule["condition"]

		if not self.__matchvar(rule["condition"], addrGroups):
			self.__condition = self.__conditionRaw

		# Set inner rule ID
		self.id = rule["id"]

		if (self.__condition != None):
			self.parseRule()

		self.__actions = list()
		self.__elseactions = list()

		# Associate actions
		for actionId in rule["actions"]:
			try:
				logger.debug("Rule %s inserting %s", self.id, actionId)
				self.__actions.append(actions[actionId])
			except KeyError as e:
				raise Exception("Missing action with ID " + str(e))

		# Associate elseactions
		if "elseactions" in rule:
			for actionId in rule["elseactions"]:
				try:
					self.__elseactions.append(actions[actionId])
				except KeyError as e:
					raise Exception("Missing elseaction with ID " + str(e))


	def parseRule(self):
		self.__condition = self.parser.parse(self.__condition)
		self.__condition = self.compiler.compile(self.__condition)

	def filter(self, record):
		"""
		Filter given record based on rule's condition

		@note If the rule's condition is empty, record is always matched.

		@return Boolean
		"""
		# The message must be converted via idea module
		if not isinstance(record, lite.Idea):
			logger.info("Converting message to IDEA")
			record = lite.Idea(record)

		logger.debug(record)
		logger.debug(self.__condition)

		if self.__condition == None:
			# Rule condition is empty (tautology) - should always match the record
			res = True
		else:
			# Match the record with non-empty rule's condition
			res = self.__filter.filter(self.__condition, record)

		logger.debug("RESULT: %s", res)

		return res

	def actions(self, record):
		for action in self.__actions:
			action.run(record)

	def elseactions(self, record):
		for action in self.__elseactions:
			action.run(record)

	def __repr__(self):
		return self.__conditionRaw

	def rule(self):
		return str(self.__condition)

	def __matchvar(self, rule, addrGroups):
		"""
		Since pyncspect doesn't support variables yet we had to provide
		a solution where every address group's name is matched against
		the rule's condition and eventually replaced with address group's values
		"""

		matched = False

		"""
		Tautology - empty rule should always match
		Don't try to match or replace any address group
		"""
		if rule == None:
			return False

		for key in addrGroups:
			if key in rule:
				self.__condition = rule.replace(key, addrGroups[key].iplist())
				matched = True

		return matched

