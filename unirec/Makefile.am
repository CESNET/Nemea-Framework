AM_CFLAGS=-std=c99 -O3

RPMDIR = RPMBUILD

lib_LTLIBRARIES=libunirec.la
libunirec_la_LDFLAGS=-static
libunirec_la_SOURCES=unirec.c

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = unirec.pc

unirecincludedir=$(includedir)/unirec
unirecinclude_HEADERS=fields.h \
		     inline.h \
		     ipaddr.h \
		     ipaddr_cpp.h \
		     links.h  \
		     unirec.h \
		     ur_time.h

BUILT_SOURCES = fields.h

EXTRA_DIST=fields field_func.c process_fields.py


fields.h field_func.c: fields process_fields.py
	$(PYTHON) process_fields.py

unirec.o: unirec.c unirec.h ipaddr.h fields.h field_func.c
	gcc -c ${CFLAGS} unirec.c -o $@

#tests:
#	make -C tests

clean-hook:
	rm -f fields.h field_func.c

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
