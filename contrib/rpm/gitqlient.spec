Name:    {{{ git_name name="gitqlient" }}}
Version: 1.2.0
Release: {{{ git_version }}}%{?dist}
Summary: A multi-platform Git client

License:    LGPLv2
URL:        https://github.com/francescmm/GitQlient
VCS:        {{{ git_vcs }}}

Source:     {{{ git_pack }}}

BuildRequires: qt5-qtbase-devel
BuildRequires: qt5-qtsvg-devel

%description
%{summary}.

%prep
{{{ git_setup_macro }}}

%build
%qmake_qt5 GitQlient.pro
%make_build

%install
%make_install
install -p -m 755 GitQlient %{buildroot}%{_bindir}/GitQlient

%files
%doc README.md
%license LICENSE
%{_bindir}/GitQlient

%changelog
{{{ git_changelog }}}
