Name:       libmm-session
Summary:    Mm-session development pkg for samsung
Version: 0.2.0
Release:    3
Group:      TO_BE/FILLED_IN
License:    TO BE FILLED IN
Source0:    libmm-session-%{version}.tar.bz2
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(audio-session-mgr)
BuildRequires:  pkgconfig(dlog)
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
%defattr(-,root,root,-)
/usr/lib/libmmfsession.so.*

%files devel
%defattr(-,root,root,-)
/usr/include/mmf/*.h
/usr/lib/libmmfsession.so
/usr/lib/pkgconfig/mm-session.pc
