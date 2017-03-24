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


#include "DeviceSettings.hpp"
#include <ctime>
#include <cstring> // memset
#include "common.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Log.hpp"

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace fs=boost::filesystem;

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
  SerialWriteDelayMs = 3;
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

// std::vector<DatabaseEntity>
// DeviceSettings::getDatabaseFiles(size_t count) const
// {
//   std::vector<DatabaseEntity> files;
//   std::string root = getFolder();
//   // for all folders
//   //   for all fit files
//   //
//   return files;
// }

Database
DeviceSettings::getDatabaseFiles(size_t count) const
{
  Database files;
  std::string root = getFolder();
  // for all folders
  //   for all fit files
  //

  fs::path p(root);

  if(!fs::is_directory(p))
  {
    return files;
  }
  //std::cout << p << " is a directory containing:\n";

  for(auto& entry : boost::make_iterator_range(fs::directory_iterator(p), {}))
  {
    if(!fs::is_directory(entry))
    {
      continue;
    }
    //std::cout << entry << "\n";
    for(auto& fit : boost::make_iterator_range(fs::directory_iterator(entry), {}))
    {
      if(boost::algorithm::ends_with(fit.path().string(), ".fit"))
      {
        int value=0;
        sscanf(fit.path().stem().c_str(), "%x", &value);
        //std::cout << "\t" << fit.path().stem() << std::endl;
        if(value == 0) // skip directory file
        {
          continue;
        }
        // std::cout << "\t0x" << toString<uint16_t>(value,4,'0') << " "
        //           << fs::file_size(fit.path()) << " " << fit.path().string() << std::endl;
        files.insert(DatabaseEntity(static_cast<uint16_t>(value),
                                    FITEntity(fit.path().string(), fs::file_size(fit.path()))));
      }
    }
  }
  
  return files;
}

/// Both input and output are represented in GMT/UTC.
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
  std::string froms(from);
  std::replace( froms.begin(), froms.end(), 'T', ' ');
  std::replace( froms.begin(), froms.end(), 'Z', '.');
  froms += "000";
  boost::posix_time::ptime t(boost::posix_time::time_from_string(froms));
  tm = boost::posix_time::to_tm( t );
#endif
  std::time_t myt  = ::mktime(&tm);
  std::time_t mytz = timezone;
  return myt - mytz;
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

#ifdef _MSC_VER
  if(::strftime(outstr, sizeof(outstr), "%Y-%m-%dT%H:%M:%SZ", &tm) == 0)
    return "";
#else
  if(::strftime(outstr, sizeof(outstr), "%Y-%m-%dT%TZ", &tm) == 0)
    return "";
#endif
  return outstr;
}


bool is_number(const std::string& s)
{
  return !s.empty() && std::find_if(s.begin(), 
                                    s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}


std::vector<std::string>
DeviceSettings::getDatabases(const char* root)
{
  std::string root_folder = root ? root : getConfigFolder();
  std::vector<std::string> devices;

  fs::path p(root_folder);

  if(!fs::is_directory(p))
  {
    return devices;
  }

  for(auto& entry : boost::make_iterator_range(fs::directory_iterator(p), {}))
  {
    entry.path();
    
    if(!fs::is_directory(entry) || !is_number(entry.path().stem().string()))
    {
      continue;
    }
    //std::cout << entry.path().stem() << "\n";
    devices.push_back(entry.path().stem().string());
  }
  
  return devices;
}

bool
DeviceSettings::saveToFile(const char *fname)
{
  boost::property_tree::ptree pt;
  pt.put("antpm.MaxFileDownloads", MaxFileDownloads);
  pt.put("antpm.LastUserProfileTime", time2str(LastUserProfileTime));
  pt.put("antpm.LastTransferredTime", time2str(LastTransferredTime));
  pt.put("antpm.SerialWriteDelayMs", SerialWriteDelayMs);
  try
  {
    boost::property_tree::ini_parser::write_ini(fname, pt);
  }
  catch(boost::property_tree::ini_parser_error& ipe)
  {
    LOG(LOG_WARN) << "DeviceSettings::saveToFile: " << ipe.message() << std::endl;
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
    LOG(LOG_WARN) << "DeviceSettings::loadFromFile: " << ipe.message() << std::endl;
    return false;
  }

  LOG_VAR(pt.get<unsigned int>("antpm.MaxFileDownloads"));
  LOG_VAR(pt.get<std::string>("antpm.LastUserProfileTime"));
  LOG_VAR(pt.get<std::string>("antpm.LastTransferredTime"));
  LOG_VAR(pt.get<size_t>("antpm.SerialWriteDelayMs"));
  MaxFileDownloads    = pt.get<unsigned int>("antpm.MaxFileDownloads");
  LastUserProfileTime = str2time(pt.get<std::string>("antpm.LastUserProfileTime").c_str());
  LastTransferredTime = str2time(pt.get<std::string>("antpm.LastTransferredTime").c_str());
  SerialWriteDelayMs = pt.get<size_t>("antpm.SerialWriteDelayMs");
  return true;
}

///
/// \param t expected to be represented as local time
void DeviceSettings::mergeLastUserProfileTime(const std::time_t gmt)
{
  //std::time_t gmt = t + timezone;
  LOG(LOG_DBG2) << "LastUserProfileTime: " << time2str(LastUserProfileTime) << " => " << time2str(gmt) << "\n";
  LOG(LOG_DBG2) << "LastUserProfileTime: " << LastUserProfileTime << " => " << gmt << "\n";
  if(gmt > LastUserProfileTime)
  {
    LastUserProfileTime = gmt;
  }
}

///
/// \param t expected to be represented as local time
void DeviceSettings::mergeLastTransferredTime(const std::time_t gmt)
{
  LOG(LOG_DBG2) << "LastTransferredTime: " << time2str(LastTransferredTime) << " => " << time2str(gmt) << "\n";
  //std::time_t gmt = t + timezone;
  LastTransferredTime = std::max(LastTransferredTime, gmt);
}

}
