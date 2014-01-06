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

namespace antpm{

// Abstract interface for serial-like communication.
class Serial
{
public:
  virtual              ~Serial() {}
  virtual bool         open() = 0;
  virtual void         close() = 0;
  virtual bool         read(char* dst, const size_t sizeBytes, size_t& bytesRead) = 0;
  virtual bool         readBlocking(char* dst, const size_t sizeBytes, size_t& bytesRead) = 0;
  // synchronous data writing, blocks until sent
  virtual bool         write(const char* src, const size_t sizeBytes, size_t& bytesWritten) = 0;
  //virtual void wait() = 0;
  /// number of raw bytes in the receive queue
  virtual const size_t getQueueLength() const = 0;
  virtual const char*  getImplName() = 0;
  virtual bool         isOpen() const = 0;
  virtual bool         setWriteDelay(const size_t ms) = 0;
};

}
