unirecincludedir=$(includedir)/unirec
unirecinclude_HEADERS=unirec.h \
		     inline.h \
		     ipaddr.h \
		     macaddr.h \
		     links.h  \
		     ur_time.h \
		     unirec2csv.h \
		     ip_prefix_search.h \
		     ur_values.h

BUILT_SOURCES=ur_values.h ur_values.c ur_values.py

ur_values.h: ${top_srcdir}/ur_values.sh
	${top_srcdir}/ur_values.sh -i ${top_srcdir}

ur_values.c: ur_values.h

ur_values.py: ur_values.h


