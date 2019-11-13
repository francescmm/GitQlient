wget -O qt5.zip https://github.com/francescmm/ci-utils/releases/download/qt_5.13/qt5.zip;
unzip -qq qt5.zip;
export QTDIR=$PWD/qt5
export PATH=$QTDIR/bin:$PATH;
export QT_PLUGIN_PATH=$PWD/qt5/plugins;
$QTDIR/bin/qmake GitQlient.pro
make -j 4
cp GitQlient GitQlient_"${TRAVIS_OS_NAME}"
