#!/usr/bin/python

import sys

size_table = {
   "char" : 1,
   "uint8_t" : 1,
   "int8_t" : 1,
   "uint16_t" : 2,
   "int16_t" : 2,
   "uint32_t" : 4,
   "int32_t" : 4,
   "uint64_t" : 8,
   "int64_t" : 8,
   "float" : 4,
   "double" : 8,
   "ip_addr_t" : 16,
   "ur_time_t" : 8,
   "char*" : -1,
   "uint8_t*" : -1,
}

type_table = {
   "char" : "UR_TYPE_CHAR",
   "uint8_t" : "UR_TYPE_UINT8",
   "int8_t" : "UR_TYPE_INT8",
   "uint16_t" : "UR_TYPE_UINT16",
   "int16_t" : "UR_TYPE_INT16",
   "uint32_t" : "UR_TYPE_UINT32",
   "int32_t" : "UR_TYPE_INT32",
   "uint64_t" : "UR_TYPE_UINT64",
   "int64_t" : "UR_TYPE_INT64",
   "float" : "UR_TYPE_FLOAT",
   "double" : "UR_TYPE_DOUBLE",
   "ip_addr_t" : "UR_TYPE_IP",
   "ur_time_t" : "UR_TYPE_TIME",
   "char*" : "UR_TYPE_STRING",
   "uint8_t*" : "UR_TYPE_BYTES",
}

python_types = {
   "char" : ("str", "c"),
   "uint8_t" : ("int", "B"),
   "int8_t" : ("int", "b"),
   "uint16_t" : ("int", "H"),
   "int16_t" : ("int", "h"),
   "uint32_t" : ("int", "i"),
   "int32_t" : ("int", "I"),
   "uint64_t" : ("int", "q"),
   "int64_t" : ("int", "Q"),
   "float" : ("float", "f"),
   "double" : ("float", "d"),
   "ip_addr_t" : ("IPAddr", "16s"),
   "ur_time_t" : ("Timestamp", "8s"),
   "char*" : ("str", "s"),
   "uint8_t*" : ("str", "s"),
}



fields = []
groups = []
constants = []

# Read input file
for n,line in enumerate(open("fields", "r").readlines()):
   line = line.strip()
   if line == "" or line[0] == '#':
      continue

   try:
      # Specifiaction of a group
      if line[0] == '@':
         g_name,g_fields = line[1:].split('=',1)
         groups.append( (g_name.strip(), g_fields.strip()) )

      # Specification of a constant
      elif line[0] == '.':
         # TODO: co kdyz bude konstanta retezec obsahujici mezeru?
         x = line.split(None,2); # name, value, [desc]
         constants.append((x[0][1:],x[1]))

      # Specification of a field
      else:
         x = line.split(None,2); # name, type, [desc]
         try:
            size = size_table[x[1]]
         except KeyError:
            print >> sys.stderr, 'Unknown type "%s" on line %i' % (x[1], n+1)
            sys.exit(1)
         fields.append((x[0],x[1],size))
   except Exception, e:
      print >> sys.stderr, 'Error on line %i: %s' % (n+1, str(e))
      sys.exit(1)

# TODO check for duplicate identifiers

# ***** Write header file with field defines *****
out = open("fields.h", "w")

out.write("#ifndef _UR_FIELDS_H_\n")
out.write("#define _UR_FIELDS_H_\n\n")
out.write("/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/\n")
out.write("/* Edit \"fields\" file and run process_fields.py script to add UniRec fields. */\n\n")

id = 0;
for name,type,_ in fields:
   out.write("#define UR_%s   %i\n" % (name, id))
   out.write("#define UR_%s_T %s\n" % (name, type))
   id += 1

out.write("\n#define UR_MAX_FIELD_ID %i\n" % (id-1))
out.write("#define UR_FIELDS_NUM %i\n\n" % id)

out.write("extern const char *UR_ALL_FIELDS_STR;\n")
out.write("extern const ur_field_id_t UR_ALL_FIELDS[];\n")
out.write("extern const int UR_ALL_FIELDS_NUM;\n")

out.write("\n");
for name,value in constants:
   out.write("#define UR_%s %s\n" % (name, value));

out.write("\n#endif\n")
out.close()


# ***** Write C file with field-related functions *****
out = open("field_func.c", "w")
out.write("/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/\n")
out.write("/* Edit \"fields\" file and run process_fields.py script to add UniRec fields. */\n\n")

# Table UR_FIELD_NAMES
out.write("// Tables are indexed by field ID\n")
out.write("const char *UR_FIELD_NAMES[] = {\n")
for name,_,_ in fields:
   out.write('   "%s",\n' % name)
out.write("};\n\n")

# Table UR_FIELD_SIZES
out.write("const short UR_FIELD_SIZES[] = {\n")
for name,_,size in fields:
   out.write("   %i, /* %s */\n" % (size, name))
out.write("};\n\n")

# Table UR_FIELD_TYPES
out.write("const ur_field_type_t UR_FIELD_TYPES[] = {\n")
for name,type,_ in fields:
   out.write('   %s, /* %s */\n' % (type_table[type], name))
out.write("};\n\n")

# Table UR_FIELD_GROUPS
out.write("const char *UR_FIELD_GROUPS[][2] = {\n")
for g_name,g_fields in groups:
   out.write('   {"%s", "%s"},\n' % (g_name, g_fields))
out.write("};\n")
out.write("#define UR_FIELD_GROUPS_NUM %i\n" % len(groups))


# Arrays of fields sorted by their order in templates
def sort_func(f1, f2):
   if (f1[2] < f2[2]):
      return 1
   elif (f1[2] > f2[2]):
      return -1
   elif (f1[0] > f2[0]):
      return 1
   elif (f1[0] < f2[0]):
      return -1
   else:
      return 0

out.write("\n")
out.write("const char *UR_ALL_FIELDS_STR = \"")
out.write(','.join(map(lambda x: x[0], sorted(fields, sort_func))))
out.write("\";\n")

out.write("const ur_field_id_t UR_ALL_FIELDS[] = {")
out.write(','.join(map(lambda x: "UR_"+x[0], sorted(fields, sort_func))))
out.write("};\n")
out.write("const int UR_ALL_FIELDS_NUM = %s;\n" % str(len(fields)))

out.close()


# ***** Write Python file *****
out = open("fields.py", "w")
out.write("#************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************\n")
out.write("# Edit \"fields\" file and run process_fields.py script to add UniRec fields.\n\n")

out.write("FIELDS = {\n")
for name,type,size in fields:
   out.write("   '%s' : FieldSpec(%i, %s, %r),\n" % \
             (name, size, python_types[type][0], python_types[type][1]) )
out.write("}\n\n")

out.write("FIELD_GROUPS = [\n")
for g_name,g_fields in groups:
   out.write('   ("%s", "%s"),\n' % (g_name, g_fields))
out.write("]\n\n")

for name,value in constants:
   out.write("%s = %s\n" % (name,value))
out.write("\n");

out.close()
