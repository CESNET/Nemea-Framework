import Action
import sys
import os
import json
import logging as log

logger = log.getLogger(__name__)

class FileAction(Action.Action):
    """Store IDEA message in a file
    Depending on the given file path the messages can be stored in a single file
    with new messages appended. Or it can store each message in separate file where
    filename is the ID of the message with .idea extension.
    """
    def __init__(self, action):
        self.actionId = action["id"]
        self.actionType = "file"
        self.path = action["path"]
        self.dir = action["dir"] if "dir" in action else False

        if self.path == '-':
            self.fileHandle = sys.stdout
            self.dir = True
        elif self.dir:
            self.fileHandle = open(self.path, "a")
        else:
            self.fileHandle = None
            if not os.path.exists(self.path):
                os.mkdir(self.path, 777)

    def run(self, record):
        if self.fileHandle:
            self.fileHandle.write(json.dumps(record) + '\n')

        else:
            """Store record in separate file
            """
            with open(os.path.join(self.path, record["ID"] + ".idea"), "w") as f:
                f.write(json.dumps(record))

    def __del__(self):
        if self.fileHandle and self.fileHandle != sys.stdout:
            self.fileHandle.close()

