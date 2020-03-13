SUBDIRS = src include tools

if ENABLE_TESTS
SUBDIRS+=tests
endif


RPMDIR = RPMBUILD

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtrap.pc

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = README README.ifcspec.md mdconvert.sh libtrap-varrun.conf doc/index.txt doc/history.txt doc/index-devel.txt doc/develop-interface.txt debian/source/local-options debian/source/format debian/patches debian/patches/series debian/watch debian/copyright debian/changelog debian/control debian/rules debian/compat debian/README.Debian debian/libtrap.install debian/libtrap-dev.install

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
	make distdir && cd libtrap-@VERSION@ && debuild -i -us -uc -b
else
endif

deb-clean:
	rm -rf libtrap_*.build libtrap_*.changes libtrap_*.deb libtrap-dev_*.deb libtrap_*.orig.tar.gz libtrap-*.tar.gz libtrap-@VERSION@

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


tmpfilesddir=$(sysconfdir)/tmpfiles.d/
tmpfilesd_DATA=libtrap-varrun.conf

