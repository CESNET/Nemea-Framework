Name:	nemea-python
Version: %(sed -n "s/^\s*version=['\"]\(.*\)['\"],\s*$/\1/p" setup.py)
Release:	1
Summary: Python wrapper for NEMEA framework

BuildArch: noarch
Group: Development/Libraries
License: BSD
Vendor: Vaclav Bartos, CESNET <bartos@cesnet.cz>
Source: nemea-python.tar.gz
BuildRoot: RPMBUILD/BUILDROOT/

Provides: nemea-python

%description
This distribution contains two related modules/packages:
  trap: Python wrapper for libtrap - an implementation of Traffic Analysis Platform (TRAP)
  unirec: Python version of a data structure used in TRAP

%prep
%setup -n nemea-python
python setup.py build
cp setup.py build/lib

%install
find \( -name '*.pyc' -o -name '*.pyo' \) -exec rm -f {} \;
mkdir -p %{buildroot}%{_datadir}/nemea-python
cp -r build/lib/* %{buildroot}%{_datadir}/nemea-python

%post
cd %{_datadir}/nemea-python
python setup.py install --record=%{_datadir}/nemea-python/py-installed.txt 2>/dev/null || true
python3 setup.py install --record=%{_datadir}/nemea-python/py3-installed.txt 2>/dev/null || true

%postun
cat %{_datadir}/nemea-python/py-installed.txt |xargs rm -f
cat %{_datadir}/nemea-python/py3-installed.txt |xargs rm -f


%files
%defattr(644,root,root,-)
%{_datadir}/nemea-python

%changelog

