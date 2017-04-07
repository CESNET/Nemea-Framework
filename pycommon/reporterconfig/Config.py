from idea import lite
from mentat.filtering.rules import *
from mentat.filtering.filters import IDEAFilterCompiler, DataObjectFilter
from mentat.filtering.gparser import MentatFilterParser

from Parser import Parser
from AddressGroup import AddressGroup
from actions import *
from Rule import Rule

import logging

logger = logging.getLogger(__name__)

class Config():
    """
    Reporter Configuration Module

    Author: Petr Stehlik <stehlik@cesnet.cz>
    Copyright (C) 2017 CESNET, z.s.p.o.

    Acknowledgements: Mentat team (namely Jan Mach) for their amazing work
    on Mentat filtering module.

    This module takes care of advanced filtering of IDEA messages.

    The configuration is quite complex and is detailed in the README file (TBD).

    """

    addrGroups = dict()
    actions = dict()
    rules = dict()
    parser = None
    compiler = None

    def __init__(self, path, trap = None):
        # Parse config gile
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
                self.actions[i["id"]] = MarkAction(i)

            elif i["type"] == "trap":
                self.actions[i["id"]] = TrapAction(i)

            elif i["type"] == "mongo":
                self.actions[i["id"]] =  MongoAction(i)

            elif i["type"] == "email":
                self.actions[i["id"]] = EmailAction(i)

            elif i["type"] == "file":
                self.actions[i["id"]] = FileAction(i)

            elif i["type"] == "warden":
                self.actions[i["id"]] = WardenAction(i)

            elif i["type"] == "trap":
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
        """Try to match the message with one of the rules.
        If the rule has elseactions defined it will be performed in case of
        not finding the match.

        All rules are always applied and are dependent on each other
        in given order.
        """
        tmp_msg = msg

        for i in self.rules:
            res = self.rules[i].filter(msg)

            if res:
                # Perform actions on given message
                tmp_msg = self.rules[i].actions(tmp_msg)
                logger.info("action running")
            else:
                tmp_msg = self.rules[i].elseactions(tmp_msg)
                logger.info("else action running")

if __name__ == "__main__":
    """Run basic tests
    """
