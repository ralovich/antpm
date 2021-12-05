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

#include "FIT.hpp"
#include <time.h>
#include <string.h>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <iostream> // DEBUG

#include "common.hpp"

using namespace std;

namespace antpm{


bool isZero(const uint32_t i) { return i==0; }


std::time_t
ZeroFileContent::getFitFileTime(const uint16_t idx)
{
  for(size_t i = 0; i < zfRecords.size(); i++)
  {
    ZeroFileRecord& zfRecord(zfRecords[i]);
    if(zfRecord.index == idx)
      return zfRecord.timeStamp;
  }
  return 0;
}

// \param files list of files already downloaded to the host and discovered in ~/.config/antpm/.../
void ZeroFileContent::cullFitFiles(const std::multimap<uint16_t, std::pair<string, size_t> >& db_files)
{
  LOG_VAR(db_files.size());
  for(auto it : db_files)
  {
    cullFitFile(it.first, it.second.second);
  }
}

void ZeroFileContent::cullFitFilesDate()
{
  // sort file in ZFC based on date (for now activity only)
  // detect file dates with fixed date offsets in between
  // only download files with later date

  unsigned threshold = 20;

  std::multimap<uint32_t, uint16_t> date_sorted_activities;

  for(size_t i = 0; i < zfRecords.size(); i++)
  {
    ZeroFileRecord& zfRecord(zfRecords[i]);

    if(zfRecord.recordType != 4) continue; // skip if not activity

//    if(date_sorted_activities.count(zfRecord.timeStamp))
//    {
//      // problem: record exists with this date
//      logger() << "problem: record exists with this date\n";
//    }
    date_sorted_activities.insert(std::make_pair(zfRecord.timeStamp, zfRecord.index));
  }

  auto remover = [&](uint16_t file_idx)
  {
    auto it = date_sorted_activities.begin();
    for(; it != date_sorted_activities.end(); it++)
    {
      if(it->second != file_idx) continue;

      uint32_t relative_age = it==date_sorted_activities.begin() ? 10000 : (it->first - std::prev(it)->first);
      //logger() << "0x" << toString<uint16_t>(it->first,4,'0') << ": relative_age=" << relative_age << "\n";
      return relative_age <= threshold;
    }
    return false;
  };

  activityFiles.erase(std::remove_if(activityFiles.begin(), activityFiles.end(),
                                     remover), activityFiles.end());
}

void ZeroFileContent::cullFitFile(const uint16_t idx, const size_t file_size_bytes)
{
  //logger() << "0x" << toString<ushort>(idx,4,'0') << " " << file_size_bytes << " bytes\n";
  auto remover = [this,idx,file_size_bytes](uint16_t file_idx)
    {
      ZeroFileRecord& zfRecord(zfRecords[file_idx]);

      uint16_t index = zfRecord.index;
      
      //if(index == idx)
      //{
        //logger() << "0x" << toString<uint16_t>(idx,4,'0') << " 0x" << toString<uint16_t>(index,4,'0')
        //         << " " << zfRecord.fileSize << " " << file_size_bytes << " bytes"  << "\n";
        if(file_size_bytes == zfRecord.fileSize)
        {
          //logger() << "CULL " << toString<uint16_t>(idx,4,'0') << "\n";
          return true;
        }
      //}
      //logger() << "KEEP\n";
      return false;
    };
  
  size_t nw = waypointsFiles.size();
  size_t na = activityFiles.size();
  size_t nc = courseFiles.size();

  waypointsFiles.erase(std::remove_if(waypointsFiles.begin(), waypointsFiles.end(),
                                      remover), waypointsFiles.end());
  activityFiles.erase(std::remove_if(activityFiles.begin(), activityFiles.end(),
                                     remover), activityFiles.end());
  courseFiles.erase(std::remove_if(courseFiles.begin(), courseFiles.end(),
                                   remover), courseFiles.end());
  //logger() << "w:" << nw << "->" << waypointsFiles.size() << ", a:" << na << "->" << activityFiles.size()
  //         << ", c:" << nc << "->" << courseFiles.size() << "\n";
}


FIT::FIT()
{
  messageTypeMap[0] = "File Id";
  messageFieldNameMap[0][0] = "Type";
  messageFieldTypeMap[0][0] = MessageFieldTypeFileType;
  messageFieldNameMap[0][1] = "Manufacturer";
  messageFieldTypeMap[0][1] = MessageFieldTypeManufacturer;
  messageFieldNameMap[0][2] = "Product";
  messageFieldTypeMap[0][2] = MessageFieldTypeProduct;
  messageFieldNameMap[0][3] = "Serial Number";
  messageFieldNameMap[0][4] = "Creation Time";
  messageFieldTypeMap[0][4] = MessageFieldTypeTimestamp;
  messageFieldNameMap[0][5] = "Number";

  messageTypeMap[1] = "Capabilities";
  messageFieldNameMap[1][21] = "Workout Supported";

  messageTypeMap[2] = "Device Settings";
  messageFieldNameMap[2][1] = "UTC Offset";

  messageTypeMap[3] = "User Profile";
  messageFieldNameMap[3][254] = "Index";
  messageFieldNameMap[3][0] = "Name";
  messageFieldNameMap[3][1] = "Gender";
  messageFieldTypeMap[3][1] = MessageFieldTypeGender;
  messageFieldNameMap[3][2] = "Age";
  messageFieldNameMap[3][3] = "Height";
  messageFieldNameMap[3][4] = "Weight";
  messageFieldTypeMap[3][4] = MessageFieldTypeWeight;
  messageFieldNameMap[3][5] = "Language";
  messageFieldTypeMap[3][5] = MessageFieldTypeLanguage;
  messageFieldNameMap[3][6] = "Elevation Units";
  messageFieldNameMap[3][7] = "Weight Units";
  messageFieldNameMap[3][8] = "HR Resting";
  messageFieldNameMap[3][9] = "HR Running Max";
  messageFieldNameMap[3][10] = "HR Biking Max";
  messageFieldNameMap[3][11] = "HR Max";
  messageFieldNameMap[3][12] = "HR Setting";
  messageFieldNameMap[3][13] = "Speed Setting";
  messageFieldNameMap[3][14] = "Dist Setting";
  messageFieldNameMap[3][16] = "Power Setting";
  messageFieldNameMap[3][17] = "Activity Class";
  messageFieldNameMap[3][18] = "Position Setting";

  messageTypeMap[4] = "HRM Profile";
  messageFieldNameMap[4][254] = "Index";
  messageFieldNameMap[4][0] = "Enabled";
  messageFieldNameMap[4][1] = "HRM ANT Id";

  messageTypeMap[5] = "SDM Profile";

  messageTypeMap[6] = "Bike Profile";
  messageFieldNameMap[6][254] = "Index";
  messageFieldNameMap[6][0] = "Name";
  messageFieldNameMap[6][1] = "Sport";
  messageFieldNameMap[6][2] = "SubSport";
  messageFieldNameMap[6][3] = "Odometer";
  messageFieldTypeMap[6][3] = MessageFieldTypeOdometr;
  messageFieldNameMap[6][4] = "Bike Spd ANT Id";
  messageFieldNameMap[6][5] = "Bike Cad ANT Id";
  messageFieldNameMap[6][6] = "Bike Spd/Cad ANT Id";
  messageFieldNameMap[6][7] = "Bike Power ANT Id";
  messageFieldNameMap[6][8] = "Custom Wheel Size";
  messageFieldNameMap[6][9] = "Auto Wheel Size";
  messageFieldNameMap[6][10] = "Bike Weight";
  messageFieldTypeMap[6][10] = MessageFieldTypeWeight;
  messageFieldNameMap[6][11] = "Power Calibration Factor";
  messageFieldNameMap[6][12] = "Auto Wheel Calibration";
  messageFieldNameMap[6][13] = "Auto Power Zero";
  messageFieldNameMap[6][14] = "Id";
  messageFieldNameMap[6][15] = "Spd Enabled";
  messageFieldNameMap[6][16] = "Cad Enabled";
  messageFieldNameMap[6][17] = "Spd/Cad Enabled";
  messageFieldNameMap[6][18] = "Power Enabled";
  messageFieldNameMap[6][19] = "Crank Length";
  messageFieldNameMap[6][20] = "Enabled";
  messageFieldNameMap[6][21] = "Bike Spd ANT Id Trans Type";
  messageFieldNameMap[6][22] = "Bike Cad ANT Id Trans Type";
  messageFieldNameMap[6][23] = "Bike Spd/Cad ANT Id Trans Type";
  messageFieldNameMap[6][24] = "Bike Power ANT Id Trans Type";

  messageTypeMap[7] = "Zones Target";
  messageFieldNameMap[7][1] = "Max Heart Rate";
  messageFieldNameMap[7][2] = "Threshold Heart Rate";
  messageFieldNameMap[7][3] = "Functional Threshold Power";
  messageFieldNameMap[7][5] = "HR Calc Type";
  messageFieldNameMap[7][6] = "PWR Calc Type";
    
  messageTypeMap[8] = "Heart Rate Zone";
  messageFieldNameMap[8][254] = "Index";
  messageFieldNameMap[8][1] = "High BPM";
  messageFieldNameMap[8][2] = "Name";

  messageTypeMap[9] = "Power Zone";
  messageFieldNameMap[9][254] = "Index";
  messageFieldNameMap[9][1] = "High Value";
  messageFieldNameMap[9][2] = "Name";

  messageTypeMap[10] = "Met Zone";
  messageFieldNameMap[10][254] = "Index";
  messageFieldNameMap[10][1] = "High BPM";
  messageFieldNameMap[10][2] = "Calories";
  messageFieldNameMap[10][3] = "Fat Calories";

  messageTypeMap[12] = "Sport";
  messageFieldNameMap[12][0] = "Sport";
  messageFieldTypeMap[12][0] = MessageFieldTypeSport;
  messageFieldNameMap[12][1] = "SubSport";
  messageFieldNameMap[12][2] = "Name";

  messageTypeMap[15] = "Traning Goals";

  messageTypeMap[18] = "Session";
  messageFieldNameMap[18][254] = "Index";
  messageFieldNameMap[18][253] = "Timestamp";
  messageFieldNameMap[18][0] = "Event";
  messageFieldTypeMap[18][0] = MessageFieldTypeEvent;
  messageFieldNameMap[18][1] = "Event Type";
  messageFieldTypeMap[18][1] = MessageFieldTypeEventType;
  messageFieldNameMap[18][2] = "Start Time";
  messageFieldTypeMap[18][2] = MessageFieldTypeTimestamp;
  messageFieldNameMap[18][3] = "Start Position Latitude";
  messageFieldTypeMap[18][3] = MessageFieldTypeCoord;
  messageFieldNameMap[18][4] = "Start Position Longitude";
  messageFieldTypeMap[18][4] = MessageFieldTypeCoord;
  messageFieldNameMap[18][5] = "Sport";
  messageFieldTypeMap[18][5] = MessageFieldTypeSport;
  messageFieldNameMap[18][6] = "SubSport";
  messageFieldNameMap[18][7] = "Total Elapsed Time";
  messageFieldTypeMap[18][7] = MessageFieldTypeTime;
  messageFieldNameMap[18][8] = "Total Timer Time";
  messageFieldTypeMap[18][8] = MessageFieldTypeTime;
  messageFieldNameMap[18][9] = "Total Distance";
  messageFieldTypeMap[18][9] = MessageFieldTypeOdometr;
  messageFieldNameMap[18][10] = "Total Cycles";
  messageFieldNameMap[18][11] = "Total Calories";
  messageFieldNameMap[18][13] = "Total Fat Calories";
  messageFieldNameMap[18][14] = "Average Speed";
  messageFieldTypeMap[18][14] = MessageFieldTypeSpeed;
  messageFieldNameMap[18][15] = "Max Speed";
  messageFieldTypeMap[18][15] = MessageFieldTypeSpeed;
  messageFieldNameMap[18][16] = "Average Heart Rate";
  messageFieldNameMap[18][17] = "Max Heart Rate";
  messageFieldNameMap[18][18] = "Average Cadence";
  messageFieldNameMap[18][19] = "Max Cadence";
  messageFieldNameMap[18][20] = "Average Power";
  messageFieldNameMap[18][21] = "Max Power";
  messageFieldNameMap[18][22] = "Total Ascent";
  messageFieldNameMap[18][23] = "Total Descent";
  messageFieldNameMap[18][24] = "Total Traning Effect";
  messageFieldNameMap[18][25] = "First Lap Index";
  messageFieldNameMap[18][26] = "Num Laps";
  messageFieldNameMap[18][27] = "Event Group";
  messageFieldNameMap[18][28] = "Trigger";
  messageFieldNameMap[18][29] = "NEC Latitude";
  messageFieldTypeMap[18][29] = MessageFieldTypeCoord;
  messageFieldNameMap[18][30] = "NEC Longitude";
  messageFieldTypeMap[18][30] = MessageFieldTypeCoord;
  messageFieldNameMap[18][31] = "SWC Latitude";
  messageFieldTypeMap[18][31] = MessageFieldTypeCoord;
  messageFieldNameMap[18][32] = "SWC Longitude";
  messageFieldTypeMap[18][32] = MessageFieldTypeCoord;
  messageFieldNameMap[18][43] = "Swimming Stroke";
  messageFieldTypeMap[18][43] = MessageFieldTypeSwimStroke;
  messageFieldNameMap[18][44] = "Pool Length";
  messageFieldNameMap[18][46] = "Pool Length Unit";
  messageFieldTypeMap[18][46] = MessageFieldTypePoolLengthUnit;

  messageTypeMap[19] = "Lap";
  messageFieldNameMap[19][254] = "Index";
  messageFieldNameMap[19][253] = "Timestamp";
  messageFieldNameMap[19][0] = "Event";
  messageFieldTypeMap[19][0] = MessageFieldTypeEvent;
  messageFieldNameMap[19][1] = "Event Type";
  messageFieldTypeMap[19][1] = MessageFieldTypeEventType;
  messageFieldNameMap[19][2] = "Start Time";
  messageFieldTypeMap[19][2] = MessageFieldTypeTimestamp;
  messageFieldNameMap[19][3] = "Start Position Latitude";
  messageFieldTypeMap[19][3] = MessageFieldTypeCoord;
  messageFieldNameMap[19][4] = "Start Position Longitude";
  messageFieldTypeMap[19][4] = MessageFieldTypeCoord;
  messageFieldNameMap[19][5] = "End Position Latitude";
  messageFieldTypeMap[19][5] = MessageFieldTypeCoord;
  messageFieldNameMap[19][6] = "End Position Longitude";
  messageFieldTypeMap[19][6] = MessageFieldTypeCoord;
  messageFieldNameMap[19][7] = "Total Elapsed Time";
  messageFieldTypeMap[19][7] = MessageFieldTypeTime;
  messageFieldNameMap[19][8] = "Total Timer Time";
  messageFieldTypeMap[19][8] = MessageFieldTypeTime;
  messageFieldNameMap[19][9] = "Total Distance";
  messageFieldTypeMap[19][9] = MessageFieldTypeOdometr;
  messageFieldNameMap[19][10] = "Total Cycles";
  messageFieldNameMap[19][11] = "Total Calories";
  messageFieldNameMap[19][12] = "Total Fat Calories";
  messageFieldNameMap[19][13] = "Average Speed";
  messageFieldTypeMap[19][13] = MessageFieldTypeSpeed;
  messageFieldNameMap[19][14] = "Max Speed";
  messageFieldTypeMap[19][14] = MessageFieldTypeSpeed;
  messageFieldNameMap[19][15] = "Average Heart Rate";
  messageFieldNameMap[19][16] = "Max Heart Rate";
  messageFieldNameMap[19][17] = "Average Cadence";
  messageFieldNameMap[19][18] = "Max Cadence";
  messageFieldNameMap[19][19] = "Average Power";
  messageFieldNameMap[19][20] = "Max Power";
  messageFieldNameMap[19][21] = "Total Ascent";
  messageFieldNameMap[19][22] = "Total Descent";
  messageFieldNameMap[19][23] = "Intensity";
  messageFieldNameMap[19][24] = "Lap Trigger";
  messageFieldNameMap[19][25] = "Sport";
  messageFieldTypeMap[19][25] = MessageFieldTypeSport;
  messageFieldNameMap[19][26] = "Event Group";
  messageFieldNameMap[19][27] = "Nec Latitude";
  messageFieldTypeMap[19][27] = MessageFieldTypeCoord;
  messageFieldNameMap[19][28] = "Nec Longitude";
  messageFieldTypeMap[19][28] = MessageFieldTypeCoord;
  messageFieldNameMap[19][29] = "Swc Latitude";
  messageFieldTypeMap[19][29] = MessageFieldTypeCoord;
  messageFieldNameMap[19][30] = "Swc Longitude";
  messageFieldTypeMap[19][30] = MessageFieldTypeCoord;

  messageTypeMap[20] = "Record";
  messageFieldNameMap[20][253] = "Timestamp";
  messageFieldNameMap[20][0] = "Latitude";
  messageFieldTypeMap[20][0] = MessageFieldTypeCoord;
  messageFieldNameMap[20][1] = "Longitude";
  messageFieldTypeMap[20][1] = MessageFieldTypeCoord;
  messageFieldNameMap[20][2] = "Altitude";
  messageFieldTypeMap[20][2] = MessageFieldTypeAltitude;
  messageFieldNameMap[20][3] = "Heart Rate";
  messageFieldNameMap[20][4] = "Cadence";
  messageFieldNameMap[20][5] = "Distance";
  messageFieldTypeMap[20][5] = MessageFieldTypeOdometr;
  messageFieldNameMap[20][6] = "Speed";
  messageFieldTypeMap[20][6] = MessageFieldTypeSpeed;
  messageFieldNameMap[20][7] = "Power";
  messageFieldNameMap[20][8] = "Compressed Speed & Distance";
  messageFieldNameMap[20][9] = "Grade";
  messageFieldNameMap[20][10] = "Registance";
  messageFieldNameMap[20][11] = "Time from Course";
  messageFieldTypeMap[20][11] = MessageFieldTypeTime;
  messageFieldNameMap[20][12] = "Cycle Length";
  messageFieldNameMap[20][13] = "Temperature";
  messageFieldNameMap[20][14] = "Speed 1s";
  messageFieldNameMap[20][15] = "Cycles";
  messageFieldNameMap[20][16] = "Total Cycles";
  messageFieldNameMap[20][17] = "Compressed Accumulated Power";
  messageFieldNameMap[20][18] = "Accumulated Power";
  messageFieldNameMap[20][19] = "Left-Right Balance";

  messageTypeMap[21] = "Event";
  messageFieldNameMap[21][253] = "Timestamp";
  messageFieldNameMap[21][0] = "Event";
  messageFieldTypeMap[21][0] = MessageFieldTypeEvent;
  messageFieldNameMap[21][1] = "Event Type";
  messageFieldTypeMap[21][1] = MessageFieldTypeEventType;
  messageFieldNameMap[21][2] = "Data1";
  messageFieldNameMap[21][3] = "Data2";
  messageFieldNameMap[21][4] = "Event Group";

  messageTypeMap[23] = "Device Info";
  messageFieldNameMap[23][253] = "Timestamp";
  messageFieldNameMap[23][0] = "Device Index";
  messageFieldNameMap[23][1] = "Device Type";
  messageFieldNameMap[23][2] = "Manufacturer";
  messageFieldTypeMap[23][2] = MessageFieldTypeManufacturer;
  messageFieldNameMap[23][3] = "Serial Number";
  messageFieldNameMap[23][4] = "Product";
  messageFieldTypeMap[23][4] = MessageFieldTypeProduct;
  messageFieldNameMap[23][5] = "Software Version";
  messageFieldNameMap[23][6] = "Hardware Version";
  messageFieldNameMap[23][10] = "Battery Voltage";
  messageFieldNameMap[23][11] = "Battery Status";

  messageTypeMap[26] = "Workout";
  messageFieldNameMap[26][4] = "Sport";
  messageFieldTypeMap[26][4] = MessageFieldTypeSport;
  messageFieldNameMap[26][5] = "Capabilities";
  messageFieldNameMap[26][6] = "Valid Steps";
  messageFieldNameMap[26][7] = "Protection";
  messageFieldNameMap[26][8] = "Name";

  messageTypeMap[27] = "Workout Step";
  messageFieldNameMap[27][254] = "Index";
  messageFieldNameMap[27][0] = "Step Name";
  messageFieldNameMap[27][1] = "Duration Type";
  messageFieldNameMap[27][2] = "Duration Value";
  messageFieldNameMap[27][3] = "Target Type";
  messageFieldNameMap[27][4] = "Target Value";
  messageFieldNameMap[27][5] = "Custom Target Value Low";
  messageFieldNameMap[27][6] = "Custom Target Value High";
  messageFieldNameMap[27][7] = "Intensity";

  messageTypeMap[28] = "Schedule";
  messageFieldNameMap[28][0] = "Manufacturer";
  messageFieldTypeMap[28][0] = MessageFieldTypeManufacturer;
  messageFieldNameMap[28][1] = "Product";
  messageFieldTypeMap[28][1] = MessageFieldTypeProduct;
  messageFieldNameMap[28][2] = "Serial Number";
  messageFieldNameMap[28][3] = "Creation Time";
  messageFieldTypeMap[28][3] = MessageFieldTypeTimestamp;
  messageFieldNameMap[28][4] = "Completed";
  messageFieldNameMap[28][5] = "Type";
  messageFieldNameMap[28][6] = "Schedule Time";
  messageFieldTypeMap[28][6] = MessageFieldTypeTime;

  messageTypeMap[29] = "Way Point";
  messageFieldNameMap[29][253] = "Timestamp";
  messageFieldNameMap[29][254] = "Index";
  messageFieldNameMap[29][0] = "Name";
  messageFieldNameMap[29][1] = "Latitude";
  messageFieldTypeMap[29][1] = MessageFieldTypeCoord;
  messageFieldNameMap[29][2] = "Longitude";
  messageFieldTypeMap[29][2] = MessageFieldTypeCoord;
  messageFieldNameMap[29][3] = "Symbol?";
  messageFieldNameMap[29][4] = "Altitude";
  messageFieldTypeMap[29][4] = MessageFieldTypeAltitude;
  messageFieldNameMap[29][5] = "???";
  messageFieldNameMap[29][6] = "Date";

  messageTypeMap[30] = "Weight Scale";
  messageFieldNameMap[30][253] = "Timestamp";
  messageFieldNameMap[30][0] = "Weight";
  messageFieldNameMap[30][1] = "Fat percent";
  messageFieldNameMap[30][2] = "Hydration percent";
  messageFieldNameMap[30][3] = "Visceral Fat Mass";
  messageFieldNameMap[30][4] = "Bone Mass";
  messageFieldNameMap[30][5] = "Muscle Mass";
  messageFieldNameMap[30][7] = "Basal Met";
  messageFieldNameMap[30][8] = "Physique Rating";
  messageFieldNameMap[30][9] = "Active Met";
  messageFieldNameMap[30][10] = "Metabolic Age";
  messageFieldNameMap[30][11] = "Visceral Fat Rating";

  messageTypeMap[31] = "Course";
  messageFieldNameMap[31][4] = "Sport";
  messageFieldTypeMap[31][4] = MessageFieldTypeSport;
  messageFieldNameMap[31][5] = "Name";
  messageFieldNameMap[31][6] = "Capabilities";

  messageTypeMap[32] = "Course Point";
  messageFieldNameMap[32][254] = "Index";
  messageFieldNameMap[32][1] = "Time";
  messageFieldTypeMap[32][1] = MessageFieldTypeTimestamp;
  messageFieldNameMap[32][2] = "Latitude";
  messageFieldTypeMap[32][2] = MessageFieldTypeCoord;
  messageFieldNameMap[32][3] = "Longitude";
  messageFieldTypeMap[32][3] = MessageFieldTypeCoord;
  messageFieldNameMap[32][4] = "Distance";
  messageFieldTypeMap[32][4] = MessageFieldTypeOdometr;
  messageFieldNameMap[32][5] = "Type";
  messageFieldNameMap[32][6] = "Name";

  messageTypeMap[33] = "Totals";
  messageFieldNameMap[33][254] = "Index";
  messageFieldNameMap[33][253] = "Timestamp";
  messageFieldNameMap[33][0] = "Timer Time";
  messageFieldTypeMap[33][0] = MessageFieldTypeTime;
  messageFieldNameMap[33][1] = "Distance";
  messageFieldTypeMap[33][1] = MessageFieldTypeOdometr;
  messageFieldNameMap[33][2] = "Calories";
  messageFieldNameMap[33][3] = "Sport";
  messageFieldTypeMap[33][3] = MessageFieldTypeSport;

  messageTypeMap[34] = "Activity";
  messageFieldNameMap[34][253] = "Timestamp";
  messageFieldNameMap[34][0] = "Total Timer Time";
  messageFieldTypeMap[34][0] = MessageFieldTypeTime;
  messageFieldNameMap[34][1] = "Number of Sessions";
  messageFieldNameMap[34][2] = "Type";
  messageFieldNameMap[34][3] = "Event";
  messageFieldTypeMap[34][3] = MessageFieldTypeEvent;
  messageFieldNameMap[34][4] = "Event Type";
  messageFieldTypeMap[34][4] = MessageFieldTypeEventType;
  messageFieldNameMap[34][5] = "Local Timestamp";
  messageFieldTypeMap[34][5] = MessageFieldTypeTimestamp;
  messageFieldNameMap[34][6] = "Event Group";

  messageTypeMap[35] = "Software";
  messageFieldNameMap[35][254] = "Index";
  messageFieldNameMap[35][3] = "Version";
  messageFieldNameMap[35][5] = "Part No";

  messageTypeMap[37] = "File Capabilities";
  messageFieldNameMap[37][254] = "Index";
  messageFieldNameMap[37][0] = "Type";
  messageFieldNameMap[37][1] = "Flags";
  messageFieldNameMap[37][2] = "Directory";
  messageFieldNameMap[37][3] = "Max Count";
  messageFieldNameMap[37][4] = "Max Size";

  messageTypeMap[38] = "Message Capabilities";
  messageFieldNameMap[38][254] = "Index";
  messageFieldNameMap[38][0] = "File";
  messageFieldNameMap[38][1] = "Message Num";
  messageFieldNameMap[38][2] = "Count Type";
  messageFieldNameMap[38][3] = "Count";

  messageTypeMap[39] = "Field Capabilities";
  messageFieldNameMap[39][254] = "Index";
  messageFieldNameMap[39][0] = "File";
  messageFieldNameMap[39][1] = "Message Num";
  messageFieldNameMap[39][2] = "Field Num";
  messageFieldNameMap[39][3] = "Count";

  messageTypeMap[49] = "File Creator";
  messageFieldNameMap[49][0] = "Software Version";
  messageFieldNameMap[49][1] = "Hardware Version";

  messageTypeMap[51] = "Blood Pressure";

  messageTypeMap[53] = "Speed Zone";
  messageFieldNameMap[53][254] = "Index";
  messageFieldNameMap[53][1] = "High Value";
  messageFieldNameMap[53][2] = "Name";

  messageTypeMap[55] = "Monitoring";
  messageTypeMap[78] = "HRV";
    
  messageTypeMap[79] = "User Profile ?";
  messageFieldNameMap[79][254] = "Index";
  messageFieldNameMap[79][253] = "Timestamp";
  messageFieldNameMap[79][1] = "Age";
  messageFieldNameMap[79][2] = "Height";
  messageFieldNameMap[79][3] = "Weight";
  messageFieldTypeMap[79][3] = MessageFieldTypeWeight;
    
  messageTypeMap[101] = "Length";
  messageFieldNameMap[101][254] = "Index";
  messageFieldNameMap[101][253] = "Timestamp";
  messageFieldTypeMap[101][253] = MessageFieldTypeTimestamp;
  messageFieldNameMap[101][0] = "Event";
  messageFieldTypeMap[101][0] = MessageFieldTypeEvent;
  messageFieldNameMap[101][1] = "Event Type";
  messageFieldTypeMap[101][0] = MessageFieldTypeEventType;
  messageFieldNameMap[101][2] = "Start Time";
  messageFieldTypeMap[101][2] = MessageFieldTypeTimestamp;
  messageFieldNameMap[101][3] = "Total Elapsed Time";
  messageFieldTypeMap[101][3] = MessageFieldTypeTime;
  messageFieldNameMap[101][4] = "Total Timer Time";
  messageFieldTypeMap[101][4] = MessageFieldTypeTime;
  messageFieldNameMap[101][5] = "Total Strokes";
  messageFieldNameMap[101][6] = "Average Speed"; // unit: m/s * 1000
  messageFieldNameMap[101][7] = "Swimming Stroke";
  messageFieldTypeMap[101][7] = MessageFieldTypeSwimStroke;
  messageFieldNameMap[101][9] = "Average Swimming Cadence";
  messageFieldNameMap[101][12] = "Length Type";
  messageFieldTypeMap[101][12] = MessageFieldTypeLengthType;
  messageFieldNameMap[101][13] = "";
    
    
  messageTypeMap[103] = "Monitoring Info";
  messageTypeMap[105] = "PAD";

    
  dataTypeMap[BT_Enum] = "enum";
  dataTypeMap[BT_Int8] = "int8";
  dataTypeMap[BT_UInt8] = "uint8";
  dataTypeMap[BT_Int16] = "int16";
  dataTypeMap[BT_Uint16] = "uint16";
  dataTypeMap[BT_Int32] = "int32";
  dataTypeMap[BT_UInt32] = "uint32";
  dataTypeMap[BT_String] = "string";
  dataTypeMap[BT_Float32] = "float32";
  dataTypeMap[BT_Float64] = "float64";
  dataTypeMap[BT_Uint8z] = "uint8z";
  dataTypeMap[BT_Uint16z] = "uint16z";
  dataTypeMap[BT_Uint32z] = "uint32z";
  dataTypeMap[BT_ByteArray] = "bytes";

  enumMap[MessageFieldTypeFileType][1] = "Device";
  enumMap[MessageFieldTypeFileType][2] = "Setting";
  enumMap[MessageFieldTypeFileType][3] = "Sport";
  enumMap[MessageFieldTypeFileType][4] = "Activity";
  enumMap[MessageFieldTypeFileType][5] = "Workout";
  enumMap[MessageFieldTypeFileType][6] = "Course";
  enumMap[MessageFieldTypeFileType][7] = "Schedules";
  enumMap[MessageFieldTypeFileType][8] = "Waypoints";
  enumMap[MessageFieldTypeFileType][9] = "Weight";
  enumMap[MessageFieldTypeFileType][10] = "Totals";
  enumMap[MessageFieldTypeFileType][11] = "Goals";
  enumMap[MessageFieldTypeFileType][14] = "Blood Pressure";
  enumMap[MessageFieldTypeFileType][15] = "Monitoring";
  enumMap[MessageFieldTypeFileType][20] = "Activity Summary";
  enumMap[MessageFieldTypeFileType][28] = "Monitoring Daily";
  enumMap[MessageFieldTypeFileType][32] = "Memory";

  enumMap[MessageFieldTypeGender][0] = "Female";
  enumMap[MessageFieldTypeGender][1] = "Male";

  enumMap[MessageFieldTypeLanguage][0] = "English";
  enumMap[MessageFieldTypeLanguage][1] = "French";
  enumMap[MessageFieldTypeLanguage][2] = "Italian";
  enumMap[MessageFieldTypeLanguage][3] = "German";
  enumMap[MessageFieldTypeLanguage][4] = "Spanish";
  enumMap[MessageFieldTypeLanguage][5] = "Croatian";
  enumMap[MessageFieldTypeLanguage][6] = "Czech";
  enumMap[MessageFieldTypeLanguage][7] = "Danish";
  enumMap[MessageFieldTypeLanguage][8] = "Dutch";
  enumMap[MessageFieldTypeLanguage][9] = "Finnish";
  enumMap[MessageFieldTypeLanguage][10] = "Greek";
  enumMap[MessageFieldTypeLanguage][11] = "Hungarian";
  enumMap[MessageFieldTypeLanguage][12] = "Norwegian";
  enumMap[MessageFieldTypeLanguage][13] = "Polish";
  enumMap[MessageFieldTypeLanguage][14] = "Portuguese";
  enumMap[MessageFieldTypeLanguage][15] = "Slovakian";
  enumMap[MessageFieldTypeLanguage][16] = "Slovenian";
  enumMap[MessageFieldTypeLanguage][17] = "Swedish";
  enumMap[MessageFieldTypeLanguage][18] = "Russian";
  enumMap[MessageFieldTypeLanguage][19] = "Turkish";
  enumMap[MessageFieldTypeLanguage][20] = "Latvian";
  enumMap[MessageFieldTypeLanguage][21] = "Ukrainian";
  enumMap[MessageFieldTypeLanguage][22] = "Arabic";
  enumMap[MessageFieldTypeLanguage][23] = "Farsi";
  enumMap[MessageFieldTypeLanguage][24] = "Bulgarian";
  enumMap[MessageFieldTypeLanguage][25] = "Romanian";
  enumMap[MessageFieldTypeLanguage][254] = "Custom";

  enumMap[MessageFieldTypeSport][0] = "Generic";
  enumMap[MessageFieldTypeSport][1] = "Running";
  enumMap[MessageFieldTypeSport][2] = "Cycling";
  enumMap[MessageFieldTypeSport][3] = "Transition";
  enumMap[MessageFieldTypeSport][4] = "Fitness Equipment";
  enumMap[MessageFieldTypeSport][5] = "Swimming";
  enumMap[MessageFieldTypeSport][6] = "Basketball";
  enumMap[MessageFieldTypeSport][7] = "Soccer";
  enumMap[MessageFieldTypeSport][8] = "Tennis";
  enumMap[MessageFieldTypeSport][9] = "American football";
  enumMap[MessageFieldTypeSport][10] = "Training";
  enumMap[MessageFieldTypeSport][254] = "All";

  enumMap[MessageFieldTypeEvent][0] = "Timer";
  enumMap[MessageFieldTypeEvent][3] = "Workout";
  enumMap[MessageFieldTypeEvent][4] = "Workout Step";
  enumMap[MessageFieldTypeEvent][5] = "Power Down";
  enumMap[MessageFieldTypeEvent][6] = "Power Up";
  enumMap[MessageFieldTypeEvent][7] = "Off Course";
  enumMap[MessageFieldTypeEvent][8] = "Session";
  enumMap[MessageFieldTypeEvent][9] = "Lap";
  enumMap[MessageFieldTypeEvent][10] = "Course Point";
  enumMap[MessageFieldTypeEvent][11] = "Battery";
  enumMap[MessageFieldTypeEvent][12] = "Virtual Partner Pace";
  enumMap[MessageFieldTypeEvent][13] = "HR High Alert";
  enumMap[MessageFieldTypeEvent][14] = "HR Low Alert";
  enumMap[MessageFieldTypeEvent][15] = "Speed High Alert";
  enumMap[MessageFieldTypeEvent][16] = "Speed Low Alert";
  enumMap[MessageFieldTypeEvent][17] = "Cadence High Alert";
  enumMap[MessageFieldTypeEvent][18] = "Cadence Low Alert";
  enumMap[MessageFieldTypeEvent][19] = "Power High Alert";
  enumMap[MessageFieldTypeEvent][20] = "Power Low Alert";
  enumMap[MessageFieldTypeEvent][21] = "Recovery HR";
  enumMap[MessageFieldTypeEvent][22] = "Battery Low";
  enumMap[MessageFieldTypeEvent][23] = "Time Duration Alert";
  enumMap[MessageFieldTypeEvent][24] = "Distance Duration Alert";
  enumMap[MessageFieldTypeEvent][25] = "Calorie Duration Alert";
  enumMap[MessageFieldTypeEvent][26] = "Activity";
  enumMap[MessageFieldTypeEvent][27] = "Fitness Equipment";

  enumMap[MessageFieldTypeEventType][0] = "Start";
  enumMap[MessageFieldTypeEventType][1] = "Stop";
  enumMap[MessageFieldTypeEventType][2] = "Consecutive Depreciated";
  enumMap[MessageFieldTypeEventType][3] = "Marker";
  enumMap[MessageFieldTypeEventType][4] = "Stop All";
  enumMap[MessageFieldTypeEventType][5] = "Begin Depreciated";
  enumMap[MessageFieldTypeEventType][6] = "End Depreciated";
  enumMap[MessageFieldTypeEventType][7] = "End All Depreciated";
  enumMap[MessageFieldTypeEventType][8] = "Stop Disable";
  enumMap[MessageFieldTypeEventType][9] = "Stop Disable All";

  enumMap[MessageFieldTypeSwimStroke][0] = "Freestyle";
  enumMap[MessageFieldTypeSwimStroke][1] = "Backstroke";
  enumMap[MessageFieldTypeSwimStroke][2] = "Breaststroke";
  enumMap[MessageFieldTypeSwimStroke][3] = "Butterlfy";
  enumMap[MessageFieldTypeSwimStroke][4] = "Drill";
  enumMap[MessageFieldTypeSwimStroke][5] = "Mixed";
    
  enumMap[MessageFieldTypeLengthType][0] = "Resting";
  enumMap[MessageFieldTypeLengthType][1] = "Active";
    
  enumMap[MessageFieldTypePoolLengthUnit][0] = "meters";
  enumMap[MessageFieldTypePoolLengthUnit][1] = "yards";
        
  manufacturerMap[ManufacturerGarmin] = "Garmin";
  manufacturerMap[ManufacturerGarminFR405ANTFS] = "Garmin (FR405 ANTFS)";
  manufacturerMap[ManufacturerZephyr] = "Zephyr";
  manufacturerMap[ManufacturerDayton] = "Dayton";
  manufacturerMap[ManufacturerIDT] = "IDT";
  manufacturerMap[ManufacturerSRM] = "SRM";
  manufacturerMap[ManufacturerQuarq] = "Quarq";
  manufacturerMap[ManufacturerIBike] = "iBike";
  manufacturerMap[ManufacturerSaris] = "Saris";
  manufacturerMap[ManufacturerSparkHK] = "Spark HK";
  manufacturerMap[ManufacturerTanita] = "Tanita";
  manufacturerMap[ManufacturerEchowell] = "Echowell";
  manufacturerMap[ManufacturerDynastreamOEM] = "Dynastream OEM";
  manufacturerMap[ManufacturerNautilus] = "Nautilus";
  manufacturerMap[ManufacturerDynastream] = "Dynastream";
  manufacturerMap[ManufacturerTimex] = "Timex";
  manufacturerMap[ManufacturerMetriGear] = "MetriGear";
  manufacturerMap[ManufacturerXelic] = "Xelic";
  manufacturerMap[ManufacturerBeurer] = "Beurer";
  manufacturerMap[ManufacturerCardioSport] = "CardioSport";
  manufacturerMap[ManufacturerAandD] = "A&D";
  manufacturerMap[ManufacturerHMM] = "HMM";

  productMap[ManufacturerGarmin][GarminHRM1] = "Heart Rate Monitor"; 
  productMap[ManufacturerGarmin][GarminAXH01] = "AXH01 HRM Chipset";
  productMap[ManufacturerGarmin][GarminAXB01] = "AXB01 Chipset";
  productMap[ManufacturerGarmin][GarminAXB02] = "AXB02 Chipset";
  productMap[ManufacturerGarmin][GarminHRM2SS] = "HRM2SS";
  productMap[ManufacturerGarmin][GarminDsiAlf02] = "DSI ALF 02";
  productMap[ManufacturerGarmin][GarminFR405] = "Forerunner 405";
  productMap[ManufacturerGarmin][GarminFR50] = "Forerunner 50";
  productMap[ManufacturerGarmin][GarminFR60] = "Forerunner 60";
  productMap[ManufacturerGarmin][GarminFR310XT] = "Forerunner 310XT";
  productMap[ManufacturerGarmin][GarminEDGE500] = "EDGE 500";
  productMap[ManufacturerGarmin][GarminFR110] = "Forerunner 110";
  productMap[ManufacturerGarmin][GarminEDGE800] = "EDGE 800";
  productMap[ManufacturerGarmin][GarminEDGE200] = "EDGE 200";
  productMap[ManufacturerGarmin][GarminFR910XT] = "Forerunner 910XT";
  productMap[ManufacturerGarmin][GarminFR610] = "Forerunner 610";
  productMap[ManufacturerGarmin][GarminFR70] = "Forerunner 70";
  productMap[ManufacturerGarmin][GarminFR310XT4T] = "Forerunner 310XT 4T";
  productMap[ManufacturerGarmin][GarminTraningCenter] = "Traning Center";
  productMap[ManufacturerGarmin][GarminConnect] = "Connect";
  productMap[ManufacturerGarmin][GarminSwim] = "Swim";

  manufacturer = 0;

  mCreationTimestamp = 0;
  mFirstTimestamp    = 0;
  mLastTimestamp     = 0;
}

FIT::~FIT()
{
}

uint16_t FIT::CRC_byte(uint16_t crc, uint8_t byte)
{
  static const uint16_t crc_table[16] =
    {
      0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
      0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
    };

  uint16_t tmp = crc_table[crc & 0xF];
  crc = (crc >> 4) & 0x0FFF;
  crc = crc ^ tmp ^ crc_table[byte & 0xF];

  tmp = crc_table[crc & 0xF];
  crc = (crc >> 4) & 0x0FFF;
  crc = crc ^ tmp ^ crc_table[(byte >> 4) & 0xF];

  return crc;
}

string FIT::getDataString(uint8_t *ptr, uint8_t size, uint8_t baseType, uint8_t messageType, uint8_t fieldNum)
{
  ostringstream strstrm;
  strstrm.setf(ios::fixed,ios::floatfield);

  BaseType bt;
  bt.byte = baseType;

  int baseTypeNum = bt.bits.baseTypeNum;
  switch(baseTypeNum)
  {
    case BT_Enum:
    {
      int val = *(int8_t *)ptr;
      uint8_t type = messageFieldTypeMap[messageType][fieldNum];
      string strVal(enumMap[type][val]);

      if (!strVal.empty())
      {
        strstrm << strVal;
      }
      else
      {
        strstrm << "[" << dec << val << "]";
      }
      break;
    }
    case BT_Int8:
    {
      int val = *(int8_t *)ptr;
      strstrm << dec << val;
      break;
    }
    case BT_UInt8:
    case BT_Uint8z:
    {
      unsigned val = *(uint8_t *)ptr;
      if (val == 0xFF)
      {
        strstrm << "undefined";
      }
      else
      {
        strstrm << dec << val;
      }
      break;
    }
    case BT_Int16:
    {
      int16_t val = *(int16_t *)ptr;
      if (val == 0x7FFF)
      {
        strstrm << "undefined";
      }
      else
      {
        strstrm << dec << val;
      }
      break;
    }
    case BT_Uint16:
    case BT_Uint16z:
    {
      uint16_t val = *(uint16_t *)ptr;
      if (val == 0xFFFF)
      {
        strstrm << "undefined";
      }
      else
      {
        switch (messageFieldTypeMap[messageType][fieldNum])
        {
          case MessageFieldTypeAltitude:
          {
            strstrm << setprecision(1) << GarminConvert::altitude(val);
            break;
          }
          case MessageFieldTypeWeight:
          {
            strstrm << setprecision(1) << GarminConvert::weight(val);
            break;
          }
          case MessageFieldTypeSpeed:
          {
            strstrm << setprecision(1) << GarminConvert::speed(val);
            break;
          }
          case MessageFieldTypeManufacturer:
          {
            manufacturer = val;
            strstrm << manufacturerMap[static_cast<uint8_t>(manufacturer)];
            break;
          }
          case MessageFieldTypeProduct:
          {
            strstrm << productMap[static_cast<uint8_t>(manufacturer)][val];
            break;
          }
          default:
          {
            strstrm << dec << val;
          }
        }
      }
      break;
    }
    case BT_Int32:
    {
      int32_t val = *(int32_t *)ptr;
      if (val == 0x7FFFFFFF)
      {
        strstrm << "undefined";
      }
      else
      {
        switch(messageFieldTypeMap[messageType][fieldNum])
        {
          case MessageFieldTypeCoord:
          {
            strstrm << setprecision(5) << GarminConvert::coord(val);
            break;
          }
          default:
          {
            strstrm << dec << val;
          }
        }
      }
      break;
    }
    case BT_UInt32:
    case BT_Uint32z:
    {
      uint32_t val = *(uint32_t *)ptr;
      if (val == 0xFFFFFFFF)
      {
        strstrm << "undefined";
      }
      else
      {
        if (fieldNum == 253)
        {
          strstrm << GarminConvert::localTime(val);
        }
        else
        {
          switch (messageFieldTypeMap[messageType][fieldNum])
          {
            case MessageFieldTypeTimestamp:
            {
              strstrm << GarminConvert::localTime(val);
              break;
            }
            case MessageFieldTypeTime:
            {
              strstrm << GarminConvert::gTime(val);
              break;
            }
            case MessageFieldTypeOdometr:
            {
              strstrm << setprecision(2) << GarminConvert::length(val);
              break;
            }
            default:
            {
              strstrm << dec << val;
            }
          }
        }
      }
      break;
    }
    case BT_String:
    {
      strstrm << "\"" << GarminConvert::gString(ptr, size) << "\"";
      break;
    }
  }

  return strstrm.str();
}

bool FIT::parse(vector<uint8_t> &fitData, GPX &gpx)
{
  ostringstream strstrm;
    
  LOG(LOG_DBG2) << "Parsing FIT file\n";

  FITHeader fitHeader;
  if(fitData.size()<sizeof(fitHeader))
    return false;

  uint8_t *ptr = &fitData.front();
  memcpy(&fitHeader, ptr, sizeof(fitHeader));

  // FIT header CRC
  uint16_t crc = 0;
  for (int i = 0; i < fitHeader.headerSize; i++)
  {
    crc = CRC_byte(crc, *(ptr+i));
  }

  ptr += fitHeader.headerSize;

  // FIT data CRC
  for (uint32_t i = 0; i < fitHeader.dataSize; i++)
  {
    crc = CRC_byte(crc, *(ptr+i));
  }

  if (memcmp(fitHeader.signature, ".FIT", sizeof(fitHeader.signature)))
  {
    LOG(LOG_DBG) << "FIT signature not found\n";
    return false;
  }

  uint16_t fitCRC = *(uint16_t *)(ptr+fitHeader.dataSize);
  if (crc != fitCRC /*&& fitCRC != 0*/)
  {
    LOG(LOG_WARN) << hex << uppercase << setw(4) << setfill('0') << "Invalid FIT CRC (" << crc << "!=" << fitCRC << ")\n";
    return false;
  }

  LOG(LOG_DBG2) << "FIT Protocol Version " << dec << (unsigned)fitHeader.protocolVersion << "\n";

  LOG(LOG_DBG2) << "FIT Profile Version " << fitHeader.profileVersion << "\n";

  LOG(LOG_DBG2) << "FIT Data size " << fitHeader.dataSize << " bytes\n";
   
  map<uint8_t, RecordDef> recDefMap;
  vector<uint32_t>        tstamps;

  for (int bytes = fitHeader.dataSize; bytes > 0;)
  {
    RecordHeader rh;
    memcpy(&rh, ptr, sizeof(rh));
    ptr += sizeof(rh);
    bytes -= sizeof(rh);

    if (!rh.normalHeader.headerType)
    {
      // Normal Header
      if (rh.normalHeader.messageType)
      {
        // Definition Message
        RecordDef rd;

        RecordFixed rfx;
        memcpy(&rfx, ptr, sizeof(rfx));
        ptr += sizeof(rfx);
        bytes -= sizeof(rfx);

        rd.rfx = rfx;

        for (int i=0; i<rfx.fieldsNum; i++)
        {
          RecordField rf;
          memcpy(&rf, ptr, sizeof(rf));
          ptr += sizeof(rf);
          bytes -= sizeof(rf);

          rd.rf.push_back(rf);
        }

        recDefMap[rh.normalHeader.localMessageType] = rd;
      }
      else
      {
        // Data Message
        map<uint8_t, RecordDef>::iterator it=recDefMap.find(rh.normalHeader.localMessageType);
        if (it != recDefMap.end())
        {
          RecordDef rd = recDefMap[rh.normalHeader.localMessageType];
          //logger() << "Local Message \"" << messageTypeMap[rd.rfx.globalNum] << "\"(" << rd.rfx.globalNum << "):\n";
                    
          switch(rd.rfx.globalNum)
          {
            case 29: // WayPoint
            {
              gpx.newWayPoint();
              break;
            }
          }

          uint32_t fileCreationTime=0;
          int8_t fileType=INT8_MAX;

          uint32_t time;

          for (int i=0; i<rd.rfx.fieldsNum; i++)
          {
            RecordField &rf = rd.rf[i];

            //BaseType bt;
            //bt.byte = rf.baseType;

//                          LOG(LOG_DBG2) << rd.rfx.globalNum << "." << (unsigned)rf.definitionNum << ": " << messageFieldNameMap[rd.rfx.globalNum][rf.definitionNum] << "\n";
//                                     " (" << dataTypeMap[bt.bits.baseTypeNum] << ") " << getDataString(ptr, rf.size, bt.bits.baseTypeNum, rd.rfx.globalNum, rf.definitionNum) << "\n";

            switch(rd.rfx.globalNum)
            {
              case 0: // File Id
              {
                switch(rf.definitionNum)
                {
                  case 0: // Type
                  {
                    fileType = *(int8_t *)ptr;
                    break;
                  }
                  case 4: // Creation Time
                  {
                    fileCreationTime = *(uint32_t*)ptr;
                    mCreationTimestamp = fileCreationTime;
                    break;
                  }
                }
                break;
              }
              case 19: // Lap
              {
                switch(rf.definitionNum)
                {
                  case 253: // Timestamp
                  {
                    time = *(uint32_t*)ptr;
                    // TODO: add to gpx?
                    tstamps.push_back(time);
                    break;
                  }
                }
                break;
              }
              case 20: // Record
              {
                switch(rf.definitionNum)
                {
                  case 253: // Timestamp
                  {
                    time = *(uint32_t*)ptr;
                    gpx.tracks.back().trackSegs.back().trackPoints[time].time = time;
                    tstamps.push_back(time);
                    break;
                  }
                  case 0: // Latitude
                  {
                    int32_t latitude = *(int32_t*)ptr;
                    gpx.tracks.back().trackSegs.back().trackPoints[time].latitude = latitude;
                    break;
                  }
                  case 1: // Longitude
                  {
                    uint32_t longitude = *(int32_t*)ptr;
                    gpx.tracks.back().trackSegs.back().trackPoints[time].longitude = longitude;
                    break;
                  }
                  case 2: // Altitude
                  {
                    uint16_t altitude = *(uint16_t*)ptr;
                    gpx.tracks.back().trackSegs.back().trackPoints[time].altitude = altitude;
                    break;
                  }
                  case 3: // Heart Rate
                  {
                    uint8_t heartRate = *(uint8_t*)ptr;
                    gpx.tracks.back().trackSegs.back().trackPoints[time].heartRate = heartRate;
                    break;
                  }
                  case 4: // Cadence
                  {
                    uint8_t cadence = *(uint8_t*)ptr;
                    gpx.tracks.back().trackSegs.back().trackPoints[time].cadence = cadence;
                    break;
                  }
                }
                break;
              }
              case 29: // WayPoint
              {
                switch(rf.definitionNum)
                {
                  case 253: // Timestamp
                  {
                    time = *(uint32_t*)ptr;
                    gpx.wayPoints.back().time = time;
                    tstamps.push_back(time);
                    break;
                  }
                  case 0: // Name
                  {
                    string name = GarminConvert::gString(ptr, 16);
                    gpx.wayPoints.back().name = name;
                    break;
                  }
                  case 1: // Latitude
                  {
                    int32_t latitude = *(int32_t*)ptr;
                    gpx.wayPoints.back().latitude = latitude;
                    break;
                  }
                  case 2: // Longitude
                  {
                    int32_t longitude = *(int32_t*)ptr;
                    gpx.wayPoints.back().longitude = longitude;
                    break;
                  }
                  case 3: // Symbol
                  {
                    break;
                  }
                  case 4: // Altitude
                  {
                    uint16_t altitude = *(uint16_t*)ptr;
                    gpx.wayPoints.back().altitude = altitude;
                    break;
                  }
                }
                break;
              }
              case 31: // Course
              {
                switch(rf.definitionNum)
                {
                  case 5: // Name
                  {
                    string name("Course_");
                    name += GarminConvert::gString(ptr, 16);
                    gpx.tracks.back().name = name;
                    break;
                  }
                }
                break;
              }
              case 101: // Length
              {
                strstrm.clear();
                strstrm << rd.rfx.globalNum << "." << (unsigned)rf.definitionNum << ": " << messageFieldNameMap[rd.rfx.globalNum][rf.definitionNum] << " (";
                switch (rf.definitionNum)
                {
                  case 2 : // Start Time
                  {
                    strstrm << getDataString(ptr, 0, BT_UInt32, rd.rfx.globalNum, rf.definitionNum);
                    break;
                  }
                  case 3 : // Total Elapsed Time
                  {
                    strstrm << getDataString(ptr, 0, BT_UInt32, rd.rfx.globalNum, rf.definitionNum);
                    break;
                  }
                  case 4 : // Total Timer Time
                  {
                    strstrm << getDataString(ptr, 0, BT_UInt32, rd.rfx.globalNum, rf.definitionNum);
                    break;
                  }
                  case 5: // Total Strokes
                  {
                    strstrm << *(uint16_t*)ptr;
                    break;
                  }
                  case 6: // Average Speed
                  {
                    // unit: m/s * 1000
                    double speed = (double)(*(uint16_t*)ptr) / 1000;
                    strstrm << speed;
                                        
                    break;
                  }
                  case 7: // Swimming stroke
                  {
                    strstrm << getDataString(ptr, 0, BT_Enum, rd.rfx.globalNum, rf.definitionNum);
                    break;
                  }
                  case 9: // Average Swimming Cadence
                  {
                    strstrm << (int) *(uint8_t*)ptr;
                    break;
                  }
                  case 12: // Length Type
                  {
                    strstrm << getDataString(ptr, 0, BT_Enum, rd.rfx.globalNum, rf.definitionNum);
                    break;
                  }
                }
                strstrm << ")\n";
                LOG(LOG_DBG2) << strstrm.str();
                break;
              }
            }

            ptr += rf.size;
            bytes -= rf.size;
          }

          switch(rd.rfx.globalNum)
          {
            case 0: // File Id
            {
              switch (fileType)
              {
                case 4: // Activity
                {
                  gpx.newTrack(string("Track_") + GarminConvert::localTime(fileCreationTime));
                  break;
                }
                case 6: // Course
                {
                  gpx.newTrack(string("Course_") + GarminConvert::localTime(fileCreationTime));
                  break;
                }
              }
              break;
            }
            case 19: // Lap
            {
              gpx.newTrackSeg();
              break;
            }
          }
        }
        else
        {
          logger() << "Undefined Local Message Type: " << (unsigned)rh.normalHeader.localMessageType << "\n";
          return false;
        }
      }
    }
    else
    {
      // Compressed Timestamp Header
      logger() << "Compressed Timestamp Header:" << endl;
      logger() << "  Local Message Type " << (unsigned)rh.ctsHeader.localMessageType << endl;
      logger() << "  Time Offset " << (unsigned)rh.ctsHeader.timeOffset << "\n";
    }
  }

  std::remove_if(tstamps.begin(), tstamps.end(), isZero);
  std::sort(tstamps.begin(), tstamps.end());
//        for(size_t i = 0; i < tstamps.size(); i++)
//        {
//          LOG_VAR2(i, tstamps[i]);
//        }
  mFirstTimestamp = tstamps.front();
  mLastTimestamp = tstamps.back();

  return true;
}

struct DateSorter
{
  bool operator()(const ZeroFileRecord& a, const ZeroFileRecord& b) const
  { return a.timeStamp > b.timeStamp; }
} dateSorter;


bool FIT::parseZeroFile(vector<uint8_t> &data, ZeroFileContent &zeroFileContent)
{
  logger() << "Parsing zero file...\n";
    
  DirectoryHeader directoryHeader;
  if (data.size() < sizeof(directoryHeader))
  {
    LOG(LOG_WARN) << "Zero file data is too short to get header\n";
    return false;
  }

  memcpy(&directoryHeader, &data.front(), sizeof(directoryHeader));
  data.erase(data.begin(), data.begin()+sizeof(directoryHeader));
    
  logger() << "Directory version: " << hex << setw(2) << (unsigned)directoryHeader.version << "\n";
  logger() << "Structure length: " << dec << (unsigned)directoryHeader.structureLength << "\n";
  logger() << "Time format: " << dec << (unsigned)directoryHeader.timeFormat << "\n";
  logger() << (unsigned)directoryHeader.reserved[0] << " " << (unsigned)directoryHeader.reserved[1] << " " << (unsigned)directoryHeader.reserved[2] << " "
           << (unsigned)directoryHeader.reserved[3] << " " << (unsigned)directoryHeader.reserved[4] << "\n";
  logger() << "Current system time: " << GarminConvert::localTime(directoryHeader.currentSystemTime) << "\n";
  logger() << "Directory modified time: " << GarminConvert::localTime(directoryHeader.directoryModifiedTime) << "\n";

  int records = static_cast<int>(data.size()) / directoryHeader.structureLength;

  if(data.empty() || data.size() < (sizeof(ZeroFileRecord)*records))
  {
    logger() << "Zero file data is truncated to read...\n";

    return false;
  }

  //logger() << uppercase;

  logger() << "_idx" << "|d" << "ata" << "type|" << "recordType|" << "_rt_" << "++ID++" << "__fileSize|" << "+++++++++++++++++++|" << "flags" << "\n";
  uint8_t *ptr = &data.front();
  for (int i=0; i<records; i++)
  {
    ZeroFileRecord zfRecord;
    memcpy(&zfRecord, ptr, sizeof(zfRecord));
    ptr += sizeof(zfRecord);

    logger() << hex << setw(4) << setfill('0') << (unsigned)zfRecord.index << ": " <<
      ((zfRecord.fileDataType == 0x80)?"FIT":"   ") <<
      "(" << setw(2) << setfill('0') << (unsigned)zfRecord.fileDataType << ") " <<
      setw(10) << setfill(' ') << enumMap[MessageFieldTypeFileType][zfRecord.recordType] << " " <<
      "(" << setw(2) << setfill('0') << (unsigned)zfRecord.recordType << ")" <<
      "(" << setw(4) << setfill('0') << (unsigned)zfRecord.identifier << ")" <<
      dec << setw(10) << setfill(' ') << (unsigned)zfRecord.fileSize << " " <<
      GarminConvert::localTime(zfRecord.timeStamp) << " ";
    if (zfRecord.generalFileFlags.read)    { LOG(antpm::LOG_RAW) << "[R]"; }
    if (zfRecord.generalFileFlags.write)   { LOG(antpm::LOG_RAW) << "[W]"; }
    if (zfRecord.generalFileFlags.erase)   { LOG(antpm::LOG_RAW) << "[E]"; }
    if (zfRecord.generalFileFlags.append)  { LOG(antpm::LOG_RAW) << "[Ap]"; }
    if (zfRecord.generalFileFlags.archive) { LOG(antpm::LOG_RAW) << "[Ar]"; }
    if (zfRecord.generalFileFlags.crypto)  { LOG(antpm::LOG_RAW) << "[C]"; }
    LOG(antpm::LOG_RAW) << "\n";
    zeroFileContent.zfRecords.push_back(zfRecord);
  }

  std::sort(zeroFileContent.zfRecords.begin(), zeroFileContent.zfRecords.end(), dateSorter);

  for(size_t i = 0; i < zeroFileContent.zfRecords.size(); i++)
  {
    ZeroFileRecord& zfRecord(zeroFileContent.zfRecords[i]);
    LOG(LOG_DBG2) << hex << setw(4) << setfill('0') << (unsigned)zfRecord.index << ": " <<
      GarminConvert::localTime(zfRecord.timeStamp) << "\n";
    switch(zfRecord.recordType)
    {
      case 4: // Activity
      {
        zeroFileContent.activityFiles.push_back(zfRecord.index);
        break;
      }
      case 6: // Course
      {
        zeroFileContent.courseFiles.push_back(zfRecord.index);
        break;
      }
      case 8: // Waypoints
      {
        zeroFileContent.waypointsFiles.push_back(zfRecord.index);
        break;
      }
    }
  }

  return true;
}


/// returns in GMT/UTC
bool
FIT::getCreationDate(std::vector<uint8_t> &fitData, std::time_t &ct)
{
  GPX gpx;
  FIT fit;
  if(!fit.parse(fitData, gpx))
    return false;
  time_t t = fit.getCreationTimestamp();
  if(t==0)
    return false;
  t = GarminConvert::gOffsetTime(t);
  ct = t;
  return true;
}


}
