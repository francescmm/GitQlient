export QTDIR="/opt/qt512"
export PATH="$QTDIR/bin:$PATH"
source /opt/qt512/bin/qt512-env.sh
mkdir build
$QTDIR/bin/qmake GitQlient.pro QMAKE_CXXFLAGS+=-Werror
make -j 4
make install
echo "Version: ${VERSION}" >> deb_pkg/DEBIAN/control
echo "Standards-Version: ${VERSION}" >> deb_pkg/DEBIAN/control
mkdir -p deb_pkg/usr/local/bin
cp gitqlient deb_pkg/usr/local/bin/
mv deb_pkg gitqlient_${VERSION}_amd64
fakeroot dpkg-deb -v --build gitqlient_${VERSION}_amd64
cp *.deb ../
