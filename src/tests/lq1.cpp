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



#include "common.hpp"
#include "lqueue.hpp"

#include <iostream>
#include <string>
#include <cctype>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>

#define BOOST_TEST_MODULE lq1
//#include <boost/test/included/unit_test.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace antpm;
//namespace fs = boost::filesystem;



DEFAULT_LOG_INSTANTIATOR

struct Producer
{
  lqueue2<int> _q;

  lqueue3<int> q;
  std::unique_ptr<boost::thread> q_th;

  lqueue3_bg<double> q_bg;
  volatile bool die;

  void operator() ()
  {
    while(!die)
    {
      _q.push(234);
      q.push(27);
      q_bg.push(435.5543);
      boost::thread::yield();
    }
  }

  void start_q_th()
  {
    q_th.reset(new boost::thread(boost::bind(&lqueue3<int>::eventLoop, &q)));
  }
};

bool
onDataArrivedI(std::vector<int>& v)
{
  cout << v.size() << endl;
  return true;
}

bool
onDataArrived(std::vector<double>& v)
{
  cout << v.size() << endl;
  return true;
}

BOOST_AUTO_TEST_CASE(test_lqueue1)
{
  antpm::Log::instance()->addSink(std::cout);
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_DBG3);

  BOOST_CHECK(true);



  Producer p;
  p.die = false;
  p.q.setOnDataArrivedCallback(onDataArrivedI);
  p.q_bg.setOnDataArrivedCallback(onDataArrived);

  // https://svn.boost.org/trac/boost/ticket/2144
  boost::thread th = boost::thread(boost::ref(p));
  p.start_q_th();

  boost::this_thread::sleep( boost::posix_time::milliseconds(20) );


  p.q.kill();

  p.die = true;
  th.join();

}

