%global srcname nemea-pytrap
%global sum The pytrap module is a native Python extension that allows for writing NEMEA modules in Python.

Name:           python-%{srcname}
Version:        %(grep version setup.py | sed "s/^.*'\([^']*\)'.*/\1/")
Release:        1%{?dist}
Summary:        %{sum}

License:        BSD
URL:            https://pypi.python.org/pypi/%{srcname}
Source0:        https://pypi.python.org/packages/source/e/%{srcname}/%{srcname}-%{version}.tar.gz
BuildRequires:  python2-devel python3-devel

%description
An python module which provides a convenient example.

%package python-%{srcname}
Summary:        %{sum}
%{?python_provide:%python_provide python-%{srcname}}

%description python-%{srcname}
An python module which provides a convenient example.


%package -n python3-%{srcname}
Summary:        %{sum}
%{?python_provide:%python_provide python3-%{srcname}}

%description -n python3-%{srcname}
An python module which provides a convenient example.


%prep
%autosetup -n %{srcname}-%{version}

%build
%py2_build
%py3_build

%install
# Must do the python2 install first because the scripts in /usr/bin are
# overwritten with every setup.py install, and in general we want the
# python3 version to be the default.
%py2_install
%py3_install

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

