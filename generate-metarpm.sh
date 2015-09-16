#!/bin/bash
#
# Copyright (C) 2013-2015 CESNET
#
# LICENSE TERMS
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name of the Company nor the names of its contributors
#    may be used to endorse or promote products derived from this
#    software without specific prior written permission.
#
# ALTERNATIVELY, provided that this notice is retained in full, this
# product may be distributed under the terms of the GNU General Public
# License (GPL) version 2 or later, in which case the provisions
# of the GPL apply INSTEAD OF those given above.
#
# This software is provided ``as is'', and any express or implied
# warranties, INCluding, but not limited to, the implied warranties of
# merchantability and fitness for a particular purpose are disclaimed.
# In no event shall the company or contributors be liable for any
# direct, indirect, INCidental, special, exemplary, or consequential
# damages (INCluding, but not limited to, procurement of substitute
# goods or services; loss of use, data, or profits; or business
# interruption) however caused and on any theory of liability, whether
# in contract, strict liability, or tort (INCluding negligence or
# otherwise) arising in any way out of the use of this software, even
# if advised of the possibility of such damage.
#


for i in unirec libtrap common; do
	( cd $i; ./configure; make rpm; )
done

missing=""
if [ -z "$(rpm -qa libtrap)" ]; then
	echo "yum install -y ./libtrap/RPMBUILD/RPMS/x86_64/libtrap-0.6.0-4.x86_64.rpm"
        missing="libtrap $missing"
fi
if [ -z "$(rpm -qa libtrap-devel)" ]; then
	echo "yum install -y ./libtrap/RPMBUILD/RPMS/x86_64/libtrap-devel-0.6.0-4.x86_64.rpm"
        missing="libtrap-devel $missing"
fi
if [ -z "$(rpm -qa nemea-common)" ]; then
	echo "yum install -y ./common/RPMBUILD/RPMS/x86_64/nemea-common-1.3.2-1.x86_64.rpm"
        missing="common $missing"
fi
if [ -z "$(rpm -qa nemea-common-devel)" ]; then
	echo "yum install -y ./common/RPMBUILD/RPMS/x86_64/nemea-common-devel-1.3.2-1.x86_64.rpm"
        missing="common-devel $missing"
fi
if [ -n "$missing" ]; then
	exit 1;
fi
if [ -z "$(rpm -qa unirec)" ]; then
	echo "yum install -y ./unirec/RPMBUILD/RPMS/x86_64/unirec-2.0.2-1.x86_64.rpm"
        missing="unirec $missing"
fi
if [ -n "$missing" ]; then
	exit 1;
fi

for i in python; do
	( cd $i; ./configure; make rpm; )
done

mkdir -p "`pwd`/RPMBUILD"
rpmbuild  -ba nemea.spec --define "_topdir `pwd`/RPMBUILD"

mkdir -p rpms
rm -f rpms/*
cp {unirec,libtrap,common,python,.}/RPMBUILD/{RPMS/{x86_64,noarch}/*,SRPMS/*} rpms 2> /dev/null
