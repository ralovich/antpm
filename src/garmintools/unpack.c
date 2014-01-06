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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "garmin.h"


#define GETU16(x) do { x = get_uint16(*pos);  *pos += 2; } while ( 0 )
#define GETS16(x) do { x = get_sint16(*pos);  *pos += 2; } while ( 0 )
#define GETU32(x) do { x = get_uint32(*pos);  *pos += 4; } while ( 0 )
#define GETS32(x) do { x = get_sint32(*pos);  *pos += 4; } while ( 0 )
#define GETF32(x) do { x = get_float32(*pos); *pos += 4; } while ( 0 )
#define GETF64(x) do { x = get_float64(*pos); *pos += 8; } while ( 0 )
#define GETPOS(x) do { GETS32((x).lat); GETS32((x).lon); } while ( 0 )
#define GETRPT(x) do { GETF64((x).lat); GETF64((x).lon); } while ( 0 )
#define GETVST(x) x = get_vstring(pos)
#define GETU8(x)  x = *(*pos)++
#define SKIP(x)   do { memset(*pos,0,x); *pos += x; }      while ( 0 )

#define GETSTR(x)                                                      \
  do {                                                                 \
    memcpy(x,*pos,sizeof(x)-1); x[sizeof(x)-1] = 0; *pos += sizeof(x); \
  } while ( 0 )


/* --------------------------------------------------------------------------*/
/* 7.4.1   D100                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d100 ( D100 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETPOS(wpt->posn);
  SKIP(4);
  GETSTR(wpt->cmnt);
}


/* --------------------------------------------------------------------------*/
/* 7.4.2   D101                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d101 ( D101 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETPOS(wpt->posn);
  SKIP(4);
  GETSTR(wpt->cmnt);
  GETF32(wpt->dst);
  GETU8(wpt->smbl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.3   D102                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d102 ( D102 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETPOS(wpt->posn);
  SKIP(4);
  GETSTR(wpt->cmnt);
  GETF32(wpt->dst);
  GETU16(wpt->smbl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.4   D103                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d103 ( D103 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETPOS(wpt->posn);
  SKIP(4);
  GETSTR(wpt->cmnt);
  GETU8(wpt->smbl);
  GETU8(wpt->dspl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.5   D104                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d104 ( D104 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETPOS(wpt->posn);
  SKIP(4);
  GETSTR(wpt->cmnt);
  GETF32(wpt->dst);
  GETU16(wpt->smbl);
  GETU8(wpt->dspl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.6   D105                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d105 ( D105 * wpt, uint8 ** pos )
{
  GETPOS(wpt->posn);
  GETU16(wpt->smbl);
  GETVST(wpt->wpt_ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.7   D106                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d106 ( D106 * wpt, uint8 ** pos )
{
  GETU8(wpt->wpt_class);
  GETSTR(wpt->subclass);
  GETPOS(wpt->posn);
  GETU16(wpt->smbl);
  GETVST(wpt->wpt_ident);
  GETVST(wpt->lnk_ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.8   D107                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d107 ( D107 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETPOS(wpt->posn);
  SKIP(4);
  GETSTR(wpt->cmnt);
  GETU8(wpt->smbl);
  GETU8(wpt->dspl);
  GETF32(wpt->dst);
  GETU8(wpt->color);
}


/* --------------------------------------------------------------------------*/
/* 7.4.9   D108                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d108 ( D108 * wpt, uint8 ** pos )
{
  GETU8(wpt->wpt_class);
  GETU8(wpt->color);
  GETU8(wpt->dspl);
  GETU8(wpt->attr);
  GETU16(wpt->smbl);
  GETSTR(wpt->subclass);
  GETPOS(wpt->posn);
  GETF32(wpt->alt);
  GETF32(wpt->dpth);
  GETF32(wpt->dist);
  GETSTR(wpt->state);
  GETSTR(wpt->cc);
  GETVST(wpt->ident);
  GETVST(wpt->comment);
  GETVST(wpt->facility);
  GETVST(wpt->city);
  GETVST(wpt->addr);
  GETVST(wpt->cross_road);
}


/* --------------------------------------------------------------------------*/
/* 7.4.10  D109                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d109 ( D109 * wpt, uint8 ** pos )
{
  GETU8(wpt->dtyp);
  GETU8(wpt->wpt_class);
  GETU8(wpt->dspl_color);
  GETU8(wpt->attr);
  GETU16(wpt->smbl);
  GETSTR(wpt->subclass);
  GETPOS(wpt->posn);
  GETF32(wpt->alt);
  GETF32(wpt->dpth);
  GETF32(wpt->dist);
  GETSTR(wpt->state);
  GETSTR(wpt->cc);
  GETU32(wpt->ete);
  GETVST(wpt->ident);
  GETVST(wpt->comment);
  GETVST(wpt->facility);
  GETVST(wpt->city);
  GETVST(wpt->addr);
  GETVST(wpt->cross_road);
}


/* --------------------------------------------------------------------------*/
/* 7.4.11  D110                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d110 ( D110 * wpt, uint8 ** pos )
{
  GETU8(wpt->dtyp);
  GETU8(wpt->wpt_class);
  GETU8(wpt->dspl_color);
  GETU8(wpt->attr);
  GETU16(wpt->smbl);
  GETSTR(wpt->subclass);
  GETPOS(wpt->posn);
  GETF32(wpt->alt);
  GETF32(wpt->dpth);
  GETF32(wpt->dist);
  GETSTR(wpt->state);
  GETSTR(wpt->cc);
  GETU32(wpt->ete);
  GETF32(wpt->temp);
  GETU32(wpt->time);
  GETU16(wpt->wpt_cat);
  GETVST(wpt->ident);
  GETVST(wpt->comment);
  GETVST(wpt->facility);
  GETVST(wpt->city);
  GETVST(wpt->addr);
  GETVST(wpt->cross_road);
}


/* --------------------------------------------------------------------------*/
/* 7.4.12  D120                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d120 ( D120 * cat, uint8 ** pos )
{
  GETSTR(cat->name);
}


/* --------------------------------------------------------------------------*/
/* 7.4.13  D150                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d150 ( D150 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETSTR(wpt->cc);
  GETU8(wpt->wpt_class);
  GETPOS(wpt->posn);
  GETS16(wpt->alt);
  GETSTR(wpt->city);
  GETSTR(wpt->state);
  GETSTR(wpt->name);
  GETSTR(wpt->cmnt);
}


/* --------------------------------------------------------------------------*/
/* 7.4.14  D151                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d151 ( D151 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETPOS(wpt->posn);
  SKIP(4);
  GETSTR(wpt->cmnt);
  GETF32(wpt->dst);
  GETSTR(wpt->name);
  GETSTR(wpt->city);
  GETSTR(wpt->state);
  GETS16(wpt->alt);
  GETSTR(wpt->cc);
  SKIP(1);
  GETU8(wpt->wpt_class);
}


/* --------------------------------------------------------------------------*/
/* 7.4.15  D152                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d152 ( D152 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETPOS(wpt->posn);
  SKIP(4);
  GETSTR(wpt->cmnt);
  GETF32(wpt->dst);
  GETSTR(wpt->name);
  GETSTR(wpt->city);
  GETSTR(wpt->state);
  GETS16(wpt->alt);
  GETSTR(wpt->cc);
  SKIP(1);
  GETU8(wpt->wpt_class);
}


/* --------------------------------------------------------------------------*/
/* 7.4.16  D154                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d154 ( D154 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETPOS(wpt->posn);
  SKIP(4);
  GETSTR(wpt->cmnt);
  GETF32(wpt->dst);
  GETSTR(wpt->name);
  GETSTR(wpt->city);
  GETSTR(wpt->state);
  GETS16(wpt->alt);
  GETSTR(wpt->cc);
  SKIP(1);
  GETU8(wpt->wpt_class);
  GETU16(wpt->smbl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.17  D155                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d155 ( D155 * wpt, uint8 ** pos )
{
  GETSTR(wpt->ident);
  GETPOS(wpt->posn);
  SKIP(4);
  GETSTR(wpt->cmnt);
  GETF32(wpt->dst);
  GETSTR(wpt->name);
  GETSTR(wpt->city);
  GETSTR(wpt->state);
  GETS16(wpt->alt);
  GETSTR(wpt->cc);
  SKIP(1);
  GETU8(wpt->wpt_class);
  GETU16(wpt->smbl);  
  GETU8(wpt->dspl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.18  D200                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d200 ( D200 * hdr, uint8 ** pos )
{
  GETU8(*hdr);
}


/* --------------------------------------------------------------------------*/
/* 7.4.19  D201                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d201 ( D201 * hdr, uint8 ** pos )
{
  GETU8(hdr->nmbr);
  GETSTR(hdr->cmnt);
}


/* --------------------------------------------------------------------------*/
/* 7.4.20  D202                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d202 ( D202 * hdr, uint8 ** pos )
{
  GETVST(hdr->rte_ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.21  D210                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d210 ( D210 * link, uint8 ** pos )
{
  GETU16(link->link_class);
  GETSTR(link->subclass);
  GETVST(link->ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.22  D300                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d300 ( D300 * point, uint8 ** pos )
{
  GETPOS(point->posn);
  GETU32(point->time);
  GETU8(point->new_trk);
}


/* --------------------------------------------------------------------------*/
/* 7.4.23  D301                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d301 ( D301 * point, uint8 ** pos )
{ 
  GETPOS(point->posn);
  GETU32(point->time);
  GETF32(point->alt);
  GETF32(point->dpth);
  GETU8(point->new_trk);
}


/* --------------------------------------------------------------------------*/
/* 7.4.24  D302                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d302 ( D302 * point, uint8 ** pos )
{
  GETPOS(point->posn);
  GETU32(point->time);
  GETF32(point->alt);
  GETF32(point->dpth);
  GETF32(point->temp);
  GETU8(point->new_trk);
}


/* --------------------------------------------------------------------------*/
/* 7.4.25  D303                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d303 ( D303 * point, uint8 ** pos )
{
  GETPOS(point->posn);
  GETU32(point->time);
  GETF32(point->alt);
  GETU8(point->heart_rate);
}


/* --------------------------------------------------------------------------*/
/* 7.4.26  D304                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d304 ( D304 * point, uint8 ** pos )
{
  GETPOS(point->posn);
  GETU32(point->time);
  GETF32(point->alt);
  GETF32(point->distance);
  GETU8(point->heart_rate);
  GETU8(point->cadence);
  GETU8(point->sensor);
}


/* --------------------------------------------------------------------------*/
/* 7.4.27  D310                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d310 ( D310 * hdr, uint8 ** pos )
{
  GETU8(hdr->dspl);
  GETU8(hdr->color);
  GETVST(hdr->trk_ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.28  D311                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d311 ( D311 * hdr, uint8 ** pos )
{
  GETU16(hdr->index);
}


/* --------------------------------------------------------------------------*/
/* 7.4.29  D312                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d312 ( D312 * hdr, uint8 ** pos )
{
  GETU8(hdr->dspl);
  GETU8(hdr->color);
  GETVST(hdr->trk_ident);
}


/* ------------------------------------------------------------------------- */
/* 7.4.30  D400                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d400 ( D400 * prx, uint8 ** pos )
{
  garmin_unpack_d100(&prx->wpt,pos);
  SKIP(sizeof(D100));
  GETF32(prx->dst);
}


/* ------------------------------------------------------------------------- */
/* 7.4.31  D403                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d403 ( D403 * prx, uint8 ** pos )
{
  garmin_unpack_d103(&prx->wpt,pos);
  SKIP(sizeof(D103));
  GETF32(prx->dst);
}


/* ------------------------------------------------------------------------- */
/* 7.4.32  D450                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d450 ( D450 * prx, uint8 ** pos )
{
  GETU32(prx->idx);
  garmin_unpack_d150(&prx->wpt,pos);
  SKIP(sizeof(D150));
  GETF32(prx->dst);
}


/* ------------------------------------------------------------------------- */
/* 7.4.33  D500                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d500 ( D500 * alm, uint8 ** pos )
{
  GETU16(alm->wn);
  GETF32(alm->toa);
  GETF32(alm->af0);
  GETF32(alm->af1);
  GETF32(alm->e);
  GETF32(alm->sqrta);
  GETF32(alm->m0);
  GETF32(alm->w);
  GETF32(alm->omg0);
  GETF32(alm->odot);
  GETF32(alm->i);
}


/* ------------------------------------------------------------------------- */
/* 7.4.34  D501                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d501 ( D501 * alm, uint8 ** pos )
{
  GETU16(alm->wn);
  GETF32(alm->toa);
  GETF32(alm->af0);
  GETF32(alm->af1);
  GETF32(alm->e);
  GETF32(alm->sqrta);
  GETF32(alm->m0);
  GETF32(alm->w);
  GETF32(alm->omg0);
  GETF32(alm->odot);
  GETF32(alm->i);
  GETU8(alm->hlth);
}


/* ------------------------------------------------------------------------- */
/* 7.4.35  D550                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d550 ( D550 * alm, uint8 ** pos )
{
  GETU8(alm->svid);
  GETU16(alm->wn);
  GETF32(alm->toa);
  GETF32(alm->af0);
  GETF32(alm->af1);
  GETF32(alm->e);
  GETF32(alm->sqrta);
  GETF32(alm->m0);
  GETF32(alm->w);
  GETF32(alm->omg0);
  GETF32(alm->odot);
  GETF32(alm->i);
}


/* ------------------------------------------------------------------------- */
/* 7.4.36  D551                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d551 ( D551 * alm, uint8 ** pos )
{
  GETU8(alm->svid);
  GETU16(alm->wn);
  GETF32(alm->toa);
  GETF32(alm->af0);
  GETF32(alm->af1);
  GETF32(alm->e);
  GETF32(alm->sqrta);
  GETF32(alm->m0);
  GETF32(alm->w);
  GETF32(alm->omg0);
  GETF32(alm->odot);
  GETF32(alm->i);
  GETU8(alm->hlth);
}


/* ------------------------------------------------------------------------- */
/* 7.4.37  D600                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d600 ( D600 * dt, uint8 ** pos )
{
  GETU8(dt->month);
  GETU8(dt->day);
  GETU16(dt->year);
  GETU16(dt->hour);
  GETU8(dt->minute);
  GETU8(dt->second);
}


/* ------------------------------------------------------------------------- */
/* 7.4.38  D650                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d650 ( D650 * fbr, uint8 ** pos )
{
  GETU32(fbr->takeoff_time);
  GETU32(fbr->landing_time);
  GETPOS(fbr->takeoff_posn);
  GETPOS(fbr->landing_posn);
  GETU32(fbr->night_time);
  GETU32(fbr->num_landings);
  GETF32(fbr->max_speed);
  GETF32(fbr->max_alt);
  GETF32(fbr->distance);
  GETU8(fbr->cross_country_flag);
  GETVST(fbr->departure_name);
  GETVST(fbr->departure_ident);
  GETVST(fbr->arrival_name);
  GETVST(fbr->arrival_ident);
  GETVST(fbr->ac_id);
}


/* ------------------------------------------------------------------------- */
/* 7.4.39  D700                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d700 ( D700 * pt, uint8 ** pos )
{
  GETRPT(*pt);
}


/* ------------------------------------------------------------------------- */
/* 7.4.40  D800                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_unpack_d800 ( D800 * pvt, uint8 ** pos )
{
  GETF32(pvt->alt);
  GETF32(pvt->epe);
  GETF32(pvt->eph);
  GETF32(pvt->epv);
  GETU16(pvt->fix);
  GETF64(pvt->tow);
  GETRPT(pvt->posn);
  GETF32(pvt->east);
  GETF32(pvt->north);
  GETF32(pvt->up);
  GETF32(pvt->msl_hght);
  GETS16(pvt->leap_scnds);
  GETU32(pvt->wn_days);
}


/* --------------------------------------------------------------------------*/
/* 7.4.41  D906                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d906 ( D906 * lap, uint8 ** pos )
{
  GETU32(lap->start_time);
  GETU32(lap->total_time);
  GETF32(lap->total_distance);
  GETPOS(lap->begin);
  GETPOS(lap->end);
  GETU16(lap->calories);
  GETU8(lap->track_index);
}


/* --------------------------------------------------------------------------*/
/* 7.4.42  D1000                                                             */
/* --------------------------------------------------------------------------*/


static void garmin_unpack_d1002 ( D1002 * wkt, uint8 ** pos );


static void
garmin_unpack_d1000 ( D1000 * run, uint8 ** pos )
{
  GETU32(run->track_index);
  GETU32(run->first_lap_index);
  GETU32(run->last_lap_index);
  GETU8(run->sport_type);
  GETU8(run->program_type);
  SKIP(2);
  GETU32(run->virtual_partner.time);
  GETF32(run->virtual_partner.distance);
  garmin_unpack_d1002(&run->workout,pos);
}


/* --------------------------------------------------------------------------*/
/* 7.4.43  D1001                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1001 ( D1001 * lap, uint8 ** pos )
{
  GETU32(lap->index);
  GETU32(lap->start_time);
  GETU32(lap->total_time);
  GETF32(lap->total_dist);
  GETF32(lap->max_speed);
  GETPOS(lap->begin);
  GETPOS(lap->end);
  GETU16(lap->calories);
  GETU8(lap->avg_heart_rate);
  GETU8(lap->max_heart_rate);
  GETU8(lap->intensity);
}


/* --------------------------------------------------------------------------*/
/* 7.4.44  D1002                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1002 ( D1002 * wkt, uint8 ** pos )
{
  int i;

  GETU32(wkt->num_valid_steps);
  for ( i = 0; i < 20; i++ ) {
    GETSTR(wkt->steps[i].custom_name);
    GETF32(wkt->steps[i].target_custom_zone_low);
    GETF32(wkt->steps[i].target_custom_zone_high);
    GETU16(wkt->steps[i].duration_value);
    GETU8(wkt->steps[i].intensity);
    GETU8(wkt->steps[i].duration_type);
    GETU8(wkt->steps[i].target_type);
    GETU8(wkt->steps[i].target_value);
    SKIP(2);
  }
  GETSTR(wkt->name);
  GETU8(wkt->sport_type);
}


/* --------------------------------------------------------------------------*/
/* 7.4.45  D1003                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1003 ( D1003 * occ, uint8 ** pos )
{
  GETSTR(occ->workout_name);
  GETU32(occ->day);
}


/* --------------------------------------------------------------------------*/
/* 7.4.46  D1004                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1004 ( D1004 * prof, uint8 ** pos )
{
  int i;
  int j;

  for ( i = 0; i < 3; i++ ) {
    for ( j = 0; j < 5; j++ ) {
      GETU8(prof->activities[i].heart_rate_zones[j].low_heart_rate);
      GETU8(prof->activities[i].heart_rate_zones[j].high_heart_rate);
      SKIP(2);
    }
    for ( j = 0; j < 10; j++ ) {
      GETF32(prof->activities[i].speed_zones[j].low_speed);
      GETF32(prof->activities[i].speed_zones[j].high_speed);
      GETSTR(prof->activities[i].speed_zones[j].name);
    }
    GETF32(prof->activities[i].gear_weight);
    GETU8(prof->activities[i].max_heart_rate);
    SKIP(3);
  }
  GETF32(prof->weight);
  GETU16(prof->birth_year);
  GETU8(prof->birth_month);
  GETU8(prof->birth_day);
  GETU8(prof->gender);
}


/* --------------------------------------------------------------------------*/
/* 7.4.47  D1005                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1005 ( D1005 * limits, uint8 ** pos )
{
  GETU32(limits->max_workouts);
  GETU32(limits->max_unscheduled_workouts);
  GETU32(limits->max_occurrences);
}


/* --------------------------------------------------------------------------*/
/* 7.4.48  D1006                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1006 ( D1006 * course, uint8 ** pos )
{
  GETU16(course->index);
  SKIP(2);
  GETSTR(course->course_name);
  GETU16(course->track_index);
}


/* --------------------------------------------------------------------------*/
/* 7.4.49  D1007                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1007 ( D1007 * lap, uint8 ** pos )
{
  GETU16(lap->course_index);
  GETU16(lap->lap_index);
  GETU32(lap->total_time);
  GETF32(lap->total_dist);
  GETPOS(lap->begin);
  GETPOS(lap->end);
  GETU8(lap->avg_heart_rate);
  GETU8(lap->max_heart_rate);
  GETU8(lap->intensity);
  GETU8(lap->avg_cadence);
}


/* --------------------------------------------------------------------------*/
/* 7.4.50  D1008                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1008 ( D1008 * wkt, uint8 ** pos )
{
  garmin_unpack_d1002((D1002 *)wkt,pos);
}


/* --------------------------------------------------------------------------*/
/* 7.4.51  D1009                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1009 ( D1009 * run, uint8 ** pos )
{
  GETU16(run->track_index);
  GETU16(run->first_lap_index);
  GETU16(run->last_lap_index);
  GETU8(run->sport_type);
  GETU8(run->program_type);
  GETU8(run->multisport);
  SKIP(3);
  GETU32(run->quick_workout.time);
  GETF32(run->quick_workout.distance);
  garmin_unpack_d1008(&run->workout,pos);  
}


/* --------------------------------------------------------------------------*/
/* 7.4.52  D1010                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1010 ( D1010 * run, uint8 ** pos )
{
  GETU32(run->track_index);
  GETU32(run->first_lap_index);
  GETU32(run->last_lap_index);
  GETU8(run->sport_type);
  GETU8(run->program_type);
  GETU8(run->multisport);
  SKIP(1);
  GETU32(run->virtual_partner.time);
  GETF32(run->virtual_partner.distance);
  garmin_unpack_d1002(&run->workout,pos);
}


/* --------------------------------------------------------------------------*/
/* 7.4.53  D1011                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1011 ( D1011 * lap, uint8 ** pos )
{
  GETU16(lap->index);
  SKIP(2);
  GETU32(lap->start_time);
  GETU32(lap->total_time);
  GETF32(lap->total_dist);
  GETF32(lap->max_speed);
  GETPOS(lap->begin);
  GETPOS(lap->end);
  GETU16(lap->calories);
  GETU8(lap->avg_heart_rate);
  GETU8(lap->max_heart_rate);
  GETU8(lap->intensity);
  GETU8(lap->avg_cadence);
  GETU8(lap->trigger_method);
}


/* --------------------------------------------------------------------------*/
/* 7.4.54  D1012                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1012 ( D1012 * point, uint8 ** pos )
{
  GETSTR(point->name);
  SKIP(1);
  GETU16(point->course_index);
  SKIP(2);
  GETU32(point->track_point_time);
  GETU8(point->point_type);
}


/* --------------------------------------------------------------------------*/
/* 7.4.55  D1013                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1013 ( D1013 * limits, uint8 ** pos )
{
  GETU32(limits->max_courses);
  GETU32(limits->max_course_laps);
  GETU32(limits->max_course_pnt);
  GETU32(limits->max_course_trk_pnt);
}


/* --------------------------------------------------------------------------*/
/* 7.4.XX  D1015 (Undocumented)                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_unpack_d1015 ( D1015 * lap, uint8 ** pos )
{
  GETU16(lap->index);
  SKIP(2);
  GETU32(lap->start_time);
  GETU32(lap->total_time);
  GETF32(lap->total_dist);
  GETF32(lap->max_speed);
  GETPOS(lap->begin);
  GETPOS(lap->end);
  GETU16(lap->calories);
  GETU8(lap->avg_heart_rate);
  GETU8(lap->max_heart_rate);
  GETU8(lap->intensity);
  GETU8(lap->avg_cadence);
  GETU8(lap->trigger_method);

  /* 
     Garmin has not gotten back to me about what these fields mean, and
     whether all of the bytes are needed or just, say, three of them.
     This is annoying, because it means we may end up with .gmn files
     that have oversized D1015 elements, but it shouldn't affect our
     ability to read those files.  We don't make any assumptions about
     the size of each element.
  */

  GETU8(lap->unknown[0]);
  GETU8(lap->unknown[1]);
  GETU8(lap->unknown[2]);
  GETU8(lap->unknown[3]);
  GETU8(lap->unknown[4]);
}


/* List */

static void
garmin_unpack_dlist ( garmin_list * list, uint8 ** pos )
{
  uint32             id;
  uint32             elements;
  uint32             type;
  uint32             size;
  uint32             i;

  GETU32(list->id);
  GETU32(elements);

  for ( i = 0; i < elements; i++ ) {
    GETU32(id);
    GETU32(type);
    GETU32(size);
    if ( id == list->id ) {
      garmin_list_append(list,garmin_unpack(pos,type));
    } else {
      /* list element has wrong list ID */
      printf("garmin_unpack_dlist: list element had ID %d, expected ID %d\n",
	     id,list->id);
    }
  }
}


/* Unpack a chunk of data. */

static garmin_data *
garmin_unpack_chunk ( uint8 ** pos )
{
  garmin_data * data = NULL;
  uint8 *       start;
  uint32        unpacked;
  uint32        version;
  uint32        size;
  uint32        type;
  uint32        chunk;

  /* First, read the header and check that it's satisfactory. */
  
  if ( memcmp(*pos,GARMIN_MAGIC,strlen(GARMIN_MAGIC)) == 0 ) {
    SKIP(12);
    GETU32(version);
    
    if ( version > GARMIN_VERSION ) {
      /* warning: version is more recent than supported. */
      printf("garmin_unpack_chunk: version %.2f supported, %.2f found\n",
	     GARMIN_VERSION/100.0, version/100.0);
    }
    
    /* This is the size of the packed data (not including the header) */
    
    GETU32(size);

    /* Now let's get the type of the data, and the size of the chunk. */

    GETU32(type);
    GETU32(chunk);

    /* Unpack from here. */

    start    = *pos;
    data     = garmin_unpack(pos,type);
    unpacked = *pos - start;

    /* Double check - did we unpack the number of bytes we were supposed to? */

    if ( unpacked != chunk ) {      
      /* unpacked the wrong number of bytes! */
      printf("garmin_unpack_chunk: unpacked %d bytes (expecting %d)\n",
	     unpacked,chunk);
    }
    
  } else {
    /* unknown file format */
    printf("garmin_unpack_chunk: not a .gmn file\n");
  }

  return data;
}

  
/* ========================================================================= */
/* garmin_load                                                               */
/* ========================================================================= */

garmin_data *
garmin_load ( const char * filename )
{
  garmin_data * data   = NULL;
  garmin_data * data_l = NULL;
  garmin_list * list;
  uint32        bytes;
  uint8 *       buf;
  uint8 *       pos;
  uint8 *       start;
  struct stat   sb;
  int           fd;

  if ( (fd = open(filename,O_RDONLY)) != -1 ) {
    if ( fstat(fd,&sb) != -1 ) {
      if ( (buf = malloc(sb.st_size)) != NULL ) {
	if ( (bytes = read(fd,buf,sb.st_size)) == sb.st_size ) {
	  data_l = garmin_alloc_data(data_Dlist);
	  list   = data_l->data;
	  pos    = buf;
	  while ( pos - buf < bytes ) {
	    start = pos;
	    garmin_list_append(list,garmin_unpack_chunk(&pos));
	    if ( pos == start ) {
	      /* did not unpack anything! */
	      printf("garmin_load:  %s: nothing unpacked!\n",filename);
	      break;
	    }
	  }

	  /* 
	     If we unpacked only a single element, return it.  Otherwise,
	     return the list.
	  */

	  if ( list->elements == 1 ) {
	    data = list->head->data;
	    list->head->data = NULL;
	    garmin_free_data(data_l);
	  } else {
	    data = data_l;
	  }	     

	} else {
	  /* read failed */
	  printf("%s: read: %s\n",filename,strerror(errno));
	}
	free(buf);
      } else {
	/* malloc failed */
	printf("%s: malloc: %s\n",filename,strerror(errno));
      }
    } else {
      /* fstat failed */
      printf("%s: fstat: %s\n",filename,strerror(errno));
    }
    close(fd);
  } else {
    /* open failed */
    printf("%s: open: %s\n",filename,strerror(errno));
  }

  return data;
}


/* ========================================================================= */
/* garmin_unpack_packet                                                      */
/* ========================================================================= */

garmin_data *
garmin_unpack_packet ( garmin_packet * p, garmin_datatype type )
{
  uint8 * pos = p->packet.data;

  return garmin_unpack(&pos,type);
}


/* ========================================================================= */
/* garmin_unpack                                                             */
/* ========================================================================= */

garmin_data *
garmin_unpack ( uint8 **         pos, 
		garmin_datatype  type )
{
  garmin_data * d = garmin_alloc_data(type);

  /* Early exit if we were asked to allocate an unknown data type. */

  if ( d->data == NULL ) {
    free(d);
    return NULL;
  }

  /* Now do the actual unpacking. */

#define CASE_DATA(x) \
  case data_D##x: garmin_unpack_d##x(d->data,pos); break

  switch ( type ) {
  CASE_DATA(list);
  CASE_DATA(100);
  CASE_DATA(101);
  CASE_DATA(102);
  CASE_DATA(103);
  CASE_DATA(104);
  CASE_DATA(105);
  CASE_DATA(106);
  CASE_DATA(107);
  CASE_DATA(108);
  CASE_DATA(109);
  CASE_DATA(110);
  CASE_DATA(120);
  CASE_DATA(150);
  CASE_DATA(151);
  CASE_DATA(152);
  CASE_DATA(154);
  CASE_DATA(155);
  CASE_DATA(200);
  CASE_DATA(201);
  CASE_DATA(202);
  CASE_DATA(210);
  CASE_DATA(300);
  CASE_DATA(301);
  CASE_DATA(302);
  CASE_DATA(303);
  CASE_DATA(304);
  CASE_DATA(310);
  CASE_DATA(311);
  CASE_DATA(312);
  CASE_DATA(400);
  CASE_DATA(403);
  CASE_DATA(450);
  CASE_DATA(500);
  CASE_DATA(501);
  CASE_DATA(550);
  CASE_DATA(551);
  CASE_DATA(600);
  CASE_DATA(650);
  CASE_DATA(700);
  CASE_DATA(800);
  CASE_DATA(906);
  CASE_DATA(1000);
  CASE_DATA(1001);
  CASE_DATA(1002);
  CASE_DATA(1003);
  CASE_DATA(1004);
  CASE_DATA(1005);
  CASE_DATA(1006);
  CASE_DATA(1007);
  CASE_DATA(1008);
  CASE_DATA(1009);
  CASE_DATA(1010);
  CASE_DATA(1011);
  CASE_DATA(1012);
  CASE_DATA(1013);
  CASE_DATA(1015);
  default: 
    printf("garmin_unpack: data type %d not supported\n",type);
    break;
  }

  return d;

#undef CASE_DATA
}
