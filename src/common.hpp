// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
////////////////////////////////////////////////////////////////////
// Copyright (c) 2011-2013 RALOVICH, Kristóf                      //
//                                                                //
// This program is free software; you can redistribute it and/or  //
// modify it under the terms of the GNU General Public License    //
// version 2 as published by the Free Software Foundation.        //
//                                                                //
////////////////////////////////////////////////////////////////////
// ***** END LICENSE BLOCK *****
#pragma once

#include <ostream>
#include <string>
#include <vector>
//#include "antdefs.hpp"
#include "Log.hpp"
#include "stdintfwd.hpp"

namespace antpm {

#define CHECK_RETURN_FALSE(x)         do { if(!(x)) { lprintf(antpm::LOG_ERR, "%s\n", #x); return false; } /*else { printf("ok: %s\n", #x); }*/ } while(0)
#define CHECK_RETURN_FALSE_LOG_OK(x)  do { if(!(x)) { lprintf(antpm::LOG_ERR, "%s\n", #x); return false; } else { printf("ok: %s\n", #x); } } while(0)
#define CHECK_RETURN_RV(x, rv)        do { if(!(x)) { lprintf(antpm::LOG_ERR, "%s\n", #x); return rv; } /*else { printf("ok: %s\n", #x); }*/ } while(0)
#define CHECK_RETURN_RV_LOG_OK(x, rv) do { if(!(x)) { lprintf(antpm::LOG_ERR, "%s\n", #x); return rv; } else { printf("ok: %s\n", #x); } } while(0)
#define CHECK_RETURN(x)               do { if(!(x)) { lprintf(antpm::LOG_ERR, "%s\n", #x); return; } else { printf("ok: %s\n", #x); } fflush(stdout); } while(0)
#define LOG_VAR(x) do { LOG(antpm::LOG_INF) << #x << "= " << x << std::endl; } while(0)
#define LOG_VAR2(x,y) do { LOG(antpm::LOG_INF) << #x << "= " << x << ", " #y << "= " << y << std::endl; } while(0)
#define LOG_VAR3(x,y, z) do { LOG(antpm::LOG_INF) << #x "= " << x << ", " #y "= " << y << ", " #z "= " << z << std::endl; } while(0)
#define LOG_VAR4(x,y, z, w) do { LOG(antpm::LOG_INF) << #x "= " << x << ", " #y "= " << y << ", " #z "= " << z << ", " #w "= " << w << std::endl; } while(0)

//std::ostream& logger();
#define logger() LOG(antpm::LOG_INF)
//FILE* loggerc();

void sleepms(const size_t timeout_ms);

#ifndef _MSC_VER
char* itoa(int value, char* result, int base);
#endif

///
template < class T >
const std::string
toString(const T& val, const int width = -1, const char fill = ' ');
template < class T >
const std::string
toStringDec(const T& val, const int width = -1, const char fill = ' ');

const std::vector<std::string> tokenize(const std::string& text, const char* delims);

extern
const std::string
getDateString();

const std::string
getConfigFileName();

const std::string
getConfigFolder();

void
readUInt64(const unsigned int clientSN, uint64_t& pairedKey);

void
writeUInt64(const unsigned int clientSN, const uint64_t& ui);

std::vector<unsigned char>
readFile(const char* fileName);

bool
mkDir(const char* dirName);

bool
mkDirNoLog(const char* dirName);

bool
folderExists(const char* dirName);

const std::string
getVersionString();

enum
{
  ANTPM_RETRIES=10,
  ANTPM_RETRY_MS=1000,
  ANTPM_MAX_CHANNELS=56
};

#define APP_NAME "antpm"
#ifndef _WIN32
# define ANTPM_SERIAL_IMPL SerialTty
#else
# define ANTPM_SERIAL_IMPL SerialUsb
#endif

}
