# unirec.py - python version of UniRec structures/functions
# Author: Vaclav Bartos (ibartosv@fit.vutbr.cz), 2013
# Author: Tomas Cejka (cejkat@cesnet.cz), 2015

from __future__ import absolute_import
import struct
import sys
from keyword import iskeyword
from unirec.ur_types import *

if sys.version_info > (3,):
    long = int
    str = str
    unicode = str
    bytes = bytes
    basestring = (str, bytes)
    import functools  # because of backward compatibility with python 2.6 (cmp_to_key is available since Python 2.7)
    newsorted = sorted

    def sorted(to_sort, cmp):
        return newsorted(to_sort, key=functools.cmp_to_key(cmp))
else:
    str = str
    unicode = unicode
    basestring = basestring

    def bytes(string, encoding):
        return str(string)

FIELD_GROUPS = {
}


def getFieldSpec(field_type):
    pt = python_types[field_type]
    return FieldSpec(size_table[field_type], pt[0], pt[1])


def genFieldsFromNegotiation(fmtspec):
    fields = fmtspec.split(b',')
    names = [(f.split(b' '))[1] for f in fields]
    types = [getFieldSpec((f.split(b' '))[0]) for f in fields]
    return (names, dict(zip(names, types)))


def cmpFields(f1, f2):
    """
    Compare two fields given by their names according to their order in UniRec
    template. Return -1, 0, 1 like the built-in "cmp" function.
    """
    size1 = FIELDS[f1].size
    size2 = FIELDS[f2].size
    if (size1 < size2):
        return 1
    elif (size1 > size2):
        return -1
    else:  # the cmp function is not available in Python3
        if f1 < f2:
            return -1
        elif f1 > f2:
            return 1
        else:
            return 0


def CreateTemplate(template_name, field_names, verbose=False):
    '''Returns a new UniRec class.'''
    global FIELDS
    # Validate template name
    if not min(c.isalnum() or c == '_' for c in template_name):
        raise ValueError('Template name can only contain alphanumeric characters'
                         ' and underscores: %r' % template_name)
    if iskeyword(template_name):
        raise ValueError('Template name cannot be a keyword: %r' % template_name)
    if template_name[0].isdigit():
        raise ValueError('Template name cannot start with a number: %r' %
                         template_name)

    # Parse string with field names
    if isinstance(field_names, basestring):
        # Substitute groups
        cont = True
        while cont:
            cont = False
            for gname, gfields in FIELD_GROUPS:
                if '<'+gname+'>' in field_names:
                    field_names = field_names.replace('<'+gname+'>', gfields)
                    cont = True
                    break
        (field_names, FIELDS) = genFieldsFromNegotiation(field_names)

    field_names = tuple(sorted(field_names, cmp=cmpFields))

    # Validate fields
    if not field_names:
        raise ValueError('Template must have at least one field')
    seen_names = set()
    for name in field_names:
        if name not in list(FIELDS.keys()):
            raise ValueError('Unknown field: %r' % name)
        if name in seen_names:
            raise ValueError('Encountered duplicate field: %r' % name)
        seen_names.add(name)

    # Create and fill-in the class template
    strFIELDS = dict(zip([f.decode('ascii') for f in field_names], [FIELDS[f] for f in field_names]))
    _field_types = dict(zip([f.decode('ascii') for f in field_names], [FIELDS[f].python_type for f in field_names]))
    _staticfmt = "="+''.join(FIELDS[f].struct_type for f in field_names if FIELDS[f].size != -1)
    minimalsize = sum(FIELDS[x].size if FIELDS[x].size != -1 else 4 for x in field_names)
    staticsize = sum(FIELDS[x].size for x in field_names if FIELDS[x].size != -1)
    _slots = tuple(f.decode('ascii') for f in field_names)

    FIELDS = strFIELDS
    classdict = {'FIELDS': FIELDS, '_slots': _slots, '_field_types': _field_types, '_staticfmt': _staticfmt, 'minimalsize': minimalsize, 'staticsize': staticsize}

    nonepart = ["self." + key + " = " + _field_types[key].__name__ + "()" for key in _field_types.keys()]

    index = 0
    staticpart = []
    for key in (x for x in _slots if FIELDS[x].size != -1):
        curr_type = _field_types[key]
        if hasattr(curr_type,  'fromUniRec'):
            staticpart.append("self." + key + " = " + curr_type.__name__ + ".fromUniRec(t[" + str(index) + "])")
        else:
            staticpart.append("self." + key + " = t[" + str(index) + "]")
        index += 1

    offset = staticsize
    dynamicpart = []
    for key in (x for x in _slots if FIELDS[x].size == -1):
        dynamicpart.append("start, length = struct.unpack_from('=HH', data, " + str(offset) +")")
        dynamicpart.append("self." + key + " = data[(start + " + str(minimalsize) + "):(start + length + " + str(minimalsize) + ")]")
        offset += 4

    strinit = """def init(self, data=None):
    if data is None:
        """ + "\n        ".join(nonepart) + """ 
    elif isinstance(data, str) or isinstance(data, bytes):
        t = struct.unpack_from('""" + _staticfmt + """', data, 0)
        """ + "\n        ".join(staticpart) + """
        """ + "\n        ".join(dynamicpart) + """
    else:
        raise TypeError("%s() argument must be a string or None, not '%s'" % (__name__, type(data).__name__))
    """

    exec_scope = globals()
    exec(strinit, exec_scope)

    classdict['__init__'] = exec_scope['init']

    def length(self):
        return len(_slots)

    classdict['__len__'] = length

    @staticmethod
    def minsize():
        """Return minimal size of a record with this template, i.e. size of the static part, in bytes."""
        return minimalsize

    classdict['minsize'] = minsize

    @staticmethod
    def fields():
        """Return list of names of all fields"""
        return list(_slots)

    classdict['fields'] = fields

    def iterate(self):
        for key in _slots:
            yield (key, self.__dict__[key])

    classdict['__iter__'] = iterate

    def todict(self):
        return self.__dict__

    classdict['todict'] = todict

    def repr(self):
        return "%s(%r)" % (__name__, self.serialize)

    classdict['__repr__'] = repr

    def tostring(self):
        return ", ".join([key+"="+str(self.__dict__[key]) for key in _slots])

    classdict['__str__'] = tostring

    def eq(self, other):
        equality = isinstance(other, self.__class__)
        for key in _slots:
            equality = equality and (self.__dict__[key] == other.__dict__[key])
        return equality

    classdict['__eq__'] = eq

    def ne(self, other):
        return not self == other

    classdict['__ne__'] = ne

    def getstate(self):
        return tuple(self.__dict__[key] for key in _slots)

    classdict['__getstate__'] = getstate

    def setstate(self, state):
        index = 0
        for key in _slots:
            self.__dict__[key] = state[index]
            index += 1

    classdict['__setstate__'] = setstate

    def setattr(self, attr, value):
        try:
            field_type = _field_types[attr]
        except KeyError:
            raise AttributeError("'%s' object has no attribute '%s'" % (__name__, attr))
        if not isinstance(value, field_type):
            if isinstance(value, str) and hasattr(field_type, 'fromUniRec'):
                value = field_type.fromUniRec(value)
            elif isinstance(value, int) and field_type == long:
                value = long(value)
            else:
                raise TypeError("'%s' is of type '%s', not '%s'" % (attr, field_type.__name__, type(value).__name__))

        self.__dict__[attr] = value

    classdict['__setattr__'] = setattr

    def serialize(self):
        staticvals = tuple(self.__dict__[key].toUniRec() if (FIELDS[key].struct_type[-1] == "s" and FIELDS[key].python_type != str) else self.__dict__[key] for key in _slots if FIELDS[key].size != -1) 
        s = struct.pack(_staticfmt, *staticvals)
        offset = 0
        for key in (x for x in _slots if FIELDS[x].size == -1):
            length = len(self.__dict__[key])
            s += struct.pack("=HH", offset, length)
            offset += length
        for key in (x for x in _slots if FIELDS[x].size == -1):
            s += self.__dict__[key]
        return s

    classdict['serialize'] = serialize

    cls = type(template_name, (), classdict)
    # For pickling to work, the __module__ variable needs to be set to the frame
    # where the named tuple is created.  Bypass this step in enviroments where
    # sys._getframe is not defined (Jython for example).
    if hasattr(sys, '_getframe') and sys.platform != 'cli':
        cls.__module__ = sys._getframe(1).f_globals['__name__']
    return cls


# if __name__ == '__main__':
#     import doctest
#     TestResults = recordtype('TestResults', 'failed, attempted')
#     print TestResults(*doctest.testmod())
