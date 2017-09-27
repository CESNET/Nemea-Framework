from .Action import Action

import pytrap
import json

class TrapAction(Action):
    def __init__(self, action, trap = None):
        super(type(self), self).__init__(actionId = action["id"], actionType = "trap")

        self.trap = trap
        self.err_printed = False

    def run(self, record):
        super(type(self), self).run(record)
        if self.trap:
            try:
                self.trap.send(json.dumps(record).encode('utf8'), 0)
            except Exception as e:
                self.logger.error(e)
        elif not err_printed:
            self.logger.warning("Skipping TRAP action, TRAP is not initialized.")
            self.err_printed = True

    def __del__(self):
        pass

    def __str__(self):
        return "TRAP\n"

