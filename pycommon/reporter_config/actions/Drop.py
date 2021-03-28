from .Action import Action

class DropMsg(Exception):
    def __init__(self):
        super(type(self), self).__init__()

class DropAction(Action):
    def __init__(self):
        super(DropAction, self).__init__(actionId = "drop", actionType = "drop")

    def run(self, record):
        super(type(self), self).run(record)
        raise DropMsg()

    def __str__(self):
        return "DROP\n"

