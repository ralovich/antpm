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

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>

#include "AntMessage.hpp"
#include "common.hpp"
#include "Log.hpp"

#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>

namespace po = boost::program_options;
using namespace std;
using namespace antpm;

namespace antpm
{

template<>
Log*
ClassInstantiator<Log>::instantiate()
{
  return new Log(NULL);
}

}


int
main(int argc, char** argv)
{
#ifndef NDEBUG
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_DBG);
#else
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_INF);
#endif

  istream* in=0;
  ifstream f;

  // Declare the supported options.
  //bool dump=false;
  //bool usbmon=false;
  //bool filter=false;
  string op;
  string inputFile;
  po::positional_options_description pd;
  pd.add("input-file", 1);
  po::options_description desc("Allowed options");
  po::variables_map vm;
  try
  {
    desc.add_options()
    ("help,h",                                                       "produce help message")
    ("op,O", po::value<string>(&op)->default_value("parse"), "possible modes of operation: parse|dump|usbmon|filter|count")
    //("d", po::value<bool>(&dump)->zero_tokens(), "diffable byte dumps + decoded strings")
    //("u", po::value<bool>(&usbmon)->zero_tokens(), "generate pseudo usbmon output")
    //("f", po::value<bool>(&filter)->zero_tokens(), "just filter ANT messages from usbmon stream")
    ("input-file,P", po::value<string>(&inputFile)->zero_tokens(),   "input file, if not given reads from standard input")
    ("version,V",                                               "Print version information")
    ;

    //po::parsed_options parsed = po::parse_command_line(argc, argv, desc);
    po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).positional(pd).run();
    po::store(parsed, vm);
    po::notify(vm);
  }
  catch(po::error& error)
  {
    cerr << error.what() << "\n";
    cerr << desc << "\n";
    return EXIT_FAILURE;
  }
  catch(boost::exception& e)
  {
    cerr << boost::diagnostic_information(e) << std::endl;
    return EXIT_FAILURE;
  }
  catch(std::exception& ex)
  {
    cerr << ex.what() << "\n";
    cerr << desc << "\n";
    return EXIT_FAILURE;
  }


  if(vm.count("version") || vm.count("V"))
  {
    cout << argv[0] << " " << antpm::getVersionString() << "\n";
    return EXIT_SUCCESS;
  }

  if(vm.count("help"))
  {
    cout << desc << "\n";
    return EXIT_SUCCESS;
  }
  //cout << dump << "\n";
  //cout << inputFile << "\n";

  if(!inputFile.empty())
  {
    f.open(inputFile.c_str());
    if(!f.is_open())
      return EXIT_FAILURE;
    in=&f;
  }
  else
  {
    in=&cin;
  }

  //if(argc==2)
  //{
  //  f.open(argv[1]);
  //  if(!f.is_open())
  //    return EXIT_FAILURE;
  //  in=&f;
  //}
  //else
  //{
  //  in=&cin;
  //}

  std::list<AntMessage> messages;

  size_t lineno=0;
  std::string line;
  unsigned long timeUs0=0;
  bool first=true;
  for( ;getline(*in, line); lineno++)
  {
    vector<string> vs(tokenize(line, " "));
    //cout << vs.size() << "\n";

    if(vs.size()<8 || vs[6]!="=" || vs[7].size()<2 || vs[7][0]!='a' || vs[7][1]!='4')
      continue;
    
    unsigned long timeUs=0;
    if(1!=sscanf(vs[1].c_str(), "%lu", &timeUs))
      continue;
    if(first)
    {
      first=false;
      timeUs0=timeUs;
    }
    double dt=(timeUs-timeUs0)/1000.0;
    timeUs0=timeUs;

    unsigned long usbExpectedBytes=0;
    if(1!=sscanf(vs[5].c_str(), "%lu", &usbExpectedBytes))
      continue;

    //cout << ".\n";
    std::string buf;
    for(size_t i=7;i<vs.size();i++)
      buf+=vs[i];
    //cout << buf << "\n";

    std::list<uchar> bytes;
    std::vector<AntMessage> messages2;

    if(!AntMessage::stringToBytes(buf.c_str(), bytes))
    {
      cerr << "\nDECODE FAILED [1]: line: " << lineno << " \"" << line << "\"!\n\n";
      continue;
    }
    size_t usbActualBytes=bytes.size();
    if(!AntMessage::interpret2(bytes, messages2))
    {
      if(usbActualBytes<usbExpectedBytes && !messages2.empty())
      {
        // long line truncated in usbmon, but we still managed to decoded one or more packets from it
        cerr << "\nTRUNCATED: line: " << lineno << " \"" << line << "\" decoded=" << messages2.size() << "!\n\n";
      }
      else
      {
        cerr << "\nDECODE FAILED [2]: line: " << lineno << " \"" << line << "\"!\n\n";
        continue;
      }
    }

    if(op=="count")
    {
      cout << lineno << "\t" << messages2.size() << "\n";
      continue;
    }
    else if(op=="filter")
    {
      cout << line << "\n";
      continue;
    }

    for (std::vector<AntMessage>::iterator i=messages2.begin(); i!=messages2.end(); i++)
    {
      i->idx = lineno;
      i->sent = (vs[2][0]=='S');
      AntMessage& m=*i;
      
      if(op=="usbmon")
      {
        messages.push_back(m);
      }
      else if(op=="dump")
      {
        cout << m.strExt() << "\n";
      }
      else
      {
        //cout << dt << "\t" << m.str() << "\n";
        cout << m.strDt(dt) << "\n";
      }
    }
  }

  if(op=="usbmon")
  {
    AntMessage::saveAsUsbMon(cout, messages);
  }

  return EXIT_SUCCESS;
}


