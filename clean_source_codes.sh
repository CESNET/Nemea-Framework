#!/bin/bash
#
# \file clean_source_codes.sh
# \brief Goes through the repository, cleans source codes and warns
# about possible complications. Base directory is given as a parameter
# (current directory is used as default).
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

#set -x
COLUMNS=$(stty -a | sed -n 's/.*columns \([0-9][0-9]*\).*/\1/p')
LINE=$(for ((i=0;i<${COLUMNS};i++)); do echo -n "-"; done)

BASEPATH="."
if [ "$1" != "" ] ; then
  BASEPATH=$1
fi

CLEANSCRIPT=./clean_trailing_white_spaces.sh

SELNAMES='( -name *.c -o -name *.cpp -o -name *.h -o -name *.hpp -o -name *.py -o -name *.sh )'
set -f
echo "Find and remove trailing white spaces."
find $BASEPATH -type f \! -regex ".*alglib.*" \
  $SELNAMES \
  \! -name "${CLEANSCRIPT/.\//}" \! -name clean_source_codes.sh \
  \! -exec "$CLEANSCRIPT" {} \; -print

echo $LINE
echo "Check for copyrights."
echo $LINE
CURRENT_YEAR=$(date +%Y)
find $BASEPATH -type f \! -regex ".*alglib.*" \
  $SELNAMES \
  \! -name "${CLEANSCRIPT/.\//}" \
  \! -exec grep -q -e "Copyright" {} \; \
  -print | while read f; do
  GITAUTHORS="$(git log --pretty=format:%ce $f | sort -u)"
  echo "Missing copyright in $f from $(echo $GITAUTHORS | tr ' ' ',')" >&2
  echo "$f" >> copyright-missing-errors
done

echo $LINE
echo "Check year in copyrights."
echo $LINE
CURRENT_YEAR=$(date +%Y)
find $BASEPATH -type f \! -regex ".*alglib.*" \
  $SELNAMES \
  \! -name "${CLEANSCRIPT/.\//}" \
  \! -exec grep -q -e "Copyright.*$CURRENT_YEAR" -e "date.*$CURRENT_YEAR" {} \; \
  -print | while read f; do
  GITYEAR=$(date -d "$(git log --pretty=format:%cd --date=short $f | head -1)" +%Y)
  if [ "$CURRENT_YEAR" == "$GITYEAR" ]; then
    GITAUTHORS="$(git log --pretty=format:%ce $f | sort -u)"
    echo "Missing year in $f last year in git $GITYEAR from $(echo $GITAUTHORS | tr ' ' ',')" >&2
    echo "$f" >> copyright-errors
  fi
done
set +f


echo "Done."

