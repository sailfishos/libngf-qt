Name:       libngf-qt5
Summary:    Qt-based client library for Non-Graphic Feedback daemon
Version:    0.6.1
Release:    1
Group:      System/Libraries
License:    LGPLv2
URL:        https://git.sailfishos.org/mer-core/libngf-qt
Source0:    %{name}-%{version}.tar.bz2
Requires:   ngfd
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  doxygen

%description
This package contains the Qt-based client library for accessing
Non-graphic feedback services.


%package devel
Summary:    Development package for Qt-based client library for NGF daemon
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
%{summary}.

%package declarative
Summary:    Declarative plugin for NGF clients
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description declarative
%{summary}.

%package tests
Summary:    Test suite for libngf-qt5
Requires:   %{name} = %{version}-%{release}

%description tests
%{summary}.

%prep
%setup -q -n %{name}-%{version}


%build
%qmake5 "VERSION=$(sed 's/+.*//' <<<"%{version}")"

make %{?_smp_mflags}


%install
rm -rf %{buildroot}
%qmake_install

# org.nemomobile.ngf legacy import
mkdir -p %{buildroot}%{_libdir}/qt5/qml/org/nemomobile/ngf/
ln -sf %{_libdir}/qt5/qml/Nemo/Ngf/libngf-declarative.so %{buildroot}%{_libdir}/qt5/qml/org/nemomobile/ngf/
sed 's/Nemo.Ngf/org.nemomobile.ngf/' < declarative/qmldir > %{buildroot}%{_libdir}/qt5/qml/org/nemomobile/ngf/qmldir

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libngf-qt5.so.*
%license COPYING

%files devel
%defattr(-,root,root,-)
%{_libdir}/libngf-qt5.so
%{_includedir}/ngf-qt5/*.h
%{_includedir}/ngf-qt5/NgfClient
%{_libdir}/pkgconfig/ngf-qt5.pc

%files declarative
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/Nemo/Ngf/

# org.nemomobile.ngf legacy import
%{_libdir}/qt5/qml/org/nemomobile/ngf/

%files tests
%defattr(-,root,root,-)
/opt/tests/libngf-qt5/
