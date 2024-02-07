import threading
import os
import copy
import logging

from pynspect.compilers import IDEAFilterCompiler
from pynspect.gparser import PynspectFilterParser

from .actions.Drop import DropAction, DropMsg
from .actions.Action import Action
from .Parser import Parser
from .AddressGroup import AddressGroup
from .Rule import Rule, clearCounters, STAT_KEYPREFIX

logger = logging.getLogger(__name__)

class RepeatTimer(threading.Timer):
    def run(self):
        while not self.finished.wait(self.interval):
            self.function(*self.args,**self.kwargs)

class Config():
    """
    Configuration loaded from the file and command line arguments that tunes
    functionality of the reporter module.

    The class provides loading/reloading config, matching rules from config,
    counters update, printing loaded config.
    """

    def __init__(self, path, dry = False, trap = None, wardenargs = None, module_name = "reporter", autoreload = 0, use_namespace = True):
        """
        :param path Path to Yaml configuration file

        :param dry "Don't run yet" switch to parse but not execute/init

        :param trap Instance of TRAP client used in TrapAction

        :param wardenargs Arguments to initiate Warden Client used in WardenAction

        :param module_name Name usually given to Run() by the reporter's
                           developer, can be related to the name of script

        :param autoreload Timeout in seconds to perform automatic checking and
                          reload of config file, default is 0 to disable this feature.

        :param use_namespace Use generated name created from namespace (from
                             config) and module_name; if use_namespace is set to False, user
                             overrided the name by -n and only the module_name is used.
        """

        self.compiler = IDEAFilterCompiler()
        self.parser = PynspectFilterParser()
        self.parser.build()
        self.trap = trap
        self.autoreload = autoreload
        self.path = path
        self.configdir = os.path.dirname(path)
        self.conf = None
        self.config_mtime = 0
        self.module_name = module_name
        self.use_namespace = use_namespace

        self.timer = None
        self.wardenclient = None
        self.addrGroups = dict()
        self.actions = dict()
        self.rules = list()

        try:
            self.loadConfig()
        except (SyntaxError, LookupError) as e:
            logger.error("Error: Loading configuration file failed: %s " % str(e))

        if not self.conf:
            raise ImportError("Loading YAML file (%s) failed. Isn't it empty?" % path)

        # Configuration was succsesfuly loaded, it is possible to continue with init.
        if not dry:
            logger.info("Subscription to configuration changes.")
            if self.autoreload > 0:
                self.timer = RepeatTimer(self.autoreload, self.checkConfigChanges)
                self.timer.start()

        if wardenargs:
            try:
                import warden_client
            except:
                logger.error("Loading warden_client module failed.  Install it or remove '--warden' from the module's arguments.")
                raise ImportError("Warden client module could not be imported.")

            config = warden_client.read_cfg(wardenargs)
            config['name'] = self.name
            self.wardenclient = warden_client.Client(**config)

        # update modification time of loaded config, this timestamp is used checkConfigChanges()
        self.config_mtime = os.stat(self.path).st_mtime

    def __del__(self):
        logger.warning("Freeing configuration...")
        if self.timer:
            logger.info("Stopping configuration autoreload.")
            self.timer.cancel()

        if self.wardenclient:
            self.wardenclient.close()

    def printConfig(self, signum = -1, frame = None):
        """Print current configuration, this is used as a signal handler for SIGUSR2."""
        print(str(self))

    def checkConfigChanges(self, signum = -1, frame = None):
        """
        Check if the configuration file was modified (time of modification is
        newer); try to reload it using self.loadConfig().

        This method is called as a signal handler (SIGUSR1) or by the timer (self.timer).
        """
        if signum != -1:
            logger.warning(f"Received signal {signum}.")

        logger.debug("Checking for changes in configuration.")
        mtime = os.stat(self.path).st_mtime
        if mtime <= self.config_mtime:
            logger.debug("Skipping configuration reload, we have newer version of the file.")
            return

        self.config_mtime = mtime
        try:
            self.loadConfig()
        except (SyntaxError, LookupError) as e:
            logger.error("Loading new configuration failed due to error(s), continue with the old one. " + str(e))

    def loadConfig(self):
        """
        Load new configuration and when everything is ok, replace the previous
        one. When the new configuration contains errors, keep the previous one.

        Raises: SyntaxError, LookupError
        """
        logger.warning("Loading new configuration.")

        conf = Parser(self.path)

        if not conf or not conf.config:
            raise SyntaxError("Yaml parsing error: " + str(e))

        addrGroups = dict()
        smtp_conns = dict()
        actions = dict()
        rules = list()

        if "namespace" not in conf:
            logger.error("ERROR: 'namespace' is required but is missing. Please specify 'namespace' in YAML config to identify names of reporters (e.g., com.example.collectornemea).")

        # Create all address groups if there are any
        if "addressgroups" in conf:
            for i in conf["addressgroups"]:
                addrGroups[i["id"]] = AddressGroup(i)


        # Check if "smtp_connections" exists when there is some "email" action in "custom_actions"
        if ("custom_actions" in conf
                and any([i.__contains__("email") for i in conf["custom_actions"]])
                and "smtp_connections" not in conf):
            raise LookupError("'smtp_connections' is required when there is at least one 'email' action but it is missing in YAML config. Check your YAML config.")

        # Parse parameters for all smtp connections
        if "smtp_connections" in conf:
            for i in conf["smtp_connections"]:
                smtp_conns[i["id"]] = i

        # Parse and instantiate all custom actions
        if "custom_actions" in conf:
            for i in conf["custom_actions"]:
                if "mark" in i:
                    from .actions.Mark import MarkAction
                    actions[i["id"]] = MarkAction(i)

                elif "mongo" in i:
                    from .actions.Mongo import MongoAction
                    actions[i["id"]] =  MongoAction(i)

                elif "email" in i:
                    from .actions.Email import EmailAction
                    actions[i["id"]] = EmailAction(i, smtp_conns[i['email']['smtp_connection']])

                elif "file" in i:
                    from .actions.File import FileAction
                    actions[i["id"]] = FileAction(i)

                elif "syslog" in i:
                    from .actions.Syslog import SyslogAction
                    actions[i["id"]] = SyslogAction(i)

                elif "warden" in i:
                    """
                    Pass Warden Client instance to the Warden action
                    """
                    if not self.wardenclient:
                        raise SyntaxError("Cannot use warden action if --warden argument was not provided.")

                    from .actions.Warden import WardenAction
                    actions[i["id"]] = WardenAction(i, self.wardenclient)

                elif "trap" in i:
                    """
                    Pass TRAP context instance to the TRAP action
                    """
                    from .actions.Trap import TrapAction
                    actions[i["id"]] = TrapAction(i, self.trap)

                elif "drop" in i:
                    logger.warning("Drop action mustn't be specified in custom_actions!")
                    continue

                else:
                    raise SyntaxError("Undefined action: " + str(i))

        actions["drop"] = DropAction()

        # Parse all rules and match them with actions and address groups
        # There must be at least one rule (mandatory field)
        if "rules" in conf:
            if conf["rules"]:
                for i in conf["rules"]:
                    r = Rule(i, actions, addrGroups,
                                           parser = self.parser, compiler = self.compiler,
                                           module_name = self.module_name)
                    rules.append(r)
            if not rules:
                raise SyntaxError("YAML file should contain at least one `rule` in `rules`.")
        else:
            raise SyntaxError("YAML file must contain `rules`.")

        self.conf = conf
        self.rules = rules
        self.actions = actions
        self.addrGroups = addrGroups
        self.smtp_conns = smtp_conns

        if self.use_namespace:

            self.name = ".".join([conf.get("namespace", "com.example"), self.module_name])
        else:
            self.name = self.module_name

        clearCounters(STAT_KEYPREFIX, self.module_name)
        logging.warning("Success: New configuration loaded, applied and counters reset.")


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
        except KeyError:
            return 30

    def __str__(self):
        smtp = dict()
        for smtp_id, conns in self.smtp_conns.items():
            smtp[smtp_id] = "\n".join(["\t{}: {}".format(param, val) for param, val in conns.items()])

        s = "\n".join("{}:\n{}\n".format(smtp_id, params) for smtp_id, params in smtp.items())
        ag = "\n".join([str(self.addrGroups[key]) for key in self.addrGroups])
        a = "\n".join([key + ":\n\t" + str(self.actions[key]) for key in self.actions])
        r = "\n".join([str(val) for val in self.rules])
        string = "Namespace: {0}\n----------------\nSmtp connections:\n{1}\n----------------\nAddress Groups:\n{2}\n"\
                 "----------------\nCustom Actions:\n{3}\n----------------\nRules:\n{4}\n".format(self.conf.get("namespace"), s, ag, a, r)
        return string

