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


#include <ctime>
#include <string>

namespace antpm {

class DeviceSettings
{
public:
  DeviceSettings(const char* devId);

  void loadDefaultValues();
  const std::string getConfigFileName() const;
  const std::string getFolder() const;
  bool saveToFile(const char* fname);
  bool loadFromFile(const char* fname);
  void mergeLastUserProfileTime(const std::time_t gmt);
  void mergeLastTransferredTime(const std::time_t gmt);

  static std::time_t str2time(const char* from);
  static const std::string time2str(const std::time_t t);

  uint        MaxFileDownloads;
  std::time_t LastUserProfileTime; // date of the latest activity successfully downloaded fromt the device, represented as GMT/UTC
  std::time_t LastTransferredTime; // last timepoint, communication happened with the device, represented as GMT/UTC
private:
  std::string mDevId;
};

}
