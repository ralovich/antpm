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

#include "GarminConvert.hpp"
#include "antdefs.hpp"
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctype.h>

using namespace std;

namespace antpm{


double GarminConvert::coord(int32_t coord)
{
    double rv = coord;
    rv *= 180;
    rv /= 0x80000000;

    return rv;
}

double GarminConvert::altitude(uint16_t alt)
{
    double rv = alt;
    rv /= 5;
    rv -= 500;

    return rv;
}

double GarminConvert::length(uint32_t centimeters)
{
    return (double)centimeters / 100;
}

double GarminConvert::speed(uint16_t speed)
{
    return ((double)speed * 0.0036);
}

double GarminConvert::weight(uint16_t weight)
{
    return ((double)weight / 10);
}

string GarminConvert::gmTime(const uint32_t time)
{
    time_t t = time;
    t += GARMIN_EPOCH; // Garmin epoch offset
    char tbuf[256];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&t));

    return tbuf;
}

string GarminConvert::localTime(const uint32_t time)
{
    time_t t = time;
    t += GARMIN_EPOCH; // Garmin epoch offset
    char tbuf[256];
    strftime(tbuf, sizeof(tbuf), "%d-%m-%Y %H:%M:%S", localtime(&t));

    return tbuf;
}


/// returns in GMT/UTC timestamp
uint32_t
GarminConvert::gOffsetTime(const uint32_t time)
{
  return time + GARMIN_EPOCH; // Garmin epoch offset
}


string GarminConvert::gTime(uint32_t time)
{
    unsigned thousandths = time % 1000;
    time /= 1000;
    unsigned seconds = time % 60;
    time /= 60;
    unsigned minutes = time % 60;
    time /= 60;
    unsigned hours = time;

    ostringstream sstr;
    sstr << dec << setw(2) << setfill('0');
    sstr << hours << ":" << minutes << ":" << seconds << "." << setw(3) << thousandths;

    return sstr.str();
}

string GarminConvert::gString(uint8_t *str, int maxSize)
{
    string rv;
    for(int i=0; i<maxSize; i++)
    {
        if (str[i])
        {
            rv += str[i];
        }
        else
        {
            break;
        }
    }

    return rv;
}

string GarminConvert::gHex(uint8_t *buf, size_t size)
{
    ostringstream sstr;
    sstr << uppercase << setw(2) << setfill('0');

    for(size_t i=0; i<size; i++)
    {
        sstr << hex << setw(2) << (unsigned)buf[i];
        if (i < size-1)
        {
            sstr << " ";
        }
    }

    return sstr.str();
}

string GarminConvert::gHex(vector<uint8_t> &buf)
{
    return gHex(&buf[0], buf.size());
}

string GarminConvert::hexDump(vector<uint8_t> &buf)
{
    ostringstream sstr;

    sstr << gHex(buf) << " ";

    for(size_t i=0; i<buf.size(); i++)
    {
        sstr << (char)(isprint(buf[i])?buf[i]:'.');
    }

    return sstr.str();
}

}
