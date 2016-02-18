SUBDIRS=unirec

EXTRA_DIST=AUTHORS COPYING trap.py setup.py

if MAKE_RPMS
RPMFILENAME=nemea-python
.PHONY: rpm
rpm:
	mkdir -p RPMBUILD/{SOURCES/$(RPMFILENAME),BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
	make distdir='RPMBUILD/SOURCES/$(RPMFILENAME)' distdir
	( cd RPMBUILD/SOURCES; tar -czf $(RPMFILENAME).tar.gz $(RPMFILENAME); rm -rf $(RPMFILENAME); )
	rpmbuild -ba nemea-python.spec --define "_topdir `pwd`/RPMBUILD"


else
endif
