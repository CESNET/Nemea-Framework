# Created by pyp2rpm-3.1.2
%global pypi_name nemea-pycommon
%global pypi_version 1.4.3


%if 0%{?el6}
%global python3_pkgversion 33
%global py3_build CFLAGS="-O3 -g -pipe -Wall -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -fexceptions --param=ssp-buffer-size=4 -mtune=generic" python3 setup.py build
%endif

%if x%{?python3_pkgversion} == x
%global python3_pkgversion 3
%endif

Name:	%{pypi_name}
Version: %{pypi_version}
Release:	2%{?dist}
Summary: Common Python modules and methods of the NEMEA system.

BuildArch: noarch
Group: Development/Libraries
License: BSD
Vendor: Vaclav Bartos, CESNET <bartos@cesnet.cz>
URL:           https://github.com/CESNET/Nemea-Framework
Source0: https://github.com/CESNET/Nemea-Framework/raw/dist-packages/pycommon/%{pypi_name}-%{version}.tar.gz


%description
The module contains methods for creation and submission of incident reports in IDEA format.

%package -n     python2-%{pypi_name}
Summary:        Common Python modules and methods of the NEMEA system.
%{?python_provide:%python_provide python2-%{pypi_name}}

Requires:	python-nemea-pytrap
Requires:	python-ply
Requires:	python-yaml
Requires:	python-idea-format
Requires:	python-typedcols
Requires:	python-ipranges
Requires:	python-pynspect
Requires:	python-jinja2
BuildRequires:	python-setuptools
BuildRequires:	python2-devel
BuildRequires:	python-nemea-pytrap
BuildRequires:	python-ply
BuildRequires:	python-yaml
BuildRequires:	python-pynspect

%description -n python2-%{pypi_name}
The module contains methods for creation and submission of incident reports in IDEA format.
This package is compatible with python2.

%package -n     python%{python3_pkgversion}-%{pypi_name}
Summary:        Common Python modules and methods of the NEMEA system.
%{?python_provide:%python_provide python%{python3_pkgversion}-%{pypi_name}}

Requires:	python%{python3_pkgversion}-nemea-pytrap
Requires:	python%{python3_pkgversion}-ply
Requires:	python%{python3_pkgversion}-yaml
Requires:	python%{python3_pkgversion}-idea-format
Requires:	python%{python3_pkgversion}-typedcols
Requires:	python%{python3_pkgversion}-ipranges
Requires:	python%{python3_pkgversion}-pynspect
Requires:	python%{python3_pkgversion}-jinja2
BuildRequires:	python%{python3_pkgversion}-setuptools
BuildRequires:	python%{python3_pkgversion}-devel
BuildRequires:	python%{python3_pkgversion}-nemea-pytrap
BuildRequires:	python%{python3_pkgversion}-ply
BuildRequires:	python%{python3_pkgversion}-yaml
BuildRequires:	python%{python3_pkgversion}-pynspect


%description -n python%{python3_pkgversion}-%{pypi_name}
The module contains methods for creation and submission of incident reports in IDEA format.
This package is compatible with python3.

%prep
%setup
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
mkdir -p %{buildroot}/%{_sysconfdir}/nemea/email-templates/; cp reporter_config/default.html %{buildroot}/%{_sysconfdir}/nemea/email-templates/default.html


%check
%{__python2} setup.py test
%{__python3} setup.py test

%files -n python2-%{pypi_name}
%doc README
%{python_sitelib}
%config(noreplace) %{_sysconfdir}/nemea/email-templates/default.html

%files -n python%{python3_pkgversion}-%{pypi_name}
%doc README
%{python3_sitelib}

%changelog

