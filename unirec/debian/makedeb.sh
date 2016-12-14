UBDIRS = src include tools

if ENABLE_TESTS
SUBDIRS+=tests
endif


RPMDIR = RPMBUILD

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = unirec.pc

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = README README.ifcspec.md mdconvert.sh doc/index.txt doc/history.txdt doc/index-devel.txt doc/develop-interface.txt debian/source/local-options debian/source/format debian/patches debian/patches/series debian/tests debian/copyright debian/changelog debian/control debian/rules debian/compat debian/README.Debian debian/unirec.install debian/unirec-dev.install

include aminclude.am

if DX_COND_doc
.PHONY: doxygen-devel
doxygen-devel:
	rm -rf devel-docs/
	doxygen devel-doxyfile

doc: doxygen-doc doxygen-devel

install-data-local:
	mkdir -p "$(DESTDIR)$(docdir)" && cp -R doc/doxygen/* "$(DESTDIR)$(docdir)" || echo "Documentation was not generated yet."
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

install-exec-hook:
	mkdir -p $(DESTDIR)/$(DEFAULTSOCKETDIR) && chmod 1777 $(DESTDIR)/$(DEFAULTSOCKETDIR) || true

changelog:
	echo -e "This will insert history into ChangeLog file and open NEWS in vim.\nC-c to interrupt."; \
	git log --date=short --format="%cd (%an): %s" $(shell git tag | head -1)..HEAD . > chlog.tmp; \
	echo "" >> chlog.tmp; \
	cat ChangeLog >> chlog.tmp; \
	vim -o NEWS chlog.tmp; \
	mv chlog.tmp ChangeLog

clean-local: rpm-clean deb-clean


buffer:
	gcc --coverage -g -O0 -DUNIT_TESTING -c tests/test_trap_buffer.c
	gcc --coverage -g -O0 -DUNIT_TESTING -c src/trap_buffer.c
	gcc -g -O0 -Wl,--wrap=_test_malloc -lcmocka --coverage -DUNIT_TESTING -o test_buffer test_trap_buffer.o trap_buffer.o
