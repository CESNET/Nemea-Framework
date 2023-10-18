lib_LTLIBRARIES=libunirec++.la
libunirec___la_CXXFLAGS=-fPIC
libunirec___la_LDFLAGS=-ltrap -lunirec
libunirec___la_SOURCES=unirec.cpp bidirectionalInterface.cpp inputInterface.cpp outputInterface.cpp

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=unirec++.pc
EXTRA_DIST=unirec++.pc.in

unirecincludedir=$(includedir)/unirec++
unirecinclude_HEADERS=unirec++/bidirectionalInterface.hpp \
	unirec++/inputInterface.hpp \
	unirec++/ipAddress.hpp \
	unirec++/macAddress.hpp \
	unirec++/outputInterface.hpp \
	unirec++/trapModuleInfo.hpp \
	unirec++/unirecArray.hpp \
	unirec++/unirecException.hpp \
	unirec++/unirec.hpp \
	unirec++/unirecRecord.hpp \
	unirec++/unirecRecordView.hpp \
	unirec++/unirecTypes.hpp \
	unirec++/unirecTypeTraits.hpp \
	unirec++/urTime.hpp

