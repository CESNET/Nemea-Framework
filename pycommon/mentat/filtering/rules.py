#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
# This file is part of Mentat system (https://mentat.cesnet.cz/).
#
# Copyright (C) since 2011 CESNET, z.s.p.o (http://www.ces.net/)
# Use of this source is governed by the MIT license, see LICENSE file.
#-------------------------------------------------------------------------------

"""
This module contains implementation of object representations of filtering
and query language grammar.

There is a separate class defined for each grammar rule. There are following
classes representing all possible constant and variable values (tree leaves,
without child nodes):

* :py:class:`VariableRule`
* :py:class:`ConstantRule`
* :py:class:`IPv4Rule`
* :py:class:`IPv6Rule`
* :py:class:`IntegerRule`
* :py:class:`FloatRule`
* :py:class:`ListRule`

There are following classes representing various binary and unary operations:

* :py:class:`LogicalBinOpRule`
* :py:class:`ComparisonBinOpRule`
* :py:class:`MathBinOpRule`
* :py:class:`UnaryOperationRule`

Desired hierarchical rule tree can be created either programatically, or by
parsing string rules using :py:mod:`mentat.filtering.gparser`.

Working with rule tree is then done via objects implementing rule tree
traverser interface:

* :py:class:`RuleTreeTraverser`

The provided :py:class:`RuleTreeTraverser` class contains also implementation of
all necessary evaluation methods.

There is a simple example implementation of rule tree traverser capable of
printing rule tree into a formated string:

* :py:class:`PrintingTreeTraverser`

Rule evaluation
^^^^^^^^^^^^^^^

* Logical operations ``and or xor not exists``

  There is no special handling for operands of logical operations. Operand(s) are
  evaluated in logical expression exactly as they are received, there is no
  mangling involved.

* Comparison operations

    All comparison operations are designed to work with lists as both operands.
    This is because :py:func:`mentat.filtering.jpath.jpath_values` function is
    used to retrieve variable values and this function always returns list.

    * Operation: ``is``

      Like in the case of logical operations, there is no mangling involved when
      evaluating this operation. Both operands are compared using Python`s native
      ``is`` operation and result is returned.

    * Operation: ``in``

      In this case left operand is iterated and each value is compared using Python`s
      native ``in`` operation with right operand. First ``True`` result wins and
      operation immediatelly returns ``True``, ``False`` is returned otherwise.

    * Any other operation: ``like eq ne gt ge lt le``

      In case of this operation both of the operands are iterated and each one is
      compared with each other. First ``True`` result wins and operation immediatelly
      returns ``True``, ``False`` is returned otherwise.

    * Math operations: ``+ - * / %``

      Current math operation implementation supports following options:

        * Both operands are lists of the same length. In this case corresponding
          elements at certain position within the list are evaluated with given
          operation. Result is a list.

        * One of the operands is a list, second is scalar value or list of the
          size 1. In this case given operation is evaluated with each element of
          the longer list. Result is a list.

        * Operands are lists of the different size. This option is **forbidden**
          and the result is ``None``.

"""

__version__ = "0.1"
__author__ = "Jan Mach <jan.mach@cesnet.cz>"
__credits__ = "Pavel Kácha <pavel.kacha@cesnet.cz>, Andrea Kropáčová <andrea.kropacova@cesnet.cz>"

import collections
import re
import datetime

class FilteringRuleException(Exception):
    """
    Custom filtering rule specific exception.

    This exception will be thrown on module specific errors.
    """
    def __init__(self, description):
        self._description = description
    def __str__(self):
        return repr(self._description)

class Rule():
    """
    Base class for all filter tree rules.
    """
    pass

class ValueRule(Rule):
    """
    Base class for all filter tree value rules.
    """
    pass

class VariableRule(ValueRule):
    """
    Class for expression variables.
    """
    def __init__(self, value):
        """
        Initialize the variable with given path value.
        """
        self.value = value

    def __str__(self):
        return "{}".format(self.value)

    def __repr__(self):
        return "VARIABLE({})".format(repr(self.value))

    def traverse(self, traverser, **kwargs):
        return traverser.variable(self, **kwargs)

class ConstantRule(ValueRule):
    """
    Class for all expression constant values.
    """
    def __init__(self, value):
        """
        Initialize the constant with given value.
        """
        self.value = value

    def __str__(self):
        return '"{}"'.format(self.value)

    def __repr__(self):
        return "CONSTANT({})".format(repr(self.value))

    def traverse(self, traverser, **kwargs):
        return traverser.constant(self, **kwargs)

class IPV4Rule(ConstantRule):
    """
    Class for IPv4 address constants.
    """
    def __str__(self):
        return '{}'.format(self.value)

    def __repr__(self):
        return "IPV4({})".format(repr(self.value))

    def traverse(self, traverser, **kwargs):
        return traverser.ipv4(self, **kwargs)

class IPV6Rule(ConstantRule):
    """
    Class for IPv6 address constants.
    """
    def __str__(self):
        return '{}'.format(self.value)

    def __repr__(self):
        return "IPV6({})".format(repr(self.value))

    def traverse(self, traverser, **kwargs):
        return traverser.ipv6(self, **kwargs)

class NumberRule(ConstantRule):
    """
    Base class for all numerical constants.
    """
    pass

class IntegerRule(NumberRule):
    """
    Class for integer constants.
    """
    def __str__(self):
        return '{}'.format(self.value)

    def __repr__(self):
        return "INTEGER({})".format(repr(self.value))

    def traverse(self, traverser, **kwargs):
        return traverser.integer(self, **kwargs)

class FloatRule(NumberRule):
    """
    Class for float constants.
    """
    def __str__(self):
        return '{}'.format(self.value)

    def __repr__(self):
        return "FLOAT({})".format(repr(self.value))

    def traverse(self, traverser, **kwargs):
        return traverser.float(self, **kwargs)

class ListRule(ValueRule):
    """
    Base class for all filter tree rules.
    """
    def __init__(self, rule, next_rule = None):
        """
        Initialize the constant with given value.
        """
        if not isinstance(rule, list):
            rule = [rule]
        self.value = rule
        if next_rule:
            self.value += next_rule.value

    def __str__(self):
        return '[{}]'.format(', '.join([str(v) for v in self.value]))

    def __repr__(self):
        return "LIST({})".format(', '.join([repr(v) for v in self.value]))

    def traverse(self, traverser, **kwargs):
        return traverser.list(self, **kwargs)

class OperationRule(Rule):
    """
    Base class for all expression operations (both unary and binary).
    """
    pass

class BinaryOperationRule(OperationRule):
    """
    Base class for all binary operations.
    """
    def __init__(self, operation, left, right):
        """
        Initialize the object with operation type and both operands.
        """
        self.operation = operation
        self.left = left
        self.right = right

    def __str__(self):
        return "({} {} {})".format(str(self.left), str(self.operation), str(self.right))

class LogicalBinOpRule(BinaryOperationRule):
    """
    Base class for all logical binary operations.
    """
    def __repr__(self):
        return "LOGBINOP({} {} {})".format(repr(self.left), str(self.operation), repr(self.right))

    def traverse(self, traverser, **kwargs):
        lr = self.left.traverse(traverser, **kwargs)
        rr = self.right.traverse(traverser, **kwargs)
        return traverser.binary_operation_logical(self, lr, rr, **kwargs)

class ComparisonBinOpRule(BinaryOperationRule):
    """
    Base class for all comparison binary operations.
    """
    def __repr__(self):
        return "COMPBINOP({} {} {})".format(repr(self.left), str(self.operation), repr(self.right))

    def traverse(self, traverser, **kwargs):
        lr = self.left.traverse(traverser, **kwargs)
        rr = self.right.traverse(traverser, **kwargs)
        return traverser.binary_operation_comparison(self, lr, rr, **kwargs)

class MathBinOpRule(BinaryOperationRule):
    """
    Base class for all mathematical binary operations.
    """
    def __repr__(self):
        return "MATHBINOP({} {} {})".format(repr(self.left), str(self.operation), repr(self.right))

    def traverse(self, traverser, **kwargs):
        lr = self.left.traverse(traverser, **kwargs)
        rr = self.right.traverse(traverser, **kwargs)
        return traverser.binary_operation_math(self, lr, rr, **kwargs)

class UnaryOperationRule(OperationRule):
    """
    Base class for all unary operations.
    """
    def __init__(self, operation, right):
        """
        Initialize the object with operation type operand.
        """
        self.operation = operation
        self.right = right

    def __str__(self):
        return "({} {})".format(str(self.operation), str(self.right))

    def __repr__(self):
        return "UNOP({} {})".format(str(self.operation), repr(self.right))

    def traverse(self, traverser, **kwargs):
        rr = self.right.traverse(traverser, **kwargs)
        return traverser.unary_operation(self, rr, **kwargs)

def _to_numeric(val):
    """
    Helper function for conversion of various data types into numeric representation.
    """
    if isinstance(val, int) or isinstance(val, float):
        return val
    if isinstance(val, datetime.datetime):
        return val.timestamp()
    return float(val)

class RuleTreeTraverser():
    """
    Base class for all rule tree traversers.
    """

    """
    Definitions of all logical binary operations.
    """
    binops_logical = {
        'OP_OR':    lambda x, y : x or y,
        'OP_XOR':   lambda x, y : (x and not y) or (not x and y),
        'OP_AND':   lambda x, y : x and y,
        'OP_OR_P':  lambda x, y : x or y,
        'OP_XOR_P': lambda x, y : (x and not y) or (not x and y),
        'OP_AND_P': lambda x, y : x and y,
    }

    """
    Definitions of all comparison binary operations.
    """
    binops_comparison = {
        'OP_LIKE': lambda x, y : re.search(y, x),
        'OP_IN':   lambda x, y : x in y,
        'OP_IS':   lambda x, y : x == y,
        'OP_EQ':   lambda x, y : x == y,
        'OP_NE':   lambda x, y : x != y,
        'OP_GT':   lambda x, y : x > y,
        'OP_GE':   lambda x, y : x >= y,
        'OP_LT':   lambda x, y : x < y,
        'OP_LE':   lambda x, y : x <= y,
    }

    """
    Definitions of all mathematical binary operations.
    """
    binops_math = {
        'OP_PLUS':   lambda x, y : x + y,
        'OP_MINUS':  lambda x, y : x - y,
        'OP_TIMES':  lambda x, y : x * y,
        'OP_DIVIDE': lambda x, y : x / y,
        'OP_MODULO': lambda x, y : x % y,
    }

    """
    Definitions of all unary operations.
    """
    unops = {
        'OP_NOT':    lambda x : not x,
        'OP_EXISTS': lambda x : x,
    }

    def evaluate_binop_logical(self, operation, left, right, **kwargs):
        """
        Evaluate given logical binary operation with given operands.
        """
        if not operation in self.binops_logical:
            raise Exception("Invalid logical binary operation '{}'".format(operation))
        result = self.binops_logical[operation](left, right)
        if result:
            return True
        else:
            return False

    def evaluate_binop_comparison(self, operation, left, right, **kwargs):
        """
        Evaluate given comparison binary operation with given operands.
        """
        if not operation in self.binops_comparison:
            raise Exception("Invalid comparison binary operation '{}'".format(operation))
        if left is None or right is None:
            return None
        if not isinstance(left, list):
            left = [left]
        if not isinstance(right, list):
            right = [right]
        if not len(left) or not len(right):
            return None
        if operation in ['OP_IS']:
            res = self.binops_comparison[operation](left, right)
            if res:
                return True
        elif operation in ['OP_IN']:
            for l in left:
                res = self.binops_comparison[operation](l, right)
                if res:
                    return True
        else:
            for l in left:
                if l is None:
                    continue
                for r in right:
                    if r is None:
                        continue
                    res = self.binops_comparison[operation](l, r)
                    if res:
                        return True
        return False

    def _calculate_vector(self, operation, left, right):
        """

        """
        result = []
        if len(right) == 1:
            right = _to_numeric(right[0])
            for l in left:
                l  = _to_numeric(l)
                result.append(self.binops_math[operation](l, right))
        elif len(left) == 1:
            left = _to_numeric(left[0])
            for r in right:
                r  = _to_numeric(r)
                result.append(self.binops_math[operation](left, r))
        elif len(left) == len(right):
            for l, r in zip(left, right):
                l  = _to_numeric(l)
                r  = _to_numeric(r)
                result.append(self.binops_math[operation](l, r))
        else:
            raise FilteringRuleException("Uneven lenth of math operation '{}' operands".format(operation))
        return result

    def evaluate_binop_math(self, operation, left, right, **kwargs):
        """
        Evaluate given mathematical binary operation with given operands.
        """
        if not operation in self.binops_math:
            raise Exception("Invalid math binary operation '{}'".format(operation))
        if left is None or right is None:
            return None
        if not isinstance(left, list):
            left = [left]
        if not isinstance(right, list):
            right = [right]
        if not len(left) or not len(right):
            return None
        try:
            v = self._calculate_vector(operation, left, right)
            if len(v) > 1:
                return v
            else:
                return v[0]
        except:
            return None

    def evaluate_unop(self, operation, right, **kwargs):
        """
        Evaluate given unary operation with given operand.
        """
        if not operation in self.unops:
            raise Exception("Invalid unary operation '{}'".format(operation))
        if right is None:
            return None
        return self.unops[operation](right)

    def evaluate(self, operation, *args):
        """
        Master method for evaluating any operation (both unary and binary).
        """
        if operation in self.binops_comparison:
            return self.evaluate_binop_comparison(rule, *args)
        if operation in self.binops_logical:
            return self.evaluate_binop_logical(rule, *args)
        if operation in self.binops_math:
            return self.evaluate_binop_math(rule, *args)
        if operation in self.unops:
            return self.evaluate_unop(rule, *args)
        raise Exception("Invalid operation '{}'".format(operation))

class PrintingTreeTraverser(RuleTreeTraverser):
    """
    Demonstation of simple rule tree traverser - printing traverser.
    """
    def ipv4(self, rule, **kwargs):
        return "IPV4({})".format(rule.value)
    def ipv6(self, rule, **kwargs):
        return "IPV6({})".format(rule.value)
    def integer(self, rule, **kwargs):
        return "INTEGER({})".format(rule.value)
    def float(self, rule, **kwargs):
        return "FLOAT({})".format(rule.value)
    def constant(self, rule, **kwargs):
        return "CONSTANT({})".format(rule.value)
    def variable(self, rule, **kwargs):
        return "VARIABLE({})".format(rule.value)
    def list(self, rule, **kwargs):
        return "LIST({})".format(', '.join([str(v) for v in rule.value]))
    def binary_operation_logical(self, rule, left, right, **kwargs):
        return "LOGBINOP({};{};{})".format(rule.operation, left, right)
    def binary_operation_comparison(self, rule, left, right, **kwargs):
        return "COMPBINOP({};{};{})".format(rule.operation, left, right)
    def binary_operation_math(self, rule, left, right, **kwargs):
        return "MATHBINOP({};{};{})".format(rule.operation, left, right)
    def unary_operation(self, rule, right, **kwargs):
        return "UNOP({};{})".format(rule.operation, right)

if __name__ == "__main__":
    """
    Perform the demonstration.
    """
    print("* Rule usage:")
    rule_var = VariableRule("Test")
    print("STR:  {}".format(str(rule_var)))
    print("REPR: {}".format(repr(rule_var)))
    rule_const = ConstantRule("constant")
    print("STR:  {}".format(str(rule_const)))
    print("REPR: {}".format(repr(rule_const)))
    rule_ipv4 = IPV4Rule("127.0.0.1")
    print("STR:  {}".format(str(rule_ipv4)))
    print("REPR: {}".format(repr(rule_ipv4)))
    rule_ipv6 = IPV6Rule("::1")
    print("STR:  {}".format(str(rule_ipv6)))
    print("REPR: {}".format(repr(rule_ipv6)))
    rule_integer = IntegerRule(15)
    print("STR:  {}".format(str(rule_integer)))
    print("REPR: {}".format(repr(rule_integer)))
    rule_float = FloatRule(15.5)
    print("STR:  {}".format(str(rule_float)))
    print("REPR: {}".format(repr(rule_float)))
    rule_binop_l = LogicalBinOpRule('OP_OR', rule_var, rule_integer)
    print("STR:  {}".format(str(rule_binop_l)))
    print("REPR: {}".format(repr(rule_binop_l)))
    rule_binop_c = ComparisonBinOpRule('OP_GT', rule_var, rule_integer)
    print("STR:  {}".format(str(rule_binop_c)))
    print("REPR: {}".format(repr(rule_binop_c)))
    rule_binop_m = MathBinOpRule('OP_PLUS', rule_var, rule_integer)
    print("STR:  {}".format(str(rule_binop_m)))
    print("REPR: {}".format(repr(rule_binop_m)))
    rule_binop = LogicalBinOpRule('OP_OR', ComparisonBinOpRule('OP_GT', MathBinOpRule('OP_PLUS', VariableRule("Test"), IntegerRule(10)), IntegerRule(20)), ComparisonBinOpRule('OP_LT', VariableRule("Test"), IntegerRule(5)))
    print("STR:  {}".format(str(rule_binop)))
    print("REPR: {}".format(repr(rule_binop)))
    rule_unop = UnaryOperationRule('OP_NOT', rule_var)
    print("STR:  {}".format(str(rule_unop)))
    print("REPR: {}".format(repr(rule_unop)))

    print("\n* Traverser usage:")
    traverser = PrintingTreeTraverser()
    print("{}".format(rule_binop_l.traverse(traverser)))
    print("{}".format(rule_binop_c.traverse(traverser)))
    print("{}".format(rule_binop_m.traverse(traverser)))
    print("{}".format(rule_binop.traverse(traverser)))
    print("{}".format(rule_unop.traverse(traverser)))
