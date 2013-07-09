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


#include "AntMessage.hpp"
#include "common.hpp"
#include <cstdio>
#include <sstream>
#include <cstdlib>
#include <iomanip> // setw
#include <cstring> // strlen
#include <cassert> // assert
#include <fstream>
#include <queue>
#include <boost/date_time/time_duration.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include "FIT.hpp"


using namespace std;


namespace antpm{

std::string
antFSCommand2Str(uchar cmd)
{
  size_t i=0;
  for(; antFSCommandNames[i].i!=-1; i++)
  {
    if(antFSCommandNames[i].i==int(cmd))
    {
      break;
    }
  }
  if(antFSCommandNames[i].i==-1)
    return std::string("UNKNOWN_0x") + toString<int>(cmd, 2, '0');
  else
    return std::string(antFSCommandNames[i].s);
}



std::string
antFSResponse2Str(uchar resp)
{
  size_t i=0;
  for(; antFSResponseNames[i].i!=-1; i++)
  {
    if(antFSResponseNames[i].i==int(resp))
    {
      break;
    }
  }
  if(antFSResponseNames[i].i==-1)
    return std::string("UNKNOWN_0x") + toString<int>(resp, 2, '0');
  else
    return std::string(antFSResponseNames[i].s);
}

bool
isAntFSCommandOrResponse(const uchar command, bool& isCommand)
{
  if(command==ANTFS_CmdLink
    || command==ANTFS_CmdDisconnect
    || command==ANTFS_CmdAuthenticate
    || command==ANTFS_CmdPing
    || command==ANTFS_ReqDownload
    || command==ANTFS_ReqUpload
    || command==ANTFS_ReqErase
    || command==ANTFS_UploadData      )
  {
    isCommand=true;
    return true;
  }
  else if (command==ANTFS_RespAuthenticate
    || command==ANTFS_RespDownload
    || command==ANTFS_RespUpload
    || command==ANTFS_RespErase
    || command==ANTFS_RespUploadData
    )
  {
    isCommand=false;
    return true;
  }
  else
  {
    return false;
  }
}


bool AntMessage::lookupInVector = false;

bool AntMessage::vrfChkSum() const
{
  if(bytes.size()<5)
  {
    lprintf(LOG_ERR, "%d bytes\n", (int)bytes.size());
    return false;
  }
  if(bytes.size()<getLenPacket())
  {
    lprintf(LOG_ERR, "%d bytes, %d payload\n", (int)bytes.size(), (int)getLenPacket());
    return false;
  }
  size_t chkIdx=getLenPayload()+3; // index of checksum byte, not necessary the last one
  uchar chk = getCheckSum();
  bool match= (chk == bytes[chkIdx]);
  if(!match) { lprintf(LOG_ERR, "checksum mismatch\n"); }
  return match;
}

std::string
AntMessage::msgId2Str(uchar msgId)
{
  size_t i=0;
  for(; msgNames[i].i!=-1; i++)
  {
    if(msgNames[i].i==int(msgId))
    {
      break;
    }
  }
  if(msgNames[i].i==-1)
    return std::string("UNKNOWN_0x") + toString<int>(msgId, 2, '0');
  else
    return std::string(msgNames[i].s);
}

std::string AntMessage::msgCode2Str(uchar msgCode)
{
  size_t i=0;
  for(; responseNames[i].i!=-1; i++)
  {
    if(responseNames[i].i==int(msgCode))
    {
      break;
    }
  }
  if(responseNames[i].i==-1)
    return std::string("UNKNOWN_0x") + toString<int>(msgCode, 2, '0');
  else
    return std::string(responseNames[i].s);
}


const std::string
hexa8(const int off, const std::vector<uchar> bytes)
{
  assert(bytes.size()>=size_t(off+8));
  std::string str;

  str += " ";
  for(int i=off; i<off+8; i++)
  {
    str += toString<int>(bytes[i], 2, '0');
  }
  str += " ";
  for(int i=off; i<off+8; i++)
  {
    str += isprint(bytes[i])?bytes[i]:'.';
  }
  return str;
}


const std::string AntMessage::str() const
{
  if(bytes.empty())
    return "? ???";

  std::string str=sent?"S ":"R ";


  str += toStringDec<size_t>(idx, 4, ' ') + " ";

  str += str2();

  return str;
}


const std::string
AntMessage::str2() const
{
  if(bytes.empty())
    return " EMPTY";

  std::stringstream sstr;
  std::string str;

  //sstr << /*"0x" +*/ toString<int>(getMsgId(), 2, ' ')  << " ";
  sstr << msgId2Str(getMsgId());

  if(getMsgId()==MESG_RESPONSE_EVENT_ID)
  {
    sstr << " chan=0x" + toString<int>(getPayload()[0], 2, '0');
    sstr << " mId=" + msgId2Str(getPayload()[1]);
    sstr << " mCode=" + msgCode2Str(getPayload()[2]);
  }
  else if(getMsgId()==MESG_ASSIGN_CHANNEL_ID)
  {
    sstr << " chan=0x" + toString<int>(getPayload()[0], 2, '0');
    sstr << " type=0x" + toString<int>(getPayload()[1], 2, '0');
    sstr << " net=0x" + toString<int>(getPayload()[2], 2, '0');
    if(getLenPayload()>3)
    {
      sstr << " ext=0x" + toString<int>(getPayload()[3], 2, '0');
    }
  }
  else if(getMsgId()==MESG_CHANNEL_MESG_PERIOD_ID)
  {
    sstr << " chan=0x" + toString<int>(getPayloadRef()[0], 2, '0');
    const ushort* mesgPeriod = reinterpret_cast<const ushort*>(&getPayloadRef()[1]);
    sstr << " mesgPeriod=0x" << toString<int>(*mesgPeriod, 4, '0');
  }
  else if(getMsgId()==MESG_CHANNEL_SEARCH_TIMEOUT_ID)
  {
    sstr << " chan=0x" + toString<int>(getPayloadRef()[0], 2, '0');
    sstr << " searchTimeout=0x" << toString<int>(getPayloadRef()[1], 2, '0');
  }
  else if(getMsgId()==MESG_CHANNEL_RADIO_FREQ_ID)
  {
    sstr << " chan=0x" + toString<int>(getPayloadRef()[0], 2, '0');
    sstr << " rfFreq=0x" << toString<int>(getPayloadRef()[1], 2, '0');
  }
  else if(getMsgId()==MESG_NETWORK_KEY_ID)
  {
    sstr << " net=0x" + toString<int>(getPayloadRef()[0], 2, '0');
    sstr << " key=0x" + hexa8(1, getPayload());
  }
  else if(getMsgId()==MESG_SEARCH_WAVEFORM_ID)
  {
    sstr << " chan=0x" + toString<int>(getPayloadRef()[0], 2, '0');
    sstr << " wave=0x" << toString<int>(ushort(getPayloadRef()[1]*256+getPayloadRef()[2]), 4, '0');
  }
  else if(getMsgId()==MESG_OPEN_CHANNEL_ID)
  {
    sstr << " chan=0x" + toString<int>(getPayloadRef()[0], 2, '0');
  }
  else if(getMsgId()==MESG_REQUEST_ID)
  {
    sstr << " chan=0x" + toString<int>(getPayload()[0], 2, '0');
    sstr << " reqMsgId=" + msgId2Str(getPayload()[1]);
  }
  else if(getMsgId()==MESG_BROADCAST_DATA_ID)
  {
    //int len = getLenPayload();
    vector<uchar> payl=getPayload();
    sstr << " chan=0x" + toString<int>(getPayload()[0], 2, '0');
    sstr << antfs2Str();
  }
  else if(getMsgId()==MESG_ACKNOWLEDGED_DATA_ID)
  {
    sstr << " chan=" + toString<int>(getPayload()[0], 2, '0');
    sstr << antfs2Str();
  }
  else if(getMsgId()==MESG_BURST_DATA_ID)
  {
    assert(getLenPayload()==9);
    for (size_t i=0;i<9;i++)
    {
      vector<uchar> payl=getPayload();
      const uchar* p=getPayloadRef();
      assert(payl[i]==p[i]);
    }
    const M_ANT_Burst* burst(reinterpret_cast<const M_ANT_Burst*>(getPayloadRef()));
    //sstr << " chan=0x" << toString<int>(burst->chan,2,'0') << ", seq=" << toStringDec<int>(burst->seq,1,' ') << ", last=" << (burst->last?"yes":"no ");
    sstr << burst->toString();
    sstr << antfs2Str();
  }
  else if(getMsgId()==MESG_CHANNEL_ID_ID)
  {
    struct M_ANT_Channel_Id 
    {
      uchar chan;
      ushort devNum;
      uchar devTypeId;
      uchar manId;
      const std::string toString() const
      {
        std::stringstream sstr;
        sstr
          << " chan=0x" << antpm::toString<int>(chan,2,'0') << ", devNum=0x" << antpm::toString<int>(devNum,4,'0')
          << ", typ=0x" << antpm::toString<int>(devTypeId,2,'0') << ", man=0x" << antpm::toString<int>(manId,2,'0');
        return sstr.str();
      }
    };
    const M_ANT_Channel_Id* mesg(reinterpret_cast<const M_ANT_Channel_Id*>(getPayloadRef()));
    sstr << mesg->toString();
  }
  else if(getMsgId()==MESG_CHANNEL_STATUS_ID)
  {
    enum {
      UnAssigned = 0,
      Assigned = 1,
      Searching = 2,
      Tracking = 3
    } ;
    sstr << " chan=" + toString<int>(getPayload()[0], 2, '0');
    uchar chanSt=getPayloadRef()[1]&0x03;
    struct {
    const char* szChanSt(uchar chanSt)
    {
      if(chanSt==UnAssigned) return "chanSt=UnAssigned";
      else if(chanSt==Assigned) return "chanSt=Assigned";
      else if(chanSt==Searching) return "chanSt=Searching";
      else if(chanSt==Tracking) return "chanSt=Tracking";
      else return "chanSt=??";
    } } CHST;
    sstr << " " << CHST.szChanSt(chanSt);
  }

  return sstr.str();
}


const string AntMessage::strDt(const double &dt) const
{
  if(bytes.empty())
    return "? ???";

  std::string str=sent?"S ":"R ";


  str += toStringDec<double>(dt, 7, ' ') + " ";

  str += str2();

  return str;
}


const std::string
AntMessage::strExt() const
{
  return dump() + str2();
}

const std::string
AntMessage::antfs2Str() const
{
  bool isCommand=false;
  bool valid = isAntFSCommandOrResponse(getPayload()[2], isCommand);
  std::stringstream sstr;
  if(getPayload()[1]==ANTFS_BeaconId)
  {
    const M_ANTFS_Beacon* beacon(reinterpret_cast<const M_ANTFS_Beacon*>(&getPayloadRef()[1]));
    sstr << beacon->toString();// << hexa8(1, getPayload());
  }
  else if(getPayload()[1]==ANTFS_CommandResponseId && isCommand && valid)
  {
    const M_ANTFS_Command* cmdResp(reinterpret_cast<const M_ANTFS_Command*>(&getPayloadRef()[1]));
    sstr << cmdResp->toString();// << hexa8(1, getPayload());
  }
  else if(getPayload()[1]==ANTFS_CommandResponseId && !isCommand && valid)
  {
    const M_ANTFS_Response* cmdResp(reinterpret_cast<const M_ANTFS_Response*>(&getPayloadRef()[1]));
    //if(lookupInVector && cmdResp->response==ANTFS_ReqDownload)
    sstr << cmdResp->toString();// << hexa8(1, getPayload());
  }
  else
  {
    sstr << hexa8(1, getPayload());
  }
  return sstr.str();
}

const std::string
AntMessage::dump() const
{
  assert(bytes.size()>=5);
  assert(bytes.size()<=13);
  std::stringstream ss;
  ss << (sent?"S":"R") << "[" << std::setw(2) << bytes.size() << "] ";
  for(size_t i=0; i<bytes.size(); i++)
  {
    ss << std::setw(2) << std::setfill('0') << std::hex << int(bytes[i]);
    if(i+1!=bytes.size()) ss << "_"; else ss << " ";
  }
  for(size_t i=bytes.size(); i<13; i++)
  {
    ss << "   ";
  }
  return ss.str();
}



const std::string
AntMessage::dumpDumb() const
{
  std::stringstream ss;
  ss << " " << "[" << std::setw(2) << bytes.size() << "] ";
  for(size_t i=0; i<bytes.size(); i++)
  {
    ss << std::setw(2) << std::setfill('0') << std::hex << int(bytes[i]);
    if(i+1!=bytes.size()) ss << "_"; else ss << " ";
  }
  for(size_t i=bytes.size(); i<13; i++)
  {
    ss << "   ";
  }
  return ss.str();
}


bool AntMessage::interpret()
{
  if(bytes.size()<5)
  {
    //printf("not enough bytes in raw stream: %s\n", dumpDumb().c_str());
    return false;
  }
  if(getSync() != MESG_TX_SYNC)
  {
    lprintf(LOG_ERR, "no TX_SYNC byte: %s", dumpDumb().c_str());
    return false;
  }
  //if(getMsgId()==0xE2 || getMsgId()==0xE1 || getMsgId()==0xE0)
  //{
  //  if(bytes.size()<static_cast<size_t>(getLenPayload()+5))
  //    return false;
  //  abort();// FIXME: extended message
  //  if(!vrfChkSum())
  //    return false;
  //}
  //else
  {
    const size_t minLen=static_cast<size_t>(getLenPayload()+4);
    if(bytes.size()<minLen)
    {
      //printf("not enough bytes to cover payload: %s", dump().c_str());
      return false;
    }
    if(!vrfChkSum())
    {
      lprintf(LOG_ERR, "checksum verification failed: %s\n", dumpDumb().c_str());
      return false;
    }
    // TODO: handle case where multiple messages are concatenated in this->bytes
    //ptrdiff_t residue = bytes.end()-bytes.begin()+getLenPacket();
    //fprintf(loggerc(), "%d bytes for next decode\n", (int)residue);
    bytes.erase(bytes.begin()+getLenPacket(), bytes.end());
  }
  return true;
}

// interpret byte stream, assemble packets and store results in \a messages
bool
AntMessage::interpret2(std::list<uchar>& q, std::vector<AntMessage>& messages)
{
  if(q.empty())
    return false;

  while(!q.empty())
  {
    if(q.size()==2
      && *(q.begin())==0x00
      && *(q.begin()++)==0x00)
    {
      // the usbmon logs seem to suffix sent messages with two zeroes...
      q.clear();
      return true;
    }

    AntMessage m;
    m.sent = false;
    m.bytes.resize(q.size());
    std::copy(q.begin(), q.end(), m.bytes.begin());

    bool ok = m.interpret();
    if(!ok)
    {
      lprintf(LOG_ERR, "interpret failed!\n"); fflush(stdout);
      return false;
    }
    for(size_t i=0; i<m.getLenPacket(); i++)
    {
      q.pop_front();
    }
    //m.idx = packetIdx++;
    messages.push_back(m);
  }

  return true;
}

size_t AntMessage::getLenPacket() const
{
  return getLenPayload()+4;
}

size_t AntMessage::getLenRaw() const
{
  return bytes.size();
}


bool
AntMessage::getChannelNumber(uchar &chan)
{
  //if(getMsgId()==MESG_RESPONSE_EVENT_ID)
  //  ;
  if(getMsgId()==MESG_BURST_DATA_ID && getLenPayload()==9)
  {
    chan = getPayload()[0];
    return true;
  }
  return false;
}


bool AntMessage::fromStringOfBytes(const char *s)
{
  return stringToBytes(s, this->bytes);
}


template <class Container>
bool
AntMessage::stringToBytes(const char *s, Container& bytes)
{
  // e.g. "a409502022008004310000b0fa"
  size_t chars=strlen(s);
  if(chars%2)
    return false;
  bytes.clear();
  for(size_t i=0; i<chars/2;i++)
  {
    char xx[3]={s[i*2], s[i*2+1], 0x00};
    bytes.push_back(uchar(strtoul(xx, NULL, 16)));
  }
  assert(bytes.size()*2==chars);
  return true;
}


bool AntMessage::assemble(unsigned char mesg, const unsigned char *inbuf, unsigned char len)
{
  bytes.clear();
  bytes.resize(len+4);
  bytes[0] = MESG_TX_SYNC;
  bytes[1] = len;
  bytes[2] = mesg;
  for(size_t i=0; i< len; i++)
    bytes[3+i] = inbuf[i];
  unsigned char chk = getCheckSum();
  bytes[len+3] = chk;
  return true;
}


template <class Container>
bool
AntMessage::saveAsUsbMon(const char* fileName, const Container& messages)
{
  std::ofstream of(fileName);
  CHECK_RETURN_FALSE(of.is_open());

  return saveAsUsbMon(of, messages);
}


template <class Container>
bool
AntMessage::saveAsUsbMon(std::ostream& os, const Container& messages)
{
  if(messages.empty())
    return true;

  CHECK_RETURN_FALSE(os.good());
  // dd65f0e8 4128379752 S Bo:1:005:2 -115 31 = 55534243 5e000000 00000000 00000600 00000000 00000000 00000000 000000
  // dd65f0e8 4128379808 C Bo:1:005:2 0 31 >
  for(typename Container::const_iterator i=messages.begin(); i!= messages.end(); i++)
  {
#define USB_SENT_FIXUP
    size_t bytes = i->getLenPacket();
    os << "ffff880064cd3f00 " << "3647721914 " /*<< i->timestamp*/ << ((i->sent?"S ":"C ")) << (i->sent?"Bo":"Bi") << ":3:002:1 " << ((i->sent?"-115 ":"0 "));
#ifdef USB_SENT_FIXUP
    if(i->sent)
      os << bytes+2;
    else
      os << bytes;
#else
    os << bytes;
#endif
    os << " = ";
    for (size_t j=0; j<bytes; j++)
    {
      uchar c=i->getBytes()[j];
      os << toString(int(c), 2, '0');
      if((j+1)%4==0 && j+1!=bytes)
        os<< " ";
    }
#ifdef USB_SENT_FIXUP
    if(i->sent) os<<"0000";
#endif
    os<<"\n";
  }
  return true;
}


template <class Container>
bool
AntMessage::saveAsAntParse(const char* fileName, const Container& messages)
{
  if(messages.empty())
    return true;

  std::ofstream of(fileName, ios_base::out | ios_base::app );
  CHECK_RETURN_FALSE(of.is_open());

  return saveAsAntParse(of, messages);
}


template <class Container>
bool
AntMessage::saveAsAntParse(std::ostream& os, const Container& messages)
{
  CHECK_RETURN_FALSE(os.good());

  boost::system_time t0;
  for(typename Container::const_iterator i=messages.begin(); i!= messages.end(); i++)
  {
    if(i==messages.begin())
      t0=i->timestamp;
    boost::system_time t1 = i->timestamp;
    boost::posix_time::time_duration td = t1 - t0;
    t0 = t1;
    double dt = double(td.total_microseconds())*0.001;
    os << i->strDt(dt) << "\n";
  }
  return true;
}


uchar
AntMessage::getCheckSum() const
{
  unsigned char chk = bytes[0];
  for(size_t i=1; i< getLenPacket()-1; i++)
    chk ^= bytes[i];
  return chk;
}


std::vector<uchar>
AntMessage::getPayload() const
{
  std::vector<uchar> pl;
  pl.resize(getLenPayload());
  std::copy(bytes.begin()+3, bytes.begin()+3+getLenPayload(), pl.begin());
  return pl;
}


uchar*
AntMessage::getPayloadRef()
{
  return &bytes[3];
}

const uchar*
AntMessage::getPayloadRef() const
{
  return &bytes[3];
}


uchar*
AntMessage::getBytes()
{
  return &bytes[0];
}

const uchar*
AntMessage::getBytes() const
{
  return &bytes[0];
}

bool
AntFsFile::checkCrc(const ushort seed) const
{
  if(bytes.size()<24)
    return false;
  ushort crc=seed;
  for (size_t i=16;i+8<bytes.size(); i++)
  {
    crc = crc16byte(crc, bytes[i]);
  }
  const ushort* tail = reinterpret_cast<const ushort*>(&bytes[bytes.size()-1-2]);
  return *tail == crc;
}

ushort
AntFsFile::crc16Calc(const ushort seed) const
{
  ushort crc=seed;
  for (size_t i=0;i<bytes.size(); i++)
  {
    crc = crc16byte(crc, bytes[i]);
  }
  return crc;
}

ushort
AntFsFile::crc16byte(const ushort crc0, uchar byte) const
{
  static const ushort crc_table[16] =
  {
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
    0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
  };

  ushort tmp = crc_table[crc0 & 0xF];
  ushort crc = (crc0 >> 4) & 0x0FFF;
  crc = crc ^ tmp ^ crc_table[byte & 0xF];

  tmp = crc_table[crc & 0xF];
  crc = (crc >> 4) & 0x0FFF;
  crc = crc ^ tmp ^ crc_table[(byte >> 4) & 0xF];

  return crc;
}


bool
AntFsFile::saveToFile(const char* fileName /* = "antfs.bin" */)
{
  logger() << "Saving '" << fileName << "'...\n";
  if(bytes.empty()) { LOG(LOG_ERR) << "nothing to save\n"; return false; }
  FILE *f=fopen(fileName, "wb");
  if(!f) { LOG(LOG_ERR) << "could not open \"" << fileName << "\"\n"; return false; }
  if(1 != fwrite(&bytes[0], bytes.size(), 1 , f)) { LOG(LOG_ERR) << "truncated fwrite\n"; fclose(f); return false; }
  fclose(f);

  FIT fit;
  std::time_t ct=0;
  CHECK_RETURN_FALSE(FIT::getCreationDate(bytes, ct));
  char tbuf[256];
  strftime(tbuf, sizeof(tbuf), "%d-%m-%Y %H:%M:%S", localtime(&ct));
  //LOG_VAR(tbuf);
  boost::filesystem::last_write_time(boost::filesystem::path(fileName), ct);

  return true;
}


// explicit template instantiations

template bool AntMessage::stringToBytes(const char *s, std::list<uchar>& bytes);
template bool AntMessage::stringToBytes(const char *s, std::vector<uchar>& bytes);
template bool AntMessage::saveAsUsbMon<std::list<AntMessage> >(const char* fileName, const std::list<AntMessage>& messages);
//template bool AntMessage::saveAsUsbMon<std::queue<AntMessage> >(const char* fileName, const std::queue<AntMessage>& messages);
template bool AntMessage::saveAsUsbMon<std::vector<AntMessage> >(const char* fileName, const std::vector<AntMessage>& messages);
template bool AntMessage::saveAsUsbMon<std::list<AntMessage> >(std::ostream& os, const std::list<AntMessage>& messages);
template bool AntMessage::saveAsUsbMon<std::vector<AntMessage> >(std::ostream& os, const std::vector<AntMessage>& messages);

template bool AntMessage::saveAsAntParse<std::list<AntMessage> >(const char* fileName, const std::list<AntMessage>& messages);
//template bool AntMessage::saveAsAntParse<std::queue<AntMessage> >(const char* fileName, const std::queue<AntMessage>& messages);
template bool AntMessage::saveAsAntParse<std::vector<AntMessage> >(const char* fileName, const std::vector<AntMessage>& messages);

template bool AntMessage::saveAsAntParse<std::list<AntMessage> >(std::ostream& os, const std::list<AntMessage>& messages);
//template bool AntMessage::saveAsAntParse<std::queue<AntMessage> >(std::ostream& os, const std::queue<AntMessage>& messages);
template bool AntMessage::saveAsAntParse<std::vector<AntMessage> >(std::ostream& os, const std::vector<AntMessage>& messages);

}
