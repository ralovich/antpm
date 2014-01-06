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

#include "AntFr405.hpp"
#include "SerialTty.hpp"
#include "SerialUsb.hpp"
#include "antdefs.hpp"
#include "common.hpp"
#include "DeviceSettings.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <functional>
#include <algorithm>
#include <vector>
#include <string>
#include <boost/thread/thread_time.hpp>
#include <iostream>
#include "stdintfwd.hpp"



using namespace std;

namespace antpm{


const uchar net  = 0x00;
const uchar chan = 0x00;
//const uint hostSN = 0x7c9101e0; // from Garmin ANT+ Agent
//const uint64_t pairedKey = 0xd273f79a6f166fa5;
const uint hostSN = 0x00000000;//0xbcdef012;
//const uint64_t pairedKey = 0xa56f166f9af773d2;
const ushort msgPeriod = 0x1000;
const uchar chanSearchTimeout = 0xff;
const uchar rfFreq = 0x32;
const ushort waveform = 0x5300;

const uchar fsFreq = 0x46;  // other values seen: 0x46 0x50 0x0f
const uchar beaconPer = 0x04;
const uchar fsSearchTimeout = 0x03;




struct AntFr405_EventLoop
{
  void operator() (AntFr405* arg)
  {
    //printf("msgFunc, arg: %p\n", arg); fflush(stdout);
    if(!arg)
    {
      rv=0;
      return;
    }
    AntFr405* This = reinterpret_cast<AntFr405*>(arg);
    //printf("msgFunc, This: %p\n", This); fflush(stdout);
    rv = This->th_eventLoop();
  }
  void* rv;
};

AntFr405::AntFr405(bool eventLoopInBgTh)
  : m_serial(new ANTPM_SERIAL_IMPL())
  , m_antMessenger(new AntMessenger(eventLoopInBgTh))
  , aplc(getConfigFolder()+std::string("antparse_")+getDateString()+".txt")
  , clientSN(0)
  , pairedKey(0)
  , m_eventLoopInBgTh(eventLoopInBgTh)
  , doPairing(false)
  , mode(MD_DOWNLOAD_ALL)
{
  m_antMessenger->setHandler(m_serial.get());
  m_antMessenger->setCallback(this);
  state = ST_ANTFS_0;
  m_eventThKill=0;

  AntFr405_EventLoop eventTh;
  eventTh.rv=0;
  m_eventTh = boost::thread(eventTh, this);

}


AntFr405::~AntFr405()
{
  m_antMessenger->setCallback(0);
  //m_antMessenger->setHandler(0);

  m_eventThKill=1;
  m_eventTh.join();
  state = ST_ANTFS_0;

  m_antMessenger.reset();
  m_serial.reset();
  lprintf(LOG_DBG, "%s\n", __FUNCTION__);
}


void
AntFr405::setModeDownloadAll()
{
  mode = MD_DOWNLOAD_ALL;
  LOG_VAR2(mode, ModeOfOperation2Str(mode));
}

void
AntFr405::setModeDownloadSingleFile( const uint16_t fileIdx )
{
  mode = MD_DOWNLOAD_SINGLE_FILE;
  singleFileIdx = fileIdx;
  LOG_VAR2(mode, ModeOfOperation2Str(mode));
}

void
AntFr405::setModeDirectoryListing()
{
  mode = MD_DIRECTORY_LISTING;
  LOG_VAR2(mode, ModeOfOperation2Str(mode));
}

void
AntFr405::setModeEraseSingleFile(const uint16_t fileIdx)
{
  mode = MD_ERASE_SINGLE_FILE;
  singleFileIdx = fileIdx;
  LOG_VAR2(mode, ModeOfOperation2Str(mode));
}


void
AntFr405::setModeEraseAllActivities()
{
  mode = MD_ERASE_ALL_ACTIVITIES;
  LOG_VAR2(mode, ModeOfOperation2Str(mode));
}


void
AntFr405::onAntReceived(const AntMessage m)
{
  postEvent(m);
}

void
AntFr405::onAntSent(const AntMessage m)
{
  LOG(antpm::LOG_DBG) << m.strDt(0.0) << "\n";
}



void
AntFr405::start()
{
  CHECK_RETURN(m_serial->open());

  //createDownloadFolder();

  //m_antMessenger->addListener(boost::bind(&AntFr310XT2::listenerFunc2, this, _1));

  //m_antMessenger->setCallback(&aplc);

  changeState(ST_ANTFS_START0);

  if(!m_eventLoopInBgTh)
    m_antMessenger->eventLoop();
}

void AntFr405::stop()
{
  m_eventThKill = 1;
  //m_eventTh.join();
  m_antMessenger->kill();
  if(m_serial->isOpen())
  {
    if(state>ST_ANTFS_LINKING)
      m_antMessenger->ANTFS_Disconnect(chan);
    m_antMessenger->ANT_CloseChannel(chan);
    m_antMessenger->ANT_ResetSystem();
  }
  m_serial->close();
  changeState(ST_ANTFS_START0, true);
}

void AntFr405::stopAsync()
{
  // FIXME: setting ST_ANTFS_LAST might not be enough for stopping immediately,
  //        as other thread might be
  //        sleeping in a listener, and we stop only when that returns.
  changeState(ST_ANTFS_LAST);
}


const int
AntFr405::getSMState() const
{
  //boost::unique_lock<boost::mutex> lock(this->stateMtx); // not needed, as this is a atomic read
  return state;
}

const char*
AntFr405::getSMStateStr() const
{
  //boost::unique_lock<boost::mutex> lock(this->stateMtx); // not needed, as this is a atomic read
  return StateFSWork2Str(state);
}

void
AntFr405::postEvent(const AntMessage& m)
{
  m_evQue.push(m);
}



void*
AntFr405::th_eventLoop()
{
  for(;;)
  {
    if(m_eventThKill)
      break;
    if(handleEvents())
    {
      //sleepms(2);
    }
    else
    {
      changeState(ST_ANTFS_BAD);
      sleepms(1000);
    }
  }
  return 0;
}


bool
AntFr405::handleEvents()
{
#define changeStateSafe(x) do                                           \
  {                                                                     \
    boost::unique_lock<boost::mutex> lock(this->stateMtx);              \
    if(this->state == ST_ANTFS_LAST)                                    \
    {                                                                   \
      lock.unlock();                                                    \
      this->stop();                                                     \
      return true;                                                      \
    }                                                                   \
    else                                                                \
    {                                                                   \
      lock.unlock();                                                    \
      this->changeState(x);                                             \
    }                                                                   \
  } while(0)

#define checkForExit() do                                               \
  {                                                                     \
    boost::unique_lock<boost::mutex> lock(this->stateMtx);              \
    if(this->state == ST_ANTFS_LAST)                                    \
    {                                                                   \
      lock.unlock();                                                    \
      this->stop();                                                     \
      return true;                                                      \
    }                                                                   \
  } while(0)

  while(!m_evQue.empty())
  {
    AntMessage m;
    m_evQue.pop(m);
    if(m.getMsgId()==MESG_RESPONSE_EVENT_ID)
    {
      //uint8_t chan=m.getPayloadRef()[0];
      uint8_t msgId=m.getPayloadRef()[1];
      if(msgId==MESG_EVENT_ID)
      {
        uint8_t msgCode = m.getPayloadRef()[2];
        if(msgCode==EVENT_RX_SEARCH_TIMEOUT)  // handle RX_SEARCH_TIMEOUT
        {
          changeState(ST_ANTFS_BAD);
        }
      }
    }
  }

  // new state machine
  if(state==ST_ANTFS_RESTART)
  {
    m_evQue.clear();
    //m_antMessenger->clearRxQueue();
    m_antMessenger->ANT_ResetSystem();
    m_antMessenger->ANT_ResetSystem();
    changeStateSafe(ST_ANTFS_START0);
  }
  if(state==ST_ANTFS_START0)
  {
    CHECK_RETURN_FALSE(m_antMessenger->ANT_ResetSystem());

    //CHECK_RETURN_FALSE(m_antMessenger->ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID));

    CHECK_RETURN_FALSE(m_antMessenger->ANT_SetNetworkKey(net, ANTP_NETKEY));

    CHECK_RETURN_FALSE(m_antMessenger->ANT_AssignChannel(chan,0,net));

    CHECK_RETURN_FALSE(m_antMessenger->ANT_SetChannelMessagePeriod(chan, msgPeriod));

    CHECK_RETURN_FALSE(m_antMessenger->ANT_SetChannelSearchTimeout(chan, chanSearchTimeout));
    CHECK_RETURN_FALSE(m_antMessenger->ANT_SetChannelRadioFreq(chan, rfFreq));
    CHECK_RETURN_FALSE(m_antMessenger->ANT_SetSearchWaveform(chan, waveform));
    CHECK_RETURN_FALSE(m_antMessenger->ANT_SetChannelId(chan, 0, 0x01, 0x05));
    CHECK_RETURN_FALSE(m_antMessenger->ANT_OpenChannel(chan));
    CHECK_RETURN_FALSE(m_antMessenger->ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID));

    changeStateSafe(ST_ANTFS_LINKING);
  }
  else if(state==ST_ANTFS_LINKING)
  {
    AntMessage m;
    //CHECK_RETURN_FALSE(m_antMessenger->waitForBroadcastDataAvail(chan, &m, 20000));//link beacon
    CHECK_RETURN_FALSE(m_antMessenger->waitForBroadcast(chan, &m, 20000));//link beacon
    M_ANTFS_Beacon* beacon(reinterpret_cast<M_ANTFS_Beacon*>(&m.getPayloadRef()[1]));
    // TODO:handle case of no available data
    if(!beacon->dataAvail)
    {
      changeStateSafe(ST_ANTFS_NODATA);
      LOG(LOG_RAW) << "\n\nNo data available from client!\n\n\n";
      return true;
    }

    //CHECK_RETURN_FALSE(m_antMessenger->ANT_RequestMessage(chan, MESG_CHANNEL_ID_ID));
    ushort devNum=0;
    uchar  devId=0;
    uchar  transType=0;
    CHECK_RETURN_FALSE(m_antMessenger->ANT_GetChannelId(chan, &devNum, &devId, &transType, 1000));
    LOG(LOG_RAW) << "\n\nFound device devNum=0x" << toString<ushort>(devNum) << " devId=0x" << toString<uint>(devId,2,'0') << " transType=0x" << toString<uint>(transType,2,'0') << "\n\n\n";
    GarminProducts prod;
    if(guessDeviceType(devNum, devId, transType, &prod))
    {
      if(prod==GarminFR310XT) { LOG(LOG_INF) << "guessed: GarminFR310XT\n\n\n"; }
      if(prod==GarminFR405) { LOG(LOG_INF) << "guessed: GarminFR405\n\n\n"; }
    }
    else { LOG(LOG_WARN) << "guessing failed!\n"; }

    CHECK_RETURN_FALSE(m_antMessenger->ANTFS_Link(chan, fsFreq, beaconPer, hostSN));

    changeStateSafe(ST_ANTFS_AUTH0_SN);
  }
  else if(state == ST_ANTFS_BAD)
  {
    // TODO: acc counter how many BADs we handle, then bail out
    m_antMessenger->ANT_CloseChannel(chan);
    sleepms(800);
    changeStateSafe(ST_ANTFS_RESTART);
    return true;
  }
  else if(state == ST_ANTFS_AUTH0_SN)
  {
    CHECK_RETURN_FALSE(m_antMessenger->ANT_SetChannelMessagePeriod(chan, msgPeriod));
    CHECK_RETURN_FALSE(m_antMessenger->ANT_SetChannelSearchTimeout(chan, fsSearchTimeout));
    CHECK_RETURN_FALSE(m_antMessenger->ANT_SetChannelRadioFreq(chan, fsFreq));

    CHECK_RETURN_FALSE(m_antMessenger->waitForBroadcast(chan));//auth beacon

    // R 105.996 40 MESG_RESPONSE_EVENT_ID chan=0x00 mId=MESG_EVENT_ID mCode=EVENT_RX_FAIL
    //CHECK_RETURN_FALSE(m_antMessenger->waitForRxFail(chan));

    //CHECK_RETURN_FALSE_LOG_OK(m_antMessenger->waitForBroadcast(chan));

    CHECK_RETURN_FALSE_LOG_OK(m_antMessenger->ANTFS_RequestClientDeviceSerialNumber(chan, hostSN, clientSN, clientDevName));

    LOG(LOG_RAW) << "\n\nFound client \"" << clientDevName << "\" SN=0x" << toString<uint>(clientSN,8,'0') << " SN=" << clientSN << "\n\n\n";

    m_ds.reset(new DeviceSettings(toStringDec<uint>(clientSN).c_str()));
    assert(m_ds.get());
    m_ds->loadDefaultValues();
    m_ds->loadFromFile(m_ds->getConfigFileName());
    m_ds->saveToFile(m_ds->getConfigFileName());
    m_serial->setWriteDelay(m_ds->SerialWriteDelayMs);

    readUInt64(clientSN, pairedKey);

    LOG_VAR3(doPairing, toString<uint64_t>(pairedKey,16,'0'), clientSN);
    if(doPairing || pairedKey==0)
      changeStateSafe(ST_ANTFS_AUTH1_PAIR);
    else
      changeStateSafe(ST_ANTFS_AUTH1_PASS);
  }
  else if(state == ST_ANTFS_AUTH1_PAIR)
  {
    const std::string hostName("libantpm");
    uint dummy;
    if(!m_antMessenger->ANTFS_Pairing(chan, hostSN, hostName, dummy, pairedKey))
    {
      changeStateSafe(ST_ANTFS_LAST);
      return true;
    }
    writeUInt64(clientSN, pairedKey);

    CHECK_RETURN_FALSE_LOG_OK(m_antMessenger->ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID));

    //changeStateSafe(ST_ANTFS_LAST);
    changeStateSafe(ST_ANTFS_AUTH1_PASS);
  }
  else if(state == ST_ANTFS_AUTH1_PAIR)
  {
    //FIXME:
    //m_antMessenger->ANTFS_Pairing(chan, hostSN, );
  }
  else if(state == ST_ANTFS_AUTH1_PASS)
  {
    CHECK_RETURN_FALSE_LOG_OK(m_antMessenger->ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID));

    if(!m_antMessenger->ANTFS_Authenticate(chan, hostSN, pairedKey))
    {
      changeStateSafe(ST_ANTFS_RESTART);
      return true;
    }

    LOG(LOG_RAW) << "\n\nClient authenticated successfully!\n\n\n";

    // channel status <>
    CHECK_RETURN_FALSE_LOG_OK(m_antMessenger->ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID));

    if(mode==MD_DOWNLOAD_ALL || mode==MD_DIRECTORY_LISTING)
      changeStateSafe(ST_ANTFS_DL_DIRECTORY);
    else if(mode==MD_DOWNLOAD_SINGLE_FILE)
      changeStateSafe(ST_ANTFS_DL_SINGLE_FILE);
    else if(mode==MD_ERASE_SINGLE_FILE)
      changeStateSafe(ST_ANTFS_ERASE_SINGLE_FILE);
    else
      changeStateSafe(ST_ANTFS_LAST);
  }
  else if(state == ST_ANTFS_DL_DIRECTORY)
  {
    // when authentication succeeds, State=Transport beacon arrives
    //R  96.026 MESG_BROADCAST_DATA_ID chan=0x00 ANTFS_BEACON(0x43) Beacon=1Hz, pairing=disabled, upload=disabled, dataAvail=no, State=Transport, Auth=PasskeyAndPairingOnly
    //R 124.999 MESG_BROADCAST_DATA_ID chan=0x00 ANTFS_BEACON(0x43) Beacon=1Hz, pairing=disabled, upload=disabled, dataAvail=no, State=Transport, Auth=PasskeyAndPairingOnly
    //S 114.743 MESG_REQUEST_ID chan=0x00 reqMsgId=MESG_CHANNEL_STATUS_ID
    //R   3.247 MESG_CHANNEL_STATUS_ID chan=00 chanSt=Tracking
    //S  12.451 MESG_BURST_DATA_ID chan=0x00, seq=0, last=no  ANTFS_CMD(0x44) ANTFS_CmdDirect fd=0xffff, offset=0x0000, data=0x0000
    //R   1.546 MESG_BROADCAST_DATA_ID chan=0x00 ANTFS_BEACON(0x43) Beacon=1Hz, pairing=disabled, upload=disabled, dataAvail=no, State=Transport, Auth=PasskeyAndPairingOnly
    //S   2.477 MESG_BURST_DATA_ID chan=0x00, seq=1, last=yes fe00000000000000 ........

    CHECK_RETURN_FALSE_LOG_OK(m_antMessenger->ANTFS_Direct(chan, SwapDWord(0xfe00000000000000)));

    CHECK_RETURN_FALSE_LOG_OK(m_antMessenger->ANTFS_Direct(chan, SwapDWord(0x06000200ff000000)));

    // 06000200f8000000
    CHECK_RETURN_FALSE_LOG_OK(m_antMessenger->ANTFS_Direct(chan, SwapDWord(0x06000200f8000000)));

    // just exit
    changeStateSafe(ST_ANTFS_LAST);
  }
  else if(state==ST_ANTFS_DL_FILES)
  {
    // standard operation: download everything between LastUserProfileTime and now
    // dl waypoint files
    // dl activity files
    // dl course files
    // NOTE: seems like, if a file was downloaded, it's date in the directory file changes to the date of transfer

    uint fileCnt=0;
    for(size_t i=0; i<zfc.waypointsFiles.size() && fileCnt<m_ds->MaxFileDownloads; i++)
    {
      checkForExit();
      LOG_VAR3(fileCnt, m_ds->MaxFileDownloads, zfc.waypointsFiles.size());
      ushort fileIdx = zfc.waypointsFiles[i];
      time_t t       = GarminConvert::gOffsetTime(zfc.getFitFileTime(fileIdx));
      if(t < m_ds->LastUserProfileTime)
      {
        logger() << "Skipping waypoints file 0x" << toString<ushort>(fileIdx,4,'0')
                 << "@" << DeviceSettings::time2str(t) << " older than "
                 << DeviceSettings::time2str(m_ds->LastUserProfileTime) <<  "\n";
        continue;
      }
      logger() << "Transfer waypoints file 0x" << hex << fileIdx << "\n";

      std::vector<uchar> data;
      if(!m_antMessenger->ANTFS_Download(chan, fileIdx, data))
      {
        changeStateSafe(ST_ANTFS_LAST);
        return true;
      }
      LOG(LOG_RAW) << "\n\nDownloaded file idx=" << toString<ushort>(fileIdx,4,'0') << "\n\n\n";
      AntFsFile file0; file0.bytes=data; file0.saveToFile((folder+toString(fileIdx, 4, '0')+".fit").c_str());
      LOG_VAR(file0.checkCrc());

      fit.parse(data, gpx);

      time_t fitDate;
      if(!FIT::getCreationDate(data, fitDate))
        fitDate = t;
      //m_ds->mergeLastUserProfileTime(fitDate);

      fileCnt++;
    }

    for (size_t i=0; i<zfc.activityFiles.size() && fileCnt<m_ds->MaxFileDownloads; i++)
    {
      checkForExit();
      LOG_VAR3(fileCnt, m_ds->MaxFileDownloads, zfc.activityFiles.size());
      ushort fileIdx = zfc.activityFiles[i];
      time_t t       = GarminConvert::gOffsetTime(zfc.getFitFileTime(fileIdx));
      if(t < m_ds->LastUserProfileTime)
      {
        logger() << "Skipping activity file 0x" << toString<ushort>(fileIdx,4,'0')
                 << "@" << DeviceSettings::time2str(t) << " older than "
                 << DeviceSettings::time2str(m_ds->LastUserProfileTime) <<  "\n";
        continue;
      }
      logger() << "# Transfer activity file 0x" << hex << fileIdx << "\n";

      std::vector<uchar> data;
      if(!m_antMessenger->ANTFS_Download(chan, fileIdx, data))
      {
        changeStateSafe(ST_ANTFS_LAST);
        return true;
      }
      LOG(LOG_RAW) << "\n\nDownloaded file idx=" << toString<ushort>(fileIdx,4,'0') << "\n\n\n";
      AntFsFile file0; file0.bytes=data; file0.saveToFile((folder+toString(fileIdx, 4, '0')+".fit").c_str());

      fit.parse(data, gpx);

      time_t fitDate=0;
      if(!FIT::getCreationDate(data, fitDate))
      {
        LOG_VAR3(fitDate, t, m_ds->LastUserProfileTime);
        fitDate = t;
      }
      else
      {
        fitDate = GarminConvert::gOffsetTime(fitDate);
        LOG_VAR3(DeviceSettings::time2str(fitDate), DeviceSettings::time2str(t), DeviceSettings::time2str(m_ds->LastUserProfileTime));
      }
      //m_ds->mergeLastUserProfileTime(fitDate); // can't update it in the middle of the loop

      fileCnt++;
    }

    for (size_t i=0; i<zfc.courseFiles.size() && fileCnt<m_ds->MaxFileDownloads; i++)
    {
      checkForExit();
      LOG_VAR3(fileCnt, m_ds->MaxFileDownloads, zfc.courseFiles.size());
      ushort fileIdx = zfc.courseFiles[i];
      time_t t       = GarminConvert::gOffsetTime(zfc.getFitFileTime(fileIdx));
      if(t < m_ds->LastUserProfileTime)
      {
        logger() << "Skipping course file 0x" << toString<ushort>(fileIdx,4,'0')
                 << "@" << DeviceSettings::time2str(t) << " older than "
                 << DeviceSettings::time2str(m_ds->LastUserProfileTime) <<  "\n";
        continue;
      }
      logger() << "Transfer course file 0x" << hex << fileIdx << "\n";

      std::vector<uchar> data;
      if(!m_antMessenger->ANTFS_Download(chan, fileIdx, data))
      {
        changeStateSafe(ST_ANTFS_LAST);
        return true;
      }
      LOG(LOG_RAW) << "\n\nDownloaded file idx=" << toString<ushort>(fileIdx,4,'0') << "\n\n\n";
      AntFsFile file0; file0.bytes=data; file0.saveToFile((folder+toString(fileIdx, 4, '0')+".fit").c_str());

      fit.parse(data, gpx);

      // FIXME: case of fit date in future, also check the date from the directory file for consistency
      time_t fitDate;
      if(!FIT::getCreationDate(data, fitDate))
        fitDate = t;
      //m_ds->mergeLastUserProfileTime(fitDate);

      fileCnt++;
    }

    std::string gpxFile=folder+"libantpm.gpx";
    logger() << "Writing output to '" << gpxFile << "'\n";
    gpx.writeToFile(gpxFile);

    m_ds->mergeLastTransferredTime(time(NULL));
    m_ds->saveToFile(m_ds->getConfigFileName());

    changeStateSafe(ST_ANTFS_LAST);
  }
  else if(state==ST_ANTFS_DL_SINGLE_FILE)
  {
    logger() << "Transfer of file 0x" << hex << singleFileIdx << dec << "\n";

    std::vector<uchar> data;
    if(!m_antMessenger->ANTFS_Download(chan, singleFileIdx, data))
    {
      changeStateSafe(ST_ANTFS_LAST);
      return true;
    }
    LOG(LOG_RAW) << "\n\nDownloaded file idx=" << toString<ushort>(singleFileIdx,4,'0') << "\n\n\n";
    AntFsFile file0; file0.bytes=data; file0.saveToFile((folder+toString(singleFileIdx, 4, '0')+".fit").c_str());//...might not be a fit file
    LOG_VAR(file0.checkCrc());

    fit.parse(data, gpx);

    std::string gpxFile=folder+"libantpm.gpx";
    logger() << "Writing output to '" << gpxFile << "'\n";
    gpx.writeToFile(gpxFile);

    changeStateSafe(ST_ANTFS_LAST);
  }
  else if(state==ST_ANTFS_ERASE_SINGLE_FILE)
  {
    CHECK_RETURN_FALSE_LOG_OK(m_antMessenger->ANTFS_Erase(chan, singleFileIdx));

    LOG(LOG_RAW) << "\n\nErased file idx=" << toString<ushort>(singleFileIdx,4,'0') << "\n\n\n";

    changeStateSafe(ST_ANTFS_LAST);
  }
  else if(state==ST_ANTFS_NODATA)
  {
    changeStateSafe(ST_ANTFS_LAST);
  }
  else if(state==ST_ANTFS_LAST)
  {
    stop();
  }

  return true;
#undef changeStateSafe
}


int
AntFr405::changeState(const int newState, bool force)
{
  boost::unique_lock<boost::mutex> lock(stateMtx);
  int oldState = this->state;
  if(oldState == ST_ANTFS_LAST && newState != ST_ANTFS_LAST&& !force)
  {
    LOG(LOG_WARN) << "This seems to be a bug, we've tried ST_ANTFS_LAST => " << StateFSWork2Str(newState) << "!\n\n";
    return oldState;
  }
  this->state = newState;
  LOG(antpm::LOG_RAW) << "\nSTATE: " << std::dec << oldState << " => " << newState
    << "\t " << StateFSWork2Str(oldState) << " => " << StateFSWork2Str(newState)
    << "\n\n";
  return oldState;
}


AntFr405::StateANTFS
AntFr405::changeFSState(const AntFr405::StateANTFS newState)
{
  StateANTFS oldState = this->clientState;
  this->clientState = newState;
  LOG(antpm::LOG_RAW) << "\nFS: " << std::dec << oldState << " => " << newState << "\n\n";
  return oldState;
}


bool
AntFr405::createDownloadFolder()
{
  if(!folder.empty())
  {
    LOG(LOG_WARN) << "folder is \"" << folder << "\", why not empty?\n";
    //return false;
  }
  CHECK_RETURN_FALSE(m_ds);
  if(clientSN==0)
  {
    LOG(LOG_WARN) << "this is strange, clientSN is 0!\n";
    return false;
  }
  std::stringstream ss;
  //ss << getConfigFolder() << "/" << clientSN << "/" << getDateString() + "/";
  ss << m_ds->getFolder()  << "/" << getDateString() + "/";
  folder = ss.str();
  //folder = getConfigFolder() + "/" + getDateString() + "/";
  if(!mkDir(folder.c_str()))
  {
    if(folderExists(folder.c_str()))
      return true;
    else
    {
      LOG(LOG_ERR) << "Folder \"" << folder << "\" doesn't exist, and could not be created either!\n";
      return false;
    }
  }
  return true;
}

/// TODO: we need to refine these matches based on more trace data
bool
AntFr405::guessDeviceType(const ushort devNum, const uchar devId, const uchar transType, GarminProducts* prod)
{
  if(!prod)
    return false;

  // 310
  // devNum=0x4cd4, devId=0x01, transType=0x05
  // Beacon=8Hz
  if( devNum==0x4cd4 )
  {
    *prod = GarminFR310XT;
    return true;
  }


  // 405
  // devNum=0xc12e, devId=0x01, transType=0x05
  // Beacon=1Hz
  if( devNum==0xc12e )
  {
    *prod = GarminFR405;
    return true;
  }


  return false;
}

}
