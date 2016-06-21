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

vp="H4sIADq7aVcAA+2UPWgUQRTHJ5fLeTFBSTQxIMhCYhLJgZr4FbSJ50UCMXfcbQ61OXdnd+9mP2Y/
Zr87sRAURQlYWAiChSCKhSTFNWJlY5FGTCGSRsTCzlpnk7vjAiZtiOx7LMx/Fub35s17LwEAeEU/
ZHCCYDFXSmxlrpBpqFIxGykbaSLDzl3LVWbniiW2Tc/PUOkgbE9NMoV8ka1kFzb16XMbR0V7TR0d
1tIXmFw5t8BW2BuFXGOjUMyz+Wx+PsMHtkiYmcvZDLEthKvMYnG+bTm51g/AHdC0ZKJr4A+1pj7Z
NVCM9Iff45cy9+rloWQZv3hQLz+h/34eqo92pADopb4fDIIxMNVmxPEtC4lYUDhH5gUo+FA0Naib
oiASP9RF3hENO/AtgkOXDxXstILoSHePtQdxvavvSFOP3K+XR1K3F1/TIB51AJBeWlmJgjhAfQAM
01CmtpgLq66sYdv3Az1QBU/EErY81XYcyZE8QTG8bbF88tRipJm31o+zFPs8t1xKNbDnn758HGHT
1LtBH2Ba2ConEMi5enRRD2k8Z8uSJHOeT2qqQGq+YBFNM3wDmduCc/v6j0b660hqXabgJZr1zxR8
Jg3AxK3MpwicoD4MjgO6HK9iXSJSaNqEQgzEK76vahiHUFRIQCzeCAPDgzqs2e3I8X8hJUn6blLk
enj411oDudBAdlJPgx6KpdcMeDckGqpVPWiHBqpZgQfNGnZ8OdB1bEEVeTZv6YjjBbJThjeq683V
9ZslShUuftNXKZXpBOBh7/vViNpD/Rg4AZLtDysh6LoukjRJQSHmjGpIFBjgMLA5pOh6qJvaTtBy
pCfuDk7PUuj0u4/Lzyh0NEHL/cvms/Zu+BA4uKWaIKJQhUO+pcoGCaCjEl1wApfncNxBu9BBcdJ3
Ienx2IrHVtxB8djaY0mPx1Y8tuIOisfWHkt6PLbisfVfdNAuFDIN+y+w4v4DkBMAAA=="

expected="H4sIANq7aVcAA+2WS2/aQBSFu+6vYEcrOaN5+InkRUqpFCkNiNBK7Qb5CYPx2Hj8/vU1z5hKMaW7
hGHFtazRfOce33s+9Z+8RZRSK6URG/SlHpZ6fRpbrpv0vj7P5g8T6VA9T4fbKqWh15s9fB/Nvz1M
n2et+vG+KTPKUoJ7k/F0Nh8+7Wuk7o7aPjvW28NOtd4b/Rw9zeazX5PR4cFkOp6Nh+NHya5Sj/fu
vwwlniaULXo/po+tv7j/+eP+mqYMMFAARtLhpqas7WqgN0gvFzYxROodxHcIzpAxgOoAQ4CJ8lt6
ofjrHYwGUAcyQc07RzATKVA6YZkKNHTpRGUaBlKkFpXZ3OqIZKrSlsckrZ+0ZTF5ViYJ9Zgb7Gps
WtnKdh23dLxN6EQbz/V4WUeenXlxWpUJZ3Vu1wE7KtCgAgMQ46SArjcKIK3B61AAwQHWAYSwW4Hm
GELktgIybCtANFWGLQlkDRJ0jQZHFXJnka9ClpZlFVVrt/CYz5KDIMU6zTI/8ws3iF+lNppeAVkH
utJNLSvAgHoHNdIHBAKCtQ5qQ9aMFjXRsK7/K/Wed2G53LHyKDowNm0uaGhb6cr3V1ZR8uXa5cvS
TXgYxmVMX+VWDaABpAOsdXGjAWmk0Yzubit60221zY3RGTiWVdQGV7CC5UvgHz4dkFnkc7/epLyh
i6kdlOU6ZKx2vIBXPLHjujqIERdO5CzPkY82uwJ51+pOg+MBRMAgsBtZka9FPrW5svOah3R5AFsU
TlrHdJlUhbNZsqxcVVHEEmdNi9ROImrZ7mWDdw22hhopgCgXBptCmsF2ZnD93OAYnTET1VCv+Kr3
7D518jynfugHtGZWvKh54FSsrlKLBkffR3W0uYysdiNDDDAmF2Y5Bho6m2TK+SxXiEz+95t+gXZo
wxxYtEzWq6ObeeVkax65WZXbFhObS2yuW9xcwu/C77fkd5HURFITSU0kNbG5xOZ6W5tL+F34/Zb8
LpKaSGoiqYmkJjaX2Fxva3MJvwu/35LfRVITSU0kNZHUxOZ6N5vrHU/wP63fRspmJwAA"


data="`mktemp`"
out="`mktemp`"

echo -n "$vp" | base64 -d | gunzip > "$data"

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

