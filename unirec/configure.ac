#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([unirec], [2.0.2], [traffic-analysis@cesnet.cz])
AC_CONFIG_SRCDIR([unirec.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall  silent-rules])
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
                CXXFLAGS="-Wall -g -O0 $CXXFLAGS"
        fi],
        [CFLAGS="-Wall -O3 $CFLAGS"
        CXXFLAGS="-Wall -O3 $CXXFLAGS"])
LT_INIT()
# Checks for programs.
AC_PROG_CC_C99
AC_PROG_CXX
AC_PROG_LIBTOOL
# Check for rpmbuild
AC_CHECK_PROG(RPMBUILD, rpmbuild, rpmbuild, [""])
AC_CHECK_PROG(PYTHON, python, python, [""])
AC_SUBST(PYTHON)

if test "x$PYTHON" = "x"; then
	AC_MSG_ERROR([Python not found.])
fi

AC_C_INLINE

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h arpa/inet.h sys/socket.h sys/types.h])
AC_HEADER_STDBOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_CHECK_FUNCS([memset strchr])
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MKTIME

RPM_RELEASE=1
AC_SUBST(RPM_RELEASE)
AM_CONDITIONAL(MAKE_RPMS, test x$RPMBUILD != x)


AC_CONFIG_FILES([Makefile
                 tests/Makefile
		 unirec.pc
		 unirec.spec
		 Doxyfile])

DX_MAN_FEATURE(OFF)
DX_INIT_DOXYGEN([UniRec], [Doxyfile], [doc/doxygen])

AC_OUTPUT
echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE $VERSION"
echo "------------------------------------------------------------------------"
echo
echo
echo "Configuration Options Summary:"
echo
echo "  ASM.(32 bit only)..: $ASM"
echo "  Static binary......: $static"
echo
echo "Documentation..........: `if test "x$DX_FLAG_doc" = x1; then echo enabled; else echo disabled; fi`"
echo
echo "Compilation............: make (or gmake)"
echo "  CPPFLAGS.............: $CPPFLAGS"
echo "  CFLAGS...............: $CFLAGS"
echo "  CXXFLAGS.............: $CXXFLAGS"
echo "  LDFLAGS..............: $LDFLAGS"
echo
echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...............: $prefix"
echo

