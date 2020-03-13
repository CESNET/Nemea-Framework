SUBDIRS=python libtrap


datafiles=libtrap/module_info_example.c \
	  libtrap/Makefile.am \
	  c/module/README.md \
	  c/module/example_module.c \
	  c/module/configure.ac \
	  c/module/m4/ac_define_dir.m4 \
	  c/module/m4/ax_prog_doxygen.m4 \
	  c/module/m4/ax_pthread.m4 \
	  c/module/m4/ax_nemeacommon_check.m4 \
	  c/module/m4/ax_libtrap_check.m4 \
	  c/module/m4/ax_unirec_check.m4 \
	  c/module/aminclude.am \
	  c/module/Makefile.am \
	  c/detector/configure.ac \
	  c/detector/m4 \
	  c/detector/m4/ax_pthread.m4 \
	  c/detector/m4/ax_libtrap_check.m4 \
	  c/detector/m4/ax_unirec_check.m4 \
	  c/detector/m4/ac_define_dir.m4 \
	  c/detector/m4/ax_nemeacommon_check.m4 \
	  c/detector/m4/ax_prog_doxygen.m4 \
	  c/detector/README.md \
	  c/detector/Makefile.am \
	  c/detector/aminclude.am \
	  c/detector/detection_example.c \
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
