.PHONY: rpm

EXTRA_DIST=MANIFEST.in README README.md test.sh nemea-pytrap.spec setup.py \
	   src/pytrapmodule.c \
	   src/unirecmodule.c \
	   src/unirecipaddr.c \
	   src/unirecipaddr.h \
	   src/unirecmacaddr.c \
	   src/unirecmacaddr.h \
	   src/unirectemplate.h \
	   src/pytrapexceptions.h \
	   src/iplist.c \
	   src/iplist.h \
	   src/fields.c src/fields.h

rpm:
	mkdir -p RPMBUILD/SOURCES
	python3 setup.py sdist
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
	python3 setup.py test || echo "Skipped python3 tests"
	@lcov --capture --directory . --output-file coverage.info 2>/dev/null && \
	genhtml coverage.info --output-directory out 2>/dev/null || echo "Skipped coverage analysis"

.PHONY: doc
doc:
	python3 setup.py build_sphinx

