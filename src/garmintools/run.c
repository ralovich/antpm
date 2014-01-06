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
#include <time.h>
#include <string.h>
#include <errno.h>
#include "garmin.h"


int
get_run_track_lap_info ( garmin_data * run,
			 uint32 *      track_index,
			 uint32 *      first_lap_index,
			 uint32 *      last_lap_index )
{
  D1000 * d1000;
  D1009 * d1009;
  D1010 * d1010;

  int ok = 1;

  switch ( run->type ) {
  case data_D1000:
    d1000            = run->data;
    *track_index     = d1000->track_index;
    *first_lap_index = d1000->first_lap_index;
    *last_lap_index  = d1000->last_lap_index;
    break;
  case data_D1009:
    d1009            = run->data;
    *track_index     = d1009->track_index;
    *first_lap_index = d1009->first_lap_index;
    *last_lap_index  = d1009->last_lap_index;
    break;
  case data_D1010:
    d1010            = run->data;
    *track_index     = d1010->track_index;
    *first_lap_index = d1010->first_lap_index;
    *last_lap_index  = d1010->last_lap_index;
    break;
  default:
    printf("get_run_track_lap_info: run type %d invalid!\n",run->type);
    ok = 0;
    break;
  }

  return ok;
}


int
get_lap_index ( garmin_data * lap, uint32 * lap_index )
{
  D1001 * d1001;
  D1011 * d1011;
  D1015 * d1015;

  int ok = 1;

  switch ( lap->type ) {
  case data_D1001:
    d1001      = lap->data;
    *lap_index = d1001->index;
    break;
  case data_D1011:
    d1011      = lap->data;
    *lap_index = d1011->index;
    break;
  case data_D1015:
    d1015      = lap->data;
    *lap_index = d1015->index;
    break;
  default:
    printf("get_lap_index: lap type %d invalid!\n",lap->type);
    ok = 0;
    break;
  }

  return ok;
}


int
get_lap_start_time ( garmin_data * lap, time_type * start_time )
{
  D1001 * d1001;
  D1011 * d1011;
  D1015 * d1015;

  int ok = 1;

  switch ( lap->type ) {
  case data_D1001:
    d1001       = lap->data;
    *start_time = d1001->start_time + TIME_OFFSET;
    break;
  case data_D1011:
    d1011       = lap->data;
    *start_time = d1011->start_time + TIME_OFFSET;
    break;
  case data_D1015:
    d1015       = lap->data;
    *start_time = d1015->start_time + TIME_OFFSET;
    break;
  default:
    printf("get_lap_start_time: lap type %d invalid!\n",lap->type);
    ok = 0;
    break;
  }

  return ok;
}


garmin_data *
get_track ( garmin_list * points, uint32 trk_index )
{
  garmin_list_node * n;
  garmin_data *      track = NULL;
  D311 *             d311;
  int                done = 0;

  /* Look for a data_D311 with an index that matches. */

  for ( n = points->head; n != NULL; n = n->next ) {    
    if ( n->data != NULL ) {
      switch ( n->data->type ) {
      case data_D311:
	if ( track == NULL ) {
	  d311 = n->data->data;
	  if ( d311->index == trk_index ) {
	    track = garmin_alloc_data(data_Dlist);
	    garmin_list_append(track->data,n->data);
	  }
	} else {
	  /* We've reached the end of the track */
	  done = 1;
	}
	break;
      case data_D300:
      case data_D301:
      case data_D302:
      case data_D303:
      case data_D304:
	if ( track != NULL ) {
	  garmin_list_append(track->data,n->data);
	}
	break;
      default:
	printf("get_track: point type %d invalid!\n",n->data->type);
	break;
      }
    }

    if ( done != 0 ) break;
  }

  return track;
}


void
garmin_save_runs ( garmin_unit * garmin )
{
  garmin_data *       data;
  garmin_data *       data0;
  garmin_data *       data1;
  garmin_data *       data2;
  garmin_data *       rlaps;
  garmin_data *       rtracks;
  garmin_list *       runs   = NULL;
  garmin_list *       laps   = NULL;
  garmin_list *       tracks = NULL;
  garmin_data *       rlist;
  garmin_list_node *  n;
  garmin_list_node *  m;
  uint32              trk;
  uint32              f_lap;
  uint32              l_lap;
  uint32              l_idx;
  time_type           start;
  time_t              start_time;
  char                filename[BUFSIZ];
  char *              filedir = NULL;
  char                path[PATH_MAX];
  char                filepath[BUFSIZ];
  struct tm *         tbuf;

  if ( (filedir = getenv("GARMIN_SAVE_RUNS")) != NULL ) {
    filedir = realpath(filedir,path);
    if ( filedir == NULL ) {
      printf("GARMIN_SAVE_RUNS: %s: %s\n",
	     getenv("GARMIN_SAVE_RUNS"),strerror(errno));
    }
  }
  if ( filedir == NULL ) {
    filedir = getcwd(path,sizeof(path));
  }

  printf("Extracting data from Garmin %s\n",
	 garmin->product.product_description);
  printf("Files will be saved in '%s'\n",filedir);

  if ( (data = garmin_get(garmin,GET_RUNS)) != NULL ) {

    /* 
       We should have a list with three elements:

       1) The runs (which identify the track and lap indices)
       2) The laps (which are related to the runs)
       3) The tracks (which are related to the runs)
    */

    data0 = garmin_list_data(data,0);
    data1 = garmin_list_data(data,1);
    data2 = garmin_list_data(data,2);

    if ( data0 != NULL && (runs   = data0->data) != NULL &&
	 data1 != NULL && (laps   = data1->data) != NULL &&
	 data2 != NULL && (tracks = data2->data) != NULL ) {

      /* Print some debug output if requested. */

      if ( garmin->verbose != 0 ) {
	for ( m = laps->head; m != NULL; m = m->next ) {
	  if ( get_lap_index(m->data,&l_idx) != 0 ) {
	    printf("[garmin] lap: index [%d]\n",l_idx);
	  } else {
	    printf("[garmin] lap: index [??]\n");
	  }
	}
      }
      
      /* For each run, get its laps and track points. */

      for ( n = runs->head; n != NULL; n = n->next ) {
	if ( get_run_track_lap_info(n->data,&trk,&f_lap,&l_lap) != 0 ) {

	  if ( garmin->verbose != 0 ) {
	    printf("[garmin] run: track [%d], laps [%d:%d]\n",trk,f_lap,l_lap);
	  }

	  start = 0;

	  /* Get the laps. */

	  rlaps = garmin_alloc_data(data_Dlist);
	  for ( m = laps->head; m != NULL; m = m->next ) {
	    if ( get_lap_index(m->data,&l_idx) != 0 ) {
	      if ( l_idx >= f_lap && l_idx <= l_lap ) {
		if ( garmin->verbose != 0 ) {
		  printf("[garmin] lap [%d] falls within laps [%d:%d]\n",
			 l_idx,f_lap,l_lap);
		}

		garmin_list_append(rlaps->data,m->data);

		if ( l_idx == f_lap ) {
		  get_lap_start_time(m->data,&start);
		  if ( garmin->verbose != 0 ) {
		    printf("[garmin] first lap [%d] has start time [%d]\n",
			   l_idx,(int)start);
		  }
		}
	      }
	    }
	  }

	  /* Get the track points. */
	  
	  rtracks = get_track(tracks,trk);

	  /* Now make a three-element list for this run. */

	  rlist = garmin_alloc_data(data_Dlist);
	  garmin_list_append(rlist->data,n->data);
	  garmin_list_append(rlist->data,rlaps);
	  garmin_list_append(rlist->data,rtracks);

	  /* 
	     Determine the filename based on the start time of the first lap. 
	  */

	  if ( (start_time = start) != 0 ) {
	    tbuf = localtime(&start_time);
	    snprintf(filepath,sizeof(filepath)-1,"%s/%d/%02d",
		    filedir,tbuf->tm_year+1900,tbuf->tm_mon+1);
	    strftime(filename,sizeof(filename),"%Y%m%dT%H%M%S.gmn",tbuf);

	    /* Save rlist to the file. */

	    if ( garmin_save(rlist,filename,filepath) != 0 ) {
	      printf("Wrote:   %s/%s\n",filepath,filename);
	    } else {
	      printf("Skipped: %s/%s\n",filepath,filename);
	    }
	  } else {
	    printf("Start time of first lap not found!\n");
	  }

	  /* Free the temporary lists we were using. */

	  if ( rlaps != NULL ) {
	    garmin_free_list_only(rlaps->data);
	    free(rlaps);
	  }

	  if ( rtracks != NULL ) {
	    garmin_free_list_only(rtracks->data);
	    free(rtracks);
	  }

	  if ( rlist != NULL ) {
	    garmin_free_list_only(rlist->data);
	    free(rlist);
	  }
	}
      }
    } else {
      if ( data0 == NULL ) {
	printf("Toplevel data missing element 0 (runs)\n");
      } else if ( runs == NULL ) {
	printf("No runs extracted!\n");
      }
      if ( data1 == NULL ) {
	printf("Toplevel data missing element 1 (laps)\n");
      } else if ( laps == NULL ) {
	printf("No laps extracted!\n");
      }
      if ( data2 == NULL ) {
	printf("Toplevel data missing element 2 (tracks)\n");
      } else if ( tracks == NULL ) {
	printf("No tracks extracted!\n");
      }
    }
    garmin_free_data(data);
  } else {
    printf("Unable to extract any data!\n");
  }
}
