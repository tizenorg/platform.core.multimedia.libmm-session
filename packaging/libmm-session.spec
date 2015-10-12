Name:       libmm-session
Summary:    Multimedia Session Library
Version:    0.2.8
Release:    0
Group:      Multimedia/Libraries
License:    Apache-2.0
Source0:    libmm-session-%{version}.tar.gz
Source1001:     libmm-session.manifest
BuildRequires:  pkgconfig(mm-common)

%description
Multimedia Session Library package.

%package devel
Summary:    Multimedia Session Library (Development)
Group:      Development/Multimedia
Requires:   %{name} = %{version}-%{release}

%description devel
Multimedia Session Library (Development)
%devel_desc

%prep
%setup -q
cp %{SOURCE1001} .

%build
CFLAGS="$CFLAGS -Wp,-D_FORTIFY_SOURCE=0"
%reconfigure
%__make %{?_smp_mflags}

%install
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%license LICENSE
%defattr(-,root,root,-)
%{_libdir}/libmmfsession.so.*

%files devel
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_includedir}/mmf/*.h
%{_libdir}/libmmfsession.so
%{_libdir}/pkgconfig/mm-session.pc
