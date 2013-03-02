# -*- mode: cmake; coding: utf-8-dos -*-
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.6)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)


PROJECT(antpm C CXX)


SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")


IF(MSVC)
	IF(NOT DEP_ROOT)
		SET(DEP_ROOT "DEP_ROOT-NOTFOUND" CACHE PATH "dependency root path")
	ENDIF()
	SET(LIBUSB_ROOT "${CMAKE_SOURCE_DIR}\\..\\3rd_party\\libusb-win32-bin-1.2.6.0\\")
	SET(BOOST_ROOT "${DEP_ROOT}\\boost_1_43_0" CACHE PATH "Boost PATH" )

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive  -fms-extensions -std=c++0x -Wall")
ENDIF()

INCLUDE(FindLIBUSB)
IF(NOT LIBUSB_FOUND)
  MESSAGE(FATAL_ERROR "Could not find LIBUSB")
ENDIF()

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
FIND_PACKAGE(Boost COMPONENTS thread unit_test_framework program_options system filesystem REQUIRED)
IF(NOT Boost_FOUND)
  MESSAGE(FATAL_ERROR "Boost not found, please set BOOST_ROOT!")
ENDIF()


INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIRS})

LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

IF(MSVC)
ENDIF()

SET(SRCS
  antdefs.hpp
  SmartPtrFwd.hpp   w_inttypes.h w_stdint.h stdintfwd.hpp
  common.hpp common.cpp
  AntMessage.hpp AntMessage.cpp
  AntChannel.hpp   AntChannel.cpp
  AntMessenger.hpp AntMessenger.cpp
  SerialUsb.hpp SerialUsb.cpp
  AntFr310XT.hpp AntFr310XT.cpp
  lqueue.hpp
  Serial.hpp

  FIT.hpp FIT.cpp
  GPX.hpp GPX.cpp
  GarminConvert.hpp GarminConvert.cpp
)
IF(NOT WIN32)
  SET(SRCS
	${SRCS}
    SerialTty.hpp SerialTty.cpp
	)
SET(Boost_THREAD_LIBRARY "${Boost_THREAD_LIBRARY};pthread")
ENDIF()


ADD_LIBRARY(antpm
  ${SRCS}
)
TARGET_LINK_LIBRARIES(antpm
  ${LIBUSB_LIBRARIES}
  ${Boost_THREAD_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
)

ADD_EXECUTABLE(antpm-usbmon2ant
  antpm-usbmon2ant.cpp
)
TARGET_LINK_LIBRARIES(antpm-usbmon2ant
  antpm
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
)
INSTALL(TARGETS antpm-usbmon2ant RUNTIME DESTINATION bin)


ADD_EXECUTABLE(antpm-downloader
  antpm-downloader.cpp
)
TARGET_LINK_LIBRARIES(antpm-downloader
  antpm
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
)
INSTALL(TARGETS antpm-downloader RUNTIME DESTINATION bin)


ADD_EXECUTABLE(antpm-fit2gpx
  antpm-fit2gpx.cpp
)
TARGET_LINK_LIBRARIES(antpm-fit2gpx
  antpm
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
)
INSTALL(TARGETS antpm-fit2gpx RUNTIME DESTINATION bin)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)


set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}-src")
MESSAGE(STATUS "${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_IGNORE_FILES
  "/build/;/.bzr/;~$;${CPACK_SOURCE_IGNORE_FILES};/CMakeLists.txt.user;/gpsbabel/;/gtest-1.6.0/")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kristof Ralovich <tade60@freemail.hu>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ANT+minus")
set(CPACK_PACKAGE_DESCRIPTION "Userspace implementation of a wire protocol similar to the ANT/ANT+ protocols. The goal is to be able to communicate with the Forerunner 310XT watch, in order to retrieve sports tracks. Communication with other watches might work, please report your experiences to improve the software.")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>=4.6.0)")
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6 (>=4.6.0), cmake, debhelper (>= 7), libboost-filesystem, libboost-system")
#set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA 	This variable allow advanced user to add custom script to the control.tar.gz (inside the .deb archive) 	${CMAKE_CURRENT_SOURCE_DIR}/postinst
#set(CPACK_DEBIAN_PACKAGE_SECTION 	Package section (see http://packages.debian.org/stable/) 	Network
#set(CPACK_DEBIAN_PACKAGE_VERSION 	Package version 	${CPACK_PACKAGE_VERSION}+lenny1
INCLUDE(CPack)

