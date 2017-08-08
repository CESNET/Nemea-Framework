from .Action import Action

import logging
import json

logger = logging.getLogger(__name__)

class WardenAction(Action):
    def __init__(self, action, client):
        self.actionId = action["id"]
        self.actionType = "warden"
        if client is None:
            logger.warning("Warden Client not instantiated! No records will be send.")
        self.client = client

    def run(self, record):
        if self.client != None:
            self.client.sendEvents([json.dumps(record)])
        else:
            logger.warning("No event was sent because there is no Warden Client instance")

    def __del__(self):
        pass
