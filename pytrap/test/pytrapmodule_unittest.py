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

