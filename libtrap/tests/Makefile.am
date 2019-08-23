normal_tests_scripts=basic_test_arg.test libtrap_disbuffer.test test_service_ifc_fail.test
long_tests_scripts=libtrap_simpleapi.test libtrap_ctxapi.test libtrap_simpleapi_t.test libtrap_ctxapi_t.test

normal_tests_progs=test_badparams test_finalize test_blackhole test_fileifc

normal_tests=$(normal_tests_progs) $(normal_tests_scripts)
long_tests=$(long_tests_scripts)

tlscertfiles=tls-certificates/ca.crt tls-certificates/ca.key tls-certificates/client.crt tls-certificates/client.key tls-certificates/server.crt tls-certificates/server.key tls-certificates/unver.crt tls-certificates/unver.key

disabled_tests=libtrap_multiclient.test

TESTS = $(normal_tests)

if ENABLE_LONG_TESTS
TESTS += $(long_tests)
if HAVE_OPENSSL
TESTS += libtrap_tls.test
endif
endif


EXTRA_DIST = generate-report.sh test_reconnection.sh test_tcpip.sh test_tls.sh $(normal_tests_scripts) $(long_tests_scripts) $(tlscertfiles) $(disabled_tests)

check_PROGRAMS = basic_test $(normal_tests_progs)

if HAVE_OPENSSL
TESTS += test_tls.sh
endif

noinst_PROGRAMS = test_tcpip_wclient test_tcpip_wserver test_tcpip_nb5client test_tcpip_nb5server test_tcpip_client test_tcpip_server test_echo test_echo_reply test_echo_ctx test_echo_reply_ctx test_parse_params valid_buffer test_rxtx

AM_LDFLAGS=-static ../src/libtrap.la
COM_CPPFLAGS=-I../src -I../include -I${top_srcdir}/include -I${top_srcdir}/src

test_blackhole_SOURCES=test_blackhole.c
test_blackhole_CPPFLAGS=$(COM_CPPFLAGS)

test_fileifc_SOURCES=test_fileifc.c
test_fileifc_CPPFLAGS=$(COM_CPPFLAGS)

test_tcpip_wclient_SOURCES=test_trap_ifc_tcpip_client.c
test_tcpip_wclient_CPPFLAGS=-DWAITING $(COM_CPPFLAGS)

test_tcpip_wserver_SOURCES=test_trap_ifc_tcpip_server.c
test_tcpip_wserver_CPPFLAGS=$(COM_CPPFLAGS)

test_tcpip_nb5client_SOURCES=test_trap_ifc_tcpip_client.c
test_tcpip_nb5client_CPPFLAGS=$(COM_CPPFLAGS)

test_tcpip_nb5server_SOURCES=test_trap_ifc_tcpip_server.c
test_tcpip_nb5server_CPPFLAGS=$(COM_CPPFLAGS)

test_tcpip_client_SOURCES=test_trap_ifc_tcpip_client.c
test_tcpip_client_CPPFLAGS=-DBLOCKING $(COM_CPPFLAGS)
test_tcpip_server_SOURCES=test_trap_ifc_tcpip_server.c
test_tcpip_server_CPPFLAGS=-DBLOCKING $(COM_CPPFLAGS)

test_echo_SOURCES=test_echo.c
test_echo_CPPFLAGS=$(COM_CPPFLAGS)
test_echo_reply_SOURCES=test_echo_reply.c
test_echo_reply_CPPFLAGS=$(COM_CPPFLAGS)

test_echo_ctx_SOURCES=test_echo_ctx.c
test_echo_ctx_CPPFLAGS=$(COM_CPPFLAGS)
test_echo_reply_ctx_SOURCES=test_echo_reply_ctx.c
test_echo_reply_ctx_CPPFLAGS=$(COM_CPPFLAGS)

test_parse_params_SOURCES=test_parse_params.c 
test_parse_params_CPPFLAGS=$(COM_CPPFLAGS)

basic_test_SOURCES=basic_test.c
basic_test_CPPFLAGS=$(COM_CPPFLAGS)

test_finalize_SOURCES=test_finalize.c
test_finalize_CPPFLAGS=$(COM_CPPFLAGS)

test_rxtx_SOURCES=test_rxtx.c
test_rxtx_CPPFLAGS=$(COM_CPPFLAGS)

valid_buffer_SOURCES=valid_buffer.c

test_badparams_SOURCES=test_badparams.c
test_badparams_CPPFLAGS=$(COM_CPPFLAGS)

test_buffering$(EXEEXT):
	$(CC) -I../src -DTESTBUFFERING -o $@ -pthread -lrt ../src/trap.c ../src/trap_error.c ../src/trap_internal.c ../src/trap_ifc_dummy.c ../src/trap_ifc_tcpip.c test_buffering.c

clean-local:
	rm -f test_buffering
	rm -f *.log.* *.log *.rpt *.gcda *.gcno

if ENABLE_LONG_TESTS
check-hook:
	./generate-report.sh

check: check-hook

endif

