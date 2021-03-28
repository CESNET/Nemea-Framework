Summary: Common library for NEMEA project
Name: nemea-common
Version: 1.6.2
Release: 1
URL: http://www.liberouter.org/
Source0: https://github.com/CESNET/Nemea-Framework/raw/dist-packages/common/%{name}-%{version}.tar.gz
Group: Liberouter
License: BSD
Vendor: CESNET, z.s.p.o.
Packager: Travis CI User <travis@example.org>
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}

BuildRequires: gcc gcc-c++ make doxygen pkgconfig libxml2-devel
Provides: nemea-common
Requires: libxml2

%description

%package devel
Summary: Nemea-common development package containing hash table interface etc.
Group: Liberouter
Requires: nemea-common = %{version}-%{release} libxml2-devel
Provides: nemea-common-devel

%description devel
This package contains header files for nemea-common library.

%prep
%setup

%build
./configure --prefix=%{_prefix} --libdir=%{_libdir} --disable-doxygen-pdf --disable-doxygen-ps;
make
make doc

%install
make DESTDIR=$RPM_BUILD_ROOT install

%post
ldconfig

%files
%{_libdir}/libnemea-common.so.*

%files devel
%{_libdir}/pkgconfig/nemea-common.pc
%{_libdir}/libnemea-common.so
%{_libdir}/libnemea-common.a
%{_libdir}/libnemea-common.la
%{_prefix}/include/nemea-common/*
%{_prefix}/share/doc/nemea-common/*

