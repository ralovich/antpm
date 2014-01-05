# -*- mode: cmake; coding: utf-8-dos -*-
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.2)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)


PROJECT(antpm C CXX)

OPTION(USE_COVERAGE "use gcov" FALSE)
OPTION(USE_BOOST_TEST "use boost::test" FALSE)
OPTION(USE_BOOST_STATIC_LINK "link statically against boost" TRUE)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_FLAGS_RELEASE        "-DNDEBUG")
#set(CMAKE_CXX_FLAGS_DEBUGFULL      "-g -Ob0 -noalign")
#set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g")
#set(CMAKE_C_FLAGS_RELEASE          "-O2")
set(CMAKE_C_FLAGS_RELEASE          "-DNDEBUG")

IF(MSVC)
	IF(NOT DEP_ROOT)
		SET(DEP_ROOT "DEP_ROOT-NOTFOUND" CACHE PATH "dependency root path")
	ENDIF()
	SET(LIBUSB_ROOT "${CMAKE_SOURCE_DIR}\\..\\3rd_party\\libusb-win32-bin-1.2.6.0\\")
	SET(BOOST_ROOT "${DEP_ROOT}\\boost_1_43_0" CACHE PATH "Boost PATH" )

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive  -fms-extensions -std=c++0x -Wall")
  set(CMAKE_CXX_FLAGS_DEBUG          "-g3 -ggdb3")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wextra -Wall")
  set(CMAKE_C_FLAGS_DEBUG            "-g3 -ggdb3")

  IF(USE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
  ENDIF()
ENDIF()

INCLUDE(FindLIBUSB)
IF(NOT LIBUSB_FOUND)
  MESSAGE(FATAL_ERROR "Could not find LIBUSB")
ENDIF()

IF(USE_BOOST_STATIC_LINK)
  set(Boost_USE_STATIC_LIBS        ON)
ELSE()
  set(Boost_USE_STATIC_LIBS        OFF)
ENDIF()
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
FIND_PACKAGE(Boost 1.41.0 COMPONENTS thread unit_test_framework program_options system filesystem date_time REQUIRED)
IF(NOT Boost_FOUND)
  MESSAGE(FATAL_ERROR "Boost not found!")
ENDIF()


INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIRS})

LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

IF(MSVC)
  ADD_DEFINITIONS(-D_CRT_NONSTDC_NO_WARNINGS)
  #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:libboost_date_time-vc90-mt-s-1_43.lib /NODEFAULTLIB:libboost_thread-vc90-mt-s-1_43.lib /NODEFAULTLIB:libboost_program_options-vc90-mt-s-1_43.lib /NODEFAULTLIB:libboost_filesystem-vc90-mt-s-1_43.lib /NODEFAULTLIB:libboost_system-vc90-mt-s-1_43.lib")
ENDIF()

FILE(READ "${PROJECT_SOURCE_DIR}/VERSION" ANTPM_VERSION)
MESSAGE(STATUS "ANTPM_VERSION=\\\"${ANTPM_VERSION}\\\"")
ADD_DEFINITIONS(-DANTPM_VERSION=${ANTPM_VERSION})

SET(SRCS
  antdefs.hpp
  SmartPtrFwd.hpp   w_inttypes.h w_stdint.h stdintfwd.hpp
  common.hpp common.cpp
  AntMessage.hpp AntMessage.cpp
  AntChannel.hpp   AntChannel.cpp
  AntMessenger.hpp AntMessenger.cpp
  SerialUsb.hpp SerialUsb.cpp
  AntFr310XT.hpp AntFr310XT.cpp
  AntFr405.hpp AntFr405.cpp
  lqueue.hpp
  Serial.hpp

  FIT.hpp FIT.cpp
  GPX.hpp GPX.cpp
  GarminConvert.hpp GarminConvert.cpp
  GarminPacketIntf.hpp GarminPacketIntf.cpp

  DeviceSettings.hpp DeviceSettings.cpp

  Log.hpp
  LazySingleton.hpp

  VERSION
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
  ${Boost_DATE_TIME_LIBRARY}
  ${LIBUSB_LIBRARIES}
  ${Boost_THREAD_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  stdc++
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

ADD_SUBDIRECTORY(gant)

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
  ADD_SUBDIRECTORY(tests)
ENDIF()
