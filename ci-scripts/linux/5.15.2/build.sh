export QTDIR="/opt/qt515"
export PATH="$QTDIR/bin:$PATH"
source /opt/qt515/bin/qt515-env.sh
export QT_PLUGIN_PATH=$QTDIR/plugins;
mkdir build
$QTDIR/bin/qmake GitQlient.pro PREFIX=$(pwd)/AppImage/gitqlient/usr QMAKE_CXXFLAGS+=-Werror
make -j 4
make install
wget -q -O linuxdeployqt https://github.com/probonopd/linuxdeployqt/releases/download/6/linuxdeployqt-6-x86_64.AppImage
chmod +x linuxdeployqt
./linuxdeployqt AppImage/gitqlient/usr/share/applications/*.desktop -appimage -no-translations -bundle-non-qt-libs -extra-plugins=iconengines,imageformats
chmod +x GitQlient-*
cp GitQlient-* ../
