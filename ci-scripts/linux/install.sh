sudo add-apt-repository --yes ppa:beineri/opt-qt597-trusty
sudo add-apt-repository --yes ppa:achadwick/mypaint-testing
sudo apt-add-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get -qq install g++-4.9 libc6-i386
export CXX="g++-4.9 -std=c++14"
export CC="gcc-4.9"

