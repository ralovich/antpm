# -*- mode: cmake; coding: utf-8-dos -*-



INCLUDE_DIRECTORIES(.)

SET(SRCS
  byte_util.c
  command.c
  config.h
  datatype.c
  garmin.h
  pack.c
  packet_id.c
  print.c
  protocol.c
  run.c
  scan.c
  symbol_name.c
  unpack.c
  usb_comm.c
)

ADD_LIBRARY(garmintools
  ${SRCS}
)
