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
#include "FIT.hpp"
#include "common.hpp"
#include "Log.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/exception/diagnostic_information.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using namespace std;
using namespace antpm;


DEFAULT_LOG_INSTANTIATOR

const
std::vector<fs::path>
find_files(const fs::path & dir_path,         // in this directory,
           const std::string & ext // search for this extension,
           )
{
  std::vector<fs::path> paths;
  if ( !fs::exists( dir_path ) ) return paths;
  fs::directory_iterator end_itr; // default construction yields past-the-end
  for ( fs::directory_iterator itr( dir_path );
    itr != end_itr;
    ++itr )
  {
    if ( fs::is_directory(itr->status()) )
    {
      std::vector<fs::path> paths2(find_files( itr->path(), ext) );
      paths.insert(paths.end(), paths2.begin(), paths2.end());
    }
    else if ( fs::extension(*itr) == ext ) // see below
    {
      paths.push_back(*itr);
    }
  }
  return paths;
}


int
main(int argc, char** argv)
{
  antpm::Log::instance()->addSink(std::cout);
#ifndef NDEBUG
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_DBG);
#else
  antpm::Log::instance()->setLogReportingLevel(antpm::LOG_INF);
#endif

  // Declare the supported options.
  std::string fitFolder;
  std::string fitRootFile;
  bool        fixLastWrt = false;
  int         verbosityLevel = antpm::Log::instance()->getLogReportingLevel();
  po::options_description desc("Allowed options");
  std::vector<const char*> args(argv, argv+argc);
  po::variables_map vm;
  try
  {
    desc.add_options()
    ("help,h",                                                  "produce help message")
    ("fitFolder,F",      po::value<std::string>(&fitFolder),    "Folder with FIT files")
    ("decode-fit-root,D", po::value<std::string>(&fitRootFile), "FIT file, encoding the root directory contents on a device, e.g. /tmp/0000.fit")
    ("fix-last-write,L", po::value<bool>(&fixLastWrt)->zero_tokens()->implicit_value(true), "Correct last-written time of .fit files on disk. No conversion to GPX takes place.")
    ("v",                po::value<int>(&verbosityLevel),       "Adjust verbosity level, varies in [1, 6]")
    ("version,V",                                               "Print version information")
    ;

    //po::parsed_options parsed = po::parse_command_line(argc, argv, desc);
    po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).run();
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

  if(vm.count("v"))
  {
    if(verbosityLevel >= antpm::LOG_ERR && verbosityLevel <= antpm::LOG_DBG3)
    {
      antpm::Log::instance()->setLogReportingLevel(static_cast<antpm::LogLevel>(verbosityLevel));
    }
  }

  if(vm.count("help") || vm.count("h")
     || (fitFolder.empty() && fitRootFile.empty()))
  {
    cout << desc << "\n";
    return EXIT_SUCCESS;
  }

  if(!fitRootFile.empty())
  {
    ZeroFileContent zfc;
    FIT fit;
    vector<uchar> v(readFile(fitRootFile.c_str()));
    if(fit.parseZeroFile(v, zfc))
       return EXIT_SUCCESS;
    else
      return EXIT_FAILURE;
  }

  if(!fs::is_directory(fs::path(fitFolder)))
  {
    LOG_VAR(fitFolder);
    CHECK_RETURN_RV_LOG_OK(fs::is_directory(fs::path(fitFolder)), EXIT_FAILURE);
  }

  std::vector<fs::path> fitFiles(find_files(fs::path(fitFolder), ".fit"));
  std::sort(fitFiles.begin(), fitFiles.end());

  for (size_t i = 0; i < fitFiles.size(); i++)
  {
    const std::string in(fitFiles[i].string());
    if(fixLastWrt) // for fixing up FIT file dates created before fixing time conversion bug
    {
      vector<uchar> fitBytes(readFile(in.c_str()));
      std::time_t fileCreationTime; // GMT/UTC
      bool has_ct = FIT::getCreationDate(fitBytes, fileCreationTime);
      time_t lastWrite             = fs::last_write_time(fs::path(in)); // GMT/UTC

      char tbuf[256];
      strftime(tbuf, sizeof(tbuf), "%d-%m-%Y %H:%M:%S", localtime(&lastWrite));

      char tbuf2[256];
      strftime(tbuf2, sizeof(tbuf2), "%d-%m-%Y %H:%M:%S", localtime(&fileCreationTime));

      if(has_ct)
      {
        time_t diff = (lastWrite-fileCreationTime);
        if(diff>1 || diff<-1)
        {
          LOG(LOG_INF) << in << ", fixing "
               << " new=" << tbuf2
               << "  ---->  old=" << tbuf
               << "\n";
          fs::last_write_time(fs::path(in), fileCreationTime);
        }
        else
        {
          LOG(LOG_DBG) << "Last write time already correct: " << in << "\n";
        }
      }
      else
      {
        LOG(LOG_INF) << "No FIT creation time: " << in << "\n";
      }
    }
    else
    {
#if BOOST_VERSION<=104300
      const std::string out(fitFiles[i].parent_path().string()+"//"+fitFiles[i].stem()+".gpx");
#else
      const std::string out(fitFiles[i].parent_path().string()+"//"+fitFiles[i].stem().string()+".gpx");
#endif
      LOG_VAR2(in, out);
      GPX gpx;
      FIT fit;
      vector<uchar> v(readFile(in.c_str()));
      if(fit.parse(v, gpx))
        gpx.writeToFile(out);
    }
  }


  return EXIT_SUCCESS;
}
