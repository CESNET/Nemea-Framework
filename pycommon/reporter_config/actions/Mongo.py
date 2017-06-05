from .Action import Action

from datetime import datetime
import pymongo
import logging as log
import copy

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

        if "host" in action["mongo"]:
            self.host = action["mongo"]["host"]
        if "port" in action["mongo"]:
            self.port = action["mongo"]["port"]

        self.db = action["mongo"]["db"]
        self.collection = action["mongo"]["collection"]

        if "user" in action["mongo"] and "password" in action["mongo"]:
            self.user = action["mongo"]["user"]
            self.password = action["mongo"]["password"]

        self.client = pymongo.MongoClient(self.host, self.port)
        self.collection = self.client[self.db][self.collection]

    def store(self, record):
        """Store IDEA message to MongoDB
        Before storing the record is transformed to be stored in more effective
        way using transform() method.
        """
        # Must do a deepcopy so other actions won't be affted by this operation
        rec = copy.deepcopy(record)

        rec = self.transform(rec)
        self.collection.insert_one(rec)

    def transform(self, record):
        """Transform certain items in IDEA message
        Convert '*Time' items to Datetime format.
        """
        for i in ["CreateTime", "DetectTime", "EventTime", "CeaseTime", "WinStartTime", "WinEndTime"]:
            if i in record:
                record[i] = datetime.strptime(record[i], "%Y-%m-%dT%H:%M:%SZ")

        return record

    def run(self, record):
        logger.debug("Storing record to mongoDB")
        return self.store(record)

    def __del__(self):
        logger.debug("Closing connection to mongoDB")
        self.client.close()
