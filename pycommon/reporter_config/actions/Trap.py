import Action
import pytrap
import logging

logger = logging.getLogger(__name__)

class TrapAction(Action.Action):
    def __init__(self, action, trap):
        self.actionId = action["id"]
        self.actionType = "trap"
        self.trap = trap

        # We must set output format to JSON with ID IDEA
        self.trap.setDataFmt(0, pytrap.FMT_JSON, "IDEA")

    def run(self, record):
        self.trap.send(json.dumps(idea), 0)

    def __del__(self):
        pass

