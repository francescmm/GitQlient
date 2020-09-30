wget -q -O qt5.zip https://github.com/francescmm/ci-utils/releases/download/qt/qt5.zip;
unzip -qq qt5.zip;
export QTDIR=$PWD/qt5
export PATH=$QTDIR/bin:$PATH;
export QT_PLUGIN_PATH=$PWD/qt5/plugins;
mkdir build
$QTDIR/bin/qmake GitQlient.pro PREFIX=$(pwd)/AppImage/gitqlient/usr
make -j 4
make install
wget -q -O linuxdeployqt https://github.com/probonopd/linuxdeployqt/releases/download/6/linuxdeployqt-6-x86_64.AppImage
chmod +x linuxdeployqt
./linuxdeployqt AppImage/gitqlient/usr/share/applications/*.desktop -appimage -no-translations -bundle-non-qt-libs -verbose=2 -extra-plugins=iconengines,imageformats
chmod +x GitQlient-*
cp GitQlient-* ../
