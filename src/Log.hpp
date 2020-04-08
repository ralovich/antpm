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

#pragma once

#include "LazySingleton.hpp"
#include <cstdarg>
#include <fstream>
#include <list>
#include <memory>
#include <ostream>
#include <sstream>
#ifdef _MSC_VER
# include <crtdbg.h>
# include <io.h>
#endif
#if defined(__linux__) || defined(__GNU__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__)
# include <unistd.h>
#endif
#include <iostream>
#include <ctime>


namespace antpm
{
  enum LogLevel
  {
    LOG_RAW,             //< dont insert any prefix
    LOG_ERR,             //< runtime error
    LOG_WARN,            //< suppressable runtime error
    LOG_INF,             //< runtime information
    LOG_DBG,             //< debug info, mainly for developers
    LOG_DBG2,            //< more debug info
    LOG_DBG3             //< even more debug info (function trace, )
  };

#ifndef NDEBUG
# define psoLogMaxLogLevel antpm::LOG_DBG3
#else
# define psoLogMaxLogLevel antpm::LOG_DBG3
#endif

  inline
  const char*
  logLevelToString(const LogLevel& level)
  {
    switch(level)
    {
      case LOG_RAW:             return "";
      case LOG_ERR:             return "ERROR: ";
      case LOG_WARN:            return "WW: ";
      case LOG_INF:             return "II: ";
      case LOG_DBG:             return "DBG: ";
      case LOG_DBG2:            return "DBG: ";
      case LOG_DBG3:            return "DBG: ";
      default:                  return 0;
    }
  }

  inline
  std::ostream&
  operator<< (std::ostream& left, const LogLevel level)
  {
    return left << logLevelToString(level);
  }
  
  extern const std::string getVersionString();


  /**
   * Encapsulates a series of C++ style (<<) writes to a log in a
   * transactional manner.
   */
  template <typename T>
  class LogLine
  {
  public:
    inline LogLine(const LogLevel& level = LOG_INF) : _level(level) {}
    inline virtual             ~LogLine()           { T::instance()->print(_level, _oss.str()); }
    inline std::ostringstream& get()                { return _oss; }
  private:
    LogLine(const LogLine&);
    LogLine& operator= (const LogLine&);
  protected:
    const LogLevel     _level;
    std::ostringstream _oss;
  };

  /**
   *
   */
  class Log
    : public ClassInstantiator<Log>
    , public LazySingleton<Log, Log>
  {
  public:
    inline Log(const char* logFileName = nullptr);
    inline virtual ~Log();

#ifdef __GNUC__
    inline virtual int lprintf2(const LogLevel level,
                                const char* format,
                                ...) __attribute__ ((format (printf, 3, 4)));
#else // !__GNUC__
    inline virtual int lprintf2(const LogLevel level,
                                const char* format,
                                ...);
#endif // !__GNUC__
    inline virtual int vlprintf(const LogLevel level,
                                const char* format,
                                ::va_list args);
    inline virtual int print(const LogLevel level, const std::string& msg);

    inline virtual void addSink(std::ostream& os);
    inline virtual void delSink(std::ostream& os);

    inline virtual const LogLevel& getLogReportingLevel() const;
    inline virtual void            setLogReportingLevel(const LogLevel& logReportingLevel);
  protected:
    inline int writeStreams(const std::string& s);
  /**
   * getTimeStamp()
   */
  inline
  const std::string
  getTimeStamp()
  {
    ::time_t ltime;
    ::time(&ltime);
    char tmp[26];
    tmp[25] = '\0';
#ifdef _MSC_VER
    ::ctime_s(tmp, 26, &ltime);
#else
    ::ctime_r(&ltime, tmp);
#endif
    return std::string(tmp);
  }
  protected:
    typedef std::list<std::ostream*> SinkList;
    std::ofstream _ofs;
    SinkList      _sinks;
    LogLevel      _logReportingLevel;
  };

  Log::Log(const char* logFileName)
    : _logReportingLevel(psoLogMaxLogLevel)
  {
    std::ios_base::sync_with_stdio(false);

    if(!logFileName)
    {
      return;
    }

    // rotate previous log file
    if(::access(logFileName, 0x00) != -1)
    {
      std::string old = std::string(logFileName) + std::string(".old");
      ::remove(old.c_str());
      ::rename(logFileName, old.c_str());
    }

    _ofs.open(logFileName, std::ios::out | std::ios::trunc);
    if(!_ofs.is_open())
    {
      std::cerr << LOG_ERR << __FUNCTION__ << ": Unable to open log file \""
                << logFileName << "\" at " << getTimeStamp() << std::endl;
    }
    else
    {
      addSink(_ofs);

      std::string ts(getTimeStamp());
      this->lprintf2(LOG_INF,
                     "%s(): Log file \"%s\" opened at %s",
                     __FUNCTION__,
                     logFileName,
                     ts.c_str());
      std::string v(antpm::getVersionString());
      this->lprintf2(LOG_INF,
                     "%s\n",
                     v.c_str());
      this->lprintf2(LOG_RAW, "logging level: %d\n", this->_logReportingLevel);
    }
  }

  Log::~Log()
  {
    if(_ofs.is_open())
    {
      this->lprintf2(LOG_INF,
                     "%s(): Closing log file at %s",
                     __FUNCTION__,
                     getTimeStamp().c_str());
    }
  }

  int
  Log::lprintf2(const LogLevel level,
                const char* format,
                ...)
  {
    va_list args;
    va_start(args, format);
    int chars = this->vlprintf(level,
                               format,
                               args);
    va_end(args);
    return chars;
  }

  int
  Log::vlprintf(const LogLevel level,
                const char* format,
                ::va_list args)
  {
    std::string s(logLevelToString(level));

    char msg[1023+1];
    msg[1023] = '\0';
    vsnprintf(msg,
              1024,
              format,
              args);
    s.append(msg);

#ifdef __PSO_BUILD_WIN__
    _RPT0(_CRT_WARN, s.c_str());
#endif

    return writeStreams(s);
  }

  int
  Log::print(const LogLevel level, const std::string& msg)
  {
    const std::string s(std::string(logLevelToString(level)) + msg);

#ifdef _MSC_VER
    _RPT0(_CRT_WARN, s.c_str());
#endif

    return writeStreams(s);
  }

  void
  Log::addSink(std::ostream& os)
  {
    _sinks.push_back(&os);
    _sinks.unique();
  }

  void
  Log::delSink(std::ostream& os)
  {
    _sinks.remove(&os);
  }

  const LogLevel&
  Log::getLogReportingLevel() const
  {
    return _logReportingLevel;
  }

  void
  Log::setLogReportingLevel(const LogLevel& logReportingLevel)
  {
    _logReportingLevel = logReportingLevel;
    this->lprintf2(LOG_RAW, "logging level: %d\n", this->_logReportingLevel);
  }

  int
  Log::writeStreams(const std::string& s)
  {
//     static int c = 0;
//     c++;
//     if(!(c % 8))
//     {
//       c = 0;
//     }
    for(SinkList::iterator i = _sinks.begin(); i != _sinks.end(); i++)
    {
      (*i)->write(s.c_str(), s.length());
//       if(!c)
//       {
      (*i)->flush();
//       }
    }

    return (int)s.length();
  }

//  template<>
//  inline
//  Log*
//  ClassInstantiator<Log>::instantiate()
//  {
//    return new Log("antpm.txt");
//  }

//  template<>
//  template<>
//  inline
//  Log*
//  ClassInstantiator<Log>::instantiate<const char*>(const char* p1)
//  {
//    return new Log(p1);
//  }

  /**
   * Start a log line.
   */
#ifdef _MSC_VER
#define lprintf(level, format, ...)                                 \
  if(level > psoLogMaxLogLevel)                                     \
  {}                                                                \
  else if(level > antpm::Log::instance()->getLogReportingLevel())     \
  {}                                                                \
  else antpm::Log::instance()->lprintf2(level, format, __VA_ARGS__)
#else // GCC
#define lprintf(level, format, ...)                                 \
  if(level > psoLogMaxLogLevel)                                     \
  {}                                                                \
  else if(level > antpm::Log::instance()->getLogReportingLevel())     \
  {}                                                                \
  else antpm::Log::instance()->lprintf2(level, format, ##__VA_ARGS__)
#endif

#define LOG(level)                                             \
  if(level > psoLogMaxLogLevel)                                   \
  {}                                                              \
  else if(level > antpm::Log::instance()->getLogReportingLevel())   \
  {}                                                              \
  else antpm::LogLine<antpm::Log>(level).get()

  /**
   * Start a log line beginning with the name of the calling function.
   */
#define LOGT(level)                                              \
  if(level > psoLogMaxLogLevel)                                     \
  {}                                                                \
  else if(level > antpm::Log::instance()->getLogReportingLevel())     \
  {}                                                                \
  else antpm::LogLine<antpm::Log>(level).get()                          \
         << __FILE__ << ":" << __LINE__ << " "                      \
         << __FUNCTION__ << "(): "

//#define logt(level) psoLogt(level)

//#define log(level) psoLog(level)

}

#define DEFAULT_LOG_INSTANTIATOR                \
  namespace antpm {                             \
  template<>                                    \
  std::unique_ptr<Log>                          \
  ClassInstantiator<Log>::make_unique()         \
  {                                             \
    return std::make_unique<Log>();             \
  }                                             \
  }
    

