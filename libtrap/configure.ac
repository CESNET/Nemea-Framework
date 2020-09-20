#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libtrap], [1.15.1], [nemea@cesnet.cz])
#for debug purposes:
#AM_INIT_AUTOMAKE([-Wall -Werror])
AM_INIT_AUTOMAKE([subdir-objects no-dependencies])
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
                CFLAGS="-Wall -Wno-format-truncation -g -O0 $CFLAGS"
        fi], [CFLAGS="-Wall -Wno-format-truncation -g -O3 $CFLAGS"
              CPPFLAGS="-DNDEBUG=1"])

LT_INIT()

AC_ARG_WITH([defaultsocketdir],
	[AS_HELP_STRING([--with-defaultsocketdir=DIR], [Directory for UNIX&service IFCs [/tmp], for production set it to e.g. /var/run/libtrap.])],
	[],
	[with_defaultsocketdir=/tmp])

AC_SUBST([defaultsocketdir], [$with_defaultsocketdir])
AC_DEFINE_DIR([DEFAULTSOCKETDIR], [defaultsocketdir], [Default path to socket directory])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LN_S
AC_PROG_INSTALL
AC_PROG_MAKE_SET
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
AC_CHECK_PROG(GIT, git, git, [""])
AC_CHECK_PROG(HAVE_DOT, dot, ["YES"], ["NO"])
AC_SUBST([HAVE_DOT])

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

PKG_CHECK_MODULES([cmocka], [cmocka], [have_cmocka="yes"], [have_cmocka="no"])
AM_CONDITIONAL([HAVE_CMOCKA], [test x$have_cmocka = xyes])

AC_ARG_WITH([openssl],
	[AS_HELP_STRING([--without-openssl], [Force to disable openssl])],
	[if test x$withval = xyes; then
        PKG_CHECK_MODULES([openssl], [openssl], [have_openssl="yes"], [have_openssl="no"])
        fi],
	[PKG_CHECK_MODULES([openssl], [openssl], [have_openssl="yes"], [have_openssl="no"])])

AM_CONDITIONAL([HAVE_OPENSSL], [test x$have_openssl = xyes])

if test x$have_openssl = xyes; then
  AC_DEFINE([HAVE_OPENSSL], [1], [Define to 1 if the openssl is available])
  LDFLAGS="$openssl_LDFLAGS $LDFLAGS"
  LIBS="$openssl_LIBS $LIBS"
  CFLAGS="$openssl_CFLAGS $CFLAGS"
  CXXFLAGS="$openssl_CFLAGS $CXXFLAGS"
else
  AC_DEFINE([HAVE_OPENSSL], [0], [Define to 1 if the openssl is available])
  AC_MSG_WARN([OpenSSL not found. You will not be able to use secure TLS interface.])
fi

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

AX_ATOMIC8()

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 include/Makefile
                 tests/Makefile
                 tools/Makefile
                 libtrap.pc
                 libtrap.spec
                 libtrap-varrun.conf
                 Doxyfile
                 devel-doxyfile
                 ])

AC_ARG_WITH([ifcbuffercount],
	AC_HELP_STRING([--with-ifcbuffercount=NUMBER],[Set number of buffers used for sending, default is 50.]),
	[
	DEFAULT_BUFFER_COUNT=$withval
	],
	[
	DEFAULT_BUFFER_COUNT=50
	]
)
AC_ARG_WITH([ifcbuffersize],
	AC_HELP_STRING([--with-ifcbuffersize=NUMBER],[Set size (in bytes) of libtrap buffer for messages, default is 100000.]),
	[
	TRAP_IFC_MESSAGEQ_SIZE=$withval
	DEFAULT_BUFFER_SIZE=$withval
	],
	[
	TRAP_IFC_MESSAGEQ_SIZE=100000
	DEFAULT_BUFFER_SIZE=100000
	]
)
AC_ARG_WITH([ifcmaxclients],
	AC_HELP_STRING([--with-ifcmaxclients=NUMBER],[Set number of clients that can connect to output interface, default is 64.]),
	[
	DEFAULT_MAX_CLIENTS=$withval
	],
	[
	DEFAULT_MAX_CLIENTS=64
	]
)

CPPFLAGS="$CPPFLAGS -DDEFAULT_BUFFER_COUNT="$DEFAULT_BUFFER_COUNT" -DDEFAULT_BUFFER_SIZE="$DEFAULT_BUFFER_SIZE" -DDEFAULT_MAX_CLIENTS="$DEFAULT_MAX_CLIENTS" -DTRAP_IFC_MESSAGEQ_SIZE="$TRAP_IFC_MESSAGEQ_SIZE" "



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
      [Compile buffering with debug messages]),
      [AC_DEFINE(TRAP_DEBUG_BUFFERING, [1], [Enable debug messages in buffering.])], [AC_DEFINE([TRAP_DEBUG_BUFFERING], [0])])

AC_ARG_ENABLE([negotiation], AC_HELP_STRING([--disable-negotiation],
      [Compile interfaces without negotiation]),
      [if test "$enableval" = "yes"; then
        AC_DEFINE(ENABLE_NEGOTIATION, [1], [Enable negotiation between interfaces.])
      fi],
      [AC_DEFINE(ENABLE_NEGOTIATION, [1], [Enable negotiation between interfaces.])])

AC_SUBST(RPMBUILD)
if test -z "$RPMBUILD"; then
	AC_MSG_WARN([Due to missing rpmbuild you will not able to generate RPM package.])
fi

RPM_RELEASE=1
AC_SUBST(RPM_RELEASE)
AM_CONDITIONAL(MAKE_RPMS, test x$RPMBUILD != x)

AM_CONDITIONAL(MAKE_DEB, test x$DEBUILD != x)


# Features libjansson
AC_ARG_ENABLE([urandom],
  [AS_HELP_STRING([--disable-urandom],
    [Don't use /dev/urandom to seed the hash function])],
  [use_urandom=$enableval], [use_urandom=yes])

if test "x$use_urandom" = xyes; then
AC_DEFINE([USE_URANDOM], [1],
  [Define to 1 if /dev/urandom should be used for seeding the hash function])
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
echo "  LIBS.................: $LIBS"
echo
echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...............: $prefix"
echo
echo "Tests..................: `if [[ -z "$ENABLE_TESTS_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo "Long performance tests.: `if [[ -z "$ENABLE_LONG_TESTS_TRUE" ]]; then echo enabled; else echo disabled; fi`"
echo

