SUBDIRS=
PYTHON_SUBDIRS=

if ENABLE_LIBTRAP
SUBDIRS+=libtrap
endif
if ENABLE_UNIREC
SUBDIRS+=unirec
endif
if ENABLE_NEMEACOMMON
SUBDIRS+=common
endif
if ENABLE_PYTHON
SUBDIRS+=python
PYTHON_SUBDIRS+=python
endif
if ENABLE_PYCOMMON
SUBDIRS+=pycommon
PYTHON_SUBDIRS+=pycommon
endif

EXTRA_DIST=README.md

bin_SCRIPTS=create_module.sh

ACLOCAL_AMFLAGS=-I m4

install-exec-hook:
	@for i in $(PYTHON_SUBDIRS); do \
		(cd $${i}; $(PYTHON) setup.py install --record=installed-files.txt;) \
	done

uninstall-hook:
	@for i in $(PYTHON_SUBDIRS); do \
		(cd $${i}; cat installed-files.txt | xargs rm -rf;) \
	done

