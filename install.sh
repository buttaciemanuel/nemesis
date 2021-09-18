#!/bin/bash
# install libzip
cd temp
git clone https://github.com/nih-at/libzip.git
cd libzip
mkdir build
cd build
cmake ..
make
make install
cd ..
rm -rf libzip
# install libcurl
cd temp
git clone https://github.com/curl/curl.git
cd curl
mkdir build
cd build
cmake ..
make
make install
cd ..
rm -rf curl
# remove temporary directory
rm temp
cd ..
# build the compiler and package manager
make
# install executable on disk
cp build/driver /usr/bin/nemesis
# install core library on disk
mkdir /usr/lib/nemesis
cp -r libcore /usr/lib/nemesis/core
# export environment variable for path (must be execute like `source install.sh`)
export NEMESIS_PATH='/usr/bin/nemesis'