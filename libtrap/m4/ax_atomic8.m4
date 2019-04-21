AC_DEFUN([AX_ATOMIC8], [
AC_MSG_CHECKING([Atomic operations 8B])
AC_LANG_PUSH([C])
AC_LINK_IFELSE([AC_LANG_SOURCE([[
#include <stdint.h>
#include <inttypes.h>
int main(int argc, char **argv) {
	uint64_t a = 1;
	__sync_bool_compare_and_swap_8(&a, 1, 0);
}
]])], [AC_DEFINE([ATOMICOPS], [1], [supported atomic operations])
AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])
AC_LANG_POP([C])
])
