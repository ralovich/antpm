// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
////////////////////////////////////////////////////////////////////
//                                                                //
// Copyright (c) 2011-2013 RALOVICH, Kristóf                      //
//                                                                //
// This program is free software; you can redistribute it and/or  //
// modify it under the terms of the GNU General Public License    //
// version 2 as published by the Free Software Foundation.        //
//                                                                //
////////////////////////////////////////////////////////////////////
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


#define BOOST_TEST_MODULE DateTime
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


BOOST_AUTO_TEST_CASE(load_fit_date)
{
  antpm::Log::instance()->addSink(std::cout);

  const std::string in(TEST_ROOT"/0046.fit");
  std::cout << in << "\n";

  vector<uchar> fitData(readFile(in.c_str()));
  BOOST_CHECK(!fitData.empty());

  FIT fit;
  GPX gpx;
  BOOST_CHECK(fit.parse(fitData, gpx));

  time_t lastWrite             = fs::last_write_time(fs::path(in));
  time_t fileCreationTimestamp = fit.getCreationTimestamp();
  time_t first                 = fit.getFirstTimestamp();
  time_t last                  = fit.getLastTimestamp();

  char tbuf[256];
  strftime(tbuf, sizeof(tbuf), "%d-%m-%Y %H:%M:%S", localtime(&lastWrite));

  cout << in << ", lw=" << tbuf
       << ", ct=" << GarminConvert::localTime(fileCreationTimestamp)
       << ", f=" << GarminConvert::localTime(first)
       << ", l=" << GarminConvert::localTime(last) << "\n";

  fileCreationTimestamp = GarminConvert::gOffsetTime(fileCreationTimestamp);
  BOOST_CHECK(DeviceSettings::time2str(fileCreationTimestamp)=="2013-07-08T18:39:45Z");

  // BOOST_CHECK(first == 7/8/2013 8:39:49 PM CEST);
  // BOOST_CHECK(last == 7/8/2013 8:40:18 PM CEST);
  first = GarminConvert::gOffsetTime(first);
  last  = GarminConvert::gOffsetTime(last);
  BOOST_CHECK(DeviceSettings::time2str(first)=="2013-07-08T18:39:49Z");
  BOOST_CHECK(DeviceSettings::time2str(last) =="2013-07-08T18:40:18Z");

  //std::time_t t = fileCreationTime;
  //t = GarminConvert::gOffsetTime(t);
  //boost::filesystem::last_write_time(boost::filesystem::path(in), t);
}

