ACLOCAL_AMFLAGS=-I m4
DIST_SUBDIRS = tests

AM_CFLAGS=-std=c99 -O3 -lunirec

RPMDIR = RPMBUILD

lib_LTLIBRARIES=libunirec.la
libunirec_la_LDFLAGS=-static -ltrap
libunirec_la_SOURCES=unirec.c

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = unirec.pc

unirecincludedir=$(includedir)/unirec
unirecinclude_HEADERS=unirec.h \
		     inline.h \
		     ipaddr.h \
		     ipaddr_cpp.h \
		     links.h  \
		     ur_time.h\
		     values.h

BUILT_SOURCES = values.h values.c

EXTRA_DIST=unirec_generate_fields_files.py process_values.py values

values.h values.c: process_values.py
	$(PYTHON) process_values.py

unirec.o: unirec.c unirec.h ipaddr.h values.h values.c
	gcc -c ${CFLAGS} unirec.c -o $@

#tests:
#	make -C tests

clean-hook:
	rm -f values.h values.c values.py

include doxygen.am

if DX_COND_doc

doc: doxygen-doc

clean-local: doxygen-clean
	rm -rf doc/doxygen
endif

if MAKE_RPMS

RPMFILENAME=$(PACKAGE_NAME)-$(VERSION)
.PHONY: rpm
rpm:
	rm -rf "$(RPMDIR)/SOURCES/$(RPMFILENAME)"
	mkdir -p $(RPMDIR)/BUILD/ $(RPMDIR)/SRPMS/ $(RPMDIR)/RPMS/ $(RPMDIR)/SOURCES
	make ${AM_MAKEFLAGS} distdir='$(RPMDIR)/SOURCES/$(RPMFILENAME)' distdir
	( cd "$(RPMDIR)/SOURCES/"; tar -z -c -f $(RPMFILENAME)-$(RELEASE).tar.gz $(RPMFILENAME); rm -rf $(RPMFILENAME); )
	$(RPMBUILD) -ba $(PACKAGE_NAME).spec --define "_topdir `pwd`/$(RPMDIR)";

rpm-clean:
	rm -rf $(RPMDIR)
else
endif
