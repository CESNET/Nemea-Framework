.PHONY: rpm

all-local:
	(cd $(srcdir); $(PYTHON) setup.py build \
	--build-base $(shell readlink -f $(builddir))/build \
	--verbose)

EXTRA_DIST=MANIFEST.in fields.c fields.h README README.md test.sh nemea-pytrap.spec setup.py pytrapmodule.c unirecmodule.c

rpm:
	mkdir -p RPMBUILD/SOURCES
	python setup.py sdist
	cp dist/*.tar.gz RPMBUILD/SOURCES
	rpmbuild  -ba nemea-pytrap.spec --define "_topdir `pwd`/RPMBUILD"
	mv RPMBUILD/RPMS/*/*.rpm RPMBUILD/SRPMS/*.rpm dist/

clean-local:
	rm -rf RPMBUILD dist build *.trs test.sh.log test-suite.log *.so *.egg-info coverage.info out/

TESTS = test.sh

.PHONY: coverage
coverage:
	CFLAGS=-coverage python setup.py build_ext --inplace
	CFLAGS=-coverage python3 setup.py build_ext --inplace
	python setup.py test
	python3 setup.py test || echo "Skipped python3 tests"
	@lcov --capture --directory . --output-file coverage.info 2>/dev/null && \
	genhtml coverage.info --output-directory out 2>/dev/null || echo "Skipped coverage analysis"

