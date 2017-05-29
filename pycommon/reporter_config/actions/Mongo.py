from actions.Action import Action

from datetime import datetime
import pymongo
import logging as log

logger = log.getLogger(__name__)

class MongoAction(Action):
    host = "localhost"
    port = 27017
    user = None
    password = None

    def __init__(self, action):
        """Parse action contents including info about MongoDB connection
        and create connection to database.
        """
        self.actionId = action["id"]

        if "host" in action:
            self.host = action["host"]
        if "port" in action:
            self.port = action["port"]

        self.db = action["db"]
        self.collection = action["collection"]

        if "user" in action and "password" in action:
            self.user = action["user"]
            self.password = action["password"]

        self.client = pymongo.MongoClient(self.host, self.port)
        self.collection = self.client[self.db][self.collection]

    def store(self, record):
        """Store IDEA message to MongoDB
        Before storing the record is transformed to be stored in more effective
        way using transform() method.
        """
        self.collection.insert(self.transform(record))

    def transform(self, record):
        """Transform certain items in IDEA message
        Convert '*Time' items to Datetime format.
        """
        for i in ["CreateTime", "DetectTime", "EventTime", "CeaseTime", "WinStartTime", "WinEndTime"]:
            if record.has_key(i):
                record[i] = datetime.strptime(record[i], "%Y-%m-%dT%H:%M:%SZ")

        return record

    def run(self, record):
        logger.debug("Storing record to mongoDB")
        return self.store(record)

    def __del__(self):
        logger.debug("Closing connection to mongoDB")
        self.client.close()
