#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([nemea-common], [1.6.2], [traffic-analysis@cesnet.cz])
AC_CONFIG_SRCDIR([nemea-common.spec.in])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([silent-rules subdir-objects])
AM_SILENT_RULES([yes])
RELEASE=1
AC_SUBST(RELEASE)
USERNAME=`git config --get user.name`
USERMAIL=`git config --get user.email`
AC_SUBST(USERNAME)
AC_SUBST(USERMAIL)

AC_ARG_ENABLE([debug],
        AC_HELP_STRING([--enable-debug],
        [Enable build with debug symbols and without optimizations.]),
        [if test "$enableval" = "yes"; then
                CFLAGS="-Wall -g -O0 $CFLAGS"
                CXXFLAGS="-Wall -g -O0 $CXXFLAGS"
        fi], [CFLAGS="-Wall -g -O3 $CFLAGS"
              CXXFLAGS="-Wall -g -O3 $CXXFLAGS"])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")
LT_INIT()

pkgincludedir=${includedir}/nemea-common
AC_SUBST(pkgincludedir)

AC_ARG_ENABLE(repobuild, AS_HELP_STRING([--enable-repobuild],
		[enable local compilation without system installed Nemea libraries, default: no]),
[case "${enableval}" in
	yes) repobuild=true ;;
	no)  repobuild=false ;;
	*)   AC_MSG_ERROR([bad value ${enableval} for --enable-repobuild]) ;;
esac], [repobuild=false])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_CXX
AC_PROG_LIBTOOL
# Check for rpmbuild
AC_CHECK_PROG(RPMBUILD, rpmbuild, rpmbuild, [""])
AC_CHECK_PROG(DEBUILD, debuild, debuild, [""])
AC_CHECK_PROG(GIT, git, git, [""])


m4_include([m4/ax_libxml2_check.m4])
AC_ARG_WITH([libxml2],
	AC_HELP_STRING([--without-libxml2],[Disable libxml2 dependent components.]),
	[
	if test x$withval = xyes; then
		AX_LIBXML2_CHECK()
		AM_CONDITIONAL([WITH_LIBXML2], [true])
	else
		AM_CONDITIONAL([WITH_LIBXML2], [false])
	fi
	],
	[
	AX_LIBXML2_CHECK()
	AM_CONDITIONAL([WITH_LIBXML2], [true])
	]
)

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.

RPM_RELEASE=1
AC_SUBST(RPM_RELEASE)
AM_CONDITIONAL(MAKE_RPMS, test x$RPMBUILD != x)
AM_CONDITIONAL(MAKE_DEB, test x$DEBUILD != x)

AC_ARG_ENABLE([tests], AC_HELP_STRING([--disable-tests],[Compile with testing modules]),
        [AM_CONDITIONAL([ENABLE_TESTS], [test "x$enableval" = xyes])],
        [AM_CONDITIONAL([ENABLE_TESTS], [true])])


DX_MAN_FEATURE(OFF)
DX_INIT_DOXYGEN([nemeacommon], [Doxyfile], [doc])

AC_CONFIG_FILES([Makefile
		 Doxyfile
		 include/Makefile
		 nemea-common.pc
		 nemea-common.spec
		 tests/Makefile])

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
echo "Tests..................: `if [[ -z "$ENABLE_TESTS_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo

