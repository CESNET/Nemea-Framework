pkgunirecdir=${pkgdatadir}/unirec
pkgunirec_DATA=unirec.py fields.py ur_time.py ur_ipaddr.py __init__.py

EXTRA_DIST=${pkgunirec_DATA}

BUILT_SOURCES=fields.py

fields.py:
	test -e "@UNIREC_PATH@/fields.py" || make -C "@UNIREC_PATH@"; \
	cp "@UNIREC_PATH@/fields.py" .

clean-local:
	rm -f fields.py

