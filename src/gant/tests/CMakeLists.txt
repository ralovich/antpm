
IF(VALGRIND_PROGRAM)
  function(ADD_MY_TESTF name binary)
    #set(memcheck_command "${CMAKE_MEMORYCHECK_COMMAND} ${CMAKE_MEMORYCHECK_COMMAND_OPTIONS}")
    #separate_arguments(memcheck_command)
    add_test(NAME ${name} COMMAND ${binary} ${ARGN})
    set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)

    add_test(NAME memcheck_${name} COMMAND ${VALGRIND_PROGRAM} --leak-check=full ./${binary} ${ARGN})
    set_tests_properties(memcheck_${name} PROPERTIES WILL_FAIL TRUE)
  endfunction()

  function(set_memcheck_test_properties name)
    set_tests_properties(${name} ${ARGN})
    set_tests_properties(memcheck_${name} ${ARGN})
  endfunction()
ELSE()
  function(ADD_MY_TESTF name binary)
    add_test(${name} ${binary} ${ARGN})
    set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
  endfunction()
ENDIF()

ADD_MY_TESTF(testGant ../gant)

