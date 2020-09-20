#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([unirec], [2.9.3], [nemea@cesnet.cz])
AC_CONFIG_SRCDIR([unirec.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([silent-rules])
AM_SILENT_RULES([yes])
RELEASE=1
AC_SUBST(RELEASE)
USERNAME=`git config --get user.name`
USERMAIL=`git config --get user.email`
AC_SUBST(USERNAME)
AC_SUBST(USERMAIL)

AC_CONFIG_MACRO_DIR([m4])

# Must be checked before default -g -O2 is set:
AC_ARG_ENABLE([debug],
        AC_HELP_STRING([--enable-debug],
        [Enable build with debug symbols and without optimizations.]),
        [if test "$enableval" = "yes"; then
                CFLAGS="-Wall -g -O0 $CFLAGS"
                CXXFLAGS="-Wall -g -O0 $CXXFLAGS"
        fi],
        [CFLAGS="-Wall -g -O3 $CFLAGS"
        CXXFLAGS="-Wall -g -O3 $CXXFLAGS"])
LT_INIT()
# Checks for programs.
AM_PROG_CC_C_O
AC_PROG_CC_C99
AC_PROG_CXX
AC_PROG_LIBTOOL
AM_CONDITIONAL([GCC], [test x$GCC = xyes])

# Check the version of the compiler, if gcc is used and it is newer than 10, we
# need -fcommon in CFLAGS
if test x"$CC" = xgcc; then
    AC_MSG_CHECKING([for version of compiler])
    compiler_version="`$CC -dumpversion`"
    if test "$compiler_version" -ge 10 2>/dev/null ; then
        CFLAGS="$CFLAGS -fcommon"
	AC_MSG_RESULT([$compiler_version -> adding -fcommon into CFLAGS])
    else
	AC_MSG_RESULT([$compiler_version])
    fi
fi

# Check for rpmbuild
AC_CHECK_PROG(RPMBUILD, rpmbuild, rpmbuild, [""])
AC_CHECK_PROG(DEBUILD, debuild, debuild, [""])

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
AC_CHECK_FUNCS([memset strchr memmove])
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_MKTIME

RPM_RELEASE=1
AC_SUBST(RPM_RELEASE)
AM_CONDITIONAL(MAKE_RPMS, test x$RPMBUILD != x)

AM_CONDITIONAL(MAKE_DEB, test x$DEBUILD != x)

PKG_CHECK_MODULES([cmocka], [cmocka], [have_cmocka="yes"], [have_cmocka="no"])
AM_CONDITIONAL([HAVE_CMOCKA], [test x$have_cmocka = xyes])

if test -z "${top_srcdir}"; then
top_srcdir="."
fi
if test -z "$TRAPLIB"; then
        PKG_CHECK_MODULES([LIBTRAP],[libtrap], [TRAPLIB="yes"], [AC_MSG_WARN([Libtrap was not found by pkg-config])])
        LDFLAGS="$LIBTRAP_LDFLAGS $LDFLAGS"
        LIBS="$LIBTRAP_LIBS $LIBS"
        CFLAGS="$LIBTRAP_CFLAGS $CFLAGS"
        CXXFLAGS="$LIBTRAP_CFLAGS $CXXFLAGS"
fi
if test -z "$TRAPLIB"; then
        # Check for TRAP toolkit as a superproject.
        if test -d "${top_srcdir}/../libtrap"; then
        TRAPINC='${top_srcdir}/../libtrap/include'
        TRAPLIB='${top_builddir}/../libtrap/src/.libs'
        fi
        # AC_SUBST command line variables from TRAPLIB and TRAPINC.
        if test -n "$TRAPLIB"; then
        AC_SUBST([TRAP_LTLIB], ["$TRAPLIB/libtrap.la"])
        AC_SUBST([TRAP_INCLUDE], ["-I$TRAPINC"])
        LDFLAGS="-L$TRAPLIB $LDFLAGS"
        CFLAGS="-I$TRAPINC $CFLAGS"
        CXXFLAGS="-I$TRAPINC $CXXFLAGS"
        fi
fi

AC_CONFIG_FILES([Makefile
		 unirec.pc
		 unirec.spec
		 Doxyfile
                 tests/Makefile])

AC_ARG_ENABLE([tests], AC_HELP_STRING([--disable-tests],[Compile without testing modules]),
        [AM_CONDITIONAL([ENABLE_TESTS], [test "x$enableval" = xyes])],
        [AM_CONDITIONAL([ENABLE_TESTS], [true])])

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
echo

