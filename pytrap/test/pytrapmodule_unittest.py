import unittest
import doctest
import sys
if sys.version_info > (3,):
    long = int

class DeviceTest(unittest.TestCase):
    def runTest(self):
        try:
            import pytrap
        except ImportError as e:
            self.fail(str(e))

class TrapCtxInitTest(unittest.TestCase):
    def runTest(self):
        import pytrap
        c = pytrap.TrapCtx()
        try:
            c.init()
            self.fail("argv is mandatory")
        except:
            pass
        try:
            c.init([])
            self.fail("argv must not be empty")
        except:
            pass
        try:
            c.init([[]])
            self.fail("argv[i] must be string")
        except:
            pass

        c.init(["-i", "u:test_init"], 0, 1)
        c.finalize()

#Skip tests that print help
#class TrapCtxHelpTest(unittest.TestCase):
#    def runTest(self):
#        import pytrap
#        c = pytrap.TrapCtx()
#        try:
#            c.init(["-h"], 0, 1)
#            self.fail("Calling method of uninitialized context.")
#        except pytrap.TrapHelp:
#            pass
#        c.finalize()
#
#class TrapCtxHelpifcTest(unittest.TestCase):
#    def runTest(self):
#        import pytrap
#        c = pytrap.TrapCtx()
#        try:
#            c.init(["-h", "1"], 0, 1)
#            self.fail("Calling method of uninitialized context.")
#        except pytrap.TrapHelp:
#            pass
#        c.finalize()

class TrapCtxNotInitTestGetdatafmt(unittest.TestCase):
    def runTest(self):
        import pytrap
        c = pytrap.TrapCtx()
        try:
            c.getDataFmt()
            self.fail("Calling method of uninitialized context.")
        except:
            pass

class TrapCtxNotInitTestSetrequiredfmt(unittest.TestCase):
    def runTest(self):
        import pytrap
        c = pytrap.TrapCtx()
        try:
            c.setRequiredFmt(0)
            self.fail("Calling method of uninitialized context.")
        except:
            pass

class TrapCtxNotInitTestGetinifcstate(unittest.TestCase):
    def runTest(self):
        import pytrap
        c = pytrap.TrapCtx()
        try:
            c.getInIFCState(0)
            self.fail("Calling method of uninitialized context.")
        except:
            pass

class TrapCtxNotInitTestIfcctl(unittest.TestCase):
    def runTest(self):
        import pytrap
        c = pytrap.TrapCtx()
        try:
            c.ifcctl(0, True, CTL_IMEOUT, 0)
            self.fail("Calling method of uninitialized context.")
        except:
            pass

class TrapCtxNotInitTestSend(unittest.TestCase):
    def runTest(self):
        import pytrap
        c = pytrap.TrapCtx()
        try:
            c.send()
            self.fail("Calling method of uninitialized context.")
        except:
            pass

class TrapCtxNotInitTestSendFlush(unittest.TestCase):
    def runTest(self):
        import pytrap
        c = pytrap.TrapCtx()
        try:
            c.sendFlush()
            self.fail("Calling method of uninitialized context.")
        except:
            pass

class TrapCtxNotInitTestRecv(unittest.TestCase):
    def runTest(self):
        import pytrap
        c = pytrap.TrapCtx()
        try:
            c.recv()
            self.fail("Calling method of uninitialized context.")
        except:
            pass

class TrapCtxGetVersion(unittest.TestCase):
    def runTest(self):
        import pytrap
        c = pytrap.getTrapVersion()
        print(c)

class StoreAndLoadMessage(unittest.TestCase):
    def runTest(self):
        import pytrap
        import os

        urtempl = "ipaddr IP,uint16 PORT"
        c = pytrap.TrapCtx()
        c.init(["-i", "f:/tmp/pytrap_test"], 0, 1)
        c.setDataFmt(0, pytrap.FMT_UNIREC, urtempl)

        t = pytrap.UnirecTemplate("ipaddr IP,uint16 PORT")
        t.createMessage()
        t.IP = pytrap.UnirecIPAddr("192.168.0.1")
        t.PORT = 123
        c.send(t.getData())
        c.sendFlush()
        c.finalize()

        c = pytrap.TrapCtx()
        c.init(["-i", "f:/tmp/pytrap_test"], 1)
        c.setRequiredFmt(0, pytrap.FMT_UNIREC, urtempl)
        data = c.recv()
        t = pytrap.UnirecTemplate(urtempl)
        t.setData(data)
        self.assertEqual(t.IP, pytrap.UnirecIPAddr("192.168.0.1"))
        self.assertEqual(t.PORT, 123)
        c.finalize()

        os.unlink("/tmp/pytrap_test")

class StoreAndLoadStringMessage(unittest.TestCase):
    def runTest(self):
        #"""json.dump returns str object, which was formerly not supported by pytrep send()"""
        import pytrap
        import json
        import os

        urtempl = "test"
        jdata = {"SRC_IP": "192.168.0.1"}
        c = pytrap.TrapCtx()
        c.init(["-i", "f:/tmp/pytrap_test2"], 0, 1)
        c.setDataFmt(0, pytrap.FMT_JSON, urtempl)

        c.send(json.dumps(jdata))
        c.sendFlush()
        c.finalize()

        c = pytrap.TrapCtx()
        c.init(["-i", "f:/tmp/pytrap_test2"], 1)
        c.setRequiredFmt(0, pytrap.FMT_JSON, urtempl)
        data = c.recv()
        c.finalize()
        self.assertEqual(jdata, json.loads(data))

        os.unlink("/tmp/pytrap_test2")

class SendAndReceiveMessageList(unittest.TestCase):
    def runTest(self):
        import pytrap
        import os
        import time

        messages = 10000000

        urtempl = "ipaddr IP,uint16 PORT"

        # Start sender
        c1 = pytrap.TrapCtx()
        c1.init(["-i", "f:/tmp/pytrap_test3"], 0, 1)
        c1.setDataFmt(0, pytrap.FMT_UNIREC, urtempl)
        c1.ifcctl(0, False, pytrap.CTL_TIMEOUT, 500000)
        c1.ifcctl(0, False, pytrap.CTL_AUTOFLUSH, 500000)

        t = pytrap.UnirecTemplate(urtempl)
        t.createMessage()

        t.IP = pytrap.UnirecIPAddr("192.168.0.1")

        for i in range(messages):
            t.PORT=i
            c1.send(t.getData())
        c1.sendFlush()

        # Start Receiver
        c2 = pytrap.TrapCtx()
        c2.init(["-i", "f:/tmp/pytrap_test3"], 1)
        c2.setRequiredFmt(0, pytrap.FMT_UNIREC, urtempl)
        startt = time.process_time()
        data = c2.recvBulk(t, time=15, count=messages)
        elapsed_time = time.process_time() - startt
        print(f"recvBulk() Elapsed time for {messages} messages is: {elapsed_time}")

        self.assertEqual(len(data), messages)
        ports = [i["PORT"] for i in data]
        self.assertEqual(ports, [i & 0xFFFF for i in range(messages)])

        # Start Receiver
        c2 = pytrap.TrapCtx()
        c2.init(["-i", "f:/tmp/pytrap_test3"], 1)
        c2.setRequiredFmt(0, pytrap.FMT_UNIREC, urtempl)
        startt = time.process_time()
        data = list()
        while True:
            d = c2.recv()
            if not d:
                break
            t.setData(d)
            data.append(t.getDict())

        elapsed_time = time.process_time() - startt
        print(f"recv() Elapsed time for {messages} messages is: {elapsed_time}")

        self.assertEqual(len(data), messages)
        ports = [i["PORT"] for i in data]
        self.assertEqual(ports, [i & 0xFFFF for i in range(messages)])

        c1.finalize()
        c2.finalize()

        os.unlink("/tmp/pytrap_test3")

