from .Action import Action

class DropMsg(Exception):
    def __init__(self):
        super(DropMsg, self).__init__()

class DropAction(Action):
    def __init__(self):
        # Should import smtp lib here
        self.actionId = "drop"

    def run(self, record):
        raise DropMsg()
