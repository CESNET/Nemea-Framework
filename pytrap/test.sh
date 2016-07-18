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

vp="H4sIAOwfilcAA8WW24/bRBSHJ0VIqKjcKqBcCqZUgKDxzozHM56uBM0m6WJtuvHaTukCxfgWiNRu
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
AA=="

expected="H4sIACsgilcAA+WZbW/bNhDH3+9T+P0qhc9HBhgQxVYMoW6sSnKxvBL0QAMBtrpA0g3dp98xipNZ
kZSoSdy4M2BA5p0o6vjD/e/oWZrlUTz5bUKF8ilhPuPCp5S+m6TJtLEwCr5Co6KN4fQiC1Mclxx/
LKLz9/lplOVnUbiY4Sh5N8miDyH+TtKsmZdITpjgODW7NS6CTlscTN+HmZsbZ57hyuJl4vwkUTht
GsX5h3SeZxdxiIPGNENpFmSrNJ8uZ25UkNul79w6i5L2IuNkmS2ny4V7GOCypnF+tgjm6e0rLLcX
2aK5cI+aBotFOMvjIMkucPTq8ssxA6GpAQIn1/bq2q/+ufeMzudPdc2j2c3aFVWaKaVPMDI57kaO
u5E3u+E80/AjummQYpKE8yjNwqSxJOHHVYjxWiUROjRjqzRM8mAenmd3Q5+iAK/x6oj55Gg1i9u7
fuzC9cvs+5kQoEcyYUQ/E872AkwwQv4HTMg9MUFw1HD0aCFBBfgUpK/ZLhIKYAQSxk0ru5HY2vqR
gKenCbFXJAgRVAjJiKHC7eRuqProAFDEfXtvaBgxpJJQqUKSWpRrWDtkGGXGEFuWhV7vkEIn0fmn
KAufC8kOBscu9LuMMCHRSDsg4cyXrbTBR6UNx4Hg/Yw42wEyQrUU95yAMietaPVhQgmhfb4NIZxo
sS5JVdWlsQKjpLiinDk8aqoMfy1C/gNBByK9aYRRNBjZYkSPKTccB7JHWra2A2SE/NrOIzuxGkok
k7tM0nlLAwpVsiw4LaGytdUui0jJLPB1qYjFwO0/lTgD1z7oNic3L4BFgmiXpQ7nUZzoAb3Rg3rT
UYLILkxa9QdIrl+TEsEI1tOE4OaRx2uQYe9bLlw0wBigFD3xQ8UJ7hfxDfgPahGuiSGD1Yh77M6T
dkAJvtaXm2pT2ysvvfzizYtr+3fxzfsQe/j0ydnv6dFfvvQVCXwsE31CeD9Zdyt8DlGajaxgsFzr
JerG9uJE4UuJvRAlzBOq2mHvhqi1Wa+FgMqzUpXWFsrDNFOiBJa3YKkHYKFmUfNUro7vY7kDV7z5
41u1+TPdfP1cx5vLz9dR7KUxenFCvVVwhHD4yA83bJgq1aLqcT1TbT0bmaeYHqBKD1J1cHqmxuuZ
6tIzCxYxq2yJlU9N1hJKw6xCPSuUsKX9AXr2KCfMtDkR4zjhtOeYZWsb4IQeFifbWI3gpH3LW6p7
nqdSXI3lpL/u4cN99vepFGdS70mlpD4dI1Od7g0ZGmQplKy8kirsq2vriapUoIH36pQw2M8erk49
6LvYSK54f29+Y/t58s+L9V1v4wjnmVUyGcmJpP2cONtL5x9FAfbUd8GYKrnTu+ECOKNEW+txy02h
Cu0JBRU1tO5tv9xxMXsr2afde+2h+lEDqqaGT48PLPu8WPUjNZiKF3UJ2EwXAtastlBWTBdaQP16
x4P92edxldJtTsZmH/e3RB8nzvbzcLKN1RiV0l2c/Khj5DYn/wIs88nqtR4AAA=="


data="`mktemp`"
out="`mktemp`"

echo -n "$vp" | base64 -d | gunzip > "$data"

errors=0

./$srcdir/../examples/python/python_example.py -i "f:$data,f:$out:w" > orig-data-parsed.txt
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