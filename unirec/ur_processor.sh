#!/bin/bash


while getopts "i:o:" opt; do
  case $opt in
    i)
      inputdir="$OPTARG" >&2
      ;;
    o)
      outputdir="$OPTARG" >&2
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

if [ -z "$inputdir" -o -z "$outputdir" ]; then
   echo "$0 -i inputdir -o outputdir"
   exit 1
fi

if [ ! -d "$inputdir" ]; then
   echo "Bad inputdir parameter"
   exit 2
fi

if [ ! -d "$outputdir" ]; then
   echo "Bad outputdir parameter"
   exit 2
fi

tempfile="`mktemp`"

sizetable='size_table["char"] = 1;
size_table["uint8"] = 1;
size_table["int8"] = 1;
size_table["uint16"] = 2;
size_table["int16"] = 2;
size_table["uint32"] = 4;
size_table["int32"] = 4;
size_table["uint64"] = 8;
size_table["int64"] = 8;
size_table["float"] = 4;
size_table["double"] = 8;
size_table["ipaddr"] = 16;
size_table["time"] = 8;
size_table["string"] = -1;
size_table["bytes*"] = -1;'

find "$inputdir" \( -name '*.c' -o -name '*.h' -o -name '*.cpp' \) | xargs -I{} sed -n '/^\s*UR_FIELDS/,/)/p' {} 2>/dev/null |
   sed 's/^\s*UR_FIELDS\s*(\s*//g; s/)//g; s/,/\r/g; /^\s*$/d; s/^\s*//; s/\s\s*/ /g; s/\s\s*$//' |
   sort -k2 -t' ' | uniq |
   awk -F' ' 'BEGIN{
'"$sizetable"'
}
{
   if (NR == 1) {
      type=$1;
      iden=$2;
   }
   if ((iden == $2) && (type != $1)) {
      printf("Conflicting types (%s, %s) of UniRec field (%s)\n", type, $1, iden);
      exit 1;
   } 
   type=$1;
   iden=$2;
   print $1, $2, size_table[$1];
}' | sort -k3nr -k2 > "$tempfile"

ret=$?
if [ "$ret" -ne 0 ]; then
   rm "$tempfile"
   exit "$ret"
fi

awk -F' ' '
BEGIN {
'"$sizetable"'
c_types["char"] = "char";
c_types["uint8"] = "uint8_t";
c_types["int8"] = "int8_t";
c_types["uint16"] = "uint16_t";
c_types["int16"] = "int16_t";
c_types["uint32"] = "uint32_t";
c_types["int32"] = "int32_t";
c_types["uint64"] = "uint64_t";
c_types["int64"] = "int64_t";
c_types["float"] = "float";
c_types["double"] = "double";
c_types["ipaddr"] = "ip_addr_t";
c_types["time"] = "time_t";
c_types["string"] = "char";
c_types["bytes*"] = "char";

field_id=0;

cfile="/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/\n";
cfile=cfile"// Tables are indexed by field ID\n";
cfile=cfile"#include \"fields.h\"\n";

cnames="char *ur_field_names_static[] = {";
csizes="short ur_field_sizes_static[] = {";
cstatsizes="ur_field_type_t ur_field_types_static[] = {";

hfile="#ifndef _UR_FIELDS_H_\n"
hfile=hfile"#define _UR_FIELDS_H_\n"
hfile=hfile"\n"
hfile=hfile"/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/\n"
hfile=hfile"#include <unirec/unirec.h>\n";
}

{
   cnames=cnames"\n   \""$2"\",";
   csizes=csizes"\n   "size_table[$1]", /* "$2" */";
   cstatsizes=cstatsizes"\n   "type_table[$1]", /* "$2" */";

   hfile=hfile"\n#define F_"$2"   "field_id;
   hfile=hfile"\n#define F_"$2"_T   "c_types[$1];
   field_id++;
}

END {
cnames=cnames"\n};";
csizes=csizes"\n};";
cstatsizes=cstatsizes"\n};";

hfile=hfile"\n\nextern uint16_t ur_last_id;\n";
hfile=hfile"extern ur_static_field_specs_t UR_FIELD_SPECS_STATIC;\n";
hfile=hfile"extern ur_field_specs_t ur_field_specs;\n";
hfile=hfile"\n";
hfile=hfile"#endif\n";

cfile=cfile"\n"cnames"\n"csizes"\n"cstatsizes
cfile=cfile"\nur_static_field_specs_t UR_FIELD_SPECS_STATIC = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, "NR"};\n"
cfile=cfile"ur_field_specs_t ur_field_specs = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, "NR", "NR", "NR", NULL, UR_UNINITIALIZED};"


print hfile > "'"$outputdir/fields.h"'";
print cfile > "'"$outputdir/fields.c"'";
}
' "$tempfile"

rm "$tempfile"

