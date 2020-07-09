export PATH="/usr/local/opt/qt/bin:$PATH"
source ~/.bash_profile
export LDFLAGS="-L/usr/local/opt/qt/lib"
export CPPFLAGS="-I/usr/local/opt/qt/include"
qmake GitQlient.pro
make
make dmg
cp GitQlient-* ../
