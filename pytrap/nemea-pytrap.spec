# Created by pyp2rpm-3.1.2
%global pypi_name nemea-pytrap

%if 0%{?el6}
%global python3_pkgversion 33
%global py3_build CFLAGS="-O2 -g -pipe -Wall -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4 -mtune=generic" python3 setup.py build
%endif

%if x%{?python3_pkgversion} == x
%global python3_pkgversion 3
%endif

Name:           %{pypi_name}
Version:        0.9.9
Release:        1%{?dist}
Summary:        Python extension of the NEMEA project

License:        BSD
URL:            https://github.com/CESNET/Nemea-Framework
Source0: https://github.com/CESNET/Nemea-Framework/raw/dist-packages/pytrap/%{name}-%{version}.tar.gz

BuildRequires:  python-setuptools
BuildRequires:  python2-devel

BuildRequires:  python%{python3_pkgversion}-setuptools
BuildRequires:  python%{python3_pkgversion}-devel

%description
The pytrap module is a native Python extension that allows for writing
NEMEA
modules in Python.

%package -n     python2-%{pypi_name}
Summary:        Python extension of the NEMEA project
%{?python_provide:%python_provide python2-%{pypi_name}}

%description -n python2-%{pypi_name}
The pytrap module is a native Python extension that allows for writing
NEMEA
modules in Python.

%package -n     python%{python3_pkgversion}-%{pypi_name}
Summary:        Python extension of the NEMEA project
%{?python_provide:%python_provide python%{python3_pkgversion}-%{pypi_name}}

%description -n python%{python3_pkgversion}-%{pypi_name}
The pytrap module is a native Python extension that allows for writing
NEMEA
modules in Python.


%prep
%setup -n %{pypi_name}-%{version}
# Remove bundled egg-info
rm -rf %{pypi_name}.egg-info

%build
%py2_build
%py3_build

%install
# Must do the subpackages' install first because the scripts in /usr/bin are
# overwritten with every setup.py install.
%{__python3} setup.py install --skip-build --single-version-externally-managed --root %{buildroot}
%{__python2} setup.py install --skip-build --single-version-externally-managed --root %{buildroot}


%check
%{__python2} setup.py test
%{__python3} setup.py test

%files -n python2-%{pypi_name}
%doc README
%{python_sitearch}/*

%files -n python%{python3_pkgversion}-%{pypi_name}
%doc README
%{python3_sitearch}/*

%changelog
* Thu Jul 21 2016 root - 0.9.6-1
- Initial package.
