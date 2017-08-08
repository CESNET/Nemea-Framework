EXTRA_DIST=report2idea.py ip_prefix_search.py nemea-pycommon.spec setup.py README-RPM-release.md README

if MAKE_RPMS
RPMFILENAME=nemea-pycommon
.PHONY: rpm
rpm:
	mkdir -p RPMBUILD/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
	spectool -C "`pwd`/RPMBUILD/SOURCES" -g $(RPMFILENAME).spec
	cp $(RPMFILENAME).spec RPMBUILD/SPECS/
	rpmbuild -ba $(RPMFILENAME).spec --define "_topdir `pwd`/RPMBUILD/";
else
endif

clean-local:
	rm -rf build/ dist/ *.egg-info/ RPMBUILD/

