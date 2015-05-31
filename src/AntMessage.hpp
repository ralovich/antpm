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
#include <list>
#include <string>
#include "common.hpp"
#include <cassert>
#include <sstream>
#include <boost/static_assert.hpp>
#include <boost/thread/thread_time.hpp> // AntMessage.timestamp


namespace antpm{

std::string antFSCommand2Str(uchar cmd);
std::string antFSResponse2Str(uchar resp);
bool        isAntFSCommandOrResponse(const uchar command, bool& isCommand);

struct AntMessageContentBase
{};

#pragma pack(push,1)
struct M_ANT_Channel_Id : public AntMessageContentBase
{
  uchar  chan;
  ushort devNum;
  uchar  devId;
  uchar  transType;
  const std::string toString() const;
};
struct M_ANTFS_Beacon : public AntMessageContentBase
{
  uchar beaconId;//0x43 ANTFS_BeaconId
  union
  {
    struct
    {
      uchar beaconChannelPeriod:3;
      uchar pairingEnabled:1;
      uchar uploadEnabled:1;
      uchar dataAvail:1;
      uchar reserved0:2;
    };
    uchar status1;
  };
  union
  {
    struct 
    {
      uchar clientDeviceState:4;
      uchar reserved1:4;
    };
    uchar status2;
  };
  uchar authType;
  union
  {
    struct  
    {
      ushort dev;
      ushort manuf;
    };
    struct  
    {
      uint sn;
    };
    uchar sn4[4];
  };


  const char* szBeaconChannelPeriod() const;
  const char* szPairingEnabled() const;
  const char* szUploadEnabled() const;
  const char* szClientDeviceState() const;
  const char* szDataAvail() const;
  const char* szAuthType() const;
  const std::string strDeviceDescriptor() const;
  const std::string strDeviceSerial() const;
  void getDeviceDescriptor(ushort& dev, ushort& manuf) const;
  uint getDeviceSerial() const;
  const std::string toString() const;
};
#pragma pack(pop)
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Beacon)==8);


#pragma pack(push,1)
struct M_ANTFS_Command : public AntMessageContentBase
{
  uchar commandId;//0x44 ANTFS_CommandResponseId
  uchar command;
  enum
  {
    ReturnToLinkLayer=0,
    ReturnToBroadcastMode=1
  };
  enum
  {
    ProceedToTransport=0,
    RequestClientDeviceSerialNumber=1,
    RequestPairing=2,
    RequestPasskeyExchange=3
  };

  union Detail1
  {
    struct Link
    {
      uchar chanFreq;
      uchar chanPeriod;
      union
      {
        struct
        {
          uint sn;
        };
        uchar sn4[4];
      };
      const std::string toString() const;
    } link;
    struct Disconnect
    {
      uchar cmdType;
      const char* szCmdType() const;
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << szCmdType();
        return sstr.str();
      }
    } disconnect;
    struct Authenticate
    {
      uchar cmdType;
      uchar authStrLen;
      union
      {
        struct
        {
          uint sn;
        };
        uchar sn4[4];
      };
      // TODO:extra bytes ....
      const char *szCmdType() const;
      const std::string toString() const;
    } authenticate;
    // ping
    struct DownloadRequest
    {
      ushort dataFileIdx;
      uint   dataOffset;
      // ...
      const std::string toString() const;
    } downloadRequest;
    struct UploadRequest
    {
      ushort dataFileIdx;
      uint   maxSize;
      //uint   dataOffset; //TODO: in next burst packet
      const std::string toString() const;
    } uploadRequest;
    struct EraseRequest
    {
      ushort dataFileIdx;
      // ...
      const std::string toString() const;
    } eraseRequest;
    struct UploadData
    {
      ushort crcSeed;
      uint   dataOffset;
      // TODO: more burst packets
      // uchar unused6[6];
      // ushort crc;
      const std::string toString() const;
    } uploadData;

    struct DirectCmd
    {
      ushort fd;
      ushort offset;
      ushort data;
      const std::string toString() const;
    } direct;
  } detail;
  const std::string toString() const;
};
struct M_ANTFS_Command_Authenticate : public M_ANTFS_Command
{
  uint64_t key;
  uint64_t footer;
};
struct M_ANTFS_Command_Download : public M_ANTFS_Command
{
  uchar zero;
  uchar initReq;
  ushort crcSeed;
  uint maxBlockSize;
};
struct M_ANTFS_Command_Pairing : public M_ANTFS_Command
{
  uchar name[8]; // seems like 310XT can display properly up to 8 chars during the pairing screen
};
struct M_ANTFS_Command_Direct : public M_ANTFS_Command
{
  uint64_t code;
};
#pragma pack(pop)
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Command)==8);
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Command_Authenticate)==24);
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Command_Download)==16);
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Command_Pairing)==16);
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Command_Direct)==16);

#pragma pack(push,1)
struct M_ANTFS_Response : public AntMessageContentBase
{
  uchar responseId;//0x44 ANTFS_CommandResponseId
  uchar response;
  enum {
    DownloadRequestOK=0,
    DataDoesNotExist=1,
    DataExistsButIsNotDownloadable=2,
    NotReadyToDownload=3,
    DownloadRequestInvalid=4,
    CRCIncorrect=5
  };
  enum {
    UploadRequestOK=0,
    DataFileIndexDoesNotExist=1,
    DataFileIndexExistsButIsNotWriteable=2,
    NotEnoughSpaceToCompleteWrite=3,
    UploadRequestInvalid=4,
    NotReadyToUpload=5
  };
  enum {
    DataUploadSuccessfulOK=0,
    DataUploadFailed=1
  };
  union Detail
  {
    struct AuthenticateResponse
    {
      uchar respType;
      uchar authStrLen;
      union
      {
        struct
        {
          uint sn;
        };
        uchar sn4[4];
      };
      // TODO:extra bytes in following burst message
      const char* szRespType() const;
      const std::string toString() const;
    } authenticateResponse;
    // ping
    struct DownloadRequestResponse
    {
      uchar responseVal;
      uchar reserved;
      uint  remainingBytes;
      // ...
      const char* szResponseVal() const;
      const std::string toString() const;
    } downloadRequestResponse;
    struct UploadRequestResponse
    {
      uchar responseVal;
      uchar unused1;
      uint  lastDataOffset;
      // TODO: 4 packets in total
      const char* szResponseVal() const;
      const std::string toString() const;
    } uploadRequestResponse;
    struct EraseRequestResponse
    {
      uchar responseVal;
      const char* szResponseVal() const;
      const std::string toString() const;
    } eraseRequestResponse;
    struct UploadDataResponse
    {
      // TODO: first is a beacon
      uchar responseVal;
      const char* szResponseVal() const;
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << szResponseVal();
        return sstr.str();
      }
    } uploadDataResponse;
    struct DirectResponse
    {
      ushort fd;
      ushort offset;
      ushort data;
      const std::string toString() const;
    } directResponse;
  } detail;
  const std::string toString() const;
};
struct M_ANTFS_Response_Download : public M_ANTFS_Response
{
  uint dataOff;
  uint fileSize;
};
struct M_ANTFS_Response_Download_Footer
{
  uchar reserved[6];
  ushort crc;
};
struct M_ANTFS_Response_Pairing : public M_ANTFS_Response
{
  uint64_t pairedKey;
};
#pragma pack(pop)
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Response)==8);
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Response_Download)==16);
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Response_Download_Footer)==8);
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Response_Pairing)==16);

#pragma pack(push,1)
struct M_ANT_Burst : public AntMessageContentBase
{
  union {
    struct {
      uchar chan:5;
      uchar seq:2;
      uchar last:1;
    };
    uchar seqchan;
  };
  uchar data8[8];
  const std::string toString() const;
  bool isLast() const
  {
    bool isLast = ((last!=0x00)?true:false);
    return isLast;
  }
};
#pragma pack(pop)
BOOST_STATIC_ASSERT(sizeof(M_ANT_Burst)==9);



//
struct AntMessage{
  bool                       sent;  // sent or received
  boost::system_time         timestamp;
  size_t                     idx;   //
  std::vector<unsigned char> bytes; // the raw message bytes


public:
  AntMessage() : sent(false), idx(0) {}
  AntMessage(uchar mesg, uchar data1) {uchar buf[1] = {data1}; if(!assemble(mesg, buf, sizeof(buf))) throw 0; }
  AntMessage(uchar mesg, uchar data1, uchar data2) {uchar buf[2] = {data1, data2 }; if(!assemble(mesg, buf, sizeof(buf))) throw 0; }
  AntMessage(uchar mesg, uchar data1, uchar data2, uchar data3) {uchar buf[3] = {data1, data2, data3 }; if(!assemble(mesg, buf, sizeof(buf))) throw 0; }

  bool vrfChkSum() const;
  static std::string msgId2Str(uchar msgId);
  static std::string msgCode2Str(uchar msgCode);
  const std::string str() const;
  const std::string str2() const;
  const std::string strDt(const double& dt) const;
  const std::string strExt() const;
  const std::string antfs2Str() const;
  const std::string dump() const;
  const std::string dumpDumb() const;

  // receiving
  bool interpret();
  static bool interpret2(std::list<uchar>& q, std::vector<AntMessage>& messages);
  size_t getLenPacket() const;
  size_t getLenRaw() const;
  bool getChannelNumber(uchar& chan);
  bool fromStringOfBytes(const char* s);
  template <class Container>
  static bool stringToBytes(const char* s, Container& bytes);

  // sending
  bool assemble(unsigned char mesg, const unsigned char *inbuf, unsigned char len);

  template <class Container>
  static bool saveAsUsbMon(const char* fileName, const Container& messages);
  template <class Container>
  static bool saveAsUsbMon(std::ostream& os, const Container& messages);
  template <class Container>
  static bool saveAsAntParse(const char* fileName, const Container& messages);
  template <class Container>
  static bool saveAsAntParse(std::ostream& os, const Container& messages);

//private:
  uchar getSync() const {return bytes[0]; }
  uchar getLenPayload() const { return bytes[1]; }
  uchar getMsgId() const { return bytes[2]; }
  uchar getCheckSum() const;
  std::vector<uchar> getPayload() const;
  uchar* getPayloadRef();
  const uchar* getPayloadRef() const;
  uchar* getBytes();
  const uchar* getBytes() const;
};


// encapsualtes the byte stream of a file retrieved through ant-fs
struct AntFsFile
{
  std::vector<uchar> bytes;
  bool checkCrc(const ushort seed = 0x0000) const;
  ushort crc16Calc(const ushort seed = 0x0000) const;
  ushort crc16byte(const ushort crc, uchar byte) const;
  bool saveToFile(const char* fileName = "antfs.bin");
};

}
