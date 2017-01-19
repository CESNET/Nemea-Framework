#!/bin/bash

if test "$#" -lt 1 -o "-h" = "$1"; then
   echo "Usage: $0 file..."
   echo ""
   echo "The first argument defines the first directory where the script
looks up the 'configure.ac'.  It goes up over parent directories till
the root (/)."
   exit 0
fi

for i in "$@"; do
   if ! test -f "$i"; then
      echo "Argument '$i' must be a regular file."
      exit 1
   fi
done

d="`dirname "$1"`"

config="$(while ! test "$d" -ef /; do
   if test -f "$d"/configure.ac; then
      echo "$d"/configure.ac
      break
   else
      d="$d/../"
   fi
done)"

version="`grep AC_INIT "$config" | sed 's/^.*\[.*\], \[\(.*\)\], *\[.*\])/\1/'`"

(sed -n '/#define MODULE_BASIC_INFO(BASIC)/,/[^\]$/p;' "$@" |
sed ':a; /\\$/{N;s/\\\n//;ba}; s/" *"//g';
sed -n '/#define MODULE_PARAMS(PARAM) \\/,/[^\]$/p' "$@"; ) |
awk '
/^.*BASIC\(/ {
sub(/^.*BASIC\(/, "");
split($0, a, ",");
s=length(a)
inifc=a[s-1]
outifc=a[s]
gsub("\"", "", a[1])
gsub("\"", "", a[2])
name=a[1]
sub("^[^,]*,","")
gsub(/ /, "", inifc)
gsub(/[ )]/, "", outifc)
if (inifc == -1) inifc = "variable"
if (outifc == -1) outifc = "variable"
sub("\",[^,]*,[^,]*)$", "")
sub("^ *\"", "")
descr=$0
}

/^ *PARAM\(/ {
sub(/^ *PARAM\(/, "");
gsub(", *\"", ",\"")
split($0, a, ",");
gsub("'\''", "", a[1])
gsub("\"", "", a[2])
gsub("\"", "", a[3])
param_long[a[1]] = a[2]
param_desc[a[1]] = a[3]
if (a[1] == "-") {
  param_add = 1
}
}

END {
if (NR == 0) {
   exit(2)
}
printf ".TH %s \"1\" \"%s\" \"%s %s\" \"User Commands\"\n", toupper(name), "'"`date +%B\ %Y`"'", "'$version'", name;
printf ".SH NAME\n"name" \\- manual page for "name" '"$version"'\n"
print ".SH SYNOPSIS\n.B "tolower(name)
printf "[\\fICOMMON\\fR]... [\\fIOPTIONS\\fR]..."
if (param_add == 1) {
printf "[\\fIADDITIONAL\\fR]...\n"
} else {
print ""
}
print ".SH DESCRIPTION"
printf ".TP\nNumber of Input IFC: \\fB%s\\fR\n..\n", inifc
printf ".TP\nNumber of Output IFC: \\fB%s\\fR\n..\n", outifc
print ".TP"
print descr
#print ".SS OPTIONS:"
print ".HP"
for (i in param_long) {
print "\\fB\\-"i"\\fR, \\fB\\-\\-"param_long[i]"\\fR"
print param_desc[i]
print ".TP"
}
print ".SH COMMON OPTIONS:\n.TP"
print "\\fB\\-h\\fR \\fB[trap,1]\\fR"
print "If no argument, print this message. If \\fItrap\\fR or 1 is given, print TRAP help."
print ".TP"
print "\\fB\\-i\\fR \\fBIFC_SPEC\\fR"
print "Specification of interface types and their parameters, see \\fI\\-h trap\\fR (mandatory parameter)."
print ".TP"
print "\\fB\\-v\\fR"
print "Be verbose."
print ".TP"
print "\\fB\\-vv\\fR"
print "Be more verbose."
print ".TP"
print "\\fB\\-vvv\\fR"
print "Be even more verbose."
print ".SH ENVIRONMENT VARIABLES"
print ".TP"
print "\\fBPAGER\\fR"
print "Show the help output in the set \\fBPAGER\\fR."
print ".TP"
print "\\fBLIBTRAP_OUTPUT_FORMAT\\fR"
print "If set to \\fIjson\\fR, information about module is printed in JSON format."
print ".TP"
print "\\fBTRAP_SOCKET_DIR\\fR"
print "Redefines directory for UNIX sockets."
print ".SH BUGS"
print "Please send problems, bugs, questions, desirable enhancements, patches etc. to:"
print "\\fInemea@cesnet.cz\\fR"


print ""



}'
