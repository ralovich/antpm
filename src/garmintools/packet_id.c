/*
  Garmintools software package
  Copyright (C) 2006-2008 Dave Bailey
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"
#include "garmin.h"


/* Given a garmin_pid, translate to a link packet ID */

uint16
garmin_lpid ( link_protocol link, garmin_pid gpid )
{
  uint16 lpid = 0x0000;

  switch ( link ) {
  case link_L001:
    switch ( gpid ) { 
    case Pid_Protocol_Array:       lpid = L000_Pid_Protocol_Array;       break;
    case Pid_Product_Rqst:         lpid = L000_Pid_Product_Rqst;         break;
    case Pid_Product_Data:         lpid = L000_Pid_Product_Data;         break;
    case Pid_Ext_Product_Data:     lpid = L000_Pid_Ext_Product_Data;     break;
    case Pid_Almanac_Data:         lpid = L001_Pid_Almanac_Data;         break;
    case Pid_Command_Data:         lpid = L001_Pid_Command_Data;         break;
    case Pid_Xfer_Cmplt:           lpid = L001_Pid_Xfer_Cmplt;           break;
    case Pid_Date_Time_Data:       lpid = L001_Pid_Date_Time_Data;       break;
    case Pid_Position_Data:        lpid = L001_Pid_Position_Data;        break;
    case Pid_Prx_Wpt_Data:         lpid = L001_Pid_Prx_Wpt_Data;         break;
    case Pid_Records:              lpid = L001_Pid_Records;              break;
    case Pid_Rte_Hdr:              lpid = L001_Pid_Rte_Hdr;              break;
    case Pid_Rte_Wpt_Data:         lpid = L001_Pid_Rte_Wpt_Data;         break;
    case Pid_Wpt_Data:             lpid = L001_Pid_Rte_Wpt_Data;         break;
    case Pid_Trk_Data:             lpid = L001_Pid_Trk_Data;             break;
    case Pid_Pvt_Data:             lpid = L001_Pid_Pvt_Data;             break;
    case Pid_Rte_Link_Data:        lpid = L001_Pid_Rte_Link_Data;        break;
    case Pid_Trk_Hdr:              lpid = L001_Pid_Trk_Hdr;              break;
    case Pid_FlightBook_Record:    lpid = L001_Pid_FlightBook_Record;    break;
    case Pid_Lap:                  lpid = L001_Pid_Lap;                  break;
    case Pid_Wpt_Cat:              lpid = L001_Pid_Wpt_Cat;              break;
    case Pid_Run:                  lpid = L001_Pid_Run;                  break;
    case Pid_Workout:              lpid = L001_Pid_Workout;              break;
    case Pid_Workout_Occurrence:   lpid = L001_Pid_Workout_Occurrence;   break;
    case Pid_Fitness_User_Profile: lpid = L001_Pid_Fitness_User_Profile; break;
    case Pid_Workout_Limits:       lpid = L001_Pid_Workout_Limits;       break;
    case Pid_Course:               lpid = L001_Pid_Course;               break;
    case Pid_Course_Lap:           lpid = L001_Pid_Course_Lap;           break;
    case Pid_Course_Point:         lpid = L001_Pid_Course_Point;         break;
    case Pid_Course_Trk_Hdr:       lpid = L001_Pid_Course_Trk_Hdr;       break;
    case Pid_Course_Trk_Data:      lpid = L001_Pid_Course_Trk_Data;      break;
    case Pid_Course_Limits:        lpid = L001_Pid_Course_Limits;        break;
    default:                                                             break;
    }
    break;
  case link_L002:
    switch ( gpid ) {
    case Pid_Protocol_Array:       lpid = L000_Pid_Protocol_Array;       break;
    case Pid_Product_Rqst:         lpid = L000_Pid_Product_Rqst;         break;
    case Pid_Product_Data:         lpid = L000_Pid_Product_Data;         break;
    case Pid_Ext_Product_Data:     lpid = L000_Pid_Ext_Product_Data;     break;
    case Pid_Almanac_Data:         lpid = L002_Pid_Almanac_Data;         break;
    case Pid_Command_Data:         lpid = L002_Pid_Command_Data;         break;
    case Pid_Xfer_Cmplt:           lpid = L002_Pid_Xfer_Cmplt;           break;
    case Pid_Date_Time_Data:       lpid = L002_Pid_Date_Time_Data;       break;
    case Pid_Position_Data:        lpid = L002_Pid_Position_Data;        break;
    case Pid_Prx_Wpt_Data:         lpid = L002_Pid_Prx_Wpt_Data;         break;
    case Pid_Records:              lpid = L002_Pid_Records;              break;
    case Pid_Rte_Hdr:              lpid = L002_Pid_Rte_Hdr;              break;
    case Pid_Rte_Wpt_Data:         lpid = L002_Pid_Rte_Wpt_Data;         break;
    case Pid_Wpt_Data:             lpid = L002_Pid_Rte_Wpt_Data;         break;
    default:                                                             break;
    }
    break;
  default:
    break;
  }

  return lpid;
}


/* Given a link-specific PID, translate it to a garmin packet ID. */

garmin_pid
garmin_gpid ( link_protocol link, uint16 lpid )
{
  garmin_pid gpid = Pid_Nil;

  switch ( link ) {
  case link_L001:
    switch ( lpid ) { 
    case L000_Pid_Protocol_Array:       gpid = Pid_Protocol_Array;       break;
    case L000_Pid_Product_Rqst:         gpid = Pid_Product_Rqst;         break;
    case L000_Pid_Product_Data:         gpid = Pid_Product_Data;         break;
    case L000_Pid_Ext_Product_Data:     gpid = Pid_Ext_Product_Data;     break;
    case L001_Pid_Almanac_Data:         gpid = Pid_Almanac_Data;         break;
    case L001_Pid_Command_Data:         gpid = Pid_Command_Data;         break;
    case L001_Pid_Xfer_Cmplt:           gpid = Pid_Xfer_Cmplt;           break;
    case L001_Pid_Date_Time_Data:       gpid = Pid_Date_Time_Data;       break;
    case L001_Pid_Position_Data:        gpid = Pid_Position_Data;        break;
    case L001_Pid_Prx_Wpt_Data:         gpid = Pid_Prx_Wpt_Data;         break;
    case L001_Pid_Records:              gpid = Pid_Records;              break;
    case L001_Pid_Rte_Hdr:              gpid = Pid_Rte_Hdr;              break;
    case L001_Pid_Rte_Wpt_Data:         gpid = Pid_Rte_Wpt_Data;         break;
    case L001_Pid_Wpt_Data:             gpid = Pid_Rte_Wpt_Data;         break;
    case L001_Pid_Trk_Data:             gpid = Pid_Trk_Data;             break;
    case L001_Pid_Pvt_Data:             gpid = Pid_Pvt_Data;             break;
    case L001_Pid_Rte_Link_Data:        gpid = Pid_Rte_Link_Data;        break;
    case L001_Pid_Trk_Hdr:              gpid = Pid_Trk_Hdr;              break;
    case L001_Pid_FlightBook_Record:    gpid = Pid_FlightBook_Record;    break;
    case L001_Pid_Lap:                  gpid = Pid_Lap;                  break;
    case L001_Pid_Wpt_Cat:              gpid = Pid_Wpt_Cat;              break;
    case L001_Pid_Run:                  gpid = Pid_Run;                  break;
    case L001_Pid_Workout:              gpid = Pid_Workout;              break;
    case L001_Pid_Workout_Occurrence:   gpid = Pid_Workout_Occurrence;   break;
    case L001_Pid_Fitness_User_Profile: gpid = Pid_Fitness_User_Profile; break;
    case L001_Pid_Workout_Limits:       gpid = Pid_Workout_Limits;       break;
    case L001_Pid_Course:               gpid = Pid_Course;               break;
    case L001_Pid_Course_Lap:           gpid = Pid_Course_Lap;           break;
    case L001_Pid_Course_Point:         gpid = Pid_Course_Point;         break;
    case L001_Pid_Course_Trk_Hdr:       gpid = Pid_Course_Trk_Hdr;       break;
    case L001_Pid_Course_Trk_Data:      gpid = Pid_Course_Trk_Data;      break;
    case L001_Pid_Course_Limits:        gpid = Pid_Course_Limits;        break;
    default:                                                             break;
    }
    break;
  case link_L002:
    switch ( lpid ) {
    case L000_Pid_Protocol_Array:       gpid = Pid_Protocol_Array;       break;
    case L000_Pid_Product_Rqst:         gpid = Pid_Product_Rqst;         break;
    case L000_Pid_Product_Data:         gpid = Pid_Product_Data;         break;
    case L000_Pid_Ext_Product_Data:     gpid = Pid_Ext_Product_Data;     break;
    case L002_Pid_Almanac_Data:         gpid = Pid_Almanac_Data;         break;
    case L002_Pid_Command_Data:         gpid = Pid_Command_Data;         break;
    case L002_Pid_Xfer_Cmplt:           gpid = Pid_Xfer_Cmplt;           break;
    case L002_Pid_Date_Time_Data:       gpid = Pid_Date_Time_Data;       break;
    case L002_Pid_Position_Data:        gpid = Pid_Position_Data;        break;
    case L002_Pid_Prx_Wpt_Data:         gpid = Pid_Prx_Wpt_Data;         break;
    case L002_Pid_Records:              gpid = Pid_Records;              break;
    case L002_Pid_Rte_Hdr:              gpid = Pid_Rte_Hdr;              break;
    case L002_Pid_Rte_Wpt_Data:         gpid = Pid_Rte_Wpt_Data;         break;
    case L002_Pid_Wpt_Data:             gpid = Pid_Rte_Wpt_Data;         break;
    default:                                                             break;
    }
    break;
  default:
    break;
  }

  return gpid;
}
