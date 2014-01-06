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
#include <string.h>
#include <stdlib.h>
#include "garmin.h"


/* ------------------------------------------------------------------------- */
/* Assign an application protocol to the Garmin unit.                        */
/* ------------------------------------------------------------------------- */

static void
garmin_assign_protocol ( garmin_unit *  garmin, 
			 uint16         protocol,
			 uint16 *       datatypes )
{
  /* Unknown protocols and their data types are ignored. */

  switch ( protocol ) {
  case appl_A010:
  case appl_A011:
    garmin->protocol.command             = protocol;
    break;

  case appl_A100:
    garmin->protocol.waypoint.waypoint   = protocol;  
    garmin->datatype.waypoint.waypoint   = datatypes[0];
    break;
    
  case appl_A101:
    garmin->protocol.waypoint.category   = protocol;
    garmin->datatype.waypoint.category   = datatypes[0];
    break;

  case appl_A200:
    garmin->protocol.route               = protocol;
    garmin->datatype.route.header        = datatypes[0];
    garmin->datatype.route.waypoint      = datatypes[1];

  case appl_A201:
    garmin->protocol.route               = protocol;
    garmin->datatype.route.header        = datatypes[0];
    garmin->datatype.route.waypoint      = datatypes[1];
    garmin->datatype.route.link          = datatypes[2];
    break;

  case appl_A300:
    garmin->protocol.track               = protocol;
    garmin->datatype.track.data          = datatypes[0];
    break;

  case appl_A301:
  case appl_A302:
    garmin->protocol.track               = protocol;
    garmin->datatype.track.header        = datatypes[0];
    garmin->datatype.track.data          = datatypes[1];
    break;

  case appl_A400:
    garmin->protocol.waypoint.proximity  = protocol;
    garmin->datatype.waypoint.proximity  = datatypes[0];
    break;

  case appl_A500:
    garmin->protocol.almanac             = protocol;
    garmin->datatype.almanac             = datatypes[0];
    break;

  case appl_A600:
    garmin->protocol.date_time           = protocol;
    garmin->datatype.date_time           = datatypes[0];
    break;

  case appl_A601:
    /* --- UNDOCUMENTED --- */
    break;

  case appl_A650:
    garmin->protocol.flightbook          = protocol;
    garmin->datatype.flightbook          = datatypes[0];
    break;

  case appl_A700:
    garmin->protocol.position            = protocol;
    garmin->datatype.position            = datatypes[0];
    break;

  case appl_A800:
    garmin->protocol.pvt                 = protocol;
    garmin->datatype.pvt                 = datatypes[0];
    break;

  case appl_A801:
    /* --- UNDOCUMENTED --- */
    break;

  case appl_A902:
    /* --- UNDOCUMENTED --- */
    break;

  case appl_A903:
    /* --- UNDOCUMENTED --- */
    break;

  case appl_A906:
    garmin->protocol.lap                 = protocol;
    garmin->datatype.lap                 = datatypes[0];
    break;

  case appl_A907:
    /* --- UNDOCUMENTED --- */
    break;

  case appl_A1000:
    garmin->protocol.run                 = protocol;
    garmin->datatype.run                 = datatypes[0];
    break;

  case appl_A1002:
    garmin->protocol.workout.workout     = protocol;
    garmin->datatype.workout.workout     = datatypes[0];
    break;

  case appl_A1003:
    garmin->protocol.workout.occurrence  = protocol;
    garmin->datatype.workout.occurrence  = datatypes[0];
    break;

  case appl_A1004:
    garmin->protocol.fitness             = protocol;
    garmin->datatype.fitness             = datatypes[0];
    break;

  case appl_A1005:
    garmin->protocol.workout.limits      = protocol;
    garmin->datatype.workout.limits      = datatypes[0];
    break;

  case appl_A1006:
    garmin->protocol.course.course       = protocol;
    garmin->datatype.course.course       = datatypes[0];
    break;

  case appl_A1007:
    garmin->protocol.course.lap          = protocol;
    garmin->datatype.course.lap          = datatypes[0];
    break;

  case appl_A1008:
    garmin->protocol.course.point        = protocol;
    garmin->datatype.course.point        = datatypes[0];

  case appl_A1009:
    garmin->protocol.course.limits       = protocol;
    garmin->datatype.course.limits       = datatypes[0];
    break;

  case appl_A1012:
    garmin->protocol.course.track        = protocol;
    garmin->datatype.course.track.header = datatypes[0];
    garmin->datatype.course.track.data   = datatypes[1];
    break;

  default:
    break;
  }
}


static char **
merge_strings ( char ** one, char ** two )
{
  int     i;
  int     n1;
  int     n2;
  char ** pos;
  char ** ret = NULL;

  for ( pos = one, n1 = 0; pos && *pos; pos++, n1++ );
  for ( pos = two, n2 = 0; pos && *pos; pos++, n2++ );

  if ( n1 + n2 > 0 ) {
    ret = calloc(n1+n2+1,sizeof(char *));
    for ( i = 0; i < n1; i++ ) ret[i]    = one[i];
    for ( i = 0; i < n2; i++ ) ret[n1+i] = two[i];
    if ( one != NULL ) free(one);
    if ( two != NULL ) free(two);
  }

  return ret;
}


/* Read a single packet with an expected packet ID and data type. */

static garmin_data *
garmin_read_singleton ( garmin_unit *     garmin,
			garmin_pid        pid,
			garmin_datatype   type )
{
  garmin_data *     d = NULL;
  garmin_packet     p;
  link_protocol     link = garmin->protocol.link;
  garmin_pid        ppid;

  if ( garmin_read(garmin,&p) > 0 ) {
    ppid = garmin_gpid(link,garmin_packet_id(&p));
    if ( ppid == pid ) {
      d = garmin_unpack_packet(&p,type);
    } else {
      /* Expected pid but got something else. */
      printf("garmin_read_singleton: expected %d, got %d\n",pid,ppid);
    }
  } else {
    /* Failed to read the packet off the link. */
    printf("garmin_read_singleton: failed to read Pid_Records packet\n");
  }

  return d;
}


/* Read a Pid_Records, (pid)+, Pid_Xfer_Cmplt sequence. */

static garmin_data *
garmin_read_records ( garmin_unit *     garmin,
		      garmin_pid        pid,
		      garmin_datatype   type )
{
  garmin_data *     d         = NULL;
  garmin_list *     l         = NULL;
  garmin_packet     p;
  link_protocol     link      = garmin->protocol.link;
  int               done      = 0;
  int               expected  = 0;
  int               got       = 0;
  garmin_pid        ppid;

  if ( garmin_read(garmin,&p) > 0 ) {
    ppid = garmin_gpid(link,garmin_packet_id(&p));
    if ( ppid == Pid_Records ) {
      expected = get_uint16(p.packet.data);

      if ( garmin->verbose != 0 ) {
	printf("[garmin] Pid_Records indicates %d packets to follow\n",
	       expected);
      }

      /* Allocate a list for the records. */

      d = garmin_alloc_data(data_Dlist);
      l = (garmin_list *)d->data;

      /* 
	 Now we expect packets with the given packet_id and datatype, up
	 until the final packet, which is a Pid_Xfer_Cmplt.
      */

      while ( !done && garmin_read(garmin,&p) > 0 ) {
	ppid = garmin_gpid(link,garmin_packet_id(&p));
	if ( ppid == Pid_Xfer_Cmplt ) {
	  if ( got != expected ) {
	    /* Incorrect number of packets received. */
	    printf("garmin_read_records: expected %d packets, got %d\n",
		   expected,got);
	  } else if ( garmin->verbose != 0 ) {
	    printf("[garmin] all %d expected packets received\n",got);
	  }
	  done = 1;
	} else if ( ppid == pid ) {
	  garmin_list_append(l,garmin_unpack_packet(&p,type));
	  got++;
	} else {
	  /* Unexpected packet ID! */
	  done = 1;
	}
      }
    } else {
      /* Expected Pid_Records but got something else. */
      printf("garmin_read_records: expected Pid_Records, got %d\n",ppid);
    }
  } else {
    /* Failed to read the Pid_Records packet off the link. */
    printf("garmin_read_records: failed to read Pid_Records packet\n");
  }

  return d;
}


/* Read a Pid_Records, (pid1, (pid2)+)+, Pid_Xfer_Cmplt sequence. */

static garmin_data *
garmin_read_records2 ( garmin_unit *     garmin,
		       garmin_pid        pid1,
		       garmin_datatype   type1,
		       garmin_pid        pid2,
		       garmin_datatype   type2 )
{
  garmin_data *     d         = NULL;
  garmin_list *     l         = NULL;
  garmin_packet     p;
  link_protocol     link      = garmin->protocol.link;
  int               expected  = 0;
  int               got       = 0;
  int               state     = 0;
  garmin_pid        ppid;

  if ( garmin_read(garmin,&p) > 0 ) {
    ppid = garmin_gpid(link,garmin_packet_id(&p));
    if ( ppid == Pid_Records ) {
      expected = get_uint16(p.packet.data);

      if ( garmin->verbose != 0 ) {
	printf("[garmin] Pid_Records indicates %d packets to follow\n",
	       expected);
      }
      
      /* Allocate a list for the records. */

      d = garmin_alloc_data(data_Dlist);
      l = (garmin_list *)d->data;

      while ( state >= 0 && garmin_read(garmin,&p) > 0 ) {
	ppid = garmin_gpid(link,garmin_packet_id(&p));
	if ( ppid == Pid_Xfer_Cmplt ) {
	  /* transfer complete! */
	  if ( got != expected ) {
	    /* wrong number of packets received! */
	    printf("garmin_read_records2: expected %d packets, got %d\n",
		   expected,got);
	  } else if ( garmin->verbose != 0 ) {
	    printf("[garmin] all %d expected packets received\n",got);
	  }
	  break;
	}
	switch ( state ) {
	case 0:  /* want pid1 */
	  if ( ppid == pid1 ) {
	    garmin_list_append(l,garmin_unpack_packet(&p,type1));
	    state = 1;
	    got++;
	  } else {
	    state = -1;
	  }
	  break;
	case 1:  /* want pid2 */
	  if ( ppid == pid2 ) {
	    garmin_list_append(l,garmin_unpack_packet(&p,type2));
	    state = 2;
	    got++;
	  } else {
	    state = -1;
	  }
	  break;
	case 2: /* want pid2 or pid1 */
	  if ( ppid == pid1 ) {
	    garmin_list_append(l,garmin_unpack_packet(&p,type1));
	    state = 1;
	    got++;
	  } else if ( ppid == pid2 ) {
	    garmin_list_append(l,garmin_unpack_packet(&p,type2));
	    state = 2;
	    got++;
	  } else {
	    state = -1;
	  }
	  break;
	default:
	  state = -1;
	  break;
	}
      }
      if ( state < 0 ) {
	/* Unexpected packet received. */
	printf("garmin_read_records2: unexpected packet %d received\n",ppid);
      }
    } else {
      /* Expected Pid_Records but got something else. */
      printf("garmin_read_records2: expected Pid_Records, got %d\n",ppid);
    }
  } else {
    /* Failed to read the Pid_Records packet off the link. */
    printf("garmin_read_records2: failed to read Pid_Records packet\n");
  }

  return d;
}


/* Read a Pid_Records, (pid1, (pid2, pid3)+)+, Pid_Xfer_Cmplt sequence. */

static garmin_data *
garmin_read_records3 ( garmin_unit *     garmin,
		       garmin_pid        pid1,
		       garmin_datatype   type1,
		       garmin_pid        pid2,
		       garmin_datatype   type2,
		       garmin_pid        pid3,
		       garmin_datatype   type3 )
{
  garmin_data *     d         = NULL;
  garmin_list *     l         = NULL;
  garmin_packet     p;
  link_protocol     link      = garmin->protocol.link;
  int               expected  = 0;
  int               got       = 0;
  garmin_pid        ppid;
  int               state     = 0;

  if ( garmin_read(garmin,&p) > 0 ) {
    ppid = garmin_gpid(link,garmin_packet_id(&p));
    if ( ppid == Pid_Records ) {
      expected = get_uint16(p.packet.data);

      if ( garmin->verbose != 0 ) {
	printf("[garmin] Pid_Records indicates %d packets to follow\n",
	       expected);
      }

      /* Allocate a list for the records. */

      d = garmin_alloc_data(data_Dlist);
      l = (garmin_list *)d->data;

      while ( state >= 0 && garmin_read(garmin,&p) > 0 ) {
	ppid = garmin_gpid(link,garmin_packet_id(&p));
	if ( ppid == Pid_Xfer_Cmplt ) {
	  /* transfer complete! */
	  if ( got != expected ) {
	    /* wrong number of packets received! */
	    printf("garmin_read_records3: expected %d packets, got %d\n",
		   expected,got);
	  } else if ( garmin->verbose != 0 ) {
	    printf("[garmin] all %d expected packets received\n",got);
	  }
	  break;
	}
	switch ( state ) {
	case 0:  /* want pid1 */
	  if ( ppid == pid1 ) {
	    garmin_list_append(l,garmin_unpack_packet(&p,type1));
	    state = 1;
	    got++;
	  } else {
	    state = -1;
	  }
	  break;
	case 1:  /* want pid2 */
	  if ( ppid == pid2 ) {
	    garmin_list_append(l,garmin_unpack_packet(&p,type2));
	    state = 2;
	    got++;
	  } else {
	    state = -1;
	  }
	  break;
	case 2: /* want pid3 */
	  if ( ppid == pid3 ) {
	    garmin_list_append(l,garmin_unpack_packet(&p,type3));
	    state = 3;
	    got++;
	  } else {
	    state = -1;
	  }
	  break;
	case 3: /* want pid2 or pid1 */
	  if ( ppid == pid1 ) {
	    garmin_list_append(l,garmin_unpack_packet(&p,type1));
	    state = 1;
	    got++;
	  } else if ( ppid == pid2 ) {
	    garmin_list_append(l,garmin_unpack_packet(&p,type2));
	    state = 2;
	    got++;
	  } else {
	    state = -1;
	  }
	  break;
	default:
	  state = -1;
	  break;
	}
      }
      if ( state < 0 ) {
	/* Unexpected packet received. */
	printf("garmin_read_records3: unexpected packet %d received\n",ppid);
      }
    } else {
      /* Expected Pid_Records but got something else. */
      printf("garmin_read_records3: expected Pid_Records, got %d\n",ppid);
    }
  } else {
    /* Failed to read the Pid_Records packet off the link. */
    printf("garmin_read_records3: failed to read Pid_Records packet\n");
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.1  A000 - Product Data Protocol                                         */
/* 6.2  A001 - Protocol Capability Protocol                                  */
/* ------------------------------------------------------------------------- */

void
garmin_read_a000_a001 ( garmin_unit * garmin )
{
  garmin_packet          p;
  garmin_product *       r;
  garmin_extended_data * e;
  int                    done = 0;
  int                    pos;
  int                    size;
  int                    i;
  int                    j;
  uint8                  tag;
  uint16                 data;
  uint16 *               datatypes;

  /* Send the product request */
  
  garmin_packetize(&p,L000_Pid_Product_Rqst,0,NULL);
  garmin_write(garmin,&p);

  /* Read the response. */
  
  while ( !done && garmin_read(garmin,&p) > 0 ) {
    switch ( garmin_packet_id(&p) ) {
    case L000_Pid_Product_Data:
      r = &garmin->product;
      /* product ID, software version, product description, additional data. */
      r->product_id = get_uint16(p.packet.data);
      r->software_version = get_sint16(p.packet.data+2);
      pos = 4;
      if ( r->product_description != NULL ) {
	free(r->product_description);
      }
      r->product_description = get_string(&p,&pos);      
      r->additional_data = merge_strings(r->additional_data,
					 get_strings(&p,&pos));
      break;
      
    case L000_Pid_Ext_Product_Data:
      e = &garmin->extended;
      /* These strings should be ignored, but we save them anyway. */
      pos = 0;
      e->ext_data = merge_strings(e->ext_data,get_strings(&p,&pos));
      break;

    case L000_Pid_Protocol_Array:
      /* This is the A001 protocol, initiated by the device. */
      size = garmin_packet_size(&p) / 3;
      datatypes = calloc(size,sizeof(uint16));
      for ( i = 0; i < size; i++ ) {
	tag  = p.packet.data[3*i];
	data = get_uint16(p.packet.data + 3*i + 1);	
	switch ( tag ) {
	case Tag_Phys_Prot_Id:  
	  garmin->protocol.physical = data;
	  break;
	case Tag_Link_Prot_Id:
	  garmin->protocol.link = data;
	  break;
	case Tag_Appl_Prot_Id:
	  memset(datatypes,0,size * sizeof(uint16));
	  for ( j = i+1; p.packet.data[3*j] == Tag_Data_Type_Id; j++ ) {
	    datatypes[j-i-1] = get_uint16(p.packet.data + 3*j + 1);
	  }
	  garmin_assign_protocol(garmin,data,datatypes);
	  break;
	case Tag_Data_Type_Id:
	  /* Skip, since we should already have handled them. */
	default:
	  break;
	}
      }
      free(datatypes);
      done = 1;
      break;

    default:
      /* Ignore any other packets sent from the device. */
      break;
    }
  }
}


/* ------------------------------------------------------------------------- */
/* 6.4  A100 - Waypoint Transfer Protocol                                    */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a100 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Wpt) != 0 ) {
    d = garmin_read_records(garmin,
			    Pid_Wpt_Data,
			    garmin->datatype.waypoint.waypoint);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.5  A101 - Waypoint Category Transfer Protocol                           */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a101 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Wpt_Cats) != 0 ) {
    d = garmin_read_records(garmin,
			    Pid_Wpt_Cat,
			    garmin->datatype.waypoint.category);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.6.2  A200 - Route Transfer Protocol                                     */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a200 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Rte) != 0 ) {
    d = garmin_read_records2(garmin,
			     Pid_Rte_Hdr,
			     garmin->datatype.route.header,
			     Pid_Rte_Wpt_Data,
			     garmin->datatype.waypoint.waypoint);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.6.3  A201 - Route Transfer Protocol                                     */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a201 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Rte) != 0 ) {
    d = garmin_read_records3(garmin,
			     Pid_Rte_Hdr,
			     garmin->datatype.route.header,
			     Pid_Rte_Wpt_Data,
			     garmin->datatype.route.waypoint,
			     Pid_Rte_Link_Data,
			     garmin->datatype.route.link);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.7.2  A300 - Track Log Transfer Protocol                                 */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a300 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Trk) != 0 ) {
    d = garmin_read_records(garmin,
			    Pid_Trk_Data,
			    garmin->datatype.track.data);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.7.3  A301 - Track Log Transfer Protocol                                 */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a301 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Trk) != 0 ) {
    d = garmin_read_records2(garmin,
			     Pid_Trk_Hdr,
			     garmin->datatype.track.header,
			     Pid_Trk_Data,
			     garmin->datatype.track.data);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.7.4  A302 - Track Log Transfer Protocol                                 */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a302 ( garmin_unit * garmin )
{
  return garmin_read_a301(garmin);
}


/* ------------------------------------------------------------------------- */
/* 6.8  A400 - Proximity Waypoint Transfer Protocol                          */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a400 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Prx) != 0 ) {
    d = garmin_read_records(garmin,
			    Pid_Prx_Wpt_Data,
			    garmin->datatype.waypoint.proximity);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.9  A500 - Almanac Transfer Protocol                                     */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a500 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Alm) != 0 ) {
    d = garmin_read_records(garmin,
			    Pid_Almanac_Data,
			    garmin->datatype.almanac);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.10  A600 - Date and Time Initialization Protocol                        */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a600 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  d = garmin_read_singleton(garmin,
			    Pid_Date_Time_Data,
			    garmin->datatype.date_time);

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.11  A650 - FlightBook Transfer Protocol                                 */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a650 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_FlightBook_Transfer) ) {
    d = garmin_read_records(garmin,
			    Pid_FlightBook_Record,
			    garmin->datatype.flightbook);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.12  A700 - Position Initialization Protocol                             */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a700 ( garmin_unit * garmin )
{
  garmin_data * d;

  d = garmin_read_singleton(garmin,
			    Pid_Position_Data,
			    garmin->datatype.position);

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.13  A800 - PVT Protocol                                                 */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a800 ( garmin_unit * garmin )
{
  garmin_data * d;

  d = garmin_read_singleton(garmin,
			    Pid_Pvt_Data,
			    garmin->datatype.pvt);

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.14  A906 - Lap Transfer Protocol                                        */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a906 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Laps) != 0 ) {
    d = garmin_read_records(garmin,Pid_Lap,garmin->datatype.lap);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.15  A1000 - Run Transfer Protocol                                       */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a1000 ( garmin_unit * garmin )
{
  garmin_data * d  = NULL;
  garmin_list * l  = NULL;

  /* Read the runs, then the laps, then the track log. */

  if ( garmin_send_command(garmin,Cmnd_Transfer_Runs) != 0 ) {
    d = garmin_alloc_data(data_Dlist);
    l = d->data;
    garmin_list_append(l,garmin_read_records(garmin,Pid_Run,
					     garmin->datatype.run));
    garmin_list_append(l,garmin_read_a906(garmin));
    garmin_list_append(l,garmin_read_a302(garmin));
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.16  A1002 - Workout Transfer Protocol                                   */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a1002 ( garmin_unit * garmin )
{
  garmin_data * d  = NULL;
  garmin_list * l  = NULL;

  /* Read the workouts, then the workout occurrences */

  if ( garmin_send_command(garmin,Cmnd_Transfer_Workouts) != 0 ) {
    d = garmin_alloc_data(data_Dlist);
    l = d->data;
    garmin_list_append(l,
		       garmin_read_records(garmin,
					   Pid_Workout,
					   garmin->datatype.workout.workout));
    garmin_list_append(l,garmin_read_a1003(garmin));
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* --- UNDOCUMENTED ---  A1003 - Workout Occurrence Transfer Protocol        */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a1003 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  /* Read the workouts, then the workout occurrences */

  if ( garmin_send_command(garmin,Cmnd_Transfer_Workout_Occurrences) != 0 ) {
    d = garmin_read_records(garmin,
			    Pid_Workout_Occurrence,
			    garmin->datatype.workout.occurrence);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.17  A1004 - Fitness User Profile Transfer Protocol                      */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a1004 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Fitness_User_Profile) != 0 ) {
    d = garmin_read_singleton(garmin,
			      Pid_Fitness_User_Profile,
			      garmin->datatype.fitness);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.18  A1005 - Workout Limits Transfer Protocol                            */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a1005 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Workout_Limits) != 0 ) {
    d = garmin_read_singleton(garmin,
			      Pid_Workout_Limits,
			      garmin->datatype.workout.limits);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.19  A1006 - Course Transfer Protocol                                    */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a1006 ( garmin_unit * garmin )
{
  garmin_data * d  = NULL;
  garmin_list * l  = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Courses) != 0 ) {
    d = garmin_alloc_data(data_Dlist);
    l = d->data;
    garmin_list_append(l,garmin_read_records(garmin,
					     Pid_Course,
					     garmin->datatype.course.course));
    garmin_list_append(l,garmin_read_a1007(garmin));
    garmin_list_append(l,garmin_read_a1012(garmin));
    garmin_list_append(l,garmin_read_a1008(garmin));
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* --- UNDOCUMENTED ---  A1007 - Course Lap Transfer Protocol                */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a1007 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Course_Laps) != 0 ) {
    d = garmin_read_records(garmin,
			    Pid_Course_Lap,
			    (garmin->datatype.course.lap != data_Dnil) ?
			    garmin->datatype.course.lap :
			    garmin->datatype.lap);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* --- UNDOCUMENTED ---  A1008 - Course Point Transfer Protocol              */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a1008 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Course_Points) != 0 ) {
    d = garmin_read_records(garmin,
			    Pid_Course_Point,
			    garmin->datatype.course.point);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* 6.20  A1009 - Course Limits Transfer Protocol                             */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a1009 ( garmin_unit * garmin )
{
  garmin_data * d = NULL;

  if ( garmin_send_command(garmin,Cmnd_Transfer_Course_Limits) != 0 ) {
    d = garmin_read_singleton(garmin,
			      Pid_Course_Limits,
			      garmin->datatype.course.limits);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* --- UNDOCUMENTED ---  A1012 - Course Track Transfer Protocol              */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_a1012 ( garmin_unit * garmin )
{
  garmin_datatype  header;
  garmin_datatype  data;
  garmin_data *    d = NULL;
  
  if ( garmin_send_command(garmin,Cmnd_Transfer_Course_Tracks) != 0 ) {

    if ( garmin->datatype.course.track.header != data_Dnil ) {
      header = garmin->datatype.course.track.header;
    } else {
      header = garmin->datatype.track.header;
    }

    if ( garmin->datatype.course.track.data != data_Dnil ) {
      data = garmin->datatype.course.track.data;
    } else {
      data = garmin->datatype.track.data;
    }

    d = garmin_read_records2(garmin,
			     Pid_Course_Trk_Hdr,
			     header,
			     Pid_Course_Trk_Data,
			     data);
  }

  return d;
}


/* ------------------------------------------------------------------------- */
/* Get data from the Garmin unit via a particular top-level protocol         */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_read_via ( garmin_unit * garmin, appl_protocol protocol )
{
  garmin_data * data = NULL;

#define CASE_PROTOCOL(x)                                                      \
  case appl_A##x:                                                             \
    if ( garmin->verbose != 0 ) {                                             \
      printf("[garmin] -> garmin_read_a" #x "\n");                            \
    }                                                                         \
    data = garmin_read_a##x(garmin);                                          \
    if ( garmin->verbose != 0 ) {                                             \
      printf("[garmin] <- garmin_read_a" #x "\n");                            \
    }                                                                         \
    break

  switch ( protocol ) {
  CASE_PROTOCOL(100);   /* waypoints */
  CASE_PROTOCOL(101);   /* waypoint categories */
  CASE_PROTOCOL(200);   /* routes */
  CASE_PROTOCOL(201);   /* routes */
  CASE_PROTOCOL(300);   /* track log */
  CASE_PROTOCOL(301);   /* track log */
  CASE_PROTOCOL(302);   /* track log */
  CASE_PROTOCOL(400);   /* proximity waypoints */
  CASE_PROTOCOL(500);   /* almanac */
  CASE_PROTOCOL(650);   /* flightbook */
  CASE_PROTOCOL(1000);  /* runs */
  CASE_PROTOCOL(1002);  /* workouts */
  CASE_PROTOCOL(1004);  /* fitness user profile */
  CASE_PROTOCOL(1005);  /* workout limits */
  CASE_PROTOCOL(1006);  /* courses */
  CASE_PROTOCOL(1009);  /* course limits */
  default:
    /* invalid top-level read protocol */
    break;
  }

  return data;
}


/* ------------------------------------------------------------------------- */
/* Get data from the Garmin unit                                             */
/* ------------------------------------------------------------------------- */

garmin_data *
garmin_get ( garmin_unit * garmin, garmin_get_type what )
{
  garmin_data * data = NULL;

#define CASE_WHAT(x,y) \
  case GET_##x: data = garmin_read_via(garmin,garmin->protocol.y); break

  switch ( what ) {
  CASE_WHAT(WAYPOINTS,waypoint.waypoint);
  CASE_WHAT(WAYPOINT_CATEGORIES,waypoint.category);
  CASE_WHAT(ROUTES,route);
  CASE_WHAT(TRACKLOG,track);
  CASE_WHAT(PROXIMITY_WAYPOINTS,waypoint.proximity);
  CASE_WHAT(ALMANAC,almanac);
  CASE_WHAT(FLIGHTBOOK,flightbook);
  CASE_WHAT(RUNS,run);
  CASE_WHAT(WORKOUTS,workout.workout);
  CASE_WHAT(FITNESS_USER_PROFILE,fitness);
  CASE_WHAT(WORKOUT_LIMITS,workout.limits);
  CASE_WHAT(COURSES,course.course);
  CASE_WHAT(COURSE_LIMITS,course.limits);
  default:
    /* invalid garmin_get_type */
    break;
  }

  return data;
}


/* Initialize a connection with a Garmin unit. */

int
garmin_init ( garmin_unit * garmin, int verbose )
{
  memset(garmin,0,sizeof(garmin_unit));
  garmin->verbose = verbose;

  if ( garmin_open(garmin) != 0 ) {
    garmin_start_session(garmin);
    garmin_read_a000_a001(garmin);
    return 1;
  } else {
    return 0;
  }
}

