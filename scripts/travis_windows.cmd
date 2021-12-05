@rem -*- mode: dos; coding: utf-8-dos -*-
@echo on

echo %PATH%
cd
choco install boost-msvc-14.1
cd cmake-build
cmake ../src -DCMAKE_BUILD_TYPE=Release -DUSE_BOOST_TEST=TRUE -DCMAKE_INSTALL_PREFIX="c:\opt\" -DUSE_BOOST_STATIC_LINK=FALSE
