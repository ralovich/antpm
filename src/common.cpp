// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2011-2014 RALOVICH, Kristóf                            //
//                                                                      //
// This program is free software; you can redistribute it and/or modify //
// it under the terms of the GNU General Public License as published by //
// the Free Software Foundation; either version 3 of the License, or    //
// (at your option) any later version.                                  //
//                                                                      //
// This program is distributed in the hope that it will be useful,      //
// but WITHOUT ANY WARRANTY; without even the implied warranty of       //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        //
// GNU General Public License for more details.                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
// ***** END LICENSE BLOCK *****


#include "common.hpp"
#include <cstring>
#include <iostream>
#include <iomanip> // setw
#include <sstream>
#include <thread>
#include "Log.hpp"
#include <filesystem>

namespace antpm {

//std::ostream&
//logger()
//{
//  //antpm::Log::instance()->
//  std::cout.flush();
//  return std::cout;
//}

// FILE*
// loggerc()
// {
//   fflush(stdout);
//   return stdout;
// }


void
sleepms(const size_t timeout_ms)
{
  using namespace std::chrono_literals;
  auto td = timeout_ms*1ms;
  std::this_thread::sleep_for(td);
}


#ifndef _MSC_VER
/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char*
itoa(int value, char* result, int base)
{
  // check that the base if valid
  if (base < 2 || base > 36) { *result = '\0'; return result; }

  char* ptr = result, *ptr1 = result, tmp_char;
  int tmp_value;

  do {
    tmp_value = value;
    value /= base;
    *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
  } while ( value );

  // Apply negative sign
  if (tmp_value < 0) *ptr++ = '-';
  *ptr-- = '\0';
  while(ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
  }
  return result;
}
#endif


template < class T >
const std::string
toString(const T& val, const int width, const char fill)
{
  std::stringstream clStr;
  if(width != -1) clStr << std::setw(width);
  if(fill != ' ') clStr << std::setfill(fill);
  clStr << std::hex;
  clStr << val;
  return clStr.str();
}


template < class T >
const std::string
toStringDec(const T& val, const int width, const char fill)
{
  std::stringstream clStr;
  if(width != -1) clStr << std::setw(width);
  if(fill != ' ') clStr << std::setfill(fill);
  clStr << val;
  return clStr.str();
}

std::vector<std::string> split(const std::string& s, char delimiter)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter))
  {
    tokens.push_back(token);
  }
  return tokens;
}

bool ends_with(const std::string& str, const std::string& ending)
{
    if (ending.size() > str.size())
        return false;
    return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}

const std::string
getDateString()
{
  //QString filename = QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss");
  //QByteArray ba=filename.toLatin1();
  //return ba.constData();
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);

  std::stringstream ss;
  ss << std::put_time(&tm, "%Y_%m_%d_%H_%M_%S");
  return ss.str();
}


const std::string
getConfigFileName()
{
  return getConfigFolder() + "/config.ini";
}


const std::string
getConfigFolder()
{
  const char* e0 = getenv("ANTPM_DIR");
  if(e0 && strlen(e0) > 0)
    return std::string(e0) + "/";

  // http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
  // $XDG_CONFIG_HOME defines the base directory relative to which user specific configuration files should be stored.
  // If $XDG_CONFIG_HOME is either not set or empty, a default equal to $HOME/.config should be used. 

#ifndef _WIN32
  const char* e1 = getenv("XDG_CONFIG_HOME");
  const char* e2 = getenv("HOME");
  if(e1 && strlen(e1) > 0)
    return std::string(e1) + "/" APP_NAME "/";
  else if(e2 && strlen(e2) > 0)
    return std::string(e2) + "/.config/" APP_NAME "/";
#else
  const char* e3 = getenv("USERPROFILE");
  if(e3 && strlen(e3) > 0)
    return std::string(e3) + "/.config/" APP_NAME "/";
#endif
  else
    return "~/.config/" APP_NAME "/";
}



void
readUInt64(const unsigned int clientSN, uint64_t& pairedKey)
{
  std::stringstream ss;
  ss << getConfigFolder() << "libantpmauth_" << clientSN;
  FILE* f=fopen(ss.str().c_str(), "r");
  LOG_VAR3(ss.str(), f, pairedKey);
  if(f)
  {
    if(1 != fread(&pairedKey, sizeof(pairedKey), 1, f))
      pairedKey = 0;
    fclose(f);
  }
}

void
writeUInt64(const unsigned int clientSN, const uint64_t& pairedKey)
{
  std::stringstream ss;
  ss << getConfigFolder() << "libantpmauth_" << clientSN;
  FILE* f=fopen(ss.str().c_str(), "w+");
  LOG_VAR3(ss.str(), f, pairedKey);
  if(f)
  {
    fwrite(&pairedKey, sizeof(pairedKey), 1, f);
    fclose(f);
  }
}

uint64_t
SwapDWord(uint64_t a)
{
  a = ((a & 0x00000000000000FFULL) << 56) |
      ((a & 0x000000000000FF00ULL) << 40) |
      ((a & 0x0000000000FF0000ULL) << 24) |
      ((a & 0x00000000FF000000ULL) <<  8) |
      ((a & 0x000000FF00000000ULL) >>  8) |
      ((a & 0x0000FF0000000000ULL) >> 24) |
      ((a & 0x00FF000000000000ULL) >> 40) |
      ((a & 0xFF00000000000000ULL) >> 56);
  return a;
}



std::vector<unsigned char>
readFile(const char* fileName)
{
  std::vector<unsigned char> v;
  FILE* f=fopen(fileName, "rb");
  if(f)
  {
    unsigned char buf[256];
    size_t lastRead=0;
    do
    {
      lastRead=fread(buf, 1, 256, f);
      v.insert(v.end(), buf, buf+lastRead);
    } while(lastRead>0);
    fclose(f);
  }
  return v;
}


bool
mkDir(const char* dirName)
{
  try
  {
    std::filesystem::path ddir(dirName);
    LOG(LOG_DBG) << "mkDir: \"" << ddir << "\"\n";
    return std::filesystem::create_directories(ddir);
  }
  // Throws:  basic_filesystem_error<Path> if exists(p) && !is_directory(p)
  catch(std::filesystem::filesystem_error& bfe)
  {
    LOG(LOG_WARN) << "mkDir: failed\n"
                  << "\twhat  " << bfe.what() << "\n"
                  << "\tpath1 =" << bfe.path1() << "\n"
                  << "\tpath2 =" << bfe.path2() << "\n";
    return false;
  }
}


bool
mkDirNoLog(const char* dirName)
{
  try
  {
    std::filesystem::path ddir(dirName);
    return std::filesystem::create_directories(ddir);
  }
  // Throws:  basic_filesystem_error<Path> if exists(p) && !is_directory(p)
  catch(std::filesystem::filesystem_error& bfe)
  {
    std::cerr << "mkDir: failed\n"
              << "\twhat  " << bfe.what() << "\n"
              << "\tpath1 =" << bfe.path1() << "\n"
              << "\tpath2 =" << bfe.path2() << "\n";
    return false;
  }
}


bool
folderExists(const char* dirName)
{
  std::filesystem::path ddir(dirName);
  return std::filesystem::exists(ddir) && std::filesystem::is_directory(ddir);
}

const std::string
getVersionString()
{
  unsigned char arr[2] = {0x01, 0x00};
  unsigned short int x = *(unsigned short int *) arr;
  bool little_endian = x == 1;

//
// Helper macro STRINGIZE:
// Converts the parameter X to a string after macro replacement
// on X has been performed.
//
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

  return std::string("") + APP_NAME
      + " v" + std::string(STRINGIZE(ANTPM_VERSION))
      + " built "
      + " under "
#ifdef __linux__
# ifdef __LP64__
  "linux64"
# else
  "linux32"
# endif
#elif defined(_WIN64)
  "win64"
#elif defined(_WIN32)
  "win32"
#elif defined(__APPLE__)
  "macos"
#else
  "unknown_os"
#endif
      + " with "
#ifdef __GNUC__
# if defined(__clang__)
  "CLANG " __VERSION__
# else
  "GCC " __VERSION__
# endif
#elif defined(_MSC_VER)
  + std::string("MSVC " STRINGIZE(_MSC_FULL_VER))
#else
  "unknow_compiler"
#endif
      + (little_endian ? std::string(" LE") : std::string(" BE"));
#undef DO_STRINGIZE
#undef STRINGIZE
}


bool
isAntpm405Override()
{
  char* ANTPM_405 = getenv("ANTPM_405");
  if(ANTPM_405!=NULL && strncmp("1",ANTPM_405,1)==0)
  {
    return true;
  }
  return false;
}

















// explicit template instantiation

template const std::string toString(const int& val, const int width, const char fill);
template const std::string toString(const char& val, const int width, const char fill);
template const std::string toString(const unsigned char& val, const int width, const char fill);
template const std::string toString(const unsigned int& val, const int width, const char fill);
template const std::string toString(const unsigned short& val, const int width, const char fill);
template const std::string toString(const uint64_t& val, const int width, const char fill);


template const std::string toStringDec(const int& val, const int width, const char fill);
template const std::string toStringDec(const unsigned long& val, const int width, const char fill);
template const std::string toStringDec(const double& val, const int width, const char fill);
template const std::string toStringDec(const unsigned int& val, const int width, const char fill);


#ifdef _WIN64
template const std::string toStringDec(const uint64_t& val, const int width, const char fill);
#endif

}
