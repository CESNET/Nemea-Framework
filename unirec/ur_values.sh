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

inputdir=./
outputdir=./

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

if [ ! -d "$inputdir" ]; then
   echo "Bad inputdir parameter"
   exit 2
fi

val_h_file="$outputdir/ur_values.h"
val_py_file="$outputdir/ur_values.py"
val_c_file="$outputdir/ur_values.c"

####################
# HEAD ur_values.h #
####################
cat > "$val_h_file" <<ENDTEXT
#ifndef _UR_VALUES_H_
#define _UR_VALUES_H_

/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/
/* Edit "values" file and run ur_values.sh script to add UniRec values.      */
/*****************************************************************************/

#include <stdint.h>

/** @brief Values names and descriptions
 * It contains a table mapping a value to name and description
 */
typedef struct ur_values_s {
   int32_t value;    ///< Numeric Value
   char *name;       ///< Name of Value
   char *description;///< Description of Value
} ur_values_t;

ENDTEXT

#####################
# HEAD ur_values.py #
#####################
cat > "$val_py_file" <<ENDTEXT
#************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************
# Edit "values" file and run ur_values.sh script to add UniRec values.
ENDTEXT

####################
# HEAD ur_values.c #
####################
cat > "$val_c_file" <<ENDTEXT
/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/
/* Edit "values" file and run ur_values.sh script to add UniRec values.      */

#include "ur_values.h"

const ur_values_t ur_values[] = {
ENDTEXT


# Generate content of all files

tr -s ' ' '\t' < "$inputdir/values" | awk '
BEGIN {
	cur_index=0;
	last_start=0;
	hfile=""
	cfile=""
	pynewfield=1
}
{
	if (NF == 1) {
		if (NR == 1) {
			last_name=$1;
			hfile=hfile"#define UR_TYPE_START_"last_name"\t"last_start"\n";
			pyfile="values = [(\""last_name"\", [";
		} else {
			hfile=hfile"#define UR_TYPE_END_"last_name"\t"cur_index"\n";
			pyfile=pyfile"\n\t])";
			last_name=$1;
			last_start=cur_index;
			hfile=hfile"#define UR_TYPE_START_"last_name"\t"last_start"\n";
			pyfile=pyfile",\n\t(\""last_name"\", [";
		}
		pynewfield=1;
	} else {
		desc=$3;
		for (i=4;i<NF;i++) {
			desc=desc" "$i;
		}
		name=substr($1, 2);
		cfile=cfile"   {"$2", \""name"\", \""desc"\"},\n";
		if (pynewfield == 1) {
			pynewfield = 0;
		} else {
			pyfile=pyfile", "
		}
		pyfile=pyfile"\n\t\t(\""name"\", \""$2"\", \""desc"\")";
		cur_index++;
	}
}
END {
	hfile=hfile"#define UR_TYPE_END_"last_name"\t"cur_index"\n";
	pyfile=pyfile"\n\t])\n]\n";
	print hfile >> "'"$val_h_file"'";
	print cfile >> "'"$val_c_file"'";
	print pyfile >> "'"$val_py_file"'";
}'


#####################
# TAIL ur_values.py #
#####################
# nothing here

####################
# TAIL ur_values.c #
####################
echo "};" >> "$val_c_file"

####################
# TAIL ur_values.h #
####################
echo "#endif" >> "$val_h_file"


