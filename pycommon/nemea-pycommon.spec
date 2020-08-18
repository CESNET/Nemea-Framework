# Created by pyp2rpm-3.1.2
%global pypi_name nemea-pycommon
%global pypi_version 1.5.3

%if x%{?python3_pkgversion} == x
%global python3_pkgversion 3
%endif

Name:	%{pypi_name}
Version: %{pypi_version}
Release:	1%{?dist}
Summary: Common Python modules and methods of the NEMEA system.

BuildArch: noarch
Group: Development/Libraries
License: BSD
Vendor: Vaclav Bartos, CESNET <bartos@cesnet.cz>
URL:           https://github.com/CESNET/Nemea-Framework
Source0: https://github.com/CESNET/Nemea-Framework/raw/dist-packages/pycommon/%{pypi_name}-%{version}.tar.gz


%description
The module contains methods for creation and submission of incident reports in IDEA format.

%package -n     python%{python3_pkgversion}-%{pypi_name}
Summary:        Common Python modules and methods of the NEMEA system.
%{?python_provide:%python_provide python%{python3_pkgversion}-%{pypi_name}}

Requires:	python%{python3_pkgversion}-nemea-pytrap
Requires:	python%{python3_pkgversion}-idea-format
Requires:	python%{python3_pkgversion}-typedcols
Requires:	python%{python3_pkgversion}-ipranges
Requires:	python%{python3_pkgversion}-pynspect
Requires:	python%{python3_pkgversion}-jinja2
%if 0%{?el7}
Requires:	python36-ply
Requires:	python36-PyYAML
BuildRequires:	python36-ply
BuildRequires:	python36-PyYAML
%else
Requires:	python%{python3_pkgversion}-ply
Requires:	python%{python3_pkgversion}-yaml
BuildRequires:	python%{python3_pkgversion}-ply
BuildRequires:	python%{python3_pkgversion}-yaml
%endif
BuildRequires:	python%{python3_pkgversion}-setuptools
BuildRequires:	python%{python3_pkgversion}-devel
BuildRequires:	python%{python3_pkgversion}-nemea-pytrap
BuildRequires:	python%{python3_pkgversion}-pynspect


%description -n python%{python3_pkgversion}-%{pypi_name}
The module contains methods for creation and submission of incident reports in IDEA format.
This package is compatible with python3.

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
mkdir -p %{buildroot}/%{_sysconfdir}/nemea/email-templates/; cp reporter_config/default.html %{buildroot}/%{_sysconfdir}/nemea/email-templates/default.html


%check
%{__python3} setup.py test

%files -n python%{python3_pkgversion}-%{pypi_name}
%doc README
%{python3_sitelib}
%config(noreplace) %{_sysconfdir}/nemea/email-templates/default.html

%changelog

