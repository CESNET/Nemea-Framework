#!/bin/bash -x
# \file test_tls.sh
# \author Jaroslav Hlavac <hlavaj20@fit.cvut.cz>
# \date 2017
#
# Copyright (C) 2017 CESNET
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
#

EX=0 #exit status of module
PROG_ECHO="test_echo"
PROG_ECHO_REPLY="test_echo_reply"

test -z "$srcdir" && export srcdir=.

#kill leftover tests
pkill "^$PROG_ECHO" &> /dev/null && sleep 1
pkill "^$PROG_ECHO_REPLY" &> /dev/null && sleep 1

INTERFACE_SENDER="T:12345:${srcdir}/tls-certificates/server.key:${srcdir}/tls-certificates/server.crt:${srcdir}/tls-certificates/ca.crt"
INTERFACE_RECEIVER="T:12345:${srcdir}/tls-certificates/unver.key:${srcdir}/tls-certificates/unver.crt:${srcdir}/tls-certificates/ca.crt"

ECHOOUT=echoout-tls.log
REPLYOUTOK=replyout-tls.log
REPLYOUTBAD=replyout-tls-bad.log
ERTIME=5
ERSIZE=100

./$PROG_ECHO -i $INTERFACE_SENDER -n $ERSIZE > "$ECHOOUT" 2>&1 &
echopid=$!

#running the client with wrong certificate
./$PROG_ECHO_REPLY -i $INTERFACE_RECEIVER > "$REPLYOUTBAD" 2>&1
sleep 1
replypid=$!

if [ "`pgrep "^$PROG_ECHO_REPLY$"`" != "" ]; then
   echo "Failed to identify wrong certificate."
   kill -9 $echopid 2>/dev/null
   exit 1
fi

INTERFACE_RECEIVER="T:12345:${srcdir}/tls-certificates/client.key:${srcdir}/tls-certificates/client.crt:${srcdir}/tls-certificates/ca.crt"

#running the client with valid certificate
./$PROG_ECHO_REPLY -i $INTERFACE_RECEIVER > "$REPLYOUTOK" 2>&1 &
replypid=$!

echo "Running for ${ERTIME}s with ${ERSIZE}B messages"
sleep $ERTIME
echo "Shutting sender"
kill -INT $echopid 2> /dev/null
sleep 1
kill -INT $replypid 2> /dev/null

RECV=$(grep "Last received value" "$REPLYOUTOK" | sed 's/.* \([0-9]*\)/\1/')
SENT=$(grep "Last sent" "$ECHOOUT" | sed 's/.* \([0-9]*\)/\1/')
ERRCOUNT=$(grep Error "$REPLYOUTOK" | cut -f2 -d" ")
echo "Errors: $ERRCOUNT"
if [ $ERRCOUNT != "0" ]; then
   echo "Some errors occured. ($ERRCOUNT)"
   exit 1
fi

if [[ ( ( -n "$RECV" ) && ( -n "$SENT" ) ) && ( x"$RECV" = x"$SENT" ) ]]; then
   echo "OK: $RECV/$SENT (R/S)"
else
   echo "FAILED: $RECV/$SENT (R/S)"
   exit 1
fi
echo "Average MPS: $((RECV/ERTIME))"

echo "scale=3; print \"AVG speed: \", (($RECV*$ERSIZE*8)/1000000)/$ERTIME, \" Mbps\"" | bc
echo ""
