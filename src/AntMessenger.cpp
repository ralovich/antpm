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

#include "AntMessenger.hpp"
#include "antdefs.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <functional>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include "common.hpp"
#include <boost/thread/thread_time.hpp>
#include <boost/foreach.hpp>

#include "Log.hpp"



namespace antpm{

// runs in other thread
struct AntMessenger_Recevier
{
  void operator() (AntMessenger* arg)
  {
    //printf("msgFunc, arg: %p\n", arg); fflush(stdout);
    if(!arg)
    {
      rv=0;
      return;
    }
    AntMessenger* This = reinterpret_cast<AntMessenger*>(arg);
    //printf("msgFunc, This: %p\n", This); fflush(stdout);
    rv = This->th_messageHandler();
  }
  void* rv;
};


AntMessenger::AntMessenger(bool eventLoopInBgTh)
  : m_io(0)
  , m_cb(0)
  , m_packerThKill(0)
  , m_rpackQueue2(eventLoopInBgTh)
{
  m_packerThKill = 0;
  AntMessenger_Recevier msgTh;
  msgTh.rv=0;
  m_packerTh = boost::thread(msgTh, this);

  m_rpackQueue2.setOnDataArrivedCallback(std::bind1st(std::mem_fun(&AntMessenger::onMessage), this));

  for(int i=0; i < ANTPM_MAX_CHANNELS; i++)
    chs[i].chan = i;

  packetIdx=0;
}

AntMessenger::~AntMessenger()
{
  m_packerThKill = 1;
  m_packerTh.join();
  m_io=0;
  m_cb=0;
}

size_t AntMessenger::getQueueLength() const
{
  return m_rpackQueue2.size();
}

std::list<AntMessage> AntMessenger::getQueue()
{
  return m_rpackQueue2.getListCopy();
}


bool
AntMessenger::ANT_ResetSystem()
{
  uchar filler = 0;
  bool rv = writeMessage(MESG_SYSTEM_RESET_ID, &filler, 1);
  sleepms(1000);
  return rv;
}


bool
AntMessenger::ANT_SetNetworkKey(const unsigned char net, const unsigned char key[8])
{
  uchar buf[9];

  buf[0] = net;
  memcpy(buf+1, key, 8);
  bool rv = sendCommand(MESG_NETWORK_KEY_ID, buf, sizeof(buf), 2000);
  return rv;
}

bool AntMessenger::ANT_AssignChannel(uchar chan, uchar chtype, uchar net)
{
  AntMessage m(MESG_ASSIGN_CHANNEL_ID, chan, chtype, net);
  return sendCommand(m, 2000);
}

bool AntMessenger::ANT_SetChannelMessagePeriod(uchar chan, ushort msgPeriod)
{
  AntMessage m(MESG_CHANNEL_MESG_PERIOD_ID, chan, msgPeriod%256, msgPeriod/256);
  return sendCommand(m, 2000);
}

bool AntMessenger::ANT_SetChannelSearchTimeout(uchar chan, uchar searchTimeout)
{
  AntMessage m(MESG_CHANNEL_SEARCH_TIMEOUT_ID, chan, searchTimeout);
  return sendCommand(m, 2000);
}

bool AntMessenger::ANT_SetChannelRadioFreq(uchar chan, uchar freq)
{
  AntMessage m(MESG_CHANNEL_RADIO_FREQ_ID, chan, freq);
  return sendCommand(m, 2000);
}

bool AntMessenger::ANT_SetSearchWaveform(uchar chan, ushort wave)
{
  AntMessage m(MESG_SEARCH_WAVEFORM_ID, chan, wave/256, wave%256);
  return sendCommand(m, 2000);
}

bool AntMessenger::ANT_SetChannelId(const uchar chan, const ushort devNum, const uchar devId, const uchar transType)
{
  uchar buf[5];

  buf[0] = chan;
  buf[1] = devNum/256;
  buf[2] = devNum%256;
  buf[3] = devId;
  buf[4] = transType;
  bool rv = sendCommand(MESG_CHANNEL_ID_ID, buf, sizeof(buf), 2000);
  return rv;
}


bool
AntMessenger::ANT_OpenChannel(uchar chan)
{
  AntMessage m(MESG_OPEN_CHANNEL_ID, chan);
  return sendCommand(m, 2000);
}


bool
AntMessenger::ANT_CloseChannel(uchar chan, const size_t timeout_ms)
{
  AntMessage m(MESG_CLOSE_CHANNEL_ID, chan);
  bool rv = sendCommand(m, timeout_ms/2);
  // TODO: read R[ 7] a4_03_40_00_01_07_e1                   MESG_RESPONSE_EVENT_ID chan=0x00 mId=MESG_EVENT_ID mCode=EVENT_CHANNEL_CLOSED

  AntChannel& pc = chs[chan];
  AntEvListener el(pc);
  //pc.addEvListener(&el);

  uint8_t msgCode;
  rv = rv && el.waitForEvent(msgCode, timeout_ms/2);
  rv = rv && (msgCode==EVENT_CHANNEL_CLOSED);

  //pc.rmEvListener(&el);

  return rv;
}


bool
AntMessenger::ANT_RequestMessage(uchar chan, uchar reqMsgId)
{
  AntMessage response;
  return sendRequest(reqMsgId, chan, &response, 2000);
}

bool AntMessenger::ANT_GetChannelId(const uchar chan, ushort *devNum, uchar *devId, uchar *transType, const size_t timeout_ms)
{
  AntMessage response;
  if(!sendRequest(MESG_CHANNEL_ID_ID, chan, &response, timeout_ms))
    return false;
  if(response.getLenRaw()<1 || response.getLenPayload()!=5)
    return false;
  const M_ANT_Channel_Id* mesg(reinterpret_cast<const M_ANT_Channel_Id*>(response.getPayloadRef()));
  if(chan != mesg->chan)
    return false;
  if(devNum)
    *devNum = mesg->devNum;
  if(devId)
    *devId = mesg->devId;
  if(transType)
    *transType = mesg->transType;
  return true;
}


bool
AntMessenger::ANT_SendAcknowledgedData(const uchar chan, const uchar data[8], const size_t timeout_ms)
{
  return sendAckData(chan, data, timeout_ms);
}

bool AntMessenger::ANT_SendBurstData(const uchar seqchan, const uchar data[8])
{
  uchar buf[9];
  buf[0] = seqchan;
  memcpy(buf+1, data, 8);

  AntMessage m;
  if(!m.assemble(MESG_BURST_DATA_ID, buf, sizeof(buf)))
    return false;

  return writeMessage(m);
}


bool
AntMessenger::ANT_SendBurstData2(const uchar chan, const uchar* data, const size_t len)
{
  CHECK_RETURN_FALSE(len>0);
  CHECK_RETURN_FALSE(len % 8 == 0);

  std::vector<uchar> vdata(len);
  for (size_t i=0; i<len; i++)
  {
    vdata[i]=data[i];
  }
  
  return ANT_SendBurstData2(chan, vdata);
}


bool
AntMessenger::ANT_SendBurstData2(const uchar chan, const std::vector<uchar>& data)
{
  CHECK_RETURN_FALSE(!data.empty());
  CHECK_RETURN_FALSE(data.size() % 8 == 0);

  M_ANT_Burst burst;
  uchar seq=0;
  for(size_t start=0; start<data.size(); start += 8)
  {
    burst.chan = chan;
    burst.seq  = seq;
    burst.last = (start+8==data.size());
    //LOG_VAR(burst.toString());
    CHECK_RETURN_FALSE(ANT_SendBurstData(burst.seqchan, &data[start]));
    if(start==0 && data.size()>8)
    {
      //LOG_VAR(waitForBroadcast(chan,0,800));
      CHECK_RETURN_FALSE(waitForBroadcast(chan,0,800));
      //sleepms(10);
    }
    ++seq;
    if(seq==4)
      seq=1;
  }
  
  return true;
}


bool
AntMessenger::ANTFS_Link( const uchar chan, const uchar freq, const uchar beaconPer, const uint hostSN )
{
  //S   3.536 4f MESG_ACKNOWLEDGED_DATA_ID chan=00 ANTFS_CMD(0x44) ANTFS_CmdLink freq=0f, period=04, SN=0x7c9101e0
  //R 113.467 4e MESG_BROADCAST_DATA_ID chan=00 ANTFS_BEACON(0x43) Beacon=8Hz, pairing=enabled, upload=disabled, dataAvail=yes, State=Link, Auth=PasskeyAndPairingOnly
  //R   1.975 40 MESG_RESPONSE_EVENT_ID chan=00 mId=UNKNOWN_0x01 mCode=EVENT_TRANSFER_TX_COMPLETED



  bool success=false;

  for (int i=0; i < ANTPM_RETRIES; i++)
  {
    M_ANTFS_Command cmd;
    cmd.commandId       = ANTFS_CommandResponseId;
    cmd.command         = ANTFS_CmdLink;
    cmd.detail.link.chanFreq   = freq;
    cmd.detail.link.chanPeriod = beaconPer;
    cmd.detail.link.sn         = hostSN;

    if(!ANT_SendAcknowledgedData(chan, reinterpret_cast<uchar*>(&cmd), 2000))
      continue;

    success=true;
    break;
  }

  return success;
}


bool
AntMessenger::ANTFS_Disconnect(const uchar chan)
{
  M_ANTFS_Command cmd;
  cmd.commandId = ANTFS_CommandResponseId;
  cmd.command   = ANTFS_CmdDisconnect;
  cmd.detail.disconnect.cmdType = M_ANTFS_Command::ReturnToLinkLayer;

  /*CHECK_RETURN_FALSE_LOG_OK(*/ANT_SendAcknowledgedData(chan, reinterpret_cast<uchar*>(&cmd), 400)/*)*/;

  return true;
}


bool
AntMessenger::ANTFS_Pairing(const uchar chan, const uint hostSN, const std::string& name1, uint& unitId, uint64_t& key)
{
  std::string name=(name1.length()>8?name1.substr(0, 8):name1);
  while(name.length()<8) { name.push_back(' '); }
  assert(name.length()==8);
  M_ANTFS_Command_Pairing cmd;
  cmd.commandId            = ANTFS_CommandResponseId;
  cmd.command              = ANTFS_CmdAuthenticate;
  cmd.detail.authenticate.cmdType = M_ANTFS_Command::RequestPairing;
  cmd.detail.authenticate.authStrLen = 8;
  cmd.detail.authenticate.sn      = hostSN;
  memcpy(cmd.name, &name[0], 8);

  AntChannel& pc(chs[chan]);
  AntBurstListener bl(pc);
  //pc.addMsgListener(&bl);

  bool sentPairing = false;
  for(int i = 0; i < ANTPM_RETRIES; i++)
  {
    sentPairing = false;

    LOG_VAR(waitForBroadcast(chan));

    //CHECK_RETURN_FALSE_LOG_OK(collectBroadcasts(chan));
    sentPairing = ANT_SendBurstData2(chan, reinterpret_cast<uchar*>(&cmd), sizeof(cmd));

    // TODO: read bcast here?
    //AntMessage reply0;
    //waitForMessage(MESG_RESPONSE_EVENT_ID, &reply0, 2000);

    AntChannel& pc(chs[chan]);
    AntEvListener el(pc);
    //pc.addEvListener(&el);

    uint8_t responseVal;
    sentPairing = sentPairing && el.waitForEvent(responseVal, 800);
    //pc.rmEvListener(&el);
    sentPairing = sentPairing && (responseVal==EVENT_TRANSFER_TX_COMPLETED);

    if(sentPairing)
      break;
    else
      sleepms(ANTPM_RETRY_MS);
  }
  CHECK_RETURN_FALSE_LOG_OK(sentPairing);

  // FIXME: look for, and handle event:EVENT_RX_FAIL; event:EVENT_TRANSFER_RX_FAILED

  // ANTFS_RespAuthenticate
  std::vector<uchar> burstData;
  /*bool rv =*/ bl.collectBurst(burstData, 30*1000); // 30s to allow user confirmation
  //pc.rmMsgListener(&bl);

  CHECK_RETURN_FALSE_LOG_OK(burstData.size()==3*8);
  const M_ANTFS_Response_Pairing* resp(reinterpret_cast<const M_ANTFS_Response_Pairing*>(&burstData[8]));
  CHECK_RETURN_FALSE_LOG_OK(resp->responseId==ANTFS_CommandResponseId);
  CHECK_RETURN_FALSE_LOG_OK(resp->response==ANTFS_RespAuthenticate);
  CHECK_RETURN_FALSE_LOG_OK(resp->detail.authenticateResponse.respType==1); // accept
  LOG_VAR((int)resp->detail.authenticateResponse.authStrLen);
  CHECK_RETURN_FALSE(resp->detail.authenticateResponse.authStrLen==8);

  unitId = resp->detail.authenticateResponse.sn;
  key    = resp->pairedKey;

  //printf("unitId=%d, key=0x016x\n", unitId, key); fflush(stdout);

  // TODO: read bcast here?


  return true;
}


bool
AntMessenger::ANTFS_Authenticate(const uchar chan, const uint hostSN, const uint64_t pairedKey)
{
  //S   2.099 50 MESG_BURST_DATA_ID chan=0x00, seq=0, last=no  ANTFS_CMD(0x44) ANTFS_CmdAuthenticate type=RequestPasskeyExchange, authStrLen=8, SN=0x7c9101e0
  //R 100.875 4e MESG_BROADCAST_DATA_ID chan=00 ANTFS_BEACON(0x43) Beacon=8Hz, pairing=enabled, upload=disabled, dataAvail=yes, State=Authentication, Auth=PasskeyAndPairingOnly
  //S   1.977 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  d273f79a6f166fa5 .s..o.o.
  //S   3.035 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=yes 0000000000000000 ........
  //R   9.986 40 MESG_RESPONSE_EVENT_ID chan=00 mId=MESG_EVENT_ID mCode=EVENT_TRANSFER_TX_COMPLETED
  //R 110.037 50 MESG_BURST_DATA_ID chan=0x00, seq=0, last=no  ANTFS_BEACON(0x43) Beacon=8Hz, pairing=enabled, upload=disabled, dataAvail=yes, State=Authentication, Auth=PasskeyAndPairingOnly
  //R    3.99 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=yes ANTFS_RESP(0x44) ANTFS_RespAuthenticate resp=accept, authStrLen=0, SN=0x00000000
  //R 120.998 4e MESG_BROADCAST_DATA_ID chan=00 ANTFS_BEACON(0x43) Beacon=8Hz, pairing=enabled, upload=disabled, dataAvail=yes, State=Transport, Auth=PasskeyAndPairingOnly
  //S  96.065 4d MESG_REQUEST_ID chan=00 reqMsgId=MESG_CHANNEL_STATUS_ID
  //R    3.91 52 MESG_CHANNEL_STATUS_ID chan=00 chanSt=Tracking
  M_ANTFS_Command_Authenticate cmd;
  cmd.commandId            = ANTFS_CommandResponseId;
  cmd.command              = ANTFS_CmdAuthenticate;
  cmd.detail.authenticate.cmdType = M_ANTFS_Command::RequestPasskeyExchange;
  cmd.detail.authenticate.authStrLen = 8;
  cmd.detail.authenticate.sn      = hostSN;
  cmd.key                  = pairedKey;
  cmd.footer               = 0x0000000000000000;

  //CHECK_RETURN_FALSE_LOG_OK(waitForBroadcast(chan));

  // TODO: start listening for burst here!

  bool sentReqAuth = false;
  for(int i = 0; i < ANTPM_RETRIES; i++)
  {
    sentReqAuth = false;

    LOG_VAR(waitForBroadcast(chan));

    //CHECK_RETURN_FALSE_LOG_OK(collectBroadcasts(chan));
    sentReqAuth = ANT_SendBurstData2(chan, reinterpret_cast<uchar*>(&cmd), sizeof(cmd));

    // TODO: read bcast here?
    //AntMessage reply0;
    //waitForMessage(MESG_RESPONSE_EVENT_ID, &reply0, 2000);

    AntChannel& pc(chs[chan]);
    AntEvListener el(pc);
    //pc.addEvListener(&el);

    uint8_t responseVal;
    sentReqAuth = sentReqAuth && el.waitForEvent(responseVal, 800);
    //pc.rmEvListener(&el);
    sentReqAuth = sentReqAuth && (responseVal==EVENT_TRANSFER_TX_COMPLETED);

    if(sentReqAuth)
      break;
    else
      sleepms(ANTPM_RETRY_MS);
  }
  CHECK_RETURN_FALSE_LOG_OK(sentReqAuth);



  // ANTFS_RespAuthenticate
  std::vector<uchar> burstData;
  CHECK_RETURN_FALSE_LOG_OK(waitForBurst(chan, burstData, 10*1000));
  CHECK_RETURN_FALSE_LOG_OK(burstData.size()==2*8);
  const M_ANTFS_Response* resp(reinterpret_cast<const M_ANTFS_Response*>(&burstData[8]));
  CHECK_RETURN_FALSE_LOG_OK(resp->responseId==ANTFS_CommandResponseId);
  CHECK_RETURN_FALSE_LOG_OK(resp->response==ANTFS_RespAuthenticate);
  CHECK_RETURN_FALSE_LOG_OK(resp->detail.authenticateResponse.respType==1); // accept

  // TODO: read bcast here?

  return true;
}



bool
AntMessenger::ANTFS_Download( const uchar chan, const ushort file, std::vector<uchar>& data )
{
  //S   1.221 50 MESG_BURST_DATA_ID chan=0x00, seq=0, last=no  ANTFS_CMD(0x44) ANTFS_ReqDownload file=0x0000, dataOffset=0x00000000
  //R 118.782 4e MESG_BROADCAST_DATA_ID chan=0x00 ANTFS_BEACON(0x43) Beacon=8Hz, pairing=enabled, upload=disabled, dataAvail=yes, State=Transport, Auth=PasskeyAndPairingOnly
  //S   1.258 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=yes 0001000000000000 ........
  //R  10.728 40 MESG_RESPONSE_EVENT_ID chan=0x00 mId=MESG_EVENT_ID mCode=EVENT_TRANSFER_TX_COMPLETED
  //R 112.995 4e MESG_BROADCAST_DATA_ID chan=0x00 ANTFS_BEACON(0x43) Beacon=8Hz, pairing=enabled, upload=disabled, dataAvail=yes, State=Busy, Auth=PasskeyAndPairingOnly
  //R 125.019 50 MESG_BURST_DATA_ID chan=0x00, seq=0, last=no  ANTFS_BEACON(0x43) Beacon=8Hz, pairing=enabled, upload=disabled, dataAvail=yes, State=Busy, Auth=PasskeyAndPairingOnly
  //R   3.994 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  ANTFS_RESP(0x44) ANTFS_RespDownload resp=DownloadRequestOK, remainingBytes=0x00000200
  //R   3.002 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  00000000c0030000 ........
  //R   2.999 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  0110000000000000 ........
  //R   2.982 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  0000000000000000 ........
  //R       3 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  0100010c00000050 .......P
  //R   6.989 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  00001d0000000000 ........
  //R   1.998 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  0200010d00000030 .......0
  //R       3 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  03008001ffff0090 ........
  //R   4.002 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  4602000000000000 F.......
  //R   2.998 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  04008002ffff00d0 ........
  //R   3.002 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  3602000000000000 6.......
  //R   3.022 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  05008003020000d0 ........
  //R   2.991 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  8204000000000000 ........
  //R   2.998 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  06008003000000d0 ........
  //R   3.004 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  8204000000000000 ........
  //R   2.999 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  07008003010000d0 ........
  //R   2.998 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  8204000000000000 ........
  //R   3.987 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  08008004180000b0 ........
  //R   3.012 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  e84c00007610d229 .L..v..)
  //R       3 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  09008004190000b0 ........
  //R   3.002 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  8a6c00007810d229 .l..x..)
  //R   2.999 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  0a0080041a0000b0 ........
  //R   2.999 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  943f00007810d229 .?..x..)
  //R   3.001 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  0b0080041b0000b0 ........
  //R   3.016 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  3d2500007a10d229 =%..z..)
  //R   3.981 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  0c0080041c0000b0 ........
  //R   2.999 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  0c2b00007a10d229 .+..z..)
  //R   3.001 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  0d0080041d0000b0 ........
  //R   3.018 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  ad2e00007a10d229 ....z..)
  //R   2.961 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  0e0080041e0000b0 ........
  //R   3.041 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  8c2400007c10d229 .$..|..)
  //R   2.998 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  0f0080041f0000b0 ........
  //R       3 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  d84d00007c10d229 .M..|..)
  //R   2.995 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  10008004200000b0 .... ...
  //R   4.008 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  442f00007e10d229 D/..~..)
  //R   2.978 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  11008004220000b0 ...."...
  //R   3.021 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  dc2c00008010d229 .,.....)
  //R   2.981 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  12008004230000b0 ....#...
  //R   2.999 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  066f00008010d229 .o.....)
  //R   3.018 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  13008004240000b0 ....$...
  //R       3 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  865100008210d229 .Q.....)
  //R   3.003 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  14008004210000b0 ....!...
  //R       3 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  5a2f00007e10d229 Z/..~..)
  //R   3.999 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  15008004250000b0 ....%...
  //R   2.998 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  232d00008210d229 #-.....)
  //R   3.002 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  16008004260000b0 ....&...
  //R   3.001 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  b09900008410d229 .......)
  //R    3.98 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  17008004270000b0 ....'...
  //R   2.005 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  803000008410d229 .0.....)
  //R   3.013 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  18008004280000b0 ....(...
  //R   3.002 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  bf2900008610d229 .).....)
  //R       4 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  19008004290000b0 ....)...
  //R   2.977 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  d01800008610d229 .......)
  //R   3.021 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  1a008004170000b0 ........
  //R   3.002 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  72250000f2acd429 r%.....)
  //R   2.996 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  1b0080042a0000b0 ....*...
  //R       3 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  592d000049b5d429 Y-..I..)
  //R   3.983 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  1c0080042b0000b0 ....+...
  //R   3.007 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  022f00007f54d729 ./...T.)
  //R   1.998 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  1d0080042c0000b0 ....,...
  //R   4.016 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  04530000e9b6d929 .S.....)
  //R   3.001 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  1e0080042d0000b0 ....-...
  //R   2.997 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  5c2a000031fbdd29 \*..1..)
  //R   3.001 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  1f0080042e0000b0 ........
  //R   2.988 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=no  373400006932e329 74..i2.)
  //R   2.998 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=yes 000000000000105b .......[
  //R  41.018 4e MESG_BROADCAST_DATA_ID chan=0x00 000000000000105b .......[
  //S   2.888 4d MESG_REQUEST_ID chan=0x00 reqMsgId=MESG_CHANNEL_STATUS_ID
  //R   3.109 52 MESG_CHANNEL_STATUS_ID chan=00 chanSt=Tracking
  CHECK_RETURN_FALSE(data.empty());

  int errorCnt=0;

  //uint bytesReceived = 0;
  ushort crc = 0x0000;
  size_t fileSize = 0;
  size_t dlIter = 0;
  uint nextOffset = 0;
  do
  {
    //fprintf(loggerc(), "dlIter=%lu, crc=0x%04x, off=0x%08x\n", (unsigned long)dlIter, crc, nextOffset);
    //LOG_VAR2(dlIter, toString(crc,4,'0'));
    M_ANTFS_Command_Download dl;
    dl.commandId = ANTFS_CommandResponseId;
    dl.command   = ANTFS_ReqDownload;
    dl.detail.downloadRequest.dataFileIdx = file;
    dl.detail.downloadRequest.dataOffset = nextOffset;
    dl.zero = 0;
    dl.initReq = (dlIter==0);
    dl.crcSeed = crc;
    dl.maxBlockSize = 0;

    AntChannel& pc(chs[chan]);
    AntBurstListener bl(pc);
    //pc.addMsgListener(&bl); // FIXME: remove this on return paths, IN DTOR!!!

    AntEvListener el(pc);
    //pc.addEvListener(&el); // FIXME: remove this on return paths, IN DTOR!!!

    bool sentReqDl = false;
    sentReqDl = false;
    //if(dlIter==0)
    //  CHECK_RETURN_FALSE_LOG_OK(collectBroadcasts(chan));
    sentReqDl = ANT_SendBurstData2(chan, reinterpret_cast<uchar*>(&dl), sizeof(dl));
    if(!sentReqDl && (++errorCnt<ANTPM_RETRIES))
    {
      //pc.rmEvListener(&el);
      //pc.rmMsgListener(&bl);
      continue;
    }

    uint8_t responseVal;
    sentReqDl = sentReqDl && el.waitForEvent(responseVal, 600);
    //pc.rmEvListener(&el);
    sentReqDl = sentReqDl && (responseVal==EVENT_TRANSFER_TX_COMPLETED);

    // TODO: handle event:EVENT_RX_FAIL and continue

    if(!sentReqDl && (++errorCnt<ANTPM_RETRIES))
    {
      //pc.rmMsgListener(&bl);
      continue;
    }

    // TODO: read bcast here?

    std::vector<uchar> burstData;
    if(!bl.collectBurst(burstData, 10000) && (++errorCnt<ANTPM_RETRIES))
    {
      //pc.rmMsgListener(&bl);
      continue;
    }
    //pc.rmMsgListener(&bl);

    //// ANTFS_RespDownload
    CHECK_RETURN_FALSE(burstData.size()>=3*8); // header and footer eats up 32 bytes already, but in case of error2 we only get 24 bytes
    const M_ANTFS_Response_Download* resp(reinterpret_cast<const M_ANTFS_Response_Download*>(&burstData[8]));
    CHECK_RETURN_FALSE(resp->responseId==ANTFS_CommandResponseId);
    CHECK_RETURN_FALSE(resp->response==ANTFS_RespDownload);
    if(resp->detail.downloadRequestResponse.responseVal!=M_ANTFS_Response::DownloadRequestOK)
    {
      const char* dlStatus=resp->detail.downloadRequestResponse.szResponseVal();
      LOG_VAR(dlStatus);
      if(resp->detail.downloadRequestResponse.responseVal==M_ANTFS_Response::CRCIncorrect)
      {
        //let's retry
        CHECK_RETURN_FALSE(waitForBroadcast(chan, NULL, 1000));
        CHECK_RETURN_FALSE(ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID));
        continue;//FIXME: this can be an endless loop
      }
      CHECK_RETURN_FALSE(resp->detail.downloadRequestResponse.responseVal==M_ANTFS_Response::DownloadRequestOK); // dl ok
    }

    //fprintf(loggerc(), "remaining bytes = %u = 0x%08x\n", resp->detail.downloadRequestResponse.remainingBytes, resp->detail.downloadRequestResponse.remainingBytes);
    nextOffset += resp->detail.downloadRequestResponse.remainingBytes;

    if(dlIter==0) fileSize = resp->fileSize;
    //fprintf(loggerc(), "fileSize = %u = 0x%08x\n", (uint)fileSize, (uint)fileSize);
    CHECK_RETURN_FALSE(fileSize == resp->fileSize);
    //fprintf(loggerc(), "nextOffset = %u = 0x%08x\n", (uint)nextOffset, (uint)nextOffset);
    //logger() << std::dec;
    //LOG_VAR(fileSize);
    //LOG_VAR2(burstData.size(), toString(nextOffset,8,'0'));
    uint c2=0;
    AntFsFile crcData;
    for(uint b=24; b<burstData.size()-8; b++)
    {
      uchar c = burstData[b];
      data.push_back(c);
      crcData.bytes.push_back(c);
      c2++;
    }
    //LOG_VAR3(burstData.size(), c2, data.size());
    CHECK_RETURN_FALSE(c2==burstData.size()-32);
    // if the file size is not a multiple of 8, in the last round we might have received more bytes and overfilled it
    if(fileSize%8!=0 && data.size()>fileSize)
    {
      logger() << "Truncating data buffer after overfill!\n";
      data.erase(data.begin()+fileSize, data.end());
    }

    const M_ANTFS_Response_Download_Footer* footer(reinterpret_cast<const M_ANTFS_Response_Download_Footer*>(&burstData[burstData.size()-8]));
    //LOG_VAR(toString(footer->crc,4,'0'));
    ushort crcCalculated=crcData.crc16Calc(crc);
    //LOG_VAR(toString(crcCalculated,4,'0'));
    bool crcCheckOk=(footer->crc==crcCalculated);
    CHECK_RETURN_FALSE_LOG_OK(crcCheckOk);
    crc = footer->crc;     //TODO: crc check

    CHECK_RETURN_FALSE(waitForBroadcast(chan, NULL, 1000));

    CHECK_RETURN_FALSE(ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID));

    LOG(antpm::LOG_RAW) << "\n\nFile " << toString(file,4,'0') << ", downloaded " << std::dec << c2 << " of " << fileSize << " bytes. Total " << data.size() << " downloaded.\n\n";
    // TODO: keep reading until there is data left
    dlIter += 1;
  } while((data.size()<fileSize) /*&& (dlIter++<ANTPM_RETRIES)*/ );

  return true;
}


bool
AntMessenger::ANTFS_Erase(const uchar chan, const ushort file)
{
  M_ANTFS_Command cmd;
  cmd.commandId = ANTFS_CommandResponseId;
  cmd.command   = ANTFS_ReqErase;
  cmd.detail.eraseRequest.dataFileIdx = file;

  for(int itry=0; itry < ANTPM_RETRIES; itry++)
  {
    AntChannel& pc(chs[chan]);
    AntBurstListener bl(pc);
    //pc.addMsgListener(&bl);

    CHECK_RETURN_FALSE_LOG_OK(ANT_SendAcknowledgedData(chan, reinterpret_cast<uchar*>(&cmd), 1500));

    std::vector<uchar> burstData;
    bool rv = bl.collectBurst(burstData, 6000);
    //pc.rmMsgListener(&bl);

    //TODO interpret bursted response
    //// M_ANTFS_Response for erase
    CHECK_RETURN_FALSE(burstData.size()>=2*8);
    const M_ANTFS_Response* resp(reinterpret_cast<const M_ANTFS_Response*>(&burstData[8]));
    CHECK_RETURN_FALSE(resp->responseId==ANTFS_CommandResponseId);
    CHECK_RETURN_FALSE(resp->response==ANTFS_RespErase);
    logger() << resp->toString() << "\n";

    rv = rv && (resp->detail.eraseRequestResponse.responseVal==0);
    if(rv)
      break;
  }

  return true;
}


bool
AntMessenger::ANTFS_RequestClientDeviceSerialNumber(const uchar chan, const uint hostSN, uint& sn, std::string& devName)
{
  //S   2.817 4f MESG_ACKNOWLEDGED_DATA_ID chan=00 ANTFS_CMD(0x44) ANTFS_CmdAuthenticate type=RequestClientDeviceSerialNumber, authStrLen=0, SN=0x7c9101e0
  //R 122.184 4e MESG_BROADCAST_DATA_ID chan=00 ANTFS_BEACON(0x43) Beacon=8Hz, pairing=enabled, upload=disabled, dataAvail=yes, State=Authentication, Auth=PasskeyAndPairingOnly
  //R   1.976 40 MESG_RESPONSE_EVENT_ID chan=00 mId=MESG_EVENT_ID mCode=EVENT_TRANSFER_TX_COMPLETED
  //R 123.022 50 MESG_BURST_DATA_ID chan=0x00, seq=0, last=no  ANTFS_BEACON(0x43) Beacon=8Hz, pairing=enabled, upload=disabled, dataAvail=yes, State=Authentication, Auth=PasskeyAndPairingOnly
  //R   3.977 50 MESG_BURST_DATA_ID chan=0x00, seq=1, last=no  ANTFS_RESP(0x44) ANTFS_RespAuthenticate resp=??, authStrLen=16, SN=0xd84e4cd4
  //R   3.018 50 MESG_BURST_DATA_ID chan=0x00, seq=2, last=no  466f726572756e6e Forerunn
  //R   3.003 50 MESG_BURST_DATA_ID chan=0x00, seq=3, last=yes 6572203331305854 er 310XT
  //S   8.095 4d MESG_REQUEST_ID chan=00 reqMsgId=MESG_CHANNEL_STATUS_ID
  //R   3.903 52 MESG_CHANNEL_STATUS_ID chan=00 chanSt=Tracking

  M_ANTFS_Command cmd;
  cmd.commandId = ANTFS_CommandResponseId;
  cmd.command   = ANTFS_CmdAuthenticate;
  cmd.detail.authenticate.cmdType = M_ANTFS_Command::RequestClientDeviceSerialNumber;
  cmd.detail.authenticate.authStrLen = 0;
  cmd.detail.authenticate.sn = hostSN;

  AntChannel& pc(chs[chan]);
  AntBurstListener bl(pc);
  //pc.addMsgListener(&bl);


  CHECK_RETURN_FALSE_LOG_OK(ANT_SendAcknowledgedData(chan, reinterpret_cast<uchar*>(&cmd), 2000));

  std::vector<uchar> burstData;
  /*bool rv =*/ bl.collectBurst(burstData, 5000);
  //pc.rmMsgListener(&bl);

  // TODO: interpret event:EVENT_TRANSFER_RX_FAILED as signal of failed bursting

  //// TODO: read bcast auth beacon
  //CHECK_RETURN_FALSE_LOG_OK(waitForBroadcast(chan));
  //
  //CHECK_RETURN_FALSE_LOG_OK(waitForBurst(chan, burstData, 30000));
  LOG_VAR(burstData.size());
  CHECK_RETURN_FALSE_LOG_OK(burstData.size()==4*8);

  const M_ANTFS_Response* cmdResp(reinterpret_cast<const M_ANTFS_Response*>(&burstData[8]));
  sn = cmdResp->detail.authenticateResponse.sn;
  uchar lenDevName=cmdResp->detail.authenticateResponse.authStrLen;
  CHECK_RETURN_FALSE_LOG_OK(lenDevName==16);

  devName = std::string(reinterpret_cast<const char*>(&burstData[16]), lenDevName);

  logger() << "devName = \"" << devName << "\"\n";

  CHECK_RETURN_FALSE_LOG_OK(ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID));

  return true;
}

void AntMessenger::eventLoop()
{
  m_rpackQueue2.eventLoop();
}

void AntMessenger::kill()
{
  m_rpackQueue2.kill();
}





bool
AntMessenger::sendCommand(uchar mesg, uchar *inbuf, uchar len, const size_t timeout_ms)
{
  AntMessage m;
  return m.assemble(mesg, inbuf, len) && sendCommand(m, timeout_ms);
}



bool
AntMessenger::sendCommand(AntMessage &m, const size_t timeout_ms)
{
  bool rv = writeMessage(m);
  if(!rv)
  {
    lprintf(antpm::LOG_ERR, "writeMessage failed\n");
    return false;
  }

  const uint8_t chan = m.getPayloadRef()[0];
  AntChannel& pc = chs[chan];
  AntRespListener respList(pc, m.getMsgId());
  //pc.addRespListener(&respList);

  uint8_t respVal;
  rv = rv&& respList.waitForResponse(respVal, timeout_ms);

  //pc.rmRespListener(&respList);

  return rv;
}

// request a message from peer
bool
AntMessenger::sendRequest(uchar reqMsgId, uchar chan, AntMessage *response, const size_t timeout_ms)
{
  AntMessage reqMsg(MESG_REQUEST_ID, chan, reqMsgId); // the request we're sending

  AntChannel& pc = chs[chan];
  AntReqListener rl(pc, reqMsgId, chan);
  //pc.addMsgListener(&rl);

  bool rv = writeMessage(reqMsg);
  if(!rv)
  {
    lprintf(antpm::LOG_ERR, "writeMessage failed\n");
    return false;
  }


  AntMessage dummy;if(!response) response=&dummy;
  rv = rv&& rl.waitForMsg(response, timeout_ms);

  //pc.rmMsgListener(&rl);

  return rv;
}


bool AntMessenger::writeMessage(unsigned char mesg, unsigned char *inbuf, unsigned char len)
{
  AntMessage m;
  return m.assemble(mesg, inbuf, len) && writeMessage(m);
}

bool AntMessenger::writeMessage(AntMessage &m)
{
  if(!m_io)
    return false;

  m.sent=true;
  m.timestamp = boost::get_system_time();
  m.idx = packetIdx++;
  assert(m.vrfChkSum());

  size_t bytesWritten=0;
  bool rv = m_io->write(reinterpret_cast<const char*>(&m.bytes[0]), m.getLenPacket(), bytesWritten);
  size_t targetBytes = static_cast<size_t>(m.getLenPayload())+4;
  if(bytesWritten!=targetBytes)
  {
    lprintf(antpm::LOG_ERR, "wrote %d instead of %d bytes\n", (int)bytesWritten, (int)targetBytes);
    rv = false;
  }

  if(m_cb && rv)
  {
    m_cb->onAntSent(m);
  }

  return rv;
}

bool
AntMessenger::sendAckData(const uchar chan, const uchar data[8], const size_t timeout_ms)
{
  uchar mesg=MESG_ACKNOWLEDGED_DATA_ID;
  uchar buf[9];
  buf[0] = chan;
  memcpy(buf+1, data, 8);

  AntMessage m;
  if(!m.assemble(mesg, buf, sizeof(buf)))
    return false;

  AntChannel& pc(chs[chan]);
  AntEvListener el(pc);
  //pc.addEvListener(&el);

  bool rv = writeMessage(m);
  if(!rv)
  {
    lprintf(antpm::LOG_ERR, "writeMessage failed\n");
    return false;
  }

  uint8_t responseVal;
  bool found = el.waitForEvent(responseVal, timeout_ms);
  //pc.rmEvListener(&el);
  found = found && (responseVal==EVENT_TRANSFER_TX_COMPLETED);
  //TODO: loop sending until responseVal==EVENT_TRANSFER_TX_COMPLETED

  //TODO: handle other events!!

    if(!found)
    {
      //lprintf(antpm::LOG_ERR, "no matching data ack before timeout\n"); fflush(stdout);
    }
    rv = rv && found;

  return rv;
}


// interpret byte stream, assemble packets and store results in \a m_rpackQueue
bool
AntMessenger::assemblePackets(std::list<uchar>& q)
{
  if(q.empty())
    return false;

  int nInterpreted=0;
  for(;;nInterpreted++)
  {

    AntMessage m;
    m.sent = false;
    m.bytes.resize(q.size());
    std::copy(q.begin(), q.end(), m.bytes.begin());

    bool cantInterpretMore = m.interpret();
    if(!cantInterpretMore)
    {
      if(nInterpreted<1)
      {
        lprintf(LOG_ERR, "interpret failed!\n"); fflush(stdout);
        return false;
      }
      break;
    }
    for(size_t i=0; i<m.getLenPacket(); i++)
    {
      q.pop_front();
    }
    m.timestamp = boost::get_system_time();
    m.idx = packetIdx++;
    //{
    //  boost::unique_lock<boost::mutex> lock(m_rpackMutex);
    //  m_rpackQueue.push_back(m);
    //}
    m_rpackQueue2.push(m);
    if(m_cb)
    {
      m_cb->onAntReceived(m);
    }

  }
  //fprintf(loggerc(), "%d interpreted\n", nInterpreted);

  return true;
}

bool AntMessenger::onMessage(std::vector<AntMessage> v)
{
  //TODO: don't presort here, but call onMsg for all incoming packets

  //fprintf(loggerc(), "%d\n", int(v.size()));
  for(size_t i = 0; i < v.size(); i++)
  {
    AntMessage& m(v[i]);
    lprintf(antpm::LOG_DBG3, "%s\n", m.str().c_str());

    if(m.getMsgId()==MESG_RESPONSE_EVENT_ID
       || m.getMsgId()==MESG_BROADCAST_DATA_ID
       || m.getMsgId()==MESG_CHANNEL_ID_ID
       || m.getMsgId()==MESG_CHANNEL_STATUS_ID)
    {
      uint8_t chan=m.getPayloadRef()[0];
      AntChannel& pc=chs[chan];
      pc.onMsg(m);
    }
    else if(m.getMsgId()==MESG_BURST_DATA_ID)
    {
      if(m.getLenPayload()!=9)
        continue; // invalid packet
      const M_ANT_Burst* burst(reinterpret_cast<const M_ANT_Burst*>(m.getPayloadRef()));
      uint8_t chan=burst->chan;
      //chan = 0; // FIXME!!!
      //printf("burst? 0x%0x chan=%d\n", (int)m.getMsgId(), int(chan));
      AntChannel& pc=chs[chan];
      pc.onMsg(m);
    }
    else
    {
      lprintf(antpm::LOG_WARN, "unhandled 0x%0x\n", (int)m.getMsgId());
    }

  }
  return true;
}


bool
AntMessenger::waitForBurst(const uchar chan,
                           std::vector<uchar>& burstData,
                           const size_t timeout_ms)
{
  // read seq=0
  // keep reading ...
  // read last one

  //uchar expectedSeq=0;
  bool found=false;
  bool lastFound=false;

  AntChannel& pc(chs[chan]);
  AntBurstListener bl(pc);
  //pc.addMsgListener(&bl);
  bool rv = bl.collectBurst(burstData, timeout_ms);
  //pc.rmMsgListener(&bl);
  found = lastFound = rv;
  if(!found || !lastFound)
  {
    lprintf(antpm::LOG_ERR, "couldn't reconstruct burst data transmission before timeout\n"); fflush(stdout);
    return false;
  }

  return true;
}


bool
AntMessenger::waitForBroadcast(const uchar chan, AntMessage* reply, const size_t timeout_ms)
{
  const uchar first=ANTFS_BeaconId;
  bool found=false;

  AntChannel& pc(chs[chan]);
  AntBCastListener bcl(pc, first);
  //pc.addBCastListener(&bcl);

  AntMessage dummy;if(!reply) reply=&dummy;
  found = bcl.waitForBCast(*reply, timeout_ms);
  //pc.rmBCastListener(&bcl);
  //M_ANTFS_Beacon* beacon(reinterpret_cast<M_ANTFS_Beacon*>(&reply->getPayloadRef()[1]));
  if(!found)
  {
    lprintf(antpm::LOG_ERR, "no matching bcast before timeout\n"); fflush(stdout);
  }
  return found;
}


// receive bytes from the serial interface
void*
AntMessenger::th_messageHandler()
{
  std::list<uchar> q;
  for(;;)
  {
    if(m_packerThKill)
      break;
    if(m_io)
    {
      uchar buf[128];
      size_t bytesRead=0;
      bool rv = m_io->readBlocking(reinterpret_cast<char*>(buf), sizeof(buf), bytesRead);
      if(rv)
      {
        //printf("rv=%d, bytesRead=%d\n", (int)rv, (int)bytesRead); fflush(stdout);
        for(size_t i=0; i<bytesRead; i++)
        {
          q.push_back(buf[i]);
        }
      }
    }
    else
    {
      sleepms(1000);
    }
    assemblePackets(q);
  }
  if(!q.empty())
  {
    lprintf(antpm::LOG_WARN, "%d remaining uninterpreted bytes\n", (int)q.size());
  }
  return NULL;
}


}

