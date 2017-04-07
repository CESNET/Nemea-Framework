import Action
import warden_client
import logging

logger = logging.getLogger(__name__)

class WardenAction(Action.Action):
    def __init__(self, action, name):
        self.actionId = action["id"]
        self.actionType = "warden"
        self.file = action["warden"]["configfile"]
        self.name = name

        try:
            config = warden_client.read(self.file)
            config["name"] = self.name
            self.client = warden_client.Client(**config)
        except ValueError as e:
            logger.error("Failed to load Warden config file '%s'\n%s\n", self.file, e)
            exit(1)

    def run(self, record):
        self.client.sendEvents([record])

    def __del__(self):
        self.client.close()

