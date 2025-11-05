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
# may be distributed under the terms of the GNU General Public License (GPL)
# version 2 or later, in which case the provisions of the GPL apply INSTEAD OF
# those given above.
#
# This software is provided "as is", and any express or implied warranties,
# including, but not limited to, the implied warranties of merchantability
# and fitness for a particular purpose are disclaimed. In no event shall the
# company or contributors be liable for any direct, indirect, incidental,
# special, exemplary, or consequential damages (including, but not limited to,
# procurement of substitute goods or services; loss of use, data, or profits;
# or business interruption) however caused and on any theory of liability,
# whether in contract, strict liability, or tort (including negligence or
# otherwise) arising in any way out of the use of this software, even if
# advised of the possibility of such damage.
#
# ===========================================================================
#

test -z "$srcdir" && export srcdir=.
test -z "$builddir" && export builddir=.

if [ "$srcdir" != "$builddir" ]; then
exit 77
fi

# Check import pytrap, skip this test on import error

python3 -c 'import pytrap' 2>/dev/null || exit 77

python3 -m pytest $srcdir/test || exit $?

path_to_logger="$srcdir"/../../modules/logger/logger

input="
H4sIAOwfilcAA8WW24/bRBSHJ0VIqKjcKqBcCqZUgKDxzozHM56uBM0m6WJtuvHaTukCxfgWiNRu
lm5a1KpQKt5AQoLCC0gIVPWZv6G0qhAPwAuqEFJB4q3ioiL1GY6deOO46e4iluKRc3LO+DLfOfOb
8QaUHKX3Oot+FB1Sao7rmdaOgefY1cQ73FnocaZMzbt1J3Ma5uyMN2W63m6z3qjt6HUOxopr7qmD
bztuzm9UwE1u0qhiVaozdbf/DMLTl1lN2818x7S8Pc60585b9XzMcStuy/GqzdowDCNbvtVQaqad
G0w/ZtlNt1ltNgauW7W83Y3KtJP5zeV/bmPHUu9QZ+GV9G3VSqNRr3lWxXbni3Fzdnp8h2fWRkJO
fS7v2/W5Vh1oW7aZD7ecuu1Vpuuzbj6616wgtOkyuoQGxwfty92/4Mj8H6Y+T/3NG9DI8eDz0WT1
103PZbYEsfObQ/R+6fxmdGd6yd1pexLdj2poE5qDSP9U0FJncScVzCBSYLGrFy/11PDY2CDDnHCD
cm7sIox7BFOPaswjhBhCZ4pdnzYdt24DygRV8USrZilwnQrXqXCdCtft1DHHq/NdKo3ynfyoOpNw
ZTbj+wrdOD597Xx/ZON+9ezGEb5TX9ipf6ZQv4unF7ceAq7MJnzfAN+Hw/op0B5Ak/D7AroFhRDp
nw+nfBgzwphOsSQsGbxQidBVgyZjSvqF4Dg5C10Sh7oIua/jiAVt0U74KaFS4jgIfKNNFHN2rwni
zxNjIJUaUA+ABUa/LwNfjUeBFS31jxUK+suZT7YnoJkdA/wotHuQBOB5APYh0j/7BSWGzobQgsuU
TKOqTjJmgjEZjWrYYO0Ah2EUyJhhKrnGiUYT1ohwqV0PlzIdcMmQ98/rFvj7c6n/RoG3+u6BxxPO
zI7hfQTaVvQ08L0EvG2I9M9BgZ8qFpgSqIHU8xVWlkuc6yRcD3yNBCKMo9hIyqvrNBZaO+A4phKv
scalhzKWs+/MjjCfe+1K6s8VJvXVJ16cSFgz2xftzfD75c8D5i1pw8DYRLcD962ogwj6GN2bMjOK
qUYxhgHjvG7HxUGDEgspBSHQAwdhu2D0WJUiQTA0A0u8rODkKYMbK4ejTjfsRvFS2ekslqf9Xvy6
f7S8xyrDk5Td+5yJI6quclxRMWUqxtponpbfUNq2Wn4mbxrNT+/UllqSl8wO83Pp60J+HkN70W0o
QnegHixxnxbzw6QYm59cvC3bbcZEWI51HsSxz8swBQIQUTBIE08FwmFNGJeldF2zugeOht2DTvfw
QmR1YQc1rbJjwb6lYVJuVSZgvqiQGU3Sa3OUPn4l3ZxP/RMF3VyYEWaSm8yum274SrpJO2MRQ8rC
OIDVIsJtXQSSxhx043MWB/FadbMS88nUf6vAHE3+1ExYM5t0f7sOzFSuwNzvXJ+1gpS2r6aFWkEL
Gy+ethLWzA618OyJAfN9aaOghX2ghTZo4Qho4bNrtKAbU+PFkOuA7TxgXA/LAeGw5UVxmYUBF4bQ
8mpgUhPG/6OG/i7yZmFm/Hb8mVaSncyu18xYwy6yPl8Ka1glWWFmfLftx/mENbPDmWFf+WerpC7G
r5K5uNAowUYcl7VYkz73jTLjIiSSRPnNJPlGpP/JvBCrzYvxKwZ8Nu4ffD7uv8Erhm4IGWp+FAjY
Gn0m2jSKRRBSwzeYiK7/SVWYFysxX0j94wXmhbvefjlhzey6acFYSQvGv/mMLDKX0N/YG/VEhA8A
AA==
"

expected="
H4sIAKQrl1cAA+WZa2/aSBSGv++v8DdabezM/cxEqhQHHGSVBNc21eaT5csgIW1D1aS76v76HYNT
wiXY3FJIkSKZ4Z3M+PjRec8Zd6I48QPrgzW4H33TuR+4RfHtXQsz4WBEHEKZgzFuvT+zorC9Skkw
OMKIBX4SXt3FXmR0nOIzq+fffkyu/Di59r1ex4yiMyv2bzzzPYzin/8tHn3R78yiiFNEGD2zCCXv
K2XPrRcGbvujF5ermjU75p6CflhO4kiYBSM/SG6ibhLfBZ4ZVGo6FMVuPIiSdr9TjjKEpzc5N7Xj
h4vbD8J+3G/3e+ViYPbYDpLrntuNqpvrP13EvelFuVTb7fW8ThK4YXxnRlsPo68XBJjEChBcPuqH
Ryf/rzXT+rfd5uLEL3fWYkhgIYkQ8tIEKDGPLzGPLymfSqWNvE+lUAJnVuh1/Sj2wuq30Ps08Ezc
BqFfSqrRQeSFidv1buNng599t/xmLs+Jg84HncBawOWijF3rj87h2GIgd2BLsYZsTYR7YIsg9Puw
xX8lW8iIFDUTatDCDBwM3JFkES0BsB1aymgw5vVozYQvowXN0xZ7XbQQYpgxTpDCrKRhLo5rKAMQ
qPx7eUrFmkI5h1ykHBUsG8KwRI9gohTSWZbK4QJx2PJvP/uxtx/Y5vi5KJ/DWtYI40aLG8BGicOX
0hjdNo1NEGK0IWsT4SmyhiVnM95AqMv5UK7BDSOEX1RXpFEk2TBDeV5kSjMTLEEFpqTErMBC0UOT
9oyeetQapzWCjVDxJdbkluXYBCHewDJnwlNkDf25mNeeB7ImsVk/M9vqSRVwWPAspTiDXBdallmN
c6KBDjOBtInf0aS2UkelA7KOt8mdUkPycvmv6A68yaY+Ktf76IoSja/CbaE+A07lQWljBJnOBSED
AGpSo9Xon/gqgwJKAcZGaz6YXZrnjRwFzopajUqkUE21Vi4+v948cO73YjTOx4V+sKPRV7ubPup/
0x/2TWCbPVjXf0Xn/zjcEch1TE3tIETXETrb6SHJlGSHCs/UwM3InAr3TqYJAHsdMplq1D3U6Csy
h2o4ZAxyW3ORaZ0K26S9zNh7VgEqVgBq3Bir5nxOW4NVkAbjv3/k4y/R+Pt9EYxH949+YEeBkVGE
7YF7bphyDIVUkTo6xXo6N/dpsezTO+RNIpvSKdfTeXo+Lbbx6cVJFa0atME115mpDAs05JApooXx
6VQwnenj8emNeSNqmTe2PW8UNzimmwnX8IZPjLcqkJvxtjTpKOvCA7svFbvw1rAupDXnK9u5LyVc
vpb7cnm1mf2unlARJoFnTPDczrAQWBTaZnkmQAJd479MUZBv3H9X9MlkBz5pwzOZqfAN5cO99snH
dQR46G4E7cAbxw15mwj3nQ8FBnitPhk260ZW6yu+gBKMpNY21VSlIpU2E5BjhYs17XL5AoQcVzas
6ZV/eXUomrq1qHkbcmrZcK/VIZegcpoWGSBEUwZDUmjIciJTyaA4/DF142y4ufvKZd52yYYADXmb
CN8Qb1UgN3TfxUlH8lpkibf/AfgoaPqlIgAA
"


data="`mktemp`"
out="`mktemp`"

echo -n "$input" | base64 -d | gunzip > "$data"

errors=0

$srcdir/../examples/python/python_example.py -i "f:$data,f:$out:w" | sed 's/ \([0-9]*\)L,/ \1,/g' > orig-data-parsed.txt
$srcdir/../examples/python/python_example.py -i "f:$out,b:" | sed 's/ \([0-9]*\)L,/ \1,/g' > processed-data-parsed.txt

diff orig-data-parsed.txt <(echo -n "$expected" | base64 -d | gunzip) || {
   echo "Historically stored expected output does not match with the current one. Test failed."
   ((errors++))
}

diff orig-data-parsed.txt processed-data-parsed.txt || {
   echo "Python returns different output. Test failed."
   ((errors++))
}

if [ -x "$path_to_logger" ]; then
"$path_to_logger" -t -i "f:$data" > logger-orig.txt
"$path_to_logger" -t -i "f:$out" > logger-processed.txt
diff -u logger-orig.txt logger-processed.txt || {
   echo "Logger verification of python output data failed. Test failed."
   ((errors++))
}
else
   echo "Logger was not found, skipping test!!! The test should be repeated with $path_to_logger present."
fi

rm -f "$data" "$out" logger-orig.txt logger-processed.txt orig-data-parsed.txt processed-data-parsed.txt
exit $errors
