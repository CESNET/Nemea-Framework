#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
# This file is part of Mentat system (https://mentat.cesnet.cz/).
#
# Copyright (C) since 2011 CESNET, z.s.p.o (http://www.ces.net/)
# Use of this source is governed by the MIT license, see LICENSE file.
#-------------------------------------------------------------------------------

"""
This module contains object encapsulation of `PLY <http://www.dabeaz.com/ply/>`__
parser for filtering and query language grammar used in Mentat project.

Grammar features
^^^^^^^^^^^^^^^^

* Logical operations: ``and or xor not exists``

  All logical operations support upper case and lower case name variants.
  Additionally, there are also symbolic variants ``|| ^^ && ! ?`` with higher
  priority and which can be used in some cases instead of parentheses.

* Comparison operations: ``like in is eq ne gt ge lt le``

  All comparison operations support upper case and lower case name variants.
  Additionally, there are also symbolic variants.

* Mathematical operations: ``+ - * / %``
* JPath variables: ``Source[0].IP4[1]``
* Directly recognized constants:

    * IPv4: ``127.0.0.1 127.0.0.1/32 127.0.0.1-127.0.0.5 127.0.0.1..127.0.0.5``
    * IPv6: ``::1 ::1/64 ::1-::5 ::1..::5``
    * integer: ``0 1 42``
    * float: ``3.14159``
* Quoted literal constants: ``"double quotes"`` or ``'single quotes'``

For more details on supported grammar token syntax please see the documentation
of :py:mod:`mentat.filtering.lexer` module.

Currently implemented grammar
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bnf

    expression : xor_expression OP_OR expression
               | xor_expression

    xor_expression : and_expression OP_XOR xor_expression
                   | and_expression

    and_expression : or_p_expression OP_AND and_expression
                   | or_p_expression

    or_p_expression : xor_p_expression OP_OR_P or_p_expression
                    | xor_p_expression

    xor_p_expression : and_p_expression OP_XOR_P xor_p_expression
                     | and_p_expression

    and_p_expression : not_expression OP_AND_P and_p_expression
                     | not_expression

    not_expression : OP_NOT ex_expression
                   | ex_expression

    ex_expression : OP_EXISTS cmp_expression
                  | cmp_expression

    cmp_expression : term OP_LIKE cmp_expression
                   | term OP_IN cmp_expression
                   | term OP_IS cmp_expression
                   | term OP_EQ cmp_expression
                   | term OP_NE cmp_expression
                   | term OP_GT cmp_expression
                   | term OP_GE cmp_expression
                   | term OP_LT cmp_expression
                   | term OP_LE cmp_expression
                   | term

    term : factor OP_PLUS term
         | factor OP_MINUS term
         | factor OP_TIMES term
         | factor OP_DIVIDE term
         | factor OP_MODULO term
         | factor

    factor : IPV4
           | IPV6
           | INTEGER
           | FLOAT
           | VARIABLE
           | CONSTANT
           | LBRACK list RBRACK
           | LPAREN expression RPAREN

    list : IPV4
         | IPV6
         | INTEGER
         | FLOAT
         | VARIABLE
         | CONSTANT
         | IPV4 COMMA list
         | IPV6 COMMA list
         | INTEGER COMMA list
         | FLOAT COMMA list
         | VARIABLE COMMA list
         | CONSTANT COMMA list

.. note::

    Implementation of this module is very *PLY* specific, please read the
    appropriate `documentation <http://www.dabeaz.com/ply/ply.html#ply_nn3>`__
    to understand it.

"""

__version__ = "0.1"
__author__ = "Jan Mach <jan.mach@cesnet.cz>"
__credits__ = "Pavel Kácha <pavel.kacha@cesnet.cz>, Andrea Kropáčová <andrea.kropacova@cesnet.cz>"

import re, logging
import ply.yacc

from mentat.filtering.lexer import MentatFilterLexer
from mentat.filtering.rules import *

class MentatFilterParser():
    """
    Object encapsulation of *PLY* parser implementation for filtering and
    query language grammar used in Mentat project.
    """

    def build(self, **kwargs):
        """
        Build/rebuild the parser object
        """
        self.logger = logging.getLogger('ply_parser')

        self.lexer = MentatFilterLexer()
        self.lexer.build()

        self.tokens = self.lexer.tokens

        self.parser = ply.yacc.yacc(
            module=self,
            outputdir='/tmp'
            #start='statements',
            #debug=yacc_debug,
            #optimize=yacc_optimize,
            #tabmodule=yacctab
        )

    def parse(self, data, filename='', debuglevel=0):
        """
        Parse given data.

            data:
                A string containing the filter definition
            filename:
                Name of the file being parsed (for meaningful
                error messages)
            debuglevel:
                Debug level to yacc
        """
        self.lexer.filename = filename
        self.lexer.reset_lineno()
        if not data or data.isspace():
            return []
        else:
            return self.parser.parse(data, lexer=self.lexer, debug=debuglevel)

    #---------------------------------------------------------------------------

    def _create_factor_rule(self, t):
        """
        Simple helper method for creating factor node objects based on node name.
        """
        if (t[0] == 'IPV4'):
            return IPV4Rule(t[1])
        elif (t[0] == 'IPV6'):
            return IPV6Rule(t[1])
        elif (t[0] == 'INTEGER'):
            return IntegerRule(t[1])
        elif (t[0] == 'FLOAT'):
            return FloatRule(t[1])
        elif (t[0] == 'VARIABLE'):
            return VariableRule(t[1])
        else:
            return ConstantRule(t[1])

    def p_expression(self, t):
        """expression : xor_expression OP_OR expression
                      | xor_expression"""
        if (len(t) == 4):
            t[0] = LogicalBinOpRule(t[2], t[1], t[3])
        else:
            t[0] = t[1]

    def p_xor_expression(self, t):
        """xor_expression : and_expression OP_XOR xor_expression
                          | and_expression"""
        if (len(t) == 4):
            t[0] = LogicalBinOpRule(t[2], t[1], t[3])
        else:
            t[0] = t[1]

    def p_and_expression(self, t):
        """and_expression : or_p_expression OP_AND and_expression
                          | or_p_expression"""
        if (len(t) == 4):
            t[0] = LogicalBinOpRule(t[2], t[1], t[3])
        else:
            t[0] = t[1]

    def p_or_p_expression(self, t):
        """or_p_expression : xor_p_expression OP_OR_P or_p_expression
                      | xor_p_expression"""
        if (len(t) == 4):
            t[0] = LogicalBinOpRule(t[2], t[1], t[3])
        else:
            t[0] = t[1]

    def p_xor_p_expression(self, t):
        """xor_p_expression : and_p_expression OP_XOR_P xor_p_expression
                          | and_p_expression"""
        if (len(t) == 4):
            t[0] = LogicalBinOpRule(t[2], t[1], t[3])
        else:
            t[0] = t[1]

    def p_and_p_expression(self, t):
        """and_p_expression : not_expression OP_AND_P and_p_expression
                          | not_expression"""
        if (len(t) == 4):
            t[0] = LogicalBinOpRule(t[2], t[1], t[3])
        else:
            t[0] = t[1]

    def p_not_expression(self, t):
        """not_expression : OP_NOT ex_expression
                          | ex_expression"""
        if (len(t) == 3):
            t[0] = UnaryOperationRule(t[1], t[2])
        else:
            t[0] = t[1]

    def p_ex_expression(self, t):
        """ex_expression : OP_EXISTS cmp_expression
                         | cmp_expression"""
        if (len(t) == 3):
            t[0] = UnaryOperationRule(t[1], t[2])
        else:
            t[0] = t[1]

    def p_cmp_expression(self, t):
        """cmp_expression : term OP_LIKE cmp_expression
                          | term OP_IN cmp_expression
                          | term OP_IS cmp_expression
                          | term OP_EQ cmp_expression
                          | term OP_NE cmp_expression
                          | term OP_GT cmp_expression
                          | term OP_GE cmp_expression
                          | term OP_LT cmp_expression
                          | term OP_LE cmp_expression
                          | term"""
        if (len(t) == 4):
            t[0] = ComparisonBinOpRule(t[2], t[1], t[3])
        else:
            t[0] = t[1]

    def p_term(self, t):
        """term : factor OP_PLUS term
                | factor OP_MINUS term
                | factor OP_TIMES term
                | factor OP_DIVIDE term
                | factor OP_MODULO term
                | factor"""
        if (len(t) == 4):
            t[0] = MathBinOpRule(t[2], t[1], t[3])
        else:
            t[0] = t[1]

    def p_factor(self, t):
        """factor : IPV4
                  | IPV6
                  | INTEGER
                  | FLOAT
                  | VARIABLE
                  | CONSTANT
                  | LBRACK list RBRACK
                  | LPAREN expression RPAREN"""
        if (len(t) == 2):
            t[0] = self._create_factor_rule(t[1])
        else:
            t[0] = t[2]

    def p_list(self, t):
        """list : IPV4
                | IPV6
                | INTEGER
                | FLOAT
                | VARIABLE
                | CONSTANT
                | IPV4 COMMA list
                | IPV6 COMMA list
                | INTEGER COMMA list
                | FLOAT COMMA list
                | VARIABLE COMMA list
                | CONSTANT COMMA list"""
        n = self._create_factor_rule(t[1])
        if (len(t) == 2):
            t[0] = ListRule(n)
        else:
            t[0] = ListRule(n, t[3])

    def p_error(self, t):
        print("Syntax error at '%s'" % t.value)

if __name__ == "__main__":
    """
    Perform the demonstration.
    """
    import pprint

    data = "1 and 1 or 1 xor 1"

    # Build the parser and try it out
    m = MentatFilterParser()
    m.build()

    print("Parsing: {}".format(data))
    pprint.pprint(m.parse(data))
