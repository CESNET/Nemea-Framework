ACLOCAL_AMFLAGS=-I m4
DIST_SUBDIRS = tests

if ENABLE_TESTS
SUBDIRS= . tests
endif

AM_CFLAGS=-std=c99

RPMDIR=RPMBUILD

BUILT_SOURCES=ur_values.h ur_values.c ur_values.py

ur_values.h: ur_values.sh
	${top_srcdir}/ur_values.sh -i ${top_srcdir}

ur_values.c: ur_values.h

ur_values.py: ur_values.h


lib_LTLIBRARIES=libunirec.la
libunirec_la_CFLAGS=-fPIC
libunirec_la_LDFLAGS=-static -ltrap
libunirec_la_SOURCES=unirec.c unirec.h ur_values.c ur_values.h inline.h ipaddr.h macaddr.h links.h ur_time.h unirec2csv.h unirec2csv.c ip_prefix_search.c ipps_internal.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = unirec.pc

unirecincludedir=$(includedir)/unirec
unirecinclude_HEADERS=unirec.h \
		     inline.h \
		     ipaddr.h \
		     macaddr.h \
		     links.h  \
		     ur_time.h \
		     unirec2csv.h \
		     ip_prefix_search.h \
		     ur_values.h

bin_SCRIPTS=ur_values.sh ur_processor.sh

EXTRA_DIST=values ur_values.sh ur_processor.sh \
		debian/changelog debian/control debian/copyright debian/compat debian/patches debian/patches/series \
		debian/README.Debian debian/rules debian/source debian/source/format debian/source/local-options \
		README.md \
		README-ip_prefix_search.md

include doxygen.am

if DX_COND_doc

doc: doxygen-doc

install-data-local:
	mkdir -p "$(DESTDIR)$(docdir)/unirec" && cp -R doc/html/* "$(DESTDIR)$(docdir)/unirec" || echo "Documentation was not generated yet."
endif

if MAKE_RPMS

RPMFILENAME=$(PACKAGE_NAME)-$(VERSION)
.PHONY: srpm
srpm:
	rm -rf "$(RPMDIR)/SOURCES/$(RPMFILENAME)"
	mkdir -p $(RPMDIR)/BUILD/ $(RPMDIR)/SRPMS/ $(RPMDIR)/RPMS/ $(RPMDIR)/SOURCES
	make ${AM_MAKEFLAGS} distdir='$(RPMDIR)/SOURCES/$(RPMFILENAME)' distdir
	( cd "$(RPMDIR)/SOURCES/"; tar -z -c -f $(RPMFILENAME).tar.gz $(RPMFILENAME); rm -rf $(RPMFILENAME); )
	$(RPMBUILD) -bs $(PACKAGE_NAME).spec --define "_topdir `pwd`/$(RPMDIR)";

.PHONY: rpm
rpm: srpm
	$(RPMBUILD) --define "_topdir `pwd`/$(RPMDIR)" --rebuild $(RPMDIR)/SRPMS/$(RPMFILENAME)-$(RELEASE).src.rpm;
else
endif

rpm-clean:
	rm -rf $(RPMDIR)

if MAKE_DEB
.PHONY: deb
deb:
	make distdir && cd unirec-@VERSION@ && debuild -i -us -uc -b
else
endif

deb-clean:
	rm -rf unirec_*.build unirec_*.changes unirec_*.deb unirec-dev_*.deb unirec_*.orig.tar.gz unirec-*.tar.gz unirec-@VERSION@

clean-local: rpm-clean deb-clean
	find \( -name ur_values.c -o -name ur_values.h -o -name ur_values.py \) -exec rm -f {} + || true
	find \( -name fields.c -o -name fields.h \) -exec rm -f {} + || true

.PHONY: coverage
coverage: check
	@lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory out 2>/dev/null || echo "Skipped coverage analysis"

