%define _build_id_links none
%define debug_package %{nil}

Summary:          Netware CWorthy Library for Linux
License:          LGPL
Name:             libcworthy
Version:          1.16
Release:          1%{?dist}

URL:              https://www.github.com/jeffmerkey/netware-cworthy-linux
Source0:          %{name}-%{version}.tar.gz

Requires:         ncurses
BuildRequires:    ncurses-devel

%description 
The %{name} package contains the Netware CWorthy libraries for Linux.

%prep
%setup -q -n %{name}-%{version}

%build
%{__make} 

%install
[ -n "%{buildroot}" -a "%{buildroot}" != "/" ] && %{__rm} -rf %{buildroot}
%{__mkdir_p} %{buildroot}%{_sbindir}
%{__mkdir_p} %{buildroot}%{_bindir}
%{__mkdir_p} %{buildroot}%{_includedir}
%{__mkdir_p} %{buildroot}%{_libdir}
%{__make} \
	DESTDIR=%{buildroot} NOCHK=1 LIBDIR=%{_libdir} \
	INCDIR=%{_includedir} BINDIR=%{_bindir} \
	install

%pre

%post
%{_sbindir}/ldconfig
%{_sbindir}/ldconfig

%preun

%postun
%{_sbindir}/ldconfig
%{_sbindir}/ldconfig

%files
%defattr(-,root,root)
%{_bindir}/ifcon
%{_includedir}/cworthy.h
%{_libdir}/libcworthy.a
%{_libdir}/libcworthy.so

%changelog
