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

#include "AntMessenger.hpp"
#include "antdefs.hpp"
#include "DeviceSettings.hpp"
#include "FIT.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <functional>
#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include "common.hpp"
#include <boost/thread/thread_time.hpp>
#include <boost/foreach.hpp>

#include <boost/filesystem.hpp>


#define BOOST_TEST_MODULE Replay
//#include <boost/test/included/unit_test.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace antpm;
namespace fs = boost::filesystem;

namespace antpm
{

template<>
Log*
ClassInstantiator<Log>::instantiate()
{
  return new Log(NULL);
}

}


BOOST_AUTO_TEST_CASE(replay_1)
{
  antpm::Log::instance()->addSink(std::cout);

  LOG(LOG_INF) << "replay_1";

  BOOST_CHECK(true);
}

