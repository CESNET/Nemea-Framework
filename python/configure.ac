#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([nemea-python], [1.0.0], [traffic-analysis@cesnet.cz])
#for debug purposes:
#AM_INIT_AUTOMAKE([-Wall -Werror])
AM_INIT_AUTOMAKE([foreign silent-rules])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([nemea-python.spec.in])
RELEASE=1
AC_SUBST(RELEASE)
USERNAME=`git config --get user.name`
USERMAIL=`git config --get user.email`
AC_SUBST(USERNAME)
AC_SUBST(USERMAIL)

AC_CONFIG_MACRO_DIR([m4])

AC_ARG_WITH([unirec-path],
	AC_HELP_STRING([--with-unirec-path=PATH],[Absolute path to UniRec directory, usually relative path into repository.]),
	[
	UNIREC_PATH=$withval
	],
	[
	UNIREC_PATH=[${PWD}/../unirec]
	]
)

AC_CHECK_FILE([${UNIREC_PATH}/unirec_generate_fields_files.py], [], [
  AC_CHECK_FILE([${srcdir}/../unirec/unirec_generate_fields_files.py],
    [UNIREC_PATH=[${srcdir}/../unirec/]],
    [AC_MSG_ERROR([Path to UniRec not found. It is not possible to obtain fields.py.])])
])

AC_SUBST(UNIREC_PATH)

# Checks for programs.
AC_CHECK_PROG(PYTHON, python, python)
AC_CHECK_PROG(RPMBUILD, rpmbuild, rpmbuild, [""])
AC_PROG_INSTALL

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 unirec/Makefile
                 nemea-python.spec
                 ])

AC_SUBST(RELEASE)
AM_CONDITIONAL(MAKE_RPMS, test x$RPMBUILD != x)

AC_OUTPUT

