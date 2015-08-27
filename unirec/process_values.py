#!/usr/bin/python
import sys
fields = []
values = []
constants = []
type_defined = False
type_name = ""
# Read input file
for n,line in enumerate(open("values", "r").readlines()):
   line = line.strip()
   if line == "" or line[0] == '#':
      continue
   try:
      if line[0] == '.':
         if type_defined == False:
            print >> sys.stderr, 'Undefined type of value, line %i' % (x[1], n+1)
            sys.exit(1)
         x = line.split(None,2); # name, value, [desc]
         if len(x) < 3:
            constants.append((x[0][1:],x[1], ""))
         else:
            constants.append((x[0][1:],x[1], x[2]))
      # Specification of a field
      else:
         x = line.split(None,1); # name, type, [desc]
         if type_defined == True:
            #store values to old type
            values.append((type_name, constants))
            constants = []
         type_name = x[0]
         type_defined = True
   except Exception, e:
      print >> sys.stderr, 'Error on line %i: %s' % (n+1, str(e))
      sys.exit(1)
# TODO check for duplicate identifiers

# ***** Write header file with field defines *****
out = open("values.h", "w")

out.write("#ifndef _UR_VALUES_H_\n")
out.write("#define _UR_VALUES_H_\n\n")
out.write("/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/\n")
out.write("/* Edit \"values\" file and run process_values.py script to add UniRec values. */\n\n")

count = 0;
for name,con in values:
   out.write("#define UR_TYPE_START_%s   %i\n" % (name, count))
   count += len(con)
   out.write("#define UR_TYPE_END_%s   %i\n" % (name, count))

out.write("\n");
out.write("\n#endif\n")
out.close()


# ***** Write C file with field-related functions *****
out = open("values.c", "w")
out.write("/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/\n")
out.write("/* Edit \"values\" file and run process_values.py script to add UniRec values. */\n\n")
out.write("const ur_values_t ur_values[] =\n{\n")
for _,con in values:
   for name,value,desc in con:
      out.write('   {%s,"%s", "%s"},\n' % (value, name, desc))
out.write("};\n\n")
out.close()


# ***** Write Python file *****
out = open("values.py", "w")
out.write("#************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************\n")
out.write("/* Edit \"values\" file and run process_values.py script to add UniRec values. */\n\n")
out.write("values = %s\n" % values)
out.close()
