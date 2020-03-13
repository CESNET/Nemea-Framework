import unittest
import doctest
import report2idea

def load_tests(loader, tests, ignore):
    tests.addTests(doctest.DocTestSuite(report2idea))
    return tests
