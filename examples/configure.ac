#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([examples], [1.0.0], [traffic-analysis@cesnet.cz])

AM_INIT_AUTOMAKE([foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes]]))

AC_CONFIG_SRCDIR([libtrap/module_info_example.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT()

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AX_TRAP_GETOPT()

AC_CONFIG_FILES([Makefile
                 libtrap/Makefile
                 python/Makefile])
AC_CONFIG_SUBDIRS([module])
AC_OUTPUT
