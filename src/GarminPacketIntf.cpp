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
#include "garmintools/garmin.h"
#include <cstring>
#include <set>

using namespace std;


namespace antpm {


const char* garmin_pid_name ( int s );
const char* A010_cmnd_name ( int s );


bool
GarminPacketIntf::interpret(int lastPid, std::vector<uint8_t> data)
{
  size_t off = 0;
  CHECK_RETURN_FALSE(data.size()>=8);

  std::stringstream sstr;
  sstr << "\n\n_______________\n";

  if(data.size()==8 || lastPid==-1)
  {
    // check for known commands
    switch(*reinterpret_cast<uint64_t*>(&data[0]))
    {
    case BSWAP_64(0xfe00000000000000):
      sstr << "L000_Pid_Product_Rqst, OK\n";
      break;
    case BSWAP_64(0x06000200ff000000):
    {
      GarminPacket* gp(reinterpret_cast<GarminPacket*>(&data[off]));
      sstr << "Known direct command, OK\n";
      sstr << gp->toString8() << "\n";
      break;
    }
    case BSWAP_64(0x06000200f8000000):
    {
      GarminPacket* gp(reinterpret_cast<GarminPacket*>(&data[off]));
      sstr << "Known direct command, OK\n";
      sstr << gp->toString8() << "\n";
      break;
    }
    default:
      sstr << "unknown cmd 0x" << toString(SwapDWord(*reinterpret_cast<uint64_t*>(&data[0])), 16, '0') << "\n";
      GarminPacket* gp(reinterpret_cast<GarminPacket*>(&data[off]));
      sstr << gp->toString8() << "\n";
      break;
    }
  }
  else
  {
    // probably response
    CHECK_RETURN_FALSE(data.size()>=8);

    switch(lastPid)
    {
    case L000_Pid_Product_Rqst:
    {
      vector<string> strings;
      if(parseStrings(data, 8, strings))
      {
        sstr << "Found " << strings.size() << " strings\n";
        for(size_t i = 0; i < strings.size(); i++)
          sstr << "\t\"" << strings[i] << "\"\n";
      }
      break;
    }
    case L000_Pid_Product_Data:
    {
#pragma pack(push,1)
      typedef struct
      {
        uint16_t productId;
        int16_t  swVersion;
      } ProductDataType;
#pragma pack(pop)
      BOOST_STATIC_ASSERT(sizeof(ProductDataType)==4);
      typedef struct
      {
        uint16_t productId;
        int16_t  swVersion;
        vector<string> strings;
      } ProductDataType2;
      ProductDataType* pdt = reinterpret_cast<ProductDataType*>(&data[0]);
      ProductDataType2 pdt2;
      pdt2.productId = pdt->productId;
      pdt2.swVersion = pdt->swVersion;
      if(parseStrings(data, 4, pdt2.strings))
      {
        sstr << "productId=0x" << toString((int)pdt2.productId, 4, '0') << ", swVer=0x" << toString((int)pdt2.swVersion, 4, '0') << "\n";
        sstr << "Found " << pdt2.strings.size() << " strings\n";
        for(size_t i = 0; i < pdt2.strings.size(); i++)
          sstr << "\t\"" << pdt2.strings[i] << "\"\n";
      }
      break;
    }
    case L000_Pid_Ext_Product_Data:
    {
      // parse A001
#pragma pack(push,1)
      typedef struct
      {
        uint8_t  tag;
        uint16_t data;
      } ProtocolDataType;
#pragma pack(pop)
      BOOST_STATIC_ASSERT(sizeof(ProtocolDataType)==3);
      sstr << "Found following protocol capabilities:\n";
      set<string> caps;
      for(size_t i = 4; i+2 < data.size(); i+=3)
      {
        ProtocolDataType* prota = reinterpret_cast<ProtocolDataType*>(&data[i]);
        if(prota->tag==Tag_Phys_Prot_Id || prota->tag==Tag_Link_Prot_Id || prota->tag==Tag_Appl_Prot_Id || prota->tag==Tag_Data_Type_Id)
        {
          sstr << "tag=0x" << toString((int)prota->tag) << "=" << prota->tag << ", data=0x" << toString(prota->data,4,'0') << "=" << prota->data << endl;
          std::stringstream sstr2;
          sstr2 << prota->tag << prota->data;
          caps.insert(sstr2.str());
        }
        else
          continue;
      }
      for(set<string>::iterator i = caps.begin(); i!= caps.end(); i++)
      {
        sstr << *i << endl;
      }
      break;
    }
    default:
    {
      sstr << "Probably reply ...\n";
      GarminPacket* gp(reinterpret_cast<GarminPacket*>(&data[off]));
      //sstr << "packet_type=0x" << toString<int>((int)gp->mPacketType, 2, '0') << "=" << (int)gp->mPacketType << endl;
      //sstr << "pid=" << (int)gp->mPacketId << endl;
      //sstr << "data_size=" << (int)gp->mDataSize << endl;
      sstr << gp->toString();
      sstr << "max  size=" << data.size()-off-12 << endl;
      break;
    }
    }


  }
  sstr << "---------------\n\n\n";

  LOG(LOG_INF) << sstr.str();

  return true;
}

int
GarminPacketIntf::interpretPid(std::vector<uint8_t>& data)
{
  if(data.size()<8) return -1;
  // check for known commands
  switch(*reinterpret_cast<uint64_t*>(&data[0]))
  {
  case BSWAP_64(0xfe00000000000000):
    return L000_Pid_Product_Rqst;
  case BSWAP_64(0x06000200ff000000):
    return L000_Pid_Product_Data;
  case BSWAP_64(0x06000200f8000000):
    return L000_Pid_Ext_Product_Data; // starts A000
  case BSWAP_64(0x0a000200c2010000):
    return A010_Cmnd_Transfer_Runs;
  default:
    return -1;
  }
}

int GarminPacketIntf::interpretPid(uint64_t& data)
{
  switch(data)
  {
  case 0xfe00000000000000:
    return L000_Pid_Product_Rqst;
  case 0x06000200ff000000:
    return L000_Pid_Product_Data;
  case 0x06000200f8000000:
    return L000_Pid_Ext_Product_Data; // starts A000
  case 0x0a000200c2010000:
    return A010_Cmnd_Transfer_Runs;
  case 0x060002001b000000:
    return L001_Pid_Records;
  default:
    return -1;
  }
}


bool
GarminPacketIntf::parseStrings(const std::vector<uint8_t>& data, const size_t skip, std::vector<std::string>& out)
{
  size_t start=skip;
  size_t end = skip;
  for(size_t i = skip; i < data.size(); i++)
  {
    if(data[i]==0)
    {
      end = i;
      if(start<end)
      {
        string s(reinterpret_cast<const char*>(&data[start]), end-start);
        out.push_back(s);
      }
      start = i+1;
    }
  }
  return true;
}


const std::string
GarminPacket::toString() const
{
  std::stringstream sstr;
  sstr << "packet_type=0x" << antpm::toString<int>((int)mPacketType, 2, '0') << "=" << (int)mPacketType;
  sstr << " pid=0x" << antpm::toString<int>((int)mPacketId, 4, '0') << "=" << (int)mPacketId << " " << garmin_pid_name((int)mPacketId);
  if(mPacketType == appl_A010)
    sstr << " " << A010_cmnd_name(mPacketId);
  sstr << " data_size=0x" << antpm::toString<uint32_t>((uint32_t)mDataSize, 8, '0') << "=" << (uint32_t)mDataSize;
  //sstr << "max  size=" << data.size()-off-12 << endl;
  return sstr.str();
}


const std::string
GarminPacket::toString8() const
{
  std::stringstream sstr;
  sstr << "packet_type=0x" << antpm::toString<int>((int)mPacketType, 2, '0') << "=" << (int)mPacketType << "\n";
  sstr << "pid=0x" << antpm::toString<int>((int)mPacketId, 4, '0') << "=" << (int)mPacketId;
  if(mPacketType == appl_A010)
    sstr << " " << A010_cmnd_name(mPacketId) << "\n";
  else
    sstr << " " << garmin_pid_name((int)mPacketId) << "\n";
  return sstr.str();
}






#define SYMBOL_NAME(func_name)                     \
  const char *                          \
  func_name ( int s )             \
  {                                     \
    const char * name = "????_Pid_Unknown";      \
                                        \
    switch ( s )

#define SYMBOL_CASE(x) case x: name = #x; break

#define SYMBOL_DEFAULT                  \
    default: break;                     \
    }                                   \
                                        \
    return name


SYMBOL_NAME(garmin_pid_name) {
  SYMBOL_CASE(L000_Pid_Protocol_Array);
  SYMBOL_CASE(L000_Pid_Product_Rqst);
  SYMBOL_CASE(L000_Pid_Product_Data);
  SYMBOL_CASE(L000_Pid_Ext_Product_Data);
  SYMBOL_CASE(L001_Pid_Command_Data);
  SYMBOL_CASE(L001_Pid_Xfer_Cmplt);
  SYMBOL_CASE(L001_Pid_Date_Time_Data);
  SYMBOL_CASE(L001_Pid_Position_Data);
  SYMBOL_CASE(L001_Pid_Prx_Wpt_Data);
  SYMBOL_CASE(L001_Pid_Records);
              /* L001_Pid_Undocumented_1    = 0x001c, */
  SYMBOL_CASE(L001_Pid_Rte_Hdr);
  SYMBOL_CASE(L001_Pid_Rte_Wpt_Data);
  SYMBOL_CASE(L001_Pid_Almanac_Data);
  SYMBOL_CASE(L001_Pid_Trk_Data);
  SYMBOL_CASE(L001_Pid_Wpt_Data);
  SYMBOL_CASE(L001_Pid_Pvt_Data);
  SYMBOL_CASE(L001_Pid_Rte_Link_Data);
  SYMBOL_CASE(L001_Pid_Trk_Hdr);
  SYMBOL_CASE(L001_Pid_FlightBook_Record);
  SYMBOL_CASE(L001_Pid_Lap);
  SYMBOL_CASE(L001_Pid_Wpt_Cat);
  SYMBOL_CASE(L001_Pid_Run);
  SYMBOL_CASE(L001_Pid_Workout);
  SYMBOL_CASE(L001_Pid_Workout_Occurrence);
  SYMBOL_CASE(L001_Pid_Fitness_User_Profile);
  SYMBOL_CASE(L001_Pid_Workout_Limits);
  SYMBOL_CASE(L001_Pid_Course);
  SYMBOL_CASE(L001_Pid_Course_Lap);
  SYMBOL_CASE(L001_Pid_Course_Point);
  SYMBOL_CASE(L001_Pid_Course_Trk_Hdr);
  SYMBOL_CASE(L001_Pid_Course_Trk_Data);
  SYMBOL_CASE(L001_Pid_Course_Limits);
/*  SYMBOL_CASE(L002_Pid_Almanac_Data);
  SYMBOL_CASE(L002_Pid_Command_Data);
  SYMBOL_CASE(L002_Pid_Xfer_Cmplt);
  SYMBOL_CASE(L002_Pid_Date_Time_Data);
  SYMBOL_CASE(L002_Pid_Position_Data);
  SYMBOL_CASE(L002_Pid_Prx_Wpt_Data);
  SYMBOL_CASE(L002_Pid_Records);
  SYMBOL_CASE(L002_Pid_Rte_Hdr);
  SYMBOL_CASE(L002_Pid_Rte_Wpt_Data);
  SYMBOL_CASE(L002_Pid_Wpt_Data);*/
  SYMBOL_DEFAULT;
}

SYMBOL_NAME(A010_cmnd_name) {
  SYMBOL_CASE(A010_Cmnd_Abort_Transfer);
  SYMBOL_CASE(A010_Cmnd_Transfer_Alm);
  SYMBOL_CASE(A010_Cmnd_Transfer_Posn);
  SYMBOL_CASE(A010_Cmnd_Transfer_Prx);
  SYMBOL_CASE(A010_Cmnd_Transfer_Rte);
  SYMBOL_CASE(A010_Cmnd_Transfer_Time);
  SYMBOL_CASE(A010_Cmnd_Transfer_Trk);
  SYMBOL_CASE(A010_Cmnd_Transfer_Wpt);
  SYMBOL_CASE(A010_Cmnd_Turn_Off_Pwr);
  SYMBOL_CASE(A010_Cmnd_Start_Pvt_Data);
  SYMBOL_CASE(A010_Cmnd_Stop_Pvt_Data);
  SYMBOL_CASE(A010_Cmnd_FlightBook_Transfer);
  SYMBOL_CASE(A010_Cmnd_Transfer_Laps);
  SYMBOL_CASE(A010_Cmnd_Transfer_Wpt_Cats);
  SYMBOL_CASE(A010_Cmnd_Transfer_Runs);
  SYMBOL_CASE(A010_Cmnd_Transfer_Workouts);
  SYMBOL_CASE(A010_Cmnd_Transfer_Workout_Occurrences);
  SYMBOL_CASE(A010_Cmnd_Transfer_Fitness_User_Profile);
  SYMBOL_CASE(A010_Cmnd_Transfer_Workout_Limits);
  SYMBOL_CASE(A010_Cmnd_Transfer_Courses);
  SYMBOL_CASE(A010_Cmnd_Transfer_Course_Laps);
  SYMBOL_CASE(A010_Cmnd_Transfer_Course_Points);
  SYMBOL_CASE(A010_Cmnd_Transfer_Course_Tracks);
  SYMBOL_CASE(A010_Cmnd_Transfer_Course_Limits);
  SYMBOL_DEFAULT;
}



}
