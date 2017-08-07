#!/bin/bash -x

EX=0 #exit status of module
PROG_ECHO="test_echo"
PROG_ECHO_REPLY="test_echo_reply"

#checking if some leftover tests are running
pgrep "^$PROG_ECHO_REPLY$" > /dev/null && { echo "Existing process..."; exit 1; }
pgrep "^$PROG_ECHO$" > /dev/null && { echo "Existing process..."; exit 1; }

INTERFACE_SENDER="T:12345:tls-certificates/server.key:tls-certificates/server.crt:tls-certificates/ca.crt"
INTERFACE_RECEIVER="T:12345:tls-certificates/unver.key:tls-certificates/unver.crt:tls-certificates/ca.crt"

ECHOOUT=echoout-tls.log
REPLYOUTOK=replyout-tls.log
REPLYOUTBAD=replyout-tlsw-bad.log
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

INTERFACE_RECEIVER="T:12345:tls-certificates/client.key:tls-certificates/client.crt:tls-certificates/ca.crt"

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
