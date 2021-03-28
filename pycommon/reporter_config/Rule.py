from pynspect.rules import *
from pynspect.filters import DataObjectFilter
from pynspect.compilers import IDEAFilterCompiler
from pynspect.gparser import PynspectFilterParser
from idea import lite
import re

import logging

logger = logging.getLogger(__name__)

class Rule():
    def __init__(self, rule, actions, addrGroups, parser=None, compiler=None):
        # Check is we got parser instance
        if parser is None:
            self.parser = PynspectFilterParser()
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
        if "actions" in rule:
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
        cond = str(self.__condition).lower()
        if cond in ["none", "null", "true"]:
            self.__condition = True
        elif cond == "false":
            self.__condition = False
        else:
            try:
                self.__condition = self.parser.parse(self.__condition)
                self.__condition = self.compiler.compile(self.__condition)
            except Exception as e:
                print("Error while parsing condition: {0}\nOriginal exception: {1}".format(self.__condition, e))

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

        if self.__condition == None or self.__condition == True:
            # Rule condition is empty (tautology) - should always match the record
            res = True
        elif self.__condition == False:
            res = False
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

    def __str__(self):
        actions = []
        for i in self.__actions:
            actions.append("{0} ({1})".format(i.actionId, i.actionType))
        elseactions = []
        for i in self.__elseactions:
            elseactions.append("{0} ({1})".format(i.actionId, i.actionType))

        return "{0}: {1}\n{2}{3}".format(self.id, " ".join([i.strip() for i in str(self.__conditionRaw).split("\n")]),
                                         "\tActions: " + (", ".join(actions)) + "\n" if actions else "",
                                         "\tElse Actions: " + (", ".join(elseactions)) + "\n" if elseactions else "")

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
        if rule == None or isinstance(rule, bool):
            return False

        for key in addrGroups:
            if key in rule:
                rule = re.sub(r"\b{0}\b".format(re.escape(key)), addrGroups[key].iplist(), rule)
                matched = True
        self.__condition = rule

        return matched

