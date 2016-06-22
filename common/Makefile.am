SUBDIRS=. include
if ENABLE_TESTS
SUBDIRS+=tests
endif

RPMDIR=RPMBUILD
lib_LTLIBRARIES=libnemea-common.la
libnemea_common_la_SOURCES=configurator/configurator.cpp \
                           configurator/configurator-internal.h \
                           cuckoo_hash/cuckoo_hash.c \
			   cuckoo_hash/hashes.h \
			   cuckoo_hash_v2/cuckoo_hash_v2.c \
			   cuckoo_hash_v2/hashes_v2.c \
			   cuckoo_hash_v2/hashes_v2.h \
			   fast_hash_table/fast_hash_table.c \
			   fast_hash_table/hashes.h \
			   fast_hash_filter/fast_hash_filter.c \
			   fast_hash_filter/fhf_hashes.h \
			   b_plus_tree/b_plus_tree.c \
			   b_plus_tree/b_plus_tree_internal.h \
			   prefix_tree/prefix_tree.c \
                           super_fast_hash/super_fast_hash.c
libnemea_common_la_LDFLAGS = -version-info 2:0:1

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = nemea-common.pc

EXTRA_DIST= README \
	    prefix_tree/README \
	    fast_hash_table/README \
	    fast_hash_filter/README \
	    super_fast_hash/README \
	    cuckoo_hash_v2/README \
	    b_plus_tree/README \
	    cuckoo_hash/README

ACLOCAL_AMFLAGS=-I m4

include aminclude.am

if DX_COND_doc
clean-local: doxygen-clean
	rm -rf doc

doc: doxygen-doc

install-data-local:
	mkdir -p "$(DESTDIR)$(docdir)" && cp -R doc/* "$(DESTDIR)$(docdir)" || echo "Documentation was not generated yet."
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
