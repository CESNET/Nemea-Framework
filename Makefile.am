SUBDIRS=
PYTHON_SUBDIRS=

if ENABLE_LIBTRAP
SUBDIRS+=libtrap
endif
if ENABLE_UNIREC
SUBDIRS+=unirec
endif
if ENABLE_NEMEACOMMON
SUBDIRS+=common
endif
if ENABLE_PYTRAP
SUBDIRS+=pytrap
PYTHON_SUBDIRS+=pytrap
endif
if ENABLE_PYCOMMON
SUBDIRS+=pycommon
PYTHON_SUBDIRS+=pycommon
endif
if ENABLE_EXAMPLES
SUBDIRS+=examples
endif

EXTRA_DIST=README.md

bin_SCRIPTS=create_module.sh

ACLOCAL_AMFLAGS=-I m4

install-exec-hook:
	@echo -e "\n\n"
	@echo "For installation of Python components run the following commands:"
	@for i in ${PYTHON_SUBDIRS}; do \
		echo -e "(pip install ./$${i})\n"; \
	done
	@echo -e "\n\n"


if MAKE_RPMS

RPMDIR = RPMBUILD
RPMFILENAME=$(PACKAGE_NAME)-$(VERSION)
.PHONY: rpm
rpm:
	rm -rf "$(RPMDIR)/SOURCES/$(RPMFILENAME)"
	mkdir -p $(RPMDIR)/BUILD/ $(RPMDIR)/SRPMS/ $(RPMDIR)/RPMS/ $(RPMDIR)/SOURCES
	make ${AM_MAKEFLAGS} dist
	cp $(RPMFILENAME).tar.gz RPMBUILD/SOURCES/
	$(RPMBUILD) -ba nemea-framework.spec --define "_topdir `pwd`/$(RPMDIR)";

rpm-clean:
	rm -rf $(RPMDIR)
else
endif

