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


/* 
   Check to see whether a command is supported by a Garmin unit based on its
   available command protocols.
*/

int
garmin_command_supported ( garmin_unit *    garmin,
			   garmin_command   cmd )
{
  int ret = 0;

  switch ( cmd ) {
  case Cmnd_Abort_Transfer:
  case Cmnd_Turn_Off_Pwr:
    ret = 1;
    break;

  case Cmnd_Transfer_Alm:
    ret = garmin->protocol.almanac;
    break;

  case Cmnd_Transfer_Prx:
    ret = garmin->protocol.waypoint.proximity;
    break;

  case Cmnd_Transfer_Rte:
    ret = garmin->protocol.route;
    break;

  case Cmnd_Transfer_Time:
    ret = garmin->protocol.date_time;
    break;

  case Cmnd_Transfer_Wpt:
    ret = garmin->protocol.waypoint.waypoint;
    break;

  case Cmnd_Transfer_Posn:
    ret = garmin->protocol.position;
    break;

  case Cmnd_Transfer_Trk:
    ret = garmin->protocol.track;
    break;

  case Cmnd_Start_Pvt_Data:
  case Cmnd_Stop_Pvt_Data:
    ret = garmin->protocol.pvt;
    break;

  case Cmnd_FlightBook_Transfer:
    ret = garmin->protocol.flightbook;
    break;

  case Cmnd_Transfer_Laps:
    ret = garmin->protocol.lap;
    break;

  case Cmnd_Transfer_Wpt_Cats:
    ret = garmin->protocol.waypoint.category;
    break;

  case Cmnd_Transfer_Runs:
    ret = garmin->protocol.run;
    break;

  case Cmnd_Transfer_Workouts:
    ret = garmin->protocol.workout.workout;
    break;

  case Cmnd_Transfer_Workout_Occurrences:
    ret = garmin->protocol.workout.occurrence;
    break;

  case Cmnd_Transfer_Fitness_User_Profile:
    ret = garmin->protocol.fitness;
    break;

  case Cmnd_Transfer_Workout_Limits:
    ret = garmin->protocol.workout.limits;
    break;

  case Cmnd_Transfer_Courses:
    ret = garmin->protocol.course.course;
    break;

  case Cmnd_Transfer_Course_Laps:
    ret = garmin->protocol.course.lap;
    break;

  case Cmnd_Transfer_Course_Points:
    ret = garmin->protocol.course.point;
    break;

  case Cmnd_Transfer_Course_Tracks:
    ret = garmin->protocol.course.track + garmin->protocol.track;
    break;

  case Cmnd_Transfer_Course_Limits:
    ret = garmin->protocol.course.limits;
    break;

  default: 
    break;
  }

  return ret;
}


/* 
   Convert a garmin_command to a command packet, returning 1 if the command
   is supported by the unit's command protocol and 0 if it isn't.
*/

int
garmin_make_command_packet ( garmin_unit *    garmin, 
			     garmin_command   cmd,
			     garmin_packet *  packet )
{
  int    r = 1;
  uint16 c = 0;
  uint16 p = 0;
  uint8  b[2];

  /* Determine the packet ID based on the link protocol. */

  switch ( garmin->protocol.link ) {
  case link_L001:           p = L001_Pid_Command_Data;                   break;
  case link_L002:           p = L002_Pid_Command_Data;                   break;
  default:                  r = 0;                                       break;
  }

  /* 
     Although it's obvious from the spec that L001 implies A010 and L002
     implies A011, this relationship is not explicit.  We have to determine
     the command ID based on the unit's stated command protocol.
  */

#define CMD_CASE(x,y) case Cmnd_##y: c = x##_Cmnd_##y; break
#define A010_CASE(x)  CMD_CASE(A010,x)
#define A011_CASE(x)  CMD_CASE(A011,x)
#define CMD_DEFAULT   default: r = 0; break

  switch ( garmin->protocol.command ) {
  case appl_A010:
    switch ( cmd ) {
      A010_CASE(Abort_Transfer);
      A010_CASE(Turn_Off_Pwr);
      A010_CASE(Start_Pvt_Data);
      A010_CASE(Stop_Pvt_Data);
      A010_CASE(Transfer_Alm);
      A010_CASE(Transfer_Posn);
      A010_CASE(Transfer_Prx);
      A010_CASE(Transfer_Rte);
      A010_CASE(Transfer_Time);
      A010_CASE(Transfer_Trk);
      A010_CASE(Transfer_Wpt);
      A010_CASE(FlightBook_Transfer);
      A010_CASE(Transfer_Laps);
      A010_CASE(Transfer_Wpt_Cats);
      A010_CASE(Transfer_Runs);
      A010_CASE(Transfer_Workouts);
      A010_CASE(Transfer_Workout_Occurrences);
      A010_CASE(Transfer_Fitness_User_Profile);
      A010_CASE(Transfer_Workout_Limits);
      A010_CASE(Transfer_Courses);
      A010_CASE(Transfer_Course_Laps);
      A010_CASE(Transfer_Course_Points);
      A010_CASE(Transfer_Course_Tracks);
      A010_CASE(Transfer_Course_Limits);
      CMD_DEFAULT;
    }
    break;
  case appl_A011:
    switch ( cmd ) {
      A011_CASE(Abort_Transfer);
      A011_CASE(Turn_Off_Pwr);
      A011_CASE(Transfer_Alm);
      A011_CASE(Transfer_Prx);
      A011_CASE(Transfer_Rte);
      A011_CASE(Transfer_Time);
      A011_CASE(Transfer_Wpt);
      CMD_DEFAULT;
    }
    break;
    CMD_DEFAULT;
  }

  /* 
     If the command is supported by the unit's command protocol, build the
     packet.
  */

  if ( r != 0 ) {
    put_uint16(b,c);
    garmin_packetize(packet,p,2,b);
  }
  
  return r;
}


/* Send a command */

int
garmin_send_command ( garmin_unit * garmin, garmin_command cmd )
{
  garmin_packet packet;
  int ret = 0;

  if ( garmin_command_supported(garmin,cmd) &&
       garmin_make_command_packet(garmin,cmd,&packet) ) {
    ret = garmin_write(garmin,&packet);
  } else {
    /* Error: command not supported */

    printf("Error: command %d not supported\n",cmd);
  }

  return ret;
}


