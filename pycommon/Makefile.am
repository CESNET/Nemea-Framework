EXTRA_DIST=report2idea.py ip_prefix_search.py nemea-pycommon.spec setup.py

if MAKE_RPMS
RPMFILENAME=nemea-pycommon
.PHONY: rpm
rpm:
	mkdir -p RPMBUILD/{SOURCES/$(RPMFILENAME),BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
	make distdir='RPMBUILD/SOURCES/$(RPMFILENAME)' distdir
	( cd RPMBUILD/SOURCES; tar -czf $(RPMFILENAME).tar.gz $(RPMFILENAME); rm -rf $(RPMFILENAME); )
	rpmbuild -ba $(RPMFILENAME).spec --define "_topdir `pwd`/RPMBUILD"


else
endif
