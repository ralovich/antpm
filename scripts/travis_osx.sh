#!/bin/sh

cd cmake-build
cmake ../src -DCMAKE_BUILD_TYPE=Release -DUSE_BOOST_TEST=TRUE -DCMAKE_INSTALL_PREFIX=/usr -DUSE_BOOST_STATIC_LINK=FALSE
VERBOSE=1 cmake --build .
ctest -j99
cmake --build . --target clean
cmake ../src -DCMAKE_BUILD_TYPE=Debug -DUSE_BOOST_TEST=TRUE -DCMAKE_INSTALL_PREFIX=/usr -DUSE_BOOST_STATIC_LINK=FALSE
VERBOSE=1 cmake --build .
ctest -j99
cmake --build . --target clean

