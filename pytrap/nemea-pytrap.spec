# Created by pyp2rpm-3.1.2
%global pypi_name nemea-pytrap

%if "x%{?python3_pkgversion}" == "x"
%global python3_pkgversion 3
%endif

Name:           %{pypi_name}
Version:        0.17.0
Release:        2%{?dist}
Summary:        Python extension of the NEMEA project

License:        BSD
URL:            https://github.com/CESNET/Nemea-Framework
Source0: https://github.com/CESNET/Nemea-Framework/raw/dist-packages/pytrap/%{pypi_name}-%{version}.tar.gz

%description
The pytrap module is a native Python extension that allows for writing
NEMEA modules in Python.

%package -n     python%{python3_pkgversion}-%{pypi_name}
Summary:        Python extension of the NEMEA project
%{?python_provide:%python_provide python%{python3_pkgversion}-%{pypi_name}}
Requires: libtrap
BuildRequires:  python%{python3_pkgversion}-build
BuildRequires:  python%{python3_pkgversion}-pip
BuildRequires:  python%{python3_pkgversion}-wheel
BuildRequires:  python%{python3_pkgversion}-setuptools
BuildRequires:  python%{python3_pkgversion}-devel
BuildRequires:  gcc
BuildRequires:  libtrap
BuildRequires:  libtrap-devel
BuildRequires:  unirec

%description -n python%{python3_pkgversion}-%{pypi_name}
The pytrap module is a native Python extension that allows for writing
NEMEA modules in Python.


%prep
%setup
# Remove bundled egg-info
rm -rf %{pypi_name}.egg-info

%build
%py3_build

%install
# Must do the subpackages' install first because the scripts in /usr/bin are
# Install into the build root using pip (PEP 517 compatible)
%{__python3} -m pip install . \
    --root %{buildroot} \
    --no-deps --disable-pip-version-check --no-cache-dir --verbose

%check
# Install test dependencies and run tests
%{__python3} -m pip install .[test] --no-deps --disable-pip-version-check --no-cache-dir
TRAP_SOCKET_DIR=/tmp PAGER="" %{__python3} -m pytest -v
%files -n python%{python3_pkgversion}-%{pypi_name}
%doc README
%{python3_sitearch}/*

%changelog
* Fri Jul 26 2024 Tomas Cejka <cejkat@cesnet.cz> - 0.17.0
- Add UnirecIPList() for lookup IPs in the list of IP prefixes
- Add UnirecIPAddr.from_ipaddress() to load from python ipaddress
- Add UnirecIPAddrRange.from_ipaddress() to load from python ipaddress
- Add UnirecIPAddr.to_ipaddress() for convertion to python ipaddress
- Add UnirecIPAddrRange.to_ipaddress() for convertion to python ipaddress
* Thu Jul 21 2016 root - 0.9.6-1
- Initial package.

