#!/usr/bin/python

# Copyright (C) 2015 CESNET
#
# LICENSE TERMS
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name of the Company nor the names of its contributors
#    may be used to endorse or promote products derived from this
#    software without specific prior written permission.
#
# ALTERNATIVELY, provided that this notice is retained in full, this
# product may be distributed under the terms of the GNU General Public
# License (GPL) version 2 or later, in which case the provisions
# of the GPL apply INSTEAD OF those given above.
#
# This software is provided ``as is'', and any express or implied
# warranties, including, but not limited to, the implied warranties of
# merchantability and fitness for a particular purpose are disclaimed.
# In no event shall the company or contributors be liable for any
# direct, indirect, incidental, special, exemplary, or consequential
# damages (including, but not limited to, procurement of substitute
# goods or services; loss of use, data, or profits; or business
# interruption) however caused and on any theory of liability, whether
# in contract, strict liability, or tort (including negligence or
# otherwise) arising in any way out of the use of this software, even
# if advised of the possibility of such damage.

import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), "python", "unirec"))
sys.path.append(os.path.join(os.path.dirname(__file__), "..", "python", "unirec"))
sys.path.append(os.path.join(os.path.dirname(__file__), "..", "..", "python", "unirec"))
try:
   from unirec.ur_types import *
except:
   from ur_types import *

fields = []
line_iterator = 0
char_iterator = 0
lines = []
line = []

def get_next_char():
   global line_iterator
   global char_iterator
   global lines
   global line
   if char_iterator >= len(line):
      if (line_iterator >= len(lines)-1):
         line = ""
         return -1
      else:
         line_iterator += 1
         char_iterator = 0
         line = lines[line_iterator]
   character = line[char_iterator]
   char_iterator +=1
   return character

def skip_line():
   global line_iterator
   global char_iterator
   global lines
   global line
   char_iterator = 0
   line_iterator += 1
   if (line_iterator >= len(lines)):
      line = ""
      return False
   line = lines[line_iterator]
   return True

def test_ur_fields():
   global line_iterator
   global char_iterator
   global lines
   global line
   if char_iterator + 8 > len(line) or char_iterator <= 0:
      return False;
   elif line[char_iterator-1 : char_iterator+8] == "UR_FIELDS":
      char_iterator +=8
      return True;

def skip_comment():
   character = get_next_char()
   if character == '/':
      skip_line()
      return get_next_char()
   elif  character == '*':
      character = get_next_char()
      while character != -1:
         if character == '*':
            character = get_next_char()
            if character == '/':
               return get_next_char()
         else:
            character = get_next_char()
   else:
      return character

def parse_field(field_str, fields):
   x = field_str.split(None,3)
   field = ""
   try:
      size = size_table[x[0]]
   except KeyError:
      if len(x) == 2 and x[1][0] == '*':
         size = size_table["bytes*"]
         x[0] = "bytes*"
         x[1] = x[1][1:]
      elif len(x) == 3 and x[1] == "*":
         x[0] = "bytes*"
         size = size_table["bytes*"]
         x[1] = x[2]
      else:
         print >> sys.stderr, 'Unknown type "%s", on line %i, in %s' % (x[0], line_iterator+1, file)
         sys.exit(1)
   notapend = False
   for elem in fields:
      if elem[0] == x[1]:
         notapend = True
         if elem[1] != x[0]:
            print >> sys.stderr, 'Redefining of existing type of field "%s", type "%s", on line %i, in %s' % (x[1], x[0], line_iterator+1, file)
            sys.exit(1)
   if notapend == False:
      fields.append((x[1],x[0],size))

def automata(fields):
   global definition_found
   field = ""
   character = get_next_char();
   while (character !=-1):
      if character == '/':
         character = skip_comment()
      elif character == 'U' and test_ur_fields():
         character = get_next_char()
         while character  == " " or character == "\t":
            character = get_next_char()
         if character == '(':
            definition_found = True
            character = get_next_char()
            while (character != ')' and character != -1):
               if character == '/':
                  character = skip_comment()
               #parse fields afrer comma
               elif character == ',':
                  parse_field(field, fields)
                  field = ""
                  character = get_next_char()
               else:
                  field += character
                  character = get_next_char()
            #parse last field
            if field.strip():
               parse_field(field, fields)
               field = ""
               character = get_next_char()
      elif character == ' ' or character == '/t':
         character = get_next_char()
      else :
         skip_line()
         character = get_next_char()


def create_fields_file (file_path):
      # ***** Write header file with field defines *****
   out = open(file_path + "fields.h", "w")

   out.write("#ifndef _UR_FIELDS_H_\n")
   out.write("#define _UR_FIELDS_H_\n\n")
   out.write("/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/\n")
   out.write("#include <unirec/unirec.h>\n")

   id = 0;
   for name,type,_ in fields:
      out.write("#define F_%s   %i\n" % (name, id))
      out.write("#define F_%s_T %s\n" % (name, c_types[type]))
      id += 1


   out.write("extern uint16_t ur_last_id;\n")
   out.write("extern ur_static_field_specs_t UR_FIELD_SPECS_STATIC;\n")
   out.write("extern ur_field_specs_t ur_field_specs;\n")

   out.write("\n#endif\n")
   out.close()


   # ***** Write C file with field-related functions *****
   out = open(file_path + "fields.c", "w")
   out.write("/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/\n")

   out.write("#include \"fields.h\"\n")

   # Table UR_FIELD_NAMES
   out.write("// Tables are indexed by field ID\n")
   out.write("char *ur_field_names_static[] = {\n")
   for name,_,_ in fields:
      out.write('   "%s",\n' % name)
   out.write("};\n\n")

   # Table UR_FIELD_SIZES
   out.write("short ur_field_sizes_static[] = {\n")
   for name,_,size in fields:
      out.write("   %i, /* %s */\n" % (size, name))
   out.write("};\n\n")

   # Table UR_FIELD_TYPES
   out.write("ur_field_type_t ur_field_types_static[] = {\n")
   for name,type,_ in fields:
      out.write('   %s, /* %s */\n' % (type_table[type], name))
   out.write("};\n\n")

   # Table UR_FIELD_SPECS
   strlen_fields = str(len(fields))
   out.write("ur_static_field_specs_t UR_FIELD_SPECS_STATIC = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, %s};\n"% strlen_fields)
   out.write("ur_field_specs_t ur_field_specs = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static,%s,%s,%s, NULL, UR_UNINITIALIZED};\n"% (strlen_fields, strlen_fields, strlen_fields))

   out.close()


if __name__ == "__main__":
   from optparse import OptionParser
   o = OptionParser()
   o.add_option("-i", "--input", action="store", dest="inputdir", help="Specify input directory.")
   o.add_option("-o", "--output", action="store", dest="outputdir", help="Specify output directory.")
   o.parse_args()

   # Read input files
   for root, dirs, files in os.walk('.' if o.values.inputdir == None else o.values.inputdir):
      definition_found = False
      for file in files:
         if file.endswith('.c') or file.endswith('.cpp') or file.endswith('.cc'):
            #print file
            #print root
            with open(os.path.join(root, file), "r") as f:
               lines = f.readlines()
               if len(lines) > 0:
                  line = lines[0]
                  line_iterator = 0
                  char_iterator = 0
                  try:
                     automata(fields)
                  except Exception, e:
                     print >> sys.stderr, 'Error on line %i: %s, file: %s' % (line_iterator+1, str(e), file)
                     sys.exit(1)
      try:
         if (definition_found == True):
            if o.values.outputdir != None:
               create_fields_file(o.values.outputdir + "/")
            else:
               create_fields_file(root + "//")
         #print fields
         fields = []
      except Exception, e:
         print >> sys.stderr, 'Error on line %i: %s, file: %s' % (line_iterator+1, str(e), file)
         sys.exit(1)

