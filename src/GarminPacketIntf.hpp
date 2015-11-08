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

#include "antdefs.hpp"
#include <vector>
#include <string>
#include <boost/static_assert.hpp>

extern "C" {
#include <garmin.h> // garmin-forerunner-tools
}

namespace antpm {

// create packets to send to device (to ask for tracks, etc.)

// interpret incoming messages from stream of bytes

// tasks:
// - interpter usbmon logs
// - download runs

#pragma pack(push,1)
struct GarminPacket
{
  uint8_t  mPacketType;
  uint8_t  mReserved123[3];
  uint16_t mPacketId;
  uint8_t  mReserved67[2];
  uint32_t mDataSize;
  uint8_t  mData[1];
  const std::string toString() const;
  const std::string toString8() const;
};
#pragma pack(pop)
BOOST_STATIC_ASSERT(sizeof(GarminPacket)==13);

struct GarminPacketIntf
{
  std::vector<uchar> bytes;
  std::vector<std::string> protos;

  bool interpret(const int lastPid);
  bool interpret(const int lastPid, std::vector<uint8_t> data);

  bool tryDecodeDirect(const int lastPid, std::vector<uint8_t>& burstData);

  bool saveToFile(const char* fileName = "antfs.bin");
  bool saveToFile(std::string folder, uint64_t code);

  int interpretPid(std::vector<uint8_t>& data);
  bool parseStrings(const std::vector<uint8_t> &data, const size_t skip, std::vector<std::string> &out);
};

}

