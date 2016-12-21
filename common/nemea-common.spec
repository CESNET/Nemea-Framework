Summary: Common library for NEMEA project
Name: nemea-common
Version: 1.5.1
Release: 1
URL: http://www.liberouter.org/
Source: https://www.liberouter.org/repo/SOURCES/%{name}-%{version}-%{release}.tar.gz
Group: Liberouter
License: BSD
Vendor: CESNET, z.s.p.o.
Packager: Ladislav Macoun <ladislavmacoun@gmail.com>
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}

BuildRequires: gcc make doxygen pkgconfig libxml2-devel
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

