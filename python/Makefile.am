SUBDIRS=unirec

pkgdata_DATA=trap.py setup.py

EXTRA_DIST=AUTHORS COPYING ${pkgdata_DATA}

if MAKE_RPMS
.PHONY: rpm
rpm:
	python setup.py bdist_rpm

else
endif
