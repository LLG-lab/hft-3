%define name		hft-system
%define rel		0_%{build_number}
%define version		3.0.0
%global __os_install_post %{nil}

BuildRoot:		%{buildroot}
Summary:		High Frequency Trading System - Professional Expert Advisor
License:		Propertiary
Name:			%{name}
Version:		%{version}
Release:		%{rel}.el%{?rhel}
Group:			System Environment/Daemons
Vendor:			LLG Ryszard Gradowski
Packager:		LLG Ryszard Gradowski
BuildRequires:		make, centos-release-scl, devtoolset-7-gcc-c++, devtoolset-7-gcc, devtoolset-7-gcc-plugin-devel, devtoolset-7-gcc-gdb-plugin,openssl-devel, python-devel, curl-devel, sqlite-devel, newt-devel, maven
Requires:		curl, sqlite, newt, php-cli >= 5.4.16, java-1.8.0-openjdk, nodejs
Requires(preun):	systemd-units
Requires(postun):	systemd-units
Requires(post):		systemd-units

%description
This is a core part of High Frequency Trading System.
Package delivers HFT server and HFT-related toolset.

%prep
# We have source already prepared on
# toplevel script, so nothing here.

%build
pushd hft
scl enable devtoolset-7 "./configure --release"
scl enable devtoolset-7 make
popd
pushd caans
scl enable devtoolset-7 "./configure --release"
scl enable devtoolset-7 make
popd
pushd dukascopy-bridge/pl.com.hlabber
mvn clean install
popd

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_sbindir}
install -m 755 -p hft/hft $RPM_BUILD_ROOT%{_sbindir}/hft
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/hft
install -m 644 -p hft/deploy/hftd.conf.default $RPM_BUILD_ROOT%{_sysconfdir}/hft/hftd.conf
install -m 644 -p hft/deploy/bridges.json  $RPM_BUILD_ROOT%{_sysconfdir}/hft/bridges.json
mkdir -p $RPM_BUILD_ROOT%{_bindir}
install -m 755 -p hft/deploy/hft-manifest-edit.js $RPM_BUILD_ROOT%{_bindir}/hft-manifest-edit
mkdir -p $RPM_BUILD_ROOT%{_unitdir}
install -m 644 -p hft/deploy/hft.service $RPM_BUILD_ROOT%{_unitdir}/hft.service
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d
install -m 644 -p hft/deploy/hft.logrotate $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d/hft
mkdir -p $RPM_BUILD_ROOT/%{_localstatedir}/log/hft
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/cache
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/cache/collectors
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/cache/HCIs
mkdir -p $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/bridge/{dukascopy-bridge,bitmex-bridge}
#Bitmex
install -m 644 -p bitmex-bridge/BitMex.php $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/bridge/bitmex-bridge/BitMex.php
install -m 644 -p bitmex-bridge/Logger.php $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/bridge/bitmex-bridge/Logger.php
install -m 644 -p bitmex-bridge/HftConnector.php $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/bridge/bitmex-bridge/HftConnector.php
install -m 644 -p bitmex-bridge/bitmex-bridge.php $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/bridge/bitmex-bridge/bitmex-bridge.php
#Dukascopy
install -m 644 -p dukascopy-bridge/pl.com.hlabber/target/hft-bridge-1.0.0.jar $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/bridge/dukascopy-bridge/hft-bridge-1.0.0.jar
find /root/.m2/repository/ -name "*.jar" | while read line ; do install -m 644 -p  "$line" $RPM_BUILD_ROOT/%{_sharedstatedir}/hft/bridge/dukascopy-bridge/ ; done
#CAANS
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/caans
install -m 644 -p caans/deploy/caansd.conf.default $RPM_BUILD_ROOT%{_sysconfdir}/caans/caansd.conf
install -m 644 -p caans/deploy/caansd.service $RPM_BUILD_ROOT%{_unitdir}/caansd.service
install -m 755 -p caans/caans $RPM_BUILD_ROOT%{_sbindir}/caans
#miscellaneous
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/cron.d/
install -m 755 -p etc/deploy/hftwatchdog-run.sh $RPM_BUILD_ROOT%{_sharedstatedir}/hft/hftwatchdog-run.sh
install -m 755 -p etc/deploy/hft-watchdog.sh $RPM_BUILD_ROOT%{_bindir}/hft-watchdog
install -m 644 -p etc/deploy/hftwatchdog-run.status $RPM_BUILD_ROOT%{_sharedstatedir}/hft/hftwatchdog-run.status
install -m 644 -p etc/deploy/hft-cron $RPM_BUILD_ROOT%{_sysconfdir}/cron.d/hft-cron

%post
mkdir -p $RPM_BUILD_ROOT%{_sharedstatedir}/hft/cache/{collectors,HCIs}
mkdir -p $RPM_BUILD_ROOT%{_localstatedir}/log/hft
%systemd_post hftd.service caansd.service

%preun
%systemd_preun hftd.service caansd.service

%postun
%systemd_postun_with_restart hftd.service caansd.service

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_sbindir}/hft
%{_sbindir}/caans
%{_bindir}/hft-manifest-edit
%{_bindir}/hft-watchdog
%{_unitdir}/hft.service
%{_unitdir}/caansd.service
%{_sharedstatedir}/hft/bridge/bitmex-bridge/BitMex.php
%{_sharedstatedir}/hft/bridge/bitmex-bridge/Logger.php
%{_sharedstatedir}/hft/bridge/bitmex-bridge/HftConnector.php
%{_sharedstatedir}/hft/bridge/bitmex-bridge/bitmex-bridge.php
%{_sharedstatedir}/hft/bridge/dukascopy-bridge/*.jar
%{_sharedstatedir}/hft/hftwatchdog-run.sh
%{_sysconfdir}/cron.d/hft-cron
%config(noreplace) %{_sysconfdir}/logrotate.d/hft
%config(noreplace) %{_sysconfdir}/hft/hftd.conf
%config(noreplace) %{_sysconfdir}/hft/bridges.json
%config(noreplace) %{_sysconfdir}/caans/caansd.conf
%config(noreplace) %{_sharedstatedir}/hft/hftwatchdog-run.status
