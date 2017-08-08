import copy
from idea import lite
from pynspect.rules import *
from pynspect.filters import IDEAFilterCompiler, DataObjectFilter
from pynspect.gparser import MentatFilterParser

from .actions.Drop import DropAction, DropMsg
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

    def __init__(self, path, trap = None, warden = None):
        """
        :param path Path to YAML configuration file

        :param trap Instance of TRAP client used in TrapAction

        :param warden Instance of Warden Client used in WardenAction
        """
        # Parse given config file
        with open(path, 'r') as f:
            self.conf = Parser(f)

        if not self.conf:
            raise Exception("Loading YAML file ({0}) failed. Isn't it empty?".format(path))

        # Build parser
        self.parser = MentatFilterParser()
        self.parser.build()

        self.compiler = IDEAFilterCompiler()

        self.addrGroups = dict()

        # Create all address groups if there are any
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
                    """
                    Pass Warden Client instance to the Warden action
                    """
                    from .actions.Warden import WardenAction
                    self.actions[i["id"]] = WardenAction(i, warden)

                elif "trap" in i:
                    """
                    Pass TRAP context instance to the TRAP action
                    """
                    from .actions.Trap import TrapAction
                    self.actions[i["id"]] = TrapAction(i, trap)

                elif "drop" in i:
                    logger.warning("Drop action mustn't be specified in custom_actions!")
                    continue

                else:
                    raise Exception("Undefined action: " + str(i))

        self.actions["drop"] = DropAction()

        self.rules = list()

        # Parse all rules and match them with actions and address groups
        # There must be at least one rule (mandatory field)
        if "rules" in self.conf:
            if self.conf["rules"]:
                for i in self.conf["rules"]:
                    self.rules.append(Rule(i
                            , self.actions
                            , self.addrGroups
                            , parser = self.parser
                            , compiler = self.compiler
                            ))
            if not self.rules:
                raise Exception("YAML file should contain at least one `rule` in `rules`.")
        else:
            raise Exception("YAML file must contain `rules`.")

    def match(self, msg):
        """
        Check if msg matches rules from config file.

        Return a list of bool values representing results of all tested rules.
        """
        results = []

        try:
            for rule in self.rules:
                res = rule.filter(msg)
                #logger.debug("Filter by rule: %s \n message: %s\n\nresult: %s", rule, msg, res)
                #print("Filter by rule: %s \n message: %s\n\nresult: %s" % (rule.rule(), msg, res))
                results.append(res)

                tmp_msg = copy.deepcopy(msg)
                if res:
                    # Perform actions on given message
                    rule.actions(tmp_msg)
                    logger.info("action running")
                else:
                    rule.elseactions(tmp_msg)
                    logger.info("else action running")
        except DropMsg:
            # This exception breaks the processing of rule list.
            pass
        return results

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
