from .Action import Action

import json

class WardenAction(Action):
    def __init__(self, action, client):
        super(type(self), self).__init__(actionId = action["id"], actionType = "warden")
        if client is None:
            self.logger.warning("Warden Client not instantiated! No records will be send.")
        self.client = client
        self.err_printed = False

    def run(self, record):
        super(type(self), self).run(record)
        try:
            if self.client != None:
                self.client.sendEvents([record])
            else:
                if not self.err_printed:
                    self.logger.warning("No event was sent because there is no Warden Client instance")
                    self.err_printed = True
        except Exception as e:
            self.logger.error(e)

    def __del__(self):
        pass

    def __str__(self):
        return "WARDEN\n"

