#!/bin/bash
#
# Copyright (C) 2016 CESNET
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

sizetable='
size_table["string"] = -1;
size_table["bytes"] = -1;

size_table["char"] = 1;
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
size_table["macaddr"] = 6;
size_table["time"] = 8;

size_table["uint8*"] = -1;
size_table["int8*"] = -1;
size_table["uint16*"] = -2;
size_table["int16*"] = -2;
size_table["uint32*"] = -4;
size_table["int32*"] = -4;
size_table["uint64*"] = -8;
size_table["int64*"] = -8;
size_table["float*"] = -4;
size_table["double*"] = -8;
size_table["ipaddr*"] = -16;
size_table["macaddr*"] = -6;
size_table["time*"] = -8;'

find "$inputdir" \( -name '*.c' -o -name '*.h' -o -name '*.cpp' \) -exec grep -l "\s*UR_FIELDS\s*" {} \; |
# remove line and block comments
   xargs -I{} sed 's,\s*//.*$,,;:a; s%\(.*\)/\*.*\*/%\1%; ta; /\/\*/ !b; N; ba'  {} |
# print contents of UR_FIELDS
   sed -n '/UR_FIELDS([^.].*)/p; /^\s*UR_FIELDS\s*([^)]*$/,/)/p; /^\s*UR_FIELDS\s*([^)]*$/,/)/p' 2>/dev/null |
# clean output to get fields only one on line
   sed 's/,/\n/g' |
   sed 's/^\s*UR_FIELDS\s*(\s*//g; s/)//g; s/,/\n/g; /^\s*$/d; s/^\s*//; s/\s\s*/ /g; s/\s\s*$//' |
# sort by name
   sort -k2 -t' ' | uniq |
# check for conflicting types and print type, name, size of fields
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

   if ($2 == "*") {
      print $1"*", $3, size_table[$1];
   } else if (substr($2, 1, 1) == "*") {
      print $1"*", substr($2, 2, length($2) - 1), size_table[$1];
   } else {
      print $1, $2, size_table[$1];
   }
}' | sort -k3nr -k2 > "$tempfile"

ret=$?
if [ "$ret" -ne 0 ]; then
   rm "$tempfile"
   exit "$ret"
fi

# generate fields.{c,h}
awk -F' ' '
BEGIN {
'"$sizetable"'
c_types["string"] = "char";
c_types["bytes"] = "char";

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
c_types["macaddr"] = "mac_addr_t";
c_types["time"] = "ur_time_t";

c_types["uint8*"] = "uint8_t*";
c_types["int8*"] = "int8_t*";
c_types["uint16*"] = "uint16_t*";
c_types["int16*"] = "int16_t*";
c_types["uint32*"] = "uint32_t*";
c_types["int32*"] = "int32_t*";
c_types["uint64*"] = "uint64_t*";
c_types["int64*"] = "int64_t*";
c_types["float*"] = "float*";
c_types["double*"] = "double*";
c_types["ipaddr*"] = "ip_addr_t*";
c_types["macaddr*"] = "mac_addr_t*";
c_types["time*"] = "ur_time_t*";

type_table["string"]="UR_TYPE_STRING";
type_table["bytes"]="UR_TYPE_BYTES";

type_table["char"]="UR_TYPE_CHAR";
type_table["uint8"]="UR_TYPE_UINT8";
type_table["int8"]="UR_TYPE_INT8";
type_table["uint16"]="UR_TYPE_UINT16";
type_table["int16"]="UR_TYPE_INT16";
type_table["uint32"]="UR_TYPE_UINT32";
type_table["int32"]="UR_TYPE_INT32";
type_table["uint64"]="UR_TYPE_UINT64";
type_table["int64"]="UR_TYPE_INT64";
type_table["float"]="UR_TYPE_FLOAT";
type_table["double"]="UR_TYPE_DOUBLE";
type_table["ipaddr"]="UR_TYPE_IP";
type_table["macaddr"]="UR_TYPE_MAC";
type_table["time"]="UR_TYPE_TIME";

type_table["uint8*"]="UR_TYPE_A_UINT8";
type_table["int8*"]="UR_TYPE_A_INT8";
type_table["uint16*"]="UR_TYPE_A_UINT16";
type_table["int16*"]="UR_TYPE_A_INT16";
type_table["uint32*"]="UR_TYPE_A_UINT32";
type_table["int32*"]="UR_TYPE_A_INT32";
type_table["uint64*"]="UR_TYPE_A_UINT64";
type_table["int64*"]="UR_TYPE_A_INT64";
type_table["float*"]="UR_TYPE_A_FLOAT";
type_table["double*"]="UR_TYPE_A_DOUBLE";
type_table["ipaddr*"]="UR_TYPE_A_IP";
type_table["macaddr*"]="UR_TYPE_A_MAC";
type_table["time*"]="UR_TYPE_A_TIME";

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
   if ($2 == "") {
      print "Error: empty line (NR): ", $0;
   } else {
      cnames=cnames"\n   \""$2"\",";
      csizes=csizes"\n   "size_table[$1]", /* "$2" */";
      cstatsizes=cstatsizes"\n   "type_table[$1]", /* "$2" */";

      hfile=hfile"\n#define F_"$2"   "field_id;
      hfile=hfile"\n#define F_"$2"_T   "c_types[$1];
      field_id++;
   }
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

