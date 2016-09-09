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

