# -*- mode: cmake; coding: utf-8-dos -*-



INCLUDE_DIRECTORIES(.)

SET(SRCS
  garmin.h
  symbol_name.c
)

ADD_LIBRARY(garmintools
  ${SRCS}
)
