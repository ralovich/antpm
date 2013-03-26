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
#include "FIT.hpp"
#include "common.hpp"
#include "Log.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using namespace std;


namespace antpm
{

template<>
Log*
ClassInstantiator<Log>::instantiate()
{
  return new Log(NULL);
}

}

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

  // Declare the supported options.
  std::string fitFolder;
  std::string fitRootFile;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("fitFolder,F", po::value<std::string>(&fitFolder), "Folder with FIT files")
    ("decode-fit-root,D", po::value<std::string>(&fitRootFile), "FIT file, encoding the root directory contents on a device")
    ;

  std::vector<const char*> args(argv, argv+argc);
  po::variables_map vm;
  try
  {
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

  if(vm.count("help")
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

  for (size_t i = 0; i < fitFiles.size(); i++)
  {
    const std::string in(fitFiles[i].string());
#if BOOST_VERSION==104300
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
  


  return EXIT_SUCCESS;
}
