Name:       libmm-session
Summary:    Multimedia Session Library
Version:    0.2.6
Release:    0
Group:      Multimedia/Libraries
License:    Apache-2.0
Source0:    libmm-session-%{version}.tar.gz
BuildRequires:  pkgconfig(audio-session-mgr)
BuildRequires:  pkgconfig(mm-common)


%description
Multimedia Session Library


%package devel
Summary:    Multimedia Session Library (Development)
Group:      Development/Multimedia
Requires:   %{name} = %{version}-%{release}

%description devel
%devel_desc

%prep
%setup -q


%build

%autogen
CFLAGS="$CFLAGS -Wp,-D_FORTIFY_SOURCE=0"
%configure
make %{?_smp_mflags} 

%install
%make_install


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest libmm-session.manifest
%license LICENSE
%defattr(-,root,root,-)
%{_libdir}/libmmfsession.so.*

%files devel
%defattr(-,root,root,-)
%{_includedir}/mmf/*.h
%{_libdir}/libmmfsession.so
%{_libdir}/pkgconfig/mm-session.pc
