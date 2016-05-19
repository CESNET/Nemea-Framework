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

path_to_logger=./$srcdir/../../modules/logger/logger

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

expected="H4sIAHPszVYAA63VTWvbQBAG4Ht/hW5uQRnmY2c/BDoU1wVDahtHFNqLCSQUH5qU4v7/jh3b2S1l
1To5ziJWj2bfHb2dLO6/Pe62t7vt40M3aRtum8n2x+3d3c/mw82wma/aY3Wznu6r3fb7fTPMP802
H+frmyGrr99b+Wv7sBNuVsv1sJkunmryh632a6d6v9m5js3s82wxbIYvq9lxYbVeDsvp8nry7s2T
onfAoMDUHiG9C4caoomfPT0j+SvkK8KBUoe+YwQW/do+I/94hqnDCE7Injm5e1Jsz+peMcX2jO5T
Im0zdG+qk7j3J7HRIIGkszhGE1MwTkVM2HEERKyLbRsRl4sd5mIJ3mFGdgGFLjAn6wy4CFHrZqeQ
MFbMFDtBEA4Vc3IhZWYJHOMFZp8gAEXgUDNTJ/ZZIdX7rNH67HMzU4Fm5ylHKyu7f0Wfjug/0IdG
V8PBHRIkwTpa3aXov6WjdgcNTQqiI3dQxe5gkY5YpoOpIItP/iVkXycjA7OMjA2GQMUl1HJsqDh5
hUDbmdubbBFHzSFR3WyhVy+52WsZjSD5rOOEIi/psxuJhrfIj41niwbFos9lnNVReIU+B7GaHPiR
OKOdRxgZ0JwgUvFLcWU2HAvnfUZVrph/A6AVd/WpBwAA"


data="`mktemp`"
out="`mktemp`"

echo -n "$vp" | base64 -d > "$data"

errors=0

./$srcdir/../examples/python/python_example.py -i "f:$data,f:$out" > orig-data-parsed.txt
./$srcdir/../examples/python/python_example.py -i "f:$out,b:" > processed-data-parsed.txt

diff orig-data-parsed.txt <(echo -n "$expected" | base64 -d | gunzip) || {
   echo "Historically stored expected output does not match with the current one. Test failed."
   ((errors++))
}

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


rm -f "$data" "$out" logger-orig.txt logger-processed.txt orig-data-parsed.txt processed-data-parsed.txt
exit $errors

