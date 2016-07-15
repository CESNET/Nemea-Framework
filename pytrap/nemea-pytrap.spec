%global srcname nemea-pytrap
%global sum Python extension for NEMEA modules.
%{!?python_sitearch: %global python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}

Name:           python-%{srcname}
Version:        %(grep version setup.py | sed "s/^.*'\([^']*\)'.*/\1/")
Release:        1%{?dist}
Summary:        %{sum}
Group:		Liberouter
Vendor:		CESNET, z.s.p.o.
Packager: %(git config --get user.name) <%(git config --get user.email)>
License:        BSD
URL:            https://pypi.python.org/pypi/%{srcname}
Source0:        https://pypi.python.org/packages/source/e/%{srcname}/%{srcname}-%{version}.tar.gz
Requires:	libtrap
BuildRequires:  python2-devel libtrap-devel unirec

%description
The pytrap module is a native Python extension that allows for writing NEMEA modules in Python.

%package python-%{srcname}
Summary:        %{sum}
%{?python_provide:%python_provide python-%{srcname}}

%description python-%{srcname}
The pytrap module is a native Python extension that allows for writing NEMEA modules in Python.


%package -n python3-%{srcname}
Summary:        %{sum}
%{?python_provide:%python_provide python3-%{srcname}}

%description -n python3-%{srcname}
An python module which provides a convenient example.


%prep
%setup -n %{srcname}-%{version}

%build
%{__python} setup.py build
%{__python3} setup.py build

%install
# Must do the python2 install first because the scripts in /usr/bin are
# overwritten with every setup.py install, and in general we want the
# python3 version to be the default.
%{__python} setup.py install --skip-build --root %{buildroot}
%{__python3} setup.py install --skip-build --root %{buildroot}

# Note that there is no %%files section for the unversioned python module if we are building for several python runtimes
%files -n python-%{srcname}
%doc README
%{python_sitearch}/*

#%{_bindir}/sample-exec-2.7
#%license COPYING

%files -n python3-%{srcname}
%doc README
%{python3_sitearch}/*

#%license COPYING
#%{_bindir}/sample-exec
#%{_bindir}/sample-exec-3.4

