ADD_DEFINITIONS(-DUSE_BOOST_TEST=1)
ADD_DEFINITIONS(-DTEST_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}\")
MESSAGE(STATUS "TEST_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}\"")

ADD_EXECUTABLE(testDeviceSettings
  testDeviceSettings.cpp
)
TARGET_LINK_LIBRARIES(testDeviceSettings
  antpm
  Boost::program_options
  Boost::unit_test_framework
)

ADD_EXECUTABLE(testDateTime
  testDateTime.cpp
)
TARGET_LINK_LIBRARIES(testDateTime
  antpm
  Boost::program_options
  Boost::unit_test_framework
)


ADD_EXECUTABLE(sm1
  sm1.cpp
)
TARGET_LINK_LIBRARIES(sm1
  antpm
  Boost::unit_test_framework
)

ADD_EXECUTABLE(lq1
  lq1.cpp
)
TARGET_LINK_LIBRARIES(lq1
  antpm
  Boost::unit_test_framework
)


IF(VALGRIND_PROGRAM)
  function(ADD_MY_TEST name binary)
    #set(memcheck_command "${CMAKE_MEMORYCHECK_COMMAND} ${CMAKE_MEMORYCHECK_COMMAND_OPTIONS}")
    #separate_arguments(memcheck_command)
    add_test(${name} ${binary} ${ARGN})
    add_test(memcheck_${name} ${VALGRIND_PROGRAM} --leak-check=full ./${binary} ${ARGN})
  endfunction()

  function(set_memcheck_test_properties name)
    set_tests_properties(${name} ${ARGN})
    set_tests_properties(memcheck_${name} ${ARGN})
  endfunction()
ELSE()
  function(ADD_MY_TEST name binary)
    add_test(${name} ${binary} ${ARGN})
  endfunction()
ENDIF()

ADD_MY_TEST(testDeviceSettings1 testDeviceSettings)

ADD_MY_TEST(testDateTime1 testDateTime)

ADD_MY_TEST(testDeviceSettings2 testDeviceSettings --help)

ADD_MY_TEST(testDateTime2 testDateTime --help)

ADD_MY_TEST(testDownload ../antpm-downloader)

ADD_MY_TEST(testFit2Gpx ../antpm-fit2gpx)

ADD_MY_TEST(testUsbmon2AntParse  ../antpm-usbmon2ant --op parse  ${CMAKE_CURRENT_SOURCE_DIR}/2012_12_21_antagent1.usbmon)
ADD_MY_TEST(testUsbmon2AntDump   ../antpm-usbmon2ant --op dump   ${CMAKE_CURRENT_SOURCE_DIR}/2012_12_21_antagent1.usbmon)
ADD_MY_TEST(testUsbmon2AntUsbmon ../antpm-usbmon2ant --op usbmon ${CMAKE_CURRENT_SOURCE_DIR}/2012_12_21_antagent1.usbmon)
ADD_MY_TEST(testUsbmon2AntFilter ../antpm-usbmon2ant --op filter ${CMAKE_CURRENT_SOURCE_DIR}/2012_12_21_antagent1.usbmon)
ADD_MY_TEST(testUsbmon2AntCount  ../antpm-usbmon2ant --op count  ${CMAKE_CURRENT_SOURCE_DIR}/2012_12_21_antagent1.usbmon)

ADD_MY_TEST(testLockedQueue lq1 --log_level=all)

ADD_MY_TEST(testStateMachineAsio sm1 --build_info --log_level=all --run_test=test_asio)
ADD_TEST(testStateMachine0 sm1 --build_info --log_level=all --run_test=test_serial0)
ADD_TEST(testStateMachine1 sm1 --build_info --log_level=all --run_test=test_serial1)
ADD_TEST(testStateMachine2 sm1 --build_info --log_level=all --run_test=test_serial2 -- ${CMAKE_CURRENT_SOURCE_DIR}/2012_12_21_antagent1.usbmon)
