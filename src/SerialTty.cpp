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

#include "SerialTty.hpp"
#include "antdefs.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <cstring>
#include <termios.h>
#include <stdlib.h>
#include <signal.h>

#include <functional>
#include <tr1/functional>
#include <algorithm>
#include <vector>
#include <string>
#include <boost/thread/thread_time.hpp>

#include "Log.hpp"

namespace antpm{

struct SerialTtyPrivate
{
  std::string m_devName;
  int m_fd;
  boost::thread m_recvTh;
  mutable boost::mutex m_queueMtx;
  boost::condition_variable m_condQueue;
  std::queue<char> m_recvQueue;
  //lqueue<uchar> m_recvQueue2;
  volatile int m_recvThKill;
  //AntCallback* m_callback;
};

// runs in other thread
struct SerialTtyIOThread
{
  void operator() (SerialTty* arg)
  {
    //printf("recvFunc, arg: %p\n", arg); fflush(stdout);
    if(!arg)
    {
      rv=0;
      return;
    }
    SerialTty* This = reinterpret_cast<SerialTty*>(arg);
    //printf("recvFunc, This: %p\n", This); fflush(stdout);
    rv = This->ioHandler();
  }
  void* rv;
};


SerialTty::SerialTty()
  : m_p(new SerialTtyPrivate())
{
  m_p->m_devName = "/dev/ttyUSB0";
  m_p->m_fd = -1;
  //, m_p->m_recvTh = 0;
  m_p->m_recvThKill = 0;
}

SerialTty::~SerialTty()
{
  //m_callback = 0;
  //printf("%s\n", __FUNCTION__);
  //printf("\n\nboost\n\n");
}

#define ENSURE_OR_RETURN_FALSE(e)                           \
  do                                                        \
  {                                                         \
    if(-1 == (e))                                           \
    {                                                       \
      /*perror(#e);*/                                       \
      const char* se=strerror(e);                          \
      LOG(antpm::LOG_ERR) << se << "\n";                    \
      return false;                                         \
    }                                                       \
  } while(false)


bool
SerialTty::open()
{
  close();

  bool rv = false;

  m_p->m_fd = ::open(m_p->m_devName.c_str(), O_RDWR | O_NONBLOCK);
  if(m_p->m_fd < 1)
  {
    m_p->m_devName = "/dev/ttyUSB1";
    m_p->m_fd = ::open(m_p->m_devName.c_str(), O_RDWR | O_NONBLOCK);
  }

  ENSURE_OR_RETURN_FALSE(m_p->m_fd);
  if(m_p->m_fd<0)
    return rv;

  //printf("m_fd=%d\n", m_fd);

  struct termios tp;
  ENSURE_OR_RETURN_FALSE(tcgetattr(m_p->m_fd, &tp));
  tp.c_iflag &=
    ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON|IXOFF|IXANY|INPCK|IUCLC);
  tp.c_oflag &= ~OPOST;
  tp.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN|ECHOE);
  tp.c_cflag &= ~(CSIZE|PARENB);
  tp.c_cflag |= CS8 | CLOCAL | CREAD | CRTSCTS;

  ENSURE_OR_RETURN_FALSE(cfsetispeed(&tp, B115200));
  ENSURE_OR_RETURN_FALSE(cfsetospeed(&tp, B115200));
  tp.c_cc[VMIN] = 1;
  tp.c_cc[VTIME] = 0;
  ENSURE_OR_RETURN_FALSE(tcsetattr(m_p->m_fd, TCSANOW, &tp));

  m_p->m_recvThKill = 0;
  SerialTtyIOThread recTh;
  m_p->m_recvTh = boost::thread(recTh, this);

  return true;
}



void
SerialTty::close()
{
  m_p->m_recvThKill = 1;

  m_p->m_condQueue.notify_all();

  m_p->m_recvTh.join();

  if(m_p->m_fd >= 0)
  {
    ::close(m_p->m_fd);
  }
  m_p->m_fd = -1;
}



//bool
//AntTtyHandler2::read(char& c)
//{
//  boost::unique_lock<boost::mutex> lock(m_queueMtx);
//
//  if(m_recvQueue.empty())
//    return false;
//
//  c = m_recvQueue.front();
//  m_recvQueue.pop();
//  return true;
//}


bool
SerialTty::read(char* dst, const size_t sizeBytes, size_t& bytesRead)
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
SerialTty::readBlocking(char* dst, const size_t sizeBytes, size_t& bytesRead)
{
  if(!dst)
    return false;

  const size_t timeout_ms = 1000;
  {
    boost::unique_lock<boost::mutex> lock(m_p->m_queueMtx);

    //while(m_recvQueue.empty()) // while - to guard agains spurious wakeups
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
SerialTty::write(const char* src, const size_t sizeBytes, size_t& bytesWritten)
{
  if(m_p->m_fd<0)
    return false;
  ssize_t written = ::write(m_p->m_fd, src, sizeBytes);
  ENSURE_OR_RETURN_FALSE(written);
  bytesWritten = written;

  return true;
}


//void
//AntTtyHandler2::wait()
//{
////    boost::unique_lock<boost::mutex> lock(m_queueMtx);
////    m_pushEvent.wait(lock);

//////    if(m_callback)
//////        m_callback->onAntReceived();
//}


// Called inside other thread, wait's on the serial line, and extracts bytes as they arrive.
// The received bytes are queued.
void*
SerialTty::ioHandler()
{
  fd_set readfds, writefds, exceptfds;
  int ready;
  struct timeval to;
  //printf("%s\n", __FUNCTION__);

  for(;;)
  {
    if(m_p->m_recvThKill)
      return NULL;
    //printf("%s\n", __FUNCTION__);
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    FD_SET(m_p->m_fd, &readfds);
    to.tv_sec = 1;
    to.tv_usec = 0;
    ready = select(m_p->m_fd+1, &readfds, &writefds, &exceptfds, &to);
    //printf("select: %d\n", ready);
    if (ready>0) {
      queueData();
    }
  }
  return NULL;
}

const size_t SerialTty::getQueueLength() const
{
  size_t len=0;
  boost::unique_lock<boost::mutex> lock(m_p->m_queueMtx);
  len += m_p->m_recvQueue.size();
  return len;
}

bool
SerialTty::isOpen() const
{
  // TODO: is thread running too??
  // TODO: return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
  return !(m_p->m_fd<1) && 1;
}


// called from other thread
void
SerialTty::queueData()
{
  //printf("queueData\n");
  unsigned char buf[256];
  ssize_t rb = ::read(m_p->m_fd, buf, sizeof(buf));
  if(rb < 0)
  {
    perror("read");
    //switch(errno)
    //{}
  }
  else if(rb == 0) // EOF, ??
  {}
  else
  {
    boost::unique_lock<boost::mutex> lock(m_p->m_queueMtx);
    for(ssize_t i = 0; i < rb; i++)
      m_p->m_recvQueue.push(buf[i]);
    m_p->m_condQueue.notify_one();
  }
}


}


