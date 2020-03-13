#!/bin/bash
# \file test_tcpip.sh
# \brief Run tests of test_tcpip_{server,client} and test_echo{,_reply}
# which use TCP/IP IFC of the TRAP library.
# \author Tomas Cejka <cejkat@cesnet.cz>
# \author Katerina Pilatova <xpilat05@cesnet.cz>
# \date 2013
# \date 2014
# \date 2015
#
# Copyright (C) 2013,2014,2015 CESNET
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
#
# set -x
# number of lines that client receives before exit
EXPECT=100

if [ $# -ne 2 ]; then
	echo "Usage: $0 simple|ctx|2thr trapparams"
        exit 0
fi

INTERFACE="$2"

if [ "x$1" == "xsimple" ]; then
	PROG_ECHO="test_echo"
	PROG_ECHO_REPLY="test_echo_reply"
elif [ "x$1" == "xctx" ]; then
	PROG_ECHO="test_echo_ctx"
	PROG_ECHO_REPLY="test_echo_reply_ctx"
elif [ "x$1" == "x2thr" ]; then
	PROG_ECHO="test_echo_2thr"
	PROG_ECHO_REPLY="test_echo_2thr_reply"
fi

#PROG_ECHO="shmem_echo"
#PROG_ECHO_REPLY="shmem_echo_reply"

DEBUG="yes"
unset DEBUG #comment out for DEBUG:


#############################################################
# Function declarations:
#############################################################

# first argument is number of test
# second argument is NUMTRY
function run_client()
{
if [ $# -lt 2 ]; then
	NUMTRY=10000
else
	NUMTRY="$2"
fi

echo "Running test#$1: $NUMTRY x connect"
for i in `seq 1 $NUMTRY`; do
	LINES="$(./test_tcpip_client 2>&1)"
	NUMRECV=$(echo "$LINES" | grep recv | wc -l)
	NUMERRS=$(echo "$LINES" | grep -e ERROR | wc -l)

	if [ "$NUMRECV" -eq "$EXPECT" ]; then
		if [ "$NUMERRS" -ne 0 ]; then
			echo "$LINES" > "test$1.lines.$i"
			echo "$i OK with errors"
		fi
	else
		echo "$i FAILED ! ($NUMRECV lines instead of $EXPECT)";
		echo "$LINES" > "test$1.lines.$i"
	fi;
done
}

# first argument is number of test
# second argument is number of clients
# third argument is size of sent data
function run_test()
{
	echo "Test#$1 Start server with $3 size"
	./test_tcpip_server $3 2>&1 > /dev/null &
	serverpid=$!
	run_client $1 $2
	echo "Test#$1 finished, stopping server"
	kill $serverpid
}


# first argument duration time of test
# second argument size of message payload
function run_ertest()
{
   ERTIME="$1"
   ERSIZE="$2"
   if [ -n "$DEBUG" ]; then echo "Echo-reply test"; fi
   ERSIZESTR=$(printf "%05d" $ERSIZE)
   ECHOOUT=echoout.log.$ERSIZESTR
   REPLYOUT=replyout.log.$ERSIZESTR

   pgrep "^$PROG_ECHO_REPLY$" > /dev/null && { echo "Existing process..."; exit 1; }
   pgrep "^$PROG_ECHO$" > /dev/null && { echo "Existing process..."; exit 1; }

   ./$PROG_ECHO_REPLY -i $INTERFACE > "$REPLYOUT" 2>&1 &
   replypid=$!
   ./$PROG_ECHO -i $INTERFACE -n $ERSIZE > "$ECHOOUT" 2>&1 &
   echopid=$!

   if [ -n "$DEBUG" ]; then echo "Running for ${ERTIME}s with ${ERSIZE}B messages"; fi
   sleep $ERTIME
   if [ -n "$DEBUG" ]; then echo "Shutting sender"; fi
   kill -INT $echopid 2> /dev/null
   if [ -n "$DEBUG" ]; then echo "Wait for shutting client"; fi
   sleep 3
   kill -INT $replypid 2> /dev/null

   RECV=$(grep "Last received value" "$REPLYOUT" | sed 's/.* \([0-9]*\)/\1/')
   SENT=$(grep "Last sent" "$ECHOOUT" | sed 's/.* \([0-9]*\)/\1/')
   ERRCOUNT=$(grep Error "$REPLYOUT" | cut -f2 -d" ")
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
}

#############################################################
# MAIN:
#############################################################
time {

##default payload size of test_tcpip_server
#run_test 1 1000
#
##run_test <number of test> <number of clients (sequential)> <size of payload>
#run_test 2 1000 1000
#
#run_test 3 1000 4096

export LD_LIBRARY_PATH=../
export GMON_OUT_PREFIX=gmon.out
echo ldd $PROG_ECHO
ldd $PROG_ECHO
echo ldd $PROG_ECHO_REPLY
ldd $PROG_ECHO_REPLY

#run_ertest <duration in seconds> <size of message>
run_ertest 5 65533
run_ertest 5 32767
run_ertest 5 16383
run_ertest 5 8191
run_ertest 5 4095
run_ertest 5 2047
run_ertest 5 1023
run_ertest 5 511
run_ertest 5 300
run_ertest 5 255
run_ertest 5 200
run_ertest 5 127
run_ertest 5 66

##long test
#run_ertest 1200 300
}

exit 0

