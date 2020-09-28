Name:    {{{ git_name name="gitqlient" }}}
Version: 1.2.0
Release: {{{ git_version }}}%{?dist}
Summary: A multi-platform Git client

License:    LGPLv2
URL:        https://github.com/francescmm/GitQlient
VCS:        {{{ git_vcs }}}

Source:     {{{ git_pack }}}

%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}
BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-qtsvg-devel
%endif

%if 0%{?suse_version}
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Svg)
%endif

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
