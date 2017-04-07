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

from mentat.filtering.gparser import MentatFilterParser

#-------------------------------------------------------------------------------
# NOTE: Sorry for the long lines in this file. They are deliberate, because the
# assertion permutations are (IMHO) more readable this way.
#-------------------------------------------------------------------------------

class TestMentatFilterParser(unittest.TestCase):

    def test_01_basic_logical(self):
        """
        Test the parsing of basic logical operations.
        """
        self.maxDiff = None

        p = MentatFilterParser()
        p.build()

        self.assertEqual(repr(p.parse('1 and 1')),  'LOGBINOP(INTEGER(1) OP_AND INTEGER(1))')
        self.assertEqual(repr(p.parse('1 AND 1')),  'LOGBINOP(INTEGER(1) OP_AND INTEGER(1))')
        self.assertEqual(repr(p.parse('1 && 1')),   'LOGBINOP(INTEGER(1) OP_AND_P INTEGER(1))')
        self.assertEqual(repr(p.parse('1 or 1')),   'LOGBINOP(INTEGER(1) OP_OR INTEGER(1))')
        self.assertEqual(repr(p.parse('1 OR 1')),   'LOGBINOP(INTEGER(1) OP_OR INTEGER(1))')
        self.assertEqual(repr(p.parse('1 || 1')),   'LOGBINOP(INTEGER(1) OP_OR_P INTEGER(1))')
        self.assertEqual(repr(p.parse('1 xor 1')),  'LOGBINOP(INTEGER(1) OP_XOR INTEGER(1))')
        self.assertEqual(repr(p.parse('1 XOR 1')),  'LOGBINOP(INTEGER(1) OP_XOR INTEGER(1))')
        self.assertEqual(repr(p.parse('1 ^^ 1')),   'LOGBINOP(INTEGER(1) OP_XOR_P INTEGER(1))')
        self.assertEqual(repr(p.parse('not 1')),    'UNOP(OP_NOT INTEGER(1))')
        self.assertEqual(repr(p.parse('NOT 1')),    'UNOP(OP_NOT INTEGER(1))')
        self.assertEqual(repr(p.parse('! 1')),      'UNOP(OP_NOT INTEGER(1))')
        self.assertEqual(repr(p.parse('exists 1')), 'UNOP(OP_EXISTS INTEGER(1))')
        self.assertEqual(repr(p.parse('EXISTS 1')), 'UNOP(OP_EXISTS INTEGER(1))')
        self.assertEqual(repr(p.parse('? 1')),      'UNOP(OP_EXISTS INTEGER(1))')

        self.assertEqual(repr(p.parse('(1 and 1)')),  'LOGBINOP(INTEGER(1) OP_AND INTEGER(1))')
        self.assertEqual(repr(p.parse('(1 AND 1)')),  'LOGBINOP(INTEGER(1) OP_AND INTEGER(1))')
        self.assertEqual(repr(p.parse('(1 && 1)')),   'LOGBINOP(INTEGER(1) OP_AND_P INTEGER(1))')
        self.assertEqual(repr(p.parse('(1 or 1)')),   'LOGBINOP(INTEGER(1) OP_OR INTEGER(1))')
        self.assertEqual(repr(p.parse('(1 OR 1)')),   'LOGBINOP(INTEGER(1) OP_OR INTEGER(1))')
        self.assertEqual(repr(p.parse('(1 || 1)')),   'LOGBINOP(INTEGER(1) OP_OR_P INTEGER(1))')
        self.assertEqual(repr(p.parse('(1 xor 1)')),  'LOGBINOP(INTEGER(1) OP_XOR INTEGER(1))')
        self.assertEqual(repr(p.parse('(1 XOR 1)')),  'LOGBINOP(INTEGER(1) OP_XOR INTEGER(1))')
        self.assertEqual(repr(p.parse('(1 ^^ 1)')),   'LOGBINOP(INTEGER(1) OP_XOR_P INTEGER(1))')
        self.assertEqual(repr(p.parse('(not 1)')),    'UNOP(OP_NOT INTEGER(1))')
        self.assertEqual(repr(p.parse('(NOT 1)')),    'UNOP(OP_NOT INTEGER(1))')
        self.assertEqual(repr(p.parse('(! 1)')),      'UNOP(OP_NOT INTEGER(1))')
        self.assertEqual(repr(p.parse('(exists 1)')), 'UNOP(OP_EXISTS INTEGER(1))')
        self.assertEqual(repr(p.parse('(EXISTS 1)')), 'UNOP(OP_EXISTS INTEGER(1))')
        self.assertEqual(repr(p.parse('(? 1)')),      'UNOP(OP_EXISTS INTEGER(1))')

        self.assertEqual(repr(p.parse('((1 and 1))')),  'LOGBINOP(INTEGER(1) OP_AND INTEGER(1))')
        self.assertEqual(repr(p.parse('((1 AND 1))')),  'LOGBINOP(INTEGER(1) OP_AND INTEGER(1))')
        self.assertEqual(repr(p.parse('((1 && 1))')),   'LOGBINOP(INTEGER(1) OP_AND_P INTEGER(1))')
        self.assertEqual(repr(p.parse('((1 or 1))')),   'LOGBINOP(INTEGER(1) OP_OR INTEGER(1))')
        self.assertEqual(repr(p.parse('((1 OR 1))')),   'LOGBINOP(INTEGER(1) OP_OR INTEGER(1))')
        self.assertEqual(repr(p.parse('((1 || 1))')),   'LOGBINOP(INTEGER(1) OP_OR_P INTEGER(1))')
        self.assertEqual(repr(p.parse('((1 xor 1))')),  'LOGBINOP(INTEGER(1) OP_XOR INTEGER(1))')
        self.assertEqual(repr(p.parse('((1 XOR 1))')),  'LOGBINOP(INTEGER(1) OP_XOR INTEGER(1))')
        self.assertEqual(repr(p.parse('((1 ^^ 1))')),   'LOGBINOP(INTEGER(1) OP_XOR_P INTEGER(1))')
        self.assertEqual(repr(p.parse('((not 1))')),    'UNOP(OP_NOT INTEGER(1))')
        self.assertEqual(repr(p.parse('((NOT 1))')),    'UNOP(OP_NOT INTEGER(1))')
        self.assertEqual(repr(p.parse('((! 1))')),      'UNOP(OP_NOT INTEGER(1))')
        self.assertEqual(repr(p.parse('((exists 1))')), 'UNOP(OP_EXISTS INTEGER(1))')
        self.assertEqual(repr(p.parse('((EXISTS 1))')), 'UNOP(OP_EXISTS INTEGER(1))')
        self.assertEqual(repr(p.parse('((? 1))')),      'UNOP(OP_EXISTS INTEGER(1))')

    def test_02_basic_comparison(self):
        """
        Test the parsing of basic comparison operations.
        """
        self.maxDiff = None

        p = MentatFilterParser()
        p.build()

        self.assertEqual(repr(p.parse('2 like 2')), 'COMPBINOP(INTEGER(2) OP_LIKE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 LIKE 2')), 'COMPBINOP(INTEGER(2) OP_LIKE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 =~ 2')),   'COMPBINOP(INTEGER(2) OP_LIKE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 in 2')),   'COMPBINOP(INTEGER(2) OP_IN INTEGER(2))')
        self.assertEqual(repr(p.parse('2 IN 2')),   'COMPBINOP(INTEGER(2) OP_IN INTEGER(2))')
        self.assertEqual(repr(p.parse('2 ~~ 2')),   'COMPBINOP(INTEGER(2) OP_IN INTEGER(2))')
        self.assertEqual(repr(p.parse('2 is 2')),   'COMPBINOP(INTEGER(2) OP_IS INTEGER(2))')
        self.assertEqual(repr(p.parse('2 IS 2')),   'COMPBINOP(INTEGER(2) OP_IS INTEGER(2))')
        self.assertEqual(repr(p.parse('2 eq 2')),   'COMPBINOP(INTEGER(2) OP_EQ INTEGER(2))')
        self.assertEqual(repr(p.parse('2 EQ 2')),   'COMPBINOP(INTEGER(2) OP_EQ INTEGER(2))')
        self.assertEqual(repr(p.parse('2 == 2')),   'COMPBINOP(INTEGER(2) OP_EQ INTEGER(2))')
        self.assertEqual(repr(p.parse('2 ne 2')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 NE 2')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 != 2')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 <> 2')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 ge 2')),   'COMPBINOP(INTEGER(2) OP_GE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 GE 2')),   'COMPBINOP(INTEGER(2) OP_GE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 >= 2')),   'COMPBINOP(INTEGER(2) OP_GE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 gt 2')),   'COMPBINOP(INTEGER(2) OP_GT INTEGER(2))')
        self.assertEqual(repr(p.parse('2 GT 2')),   'COMPBINOP(INTEGER(2) OP_GT INTEGER(2))')
        self.assertEqual(repr(p.parse('2 > 2')),    'COMPBINOP(INTEGER(2) OP_GT INTEGER(2))')
        self.assertEqual(repr(p.parse('2 le 2')),   'COMPBINOP(INTEGER(2) OP_LE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 LE 2')),   'COMPBINOP(INTEGER(2) OP_LE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 <= 2')),   'COMPBINOP(INTEGER(2) OP_LE INTEGER(2))')
        self.assertEqual(repr(p.parse('2 lt 2')),   'COMPBINOP(INTEGER(2) OP_LT INTEGER(2))')
        self.assertEqual(repr(p.parse('2 LT 2')),   'COMPBINOP(INTEGER(2) OP_LT INTEGER(2))')
        self.assertEqual(repr(p.parse('2 < 2')),    'COMPBINOP(INTEGER(2) OP_LT INTEGER(2))')

        self.assertEqual(repr(p.parse('(2 like 2)')), 'COMPBINOP(INTEGER(2) OP_LIKE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 LIKE 2)')), 'COMPBINOP(INTEGER(2) OP_LIKE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 =~ 2)')),   'COMPBINOP(INTEGER(2) OP_LIKE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 in 2)')),   'COMPBINOP(INTEGER(2) OP_IN INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 IN 2)')),   'COMPBINOP(INTEGER(2) OP_IN INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 ~~ 2)')),   'COMPBINOP(INTEGER(2) OP_IN INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 is 2)')),   'COMPBINOP(INTEGER(2) OP_IS INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 IS 2)')),   'COMPBINOP(INTEGER(2) OP_IS INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 eq 2)')),   'COMPBINOP(INTEGER(2) OP_EQ INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 EQ 2)')),   'COMPBINOP(INTEGER(2) OP_EQ INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 == 2)')),   'COMPBINOP(INTEGER(2) OP_EQ INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 ne 2)')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 NE 2)')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 != 2)')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 <> 2)')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 ge 2)')),   'COMPBINOP(INTEGER(2) OP_GE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 GE 2)')),   'COMPBINOP(INTEGER(2) OP_GE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 >= 2)')),   'COMPBINOP(INTEGER(2) OP_GE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 gt 2)')),   'COMPBINOP(INTEGER(2) OP_GT INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 GT 2)')),   'COMPBINOP(INTEGER(2) OP_GT INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 > 2)')),    'COMPBINOP(INTEGER(2) OP_GT INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 le 2)')),   'COMPBINOP(INTEGER(2) OP_LE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 LE 2)')),   'COMPBINOP(INTEGER(2) OP_LE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 <= 2)')),   'COMPBINOP(INTEGER(2) OP_LE INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 lt 2)')),   'COMPBINOP(INTEGER(2) OP_LT INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 LT 2)')),   'COMPBINOP(INTEGER(2) OP_LT INTEGER(2))')
        self.assertEqual(repr(p.parse('(2 < 2)')),    'COMPBINOP(INTEGER(2) OP_LT INTEGER(2))')

        self.assertEqual(repr(p.parse('((2 like 2))')), 'COMPBINOP(INTEGER(2) OP_LIKE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 LIKE 2))')), 'COMPBINOP(INTEGER(2) OP_LIKE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 =~ 2))')),   'COMPBINOP(INTEGER(2) OP_LIKE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 in 2))')),   'COMPBINOP(INTEGER(2) OP_IN INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 IN 2))')),   'COMPBINOP(INTEGER(2) OP_IN INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 ~~ 2))')),   'COMPBINOP(INTEGER(2) OP_IN INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 is 2))')),   'COMPBINOP(INTEGER(2) OP_IS INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 IS 2))')),   'COMPBINOP(INTEGER(2) OP_IS INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 eq 2))')),   'COMPBINOP(INTEGER(2) OP_EQ INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 EQ 2))')),   'COMPBINOP(INTEGER(2) OP_EQ INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 == 2))')),   'COMPBINOP(INTEGER(2) OP_EQ INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 ne 2))')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 NE 2))')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 != 2))')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 <> 2))')),   'COMPBINOP(INTEGER(2) OP_NE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 ge 2))')),   'COMPBINOP(INTEGER(2) OP_GE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 GE 2))')),   'COMPBINOP(INTEGER(2) OP_GE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 >= 2))')),   'COMPBINOP(INTEGER(2) OP_GE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 gt 2))')),   'COMPBINOP(INTEGER(2) OP_GT INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 GT 2))')),   'COMPBINOP(INTEGER(2) OP_GT INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 > 2))')),    'COMPBINOP(INTEGER(2) OP_GT INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 le 2))')),   'COMPBINOP(INTEGER(2) OP_LE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 LE 2))')),   'COMPBINOP(INTEGER(2) OP_LE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 <= 2))')),   'COMPBINOP(INTEGER(2) OP_LE INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 lt 2))')),   'COMPBINOP(INTEGER(2) OP_LT INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 LT 2))')),   'COMPBINOP(INTEGER(2) OP_LT INTEGER(2))')
        self.assertEqual(repr(p.parse('((2 < 2))')),    'COMPBINOP(INTEGER(2) OP_LT INTEGER(2))')

    def test_03_basic_math(self):
        """
        Test the parsing of basic mathematical operations.
        """
        self.maxDiff = None

        p = MentatFilterParser()
        p.build()

        self.assertEqual(repr(p.parse('3 + 3')), 'MATHBINOP(INTEGER(3) OP_PLUS INTEGER(3))')
        self.assertEqual(repr(p.parse('3 - 3')), 'MATHBINOP(INTEGER(3) OP_MINUS INTEGER(3))')
        self.assertEqual(repr(p.parse('3 * 3')), 'MATHBINOP(INTEGER(3) OP_TIMES INTEGER(3))')
        self.assertEqual(repr(p.parse('3 / 3')), 'MATHBINOP(INTEGER(3) OP_DIVIDE INTEGER(3))')
        self.assertEqual(repr(p.parse('3 % 3')), 'MATHBINOP(INTEGER(3) OP_MODULO INTEGER(3))')

        self.assertEqual(repr(p.parse('(3 + 3)')), 'MATHBINOP(INTEGER(3) OP_PLUS INTEGER(3))')
        self.assertEqual(repr(p.parse('(3 - 3)')), 'MATHBINOP(INTEGER(3) OP_MINUS INTEGER(3))')
        self.assertEqual(repr(p.parse('(3 * 3)')), 'MATHBINOP(INTEGER(3) OP_TIMES INTEGER(3))')
        self.assertEqual(repr(p.parse('(3 / 3)')), 'MATHBINOP(INTEGER(3) OP_DIVIDE INTEGER(3))')
        self.assertEqual(repr(p.parse('(3 % 3)')), 'MATHBINOP(INTEGER(3) OP_MODULO INTEGER(3))')

        self.assertEqual(repr(p.parse('((3 + 3))')), 'MATHBINOP(INTEGER(3) OP_PLUS INTEGER(3))')
        self.assertEqual(repr(p.parse('((3 - 3))')), 'MATHBINOP(INTEGER(3) OP_MINUS INTEGER(3))')
        self.assertEqual(repr(p.parse('((3 * 3))')), 'MATHBINOP(INTEGER(3) OP_TIMES INTEGER(3))')
        self.assertEqual(repr(p.parse('((3 / 3))')), 'MATHBINOP(INTEGER(3) OP_DIVIDE INTEGER(3))')
        self.assertEqual(repr(p.parse('((3 % 3))')), 'MATHBINOP(INTEGER(3) OP_MODULO INTEGER(3))')

    def test_04_basic_factors(self):
        """
        Test parsing of all available factors.
        """
        self.maxDiff = None

        p = MentatFilterParser()
        p.build()

        self.assertEqual(repr(p.parse("127.0.0.1")),   "IPV4('127.0.0.1')")
        self.assertEqual(repr(p.parse("::1")),         "IPV6('::1')")
        self.assertEqual(repr(p.parse("1")),           "INTEGER(1)")
        self.assertEqual(repr(p.parse("1.1")),         "FLOAT(1.1)")
        self.assertEqual(repr(p.parse("Test")),        "VARIABLE('Test')")
        self.assertEqual(repr(p.parse('"constant1"')), "CONSTANT('constant1')")

        self.assertEqual(repr(p.parse("(127.0.0.1)")),   "IPV4('127.0.0.1')")
        self.assertEqual(repr(p.parse("(::1)")),         "IPV6('::1')")
        self.assertEqual(repr(p.parse("(1)")),           "INTEGER(1)")
        self.assertEqual(repr(p.parse("(1.1)")),         "FLOAT(1.1)")
        self.assertEqual(repr(p.parse("(Test)")),        "VARIABLE('Test')")
        self.assertEqual(repr(p.parse('("constant1")')), "CONSTANT('constant1')")

        self.assertEqual(repr(p.parse("((127.0.0.1))")),   "IPV4('127.0.0.1')")
        self.assertEqual(repr(p.parse("((::1))")),         "IPV6('::1')")
        self.assertEqual(repr(p.parse("((1))")),           "INTEGER(1)")
        self.assertEqual(repr(p.parse("((1.1))")),         "FLOAT(1.1)")
        self.assertEqual(repr(p.parse("((Test))")),        "VARIABLE('Test')")
        self.assertEqual(repr(p.parse('(("constant1"))')), "CONSTANT('constant1')")

        self.assertEqual(repr(p.parse("[127.0.0.1]")),   "LIST(IPV4('127.0.0.1'))")
        self.assertEqual(repr(p.parse("[::1]")),         "LIST(IPV6('::1'))")
        self.assertEqual(repr(p.parse("[1]")),           "LIST(INTEGER(1))")
        self.assertEqual(repr(p.parse("[1.1]")),         "LIST(FLOAT(1.1))")
        self.assertEqual(repr(p.parse("[Test]")),        "LIST(VARIABLE('Test'))")
        self.assertEqual(repr(p.parse('["constant1"]')), "LIST(CONSTANT('constant1'))")

        self.assertEqual(repr(p.parse("[127.0.0.1 , 127.0.0.2]")),        "LIST(IPV4('127.0.0.1'), IPV4('127.0.0.2'))")
        self.assertEqual(repr(p.parse("[::1 , ::2]")),                    "LIST(IPV6('::1'), IPV6('::2'))")
        self.assertEqual(repr(p.parse("[1,2, 3,4 , 5]")),                 "LIST(INTEGER(1), INTEGER(2), INTEGER(3), INTEGER(4), INTEGER(5))")
        self.assertEqual(repr(p.parse("[1.1,2.2, 3.3,4.4 , 5.5]")),       "LIST(FLOAT(1.1), FLOAT(2.2), FLOAT(3.3), FLOAT(4.4), FLOAT(5.5))")
        self.assertEqual(repr(p.parse("[Var1,Var2, Var3,Var4 , Var5 ]")), "LIST(VARIABLE('Var1'), VARIABLE('Var2'), VARIABLE('Var3'), VARIABLE('Var4'), VARIABLE('Var5'))")
        self.assertEqual(repr(p.parse('["c1","c2", "c3","c4" , "c5" ]')), "LIST(CONSTANT('c1'), CONSTANT('c2'), CONSTANT('c3'), CONSTANT('c4'), CONSTANT('c5'))")

    def test_05_advanced(self):
        """
        Test parsing of advanced filtering expressions.
        """
        self.maxDiff = None

        p = MentatFilterParser()
        p.build()

        self.assertEqual(repr(p.parse('Category in ["Abusive.Spam" , "Attempt.Exploit"]')), "COMPBINOP(VARIABLE('Category') OP_IN LIST(CONSTANT('Abusive.Spam'), CONSTANT('Attempt.Exploit')))")
        self.assertEqual(repr(p.parse('Category is ["Abusive.Spam" , "Attempt.Exploit"]')), "COMPBINOP(VARIABLE('Category') OP_IS LIST(CONSTANT('Abusive.Spam'), CONSTANT('Attempt.Exploit')))")
        self.assertEqual(repr(p.parse('Node.Name in ["cz.cesnet.labrea"]')), "COMPBINOP(VARIABLE('Node.Name') OP_IN LIST(CONSTANT('cz.cesnet.labrea')))")
        self.assertEqual(repr(p.parse('Source.IP4 in [127.0.0.1 , 127.0.0.2]')), "COMPBINOP(VARIABLE('Source.IP4') OP_IN LIST(IPV4('127.0.0.1'), IPV4('127.0.0.2')))")
        self.assertEqual(repr(p.parse('(Source.IP4 eq 127.0.0.1) or (Node[#].Name is "cz.cesnet.labrea")')), "LOGBINOP(COMPBINOP(VARIABLE('Source.IP4') OP_EQ IPV4('127.0.0.1')) OP_OR COMPBINOP(VARIABLE('Node[#].Name') OP_IS CONSTANT('cz.cesnet.labrea')))")

if __name__ == '__main__':
    unittest.main()
