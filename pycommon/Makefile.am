EXTRA_DIST=report2idea.py ip_prefix_search.py nemea-pycommon.spec setup.py README-RPM-release.md README reporter_config

if MAKE_RPMS
RPMFILENAME=nemea-pycommon
.PHONY: rpm
rpm:
	mkdir -p RPMBUILD/SOURCES
	python3 setup.py sdist
	cp dist/*.tar.gz RPMBUILD/SOURCES
	rpmbuild  -ba nemea-pycommon.spec --define "_topdir `pwd`/RPMBUILD"
	mv RPMBUILD/RPMS/*/*.rpm RPMBUILD/SRPMS/*.rpm dist/
else
endif

clean-local:
	rm -rf build/ dist/ *.egg-info/ RPMBUILD/

