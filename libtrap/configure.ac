#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libtrap], [0.7.6], [traffic-analysis@cesnet.cz])
#for debug purposes:
#AM_INIT_AUTOMAKE([-Wall -Werror])
AM_INIT_AUTOMAKE([subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes]]))
AC_CONFIG_SRCDIR([libtrap.spec.in])
AC_CONFIG_HEADERS([config.h])
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
        fi], [CFLAGS="-Wall -O3 $CFLAGS"
              CPPFLAGS="-DNDEBUG=1"])

LT_INIT()

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AM_CONDITIONAL([GCC], [test x$GCC = xyes])

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
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h stdarg.h string.h sys/socket.h sys/time.h unistd.h pthread.h endian.h locale.h sched.h sys/param.h sys/stat.h sys/types.h getopt.h])


# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_LONG_LONG_INT
AC_C_INLINE
case $ac_cv_c_inline in
    yes) json_inline=inline;;
    no) json_inline=;;
    *) json_inline=$ac_cv_c_inline;;
esac
AC_SUBST([json_inline])
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
AC_CHECK_FUNCS([close getpid gettimeofday localeconv open read sched_yield strtoll getopt_long])

AC_MSG_CHECKING([for gcc __sync builtins])
have_sync_builtins=no
AC_TRY_LINK(
  [], [unsigned long val; __sync_bool_compare_and_swap(&val, 0, 1);],
  [have_sync_builtins=yes],
)
if test "x$have_sync_builtins" = "xyes"; then
  AC_DEFINE([HAVE_SYNC_BUILTINS], [1],
    [Define to 1 if gcc's __sync builtins are available])
fi
AC_MSG_RESULT([$have_sync_builtins])

AC_MSG_CHECKING([for gcc __atomic builtins])
have_atomic_builtins=no
AC_TRY_LINK(
  [], [char l; unsigned long v; __atomic_test_and_set(&l, __ATOMIC_RELAXED); __atomic_store_n(&v, 1, __ATOMIC_RELEASE); __atomic_load_n(&v, __ATOMIC_ACQUIRE);],
  [have_atomic_builtins=yes],
)
if test "x$have_atomic_builtins" = "xyes"; then
  AC_DEFINE([HAVE_ATOMIC_BUILTINS], [1],
    [Define to 1 if gcc's __atomic builtins are available])
fi
AC_MSG_RESULT([$have_atomic_builtins])

case "$ac_cv_type_long_long_int$ac_cv_func_strtoll" in
     yesyes) json_have_long_long=1;;
     *) json_have_long_long=0;;
esac
AC_SUBST([json_have_long_long])

case "$ac_cv_header_locale_h$ac_cv_func_localeconv" in
     yesyes) json_have_localeconv=1;;
     *) json_have_localeconv=0;;
esac
AC_SUBST([json_have_localeconv])

AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_FORK
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime memset munmap select socket strchr strdup strerror dup2 mkdir])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 include/Makefile
                 tests/Makefile
                 tools/Makefile
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



AC_ARG_ENABLE([tests], AC_HELP_STRING([--disable-tests],[Compile with testing modules]),
        [AM_CONDITIONAL([ENABLE_TESTS], [test "x$enableval" = xyes])],
        [AM_CONDITIONAL([ENABLE_TESTS], [true])])

AC_ARG_ENABLE([long-tests], AC_HELP_STRING([--enable-long-tests], [Run long performance tests (--enable-tests must be used for any effect)]),
        [AM_CONDITIONAL([ENABLE_LONG_TESTS], [test "x$enableval" = xyes])],
        [AM_CONDITIONAL([ENABLE_LONG_TESTS], [false])])

AC_ARG_ENABLE([debugifc], AC_HELP_STRING([--enable-debugifc],
      [Compile IFC with debug messages]),
      [AC_DEFINE(TRAP_DEBUG_IFC, [1], [Enable debug messages in IFC.])], [AC_DEFINE([TRAP_DEBUG_IFC], [0])])

AC_ARG_ENABLE([debugbuffering], AC_HELP_STRING([--enable-debugbuffering],
      [Compile IFC with debug messages]),
      [AC_DEFINE(TRAP_DEBUG_BUFFERING, [1], [Enable debug messages in buffering.])], [AC_DEFINE([TRAP_DEBUG_BUFFERING], [0])])

AC_ARG_ENABLE([negotiation], AC_HELP_STRING([--disable-negotiation],
      [Compile interfaces with negotiation]),
      [if test "$enableval" = "yes"; then
        AC_DEFINE(ENABLE_NEGOTIATION, [1], [Disable negotiation between interfaces.])
      fi],
      [AC_DEFINE(ENABLE_NEGOTIATION, [1], [Enable negotiation between interfaces.])])

AC_SUBST(RPMBUILD)
if test -z "$RPMBUILD"; then
	AC_MSG_WARN([Due to missing rpmbuild you will not able to generate RPM package.])
fi

RPM_RELEASE=1
AC_SUBST(RPM_RELEASE)
AM_CONDITIONAL(MAKE_RPMS, test x$RPMBUILD != x)


# Features libjansson
AC_ARG_ENABLE([urandom],
  [AS_HELP_STRING([--disable-urandom],
    [Don't use /dev/urandom to seed the hash function])],
  [use_urandom=$enableval], [use_urandom=yes])

if test "x$use_urandom" = xyes; then
AC_DEFINE([USE_URANDOM], [1],
  [Define to 1 if /dev/urandom should be used for seeding the hash function])
fi

AC_ARG_ENABLE([windows-cryptoapi],
  [AS_HELP_STRING([--disable-windows-cryptoapi],
    [Don't use CryptGenRandom to seed the hash function])],
  [use_windows_cryptoapi=$enableval], [use_windows_cryptoapi=yes])

if test "x$use_windows_cryptoapi" = xyes; then
AC_DEFINE([USE_WINDOWS_CRYPTOAPI], [1],
  [Define to 1 if CryptGenRandom should be used for seeding the hash function])
fi

AC_ARG_ENABLE([initial-hashtable-order],
  [AS_HELP_STRING([--enable-initial-hashtable-order=VAL],
    [Number of buckets new object hashtables contain is 2 raised to this power. The default is 3, so empty hashtables contain 2^3 = 8 buckets.])],
  [initial_hashtable_order=$enableval], [initial_hashtable_order=3])
AC_DEFINE_UNQUOTED([INITIAL_HASHTABLE_ORDER], [$initial_hashtable_order],
  [Number of buckets new object hashtables contain is 2 raised to this power. E.g. 3 -> 2^3 = 8.])

DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
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
echo "Tests..................: `if [[ -z "$ENABLE_TESTS_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo "Long performance tests.: `if [[ -z "$ENABLE_LONG_TESTS_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo

