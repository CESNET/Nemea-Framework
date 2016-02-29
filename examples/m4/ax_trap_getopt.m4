dnl @synopsis AX_TRAP_GETOPT
dnl
dnl This macro test if getopt_long is present on the system.
dnl It sets TRAP_GETOPT() that can be used in modules.
dnl The prototype is:
dnl  AC_DEFINE_UNQUOTED([TRAP_GETOPT(argc, argv, optstr, longopts)],
dnl
dnl @category InstalledPackages
dnl @author Tomas Cejka <cejkat@cesnet.cz>
dnl @version 2016-02-23
dnl @license BSD

AC_DEFUN([AX_TRAP_GETOPT], [
   AC_CHECK_HEADERS([getopt.h])
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
])

