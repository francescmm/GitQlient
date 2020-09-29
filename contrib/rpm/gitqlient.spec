Name:    {{{ git_name name="gitqlient" }}}
Version: 1.2.0
Release: {{{ git_version }}}%{?dist}
Summary: A multi-platform Git client

License:    LGPLv2
URL:        https://github.com/francescmm/GitQlient
VCS:        {{{ git_vcs }}}

Source:     {{{ git_pack }}}

BuildRequires:  /usr/bin/qmake-qt5
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5Network)

BuildRequires: desktop-file-utils

%description
%{summary}.

%prep
{{{ git_setup_macro }}}

%build
%if 0%{?suse_version}
#https://en.opensuse.org/openSUSE:Build_system_recipes#qmake
qmake-qt5 -makefile QMAKE_CFLAGS+="%optflags" QMAKE_CXXFLAGS+="%optflags" QMAKE_STRIP="/bin/true" GitQlient.pro
%else
%qmake_qt5 GitQlient.pro
%endif

%make_build

%install

#installs files into the user home directory by default
#%make_install
install -D -p -m755 GitQlient %{buildroot}%{_bindir}/GitQlient

desktop-file-install                                    \
--dir=%{buildroot}%{_datadir}/applications              \
AppImage/GitQlient/usr/share/applications/GitQlient.desktop

install -dD %{buildroot}%{_datadir}/icons
cp -a AppImage/GitQlient/usr/share/icons/* %{buildroot}%{_datadir}/icons/

%files
%doc README.md
%license LICENSE
%{_bindir}/GitQlient
%{_datadir}/applications/GitQlient.desktop
%{_datadir}/icons

%changelog
{{{ git_changelog }}}
