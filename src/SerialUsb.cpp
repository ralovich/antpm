// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
/*
 * Portions copyright as:
 * Silicon Laboratories CP2101/CP2102/CP2103 USB to RS232 serial adaptor driver
 *
 * Copyright (C) 2010 Craig Shelley (craig@microtron.org.uk)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License version
 *	2 as published by the Free Software Foundation.
 *
 * Support to set flow control line levels using TIOCMGET and TIOCMSET
 * thanks to Karl Hiramoto karl@hiramoto.org. RTSCTS hardware flow
 * control thanks to Munir Nassar nassarmu@real-time.com
 * Original backport to 2.4 by andreas 'randy' weinberger randy@ebv.com
 * Several fixes and enhancements by Bill Pfutzenreuter BPfutzenreuter@itsgames.com
 * Ported to userspace by Kristof Ralovich
 *
 */
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
#include "usb.h"
#endif


namespace antpm {

const uchar USB_ANT_CONFIGURATION = 1;
const uchar USB_ANT_INTERFACE = 0;
const uchar USB_ANT_EP_IN  = 0x81;
const uchar USB_ANT_EP_OUT = 0x01;







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
  size_t m_writeDelay;

#define REQTYPE_HOST_TO_INTERFACE	0x41
#define REQTYPE_INTERFACE_TO_HOST	0xc1
#define REQTYPE_HOST_TO_DEVICE	0x40
#define REQTYPE_DEVICE_TO_HOST	0xc0

#define USB_CTRL_GET_TIMEOUT	5000
#define USB_CTRL_SET_TIMEOUT	5000

  /* Config request codes */
  #define CP210X_IFC_ENABLE	0x00
  #define CP210X_SET_BAUDDIV	0x01
  #define CP210X_GET_BAUDDIV	0x02
  #define CP210X_SET_LINE_CTL	0x03
  #define CP210X_GET_LINE_CTL	0x04
  #define CP210X_SET_BREAK	0x05
  #define CP210X_IMM_CHAR		0x06
  #define CP210X_SET_MHS		0x07
  #define CP210X_GET_MDMSTS	0x08
  #define CP210X_SET_XON		0x09
  #define CP210X_SET_XOFF		0x0A
  #define CP210X_SET_EVENTMASK	0x0B
  #define CP210X_GET_EVENTMASK	0x0C
  #define CP210X_SET_CHAR		0x0D
  #define CP210X_GET_CHARS	0x0E
  #define CP210X_GET_PROPS	0x0F
  #define CP210X_GET_COMM_STATUS	0x10
  #define CP210X_RESET		0x11
  #define CP210X_PURGE		0x12
  #define CP210X_SET_FLOW		0x13
  #define CP210X_GET_FLOW		0x14
  #define CP210X_EMBED_EVENTS	0x15
  #define CP210X_GET_EVENTSTATE	0x16
  #define CP210X_SET_CHARS	0x19
  #define CP210X_GET_BAUDRATE	0x1D
  #define CP210X_SET_BAUDRATE	0x1E

  /* CP210X_IFC_ENABLE */
  #define UART_ENABLE		0x0001
  #define UART_DISABLE		0x0000

  /* CP210X_(SET|GET)_BAUDDIV */
  #define BAUD_RATE_GEN_FREQ	0x384000

  /* CP210X_(SET|GET)_LINE_CTL */
  #define BITS_DATA_MASK		0X0f00
  #define BITS_DATA_5		0X0500
  #define BITS_DATA_6		0X0600
  #define BITS_DATA_7		0X0700
  #define BITS_DATA_8		0X0800
  #define BITS_DATA_9		0X0900

  #define BITS_PARITY_MASK	0x00f0
  #define BITS_PARITY_NONE	0x0000
  #define BITS_PARITY_ODD		0x0010
  #define BITS_PARITY_EVEN	0x0020
  #define BITS_PARITY_MARK	0x0030
  #define BITS_PARITY_SPACE	0x0040

  #define BITS_STOP_MASK		0x000f
  #define BITS_STOP_1		0x0000
  #define BITS_STOP_1_5		0x0001
  #define BITS_STOP_2		0x0002

  /* CP210X_SET_BREAK */
  #define BREAK_ON		0x0001
  #define BREAK_OFF		0x0000

  /* CP210X_(SET_MHS|GET_MDMSTS) */
  #define CONTROL_DTR		0x0001
  #define CONTROL_RTS		0x0002
  #define CONTROL_CTS		0x0010
  #define CONTROL_DSR		0x0020
  #define CONTROL_RING		0x0040
  #define CONTROL_DCD		0x0080
  #define CONTROL_WRITE_DTR	0x0100
  #define CONTROL_WRITE_RTS	0x0200


#define TIOCM_DTR       0x002
#define TIOCM_RTS       0x004

  int get_config(int request, char* data, int size)
  {
    if(!dev)
      return -1111;

    int index = 0; // bInterfaceNumber ==? USB_ANT_INTERFACE

    int irv;
    irv = usb_control_msg(dev, REQTYPE_INTERFACE_TO_HOST, request, 0,
                          index, data, size, USB_CTRL_GET_TIMEOUT);
    LOG_VAR(irv);

    return irv;
  }

  // size in bytes
  int set_config(int request, char* data, int size)
  {
    if(!dev)
      return -1111;

    int index = 0; // bInterfaceNumber ==? USB_ANT_INTERFACE

    int irv;
    if(size>2)
    {
      irv = usb_control_msg(dev, REQTYPE_HOST_TO_INTERFACE, request, 0,
                            index, data, size, USB_CTRL_SET_TIMEOUT);
    }
    else
    {
      irv = usb_control_msg(dev, REQTYPE_HOST_TO_INTERFACE, request, data[0],
                            index, NULL, 0, USB_CTRL_SET_TIMEOUT);
    }
    LOG_VAR(irv);

    return irv;
  }

  int set_config_single(int request, unsigned short data)
  {
    return set_config(request, reinterpret_cast<char*>(&data), 2);
  }

  int change_speed(unsigned int baud)
  {
    return set_config(CP210X_SET_BAUDRATE, reinterpret_cast<char*>(&baud), sizeof(baud));
  }

  int tiocmset(unsigned int set, unsigned int clear)
  {
	  unsigned int control = 0;

	  if (set & TIOCM_RTS) {
		  control |= CONTROL_RTS;
		  control |= CONTROL_WRITE_RTS;
	  }
	  if (set & TIOCM_DTR) {
		  control |= CONTROL_DTR;
		  control |= CONTROL_WRITE_DTR;
	  }
	  if (clear & TIOCM_RTS) {
		  control &= ~CONTROL_RTS;
		  control |= CONTROL_WRITE_RTS;
	  }
	  if (clear & TIOCM_DTR) {
		  control &= ~CONTROL_DTR;
		  control |= CONTROL_WRITE_DTR;
	  }

    lprintf(LOG_INF, "%s - control = 0x%.4x\n", __FUNCTION__, control);

	  return set_config(CP210X_SET_MHS, reinterpret_cast<char*>(&control), 2);
  }


  void
  modprobe()
  {
//    ffff8800364a8300 962108826 S Co:3:001:0 s 23 03 0004 0001 0000 0
//    ffff8800364a8300 962108840 C Co:3:001:0 0 0
//    ffff8800b225b780 962162649 S Ci:3:001:0 s a3 00 0000 0001 0004 4 <
//    ffff8800b225b780 962162697 C Ci:3:001:0 0 4 = 03010000
//    ffff8800b225b780 962218579 S Co:3:001:0 s 23 01 0014 0001 0000 0
//    ffff8800b225b780 962218591 C Co:3:001:0 0 0
//    ffff8800b225b780 962218620 S Ci:3:000:0 s 80 06 0100 0000 0040 64 <
//    ffff8800b225b780 962222272 C Ci:3:000:0 0 18 = 12011001 00000040 cf0f0410 00030102 0301
//    ffff8800364a8300 962222320 S Co:3:001:0 s 23 03 0004 0001 0000 0
//    ffff8800364a8300 962222330 C Co:3:001:0 0 0
//    ffff8800b225b3c0 962274639 S Ci:3:001:0 s a3 00 0000 0001 0004 4 <
//    ffff8800b225b3c0 962274675 C Ci:3:001:0 0 4 = 03010000
//    ffff8800364a8300 962330667 S Co:3:001:0 s 23 01 0014 0001 0000 0
//    ffff8800364a8300 962330676 C Co:3:001:0 0 0
//    ffff8800364a8300 962330685 S Co:3:000:0 s 00 05 0002 0000 0000 0
//    ffff8800364a8300 962333269 C Co:3:000:0 0 0
//    ffff8800b225b3c0 962350649 S Ci:3:002:0 s 80 06 0100 0000 0012 18 <
//    ffff8800b225b3c0 962354268 C Ci:3:002:0 0 18 = 12011001 00000040 cf0f0410 00030102 0301
//    ffff8800b225b3c0 962354312 S Ci:3:002:0 s 80 06 0200 0000 0020 32 <
//    ffff8800b225b3c0 962357263 C Ci:3:002:0 0 32 = 09022000 01010080 32090400 0002ff00 00020705 81024000 00070501 02400000
//    ffff8800364a8300 962357309 S Ci:3:002:0 s 80 06 0303 0409 00ff 255 <
//    ffff8800364a8300 962364268 C Ci:3:002:0 0 128 = 80033100 30003000 36003200 00001400 28002f00 f300a700 18003a00 80004700
//    ffff8800b225ba80 962364316 S Co:3:002:0 s 00 09 0001 0000 0000 0
//    ffff8800b225ba80 962366264 C Co:3:002:0 0 0
    if(!dev)
      return;

    //cp210x_startup: usb_reset_device

    set_config_single(CP210X_IFC_ENABLE, UART_ENABLE);
    //cp2101_set_config_single(port, CP210X_IFC_ENABLE, UART_ENABLE);
    /* Configure the termios structure */
    //cp2101_get_termios(port);
    change_speed(115200);
    /* Set the DTR and RTS pins low */
    //cp2101_tiocmset(port, NULL, TIOCM_DTR | TIOCM_RTS, 0);
    tiocmset(TIOCM_DTR | TIOCM_RTS, 0);

  }

  void
  open_tty()
  {}

  usb_dev_handle*
  libUSBGetDevice (unsigned short vid, unsigned short pid)
  {
    struct usb_bus *UsbBus = NULL;
    struct usb_device *UsbDevice = NULL;
    usb_dev_handle *ret;

    int dBuses, dDevices;
    dBuses = usb_find_busses ();
    dDevices = usb_find_devices ();

    LOG_VAR2(dBuses, dDevices);
    lprintf(LOG_INF, "bus: %s, dev: %s, vid: 0x%04hx, pid: 0x%04hx\n", "", "", vid, pid);

    for (UsbBus = usb_get_busses(); UsbBus; UsbBus = UsbBus->next)
    {
      bool found = false;
      for (UsbDevice = UsbBus->devices; UsbDevice; UsbDevice = UsbDevice->next)
      {
        lprintf(LOG_INF, "bus: %s, dev: %s, vid: 0x%04hx, pid: 0x%04hx\n", UsbBus->dirname, UsbDevice->filename, UsbDevice->descriptor.idVendor, UsbDevice->descriptor.idProduct);
        if (UsbDevice->descriptor.idVendor == vid && UsbDevice->descriptor.idProduct== pid)
        {
          lprintf(LOG_INF, "found!\n");
          found = true;
          break;
        }
      }
      if(found)
        break;
    }

    if (!UsbDevice) return NULL;
    ret = usb_open (UsbDevice);


    //int cfg = usb_get_configuration(UsbDevice);
    int rv=usb_set_configuration (ret, USB_ANT_CONFIGURATION);
    LOG_VAR(rv);
    if (rv < 0) {
      usb_close (ret);
      return NULL;
    }


    rv=usb_claim_interface (ret, USB_ANT_INTERFACE);
    LOG_VAR(rv);
    if (rv < 0) {
      usb_close (ret);
      return NULL;
    }

    return ret;
  }
};



SerialUsb::SerialUsb()
{
  LOG(LOG_INF) << "Using SerialUsb...\n";

  m_p.reset(new SerialUsbPrivate());
  m_p->m_recvThKill = 0;
  m_p->dev = 0;
  m_p->m_writeDelay = 0;

  usb_init();

}

SerialUsb::~SerialUsb()
{
  close();
  m_p.reset();
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
  close();

  //bool rv = false;

  // ffff8800b1c470c0 1328871577 S Co:3:002:0 s 00 09 0001 0000 0000 0
  // ffff8800b1c470c0 1328873340 C Co:3:002:0 0 0
  m_p->dev = m_p->libUSBGetDevice(0x0fcf, 0x1004);
  if(!m_p->dev)
  {
    m_p->dev = m_p->libUSBGetDevice(0x0fcf, 0x1008);
    if(!m_p->dev)
    {
      return false;
    }
  }

  m_p->modprobe();

  m_p->open_tty();

  //cp2101_set_config_single(port, CP2101_UART, UART_ENABLE)
  /* Configure the termios structure */
  //cp2101_get_termios(port);
  /* Set the DTR and RTS pins low */
  //cp2101_tiocmset(port, NULL, TIOCM_DTR | TIOCM_RTS, 0);

  // ffff8800b1c470c0 1328873580 S Co:3:002:0 s 02 01 0000 0001 0000 0
  // ffff8800b1c470c0 1328874338 C Co:3:002:0 0 0
  //int irv = usb_clear_halt(m_p->dev, USB_ANT_EP_OUT);
  //LOG_VAR(irv);

  // usb_clear_halt()
  // usb_get_string_simple()
  // usb_reset()
  // usb_device()
  // usb_set_configuration()
  // usb_set_debug()

  m_p->m_recvThKill = 0;
  AntUsbHandler2_Recevier recTh;
  recTh.rv = 0;
  m_p->m_recvTh = boost::thread(recTh, this);

  return true;
}



void
SerialUsb::close()
{
  m_p->m_recvThKill = 1;
  m_p->m_recvTh.join();

  if(m_p.get())
  {
    if(m_p->dev)
    {
      //usb_reset(m_p->dev);
      usb_release_interface(m_p->dev, USB_ANT_INTERFACE);
      usb_close(m_p->dev);
    }
    m_p->dev = 0;
  }
  m_p->m_writeDelay = 0;
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
    char* usberr=usb_strerror();
    LOG_VAR(usberr);
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
    else if(rv==-116) // timeout
    {}
    else
    {
      char* usberr=usb_strerror();
      LOG_VAR2(rv, usberr);
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


//// called from other thread
//void
//AntUsbHandler::queueData()
//{
//}




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
