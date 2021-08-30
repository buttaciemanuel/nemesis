#!/bin/bash
# install dependencies
# install boost
apt install build-essential libboost-system-dev libboost-thread-dev libboost-program-options-dev libboost-test-dev
# install libzip
cd
wget https://libzip.org/download/libzip-1.8.0.tar.gz
tar xvfz libzip-1.8.0.tar.gz
cd libzip-1.8.0
mkdir build
cd build
cmake ..
make
make install
cd
rm -rf libzip-1.8.0
# install cppnetlib
cd
git clone https://github.com/cpp-netlib/cpp-netlib
cd cpp-netlib
git submodule init
git submodule update
mkdir build
cd build
cmake -DCPP-NETLIB_ENABLE_HTTPS=Off -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
make
cd
rm -rf cpp-netlib
