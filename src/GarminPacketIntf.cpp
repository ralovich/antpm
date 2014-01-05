// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
////////////////////////////////////////////////////////////////////
// Copyright (c) 2011-2014 RALOVICH, Kristóf                      //
//                                                                //
// This program is free software; you can redistribute it and/or  //
// modify it under the terms of the GNU General Public License    //
// version 2 as published by the Free Software Foundation.        //
//                                                                //
////////////////////////////////////////////////////////////////////
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
  sstr << "0x" << toString<int>((int)gp->mPacketType, 2, '0') << endl;
  sstr << (int)gp->mPacketId << endl;
  sstr << (int)gp->mDataSize << endl;

  switch(gp->mPacketType)
  {

  }

  sstr << "---------------\n\n\n";

  cout << sstr.str();

  return true;
}

}
