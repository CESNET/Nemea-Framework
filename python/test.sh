#!/bin/bash
#
# COPYRIGHT AND PERMISSION NOTICE
#
# Copyright (C) 2016 CESNET, z.s.p.o.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   1. Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#   2. Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the distribution.
#   3. Neither the name of the Company nor the names of its contributors may
#      be used to endorse or promote products derived from this software
#      without specific prior written permission.
#
# ALTERNATIVELY, provided that this notice is retained in full, this product
# may be distributed under the terms of the GNU General Public License ()GPL)
# version 2 or later, in which case the provisions of the GPL apply INSTEAD OF
# those given above.
#
# This software is provided "as is", and any express or implied warranties,
# including, but not limited to, the implied warranties of merchantability
# and fitness for a particular purpose are disclaimed. In no event shall the
# company or contributors be liable for any direct, indirect, incidental,
# special, exemplary, or consequential damages ()including, but not limited to,
# procurement of substitute goods or services; loss of use, data, or profits;
# or business interruption) however caused and on any theory of liability,
# whether in contract, strict liability, or tort ()including negligence or
# otherwise) arising in any way out of the use of this software, even if
# advised of the possibility of such damage.
#
# ===========================================================================
#

path_to_logger=../../modules/logger/logger

vp="AgAAAIoAAABpcGFkZHIgRFNUX0lQLGlwYWRkciBTUkNfSVAsdGltZSBUSU1FX0ZJUlNULHRpbWUg
VElNRV9MQVNULHVpbnQzMiBQT1JUX0NOVCx1aW50MTYgRFNUX1BPUlQsdWludDE2IFNSQ19QT1JU
LHVpbnQ4IEVWRU5UX1RZUEUsdWludDggUFJPVE9DT0xbAgAAOgAAAAAAAAAAAAQCBRX/////AAAA
AAAAAAAvBRVS/////8L1KDwsirtWGARWbqSNu1aWAAAA6hO7JgEGOgAAAAAAAAAAAAEICSf/////
AAAAAAAAAABYBREX/////wAAAAAki7tWJAaBVa2Nu1aQAQAACJO3twEGOgAAAAAAAAAAAAEICSf/
////AAAAAAAAAABiBDBV/////yCwcug1i7tWoUW2UwaNu1aQAQAAN5qokQEGOgAAAAAAAAAAAAEI
CSf/////AAAAAAAAAABFBxIb/////90kBuFqi7tWkxgEVtaNu1Y0CAAAK2AszQEGOgAAAAAAAAAA
AAEICSj/////AAAAAAAAAABFBxIb/////2ZmZuZxi7tW4XoU7tmNu1Y0CAAATmAszQEGOgAAAAAA
AAAAAAEICSf/////AAAAAAAAAABiBDBS/////65H4VpTi7tWZDvfb9GNu1YgAwAAjgzA0QEGOgAA
AAAAAAAAAAEICSf/////AAAAAAAAAABiBDBW/////yuHFjlGi7tWObTItp6Nu1YmAgAAL9iokQEG
OgAAAAAAAAAAAAEICSf/////AAAAAAAAAAAtAk5a/////9v5fspGi7tWm8QgkMKNu1aKAgAAomBp
cQEGOgAAAAAAAAAAAAEICSf/////AAAAAAAAAABiBDBU/////2ZmZkZUi7tWSQwCa5+Nu1b0AQAA
SWOokQEGOgAAAAAAAAAAAAEICSf/////AAAAAAAAAABJCQ4+/////1G4HkVJi7tWLbKdz7mNu1bC
AQAA2NNIUAEGAQAA"


data="`mktemp`"
out="`mktemp`"

echo -n "$vp" | base64 -d > "$data"

errors=0

../examples/python/python_example.py -i "f:$data,f:$out" > orig-data-parsed.txt
../examples/python/python_example.py -i "f:$out,b:" > processed-data-parsed.txt

diff orig-data-parsed.txt processed-data-parsed.txt || {
   echo "Python returns different output. Test failed."
   ((errors++))
}

if [ -x "$path_to_logger" ]; then
"$path_to_logger" -i "f:$data" > logger-orig.txt
"$path_to_logger" -i "f:$out" > logger-processed.txt
diff logger-orig.txt logger-processed.txt || {
   echo "Logger verification of python output data failed. Test failed."
   ((errors++))
}
else
   echo "Logger was not found, skipping test!!! The test should be repeated with $path_to_logger present."
fi


rm "$data" "$out"
exit $errors

