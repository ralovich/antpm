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

#include <atomic>
#include <functional>
#include <algorithm>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "Log.hpp"
#include "common.hpp"
#include "DeviceSettings.hpp"

#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef IUCLC
# define IUCLC 0
#endif

namespace fs = std::filesystem;

namespace antpm{

struct SerialTtyPrivate
{
  std::string m_devName;
  int m_fd;
  std::thread m_recvTh;
  mutable std::mutex m_queueMtx;
  std::condition_variable m_condQueue;
  std::queue<char> m_recvQueue;
  std::atomic<bool> m_recvThKill = false;
  size_t       m_writeDelay;

  bool guessDeviceName(std::vector<std::string>& guessedNames);
  bool openDevice(std::vector<std::string>& names);
};

// runs in other thread
struct SerialTtyIOThread
{
  SerialTtyIOThread() : rv(NULL) {}
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


std::string
find_file_starts_with(const fs::path & dir,
                      const std::string& start)
{
  fs::directory_iterator end_iter;
  for( fs::directory_iterator dir_iter(dir) ; dir_iter != end_iter ; ++dir_iter)
  {
    fs::path p = *dir_iter;
    if(p.filename().string().find(start)==0)
    {
      return p.filename().string();
    }
  }
  return "";
}


struct InvalidChar
{
    bool operator()(char c) const {
        return !isprint((unsigned)c);
    }
};


bool
SerialTtyPrivate::guessDeviceName(std::vector<std::string> &guessedNames)
{
#ifdef __linux__
  // check for cp210x kernel module
  std::string line;
  std::ifstream mods("/proc/modules");
  bool cp210x_found=false;
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
    }
    mods.close();
    if(cp210x_found)
    {
      LOG(LOG_DBG) << "Found loaded cp210x kernel module, good.\n";
    }
    else
    {
      LOG(LOG_WARN) << "cp210x is not listed among loaded kernel modules!\n";
    }
  }
  // check for usb ids inside /sys/bus/usb/devices/N-N/idProduct|idVendor

  // /sys/bus/usb/drivers/cp210x

  // check /sys/bus/usb/drivers/cp210x/6-2:1.0/interface for "Dynastream ANT2USB"
  const char* driverDir="/sys/bus/usb/drivers/cp210x";
  if(!folderExists(driverDir))
  {
    LOG(LOG_WARN) << driverDir << " doesn't exist!\n";
  }
  else
  {
    LOG(LOG_DBG) << "Detecting in " << driverDir << " ...\n";
    for(fs::recursive_directory_iterator end, iter(driverDir, fs::directory_options::follow_directory_symlink); iter != end; )
    {
      if(iter.depth()>=2)
      {
        iter.pop();
        continue;
      }
      //cout << iter.depth() << ", " << *iter << std::endl;
      fs::path p = iter->path();
      //cout << p.parent_path() << "\n";
      if(iter.depth()==1 && p.filename()=="interface" && find_file_starts_with(p.parent_path(), "ttyUSB")!="")
      {
        std::vector<unsigned char> vuc(readFile(p.c_str()));
        std::string iface="";
        if(!vuc.empty())
        {
          iface=std::string(reinterpret_cast<char*>(&vuc[0]));
          iface.erase(std::remove_if(iface.begin(),iface.end(),InvalidChar()), iface.end());
        }
        std::string ttyUSB = find_file_starts_with(p.parent_path(), "ttyUSB");
        LOG(LOG_DBG) << "Found: \"" << iface << "\" as " << ttyUSB << " in " << p << "\n";
        guessedNames.push_back("/dev/"+ttyUSB);
      }
      ++iter;
    }
  }
//  // check for /sys/bus/usb-serial/drivers/cp210x/ttyUSBxxx
//  const char* driverDir="/sys/bus/usb-serial/drivers/cp210x";
//  if(!folderExists(driverDir))
//    LOG(LOG_WARN) << driverDir << " doesn't exist!\n";
//  else
//  {
//    LOG(LOG_DBG) << "Detecting in " << driverDir << " ...\n";
//    fs::directory_iterator end_iter;
//    for( fs::directory_iterator dir_iter(driverDir) ; dir_iter != end_iter ; ++dir_iter)
//    {
//      fs::path p = *dir_iter;
//      if(p.leaf().string().find("ttyUSB")==0)
//      {
//        LOG(LOG_DBG) << "Will try " << p.leaf() << "\n";
//        guessedName.push_back(p.leaf().string());
//      }
//      //string name = p.string();
//      //LOG(LOG_DBG) << name << "\n";
//    }
//  }
  return !guessedNames.empty();
#else
  return false;
#endif
}


void
getFileMode(const char* fname)
{
  struct stat fileStat;
  struct group *grp;
  struct passwd *pwd;

  if(stat(fname,&fileStat) == 0)
  {
    grp = getgrgid(fileStat.st_gid);
    pwd = getpwuid(fileStat.st_uid);

    LOG(LOG_DBG) << fname << ": \t";
    LOG(LOG_RAW) << pwd->pw_name << ":" << grp->gr_name << "\t";
    LOG(LOG_RAW) << ((S_ISLNK(fileStat.st_mode)) ? "l" : ((S_ISDIR(fileStat.st_mode)) ? "d" : "-"));
    LOG(LOG_RAW) << ((fileStat.st_mode & S_IRUSR) ? "r" : "-");
    LOG(LOG_RAW) << ((fileStat.st_mode & S_IWUSR) ? "w" : "-");
    LOG(LOG_RAW) << ((fileStat.st_mode & S_IXUSR) ? "x" : "-");
    LOG(LOG_RAW) << ((fileStat.st_mode & S_IRGRP) ? "r" : "-");
    LOG(LOG_RAW) << ((fileStat.st_mode & S_IWGRP) ? "w" : "-");
    LOG(LOG_RAW) << ((fileStat.st_mode & S_IXGRP) ? "x" : "-");
    LOG(LOG_RAW) << ((fileStat.st_mode & S_IROTH) ? "r" : "-");
    LOG(LOG_RAW) << ((fileStat.st_mode & S_IWOTH) ? "w" : "-");
    LOG(LOG_RAW) << ((fileStat.st_mode & S_IXOTH) ? "x\n" : "-\n");
  }
}


bool
SerialTtyPrivate::openDevice(std::vector<std::string> &names)
{
  for(size_t i = 0; i < names.size(); i++)
  {
    m_devName = names[i];
    getFileMode(m_devName.c_str());
    LOG(LOG_INF) << "Trying to open " << m_devName << " ...";
    m_fd = ::open(m_devName.c_str(), O_RDWR | O_NONBLOCK);
    if(m_fd < 0)
    {
      LOG(LOG_RAW) << " failed.\n";
    }
    else
    {
      LOG(LOG_RAW) << " OK.\n";
      return true;
    }
  }
  return false;
}


SerialTty::SerialTty()
  : m_p(new SerialTtyPrivate())
{
  LOG(LOG_INF) << "Using SerialTty...\n";

  m_p->m_fd = -1;
  m_p->m_writeDelay = 0;
}

SerialTty::~SerialTty()
{
  lprintf(LOG_DBG2, "%s\n", __FUNCTION__);
}

#define ENSURE_OR_RETURN_FALSE(e)                           \
  do                                                        \
  {                                                         \
    if((e)<0)                                               \
    {                                                       \
      /*perror(#e);*/                                       \
      const char* se=strerror(errno);                       \
      LOG(antpm::LOG_ERR) << se << "\n";                    \
      return false;                                         \
    }                                                       \
  } while(false)

// No need to inherit from binary_function for c++11 or later
#if __cplusplus >= 201103L
struct contains {
#else
struct contains : public std::binary_function<std::vector<std::string>, std::string,bool> {
#endif
  inline bool operator() (std::vector<std::string> v, std::string e) const {return find(v.begin(), v.end(), e) != v.end();}
};

bool
SerialTty::open()
{
  close();

  bool rv = false;

  std::vector<std::string> guessedNames;
  m_p->guessDeviceName(guessedNames);
  if(!m_p->openDevice(guessedNames))
  {
    std::vector<std::string> possibleNames;
    possibleNames.push_back("/dev/ttyUSB0");
    possibleNames.push_back("/dev/ttyUSB1");
    possibleNames.push_back("/dev/ttyUSB2");
    possibleNames.push_back("/dev/ttyUSB3");
    possibleNames.push_back("/dev/ttyUSB4");
    possibleNames.push_back("/dev/ttyUSB5");
    possibleNames.push_back("/dev/ttyUSB6");
    possibleNames.push_back("/dev/ttyUSB7");
    possibleNames.push_back("/dev/ttyUSB8");
    possibleNames.push_back("/dev/ttyUSB9");
    possibleNames.push_back("/dev/cu.ANTUSBStick.slabvcp");
    possibleNames.push_back("/dev/cu.ANTUSBStick.slabvcp");

    possibleNames.erase(remove_if(possibleNames.begin(),
                                  possibleNames.end(),
                                  bind(contains(), guessedNames, std::placeholders::_1)),
                        possibleNames.end());

    m_p->openDevice(possibleNames);
  }

  if(m_p->m_fd<0)
  {
    // Ofcourse you don't want to be running as root, so add your user to the group dialout like so:
    //
    // sudo usermod -a -G dialout yourUserName
    //
    // Log off and log on again for the changes to take effect!
    char se[256];
    auto unused = strerror_r(m_p->m_fd, se, sizeof(se));
    (void)unused;
    LOG(antpm::LOG_ERR) << "Opening serial port failed! Make sure cp210x kernel module is loaded, and /dev/ttyUSBxxx was created by cp210x!\n"
                        << "\tAlso make sure that /dev/ttyUSBxxx is R+W accessible by your user (usually enabled through udev.rules)!\n";
    LOG(antpm::LOG_ERR) << "error=" << m_p->m_fd << ", strerror=" << se << "\n";
    return rv;
  }

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

  m_p->m_recvThKill = false;
  SerialTtyIOThread recTh;
  m_p->m_recvTh = std::thread(recTh, this);

  return true;
}



void
SerialTty::close()
{
  m_p->m_recvThKill = true;

  {
    std::unique_lock<std::mutex> lock(m_p->m_queueMtx);
    m_p->m_condQueue.notify_all();
  }

  if(m_p->m_recvTh.joinable())
  {
    m_p->m_recvTh.join();
  }

  if(m_p->m_fd >= 0)
  {
    ::close(m_p->m_fd);
  }
  m_p->m_fd = -1;
  m_p->m_writeDelay = 0;
}


bool
SerialTty::read(char* dst, const size_t sizeBytes, size_t& bytesRead)
{
  if(!dst)
    return false;

  std::unique_lock<std::mutex> lock(m_p->m_queueMtx);

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
    std::unique_lock<std::mutex> lock(m_p->m_queueMtx);

    //while(m_recvQueue.empty()) // while - to guard agains spurious wakeups
    {
      using namespace std::chrono_literals;
      m_p->m_condQueue.wait_for(lock, timeout_ms*1ms);
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

  if(m_p->m_writeDelay>0 && m_p->m_writeDelay<=10)
    sleepms(m_p->m_writeDelay);

  return true;
}



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
  std::unique_lock<std::mutex> lock(m_p->m_queueMtx);
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


bool
SerialTty::setWriteDelay(const size_t ms)
{
  m_p->m_writeDelay = ms;
  return true;
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
    std::unique_lock<std::mutex> lock(m_p->m_queueMtx);
    for(ssize_t i = 0; i < rb; i++)
      m_p->m_recvQueue.push(buf[i]);
    m_p->m_condQueue.notify_one();
  }
}


}


