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


#include <ctime>
#include <string>
#include <vector>
#include <map>

namespace antpm {

// struct DatabaseEntity
// {
//   std::string path;
//   ushort fileIdx;
//   size_t bytes;
// };

// <fileIdx, <path, bytes> >
typedef std::pair<std::string, size_t> FITEntity;
typedef std::multimap<uint16_t, FITEntity> Database;
typedef std::pair<uint16_t, FITEntity> DatabaseEntity;

class DeviceSettings
{
public:
  DeviceSettings(const char* devId);

  void loadDefaultValues();
  const std::string getConfigFileName() const;
  const std::string getFolder() const;
  //std::vector<DatabaseEntity> getDatabaseFiles(size_t count) const;
  Database getDatabaseFiles(size_t count = 0) const;
  bool saveToFile(const char* fname);
  bool saveToFile(const std::string& fname) { return saveToFile(fname.c_str()); }
  bool loadFromFile(const char* fname);
  bool loadFromFile(const std::string& fname) { return loadFromFile(fname.c_str()); }
  void mergeLastUserProfileTime(const std::time_t gmt);
  void mergeLastTransferredTime(const std::time_t gmt);

  static std::time_t str2time(const char* from);
  static const std::string time2str(const std::time_t t);
  static std::vector<std::string> getDatabases(const char* root = nullptr);

  unsigned int MaxFileDownloads;
  std::time_t  LastUserProfileTime; // date of the latest activity successfully downloaded fromt the device, represented as GMT/UTC
  std::time_t  LastTransferredTime; // last timepoint, communication happened with the device, represented as GMT/UTC
  size_t       SerialWriteDelayMs;
private:
  std::string mDevId;
};

}
