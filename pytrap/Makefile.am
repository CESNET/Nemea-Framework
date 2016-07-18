.PHONY: rpm
rpm:
	mkdir -p RPMBUILD/SOURCES
	python setup.py sdist
	cp dist/*.tar.gz RPMBUILD/SOURCES
	rpmbuild  -ba nemea-pytrap.spec --define "_topdir `pwd`/RPMBUILD"
	mv RPMBUILD/RPMS/*/*.rpm RPMBUILD/SRPMS/*.rpm dist/

.PHONY: clean
clean:
	rm -rf RPMBUILD dist build

TESTS = test.sh
