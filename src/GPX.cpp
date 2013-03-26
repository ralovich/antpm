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

#include "GPX.hpp"
#include <iostream>
#include <iomanip>
#include "common.hpp"

using namespace std;

namespace antpm{


WayPoint::WayPoint():
    time(0),
    latitude(INT32_MAX),
    longitude(INT32_MAX),
    altitude(UINT16_MAX)
{
}

WayPoint::~WayPoint()
{
}

void WayPoint::putToFile(ofstream &file)
{
    if ((latitude != INT32_MAX) && (longitude != INT32_MAX))
    {
        file << "  <wpt lat=\"" << setprecision(5) << GarminConvert::coord(latitude) << "\" lon=\"" << setprecision(5) << GarminConvert::coord(longitude) << "\">" << endl;
        file << "    <name>" << name << "</name>" << endl;
        if (altitude != UINT16_MAX)
        {
            file << "    <ele>" << setprecision(1) << GarminConvert::altitude(altitude) << "</ele>" << endl;
        }
        file << "    <time>" << GarminConvert::gmTime(time) << "</time>" << endl;
        file << "    <sym>city (small)</sym>" << endl;
        file << "  </wpt>" << endl;
    }
}

TrackPoint::TrackPoint()
{
    time = 0;
    latitude = INT32_MAX;
    longitude = INT32_MAX;
    altitude = UINT16_MAX;
    heartRate = UINT8_MAX;
    cadence = UINT8_MAX;

}

TrackPoint::~TrackPoint()
{
}

void TrackPoint::putToFile(ofstream &file)
{
    if ((latitude != INT32_MAX) && (longitude != INT32_MAX))
    {
        file << "    <trkpt lat=\"" << setprecision(5) << GarminConvert::coord(latitude) << "\" lon=\"" << setprecision(5) << GarminConvert::coord(longitude) << "\">" << endl;
        if (altitude != UINT16_MAX)
        {
            file << "      <ele>" << setprecision(1) << GarminConvert::altitude(altitude) << "</ele>" << endl;
        }
        file << "      <time>" << GarminConvert::gmTime(time) << "</time>" << endl;
        if ((heartRate != UINT8_MAX) || (cadence != UINT8_MAX))
        {
            file << "      <extensions>" << endl;
            file << "        <gpxtpx:TrackPointExtension>" << endl;
            if (heartRate != UINT8_MAX)
            {
                file << "          <gpxtpx:hr>" << dec << (unsigned)heartRate << "</gpxtpx:hr>" << endl;
            }
            if (cadence != UINT8_MAX)
            {
                file << "          <gpxtpx:cad>" << dec << (unsigned)cadence << "</gpxtpx:cad>" << endl;
            }
            file << "        </gpxtpx:TrackPointExtension>" << endl;
            file << "      </extensions>" << endl;
        }
        file << "    </trkpt>" << endl;
    }
}

TrackSeg::TrackSeg()
{
}

TrackSeg::~TrackSeg()
{
}

void TrackSeg::putToFile(ofstream &file)
{
    if (trackPoints.size())
    {
        file << "  <trkseg>" << endl;
        map<uint32_t,TrackPoint>::iterator it;
        for (it = trackPoints.begin(); it != trackPoints.end(); it++)
        {
            TrackPoint &trackPoint = it->second;
            trackPoint.putToFile(file);
        }
        file << "  </trkseg>" << endl;
    }
}


Track::Track(string &p_name) : name(p_name)
{
}

Track::~Track()
{
}

void Track::newTrackSeg()
{
    TrackSeg trackSeg;
    trackSegs.push_back(trackSeg);
}

void Track::putToFile(ofstream &file)
{
    file << "<trk>" << endl;
    file << "  <name>" << name << "</name>" << endl;

    for (size_t i=0; i<trackSegs.size(); i++)
    {
        trackSegs[i].putToFile(file);
    }

    file << "</trk>" << endl;
}

GPX::GPX()
{
}

GPX::~GPX()
{
}

void GPX::newTrack(string name)
{
    Track track(name);
    tracks.push_back(track);
    newTrackSeg();
}

void GPX::newTrackSeg()
{
    tracks.back().newTrackSeg();
}

void GPX::newWayPoint()
{
    WayPoint wayPoint;
    wayPoints.push_back(wayPoint);
}

bool GPX::writeToFile(string fileName)
{
    ofstream file(fileName.c_str());
    if (!file.is_open())
    {
        cerr << "Error writing to file '" << fileName << "'" << endl;
        return false;
    }

    file.setf(ios::fixed,ios::floatfield);

    file << "<?xml version=\"1.0\"?>" << endl;
    file << "<gpx version=\"1.1\" creator=\"" APP_NAME "\"" << endl;
    file << "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd "\
        "http://www.garmin.com/xmlschemas/GpxExtensions/v3 http://www.garmin.com/xmlschemas/GpxExtensionsv3.xsd "\
        "http://www.garmin.com/xmlschemas/TrackPointExtension/v1 http://www.garmin.com/xmlschemas/TrackPointExtensionv1.xsd\"" << endl;
    file << "xmlns=\"http://www.topografix.com/GPX/1/1\"" << endl;
    file << "xmlns:gpxtpx=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\"" << endl;
    file << "xmlns:gpxx=\"http://www.garmin.com/xmlschemas/GpxExtensions/v3\"" << endl;
    file << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">" << endl;

    for (size_t i=0; i<wayPoints.size(); i++)
    {
        wayPoints[i].putToFile(file);
    }

    for (size_t i=0; i<tracks.size(); i++)
    {
        tracks[i].putToFile(file);
    }

    file << "</gpx>" << endl;
    file.close();

    return true;
}

}
