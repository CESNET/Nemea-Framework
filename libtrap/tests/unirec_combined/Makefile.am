ur_c_normal_tests_scripts=libtrap_negclb.test   
ur_c_long_tests_scripts=

ur_c_normal_tests_progs=

ur_c_normal_tests=$(ur_c_normal_tests_progs) $(ur_c_normal_tests_scripts)
ur_c_long_tests=$(ur_c_long_tests_scripts)

ur_c_disabled_tests=

UR_C_TESTS = $(ur_c_normal_tests)

if ENABLE_UR_C_LONG_TESTS
UR_C_TESTS += $(ur_c_long_tests)
endif

noinst_PROGRAMS = testing_sender neg_clb_test

AM_LDFLAGS=-lunirec -static ../../src/libtrap.la
#TODO unirec.h?
COM_CPPFLAGS=-I../../include -I../../ -I../../../unirec -I${top_srcdir}/include -I${top_srcdir}/src

testing_sender_SOURCES=testing_sender.c fields.c
testing_sender_CPPFLAGS=$(COM_CPPFLAGS)

neg_clb_test_SOURCES=neg_clb_test.c fields.c
neg_clb_test_CPPFLAGS=$(COM_CPPFLAGS)

clean-local:
	rm -f testing_sender neg_clb_test
	rm -f *.log.* *.log *.rpt *.gcda *.gcno
  
include ./aminclude.am

#if ENABLE_UR_C_LONG_TESTS
#check-hook:
#	./generate-report.sh
#
#check: check-hook
