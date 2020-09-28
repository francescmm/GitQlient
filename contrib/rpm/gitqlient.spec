Name:    {{{ git_name name="gitqlient" }}}
Version: 1.2.0
Release: {{{ git_version }}}%{?dist}
Summary: A multi-platform Git client

License:    LGPLv2
URL:        https://github.com/francescmm/GitQlient
VCS:        {{{ git_vcs }}}

Source:     {{{ git_pack }}}

BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5Network)

%description
%{summary}.

%prep
{{{ git_setup_macro }}}

%build
%qmake_qt5 GitQlient.pro
%make_build

%install
#installs files into the user home directory by default
#%make_install
install -D -p -m755 GitQlient %{buildroot}%{_bindir}/GitQlient

%files
%doc README.md
%license LICENSE
%{_bindir}/GitQlient

%changelog
{{{ git_changelog }}}
