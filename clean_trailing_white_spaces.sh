#!/bin/bash
#
# \file clean_trailing_white_spaces.sh
# \brief Removes white spaces from the end of lines of the given file.
# On success, the return code is 0. When file can't be opened, the return code is 2.
# When modification or replacement of the old file fails, the error code is 1.
# \author Tomas Cejka <cejkat@cesnet.cz>
# \date 2014
#
# Copyright (C) 2014 CESNET
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

if [ $# -ne 1 ]; then
  echo "Usage: $0 <path_to_file>"
  exit 2;
fi

if [ -e "$1" -a -r "$1" ]; then
  if [ -x "$1" ]; then
    SETCHMOD=1
  else
    SETCHMOD=0
  fi
  grep -q '\s\s*$' "$1" &&
    sed 's/\s\s*$//g' "$1" > "$1.tmp" &&
    mv "$1.tmp" "$1" && { [ $SETCHMOD -eq 1 ] && chmod a+x "$1"; } &&
    exit 0;
  exit 1
else
  echo "Cannot open file \"$1\"." >&2
  exit 2
fi

