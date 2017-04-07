from mentat.filtering.rules import *
from mentat.filtering.filters import IDEAFilterCompiler, DataObjectFilter
from mentat.filtering.gparser import MentatFilterParser
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

        self.parseRule()

        self.__actions = dict()
        self.__elseactions = dict()

        # Associate actions
        for actionId in rule["actions"]:
            try:
                logger.debug("Rule %s inserting %s", self.id, actionId)
                self.__actions[actionId] = actions[actionId]
            except KeyError as e:
                raise Exception("Missing action with ID " + str(e))

        # Associate elseactions
        if "elseactions" in rule:
            for actionId in rule["elseactions"]:
                try:
                    self.__elseactions[actionId] = actions[actionId]
                except KeyError as e:
                    raise Exception("Missing elseaction with ID " + str(e))


    def parseRule(self):
        self.__condition = self.parser.parse(self.__condition)
        self.__condition = self.compiler.compile(self.__condition)

    def filter(self, record):
        # The message must be converted via idea module
        #logger.debug(record)
        if not isinstance(record, lite.Idea):
            logger.info("Converting message to IDEA")
            record = lite.Idea(record)
        
        logger.debug(record)
        logger.debug(self.__condition)
        res = self.__filter.filter(self.__condition, record)
        logger.debug("RESULT: %s", res)
        return res

    def actions(self, record):
        for action in self.__actions:
            self.__actions[action].run(record)

    def elseactions(self, record):
        for action in self.__elseactions:
            self.__elseactions[action].run(record)

    def __repr__(self):
        return self.__conditionRaw

    def rule(self):
        return str(self.__condition)

    def __matchvar(self, rule, addrGroups):
        matched = False

        for key in addrGroups:
            if key in rule:
                self.__condition = rule.replace(key, addrGroups[key].iplist())
                matched = True

        return matched

