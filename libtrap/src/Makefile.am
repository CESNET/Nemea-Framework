lib_LTLIBRARIES = libtrap.la
libtrap_la_LDFLAGS = -version-info 6:0:5
libtrap_la_SOURCES = trap.c trap_error.c ifc_dummy.c ifc_tcpip.c trap_internal.c ifc_tcpip_internal.h ifc_file.c ifc_file.h help_trapifcspec.c \
   third-party/libjansson/dump.c \
   third-party/libjansson/error.c \
   third-party/libjansson/hashtable.c \
   third-party/libjansson/hashtable.h \
   third-party/libjansson/hashtable_seed.c \
   third-party/libjansson/jansson_private.h \
   third-party/libjansson/load.c \
   third-party/libjansson/lookup3.h \
   third-party/libjansson/memory.c \
   third-party/libjansson/pack_unpack.c \
   third-party/libjansson/strbuffer.c \
   third-party/libjansson/strbuffer.h \
   third-party/libjansson/strconv.c \
   third-party/libjansson/utf.c \
   third-party/libjansson/utf.h \
   third-party/libjansson/value.c
EXTRA_libtrap_la_SOURCES = ifc_dummy.h trap_ifc.h ifc_tcpip.h trap_internal.h trap_error.h ifc_tls.c ifc_tls.h ifc_tls_internal.h ifc_socket_common.h

if HAVE_OPENSSL
libtrap_la_SOURCES += ifc_tls.c ifc_tls.h ifc_tls_internal.h
endif

BUILT_SOURCES=help_trapifcspec.c

help_trapifcspec.c: ${top_srcdir}/README.ifcspec.md
	${top_srcdir}/mdconvert.sh $< $@ trap_help_ifcspec

clean-local:
	rm -f help_trapifcspec.c *.gcda *.gcno
