#!/bin/sh
#
# \file mdconvert.sh
# \brief This script converts markdown file into C file defining string.
#
# The script expects 3 parameters: 1) input file (in MarkDown)
# 2) output file and 3) name of variable that will be defined (containing
# the text)
# \author Tomas Cejka <cejkat@cesnet.cz>
# \date 2015
#
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


if [ $# -ne 3 ]; then
echo "Usage: mdconvert.sh inputfile.md outputfile.c varname

where varname is the name of char * pointer that will be defined.
"
exit 0
fi


sed '
# escape quotes
s/"/\\"/g;

# add first line with definition
1s/.*/const char *'"$3"' = "&\\n"/;

# remove blocks for examples
s/```//g;

# every line except the first one (that is already handled) needs to be quoted
2,$s/^\(.*\)$/   "\1\\n"/;

# end of definition
$s/.*/&;/;' "$1" > "$2"

