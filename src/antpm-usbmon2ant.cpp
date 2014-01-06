// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
////////////////////////////////////////////////////////////////////
// Copyright (c) 2011-2014 RALOVICH, Kristóf                      //
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
#include "common.hpp"
#include "Log.hpp"
#include "GarminPacketIntf.hpp"
#include "garmintools/garmin.h"

#include <boost/program_options.hpp>

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


// Interpret a sequence of burst messages while replaying a trace file.
class BurstDecodeVisitor
{
public:
  void onMessage(const AntMessage& m);
  void onLastBurst(const uchar chan);
  bool tryDecodeDirect(const uchar chan, std::vector<uint8_t> &burstData);
protected:
  map<uchar, list<AntMessage> > chanBursts; // <channel, aggregated burst> pairs
  map<uchar, int> chanLastCmd;              // <channel, pid> pairs
};


void
BurstDecodeVisitor::onMessage(const AntMessage &m)
{
  if(m.bytes.size()<3 || m.getMsgId()!=MESG_BURST_DATA_ID || m.getLenPayload()!=9)
    return;
  const M_ANT_Burst* burst(reinterpret_cast<const M_ANT_Burst*>(m.getPayloadRef()));

  uchar chan = burst->chan;


  chanBursts[chan].push_back(m);

  if(burst->isLast())
    onLastBurst(chan);
}

void
BurstDecodeVisitor::onLastBurst(const uchar chan)
{
  list<AntMessage>& msgs = chanBursts[chan];
  std::vector<uint8_t> burstData;
  uchar expectedSeq=0;
  //bool found=false;
  bool lastFound=false;

  for(std::list<AntMessage>::iterator i = msgs.begin(); i != msgs.end(); i++)
  {
    AntMessage& repl(*i);
    assert(repl.getLenPayload()==9);
    const M_ANT_Burst* burst(reinterpret_cast<const M_ANT_Burst*>(repl.getPayloadRef()));
    //CHECK_RETURN_FALSE(burst->seq == expectedSeq);
    ++expectedSeq;
    if(expectedSeq == 4)
      expectedSeq = 1;
    //found = true;
    std::vector<uchar> crtBurst(repl.getPayload());
    burstData.insert(burstData.end(), crtBurst.begin()+1,crtBurst.end());
    assert(burstData.size()%8==0);
    lastFound = burst->isLast();
    if(lastFound) break;
  }
  msgs.clear();

  tryDecodeDirect(chan, burstData);
}


bool
BurstDecodeVisitor::tryDecodeDirect(const uchar chan, std::vector<uint8_t>& burstData)
{
  CHECK_RETURN_FALSE(burstData.size()>=8);
  const M_ANTFS_Beacon* beac(reinterpret_cast<const M_ANTFS_Beacon*>(&burstData[0]));
  const M_ANTFS_Command* cmd(reinterpret_cast<const M_ANTFS_Command*>(&burstData[0]));
  std::vector<uint8_t> data;
  GarminPacketIntf gpi;

  if(beac->beaconId==ANTFS_BeaconId)
  {
    // beacon+response
    CHECK_RETURN_FALSE(burstData.size()>=2*8);
    const M_ANTFS_Response* resp(reinterpret_cast<const M_ANTFS_Response*>(&burstData[8]));
    CHECK_RETURN_FALSE(resp->responseId==ANTFS_CommandResponseId);
    //CHECK_RETURN_FALSE(resp->response==ANTFS_RespDirect);
    if(resp->response!=ANTFS_RespDirect) return false;

    logger() << "expecting " << resp->detail.directResponse.data << "x8 bytes of direct data, plus 16 bytes\n";
    logger() << "got back = \"" << burstData.size() << "\" bytes\n";
    CHECK_RETURN_FALSE(burstData.size()==size_t((2+resp->detail.directResponse.data)*8));

    data = vector<uint8_t>(burstData.begin()+16, burstData.end());

    bool rv = gpi.interpret(this->chanLastCmd[chan], data);
    this->chanLastCmd[chan] = -1;
    return rv;
  }
  else if(cmd->commandId==ANTFS_CommandResponseId)
  {
    // command
    //CHECK_RETURN_FALSE(cmd->command==ANTFS_CmdDirect);
    if(cmd->command!=ANTFS_CmdDirect) return false;

    logger() << "got = \"" << burstData.size() << "\" bytes\n";
    CHECK_RETURN_FALSE(burstData.size()==size_t((1+1)*8));

    data = vector<uint8_t>(burstData.begin()+8, burstData.end());

    this->chanLastCmd[chan] = gpi.interpretPid(data);

    return gpi.interpret(-1, data);
  }
  else
  {
    LOG(LOG_WARN) << "Couldn't decode burst data starting with 0x" << toString<int>((int)burstData[0], 2, '0') << " !\n";
    return false;
  }
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
  desc.add_options()
    ("help,h",                                                       "produce help message")
    ("op,operation", po::value<string>(&op)->default_value("parse"), "possible modes of operation: parse|dump|usbmon|filter|count")
    //("d", po::value<bool>(&dump)->zero_tokens(), "diffable byte dumps + decoded strings")
    //("u", po::value<bool>(&usbmon)->zero_tokens(), "generate pseudo usbmon output")
    //("f", po::value<bool>(&filter)->zero_tokens(), "just filter ANT messages from usbmon stream")
    ("input-file,P", po::value<string>(&inputFile)->zero_tokens(),   "input file, if not given reads from standard input")
    ("version,V",                                               "Print version information")
    ;

  po::variables_map vm;
  try
  {
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
  BurstDecodeVisitor burstDecoder;

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
        burstDecoder.onMessage(m);
      }
    }
  }

  if(op=="usbmon")
  {
    AntMessage::saveAsUsbMon(cout, messages);
  }

  return EXIT_SUCCESS;
}


