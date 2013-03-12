Name:       libmm-session
Summary:    mm-session development pkg for samsung
Version:    0.2.7
Release:    0
Group:      System/Libraries
License:    Apache License, Version 2.0
URL:        http://source.tizen.org
Source0:    libmm-session-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(audio-session-mgr)
BuildRequires:  pkgconfig(mm-common)


%description
mm-session development package for samsung



%package devel
Summary:    mm-session development pkg for samsung (devel)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
mm-session development package for samsung (devel)


%prep
%setup -q


%build

./autogen.sh
CFLAGS="$CFLAGS -Wp,-D_FORTIFY_SOURCE=0"
./configure --prefix=/usr 
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest libmm-session.manifest
%defattr(-,root,root,-)
/usr/lib/libmmfsession.so.*

%files devel
%defattr(-,root,root,-)
/usr/include/mmf/*.h
/usr/lib/libmmfsession.so
/usr/lib/pkgconfig/mm-session.pc
