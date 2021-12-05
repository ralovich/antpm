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
#include <fstream>
#include "Log.hpp"

#ifdef __linux__
# include <sys/utsname.h>
#endif

namespace antpm {



Serial*
Serial::instantiate(void*)
{
#ifdef __linux__
  utsname u;
  int urv = uname(&u);
  if(urv==0)
  {
    LOG(LOG_DBG) << "Running under: " << u.sysname << ", " << u.release << ", " << u.version << ", " << u.machine << "\n";
  }

  // check for cp210x kernel module
  std::string line;
  std::ifstream mods("/proc/modules");
  bool cp210x_found=false;
  bool usbserial_found=false;
  bool usbs_simple_found=false;
  if(!mods.is_open())
  {
    LOG(LOG_WARN) << "Could not open /proc/modules!\n";
  }
  else
  {
    while (mods.good())
    {
      getline(mods,line);
      if(line.find("cp210x ")==0)
        cp210x_found = true;
      if(line.find("usbserial ")==0)
        usbserial_found = true;
      if(line.find("usb_serial_simple ")==0)
        usbs_simple_found = true;
    }
    mods.close();
    if(cp210x_found)
    {
      LOG(LOG_DBG) << "Found loaded cp210x kernel module.\n";
    }
    else
    {
      LOG(LOG_DBG) << "cp210x is not listed among loaded kernel modules.\n";
    }
    if(usbserial_found)
    {
      LOG(LOG_DBG) << "Found loaded usbserial kernel module.\n";
    }
    else
    {
      LOG(LOG_DBG) << "usbserial is not listed among loaded kernel modules.\n";
    }
    if(usbs_simple_found)
    {
      LOG(LOG_DBG) << "Found loaded usb_serial_simple kernel module.\n";
    }
    else
    {
      LOG(LOG_DBG) << "usb_serial_simple is not listed among loaded kernel modules.\n";
    }
  }
#endif

  Serial* s = new SerialUsb();
  if(!s)
    return NULL;
#ifdef _WIN32
  if(!s->open())
  {
    delete s;
    return NULL;
  }
#else
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

