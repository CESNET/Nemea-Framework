from .Action import Action

class EmailAction(Action):
    def __init__(self, action):
        # Should import smtp lib here
        self.actionId = action["id"]
        self.to = action["to"]
        self.subject = action["subject"]

    def run(self, record):
        pass
