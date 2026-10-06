# Build from a source tarball: packaging/opensuse/build-rpm.sh
Name:           p2td
Version:        0.1.0
Release:        1%{?dist}
Summary:        Push-to-talk daemon with system tray (evdev, PulseAudio)
License:        MIT
URL:            https://github.com/example/p2td
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  cmake >= 3.20
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-svg-devel
BuildRequires:  libX11-devel
BuildRequires:  libXtst-devel
BuildRequires:  pkgconfig(libpulse-simple)
BuildRequires:  python3
BuildRequires:  ImageMagick
BuildRequires:  systemd-rpm-macros
Requires:       pipewire-pulse

%description
Global push-to-talk: mute/unmute PulseAudio recording sources on button
press/release. Qt system tray, evdev input (setgid input on the binary).

%prep
%setup -q -n %{name}-%{version}

%build
cmake -S cpp -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j%{?_smp_build_cpus}

%install
mkdir -p %{buildroot}%{_datadir}/icons/hicolor/48x48/apps
bash packaging/generate-icon.sh %{buildroot}%{_datadir}/icons/hicolor/48x48/apps/p2td.png
cmake --install build --prefix %{buildroot}%{_prefix}

%files
%attr(2755, root, input) %{_bindir}/p2td
%{_datadir}/p2td/sounds/mute.wav
%{_datadir}/p2td/sounds/unmute.wav
%{_libdir}/systemd/user/p2td.service
%{_datadir}/applications/p2td.desktop
%{_datadir}/icons/hicolor/48x48/apps/p2td.png

%post
%systemd_user_post p2td.service

%preun
%systemd_user_preun p2td.service

%postun
%systemd_user_postun_with_restart p2td.service

%changelog
