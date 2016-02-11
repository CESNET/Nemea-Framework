#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([nemea-framework], [1.0.0], [traffic-analysis@cesnet.cz])
AC_CONFIG_SRCDIR([unirec/unirec.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])
LT_INIT()
RELEASE=1
AC_SUBST(RELEASE)
USERNAME=`git config --get user.name`
USERMAIL=`git config --get user.email`
AC_SUBST(USERNAME)
AC_SUBST(USERMAIL)


# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
# Check for rpmbuild
AC_CHECK_PROG(RPMBUILD, rpmbuild, rpmbuild, [""])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

pkgdatadir=${datadir}/nemea
PKGDATADIR=$(eval echo $(eval echo ${pkgdatadir}))
AC_SUBST(PKGDATADIR)
AC_DEFINE_DIR(PKGDATADIR, [pkgdatadir], [Path to configuration files])
AC_DEFINE_DIR(SYSCONFDIR, [sysconfdir], [Path to configuration files])

AC_SUBST(RPMBUILD)
if test -z "$RPMBUILD"; then
	AC_MSG_WARN([Due to missing rpmbuild you will not able to generate RPM package.])
fi

RPM_RELEASE=1
AC_SUBST(RPM_RELEASE)
AM_CONDITIONAL(MAKE_RPMS, test x$RPMBUILD != x)

AC_CONFIG_FILES([Makefile
		 pycommon/Makefile
		 nemea.spec
		 create_module.sh])

AC_ARG_ENABLE([libtrap], AC_HELP_STRING([--disable-libtrap], [Disable libtrap.]),
        [AM_CONDITIONAL([ENABLE_LIBTRAP], [test "x$enableval" = xyes])],
        [AM_CONDITIONAL([ENABLE_LIBTRAP], [true])])

AC_ARG_ENABLE([common], AC_HELP_STRING([--disable-common], [Disable nemea-common.]),
        [AM_CONDITIONAL([ENABLE_NEMEACOMMON], [test "x$enableval" = xyes])],
        [AM_CONDITIONAL([ENABLE_NEMEACOMMON], [true])])

AC_ARG_ENABLE([pycommon], AC_HELP_STRING([--disable-pycommon], [Disable nemea-pycommon.]),
        [AM_CONDITIONAL([ENABLE_PYCOMMON], [test "x$enableval" = xyes])],
        [AM_CONDITIONAL([ENABLE_PYCOMMON], [true])])

AC_ARG_ENABLE([unirec], AC_HELP_STRING([--disable-unirec], [Disable UniRec.]),
        [AM_CONDITIONAL([ENABLE_UNIREC], [test "x$enableval" = xyes])],
        [AM_CONDITIONAL([ENABLE_UNIREC], [true])])

AC_ARG_ENABLE([python], AC_HELP_STRING([--disable-python], [Disable python wrappers.]),
        [AM_CONDITIONAL([ENABLE_PYTHON], [test "x$enableval" = xyes])],
        [AM_CONDITIONAL([ENABLE_PYTHON], [true])])

AC_CONFIG_SUBDIRS([libtrap common python unirec])
AC_OUTPUT

echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE $VERSION"
echo "------------------------------------------------------------------------"
echo "Configuration Options Summary:"
echo
echo "  ASM.(32 bit only)..: $ASM"
echo "  Static binary......: $static"
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
echo "Components:"
echo "  libtrap................: `if [[ -z "$ENABLE_LIBTRAP_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo "  nemea-common...........: `if [[ -z "$ENABLE_NEMEACOMMON_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo "  python.................: `if [[ -z "$ENABLE_PYTHON_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo "  unirec.................: `if [[ -z "$ENABLE_UNIREC_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo

