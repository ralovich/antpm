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

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>

#include "AntMessage.hpp"
#include "AntFr310XT.hpp"
#include "common.hpp"
#include "Log.hpp"

#include <boost/program_options.hpp>
#include <boost/bind.hpp>


#ifdef _WIN32
# include <Windows.h>
#elif defined(__linux)
# include <signal.h>
#endif

namespace po = boost::program_options;
using namespace std;
using namespace antpm;


namespace antpm
{

template<>
Log*
ClassInstantiator<Log>::instantiate()
{
  mkDirNoLog(getConfigFolder().c_str());
  std::string l=getConfigFolder() + "/antpm_" + getDateString() + ".txt";
  return new Log(l.c_str());
}

}



boost::function<void(void)> stopFunc;

static
void
stopIt()
{
  if(stopFunc)
    stopFunc();
}

#ifdef _WIN32
BOOL CtrlHandler( DWORD fdwCtrlType )
{
  switch( fdwCtrlType )
  {
    // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
      printf( "Ctrl-C event\n\n" );
      //Beep( 750, 300 );
      stopIt();
      return( TRUE );

      // CTRL-CLOSE: confirm that the user wants to exit.
    case CTRL_CLOSE_EVENT:
      //Beep( 600, 200 );
      printf( "Ctrl-Close event\n\n" );
      stopIt();
      return( TRUE );

      // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
      //Beep( 900, 200 );
      printf( "Ctrl-Break event\n\n" );
      stopIt();
      return FALSE;

    case CTRL_LOGOFF_EVENT:
      //Beep( 1000, 200 );
      printf( "Ctrl-Logoff event\n\n" );
      stopIt();
      return FALSE;

    case CTRL_SHUTDOWN_EVENT:
      //Beep( 750, 500 );
      printf( "Ctrl-Shutdown event\n\n" );
      stopIt();
      return FALSE;

    default:
      return FALSE;
  }
}
#elif defined(__linux)
void my_handler(int s)
{
  printf("Caught signal %d\nantpm teardown initiated\n\n",s);
  stopIt();
}
#endif


int
main(int argc, char** argv)
{
  printf("press Ctrl-C to exit\n");
#ifdef _WIN32
  if( !SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) )
  {
    printf( "\nERROR: Could not set control handler...");
  }
#elif defined(__linux)
  signal(SIGINT, my_handler);
#endif

#ifndef NDEBUG
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_DBG);
#else
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_INF);
#endif
  antpm::Log::instance()->addSink(std::cout);

  // Declare the supported options.
  bool pairing         =false;
  bool dirOnly         =false;
  uint16_t dlFileIdx   =0x0000;
  uint16_t eraseFileIdx=0x0000;
  int         verbosityLevel = antpm::Log::instance()->getLogReportingLevel();
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",                                                                    "produce help message")
    ("pairing,P", po::value<bool>(&pairing)->zero_tokens()->implicit_value(true), "Force pairing first")
    ("dir-only",  po::value<bool>(&dirOnly)->zero_tokens()->implicit_value(true), "Download and list device directory")
    ("download,D",po::value<std::string>(),                                       "Download a single file (hex id e.g. 0x12FB) from device")
    ("erase",     po::value<std::string>(),                                       "Erase a single file (hex id e.g. 0x12FB) from device")
    ("v",                po::value<int>(&verbosityLevel),       "Adjust verbosity level, varies in [1, 6]")
    ("version,V",                                               "Print version information")
    ;

  std::vector<const char*> args(argv, argv+argc);
  po::variables_map vm;
  try
  {
    //po::parsed_options parsed = po::parse_command_line(argc, argv, desc);
    po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).run();
    po::store(parsed, vm);

    if(vm.find("download")!=vm.end())
    {
      std::stringstream interpreter;
      interpreter << std::hex << vm["download"].as<std::string>();
      interpreter >> dlFileIdx;
    }
    if(vm.find("erase")!=vm.end())
    {
      std::stringstream interpreter;
      interpreter << std::hex << vm["erase"].as<std::string>();
      interpreter >> eraseFileIdx;
    }

    po::notify(vm);
  }
  catch(po::error& error)
  {
    cerr << error.what() << "\n";
    cerr << desc << "\n";
    return EXIT_FAILURE;
  }

  if(vm.count("version") || vm.count("V"))
  {
    cout << argv[0] << " " << antpm::getVersionString() << "\n";
    return EXIT_SUCCESS;
  }

  if(vm.count("v"))
  {
    if(verbosityLevel >= antpm::LOG_ERR && verbosityLevel <= antpm::LOG_DBG3)
    {
      antpm::Log::instance()->setLogReportingLevel(static_cast<antpm::LogLevel>(verbosityLevel));
    }
  }

  if(vm.count("help"))
  {
    logger() << desc << "\n";
    return EXIT_SUCCESS;
  }
  LOG_VAR4(pairing, dirOnly, int(dlFileIdx), int(eraseFileIdx));


  AntFr310XT2 watch2(false);
  stopFunc = boost::bind(&AntFr310XT2::stopAsync, &watch2);
  {
    watch2.setModeDownloadAll();
    if(pairing) watch2.setModeForcePairing();
    if(dirOnly) watch2.setModeDirectoryListing();
    else if(dlFileIdx!=0x0000) watch2.setModeDownloadSingleFile(dlFileIdx);
    else if(eraseFileIdx!=0x000) watch2.setModeEraseSingleFile(eraseFileIdx);
    
    watch2.start();


    watch2.stop();
  }



  return EXIT_SUCCESS;
}
