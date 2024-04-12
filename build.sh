#!/bin/bash
cd "$(dirname "$0")"
cmake . && make
rm -rf cmake-build-debug
rm -rf cmake-build-release
rm -rf CMakeFiles
rm -f cmake_install.cmake
rm -f CMakeCache.txt
rm -f Makefile