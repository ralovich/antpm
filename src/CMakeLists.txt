# -*- mode: cmake; coding: utf-8-dos -*-
CMAKE_MINIMUM_REQUIRED(VERSION 3.14.2)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

option(macos_native "don't look for non apple system paths" OFF) # https://gitlab.kitware.com/cmake/cmake/-/issues/21918
if(macos_native)
  if(APPLE)
    set(CMAKE_IGNORE_PATH)
    # Fink installs in /sw, and MacPort in /opt/local and Brew in /usr/local (x86-64) and /opt/homebrew (arm64)
    foreach(_prefix /sw /opt/local /usr/local /opt/homebrew)
      list(APPEND CMAKE_IGNORE_PATH ${_prefix}/bin ${_prefix}/include ${_prefix}/lib)
    endforeach()
  else()
    message(STATUS "Option 'macos_native' is only for MacOS systems. Ignoring it.")
  endif()
endif()

PROJECT(antpm C CXX)
SET(CMAKE_CXX_STANDARD 17)
SET(CXX_STANDARD_REQUIRED ON)

OPTION(USE_COVERAGE "use gcov" FALSE)
OPTION(USE_BOOST_TEST "use boost::test" FALSE)
OPTION(USE_BOOST_STATIC_LINK "link statically against boost" TRUE)
IF(MSVC OR APPLE)
  OPTION(USE_GANT "build gant" FALSE)
ELSE()
  OPTION(USE_GANT "build gant" TRUE)
ENDIF()

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Set a default build type for single-configuration
# CMake generators if no build type is set.
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE Debug)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
MESSAGE(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "CMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATION_TYPES}")

set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG=1")
set(CMAKE_C_FLAGS_RELEASE          "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG=1")

IF(MSVC)
    SET(LIBUSB_ROOT "C:/vcpkg/installed/x64-windows" CACHE PATH "Libusb PATH")
    SET(BOOST_ROOT "c:/local/boost_1_87_0"  CACHE PATH "Boost PATH" )

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
  ADD_DEFINITIONS(-D_WIN32_WINNT=0x0500)
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS                "${CMAKE_CXX_FLAGS} -Wall -Werror")
  SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g")
  SET(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} -O2")
  set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3 -ggdb3")
  set(CMAKE_C_FLAGS                  "${CMAKE_C_FLAGS} -std=c99 -pedantic -Wextra -Wall")
  set(CMAKE_C_FLAGS_DEBUG            "${CMAKE_C_FLAGS_DEBUG} -g3 -ggdb3")

  IF(USE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
  ENDIF()
ENDIF()

IF(MSVC)
  INCLUDE(Findlibusb-1.0)
  IF(NOT LIBUSB_1_FOUND)
    MESSAGE(FATAL_ERROR "Could not find libusb-1.0")
  ENDIF()
ELSE()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBUSB_1 REQUIRED libusb-1.0)
ENDIF()
MESSAGE(STATUS "LIBUSB_1: ${LIBUSB_1_INCLUDE_DIRS} ${LIBUSB_1_LIBRARY_DIRS} ${LIBUSB_1_LIBRARIES}")

IF(USE_BOOST_STATIC_LINK)
#    set(Boost_VERBOSE ON)
#    set(Boost_DEBUG ON)
  set(Boost_USE_STATIC_LIBS        ON)
  set(Boost_USE_STATIC_RUNTIME    OFF)
ELSE()
  set(Boost_USE_STATIC_LIBS        OFF)
  set(Boost_USE_STATIC_RUNTIME    OFF)
  ADD_DEFINITIONS(-DBOOST_TEST_MAIN)
  ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
ENDIF()
set(Boost_USE_MULTITHREADED      ON)
FIND_PACKAGE(Boost 1.41.0 COMPONENTS unit_test_framework program_options REQUIRED)
#IF(NOT Boost_FOUND)
#  MESSAGE(FATAL_ERROR "Boost not found!")
#ENDIF()

IF(MSVC)
  ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_WARNINGS)
  #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libboost_date_time-vc90-mt-s-1_43.lib /NODEFAULTLIB:libboost_thread-vc90-mt-s-1_43.lib /NODEFAULTLIB:libboost_program_options-vc90-mt-s-1_43.lib /NODEFAULTLIB:libboost_filesystem-vc90-mt-s-1_43.lib /NODEFAULTLIB:libboost_system-vc90-mt-s-1_43.lib")
ENDIF()

FILE(READ "${PROJECT_SOURCE_DIR}/ANTPM_VERSION.inc" ANTPM_VERSION)
MESSAGE(STATUS "ANTPM_VERSION=\\\"${ANTPM_VERSION}\\\"")
ADD_DEFINITIONS(-DANTPM_VERSION=${ANTPM_VERSION})

IF(USE_BOOST_TEST)

  if(${VALGRIND_PREFIX})
    message(STATUS "Valgrind Prefix: ${VALGRIND_PREFIX}")
  endif()
  find_program(VALGRIND_PROGRAM NAMES valgrind PATH /usr/bin /usr/local/bin ${VALGRIND_PREFIX}/bin)
  IF(VALGRIND_PROGRAM)
    message(STATUS "Found Valgrind: ${VALGRIND_PROGRAM}")
    SET(MEMORYCHECK_COMMAND:FILEPATH=${VALGRIND_PROGRAM})
    # You can add additional options by specifying MEMORYCHECK_COMMAND_OPTIONS and MEMORYCHECK_SUPPRESSIONS_FILE. 
  ENDIF()

  enable_testing()
ENDIF()
  
SET(SRCS
  antdefs.hpp antdefs.cpp
  w_inttypes.h w_stdint.h stdintfwd.hpp
  common.hpp common.cpp
  AntMessage.hpp AntMessage.cpp
  AntChannel.hpp   AntChannel.cpp
  AntMessenger.hpp AntMessenger.cpp
  SerialUsb.hpp SerialUsb.cpp
  AntFr310XT.hpp AntFr310XT.cpp
  lqueue.hpp
  Serial.hpp Serial.cpp

  FIT.hpp FIT.cpp
  GPX.hpp GPX.cpp
  GarminConvert.hpp GarminConvert.cpp
  GarminPacketIntf.hpp GarminPacketIntf.cpp

  DeviceSettings.hpp DeviceSettings.cpp

  Log.hpp
  LazySingleton.hpp

  ANTPM_VERSION.inc
)
IF(NOT WIN32)
  SET(SRCS
      ${SRCS}
      SerialTty.hpp SerialTty.cpp
      )
  SET(MY_ANTPM_LIBRARIES "pthread")
  IF(NOT APPLE)
    SET(MY_ANTPM_LIBRARIES "${MY_ANTPM_LIBRARIES};rt")
  ENDIF()
  IF(APPLE)
      SET(LIBUSB_1_LIBRARIES ${pkgcfg_lib_LIBUSB_1_usb-1.0})
  ENDIF()
ENDIF()


ADD_LIBRARY(antpm
  ${SRCS}
  )
TARGET_INCLUDE_DIRECTORIES(antpm PUBLIC .)
TARGET_INCLUDE_DIRECTORIES(antpm PRIVATE ${LIBUSB_1_INCLUDE_DIRS})
TARGET_INCLUDE_DIRECTORIES(antpm PRIVATE ${Boost_INCLUDE_DIR})
TARGET_LINK_DIRECTORIES(antpm
    PRIVATE ${LIBUSB_1_LIBRARY_DIRS}
    )
TARGET_LINK_LIBRARIES(antpm
  ${LIBUSB_1_LIBRARIES}
  ${MY_ANTPM_LIBRARIES}
)

ADD_EXECUTABLE(antpm-usbmon2ant
  antpm-usbmon2ant.cpp
)
TARGET_LINK_LIBRARIES(antpm-usbmon2ant
  antpm
  Boost::program_options
)
INSTALL(TARGETS antpm-usbmon2ant RUNTIME DESTINATION bin)


ADD_EXECUTABLE(antpm-downloader
  antpm-downloader.cpp
)
TARGET_LINK_LIBRARIES(antpm-downloader
  antpm
  Boost::program_options
)
INSTALL(TARGETS antpm-downloader RUNTIME DESTINATION bin)


ADD_EXECUTABLE(antpm-fit2gpx
  antpm-fit2gpx.cpp
)
TARGET_LINK_LIBRARIES(antpm-fit2gpx
  antpm
  Boost::program_options
)
INSTALL(TARGETS antpm-fit2gpx RUNTIME DESTINATION bin)

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

IF(USE_GANT AND NOT WIN32)
  ADD_SUBDIRECTORY(gant)
ENDIF()


IF(USE_BOOST_TEST)
  ADD_SUBDIRECTORY(tests)
ENDIF()
