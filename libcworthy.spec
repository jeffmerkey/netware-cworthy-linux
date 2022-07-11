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
%{__mkdir_p} %{buildroot}/usr/lib
%{__make} \
	DESTDIR=%{buildroot} NOCHK=1\
	install

%pre

%post
/sbin/ldconfig
/sbin/ldconfig

%preun

%postun
/sbin/ldconfig
/sbin/ldconfig

%files
%defattr(-,root,root)
%{_bindir}/ifcon
%{_includedir}/cworthy.h
/usr/lib/libcworthy.a
/usr/lib/libcworthy.so

%changelog
