from .Action import Action

import sys
import os
from stat import *
import json
import logging as log
import fcntl

logger = log.getLogger(__name__)

class FileAction(Action):
    """Store IDEA message in a file
    Depending on the given file path the messages can be stored in:
        * FILE: a single file with new messages appended to it
        * DIR: each message in separate file where filename is the ID of the message
          with .idea extension
        * STDOUT: message is written to STDOUT pipe
    """
    def __init__(self, action):
        super(type(self), self).__init__(actionId = action["id"], actionType = "file")

        a = action["file"]

        self.path = a["path"] if "path" in a else None
        if not self.path:
            raise Exception("File action requires `path` argument.")

        try:
            # path already exists, check if it is a directory
            s = os.stat(self.path)
            self.dir = S_ISDIR(s.st_mode)
        except:
            self.dir = False

        if self.path == '-':
            self.fileHandle = sys.stdout
            self.dir = True
        elif not self.dir:
            # Open file if dir is not specified
            self.fileHandle = open(self.path, "a")
        else:
            self.fileHandle = None
            if not os.path.exists(self.path):
                # TODO set permissions to some better value
                os.mkdir(self.path, 777)

    def run(self, record):
        super(type(self), self).run(record)
        try:
            if self.path == '-':
                sys.stdout.write(json.dumps(record) + '\n')
                sys.stdout.flush()
            elif self.dir:
                # Store record into separate file
                with open(os.path.join(self.path, record["ID"] + ".idea"), "w") as f:
                    f.write(json.dumps(record))
            else:
                # Open file if dir is not specified
                with open(self.path, "a") as f:
                    fcntl.flock(f, fcntl.LOCK_EX)
                    f.write(json.dumps(record) + '\n')
                    f.flush()
                    fcntl.flock(f, fcntl.LOCK_UN)
        except Exception as e:
            self.logger.error(e)

    def __del__(self):
        if self.fileHandle:
            if self.fileHandle != sys.stdout:
                self.fileHandle.close()

