// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
////////////////////////////////////////////////////////////////////
// Copyright (c) 2011-2013 RALOVICH, Kristóf                      //
//                                                                //
// This program is free software; you can redistribute it and/or  //
// modify it under the terms of the GNU General Public License    //
// version 2 as published by the Free Software Foundation.        //
//                                                                //
////////////////////////////////////////////////////////////////////
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
#include <boost/thread/thread_time.hpp>


namespace antpm{

std::string antFSCommand2Str(uchar cmd);
std::string antFSResponse2Str(uchar resp);
bool        isAntFSCommandOrResponse(const uchar command, bool& isCommand);

#pragma pack(push,1)
struct M_ANTFS_Beacon
{
  uchar beaconId;//0x43
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


  const char* szBeaconChannelPeriod() const
  {
    if(beaconChannelPeriod==0x0) return "Beacon=0.5Hz";
    else if(beaconChannelPeriod==0x1) return "Beacon=1Hz";
    else if(beaconChannelPeriod==0x2) return "Beacon=2Hz";
    else if(beaconChannelPeriod==0x3) return "Beacon=4Hz";
    else if(beaconChannelPeriod==0x4) return "Beacon=8Hz";
    else if(beaconChannelPeriod==0x7) return "Beacon=MatchEstablishedChannelPeriod";
    else return "Beacon=??";
  }
  const char* szPairingEnabled() const
  {
    return pairingEnabled ? "pairing=enabled" : "pairing=disabled";
  }
  const char* szUploadEnabled() const
  {
    return uploadEnabled ? "upload=enabled" : "upload=disabled";
  }
  const char* szClientDeviceState() const
  {
    if(clientDeviceState==0x00) return "State=Link";
    else if(clientDeviceState==0x01) return "State=Authentication";
    else if(clientDeviceState==0x02) return "State=Transport";
    else if(clientDeviceState==0x03) return "State=Busy";
    else return "State=??";
  }
  const char* szDataAvail() const
  {
    return dataAvail ? "dataAvail=yes" : "dataAvail=no";
  }
  const char* szAuthType() const
  {
    if(authType==0x0) return "Auth=Passthrough";
    else if(authType==0x1) return "Auth=NA";
    else if(authType==0x2) return "Auth=PairingOnly";
    else if(authType==0x3) return "Auth=PasskeyAndPairingOnly";
    else  return "Auth=??";
  }
  const std::string strDeviceDescriptor() const
  {
    return std::string("dev=0x") + antpm::toString(this->dev, 4, '0') + std::string("manuf=0x") + antpm::toString(this->manuf, 4, '0');
  }
  const std::string strDeviceSerial() const
  {
    return std::string("SN=0x") + antpm::toString(this->sn, 8, '0');
  }
  void getDeviceDescriptor(ushort& dev, ushort& manuf) const
  {
    dev=this->dev;
    manuf=this->manuf;
  }
  uint getDeviceSerial() const
  {
    return this->sn;
  }
  const std::string toString() const
  {
    assert(beaconId==0x43);
    std::stringstream sstr;
    sstr << " ANTFS_BEACON(0x" << antpm::toString(unsigned(beaconId), 2, '0') << ") "
      << this->szBeaconChannelPeriod()
      << ", " << this->szPairingEnabled()
      << ", " << this->szUploadEnabled()
      << ", " << this->szDataAvail()
      << ", " << this->szClientDeviceState()
      << ", " << this->szAuthType();
    return sstr.str();
  }
};
#pragma pack(pop)
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Beacon)==8);


#pragma pack(push,1)
struct M_ANTFS_Command
{
  uchar commandId;//0x44
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
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << "freq=0x" << antpm::toString(unsigned(chanFreq), 2, '0') << ", period=0x" << antpm::toString(unsigned(chanPeriod), 2, '0') << ", SNhost=0x" << antpm::toString(sn, 8, '0');
        return sstr.str();
      }
    } link;
    struct Disconnect
    {
      uchar cmdType;
      const char* szCmdType() const
      {
        if(cmdType==ReturnToLinkLayer) return "type=ReturnToLinkLayer";
        else if(cmdType==ReturnToBroadcastMode) return "type=ReturnToBroadcastMode";
        else return "type=??";
      }
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
      const char* szCmdType() const
      {
        if(cmdType==ProceedToTransport) return "type=ProceedToTransport(pass-through)";
        else if(cmdType==RequestClientDeviceSerialNumber) return "type=RequestClientDeviceSerialNumber";
        else if(cmdType==RequestPairing) return "type=RequestPairing";
        else if(cmdType==RequestPasskeyExchange) return "type=RequestPasskeyExchange";
        else return "type=??";
      }
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << szCmdType() << ", authStrLen=" << int(authStrLen) << ", SNhost=0x" << antpm::toString(sn, 8, '0');
        return sstr.str();
      }
    } authenticate;
    // ping
    struct DownloadRequest
    {
      ushort dataFileIdx;
      uint   dataOffset;
      // ...
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << "file=0x" << antpm::toString(dataFileIdx, 4, '0') << ", dataOffset=0x" << antpm::toString(dataOffset, 8, '0');
        return sstr.str();
      }
    } downloadRequest;
    struct UploadRequest
    {
      ushort dataFileIdx;
      uint   maxSize;
      //uint   dataOffset; //TODO: in next burst packet
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << "file=0x" << antpm::toString(dataFileIdx, 4, '0') << ", maxSize=0x" << antpm::toString(maxSize, 8, '0');
        return sstr.str();
      }
    } uploadRequest;
    struct EraseRequest
    {
      ushort dataFileIdx;
      // ...
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << "dataFileIdx=0x" << antpm::toString(dataFileIdx, 4, '0');
        return sstr.str();
      }
    } eraseRequest;
    struct UploadData
    {
      ushort crcSeed;
      uint   dataOffset;
      // TODO: more burst packets
      // uchar unused6[6];
      // ushort crc;
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << "crcSeed=0x" << antpm::toString(crcSeed, 4, '0') << ", dataOffset=0x" << antpm::toString(dataOffset, 8, '0');
        return sstr.str();
      }
    } uploadData;
  } detail;
  const std::string toString() const
  {
    assert(commandId==0x44);
    std::stringstream sstr;
    sstr << " ANTFS_CMD(0x" << antpm::toString(unsigned(commandId),2,'0') << ") "
      << antFSCommand2Str(command);
    if(command==ANTFS_CmdLink) sstr << " " << detail.link.toString();
    else if(command==ANTFS_CmdDisconnect) sstr << " " << detail.disconnect.toString();
    else if(command==ANTFS_CmdAuthenticate) sstr << " " << detail.authenticate.toString();
    else if(command==ANTFS_ReqDownload) sstr << " " << detail.downloadRequest.toString();
    else if(command==ANTFS_ReqUpload)  sstr << " " << detail.uploadRequest.toString();
    else if(command==ANTFS_ReqErase)   sstr << " " << detail.eraseRequest.toString();
    else if(command==ANTFS_UploadData) sstr << " " << detail.uploadData.toString();
    return sstr.str();
  }
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
#pragma pack(pop)
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Command)==8);
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Command_Authenticate)==24);
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Command_Download)==16);
BOOST_STATIC_ASSERT(sizeof(M_ANTFS_Command_Pairing)==16);

#pragma pack(push,1)
struct M_ANTFS_Response
{
  uchar responseId;//0x44
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
      const char* szRespType() const
      {
        if(respType==0) return "resp=SN";
        else if(respType==1) return "resp=accept";
        else if (respType==2) return "resp=reject";
        else return "resp=??";
      }
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << szRespType() << ", authStrLen=" << int(authStrLen) << ", SNclient=0x" << antpm::toString(sn, 8, '0');
        return sstr.str();
      }
    } authenticateResponse;
    // ping
    struct DownloadRequestResponse
    {
      uchar responseVal;
      uchar reserved;
      uint  remainingBytes;
      // ...
      const char* szResponseVal() const
      {
        if(responseVal==DownloadRequestOK) return "resp=DownloadRequestOK";
        else if(responseVal==DataDoesNotExist) return "resp=DataDoesNotExist";
        else if(responseVal==DataExistsButIsNotDownloadable) return "resp=DataExistsButIsNotDownloadable";
        else if(responseVal==NotReadyToDownload) return "resp=NotReadyToDownload";
        else if(responseVal==DownloadRequestInvalid) return "resp=DownloadRequestInvalid";
        else if(responseVal==CRCIncorrect) return "resp=CRCIncorrect";
        return "resp=??";
      }
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << szResponseVal() << ", remainingBytes=0x" << antpm::toString(remainingBytes, 8, '0');
        return sstr.str();
      }
    } downloadRequestResponse;
    struct UploadRequestResponse
    {
      uchar responseVal;
      uchar unused1;
      uint  lastDataOffset;
      // TODO: 4 packets in total
      const char* szResponseVal() const
      {
        if(responseVal==UploadRequestOK) return "resp=UploadRequestOK";
        else if(responseVal==DataFileIndexDoesNotExist) return "resp=DataFileIndexDoesNotExist";
        else if(responseVal==DataFileIndexExistsButIsNotWriteable) return "resp=DataFileIndexExistsButIsNotWriteable";
        else if(responseVal==NotEnoughSpaceToCompleteWrite) return "resp=NotEnoughSpaceToCompleteWrite";
        else if(responseVal==UploadRequestInvalid) return "resp=UploadRequestInvalid";
        else if(responseVal==NotReadyToUpload) return "resp=NotReadyToUpload";
        return "resp=??";
      }
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << szResponseVal() << ", lastDataOffset=0x" << antpm::toString(lastDataOffset, 8, '0');
        return sstr.str();
      }
    } uploadRequestResponse;
    struct EraseRequestResponse
    {
      uchar responseVal;
      const char* szResponseVal() const
      {
        if(responseVal==0) return "resp=EraseSuccessful";
        else if(responseVal==1) return "resp=EraseFailed";
        else if(responseVal==2) return "resp=NotReady";
        return "resp=??";
      }
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << szResponseVal() << " 0x" << antpm::toString<int>(int(responseVal),2,'0');
        return sstr.str();
      }
    } eraseRequestResponse;
    struct UploadDataResponse
    {
      // TODO: first is a beacon
      uchar responseVal;
      const char* szResponseVal() const
      {
        if(responseVal==DataUploadSuccessfulOK) return "resp=DataUploadSuccessfulOK";
        else if(responseVal==DataUploadFailed) return "resp=DataUploadFailed";
        return "resp=??";
      }
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr << szResponseVal();
        return sstr.str();
      }
    } uploadDataResponse;
  } detail;
  const std::string toString() const
  {
    assert(responseId==0x44);
    std::stringstream sstr;
    sstr << " ANTFS_RESP(0x" << antpm::toString(unsigned(responseId),2,'0') << ") "
      << antFSResponse2Str(response);
    if(response==ANTFS_RespAuthenticate) sstr << " " << detail.authenticateResponse.toString();
    else if(response==ANTFS_RespDownload) sstr << " " << detail.downloadRequestResponse.toString();
    else if(response==ANTFS_RespUpload) sstr << " " << detail.uploadRequestResponse.toString();
    else if(response==ANTFS_RespErase)      sstr << " " << detail.eraseRequestResponse.toString();
    else if(response==ANTFS_RespUploadData) sstr << " " << detail.uploadDataResponse.toString();
    return sstr.str();
  }
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
struct M_ANT_Burst
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
  const std::string toString() const
  {
    std::stringstream sstr;
    sstr << " chan=0x" << antpm::toString<int>(chan,2,'0') << ", seq=" << antpm::toStringDec<int>(seq,1,' ') << ", last=" << (isLast()?"yes":"no ");
    return sstr.str();
  }
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

  static bool lookupInVector;

public:
  AntMessage() {}
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


struct AntFsFile
{
  std::vector<uchar> bytes;
  bool checkCrc(const ushort seed = 0x0000) const;
  ushort crc16Calc(const ushort seed = 0x0000) const;
  ushort crc16byte(const ushort crc, uchar byte) const;
  bool saveToFile(const char* fileName = "antfs.bin");
};

}
