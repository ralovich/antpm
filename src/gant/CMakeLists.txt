# -*- mode: cmake; coding: utf-8-dos -*-


FIND_PACKAGE(LibXml2)
IF(NOT LIBXML2_FOUND)
  MESSAGE(FATAL_ERROR "libxml2 not found!")
ENDIF()

INCLUDE_DIRECTORIES(.)
INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})

SET(SRCS
  antdefs.h
  antlib.c
  antlib.h
  gant.c
)

ADD_EXECUTABLE(gant
  ${SRCS}
)
TARGET_LINK_LIBRARIES(gant
  m
  pthread
  ${LIBXML2_LIBRARIES}
)
INSTALL(TARGETS gant RUNTIME DESTINATION bin)
