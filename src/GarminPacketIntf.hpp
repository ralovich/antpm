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

extern "C" {
#include "garmintools/garmin.h" // garmin-forerunner-tools
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
static_assert(sizeof(GarminPacket)==13);

struct GarminPacketIntf
{
  bool interpret(int lastPid, std::vector<uint8_t> data);
  int  interpretPid(std::vector<uint8_t> &data); // used during off-line analysis
  int  interpretPid(uint64_t& data); // used for on-line interpretation

public:
  bool parseStrings(const std::vector<uint8_t> &data, const size_t skip, std::vector<std::string> &out);
};

}

