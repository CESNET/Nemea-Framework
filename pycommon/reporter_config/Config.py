import copy
from idea import lite
from pynspect.rules import *
from pynspect.filters import DataObjectFilter
from pynspect.compilers import IDEAFilterCompiler
from pynspect.gparser import PynspectFilterParser

from .actions.Drop import DropAction, DropMsg
from .actions.Action import Action
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
        self.parser = PynspectFilterParser()
        self.parser.build()

        self.compiler = IDEAFilterCompiler()

        self.addrGroups = dict()

        # Create all address groups if there are any
        if "addressgroups" in self.conf:
            for i in self.conf["addressgroups"]:
                self.addrGroups[i["id"]] = AddressGroup(i)

        self.smtp_conns = dict()

        # Check if "smtp_connections" exists when there is some "email" action in "custom_actions"
        if ("custom_actions" in self.conf
                and any([i.__contains__("email") for i in self.conf["custom_actions"]])
                and "smtp_connections" not in self.conf):
            raise LookupError("'smtp_connections' is required when there is at least one 'email' action but it is missing in YAML config. Check your YAML config.")

        # Parse parameters for all smtp connections
        if "smtp_connections" in self.conf:
            for i in self.conf["smtp_connections"]:
                self.smtp_conns[i["id"]] = i

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
                    self.actions[i["id"]] = EmailAction(i, self.smtp_conns[i['email']['smtp_connection']])

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
        actionsDone = []

        try:
            for rule in self.rules:
                self.clearActionLog()
                res = rule.filter(msg)
                #logger.debug("Filter by rule: %s \n message: %s\n\nresult: %s", rule, msg, res)
                #print("Filter by rule: %s \n message: %s\n\nresult: %s" % (rule.rule(), msg, res))

                results.append(res)

                tmp_msg = copy.deepcopy(msg)
                if res:
                    # condition is True
                    rule.actions(tmp_msg)
                    logger.info("action running")
                else:
                    # condition is False
                    rule.elseactions(tmp_msg)
                    logger.info("else action running")
                actionsDone.append(self.getActionLog())
        except DropMsg:
            # This exception breaks the processing of rule list.
            pass
        return (results, actionsDone)

    def getActionLog(self):
        return Action.actionLog

    def clearActionLog(self):
        Action.actionLog = []

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

    def __str__(self):
        smtp = dict()
        for smtp_id, conns in self.smtp_conns.items():
            smtp[smtp_id] = "\n".join(["\t{}: {}".format(param, val) for param, val in conns.items()])

        s = "\n".join("{}:\n{}\n".format(smtp_id, params) for smtp_id, params in smtp.items())
        ag = "\n".join([str(self.addrGroups[key]) for key in self.addrGroups])
        a = "\n".join([key + ":\n\t" + str(self.actions[key]) for key in self.actions])
        r = "\n".join([str(val) for val in self.rules])
        string = "Smtp connections:\n{0}\n----------------\nAddress Groups:\n{1}\n----------------\n"\
                 "Custom Actions:\n{2}\n----------------\nRules:\n{3}\n".format(s, ag, a, r)
        return string

if __name__ == "__main__":
    """Run basic tests
    """
