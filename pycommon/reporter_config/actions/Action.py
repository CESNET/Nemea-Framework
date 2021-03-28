import logging as log

class Action(object):
    actionLog = []
    def __init__(self, actionId = None, actionType = None):
        super(Action, self).__init__()
        self.logger = log.getLogger(__name__)
        self.actionId = actionId
        self.actionType = actionType

    def run(self, record):
        self.actionLog.append(self)

