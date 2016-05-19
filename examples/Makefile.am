SUBDIRS=python libtrap


datafiles=libtrap/module_info_example.c \
	  libtrap/Makefile.am \
	  module/README.md \
	  module/example_module.c \
	  module/configure.ac \
	  module/m4/ac_define_dir.m4 \
	  module/m4/ax_prog_doxygen.m4 \
	  module/m4/ax_pthread.m4 \
	  module/m4/ax_nemeacommon_check.m4 \
	  module/m4/ax_libtrap_check.m4 \
	  module/m4/ax_unirec_check.m4 \
	  module/aminclude.am \
	  module/Makefile.am \
	  python/README.md \
	  python/python_example.py

EXTRA_DIST=${datafiles}

install-data-hook:
	for i in ${datafiles}; do \
	mkdir -p ${DESTDIR}/${datadir}/`dirname $$i`; \
	cp ./${srcdir}/$$i ${DESTDIR}/${datadir}/$$i; done

uninstall-local:
	for i in ${datafiles}; do \
	rm -f ${DESTDIR}/${datadir}/$$i; done

ACLOCAL_AMFLAGS=-I m4
