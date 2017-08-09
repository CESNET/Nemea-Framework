from .Action import Action

import json

class WardenAction(Action):
    def __init__(self, action, client):
        super(type(self), self).__init__(actionId = action["id"], actionType = "warden")
        if client is None:
            self.logger.warning("Warden Client not instantiated! No records will be send.")
        self.client = client

    def run(self, record):
        try:
            if self.client != None:
                self.client.sendEvents([json.dumps(record)])
            else:
                self.logger.warning("No event was sent because there is no Warden Client instance")
        except Exception as e:
            self.logger.error(e)

    def __del__(self):
        pass
