from .Action import Action

from pynspect import jpath

class MarkAction(Action):
    def __init__(self, action):
        self.actionId = action["id"]
        # TODO: parse path according to mentat jpath
        self.path = action["mark"]["path"]
        self.value = action["mark"]["value"]

    def mark(self, msg):
        return jpath.jpath_set(msg, self.path, self.value)

    def run(self, msg):
        return self.mark(msg)
