import logging as log

class Action(object):
    def __init__(self, actionId = None, actionType = None):
        super(Action, self).__init__()
        self.logger = log.getLogger(__name__)

    def run(self, record):
        raise Exception("Run method not implemented")

