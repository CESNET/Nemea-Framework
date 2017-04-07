class Action():
    actionId = None
    actionType = None

    def run(self, record):
        raise Exception("Run method not implemented")


#import os
#
#class WardenAction(Action):
#    def __init__(self, action):
#        from vendors import warden_client
#
#        self.actionId = action["id"]
#        self.actionType = "warden"
#        self.url = action["warden"]["url"]
#
#        try:
#            config = warden_client.read_cfg(self.url)
#            self.client = warden_client.Client(**config)
#        except ValueError as e:
#            sys.stderr.write("{0}: Failed to load Warden config file '{1}'\n{2}\n".format("WardenAction", self.url, e))
#            exit(1)
#
#    def run(self, record):
#        self.client.sendEvents([record])
#
#    def __del__(self):
#        self.client.close()
#
#class FileAction(Action):
#
#    def __init__(self, action):
#        self.actionId = action["id"]
#        self.actionType = "file"
#        self.path = action["path"]
#        self.file = os.path.isfile(self.path)
#
#        if self.file:
#            self.fileHandle = open(self.path, "a")
#        else:
#            self.fileHandle = None
#
#    def run(self, record):
#        if self.fileHandle:
#            self.fileHandle.write(record)
#
#        else:
#            with open(self.path) as f:
#                pass
#
#    def __del__(self):
#        self.fileHandle.close()
#
#class EmailAction(Action):
#    def __init__(self, action):
#        # Should import smtp lib here
#        self.actionId = action["id"]
#        self.to = action["to"]
#        self.subject = action["subject"]
#
#    def run(self, record):
#        pass
#
#class MongoAction(Action):
#    host = "localhost"
#    port = 27017
#    user = None
#    password = None
#
#    def __init__(self, action):
#        """Parse action contents including info about MongoDB connection
#        and create connection to database.
#        """
#        import pymongo
#        self.actionId = action["id"]
#
#        if "host" in action:
#            self.host = action["host"]
#        if "port" in action:
#            self.port = action["port"]
#
#        self.db = action["db"]
#        self.collection = action["collection"]
#
#        if "user" in action and "password" in action:
#            self.user = action["user"]
#            self.password = action["password"]
#
#        self.client = pymongo.MongoClient(self.host, self.port)
#        self.collection = self.client[self.db][self.collection]
#
#    def store(self, record):
#        """Store IDEA message to MongoDB
#        Before storing the record is transformed to be stored in more effective
#        way using transform() method.
#        """
#        self.collection.insert(self.transform(record))
#
#    def transform(self, record):
#        """Transform certain items in IDEA message
#        Convert '*Time' items to Datetime format.
#        """
#
#        for i in ["DetectTime", "CreateTime", "EventTime", "CeaseTime"]:
#            if record.has_key(i):
#                record[i] = datetime.strptime(record[i], "%Y-%m-%dT%H:%M:%SZ")
#
#        return record
#
#    def run(self, record):
#        return self.store(record)
#
#class MarkAction(Action):
#    def __init__(self, action):
#        from vendors.mentat import jpath
#        self.actionId = action["id"]
#        # TODO: parse path according to mentat jpath
#        self.path = action["mark"]["path"]
#        self.value = action["mark"]["value"]
#
#    def mark(self, record):
#        pass
#
#    def run(self, record):
#        return self.mark(record)
#
#
