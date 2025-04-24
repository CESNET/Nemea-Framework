import re
import logging
import redis
from ransack import Parser, Filter
from idea import lite

logger = logging.getLogger(__name__)

STAT_KEYPREFIX = "nemea"

redis_con = redis.StrictRedis()

def UpdateCounter(prefix, module, ruleid, result, actionid, actiontype):
    """Increment counter in redis, key is composed from:

    prefix - string identifying all redis keys (NEMEA)
    module - name of reporter
    ruleid - identifier of rule from config
    result - True / False
    actionid - identifier of custom action from config
    actiontype - type of the action
    """

    try:
        if redis_con:
            key = f"{prefix}|{module}|{ruleid}|{result}|{actionid}|{actiontype}"
            redis_con.incr(key)
    except redis.exceptions.ConnectionError:
        logging.error("redis: Could not update statistics.")

def clearCounters(prefix, module):
    try:
        if redis_con:
            key = f"{prefix}|{module}|*"
            for i in redis_con.keys(key):
                redis_con.delete(i)
    except redis.exceptions.ConnectionError:
        logging.error("redis: Could not update statistics.")

class Rule():
    def __init__(self, rule, actions, parser=None, module_name=""):

        if not "condition" in rule:
            raise SyntaxError("Missing 'condition' in the rule: " + str(rule))

        if not "id" in rule:
            raise SyntaxError("Missing 'id' in the rule: " + str(rule))

        # Check if we got parser instance
        if parser is None:
            self.parser = Parser()
        else:
            self.parser = parser

        # Instantiate filter
        self.__filter = Filter()


        # Store rule condition in raw form
        self.__conditionRaw = rule["condition"]

        self.module_name = module_name
        # Set inner rule ID
        self.id = rule["id"]

        # Store the parsed condition
        self.__condition = self.__conditionRaw
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
                    raise SyntaxError("Missing action with ID %s" % str(e))

        # Associate elseactions
        if "elseactions" in rule:
            for actionId in rule["elseactions"]:
                try:
                    self.__elseactions.append(actions[actionId])
                except KeyError as e:
                    raise SyntaxError("Missing elseaction with ID %s" % str(e))


    def parseRule(self):
        cond = str(self.__condition).lower()
        if cond in ["none", "null", "true"]:
            self.__condition = True
        elif cond == "false":
            self.__condition = False
        else:
            try:
                self.__condition = self.parser.parse(self.__condition)
            except Exception as e:
                raise SyntaxError("Error while parsing condition: {0}\nOriginal exception: {1}".format(self.__condition, e))

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
            res = self.__filter.eval(self.__condition, record)

        logger.debug("RESULT: %s", res)

        return res

    def actions(self, record):
        for action in self.__actions:
            UpdateCounter(STAT_KEYPREFIX, self.module_name, self.id, True, action.actionId, action.actionType)
            action.run(record)

    def elseactions(self, record):
        for action in self.__elseactions:
            UpdateCounter(STAT_KEYPREFIX, self.module_name, self.id, False, action.actionId, action.actionType)
            action.run(record)


    def __repr__(self):
        return self.__conditionRaw

    def __str__(self):
        actions = []
        for i in self.__actions:
            key = "{prefix}|{module}|{ruleid}|{result}|{actionid}|{actiontype}".format(prefix=STAT_KEYPREFIX,
                                                                                       module=self.module_name, ruleid=self.id,
                                                                                       result="True", actionid=i.actionId,
                                                                                       actiontype=i.actionType)
            try:
                cnt = redis_con.get(key)
                if cnt:
                    cnt = cnt.decode("utf-8")
                else:
                    cnt = 0
            except redis.exceptions.ConnectionError:
                logging.error("redis: Could not retrieve statistics.")
                cnt = 0

            actions.append(f"{i.actionId} ({i.actionType}) [{cnt}x]")
        elseactions = []
        for i in self.__elseactions:
            elseactions.append("{0} ({1})".format(i.actionId, i.actionType))

        return "{0}: {1}\n{2}{3}".format(self.id, " ".join([i.strip() for i in str(self.__conditionRaw).split("\n")]),
                                         "\tActions: " + (", ".join(actions)) + "\n" if actions else "",
                                         "\tElse Actions: " + (", ".join(elseactions)) + "\n" if elseactions else "")

    def rule(self):
        return str(self.__condition)

