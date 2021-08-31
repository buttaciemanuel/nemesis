#!/bin/bash
# install dependencies
# install libzip
cd
git clone https://github.com/nih-at/libzip.git
cd libzip
mkdir build
cd build
cmake ..
make
make install
cd
rm -rf libzip
# install libcurl
cd
git clone https://github.com/curl/curl.git
cd curl
mkdir build
cd build
cmake ..
make
make install
cd
rm -rf curl