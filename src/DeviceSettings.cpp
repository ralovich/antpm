// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
////////////////////////////////////////////////////////////////////
//                                                                //
// Copyright (c) 2011-2013 RALOVICH, Kristóf                      //
//                                                                //
// This program is free software; you can redistribute it and/or  //
// modify it under the terms of the GNU General Public License    //
// version 2 as published by the Free Software Foundation.        //
//                                                                //
////////////////////////////////////////////////////////////////////
// ***** END LICENSE BLOCK *****


#include "DeviceSettings.hpp"
#include <ctime>
#include <cstring> // memset
#include "common.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Log.hpp"


namespace antpm {


DeviceSettings::DeviceSettings(const char *devId)
  : mDevId(devId)
{
  loadDefaultValues();
}

void
DeviceSettings::loadDefaultValues()
{
  MaxFileDownloads = 1000;
  struct tm y2k; // 2000-01-01T00:00:00Z 946684800
  y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
  y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;
  y2k.tm_isdst = -1;
  LastUserProfileTime = ::mktime(&y2k) - timezone;
  LastTransferredTime = ::mktime(&y2k) - timezone;
}

const std::string
DeviceSettings::getConfigFileName() const
{
  return getFolder()  + "/config.ini";
}


const std::string
DeviceSettings::getFolder() const
{
  return getConfigFolder() + "/" + mDevId + "/";
}

/// Both inpout and output are represented in GMT/UTC.
std::time_t
DeviceSettings::str2time(const char* from)
{
  if(!from)
    return 0;
  struct tm tm;
  memset(&tm, 0, sizeof(struct tm));
  //strptime("2001-11-12 18:31:01", "%Y-%m-%d %H:%M:%S", &tm);
#ifndef _WIN32
  char* rv = ::strptime(from, "%Y-%m-%dT%H:%M:%SZ", &tm);
  bool ok = rv==from+::strlen(from);
  if(!ok)
    return 0;
#else
  //std::string ts("2002-01-20 23:59:59.000");
  boost::posix_time::ptime t(boost::posix_time::time_from_string(from));
  tm = boost::posix_time::to_tm( t );
#endif
  return ::mktime(&tm) - timezone;
}

/// Both input and output are represented in GMT/UTC.
const
std::string
DeviceSettings::time2str(const std::time_t t)
{
  char outstr[256];
  memset(outstr, 0, sizeof(outstr));
  struct tm tm;
  memset(&tm, 0, sizeof(struct tm));
#ifndef _WIN32
  ::gmtime_r(&t, &tm);
#else
  gmtime_s(&tm, &t);
#endif

  if(::strftime(outstr, sizeof(outstr), "%Y-%m-%dT%TZ", &tm) == 0)
    return "";

  return outstr;
}

bool
DeviceSettings::saveToFile(const char *fname)
{
  boost::property_tree::ptree pt;
  pt.put("antpm.MaxFileDownloads", MaxFileDownloads);
  pt.put("antpm.LastUserProfileTime", time2str(LastUserProfileTime));
  pt.put("antpm.LastTransferredTime", time2str(LastTransferredTime));
  try
  {
    boost::property_tree::ini_parser::write_ini(fname, pt);
  }
  catch(boost::property_tree::ini_parser_error& ipe)
  {
    LOG(LOG_WARN) << ipe.message() << std::endl;
    return false;
  }
  return true;
}

bool DeviceSettings::loadFromFile(const char *fname)
{
  boost::property_tree::ptree pt;
  try
  {
    boost::property_tree::ini_parser::read_ini(fname, pt);
  }
  catch(boost::property_tree::ini_parser_error& ipe)
  {
    LOG(LOG_WARN) << ipe.message() << std::endl;
    return false;
  }

  LOG_VAR(pt.get<unsigned int>("antpm.MaxFileDownloads"));
  LOG_VAR(pt.get<std::string>("antpm.LastUserProfileTime"));
  LOG_VAR(pt.get<std::string>("antpm.LastTransferredTime"));
  MaxFileDownloads    = pt.get<unsigned int>("antpm.MaxFileDownloads");
  LastUserProfileTime = str2time(pt.get<std::string>("antpm.LastUserProfileTime").c_str());
  LastTransferredTime = str2time(pt.get<std::string>("antpm.LastTransferredTime").c_str());
  return true;
}

///
/// \param t expected to be represented as local time
void DeviceSettings::mergeLastUserProfileTime(const std::time_t gmt)
{
  //std::time_t gmt = t + timezone;
  LOG(LOG_DBG) << "LastUserProfileTime: " << time2str(LastUserProfileTime) << " => " << time2str(gmt) << "\n";
  if(gmt > LastUserProfileTime)
  {
    LastUserProfileTime = gmt;
  }
}

///
/// \param t expected to be represented as local time
void DeviceSettings::mergeLastTransferredTime(const std::time_t gmt)
{
  LOG(LOG_DBG) << "LastTransferredTime: " << time2str(LastTransferredTime) << " => " << time2str(gmt) << "\n";
  //std::time_t gmt = t + timezone;
  LastTransferredTime = std::max(LastTransferredTime, gmt);
}

}
