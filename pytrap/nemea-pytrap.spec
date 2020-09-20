# Created by pyp2rpm-3.1.2
%global pypi_name nemea-pytrap

%if x%{?python3_pkgversion} == x
%global python3_pkgversion 3
%endif

Name:           %{pypi_name}
Version:        0.12.2
Release:        1%{?dist}
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
BuildRequires:  python%{python3_pkgversion}-setuptools
BuildRequires:  python%{python3_pkgversion}-devel
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
# overwritten with every setup.py install.
%{__python3} setup.py install --skip-build --single-version-externally-managed --root %{buildroot}


%check
TRAP_SOCKET_DIR=/tmp PAGER="" %{__python3} setup.py test

%files -n python%{python3_pkgversion}-%{pypi_name}
%doc README
%{python3_sitearch}/*

%changelog
* Thu Jul 21 2016 root - 0.9.6-1
- Initial package.
