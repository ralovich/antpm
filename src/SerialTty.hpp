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

//#include <pthread.h>
#include <queue>
#include <boost/thread.hpp>
#include "antdefs.hpp"
#include "lqueue.hpp"
#include <list>
#include "Serial.hpp"

namespace antpm{

struct SerialTtyPrivate;
struct SerialTtyIOThread;
// Serial communication over a POSIX serial port.
class SerialTty : public Serial
{
public:
  SerialTty();
  virtual ~SerialTty();

  virtual bool open();
  virtual void close();

  //virtual bool read(char& c);
  virtual bool read(char* dst, const size_t sizeBytes, size_t& bytesRead);
  virtual bool readBlocking(char* dst, const size_t sizeBytes, size_t& bytesRead);
  virtual bool write(const char* src, const size_t sizeBytes, size_t& bytesWritten);

  //virtual void wait();

private:
  friend struct SerialTtyIOThread;
  void* ioHandler();

public:
  virtual const size_t getQueueLength() const;

  virtual const char* getImplName() { return "AntTtyHandler2"; }

  virtual bool isOpen() const;

private:
  void queueData();

private:
  std::auto_ptr<SerialTtyPrivate> m_p;
};

}
