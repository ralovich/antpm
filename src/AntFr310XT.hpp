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

#include "AntMessenger.hpp"
#include <queue>
#include "antdefs.hpp"
#include <list>
#include <memory>
#include <boost/thread.hpp>
#include "FIT.hpp"

namespace antpm{

class DeviceSettings;
struct AntFr310XT2_EventLoop;
// State-machine for ANT+ communication with Forerunner 310XT.
class AntFr310XT2: public AntCallback
{
public:
  AntFr310XT2(bool eventLoopInBgTh = true);
  virtual ~AntFr310XT2();

  void setModeForcePairing() { doPairing=true; }
  void setModeDownloadAll();
  void setModeDownloadSingleFile(const uint16_t fileIdx);
  void setModeDirectoryListing();
  void setModeEraseSingleFile(const uint16_t fileIdx);
  void setModeEraseAllActivities();

  virtual void onAntReceived(const AntMessage m);
  virtual void onAntSent(const AntMessage m);

  void start();
  void stop();
  void stopAsync();

  const int getSMState() const;
  const char* getSMStateStr() const;
  AntMessenger* getMessenger() { return m_antMessenger.get(); }

  void postEvent(const AntMessage& m);

protected:
  boost::scoped_ptr<Serial> m_serial;
  boost::scoped_ptr<AntMessenger> m_antMessenger;
  typedef enum { LINK,AUTHENTICATION,TRANSPORT,BUSY} StateANTFS;
  StateANTFS clientState;
  int state;
  boost::mutex stateMtx;
  volatile int m_eventThKill;
  boost::thread m_eventTh;
  lqueue4<AntMessage> m_evQue;
  AntParsedLoggerCallback aplc;
  boost::scoped_ptr<DeviceSettings> m_ds;

  FIT             fit;
  ZeroFileContent zfc;
  GPX             gpx;

  uint        clientSN;
  std::string clientDevName;
  uint64_t    pairedKey;

  bool m_eventLoopInBgTh;

  bool        doPairing;
  std::string folder;
  int         mode;
  uint16_t    singleFileIdx;
private:
  friend struct AntFr310XT2_EventLoop;
  void* th_eventLoop();
  bool handleEvents();
  int changeState(const int newState, bool force = false);
  StateANTFS changeFSState(const StateANTFS newState);
  bool createDownloadFolder();
  static bool guessDeviceType(const ushort devNum, const uchar devId, const uchar transType, GarminProducts *prod);
};

}
