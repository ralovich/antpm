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



#include "SerialUsb.hpp"
#include "antdefs.hpp"
#include "common.hpp"
#include <sys/types.h>
#include <sys/stat.h>
//#include <fcntl.h>
#include <stdio.h>
#include <string.h>
//#include <pthread.h>
//#include <termios.h>
#include <stdlib.h>
#include <signal.h>

#include <functional>
//#include <tr1/functional>
#include <algorithm>
#include <vector>
#include <string>
#include <boost/thread/thread_time.hpp>
#include <iostream>

#ifdef _WIN32
#define VC_EXTRALEAN 1
#define NOMINMAX 1
#include "lusb0_usb.h"
#else
#include <errno.h>
#include <usb.h>
#include "SerialTty.hpp"
#endif


namespace antpm {

const uchar USB_ANT_CONFIGURATION = 1;
const uchar USB_ANT_INTERFACE = 0;
const uchar USB_ANT_EP_IN  = 0x81;
const uchar USB_ANT_EP_OUT = 0x01;



#define LOG_USB_WARN(func, rv) \
  do { LOG(LOG_WARN) << func << ": " << rv << ": \"" << usb_strerror() << "\"\n"; } while(0)




struct SerialUsbPrivate
{
  boost::thread m_recvTh;
  mutable boost::mutex m_queueMtx;
  boost::condition_variable m_condQueue;
  std::queue<char> m_recvQueue;
  //lqueue<uchar> m_recvQueue2;
  volatile int m_recvThKill;
  //AntCallback* m_callback;
  usb_dev_handle* dev;
  unsigned short dev_vid;
  unsigned short dev_pid;
  size_t m_writeDelay;

#define REQTYPE_HOST_TO_INTERFACE	0x41
#define REQTYPE_INTERFACE_TO_HOST	0xc1
#define REQTYPE_HOST_TO_DEVICE	0x40
#define REQTYPE_DEVICE_TO_HOST	0xc0

#define USB_CTRL_GET_TIMEOUT	5000
#define USB_CTRL_SET_TIMEOUT	5000

#define CP210X_IFC_ENABLE	0x00
#define CP210X_SET_MHS		0x07
#define CP210X_SET_BAUDRATE	0x1E
#define UART_ENABLE		0x0001

#define CONTROL_DTR		0x0001
#define CONTROL_RTS		0x0002
#define CONTROL_WRITE_DTR	0x0100
#define CONTROL_WRITE_RTS	0x0200

  // size in bytes
  bool
  setConfig(int request, char* data, int size)
  {
    if(!dev || size<0)
      return false;

    int index = 0; // bInterfaceNumber ==? USB_ANT_INTERFACE

    int irv = -1;
    int sz = 0;
    if(size>2)
    {
      sz = size;
      irv = usb_control_msg(dev, REQTYPE_HOST_TO_INTERFACE, request, 0, index, data, sz, USB_CTRL_SET_TIMEOUT);
    }
    else
    {
      sz = 0;
      irv = usb_control_msg(dev, REQTYPE_HOST_TO_INTERFACE, request, data[0], index, NULL, sz, USB_CTRL_SET_TIMEOUT);
    }
    if(irv<0) { LOG_VAR(irv); }

    return irv==sz;
  }

  bool
  cp210xInit()
  {
    if((dev_vid==0x0fcf) && (dev_pid==0x1008 || dev_pid==0x1009))
      return true;

    CHECK_RETURN_FALSE(dev);

    unsigned short word = UART_ENABLE;
    CHECK_RETURN_FALSE(setConfig(CP210X_IFC_ENABLE, reinterpret_cast<char*>(&word), sizeof(word)));
    unsigned int baud = 115200;
    CHECK_RETURN_FALSE(setConfig(CP210X_SET_BAUDRATE, reinterpret_cast<char*>(&baud), sizeof(baud)));

    unsigned int control = 0;
    control |= CONTROL_RTS;
    control |= CONTROL_WRITE_RTS;
    control |= CONTROL_DTR;
    control |= CONTROL_WRITE_DTR;
    CHECK_RETURN_FALSE(setConfig(CP210X_SET_MHS, reinterpret_cast<char*>(&control), 2));

    return true;
  }

  usb_dev_handle*
  libUSBGetDevice (const unsigned short vid, const unsigned short pid)
  {
    struct usb_bus *UsbBus = NULL;
    struct usb_device *UsbDevice = NULL;
    usb_dev_handle *udev;

    int dBuses, dDevices;
    dBuses = usb_find_busses ();
    dDevices = usb_find_devices ();

    //LOG_VAR2(dBuses, dDevices);
    //lprintf(LOG_INF, "bus: %s, dev: %s, vid: 0x%04hx, pid: 0x%04hx\n", "", "", vid, pid);
    UNUSED(dBuses);
    UNUSED(dDevices);

    for (UsbBus = usb_get_busses(); UsbBus; UsbBus = UsbBus->next)
    {
      bool found = false;
      for (UsbDevice = UsbBus->devices; UsbDevice; UsbDevice = UsbDevice->next)
      {
        //lprintf(LOG_INF, "bus: %s, dev: %s, vid: 0x%04hx, pid: 0x%04hx\n", UsbBus->dirname, UsbDevice->filename, UsbDevice->descriptor.idVendor, UsbDevice->descriptor.idProduct);
        if (UsbDevice->descriptor.idVendor == vid && UsbDevice->descriptor.idProduct== pid)
        {
          //lprintf(LOG_INF, "found!\n");
          found = true;
          dev_vid = vid;
          dev_pid = pid;
          break;
        }
      }
      if(found)
        break;
    }

    if (!UsbDevice) return NULL;
    udev = usb_open (UsbDevice);
    if(!udev)
    {
      LOG_USB_WARN("usb_open", (void*)udev);
      return NULL;
    }

    int rv = 0;
    char string[256];
    if(0 < usb_get_string_simple(udev, UsbDevice->descriptor.iProduct, string, sizeof(string)))
    {
      LOG(LOG_INF) << string << "\n";
    }
    if(0 < usb_get_string_simple(udev, UsbDevice->descriptor.iManufacturer, string, sizeof(string)))
    {
      LOG(LOG_INF) << string << "\n";
    }
    if(0 < usb_get_string_simple(udev, UsbDevice->descriptor.iSerialNumber, string, sizeof(string)))
    {
      LOG(LOG_INF) << string << "\n";
    }

    //int cfg = usb_get_configuration(UsbDevice);
    rv=usb_set_configuration (udev, USB_ANT_CONFIGURATION);
    if(rv < 0)
    {
      LOG_USB_WARN("USB_ANT_CONFIGURATION", rv);
      usb_close (udev);
      return NULL;
    }


    rv=usb_claim_interface (udev, USB_ANT_INTERFACE);
    if(rv < 0)
    {
      LOG_USB_WARN("USB_ANT_INTERFACE", rv);
      usb_close (udev);
      return NULL;
    }

    return udev;
  }
};



SerialUsb::SerialUsb()
{
  LOG(LOG_INF) << "Using SerialUsb...\n";

  m_p.reset(new SerialUsbPrivate());
  m_p->m_recvThKill = 0;
  m_p->dev = 0;
  m_p->dev_vid = 0;
  m_p->dev_pid = 0;
  m_p->m_writeDelay = 0;

  usb_init();

}

SerialUsb::~SerialUsb()
{
  close();
  lprintf(LOG_DBG2, "%s\n", __FUNCTION__);
}

//#define ENSURE_OR_RETURN_FALSE(e) do {  if(-1 == (e)) {perror(#e); return false;} } while(false)

// runs in other thread
struct AntUsbHandler2_Recevier
{
  void operator() (SerialUsb* arg)
  {
    //printf("recvFunc, arg: %p\n", arg); fflush(stdout);
    if(!arg)
    {
      rv=0;
      return;
    }
    SerialUsb* This = reinterpret_cast<SerialUsb*>(arg);
    //printf("recvFunc, This: %p\n", This); fflush(stdout);
    rv = This->receiveHandler();
  }
  void* rv;
};


bool
SerialUsb::open()
{
  assert(!isOpen());
  if(isOpen())
    return false;

  enum {NUM_DEVS=5};
  uint16_t known[NUM_DEVS][2] =
  {
    {0x0fcf, 0x1003},
    {0x0fcf, 0x1004},
    {0x0fcf, 0x1006},
    {0x0fcf, 0x1008},
    {0x0fcf, 0x1009},
  };

  //bool rv = false;

  // ffff8800b1c470c0 1328871577 S Co:3:002:0 s 00 09 0001 0000 0000 0
  // ffff8800b1c470c0 1328873340 C Co:3:002:0 0 0
  for(size_t i = 0; i < NUM_DEVS; i++)
  {
    uint16_t vid = known[i][0];
    uint16_t pid = known[i][1];
    LOG(LOG_INF) << "Trying to open vid=0x" << toString(vid,4,'0') << ", pid=0x" << toString(pid,4,'0') << " ...\n";
    m_p->dev = m_p->libUSBGetDevice(vid, pid);
    if(m_p->dev)
    {
      LOG(LOG_INF) << "Opened vid=0x" << toString(vid,4,'0') << ", pid=0x" << toString(pid,4,'0') << " OK.\n";
      break;
    }
    //LOG(LOG_RAW) << " failed.\n";
  }
  if(!m_p->dev)
  {
    LOG(antpm::LOG_ERR) << "Opening any known usb VID/PID failed!\n";
    return false;
  }

  assert(m_p->dev_vid!=0);
  assert(m_p->dev_pid!=0);

  CHECK_RETURN_FALSE(m_p->cp210xInit());

  m_p->m_recvThKill = 0;
  AntUsbHandler2_Recevier recTh;
  recTh.rv = 0;
  m_p->m_recvTh = boost::thread(recTh, this);

  return true;
}



void
SerialUsb::close()
{
  if(m_p.get())
  {
    m_p->m_recvThKill = 1;
    {
      boost::unique_lock<boost::mutex> lock(m_p->m_queueMtx);
      m_p->m_condQueue.notify_all(); // make sure an other thread calling readBlocking() moves on too
    }
    m_p->m_recvTh.join();

    if(m_p->dev)
    {
      //usb_reset(m_p->dev);
      usb_release_interface(m_p->dev, USB_ANT_INTERFACE);
      usb_close(m_p->dev);
    }
    m_p->dev = 0;
    m_p->m_writeDelay = 0;
    m_p.reset();
  }
}



bool
SerialUsb::read(char& c)
{
  boost::unique_lock<boost::mutex> lock(m_p->m_queueMtx);

  if(m_p->m_recvQueue.empty())
    return false;

  c = m_p->m_recvQueue.front();
  m_p->m_recvQueue.pop();
  return true;
}


bool
SerialUsb::read(char* dst, const size_t sizeBytes, size_t& bytesRead)
{
  if(!dst)
    return false;

  boost::unique_lock<boost::mutex> lock(m_p->m_queueMtx);

  size_t s = m_p->m_recvQueue.size();
  s = std::min(s, sizeBytes);
  for(size_t i = 0; i < s; i++)
  {
    dst[i] = m_p->m_recvQueue.front();
    m_p->m_recvQueue.pop();
  }
  bytesRead = s;

  if(bytesRead==0)
    return false;

  return true;
}


bool
SerialUsb::readBlocking(char* dst, const size_t sizeBytes, size_t& bytesRead)
{
  if(!dst)
    return false;

  const size_t timeout_ms = 1000;
  {
    boost::unique_lock<boost::mutex> lock(m_p->m_queueMtx);

    //while(m_p->m_recvQueue.empty()) // while - to guard agains spurious wakeups
    {
      m_p->m_condQueue.timed_wait(lock, boost::posix_time::milliseconds(timeout_ms));
    }
    size_t s = m_p->m_recvQueue.size();
    s = std::min(s, sizeBytes);
    for(size_t i = 0; i < s; i++)
    {
      dst[i] = m_p->m_recvQueue.front();
      m_p->m_recvQueue.pop();
    }
    bytesRead = s;
  }

  if(bytesRead==0)
    return false;

  return true;
}


bool
SerialUsb::write(const char* src, const size_t sizeBytes, size_t& bytesWritten)
{
  if(!m_p->dev)
    return false;

  int size = static_cast<int>(sizeBytes);
  //int written = usb_interrupt_write(m_p->dev, USB_ANT_EP_OUT, const_cast<char*>(src), size, 3000);
  int written = usb_bulk_write(m_p->dev, USB_ANT_EP_OUT, const_cast<char*>(src), size, 3000);
  if(written < 0)
  {
    LOG_USB_WARN("SerialUsb::write", written);
    return false;
  }

  bytesWritten = written;

  if(m_p->m_writeDelay>0 && m_p->m_writeDelay<=10)
    sleepms(m_p->m_writeDelay);

  return (bytesWritten==sizeBytes);
}




void*
SerialUsb::receiveHandler()
{
  for(;;)
  {
    if(m_p->m_recvThKill)
      return NULL;

    unsigned char buf[4096];
    int timeout_ms = 1000;
    int rv = usb_bulk_read(m_p->dev, USB_ANT_EP_IN, (char*)buf, sizeof(buf), timeout_ms);

    if(rv > 0)
    {
      boost::unique_lock<boost::mutex> lock(m_p->m_queueMtx);
      for(int i = 0; i < rv; i++)
        m_p->m_recvQueue.push(buf[i]);
      m_p->m_condQueue.notify_one();
    }
    else if(rv==0)
    {}
#ifdef _WIN32
    else if(rv==-116) // timeout
    {}
#else
    else if(rv==-ETIMEDOUT)
    {}
#endif
    else
    {
      LOG_USB_WARN("SerialUsb::receiveHandler", rv);
    }
  }

  return NULL;
}

const size_t SerialUsb::getQueueLength() const
{
  size_t len=0;
  boost::unique_lock<boost::mutex> lock(m_p->m_queueMtx);
  len += m_p->m_recvQueue.size();
  return len;
}


bool
SerialUsb::isOpen() const
{
  // TODO: is thread running too??
  return m_p.get() && m_p->dev;
}


bool
SerialUsb::setWriteDelay(const size_t ms)
{
  m_p->m_writeDelay = ms;
  return true;
}




}
