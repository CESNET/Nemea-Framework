#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([nemea-framework], [2.0.1], [traffic-analysis@cesnet.cz])
AC_CONFIG_SRCDIR([unirec/unirec.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign silent-rules subdir-objects])
AM_SILENT_RULES([yes])
RELEASE=1
AC_SUBST(RELEASE)
USERNAME=`git config --get user.name`
USERMAIL=`git config --get user.email`
AC_SUBST(USERNAME)
AC_SUBST(USERMAIL)

AC_PROG_CC

# Check for rpmbuild
AC_CHECK_PROG(RPMBUILD, rpmbuild, rpmbuild, [""])
AC_CHECK_PROG(PYTHON, python, python, [""])
AC_SUBST(PYTHON)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([getopt_long getopt])

# Define TRAP_GETOPT() for example of module_info
if test "x$ac_cv_func_getopt_long" = xyes; then
  AC_DEFINE_UNQUOTED([TRAP_GETOPT(argc, argv, optstr, longopts)],
    [getopt_long(argc, argv, optstr, longopts, NULL)],
    [Trap getopt macro. Argc and argv are number and values of arguments, optstr is a string containing legitimate option characters, longopts is the array of option structures (unused for on system without getopt_long())])
elif test "x$ac_cv_func_getopt" = xyes; then
  AC_DEFINE_UNQUOTED([TRAP_GETOPT(argc, argv, optstr, longopts)],
  [getopt(argc, argv, optstr)],
  [Trap getopt macro. Argc and argv are number and values of arguments, optstr is a string containing legitimate option characters, longopts is the array of option structures (unused for on system without getopt_long())])
else
  AC_MSG_ERROR([getopt() was not found, module depend on it...])
fi

pkgdatadir=${datadir}/nemea
PKGDATADIR=$(eval echo $(eval echo ${pkgdatadir}))
AC_SUBST(PKGDATADIR)
AC_DEFINE_DIR(DATADIR, [datadir], [Path to datadir files])
AC_DEFINE_DIR(PKGDATADIR, [pkgdatadir], [Path to pkgdatadir files])
AC_DEFINE_DIR(SYSCONFDIR, [sysconfdir], [Path to sysconfdir files])

AC_SUBST(RPMBUILD)
if test -z "$RPMBUILD"; then
	AC_MSG_WARN([Due to missing rpmbuild you will not able to generate RPM package.])
fi

RPM_RELEASE=1
AC_SUBST(RPM_RELEASE)
AM_CONDITIONAL(MAKE_RPMS, test x$RPMBUILD != x)

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

AC_ARG_ENABLE([examples], AC_HELP_STRING([--disable-example], [Disable examples.]),
        [AM_CONDITIONAL([ENABLE_EXAMPLES], [test "x$enableval" = xyes])],
        [AM_CONDITIONAL([ENABLE_EXAMPLES], [true])])

AC_CONFIG_FILES([Makefile
		 pycommon/Makefile
		 python/Makefile
		 python/unirec/Makefile
                 examples/Makefile
                 examples/libtrap/Makefile
                 examples/python/Makefile
		 nemea-framework.spec
		 create_module.sh])

AC_CONFIG_SUBDIRS([libtrap common unirec])
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
echo "  pycommon...............: `if [[ -z "$ENABLE_PYCOMMON_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo "  unirec.................: `if [[ -z "$ENABLE_UNIREC_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo "  examples...............: `if [[ -z "$ENABLE_EXAMPLES_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo

