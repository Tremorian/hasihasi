# HasiHasi – Fedora SPEC file
# SPDX-FileCopyrightText: 2026 Nils Pankoke <tremorian@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

Name:           hasihasi
Version:        0.1.0
Release:        1%{?dist}
Summary:        Low-Latency-Voice-Chat für Linux-Gamer
License:        GPL-3.0-or-later
URL:            https://hasihasi.de
Source0:        https://github.com/Tremorian/hasihasi/archive/v%{version}.tar.gz

BuildRequires:  cmake >= 3.25
BuildRequires:  gcc-c++
BuildRequires:  spdlog-devel
BuildRequires:  tomlplusplus-devel
BuildRequires:  asio-devel
BuildRequires:  opus-devel
BuildRequires:  libpipewire-devel
BuildRequires:  openssl-devel
BuildRequires:  protobuf-devel
BuildRequires:  sqlite-devel
BuildRequires:  catch2-devel
BuildRequires:  qt6-qtbase-devel

%description
HasiHasi ist ein Low-Latency-Voice-Chat für Linux-Gamer.
Ziel ist eine Mund-zu-Ohr-Latenz von unter 50 ms im LAN.

%package server
Summary:        HasiHasi Voice Chat Server
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description server
Der HasiHasi Server-Binary, hasihasi-cli und systemd-Unit.

%package cli
Summary:        HasiHasi CLI Admin Tool
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description cli
Command-line admin tool for HasiHasi server administration.

%prep
%autosetup

%build
%cmake
%cmake_build

%install
%cmake_install

%files
/usr/bin/hasihasi
/usr/share/applications/de.hasihasi.hasihasi.desktop
/usr/share/metainfo/de.hasihasi.hasihasi.metainfo.xml

%files server
/usr/bin/hasihasi-server
%{_unitdir}/hasihasi-server.service
/etc/hasihasi/server.toml

%files cli
/usr/bin/hasihasi-cli

%post server
%systemd_post hasihasi-server.service

%preun server
%systemd_preun hasihasi-server.service

%postun server
%systemd_unpack hasihasi-server.service
