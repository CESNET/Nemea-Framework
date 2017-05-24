class Action():
    actionId = None
    actionType = None

    def run(self, record):
        raise Exception("Run method not implemented")

