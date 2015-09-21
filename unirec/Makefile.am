ACLOCAL_AMFLAGS=-I m4
DIST_SUBDIRS = tests

if ENABLE_TESTS
SUBDIRS= . tests
endif

AM_CFLAGS=-std=c99

RPMDIR=RPMBUILD

BUILT_SOURCES=ur_values.h ur_values.c ur_values.py

ur_values.h ur_values.c ur_values.py: process_values.py
	$(PYTHON) ${top_srcdir}/process_values.py -i ${top_srcdir}


lib_LTLIBRARIES=libunirec.la
libunirec_la_LDFLAGS=-static -ltrap
libunirec_la_SOURCES=unirec.c ur_values.c unirec.h inline.h ipaddr_cpp.h ipaddr.h links.h ur_time.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = unirec.pc

unirecincludedir=$(includedir)/unirec
unirecinclude_HEADERS=unirec.h \
		     inline.h \
		     ipaddr.h \
		     ipaddr_cpp.h \
		     links.h  \
		     ur_time.h \
		     ur_values.h

bin_SCRIPTS=unirec_generate_fields_files.py process_values.py

EXTRA_DIST=values unirec_generate_fields_files.py process_values.py

include doxygen.am

if DX_COND_doc

doc: doxygen-doc

clean-local: doxygen-clean
	rm -rf doc/doxygen
	find \( -name ur_values.c -o -name ur_values.h -o -name ur_values.py \) -exec rm -f {} +
	find \( -name fields.c -o -name fields.h \) -exec rm -f {} +
else
clean-local:
	find \( -name ur_values.c -o -name ur_values.h -o -name ur_values.py \) -exec rm -f {} +
	find \( -name fields.c -o -name fields.h \) -exec rm -f {} +
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
