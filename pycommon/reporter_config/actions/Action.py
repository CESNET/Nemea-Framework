import logging as log

class Action():
    actionId = None
    actionType = None
    def __init__(self):
        self.logger = log.getLogger(__name__)

    def run(self, record):
        raise Exception("Run method not implemented")

