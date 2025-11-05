.PHONY: rpm

EXTRA_DIST=MANIFEST.in README README.md test.sh nemea-pytrap.spec pyproject.toml \
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
	python3 -m build --sdist
	cp dist/*.tar.gz RPMBUILD/SOURCES
	rpmbuild  -ba nemea-pytrap.spec --define "_topdir `pwd`/RPMBUILD"
	mv RPMBUILD/RPMS/*/*.rpm RPMBUILD/SRPMS/*.rpm dist/

clean-local:
	rm -rf RPMBUILD dist build *.trs test.sh.log test-suite.log *.so *.egg-info coverage.info out/

TESTS = test.sh

.PHONY: coverage
coverage:
	CFLAGS=-coverage python3 -m pip install .[test] --disable-pip-version-check --no-cache-dir
	CFLAGS=-coverage python3 -m pytest --cov=pytrap || echo "Skipped python3 tests"
	@lcov --capture --directory . --output-file coverage.info 2>/dev/null && \
	genhtml coverage.info --output-directory out 2>/dev/null || echo "Skipped coverage analysis"

.PHONY: test
test:
	python3 -m pip  install .[test] --no-cache-dir
	python3 -m pytest

.PHONY: doc
doc:
	python3 -m pip install .[docs] --no-cache-dir
	cd docs
	make html
