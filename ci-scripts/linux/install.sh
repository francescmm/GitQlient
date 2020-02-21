sudo apt-get -qq update
sudo apt-get -qq install libgl1-mesa-dev
sudo apt-add-repository -y ppa:ubuntu-toolchain-r/test
sudo add-apt-repository -y ppa:jonathonf/gcc-7.1
sudo apt-get -qq install g++-7 libc6-i386 build-essential libgl1-mesa-dev mesa-common-dev libgles2-mesa-dev
sudo apt-get update
sudo apt-get install gcc-7 g++-7
