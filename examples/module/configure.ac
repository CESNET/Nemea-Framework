#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([example_module], [1.0.0], [traffic-analysis@cesnet.cz])
AC_CONFIG_SRCDIR([example_module.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign silent-rules subdir-objects])
AC_CONFIG_MACRO_DIR([m4])

AX_LIBTRAP_CHECK
AX_UNIREC_CHECK
AX_NEMEACOMMON_CHECK

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ltrap':
AC_CHECK_LIB([trap], [main])
# FIXME: Replace `main' with a function in `-lunirec':
AC_CHECK_LIB([unirec], [main])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
