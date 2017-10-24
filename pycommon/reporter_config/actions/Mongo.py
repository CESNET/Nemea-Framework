from .Action import Action

from datetime import datetime
import copy

class MongoAction(Action):

    def __init__(self, action):
        """Parse action contents including info about MongoDB connection
        and create connection to database.
        """
        super(type(self), self).__init__(actionId = action["id"], actionType = "mongo")
        a = action["mongo"]
        self.host = a.get("host", "localhost")
        self.port = a.get("port", 27017)

        self.db = a.get("db", "nemeadb")
        self.collection_name = a.get("collection", "alerts_new")

        self.user = a.get("user", None)
        self.password = a.get("password", None)
        self.err_printed = False

        try:
            import pymongo
            try:
                # Python 3.x
                from urllib.parse import quote_plus
            except ImportError:
                # Python 2.x
                from urllib import quote_plus

            self.uri = "mongodb://"
            if self.user and self.password:
                self.uri += quote_plus(self.user) + ":" + quote_plus(self.password) + "@"
                self.uri += self.host + ":" + str(self.port) + "/" + self.db
            else:
                self.uri += self.host + ":" + str(self.port)

            self.client = pymongo.MongoClient(self.uri)
            self.collection = self.client[self.db][self.collection_name]
            if pymongo.version_tuple[0] < 3:
                self.collection.insert_one = self.collection.insert
        except Exception:
            self.client = None

    def store(self, record):
        """Store IDEA message to MongoDB
        Before storing the record is transformed to be stored in more effective
        way using transform() method.
        """
        # Must do a deepcopy so other actions won't be affected by this operation
        rec = copy.deepcopy(record)

        rec = self.transform(rec)
        try:
            self.collection.insert_one(rec)
        except Exception as e:
            self.logger.error(e)

    def transform(self, record):
        """Transform certain items in IDEA message
        Convert '*Time' items to Datetime format.
        """
        for i in ["CreateTime", "DetectTime", "EventTime", "CeaseTime", "WinStartTime", "WinEndTime"]:
            if i in record:
                record[i] = datetime.strptime(record[i], "%Y-%m-%dT%H:%M:%SZ")

        return record

    def run(self, record):
        super(type(self), self).run(record)
        if self.client:
            return self.store(record)
        elif not self.err_printed:
            self.logger.warning("Skipping mongo action, pymongo is not initialized.")
            self.err_printed = True

    def __del__(self):
        self.logger.debug("Closing connection to mongoDB")
        if self.client:
            self.client.close()

    def __str__(self):
        f = []
        f.append("Host: " + self.host)
        f.append("Port: " + str(self.port))
        f.append("DB: " + self.db)
        f.append("Collection: " + self.collection_name)
        if self.user and self.password:
            f.append("Auth to DB using User: " + self.user + " and *PASSWORD*")
        return ", ".join(f) + "\n"

