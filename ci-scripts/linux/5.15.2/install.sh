sudo add-apt-repository -y ppa:beineri/opt-qt-5.15.2-xenial
sudo apt-add-repository -y ppa:ubuntu-toolchain-r/test
sudo add-apt-repository -y ppa:jonathonf/gcc-9
sudo apt-get -qq update
sudo apt-get -qq install libgl1-mesa-dev libxkbcommon-x11-0 libc6-i386 build-essential libgl1-mesa-dev mesa-common-dev libgles2-mesa-dev libxkbcommon-x11-0 libxcb-icccm4-dev libxcb-xinerama0 libxcb-image0 libxcb-keysyms1 libxcb-*
sudo apt-get -qq remove gcc g++
sudo apt-get install -y -qq gcc-9 g++-9 qt515base qt515webchannel qt515webengine
sudo ln -s /usr/bin/g++-9 /usr/bin/g++
sudo ln -s /usr/bin/gcc-9 /usr/bin/gcc
