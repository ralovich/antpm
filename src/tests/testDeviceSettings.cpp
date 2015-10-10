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


#define BOOST_TEST_MODULE DeviceSettings
//#include <boost/test/included/unit_test.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace antpm;


DEFAULT_LOG_INSTANTIATOR


BOOST_AUTO_TEST_CASE( free_test_function )
{
  BOOST_CHECK( true /* test assertion */ );

  antpm::Log::instance()->addSink(std::cout);

  LOG(LOG_INF) << getVersionString() << "\n";
}

BOOST_AUTO_TEST_CASE(convert_0)
{
  // $ date -u +%F\ %X\ %s
  // 2013-07-06 08:40:16 PM 1373143216
  // 2013-07-06 09:01:28 PM 1373144488
  {
    const char* s="2013-07-06T20:40:16Z";
    std::time_t t=1373143216;
    (void)(s);
    (void)(t);

    //LOG(LOG_INF) << DeviceSettings::str2time(s) << "\t" << t << "\n" << std::endl;
  }

  BOOST_CHECK( true /* test assertion */ );
}

BOOST_AUTO_TEST_CASE(convert)
{
  // $ date -u +%F\ %X\ %s
  // 2013-07-06 08:40:16 PM 1373143216
  // 2013-07-06 09:01:28 PM 1373144488
  {
    const char* s="2013-07-06T20:40:16Z";
    std::time_t t=1373143216;

    //LOG(LOG_INF) << DeviceSettings::str2time(s) << "\t" << t << "\n" << std::endl;

    BOOST_CHECK(DeviceSettings::str2time(s) == t);

    //std::cout << DeviceSettings::time2str(t) << "\n";

    BOOST_CHECK(DeviceSettings::time2str(t)==s);
  }
  {
    const char* s="2013-07-06T21:01:28Z";
    std::time_t t=1373144488;

    //std::cout << DeviceSettings::str2time(s) << "\n";

    BOOST_CHECK(DeviceSettings::str2time(s) == t);

    //std::cout << DeviceSettings::time2str(t) << "\n";

    BOOST_CHECK(DeviceSettings::time2str(t)==s);
  }

}

BOOST_AUTO_TEST_CASE(load_save)
{
  boost::scoped_ptr<DeviceSettings> m_ds;
  uint clientSN = 1279010136;
  const char* fname = TEST_ROOT"/config.ini";
  std::cout << fname << "\n";
  const char* fname_tmp = TEST_ROOT"/config_tmp.ini";

  m_ds.reset(new DeviceSettings(toStringDec<uint>(clientSN).c_str()));
  assert(m_ds.get());
  m_ds->loadDefaultValues();
  boost::filesystem::remove(fname_tmp);
  BOOST_CHECK(m_ds->saveToFile(fname_tmp));


  BOOST_CHECK(m_ds->loadFromFile(fname));
  BOOST_CHECK(m_ds->MaxFileDownloads==1333);
  //std::cout << m_ds->LastUserProfileTime << "\n"; // 1304868688
  //std::cout << m_ds->LastTransferredTime << "\n"; // 1340218950
  BOOST_CHECK(DeviceSettings::time2str(m_ds->LastUserProfileTime)=="2011-05-08T15:31:28Z");
  BOOST_CHECK(DeviceSettings::time2str(m_ds->LastTransferredTime)=="2012-06-20T19:02:30Z");
  BOOST_CHECK(m_ds->LastUserProfileTime == 1304868688);
  BOOST_CHECK(m_ds->LastTransferredTime == 1340218950);
  BOOST_CHECK(m_ds->SerialWriteDelayMs  == 3);


  BOOST_CHECK(m_ds->loadFromFile(fname_tmp));
  BOOST_CHECK(m_ds->MaxFileDownloads==1000);
  std::cout << m_ds->LastUserProfileTime << "\n"; // 946684800
  std::cout << m_ds->LastTransferredTime << "\n"; // 946684800
  std::cout << DeviceSettings::time2str(m_ds->LastUserProfileTime) << "\n"; // 2000-01-01T00:00:00Z
  std::cout << DeviceSettings::time2str(m_ds->LastTransferredTime) << "\n"; // 2000-01-01T00:00:00Z
  BOOST_CHECK(DeviceSettings::time2str(m_ds->LastUserProfileTime)=="2000-01-01T00:00:00Z");
  BOOST_CHECK(DeviceSettings::time2str(m_ds->LastTransferredTime)=="2000-01-01T00:00:00Z");
  BOOST_CHECK(m_ds->LastUserProfileTime == 946684800);
  BOOST_CHECK(m_ds->LastTransferredTime == 946684800);
  BOOST_CHECK(m_ds->SerialWriteDelayMs  == 3);


}



