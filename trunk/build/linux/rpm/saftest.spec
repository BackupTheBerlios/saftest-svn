Summary: SAFTest
Name: saftest
Version: %{saftest_version}
Release: %{saftest_release}
Group: System
Copyright: Hewlett Packard Company, see COPYRIGHT file for details
Packager: Chad Tindel <chad@tindel.net>
URL: http://saftest.berlios.de
Source: unavailable
AutoReqProv: no

#Requires: 

%define _builddir	%{_topdir}
%define objdir	%{_topdir}/%{buildobjdir}
%define _buildroot	%{objdir}/rpmimage/saftest
BuildRoot:	%{_buildroot}
%define _rpmdir	%{objdir}
%define _rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%{distro}.%%{ARCH}.rpm

# turn off man page compressing and binary stripping
%define __spec_install_post /bin/true

%description
SAF AIS Compliance and Functional Tests

%prep

%build

%install
rm -rf %{_buildroot}
%define testbase /opt/saftest
%define installroot %{_buildroot}/%{testbase}

mkdir -p %{installroot}
mkdir -p %{installroot}/{bin,cases,conf,implementation,lib,xml}
mkdir -p %{installroot}/objs/final

cp -Rf %{_topdir}/bin %{installroot}
cp -Rf %{_topdir}/cases %{installroot}
cp -Rf %{_topdir}/conf %{installroot}
cp -Rf %{_topdir}/implementation %{installroot}
cp -Rf %{_topdir}/lib %{installroot}
cp -Rf %{_topdir}/xml %{installroot}
cp -Rf %{_topdir}/objs/final %{installroot}

%clean
chmod 777 $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_ROOT

%post 

%preun

%postun

%files

%defattr(-,root,root)
%attr(-,root,root) %{testbase}
