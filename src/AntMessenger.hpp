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
#include "AntMessage.hpp"
#include "AntChannel.hpp"
#include "lqueue.hpp"
#include <list>
#include "SmartPtrFwd.hpp"
#include "Serial.hpp"
#include <boost/thread.hpp>





namespace antpm{

// Interface for delivering event of sent/received ANT+ messages. MUST be thread safe.
class AntCallback
{
public:
  virtual ~AntCallback() {}
  virtual void onAntReceived(const AntMessage m) {}
  virtual void onAntSent(const AntMessage m) {}
};


class AntLoggerCallback : public AntCallback
{
public:
  virtual ~AntLoggerCallback() {}
  virtual void onAntReceived(const AntMessage m) {}
  virtual void onAntSent(const AntMessage m) {}
protected:
private:
};


class AntUsbMonLoggerCallback : public AntCallback
{
public:
  AntUsbMonLoggerCallback(const std::string& s) : m_logFileName(s) {}
  virtual ~AntUsbMonLoggerCallback() { AntMessage::saveAsUsbMon(m_logFileName.c_str(), m_l); }
  virtual void onAntReceived(const AntMessage m) {m_l.push_back(m);}
  virtual void onAntSent(const AntMessage m) {m_l.push_back(m);}
protected:
private:
  std::string m_logFileName;
  std::list<AntMessage> m_l;
};

class AntParsedLoggerCallback : public AntCallback
{
public:
  AntParsedLoggerCallback(const std::string& s) : m_logFileName(s), cnt(0)
  {
    LOG(LOG_DBG3) << "Protocol log file: " << m_logFileName << "\n";
  }
  virtual ~AntParsedLoggerCallback()
  {
    AntMessage::saveAsAntParse(m_logFileName.c_str(), m_l.getListCopy());
    m_l.clear();
  }
  virtual void onAntReceived(const AntMessage m) {m_l.push(m); saveIfNeeded(); }
  virtual void onAntSent(const AntMessage m) {m_l.push(m); saveIfNeeded(); }
protected:
  void saveIfNeeded()
  {
    if(++cnt%10==0)
    {
      LOG(LOG_DBG3) << "cnt=" << cnt << " saving protocol log\n";
      cnt=0;
      AntMessage::saveAsAntParse(m_logFileName.c_str(), m_l.getListCopy());
      m_l.clear();
    }
  }
private:
  std::string m_logFileName;
  //std::list<AntMessage> m_l;
  lqueue2<AntMessage>m_l;
  int cnt;
};







struct AntMessenger_Recevier;
// Deals with sending/receiving ANT+ messages. Performs message framing.
class AntMessenger
{
public:
  AntMessenger();
  ~AntMessenger();
  void setHandler(Serial* io){m_io=io;}
  void setCallback(AntCallback* cb){m_cb=cb;}
  //void addCallback(std::shared_ptr<AntCallback> cb) { m_cbs.push_back(cb); }
  size_t getQueueLength() const;
  std::list<AntMessage> getQueue();

  bool ANT_ResetSystem();
  bool ANT_SetNetworkKey(const unsigned char net, const unsigned char key[8]);
  bool ANT_AssignChannel(uchar chan, uchar chtype, uchar net);
  bool ANT_SetChannelMessagePeriod(uchar chan, ushort msgPeriod);
  bool ANT_SetChannelSearchTimeout(uchar chan, uchar searchTimeout);
  bool ANT_SetChannelRadioFreq(uchar chan, uchar freq);
  bool ANT_SetSearchWaveform(uchar chan, ushort wave);
  bool ANT_SetChannelId(const uchar chan, const ushort devNum, const uchar devId, const uchar transType);
  bool ANT_OpenChannel(uchar chan);
  bool ANT_CloseChannel(uchar chan, const size_t timeout_ms = 1000);
  bool ANT_RequestMessage(uchar chan, uchar reqMsgId);
  bool ANT_GetChannelId(const uchar chan, ushort* devNum, uchar* devId, uchar* transType, const size_t timeout_ms = 0);
  bool ANT_SendAcknowledgedData(const uchar chan, const uchar data[8], const size_t timeout_ms = 0);
  bool ANT_SendBurstData(const uchar seqchan, const uchar data[8]);
  bool ANT_SendBurstData2(const uchar chan, const uchar* data, const size_t len);
  bool ANT_SendBurstData2(const uchar chan, const std::vector<uchar>& data);

  bool ANTFS_Link(const uchar chan, const uchar freq, const uchar beaconPer, const uint hostSN);
  bool ANTFS_Disconnect(const uchar chan);
  bool ANTFS_Pairing(const uchar chan, const uint hostSN, const std::string& name1, uint& unitId, uint64_t& key);
  bool ANTFS_Authenticate(const uchar chan, const uint hostSN, const uint64_t pairedKey);
  bool ANTFS_Download(const uchar chan, const ushort file, std::vector<uchar>& data);
  //bool ANTFS_Upload(const uchar chan);
  bool ANTFS_Erase(const uchar chan, const ushort file);
  bool ANTFS_RequestClientDeviceSerialNumber(const uchar chan, const uint hostSN, uint& sn, std::string& devName);

  bool ANTFS_Direct(const uchar chan, const uint64_t code, std::vector<uint8_t> &bytes);

  void eventLoop();
  void kill();
private:
  bool sendCommand(uchar mesg, uchar *inbuf, uchar len, const size_t timeout_ms = 0);
  bool sendCommand(AntMessage& m, const size_t timeout_ms = 0);
  bool sendRequest(uchar reqMsgId, uchar chan, AntMessage* response = NULL, const size_t timeout_ms = 0);
  bool writeMessage(uchar mesg, uchar *inbuf, uchar len);
  bool writeMessage(AntMessage& m);
  bool sendAckData(const uchar chan, const uchar data[8], const size_t timeout_ms = 0);

  bool assemblePackets(std::list<uchar>& q);

  bool onMessage(std::vector<AntMessage> v);

  void sanityCheck(const char* caller);
  
public:
  bool waitForBurst(const uchar chan, std::vector<uchar>& burstData, const size_t timeout_ms = 30000);
  bool waitForBroadcast(const uchar chan, AntMessage* reply = NULL, const size_t timeout_ms = 2000);
  void interruptWait();
//public:
private:
  Serial* m_io;
  AntCallback* m_cb;
  //std::vector<std::shared_ptr<AntCallback>> m_cbs;
  boost::thread m_packerTh; // thread to reconstruct messages from bytes flowing in
  volatile int m_packerThKill;

  // received packet queue
  lqueue3<AntMessage> m_rpackQueue2;
  volatile size_t packetIdx;
private:
  std::vector<std::unique_ptr<AntChannel>> chs;
  //AntChannel chs[ANTPM_MAX_CHANNELS];
  friend struct AntMessenger_Recevier;
  void* th_messageHandler(); // PUBLIC on purpose
};



}








