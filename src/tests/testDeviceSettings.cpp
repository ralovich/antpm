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


using namespace antpm;

BOOST_AUTO_TEST_CASE( free_test_function )
{
  BOOST_CHECK( true /* test assertion */ );
}

BOOST_AUTO_TEST_CASE( dummy2_test )
{
  BOOST_CHECK_EQUAL( 6, 6 );
}

int add( int i, int j ) { return i+j; }

BOOST_AUTO_TEST_CASE( my_test )
{
    // seven ways to detect and report the same error:
    BOOST_CHECK( add( 2,2 ) == 4 );        // #1 continues on error

    BOOST_REQUIRE( add( 2,2 ) == 4 );      // #2 throws on error

    if( add( 2,2 ) != 4 )
      BOOST_ERROR( "Ouch..." );            // #3 continues on error

    if( add( 2,2 ) != 4 )
      BOOST_FAIL( "Ouch..." );             // #4 throws on error

    if( add( 2,2 ) != 4 ) throw "Ouch..."; // #5 throws on error

    BOOST_CHECK_MESSAGE( add( 2,2 ) == 4,  // #6 continues on error
                         "add(..) result: " << add( 2,2 ) );

    BOOST_CHECK_EQUAL( add( 2,2 ), 4 );	  // #7 continues on error
}

namespace antpm
{

template<>
Log*
ClassInstantiator<Log>::instantiate()
{
  return new Log(NULL);
}

}

BOOST_AUTO_TEST_CASE(convert)
{
  // $ date -u +%F\ %X\ %s
  // 2013-07-06 08:40:16 PM 1373143216
  // 2013-07-06 09:01:28 PM 1373144488
  {
    const char* s="2013-07-06T20:40:16Z";
    std::time_t t=1373143216;

    //std::cout << DeviceSettings::str2time(s) << "\n";

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
  antpm::Log::instance()->addSink(std::cout);

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


  BOOST_CHECK(m_ds->loadFromFile(fname_tmp));
  BOOST_CHECK(m_ds->MaxFileDownloads==1000);
  //std::cout << m_ds->LastUserProfileTime << "\n"; // 946684800
  //std::cout << m_ds->LastTransferredTime << "\n"; // 946684800
  //std::cout << DeviceSettings::time2str(m_ds->LastUserProfileTime) << "\n"; // 2000-01-01T00:00:00Z
  //std::cout << DeviceSettings::time2str(m_ds->LastTransferredTime) << "\n"; // 2000-01-01T00:00:00Z
  BOOST_CHECK(DeviceSettings::time2str(m_ds->LastUserProfileTime)=="2000-01-01T00:00:00Z");
  BOOST_CHECK(DeviceSettings::time2str(m_ds->LastTransferredTime)=="2000-01-01T00:00:00Z");
  BOOST_CHECK(m_ds->LastUserProfileTime == 946684800);
  BOOST_CHECK(m_ds->LastTransferredTime == 946684800);


}


// a test fixture or test context is the collection of one or more of the following items, required to perform the test:
//  preconditions
//  particular states of tested unites
//  necessary cleanup procedures
class MyFixture { public: MyFixture() : foo(0) { /* setup here */} int foo; };

BOOST_FIXTURE_TEST_CASE( my_test_fixture, MyFixture )
{
  BOOST_CHECK_EQUAL(0, foo);
}


