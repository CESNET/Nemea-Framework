# unirec.py - python version of UniRec structures/functions
# Author: Vaclav Bartos (ibartosv@fit.vutbr.cz), 2013
# Author: Tomas Cejka (cejkat@cesnet.cz), 2015

import struct
import sys
import os.path
from textwrap import dedent
from keyword import iskeyword
from distutils.sysconfig import get_python_lib
from .ur_types import *

if sys.version_info > (3,):
   long = int
   str = str
   unicode = str
   bytes = bytes
   basestring = (str,bytes)
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
   else:
      return cmp(f1, f2)



# Inspired by "Records" python recipe by George Sakkis available at:
# http://code.activestate.com/recipes/576555/
def CreateTemplate(template_name, field_names, verbose=False):
   '''Returns a new UniRec class.'''
   global FIELDS
   # Validate template name
   if not min(c.isalnum() or c=='_' for c in template_name):
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

   field_names = tuple(sorted(field_names,key=lambda f1: FIELDS[f1].size, reverse=True))

   # Validate fields
   if not field_names:
       raise ValueError('Template must have at least one field')
   seen_names = set()
   for name in field_names:
      if name not in FIELDS.iterkeys():
         raise ValueError('Unknown field: %r' % name)
      if name in seen_names:
         raise ValueError('Encountered duplicate field: %r' % name)
      seen_names.add(name)

   # Create and fill-in the class template
   numfields = len(field_names)
   minsize = sum((FIELDS[f].size if FIELDS[f].size != -1 else 4) for f in field_names)
   argtxt = ', '.join(field_names)
   fieldnamestxt = ', '.join('%r' % f for f in field_names)
   valuestxt = ', '.join('self.%s' % f for f in field_names)
   strtxt = ', '.join('%s=%%s' % f for f in field_names)
   dicttxt = ', '.join('%r: self.%s' % (f,f) for f in field_names)
   tupletxt = repr(tuple('self.%s' % f for f in field_names)).replace("'",'')
   inittxt = '; '.join('self.%s=%s()' % (f,FIELDS[f].python_type.__name__) for f in field_names)
   itertxt = '; '.join('yield (%r, self.%s)' % (f,f) for f in field_names)
   eqtxt   = ' and '.join('self.%s==other.%s' % (f,f) for f in field_names)
   typedict = '{' + ', '.join('%r : %s' % (f, FIELDS[f].python_type.__name__) for f in field_names) + '}'
   stat_field_names = filter(lambda f: FIELDS[f].size != -1, field_names)
   dyn_field_names = filter(lambda f: FIELDS[f].size == -1, field_names)
   staticfmt = "="+''.join(FIELDS[f].struct_type for f in stat_field_names)
   staticvalues = ', '.join(\
                  "self.%s.toUniRec()" % f if (FIELDS[f].struct_type[-1] == "s" and FIELDS[f].python_type != str) else "self.%s" % f \
                  for f in stat_field_names)
   staticsize = sum(FIELDS[f].size for f in stat_field_names)
   initstatic = 't = struct.unpack_from("%s", data, 0);' % staticfmt + \
                '; '.join(\
                   "object.__setattr__(self, '%s', %s.fromUniRec(t[%d]))" % (f, FIELDS[f].python_type.__name__, i) \
                      if hasattr(FIELDS[f].python_type, 'fromUniRec') \
                   else "object.__setattr__(self, '%s', t[%d])" % (f,i) \
                   for i,f in enumerate(stat_field_names)
                )
   offsetcode = ('o = 0; ' + \
                '; '.join('l = len(self.%s); s += struct.pack("=HH", o, l); o += l' % f for f in dyn_field_names)) \
                if len(dyn_field_names) > 0 else ''
   dynfieldcode = '; '.join('s += self.%s' % f for f in dyn_field_names)
   tuplestatic = repr(tuple('self.%s' % f for f in stat_field_names)).replace("'",'')
   unpackdyncode = '; offset += 4; '.join(\
                   'start,length = struct.unpack_from("=HH", data, offset); self.%s = data[%d+start : %d+start+length]' % (f, minsize, minsize) \
                   for f in dyn_field_names\
                   )
   class_code = dedent('''
      import struct
      class %(template_name)s(object):
         "UniRec template %(template_name)s(%(argtxt)s) for data manipulation."

         __slots__  = ('_field_types', %(fieldnamestxt)s)

         _field_types = %(typedict)s

         def __init__(self, data=None):
            if data is None:
               %(inittxt)s
            elif isinstance(data, str):
               %(initstatic)s
               #(%(tuplestatic)s) = struct.unpack_from("%(staticfmt)s", data, 0)
               offset = %(staticsize)d
               last_end = %(minsize)d
               %(unpackdyncode)s
            else:
               raise TypeError("%%s() argument must be a string or None, not '%%s'" %% (%(template_name)r, type(data).__name__))

         def __len__(self):
            "Return number of fields of the template"
            return %(numfields)d

         @staticmethod
         def minsize():
            """Return minimal size of a record with this template, i.e. size
            of the static part, in bytes."""
            return %(minsize)d

         @staticmethod
         def fields():
            """Return list of names of all fields"""
            return [%(fieldnamestxt)s]

         def __iter__(self):
            %(itertxt)s

         def todict(self):
            "Return a new dict which maps field names to their values"
            return {%(dicttxt)s}

         def __repr__(self):
            return '%(template_name)s(%%r)' %% self.serialize()

         def __str__(self):
            return '%(strtxt)s' %% %(tupletxt)s

         def __eq__(self, other):
            return isinstance(other, self.__class__) and %(eqtxt)s

         def __ne__(self, other):
            return not self==other

         def __getstate__(self):
            return %(tupletxt)s

         def __setstate__(self, state):
            %(tupletxt)s = state

         def __setattr__(self, attr, value):
            try:
               field_type = self._field_types[attr]
            except KeyError:
               raise AttributeError("'%(template_name)s' object has no attribute '%%s'" %% attr)
            if not isinstance(value, field_type):
               if isinstance(value, str) and hasattr(field_type, 'fromUniRec'):
                  value = field_type.fromUniRec(value)
               elif isinstance(value, int) and field_type == long:
                  value = long(value)
               else:
                  raise TypeError("'%%s' is of type '%%s', not '%%s'" %% (attr, field_type.__name__, type(value).__name__))
            object.__setattr__(self, attr, value)

         def serialize(self):
            # Fill static fields
            s = struct.pack("%(staticfmt)s", %(staticvalues)s)
            # Fill offsets of dynamic fields
            %(offsetcode)s
            # Fill dynamic fields
            %(dynfieldcode)s
            return s
   ''') % locals()
   # Execute the template string in a temporary namespace
   namespace = {'IPAddr':IPAddr, 'Timestamp':Timestamp}
   try:
      exec(class_code) in namespace
      if verbose: print(class_code)
   except SyntaxError as e:
      raise SyntaxError(e.message + ':\n' + class_code)
   cls = namespace[template_name]
   # For pickling to work, the __module__ variable needs to be set to the frame
   # where the named tuple is created.  Bypass this step in enviroments where
   # sys._getframe is not defined (Jython for example).
   if hasattr(sys, '_getframe') and sys.platform != 'cli':
      cls.__module__ = sys._getframe(1).f_globals['__name__']
   return cls


# if __name__ == '__main__':
#    import doctest
#    TestResults = recordtype('TestResults', 'failed, attempted')
#    print TestResults(*doctest.testmod())
