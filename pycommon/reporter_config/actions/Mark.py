from .Action import Action

from pynspect import jpath

class MarkAction(Action):
    def __init__(self, action):
        super(type(self), self).__init__(actionId = action["id"], actionType = "mark")
        # TODO: parse path according to mentat jpath
        self.path = action["mark"]["path"]
        self.value = action["mark"]["value"]

    def mark(self, record):
        return jpath.jpath_set(record, self.path, self.value)

    def run(self, record):
        super(type(self), self).run(record)
        return self.mark(record)

    def __str__(self):
        return "Path: " + self.path + ", Value: " + self.value + "\n"
