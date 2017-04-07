import Action
import sys

class FileAction(Action.Action):

    def __init__(self, action):
        self.actionId = action["id"]
        self.actionType = "file"
        self.path = action["path"]
        self.file = os.path.isfile(self.path)

        if self.path == '-':
            self.fileHandle = sys.stdout
            self.file = True
        elif self.file:
            self.fileHandle = open(self.path, "a")
        else:
            self.fileHandle = None

    def run(self, record):
        if self.fileHandle:
            self.fileHandle.write(json.dumps(record) + '\n')

        else:
            with open(self.path) as f:
                pass

    def __del__(self):
        if self.fileHandle and self.fileHandle != sys.stdout:
            self.fileHandle.close()

