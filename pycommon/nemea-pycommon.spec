# Created by pyp2rpm-3.1.2
%global pypi_name nemea-pycommon

%if 0%{?el6}
%global python3_pkgversion 33
%endif

%if x%{?python3_pkgversion} == x
%global python3_pkgversion 3
%endif

Name:	python-%{pypi_name}
Version: 1.0.8
Release:	1%{?dist}
Summary: Common Python modules and methods of the NEMEA system.

BuildArch: noarch
Group: Development/Libraries
License: BSD
Vendor: Vaclav Bartos, CESNET <bartos@cesnet.cz>
URL:           https://github.com/CESNET/Nemea-Framework
Source0:       https://files.pythonhosted.org/packages/source/n/%{pypi_name}/%{pypi_name}-%{version}.tar.gz

BuildRequires:  python-setuptools
BuildRequires:  python2-devel

BuildRequires:  python%{python3_pkgversion}-setuptools
BuildRequires:  python%{python3_pkgversion}-devel

%description
The module contains methods for creation and submission of incident reports in IDEA format.

%package -n     python2-%{pypi_name}
Summary:        Common Python modules and methods of the NEMEA system.
%{?python_provide:%python_provide python2-%{pypi_name}}

%description -n python2-%{pypi_name}
The module contains methods for creation and submission of incident reports in IDEA format.
This package is compatible with python2.

%package -n     python%{python3_pkgversion}-%{pypi_name}
Summary:        Common Python modules and methods of the NEMEA system.
%{?python_provide:%python_provide python%{python3_pkgversion}-%{pypi_name}}

%description -n python%{python3_pkgversion}-%{pypi_name}
The module contains methods for creation and submission of incident reports in IDEA format.
This package is compatible with python3.

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
%{python_sitelib}

%files -n python%{python3_pkgversion}-%{pypi_name}
%doc README
%{python3_sitelib}

%changelog

