lib_LTLIBRARIES=libunirec++.la
libunirec___la_CPPFLAGS=-I../../ -I${top_srcdir}/include/
libunirec___la_CXXFLAGS=-fPIC
libunirec___la_LDFLAGS=-ltrap -L../.libs/ -lunirec
libunirec___la_SOURCES=unirec.cpp bidirectionalInterface.cpp inputInterface.cpp outputInterface.cpp ipAddress.cpp

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=unirec++.pc
EXTRA_DIST=unirec++.pc.in

