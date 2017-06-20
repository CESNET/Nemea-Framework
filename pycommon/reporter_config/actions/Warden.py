from .Action import Action

import warden_client
import logging
import json

logger = logging.getLogger(__name__)

class WardenAction(Action):
    def __init__(self, action):
        self.actionId = action["id"]
        self.actionType = "warden"
        self.file = action["warden"]["configfile"]

        try:
            self.client = warden_client.Client(**warden_client.read_cfg(self.file))
        except ValueError as e:
            logger.error("Failed to load Warden config file '%s'\n%s\n", self.file, e)
            exit(1)

    def run(self, record):
        self.client.sendEvents([json.dumps(record)])

    def __del__(self):
        self.client.close()

