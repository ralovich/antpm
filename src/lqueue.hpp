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


#include <queue>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>
#include <list>





template <typename DataType>
class lqueue2
{
public:
  typedef std::list<DataType> DataList;
public:
  bool empty() const
  {
    boost::mutex::scoped_lock lock(m_mtx);
    return m_q.empty();
  }

  void push(DataType const& data)
  {
    boost::mutex::scoped_lock lock(m_mtx);
    m_q.push_back(data);
    m_pushEvent.notify_all();
  }

  void pushArray(DataType const* data, const size_t nItems)
  {
    boost::mutex::scoped_lock lock(m_mtx);
    for(size_t i = 0; i < nItems; i++)
      m_q.push_back(data[i]);
    m_pushEvent.notify_all();
  }


  const typename std::queue<DataType>::size_type
  size() const
  {
    boost::unique_lock<boost::mutex> lock(m_mtx);
    return m_q.size();
  }

  const DataList
  getListCopy() const
  {
    boost::unique_lock<boost::mutex> lock(m_mtx);
    return m_q; // copy
  }

  void
  clear()
  {
    boost::unique_lock<boost::mutex> lock(m_mtx);
    m_q.clear();
  }

protected:
  mutable boost::mutex m_mtx;
  boost::condition_variable m_pushEvent;
  DataList m_q;
};



// implements push consumer
template < class DataType>
class lqueue3 : public lqueue2<DataType>
{
public:
  typedef boost::function<bool (DataType&)>     Listener;
  typedef boost::function<bool (std::vector<DataType>&)> Listener2;
  typedef lqueue2<DataType>                              Super;

  struct ListenerProc
  {
    void operator() (lqueue3* This)
    {
      This->eventLoop();
    }
  };


  lqueue3(bool eventLoopInBgTh = true)
    : stop(false)
  {
    if(eventLoopInBgTh)
      th_listener.reset( new boost::thread(lp, this) );
  }

  ~lqueue3()
  {
    stop = true;
    if(th_listener.get())
      th_listener->join();
  }

  void
  kill()
  {
    stop = true;
  }

  void
  setOnDataArrivedCallback(Listener2 l)
  {
    mCallback = l;
  }

public:
  void eventLoop()
  {
    while(!stop)
    {
      boost::unique_lock<boost::mutex> lock(Super::m_mtx);

      boost::posix_time::time_duration td = boost::posix_time::milliseconds(2000);
      if(!Super::m_pushEvent.timed_wait(lock, td)) // will automatically and atomically unlock mutex while it waits
      {
        //std::cout << "no event before timeout\n";
        continue;
      }

      assert(!Super::m_q.empty());
      size_t s = Super::m_q.size();
      std::vector<DataType> v(s);
      for(size_t i = 0; i < s; i++)
      {
        v[i] = Super::m_q.front();
        Super::m_q.pop_front();
      }
      if(mCallback)
        /*bool rv =*/ mCallback(v);
    }
  }

protected:
  ListenerProc lp;
  boost::scoped_ptr<boost::thread> th_listener;
  volatile bool stop;
  Listener2 mCallback;
};


// implements poll-able pop consumer
template < class DataType>
class lqueue4 : public lqueue2<DataType>
{
public:
  typedef lqueue2<DataType>                 Super;

  template < class Cmp >
  bool
  tryFindPop(DataType& needle, Cmp cmp)
  {
    boost::mutex::scoped_lock lock(Super::m_mtx);
    typename Super::DataList::iterator i;
    for(i = Super::m_q.begin(); i != Super::m_q.end(); i++)
    {
      if(cmp(needle, *i))
        break;
    }
    if(i==Super::m_q.end())
      return false;
    needle = *i; // copy
    Super::m_q.erase(i);
    return true;
  }
  bool
  pop(DataType& data, const size_t timeout = 0)
  {
    boost::mutex::scoped_lock lock(Super::m_mtx);

    /// if queue empty, wait untile timeout if there was anything pushed
    if(Super::m_q.empty() && timeout > 0)
    {
      boost::posix_time::time_duration td = boost::posix_time::milliseconds(timeout);
      if(!Super::m_pushEvent.timed_wait(lock, td))
        return false;
    }
    assert(!Super::m_q.empty());

    data = Super::m_q.front();
    Super::m_q.pop_front();
    return true;
  }

  bool
  popArray(DataType* dst, const size_t sizeBytes, size_t& bytesRead, const size_t timeout = 0)
  {
    if(!dst)
      return false;

    boost::unique_lock<boost::mutex> lock(Super::m_mtx);

    /// if queue empty, wait until timeout if there was anything pushed
    if(Super::m_q.empty() && timeout > 0)
    {
      boost::posix_time::time_duration td = boost::posix_time::milliseconds(timeout);
      if(!Super::m_pushEvent.timed_wait(lock, td))
      {
        bytesRead = 0;
        return false;
      }
    }
    assert(!Super::m_q.empty());

    size_t s = Super::m_q.size();
    s = std::min(s, sizeBytes);
    for(size_t i = 0; i < s; i++)
    {
      *(dst+i) = Super::m_q.front();
      Super::m_q.pop_front();
    }
    bytesRead = s;

    return true;
  }
};


