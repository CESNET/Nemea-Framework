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

        self.path = a.get("path", None)
        if not self.path:
            raise Exception("File action requires `path` argument.")
        self.temp_path = a.get("temp_path", None)
        self.save_path = self.temp_path if self.temp_path else self.path

        try:
            # path already exists, check if it is a directory
            s = os.stat(self.path)
            self.dir = S_ISDIR(s.st_mode)
        except Exception:
            self.dir = False

    def run(self, record):
        super(type(self), self).run(record)
        try:
            if self.path == '-':
                sys.stdout.write(json.dumps(record) + '\n')
                sys.stdout.flush()
            elif self.dir:
                # Store record into separate file
                filename = record["ID"] + ".idea"
                outfile = os.path.join(self.save_path, filename)
                with open(outfile, "w") as f:
                    f.write(json.dumps(record))

                # if the save_path is temporary, we need to move the file
                if self.temp_path:
                    targetfile = os.path.join(self.path, filename)
                    os.rename(outfile, targetfile)
            else:
                # Open file if dir is not specified
                with open(self.path, "a") as f:
                    fcntl.flock(f, fcntl.LOCK_EX)
                    f.write(json.dumps(record) + '\n')
                    f.flush()
                    fcntl.flock(f, fcntl.LOCK_UN)
        except Exception as e:
            self.logger.error(e)

    def __str__(self):
        return "Path: " + self.path + (" (Directory)" if self.dir else "") + ((" using temp: " + self.temp_path) if self.temp_path else "") + "\n"

