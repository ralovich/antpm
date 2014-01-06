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

#include "GarminPacketIntf.hpp"
#include "common.hpp"
//#include <iostream>

using namespace std;


namespace antpm {

bool
GarminPacketIntf::interpret(std::vector<uint8_t> data)
{
  size_t off = 0;
  CHECK_RETURN_FALSE(data.size()>=8);

  GarminPacket* gp(reinterpret_cast<GarminPacket*>(&data[off]));

  std::stringstream sstr;

  sstr << "\n\n_______________\n";
  if(data.size()==8)
  {
    // check for known commands
    switch(*reinterpret_cast<uint64_t*>(&data[0]))
    {
    case BSWAP_64(0xfe00000000000000):
      //break;
    case BSWAP_64(0x06000200ff000000):
      //break;
    case BSWAP_64(0x06000200f8000000):
      sstr << "Known direct command, OK\n";
      break;
    default:
      sstr << "unknown cmd 0x" << toString(SwapDWord(*reinterpret_cast<uint64_t*>(&data[0])), 16, '0') << "\n";
      break;
    }
  }
  else
  {
    // probably response
    sstr << "packet_type=0x" << toString<int>((int)gp->mPacketType, 2, '0') << "=" << (int)gp->mPacketType << endl;
    sstr << "pid=" << (int)gp->mPacketId << endl;
    sstr << "data_size=" << (int)gp->mDataSize << endl;
    sstr << "max  size=" << data.size()-off-12 << endl;

    switch(gp->mPacketType)
    {

    }
  }
  sstr << "---------------\n\n\n";

  cout << sstr.str();

  return true;
}

}
