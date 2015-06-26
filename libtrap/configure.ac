#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libtrap], [0.7.0], [traffic-analysis@cesnet.cz])
#for debug purposes:
#AM_INIT_AUTOMAKE([-Wall -Werror])
AM_INIT_AUTOMAKE([-Wall silent-rules])
AC_CONFIG_SRCDIR([libtrap.spec.in])
AC_CONFIG_HEADERS([config.h])
RELEASE=1
AC_SUBST(RELEASE)
USERNAME=`git config --get user.name`
USERMAIL=`git config --get user.email`
AC_SUBST(USERNAME)
AC_SUBST(USERMAIL)
AM_SILENT_RULES([yes])


AC_CONFIG_MACRO_DIR([m4])
# Must be checked before default -g -O2 is set:
AC_ARG_ENABLE([debug],
        AC_HELP_STRING([--enable-debug],
        [Enable build with debug symbols and without optimizations.]),
        [if test "$enableval" = "yes"; then
                CFLAGS="-Wall -g -O0 $CFLAGS"
        fi], [CFLAGS="-Wall -O3 $CFLAGS"])

LT_INIT()

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
# Check for rpmbuild
AC_CHECK_PROG(RPMBUILD, rpmbuild, rpmbuild, [""])
AC_CHECK_PROG(GIT, git, git, [""])

if test -n "$GIT"; then
	GIT_VERSION=$($GIT log --format="%h" HEAD^..HEAD | head -1)
else
   GIT_VERSION=""
fi
AC_DEFINE_UNQUOTED(GIT_VERSION, ["$GIT_VERSION"], [Git revision])

AC_CHECK_LIB([rt], [shm_open])
AX_PTHREAD([LIBS="$PTHREAD_LIBS $LIBS"
	    CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
	    CC="$PTHREAD_CC"],
	    [AC_MSG_ERROR([pthread not found])]
	   )

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h stdarg.h string.h sys/socket.h unistd.h pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT8_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_FORK
AC_CHECK_FUNCS([clock_gettime memset munmap select socket strchr strdup strerror dup2 mkdir])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 include/Makefile
                 tests/Makefile
                 libtrap.pc
                 libtrap.spec
                 Doxyfile
                 devel-doxyfile
                 ])

AC_ARG_WITH([ifcbuffersize],
	AC_HELP_STRING([--with-ifcbuffersize=NUMBER],[Set size (in bytes) of libtrap buffer for messages, default is 100000.]),
	[
	TRAP_IFC_MESSAGEQ_SIZE=$withval
	],
	[
	TRAP_IFC_MESSAGEQ_SIZE=100000
	]
)

CPPFLAGS="$CPPFLAGS -DTRAP_IFC_MESSAGE_SIZE="$TRAP_IFC_MESSAGEQ_SIZE" "



AC_ARG_ENABLE([tests], AC_HELP_STRING([--enable-tests],[Compile with testing modules]),[with_tests=yes],[with_tests=no])
AS_IF([test "x$with_tests" = "xyes"], [ENABLEDTESTS=./tests], [ENABLEDTESTS=""])
AC_SUBST(ENABLEDTESTS)

AC_ARG_ENABLE([debugifc], AC_HELP_STRING([--enable-debugifc],
      [Compile IFC with debug messages]),
      [AC_DEFINE(TRAP_DEBUG_IFC, [1], [Enable debug messages in IFC.])], [AC_DEFINE([TRAP_DEBUG_IFC], [0])])

AC_ARG_ENABLE([debugbuffering], AC_HELP_STRING([--enable-debugbuffering],
      [Compile IFC with debug messages]),
      [AC_DEFINE(TRAP_DEBUG_BUFFERING, [1], [Enable debug messages in buffering.])], [AC_DEFINE([TRAP_DEBUG_BUFFERING], [0])])

AC_SUBST(RPMBUILD)
if test -z "$RPMBUILD"; then
	AC_MSG_WARN([Due to missing rpmbuild you will not able to generate RPM package.])
fi

RPM_RELEASE=1
AC_SUBST(RPM_RELEASE)
AM_CONDITIONAL(MAKE_RPMS, test x$RPMBUILD != x)

DX_MAN_FEATURE(OFF)
DX_INIT_DOXYGEN([libtrap], [Doxyfile], [doc/doxygen])

AC_OUTPUT
echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE $VERSION $GIT_VERSION"
echo "------------------------------------------------------------------------"
echo
echo
echo "Configuration Options Summary:"
echo
echo "  ASM.(32 bit only)..: $ASM"
echo "  Static binary......: $static"
echo
echo "Documentation..........: ${build_doc}"
echo
echo "Compilation............: make (or gmake)"
echo "  CPPFLAGS.............: $CPPFLAGS"
echo "  CFLAGS...............: $CFLAGS"
echo "  CXXFLAGS.............: $CXXFLAGS"
echo "  LDFLAGS..............: $LDFLAGS"
echo
echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...............: $prefix"
echo "Subdirs................: $ENABLEDTESTS"
echo

