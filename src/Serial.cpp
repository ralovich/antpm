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


#include "Serial.hpp"
#include "SerialUsb.hpp"
#include "SerialTty.hpp"

namespace antpm {

Serial*
Serial::instantiate(void*)
{
  Serial* s = new SerialUsb();
  if(!s)
    return NULL;
#ifndef _WIN32
  if(!s->open())
  {
    delete s;
    s = new SerialTty();
    if(!s)
      return NULL;
    if(!s->open())
    {
      delete s;
      return NULL;
    }
  }
#endif
  return s;
}


}

