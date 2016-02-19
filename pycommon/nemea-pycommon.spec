Name:	nemea-pycommon
Version: %(sed -n "s/^\s*version=['\"]\(.*\)['\"],\s*$/\1/p" setup.py)
Release:	1
Summary: Common Python modules and methods of the NEMEA system.

BuildArch: noarch
Group: Development/Libraries
License: BSD
Vendor: Vaclav Bartos, CESNET <bartos@cesnet.cz>
Source: nemea-pycommon.tar.gz
BuildRoot: RPMBUILD/BUILDROOT/

Provides: nemea-pycommon

%description
The module contains methods for creation and submission of incident reports in IDEA format.

%prep
%setup -n nemea-pycommon
python setup.py build
cp setup.py build/lib

%install
find \( -name '*.pyc' -o -name '*.pyo' \) -exec rm -f {} \;
mkdir -p %{buildroot}%{_datadir}/nemea-pycommon
cp -r build/lib/* %{buildroot}%{_datadir}/nemea-pycommon

%post
cd %{_datadir}/nemea-pycommon
python setup.py install --record=%{_datadir}/nemea-pycommon/py-installed.txt 2>/dev/null || true
python3 setup.py install --record=%{_datadir}/nemea-pycommon/py3-installed.txt 2>/dev/null || true

%postun
cat %{_datadir}/nemea-pycommon/py-installed.txt |xargs rm -f
cat %{_datadir}/nemea-pycommon/py3-installed.txt |xargs rm -f


%files
%defattr(644,root,root,-)
%{_datadir}/nemea-pycommon

%changelog

