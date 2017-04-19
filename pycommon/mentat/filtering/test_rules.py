#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
# This file is part of Mentat system (https://mentat.cesnet.cz/).
#
# Copyright (C) since 2011 CESNET, z.s.p.o (http://www.ces.net/)
# Use of this source is governed by the MIT license, see LICENSE file.
#-------------------------------------------------------------------------------

import os
import sys
import shutil
import unittest
from pprint import pformat, pprint

# Generate the path to custom 'lib' directory
lib = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../lib'))
sys.path.insert(0, lib)

from mentat.filtering.rules import *

#-------------------------------------------------------------------------------
# NOTE: Sorry for the long lines in this file. They are deliberate, because the
# assertion permutations are (IMHO) more readable this way.
#-------------------------------------------------------------------------------

class TestMentatRules(unittest.TestCase):

    def test_01_basic(self):
        """
        Perform basic rules tests: instantinate and check all rule objects.
        """
        self.maxDiff = None

        rule_var = VariableRule("Test")
        self.assertEqual(str(rule_var), "Test")
        self.assertEqual(repr(rule_var), "VARIABLE('Test')")
        rule_const = ConstantRule("constant")
        self.assertEqual(str(rule_const), '"constant"')
        self.assertEqual(repr(rule_const), "CONSTANT('constant')")
        rule_ipv4 = IPV4Rule("127.0.0.1")
        self.assertEqual(str(rule_ipv4), "127.0.0.1")
        self.assertEqual(repr(rule_ipv4), "IPV4('127.0.0.1')")
        rule_ipv6 = IPV6Rule("::1")
        self.assertEqual(str(rule_ipv6), "::1")
        self.assertEqual(repr(rule_ipv6), "IPV6('::1')")
        rule_integer = IntegerRule(15)
        self.assertEqual(str(rule_integer), "15")
        self.assertEqual(repr(rule_integer), "INTEGER(15)")
        rule_float = FloatRule(15.5)
        self.assertEqual(str(rule_float), "15.5")
        self.assertEqual(repr(rule_float), "FLOAT(15.5)")
        rule_list = ListRule(VariableRule("Test"), ListRule(ConstantRule("constant"), ListRule(IPV4Rule("127.0.0.1"))))
        self.assertEqual(str(rule_list), '[Test, "constant", 127.0.0.1]')
        self.assertEqual(repr(rule_list), "LIST(VARIABLE('Test'), CONSTANT('constant'), IPV4('127.0.0.1'))")
        self.assertEqual(str(rule_list.value), "[VARIABLE('Test'), CONSTANT('constant'), IPV4('127.0.0.1')]")
        self.assertEqual(pformat(rule_list.value), "[VARIABLE('Test'), CONSTANT('constant'), IPV4('127.0.0.1')]")
        rule_binop_l = LogicalBinOpRule("OP_OR", rule_var, rule_integer)
        self.assertEqual(str(rule_binop_l), "(Test OP_OR 15)")
        self.assertEqual(repr(rule_binop_l), "LOGBINOP(VARIABLE('Test') OP_OR INTEGER(15))")
        rule_binop_c = ComparisonBinOpRule("OP_GT", rule_var, rule_integer)
        self.assertEqual(str(rule_binop_c), "(Test OP_GT 15)")
        self.assertEqual(repr(rule_binop_c), "COMPBINOP(VARIABLE('Test') OP_GT INTEGER(15))")
        rule_binop_m = MathBinOpRule("OP_PLUS", rule_var, rule_integer)
        self.assertEqual(str(rule_binop_m), "(Test OP_PLUS 15)")
        self.assertEqual(repr(rule_binop_m), "MATHBINOP(VARIABLE('Test') OP_PLUS INTEGER(15))")
        rule_binop = LogicalBinOpRule("OP_OR", ComparisonBinOpRule("OP_GT", MathBinOpRule("OP_PLUS", VariableRule("Test"), IntegerRule(10)), IntegerRule(20)), ComparisonBinOpRule("OP_LT", VariableRule("Test"), IntegerRule(5)))
        self.assertEqual(str(rule_binop), "(((Test OP_PLUS 10) OP_GT 20) OP_OR (Test OP_LT 5))")
        self.assertEqual(repr(rule_binop), "LOGBINOP(COMPBINOP(MATHBINOP(VARIABLE('Test') OP_PLUS INTEGER(10)) OP_GT INTEGER(20)) OP_OR COMPBINOP(VARIABLE('Test') OP_LT INTEGER(5)))")
        rule_unop = UnaryOperationRule("OP_NOT", rule_var)
        self.assertEqual(str(rule_unop), "(OP_NOT Test)")
        self.assertEqual(repr(rule_unop), "UNOP(OP_NOT VARIABLE('Test'))")

class TestMentatRuleTreeTraverser(unittest.TestCase):

    def test_01_evaluate_binops_logical(self):
        """
        Test the logical binary operations evaluations.
        """
        self.maxDiff = None

        traverser = RuleTreeTraverser()

        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', 1,    1),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', 0,    1),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', 1,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', None, None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', 0,    None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', None, 0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', 0,    0),    False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', 1,    1),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', 0,    1),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', 1,    0),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', None, None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', 0,    None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', None, 0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', 1,    None), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', None, 1),    True)

        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', 1,    1),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', 0,    1),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', 1,    0),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', None, None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', 0,    None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', None, 0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', 1,    None), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', None, 1),    True)

        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', "True", "True"), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', "",     "True"), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', "True", ""),     False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', "",     ""),     False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', "True", "True"), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', "",     "True"), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', "True", ""),     True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', "",     ""),     False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', "True", "True"), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', "",     "True"), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', "True", ""),     True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', "",     ""),     False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', [1,2], [1,2]), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', [],    [1,2]), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', [1,2], []),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', [],    []),    False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', [1,2], [1,2]), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', [],    [1,2]), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', [1,2], []),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', [],    []),    False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', [1,2], [1,2]), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', [],    [1,2]), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', [1,2], []),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', [],    []),    False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', {"x":1}, {"x":1}), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', {},      {"x":1}), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', {"x":1}, {}),      False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND', {},      {}),      False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', {"x":1}, {"x":1}), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', {},      {"x":1}), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', {"x":1}, {}),      True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR', {},      {}),      False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', {"x":1}, {"x":1}), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', {},      {"x":1}), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', {"x":1}, {}),      True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR', {},      {}),      False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', 1,    1),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', 0,    1),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', 1,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', None, None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', 0,    None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', None, 0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', 1,    None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', None, 1),    False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', 1,    1),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', 0,    1),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', 1,    0),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', None, None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', 0,    None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', None, 0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', 1,    None), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', None, 1),    True)

        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', 1,    1),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', 0,    1),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', 1,    0),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', None, None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', 0,    None), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', None, 0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', 0,    0),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', 1,    None), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', None, 1),    True)

        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', "True", "True"), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', "",     "True"), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', "True", ""),     False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', "",     ""),     False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', "True", "True"), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', "",     "True"), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', "True", ""),     True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', "",     ""),     False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', "True", "True"), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', "",     "True"), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', "True", ""),     True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', "",     ""),     False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', [1,2], [1,2]), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', [],    [1,2]), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', [1,2], []),    False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', [],    []),    False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', [1,2], [1,2]), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', [],    [1,2]), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', [1,2], []),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', [],    []),    False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', [1,2], [1,2]), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', [],    [1,2]), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', [1,2], []),    True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', [],    []),    False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', {"x":1}, {"x":1}), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', {},      {"x":1}), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', {"x":1}, {}),      False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_AND_P', {},      {}),      False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', {"x":1}, {"x":1}), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', {},      {"x":1}), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', {"x":1}, {}),      True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_OR_P', {},      {}),      False)

        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', {"x":1}, {"x":1}), False)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', {},      {"x":1}), True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', {"x":1}, {}),      True)
        self.assertEqual(traverser.evaluate_binop_logical('OP_XOR_P', {},      {}),      False)

    def test_02_evaluate_binops_comparison(self):
        """
        Test the comparison binary operations evaluations.
        """
        self.maxDiff = None

        traverser = RuleTreeTraverser()

        self.assertEqual(traverser.evaluate_binop_comparison('OP_LIKE', 'abcd', 'a'), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_LIKE', 'abcd', 'e'), False)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_IN', 'a', ['a','b','c','d']), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_IN', 'e', ['a','b','c','d']), False)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_IS', ['a','b','c','d'], ['a','b','c','d']), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_IS', ['a','b','c','e'], ['a','b','c','d']), False)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_EQ', 'a', 'a'), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_EQ', 'e', 'a'), False)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_NE', 'e', 'a'), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_NE', 'a', 'a'), False)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_GT', 'ab', 'ab'), False)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_GT', 'eb', 'ab'), True)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_GE', 'eb', 'ab'), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_GE', 'ab', 'ab'), True)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_LT', 'ab', 'ab'), False)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_LT', 'eb', 'ab'), False)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_LE', 'eb', 'ab'), False)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_LE', 'ab', 'ab'), True)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_IN', 1, [1,2,3,4]), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_IN', 5, [1,2,3,4]), False)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_IS', 1, 1), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_IS', 1, 5), False)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_IS', "Test", "Test"),     True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_IS', "Test", ["Test"]),   True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_IS', ["Test"], ["Test"]), True)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_EQ', 1, 1), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_EQ', 2, 1), False)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_NE', 2, 1), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_NE', 1, 1), False)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_GT', 1, 1), False)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_GT', 2, 1), True)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_GE', 1, 1), True)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_GE', 1, 2), False)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_LT', 1, 1), False)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_LT', 1, 2), True)

        self.assertEqual(traverser.evaluate_binop_comparison('OP_LE', 2, 1), False)
        self.assertEqual(traverser.evaluate_binop_comparison('OP_LE', 1, 2), True)

    def test_03_evaluate_binops_math(self):
        """
        Test the mathematical binary operations evaluations.
        """
        self.maxDiff = None

        traverser = RuleTreeTraverser()

        self.assertEqual(traverser.evaluate_binop_math('OP_PLUS',   10, 10), 20)
        self.assertEqual(traverser.evaluate_binop_math('OP_MINUS',  10, 10), 0)
        self.assertEqual(traverser.evaluate_binop_math('OP_TIMES',  10, 10), 100)
        self.assertEqual(traverser.evaluate_binop_math('OP_MODULO', 10,  3), 1)

        self.assertEqual(traverser.evaluate_binop_math('OP_PLUS',   [10], [10]), 20)
        self.assertEqual(traverser.evaluate_binop_math('OP_MINUS',  [10], [10]), 0)
        self.assertEqual(traverser.evaluate_binop_math('OP_TIMES',  [10], [10]), 100)
        self.assertEqual(traverser.evaluate_binop_math('OP_MODULO', [10],  [3]), 1)

        self.assertEqual(traverser.evaluate_binop_math('OP_PLUS',   [10,20], [10]), [20,30])
        self.assertEqual(traverser.evaluate_binop_math('OP_MINUS',  [10,20], [10]), [0,10])
        self.assertEqual(traverser.evaluate_binop_math('OP_TIMES',  [10,20], [10]), [100,200])
        self.assertEqual(traverser.evaluate_binop_math('OP_MODULO', [10,20],  [3]), [1,2])

        self.assertEqual(traverser.evaluate_binop_math('OP_PLUS',   [10], [10,20]), [20,30])
        self.assertEqual(traverser.evaluate_binop_math('OP_MINUS',  [10], [10,20]), [0,-10])
        self.assertEqual(traverser.evaluate_binop_math('OP_TIMES',  [10], [10,20]), [100,200])
        self.assertEqual(traverser.evaluate_binop_math('OP_MODULO', [10],   [3,4]), [1,2])

class TestMentatPrintingTreeTraverser(unittest.TestCase):

    def test_01_basic(self):
        """
        Demonstrate and test the PrintingTreeTraverser object.
        """
        self.maxDiff = None

        traverser = PrintingTreeTraverser()

        rule_binop_l = LogicalBinOpRule('OP_OR', VariableRule("Test"), IntegerRule(10))
        self.assertEqual(rule_binop_l.traverse(traverser), 'LOGBINOP(OP_OR;VARIABLE(Test);INTEGER(10))')

        rule_binop_c = ComparisonBinOpRule('OP_GT', VariableRule("Test"), IntegerRule(15))
        self.assertEqual(rule_binop_c.traverse(traverser), 'COMPBINOP(OP_GT;VARIABLE(Test);INTEGER(15))')

        rule_binop_m = MathBinOpRule('OP_PLUS', VariableRule("Test"), IntegerRule(10))
        self.assertEqual(rule_binop_m.traverse(traverser), 'MATHBINOP(OP_PLUS;VARIABLE(Test);INTEGER(10))')

        rule_binop = LogicalBinOpRule('OP_OR', ComparisonBinOpRule('OP_GT', MathBinOpRule('OP_PLUS', VariableRule("Test"), IntegerRule(10)), IntegerRule(20)), ComparisonBinOpRule('OP_LT', VariableRule("Test"), IntegerRule(5)))
        self.assertEqual(rule_binop.traverse(traverser), 'LOGBINOP(OP_OR;COMPBINOP(OP_GT;MATHBINOP(OP_PLUS;VARIABLE(Test);INTEGER(10));INTEGER(20));COMPBINOP(OP_LT;VARIABLE(Test);INTEGER(5)))')

        rule_unop = UnaryOperationRule('OP_NOT', VariableRule("Test"))
        self.assertEqual(rule_unop.traverse(traverser), 'UNOP(OP_NOT;VARIABLE(Test))')

if __name__ == '__main__':
    unittest.main()
