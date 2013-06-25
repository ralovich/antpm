
set BOOST_ROOT=d:\work\deps-vs2008\boost_1_43_0\
set BUILD=..\build-vs2008\

mkdir %BUILD%

pushd %BUILD%
cmake.exe -G "Visual Studio 9 2008" -DBOOST_ROOT=%BOOST_ROOT% ..\src\
popd

set PATH=%CD%\..\3rd_party\libusb-win32-bin-1.2.6.0\bin\x86\;%PATH%

start "c:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv.exe" "%BUILD%\antpm.sln"
