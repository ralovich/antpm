#!/bin/bash


if [ "$CXX" = "g++" ]; then export CXX="g++-5" CC="gcc-5"; fi
if [ "$CXX" = "clang++" ]; then export CXX="clang++-3.7" CC="clang-3.7"; fi

cd cmake-build
cmake ../src -DCMAKE_BUILD_TYPE=Release -DUSE_BOOST_TEST=TRUE -DCMAKE_INSTALL_PREFIX=/usr -DUSE_BOOST_STATIC_LINK=FALSE
VERBOSE=1 cmake --build .
ctest -j99
cmake --build . --target clean
cmake ../src -DCMAKE_BUILD_TYPE=Debug -DUSE_BOOST_TEST=TRUE -DCMAKE_INSTALL_PREFIX=/usr -DUSE_BOOST_STATIC_LINK=FALSE
VERBOSE=1 cmake --build .
ctest -j99
cmake --build . --target clean
