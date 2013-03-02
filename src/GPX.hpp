// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
/***************************************************************************
 *   Copyright (C) 2010-2012 by Oleg Khudyakov                             *
 *   prcoder@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
// ***** BEGIN LICENSE BLOCK *****
////////////////////////////////////////////////////////////////////
// Copyright (c) 2012-2013 RALOVICH, Kristóf                      //
//                                                                //
// This program is free software; you can redistribute it and/or  //
// modify it under the terms of the GNU General Public License    //
// version 2 as published by the Free Software Foundation.        //
//                                                                //
////////////////////////////////////////////////////////////////////
// ***** END LICENSE BLOCK *****

#pragma once

#include "GarminConvert.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include "stdintfwd.hpp"

//#ifndef _MSC_VER
#define INT8_MAX 0x7F
#define UINT8_MAX 0xFF
#define INT16_MAX 0x7FFF
#define UINT16_MAX 0xFFFF
#define INT32_MAX 0x7FFFFFFF
#define UINT32_MAX 0xFFFFFFFF
//#endif

using namespace std;

class WayPoint
{
public:
    WayPoint();
    ~WayPoint();

    void putToFile(ofstream &file);

public:
    string name;
    uint32_t time;
    int32_t latitude;
    int32_t longitude;
    uint16_t altitude;
};

class TrackPoint
{
public:
    TrackPoint();
    ~TrackPoint();

    void putToFile(ofstream &file);

public:
    uint32_t time;
    int32_t latitude;
    int32_t longitude;
    uint16_t altitude;
    uint8_t heartRate;
    uint8_t cadence;
};

class TrackSeg
{
public:
    TrackSeg();
    ~TrackSeg();

    void putToFile(ofstream &file);

public:
    map<uint32_t,TrackPoint> trackPoints;
};

class Track
{
public:
    Track(string &name);
    ~Track();

    void newTrackSeg();
    void putToFile(ofstream &file);

public:
    string name;
    vector<TrackSeg> trackSegs;
};

class GPX
{
public:
    GPX();
    ~GPX();

    void newTrack(string name);
    void newTrackSeg();
    void newWayPoint();

    bool writeToFile(string fileName);

public:
    vector<WayPoint> wayPoints;
    vector<Track> tracks;
};

