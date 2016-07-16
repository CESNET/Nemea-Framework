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

vp="H4sIAFMOilcAA71WTYwURRSuwZgQEjBKokb+Bn/QyE5vVXV1VRdLcGdnhqWzw24z3YOugmP/jYyB
nXV3wYAoEqIxmJgoGg+eDOFmgvHoDSHEeFAvBo0RjXqRoGYPnPXVzPTOTO/OQMhiV6rfvPdquvt7
732vagVSV+rd2rQXhjPpvONWLHugpTmlnNIO16bmOEuPTLoFJ1aK1vhYZcRyKzutQjE/MFc7FKVd
a3cB9JLjdujFLKjqTzpN29ncWMFtPoPwxsvsiZIb645lV3Y7oxV30i502hw365adSm4i3zbDly38
1UznrVLHxzRtdmnCnchNFFuqm7MrO4vZUSfWJxZ+ucWB2bmZ2tSLjbflssViIV+xsyV3Mmm3xkeX
dlSsfJfJKezp1EuFPeUCoC2XrE5z2SmUKtnRwrjbad1rZRFasxX9hlrX+9VrH/0LV6z/OPLJSqWv
XYG6rvXPhkO566ufjmUKbJfWBui91KW16N7GkgcaQ0MPoV1oNSqDpTnTaLY2vY0KZhIpsBg+4GWO
RAe14FgPM8OccJNybg4TxisE0wrVWYUYxBQGS5cKo5bjFkoAZ5BqeLCct9OwToN1GqzTYN02A3N8
c4xXU90YT36YG1PYYhlj/Ar9vxiNW8f4d/ztBy6sGujEeOZ86Qeln0vk8crZ6Q0zgC2WCuM3gPGD
dh7TMNahIbg/h1aiACzNubGBEWNGGDMoloSpjxcaEYZmwncRrvxCcKxmwiVxYIiAewYOmV8VVYWf
EioljnzfM6skbY3vtaAJdCLGgFTqgFpjDPCKTrw3op+78Kb1eaUfS+T093MfP6pwxnIJvI/AuB9J
wDsJeD2wNGczp8Q0WBuz4LIBTKeaQTTKSGMJxqTbqmOTVX0cBKEvI4ap5DonOlVQQ4iF3gstZQag
hUdQ0sT7T8/8fn9xndJfS+DNvXPwcYUzlj3yux5th/s+wBuCpTlb+d2azC8lkAJpaFTE+U0vJLjt
I9zwPZ34IojCyFS5NQwaCb3qcxxRiW8twSkcA7lwevyPTsAXX56/pvQ9iYK+8cS+QQU0lk3S3g33
L39tAd7YGNvRJijoe9BLYH0VEfQpEFkBYhRTnWIM34uHvVntSG36QH0qanG3txe4KLGQUhACfrgI
GwYgWJNCgTJ1E0u8wGT1rK6/Zw+HtXpQD6PZjFObzox6c9Er3tHMbjsDz0vvfMYZPKIZGsdZDVOm
Yax3B27hPSnaJ2LXlT50V3fE5s48mFeRimU7Yle/TkTsMaDBGlQHeryJnkTnQXbFhEnRJ2KLvFVZ
rTImgkxkcD+KPJ6B+vCBX34rcFyTEohCZO+wNdqeXT94NKgfcuqHp0K7DhutZWccG7Y3HZNMOTsI
FaVBqHRJFwdNvQPN96bVpS+UfiJBq8tjwlKxiuUStNoMYwPaAbR6HmhVBUtz9qcV9HST9+JVwxmJ
CMIWRD40kxBXDeFLGnEglsdZ5Ee3SKx+kE+eVvobCcjh0C8TCmoslfvbZYBMpUZ0swfkpnNZeglJ
8T7M+FPp+QQzVl05ayuosWwzY9eJFuRNjbEDbYGGuQbNACPeBmZ8nuwlzDBH+lFjkRu2fZ9xI8j4
hMPWGEYZFvhcmEJf4AYcGJjUhXlHyQEv6bvpbFb664lS+ev4U2UVr1guUSq3u+noeu9NR9eX5VRB
UuxmhcIShfLdwz9NKqCxbBdKaT7RQrcgHwplGgrlLSiUzxZtOqJfC13kFTol2IyijB7p0uOemWFc
BESSsL33iMbRkt7BMmm8pG9HObVUR4Ez5/7W2XP/sncU0q+jEMMUMtC90Bewk3pMVGkYCT+gpmcy
EfY+kHUXSj9mXN6r9OMJyFP3nXpBQY3lsjHD1HpvG8p3uyfQBOAU+g9pihIMxQ8AAA=="

expected="H4sIAHkNilcAA+WZbU/bSBDH39+nyPvDyz4/IJ1ESExkNSWu7VTHK8sPGxWpR5BoOfU+fWdxQpxc
7LIFctBDQjIzO+v17E/zn13GaZZH8eCPAeESEUwRZRwRQY4GaTJqPJQoJMEpCdJHg7PLLEzBKhiM
mUYX7/KzKMvPo3A6Bis+GmTR+xD+TtKsmRULhilnMDFdOafDvb54OHoXZm5umHkM64pniRsnsIRp
0yjO36eTPLuMQzAa05jSbJjN03w0Gzsrx6uFb4WOo2R3kXEyy2aj2dS9TMGyRnF+Ph1O0tUnzNYP
2bR5cK8aDafTcJzHwyS7BOvt1c0JVVwTo7A6/VQEd/Yzqv7ZjI0uJo8fnEfj+/VLIjWVUp9CdnLY
jxz2I2/2w41Mww8wTCvBB0k4idIsTBpPEn6Yh5CzeRLBgMY2T8MkH07Ci+zB9DEawjM8HVOEj+fj
eHffT1zKfhv/LBVcaU8qDO+mwvmegQqK8f+ECnEgKjBYDYMRiPMWE4QrRJRAGgKJ3FAhlfKgwmBE
iNhPxdrXTYV6fK3gB6UCY044FxQbwt1GbqeqCw+lJHa/nQENIgZXQlWyELjm5UItHDGUUGOwLctC
L7ZAIYPo4mOUhU9lpE3Bicv8NiGUC/ARRCnZYYRRJMDOyQYR5lU4HAacdSPifG8QEaIF32CipDnd
yVYXJQRj0jW2AYRhzRclrqq6NJZDliSThFFHR02kYS8FSAuCPYh0FRFKwG4EoqpFiPZpOBwFokNa
1r43SAj+fbeItFPVV0QGD1VkX0QDCZGiLBgpVWVrq10BEYJaxRalxBbSdvAq4uxMI6WBH91mBJZP
4DMAdCdRra7UkewFie5RGt2rNHv6D7GPkZ3mQwmmXxIRTjG00xjD3uHT4hbdXd18Wl7b3ibkMTEr
RFxmlDGKEBgPP4SfwtZhZJTbxu2GhGlscG9L4l6+531b5Ay/1lfLalnb2yC9ugkmxRf7d/EteB8H
sIbB+Z/p8R0SSOIhgq4RYcy6UXtYpzdjrW5GU89uBjq3Tsbufc/OGHwUPwhj3ChvxnpiGsYWZrHg
XFWBFbK0tpAB1KAStLFcoSbRuhqvSAMtI8YPtJNNcrdoi5efv1XLv9Ll1+s6Xl5df4niII1hFMMk
mA+PgRYEQDFD+zFza/QRO+i3tWyrnWcho7oHMt0L2ZtTu1WufORuJ6QBzSoLpFW2hK6oxguhSkOt
BL0rJLelPbze/YgSalytalPC/ShhpOMSZu3roYS8LUrWufKgZDfkFXVFxFuxWEuxnHz5YdLdFbH+
8/fPKRajQh9IsYQ+85esnqAGEq1EyaWogpJIOHvXNuBVKZVW7EGziNRbqHADh97XJlqwSL8jWpsz
pqknZ6z7EH/v+3XK0SpVfke0dsQruuh5WjGi2BMSd8/ZBYnzPXcxkkSpAx3RlH/73BPTIKIYJVhb
GzDLTCELHXCpKmJIvTmpKbkNitSUvq5SdL9I39aIPKU1kj2aJ/vvnN9YLVrnyrc1+tetotDKVKyo
SwXH7oKrBa2tKiuqC81V/XK3ip3V6IeCpdH2Kcu3Frn/ZHRB4ny/DiSrVPkI1nbEf3vxvIPId25h
BuzoHgAA"


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