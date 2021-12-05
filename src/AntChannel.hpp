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

#include "AntMessage.hpp"
#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>


namespace antpm{

struct AntChannel;

struct AntListenerBase
{
protected:
  boost::mutex m_mtxResp;
  boost::condition_variable m_cndResp;
  boost::scoped_ptr<AntMessage> m_msgResp;
  AntChannel& owner;

public:
  AntListenerBase(AntChannel& o);
  virtual ~AntListenerBase();
  virtual void onMsg(AntMessage& m);
  virtual void interruptWait();
protected:
  virtual bool match(AntMessage& other) const = 0;
public:
  virtual const char* name() const = 0;
  // whether there was a response before timeout
  bool waitForMsg(AntMessage* m, const size_t timeout_ms);
};

struct AntChannel
{
  AntChannel() = default;
  AntChannel(const uchar ch);
private:
  const uchar chan;
  boost::mutex m_mtxListeners;
  std::list<AntListenerBase*> listeners;
public:
  void addMsgListener2(AntListenerBase* lb);
  void rmMsgListener2(AntListenerBase* lb);
  void onMsg(AntMessage &m);
  void interruptWait();
  void sanityCheck(const char* caller);
  const uchar getChan() const { return chan; }
  
};


struct AntEvListener : public AntListenerBase
{
  AntEvListener(AntChannel& o) : AntListenerBase(o) {}
  virtual ~AntEvListener() {}
  virtual bool match(AntMessage& other) const override;
  virtual const char* name() const override { return "AntEvListener"; }
  // whether there was a response before timeout
  bool waitForEvent(uint8_t& msgCode, const size_t timeout_ms);
};

struct AntRespListener : public AntListenerBase
{
  uint8_t msgId;//

  AntRespListener(AntChannel& o, const uint8_t msgId_) : AntListenerBase(o), msgId(msgId_) {}
  virtual ~AntRespListener() {}
  virtual bool match(AntMessage& other) const override;
  virtual const char* name() const override { return "AntRespListener"; }
  // whether there was a response before timeout
  bool waitForResponse(uint8_t& respVal, const size_t timeout_ms);
};

struct AntReqListener : public AntListenerBase
{
  uint8_t msgId;
  uint8_t chan;

  AntReqListener(AntChannel& o, uint8_t m, uint8_t c) : AntListenerBase(o), msgId(m), chan(c) {}
  virtual ~AntReqListener() {}
  virtual bool match(AntMessage& other) const override;
  virtual const char* name() const override { return "AntReqListener"; }
};

struct AntBCastListener : public AntListenerBase
{
  AntBCastListener(AntChannel& o) : AntListenerBase(o) {}
  virtual ~AntBCastListener() {}
  virtual bool match(AntMessage& other) const override;
  virtual const char* name() const override { return "AntBCastListener"; }
  bool waitForBCast(AntMessage& bcast, const size_t timeout_ms);
};

struct AntBurstListener : public AntListenerBase
{
  std::list<AntMessage> m_bursts;

  AntBurstListener(AntChannel& o) : AntListenerBase(o) {}
  virtual ~AntBurstListener() {}
  virtual void onMsg(AntMessage& m) override;
  virtual void interruptWait() override;
  virtual bool match(AntMessage& other) const override;
  virtual const char* name() const override { return "AntBurstListener"; }
  virtual bool waitForBursts(std::list<AntMessage>& bs, const size_t timeout_ms);
  bool collectBurst(std::vector<uint8_t>& burstData, const size_t timeout_ms);
};

}
