bin_PROGRAMS = test_basic test_creation  test_ipaddr  test_speed test_speed_o test_speed_ur test_speed_uro

AM_LDFLAGS=-static ../libunirec.la
COM_CPPFLAGS=-I../../

test_basic_SOURCES=test_basic.c
test_basic_CPPFLAGS=$(COM_CPPFLAGS)

test_creation_SOURCES=test_creation.c
test_creation_CPPFLAGS=$(COM_CPPFLAGS)

test_ipaddr_SOURCES=test_ipaddr.c
test_ipaddr_CPPFLAGS=$(COM_CPPFLAGS)

test_speed_SOURCES=test_speed.c
test_speed_CPPFLAGS=$(COM_CPPFLAGS)

test_speed_o_SOURCES=test_speed.c
test_speed_o_CPPFLAGS=$(COM_CPPFLAGS)

test_speed_ur_SOURCES=test_speed.c
test_speed_ur_CFLAGS=-DUNIREC
test_speed_ur_CPPFLAGS=$(COM_CPPFLAGS)

test_speed_uro_SOURCES=test_speed.c
test_speed_uro_CFLAGS=-DUNIREC
test_speed_uro_CPPFLAGS=$(COM_CPPFLAGS)

BUILT_SOURCES = fields.h fields.c

fields.h fields.c: ../unirec_generate_fields_files.py
	$(PYTHON) ../unirec_generate_fields_files.py

CLEANFILES = fields.c fields.h
