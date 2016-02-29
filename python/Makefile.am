SUBDIRS=unirec

EXTRA_DIST=AUTHORS COPYING nemea-python.spec trap.py setup.py test.sh

TESTS = test.sh

if MAKE_RPMS
RPMFILENAME=nemea-python
.PHONY: rpm
rpm:
	mkdir -p RPMBUILD/{SOURCES/$(RPMFILENAME),BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
	make distdir='RPMBUILD/SOURCES/$(RPMFILENAME)' distdir
	( cd RPMBUILD/SOURCES; tar -czf $(RPMFILENAME).tar.gz $(RPMFILENAME); rm -rf $(RPMFILENAME); )
	rpmbuild -ba $(RPMFILENAME).spec --define "_topdir `pwd`/RPMBUILD"
else
endif
