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


namespace antpm{

class WayPoint
{
public:
    WayPoint();
    ~WayPoint();

    void putToFile(std::ofstream &file);

public:
    std::string name;
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

    void putToFile(std::ofstream &file);

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

    void putToFile(std::ofstream &file);

public:
    std::map<uint32_t,TrackPoint> trackPoints;
};

class Track
{
public:
    Track(std::string &name);
    ~Track();

    void newTrackSeg();
    void putToFile(std::ofstream &file);

public:
    std::string name;
    std::vector<TrackSeg> trackSegs;
};

class GPX
{
public:
    GPX();
    ~GPX();

    void newTrack(std::string name);
    void newTrackSeg();
    void newWayPoint();

    bool writeToFile(std::string fileName);

public:
    std::vector<WayPoint> wayPoints;
    std::vector<Track> tracks;
};

}
