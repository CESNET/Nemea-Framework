from .Action import Action

import pytrap
import json

class TrapAction(Action):
    def __init__(self, action, trap = None):
        super(type(self), self).__init__(actionId = action["id"], actionType = "trap")

        if trap == None:
            # Initialize TRAP interface
            self.trap = pytrap.TrapCtx()

            # Only output interface is needed
            self.trap.init(['-i', action["trap"]["config"]], 0, 1)

            # We must set output format to JSON with ID IDEA
            self.trap.setDataFmt(0, pytrap.FMT_JSON, "IDEA")
        else:
            self.trap = trap

    def run(self, record):
        super(type(self), self).run(record)
        try:
            self.trap.send(json.dumps(record).encode('utf8'), 0)
        except Exception as e:
            self.logger.error(e)

    def __del__(self):
        pass

    def __str__(self):
        return "TRAP\n"

