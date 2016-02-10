SUBDIRS=unirec

pkgdata_DATA=trap.py setup.py

EXTRA_DIST=AUTHORS COPYING ${pkgdata_DATA}

if MAKE_RPMS

RPMDIR = RPMBUILD
RPMFILENAME=$(PACKAGE_NAME)-$(VERSION)
.PHONY: rpm
rpm:
	python setup.py bdist_rpm

rpm-clean:
	rm -rf $(RPMDIR)
else
endif
