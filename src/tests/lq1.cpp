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
#include <atomic>
#include <iostream>
#include <string>
#include <cctype>

#define BOOST_TEST_MODULE lq1
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace antpm;



DEFAULT_LOG_INSTANTIATOR


struct Producer
{
  lqueue2<float> _q;

  lqueue3<int> q;
  std::thread q_th;

  lqueue3_bg<double> q_bg;

  lqueue4<short> q4;
  std::thread q4_th;

  std::atomic<bool> die_producer = false;
  std::atomic<size_t> num_produced = 0;

  std::atomic<bool> die_receiver = false;
  size_t num_received_fg = 0;
  size_t num_received_fg4 = 0;
  size_t num_received_bg = 0;

  void operator() ()
  {
    while(!die_producer)
    {
      _q.push(234.5F);
      q.push(27+num_produced);
      q_bg.push(435.5543);
      q4.push(static_cast<short>(num_produced));
      num_produced++;
      std::this_thread::yield();
    }
  }

  // start foreground consumer
  void start_q_th()
  {
    q_th = std::thread([&]{
        q.eventLoop();
    });
    q4_th = std::thread([&]{
        while(!die_receiver)
        {
            std::vector<short> received(4);
            size_t elemsRead = 0;
            q4.popArray(received.data(), received.size(), elemsRead, 10);
            //assert(bytesRead % 2 == 0);
            num_received_fg4 += elemsRead;
            printf("l4: %zu\n", elemsRead);
        }
    });
  }

  void kill_producer()
  {
    die_producer = true;
  }

  void kill_consumers()
  {
    die_receiver = true;
    q.kill();
    q_bg.kill();
    if(q_th.joinable())
    {
      q_th.join();
    }
    if(q4_th.joinable())
    {
      q4_th.join();
    }
  }

  bool
  onDataArrivedFg(const std::vector<int>& v)
  {
    cout << "fg: " << v.size() << endl;
    num_received_fg += v.size();
    return true;
  }

  bool
  onDataArrivedBg(const std::vector<double>& v)
  {
    cout << "bg: " << v.size() << endl;
    num_received_bg += v.size();
    return true;
  }
};



BOOST_AUTO_TEST_CASE(test_lqueue1)
{
  antpm::Log::instance()->addSink(std::cout);
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_DBG3);

  BOOST_CHECK(true);



  Producer p;
  p.q.setOnDataArrivedCallback([&](const std::vector<int>& v){return p.onDataArrivedFg(v);});
  p.q_bg.setOnDataArrivedCallback([&](const std::vector<double>& v){return p.onDataArrivedBg(v);});

  // https://svn.boost.org/trac/boost/ticket/2144
  std::thread th = std::thread(std::ref(p)); // start producer
  p.start_q_th();

  using namespace std::chrono_literals;
  std::this_thread::sleep_for( 100ms );


  p.kill_producer();
  if(th.joinable())
  {
    th.join();
  }

  p.kill_consumers();
  std::cout << p.num_produced << std::endl;
  std::cout << p.num_received_fg << std::endl;
  std::cout << p.num_received_fg4 << std::endl;
  std::cout << p.num_received_bg << std::endl;
  BOOST_CHECK(p.num_produced > 0);

  BOOST_CHECK(p.num_produced == p.num_received_fg);
  BOOST_CHECK(p.num_produced == p.num_received_fg4);
  BOOST_CHECK(p.num_produced == p.num_received_bg);
}

