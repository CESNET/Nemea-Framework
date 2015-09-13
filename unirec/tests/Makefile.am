bin_PROGRAMS = test_basic test_creation  test_ipaddr  test_speed test_speed_o test_speed_ur test_speed_uro

AM_LDFLAGS=-static ../libunirec.la
COM_CPPFLAGS=-I../

test_basic_SOURCES=test_basic.c
test_basic_CFLAGS=-O2

test_creation_SOURCES=test_creation.c
test_creation_CFLAGS=-O2

test_ipaddr_SOURCES=test_ipaddr.c
test_ipaddr_CFLAGS=-O2

test_speed_SOURCES=test_speed.c
test_speed_CFLAGS=

test_speed_o_SOURCES=test_speed.c
test_speed_o_CFLAGS=-O2

test_speed_ur_SOURCES=test_speed.c
test_speed_ur_CFLAGS=-DUNIREC

test_speed_uro_SOURCES=test_speed.c
test_speed_uro_CFLAGS=-O2 -DUNIREC

BUILT_SOURCES = fields.h fields.c

fields.h fields.c: ../unirec_generate_fields_files.py
	$(PYTHON) ../unirec_generate_fields_files.py

CLEANFILES = fields.c fields.h
