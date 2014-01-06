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
#include <errno.h>
#include <string.h>
#include "garmin.h"


#define PUTU16(x) do { put_uint16(*pos,x);  *pos += 2; }   while ( 0 )
#define PUTS16(x) do { put_sint16(*pos,x);  *pos += 2; }   while ( 0 )
#define PUTU32(x) do { put_uint32(*pos,x);  *pos += 4; }   while ( 0 )
#define PUTS32(x) do { put_sint32(*pos,x);  *pos += 4; }   while ( 0 )
#define PUTF32(x) do { put_float32(*pos,x); *pos += 4; }   while ( 0 )
#define PUTF64(x) do { put_float64(*pos,x); *pos += 8; }   while ( 0 )
#define PUTPOS(x) do { PUTS32((x).lat); PUTS32((x).lon); } while ( 0 )
#define PUTRPT(x) do { PUTF64((x).lat); PUTF64((x).lon); } while ( 0 )
#define PUTVST(x) put_vstring(pos,x)
#define PUTU8(x)  *(*pos)++ = x
#define SKIP(x)   do { memset(*pos,0,x); *pos += x; }      while ( 0 )

#define PUTSTR(x)                                                         \
  do {                                                                    \
    memcpy(*pos,x,sizeof(x)-1);                                           \
    (*pos)[sizeof(x)-1] = 0;	                                          \
    *pos += sizeof(x);                                                    \
  } while ( 0 )


/* --------------------------------------------------------------------------*/
/* 7.4.1   D100                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d100 ( D100 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTPOS(wpt->posn);
  SKIP(4);
  PUTSTR(wpt->cmnt);
}


/* --------------------------------------------------------------------------*/
/* 7.4.2   D101                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d101 ( D101 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTPOS(wpt->posn);
  SKIP(4);
  PUTSTR(wpt->cmnt);
  PUTF32(wpt->dst);
  PUTU8(wpt->smbl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.3   D102                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d102 ( D102 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTPOS(wpt->posn);
  SKIP(4);
  PUTSTR(wpt->cmnt);
  PUTF32(wpt->dst);
  PUTU16(wpt->smbl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.4   D103                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d103 ( D103 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTPOS(wpt->posn);
  SKIP(4);
  PUTSTR(wpt->cmnt);
  PUTU8(wpt->smbl);
  PUTU8(wpt->dspl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.5   D104                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d104 ( D104 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTPOS(wpt->posn);
  SKIP(4);
  PUTSTR(wpt->cmnt);
  PUTF32(wpt->dst);
  PUTU16(wpt->smbl);
  PUTU8(wpt->dspl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.6   D105                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d105 ( D105 * wpt, uint8 ** pos )
{
  PUTPOS(wpt->posn);
  PUTU16(wpt->smbl);
  PUTVST(wpt->wpt_ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.7   D106                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d106 ( D106 * wpt, uint8 ** pos )
{
  PUTU8(wpt->wpt_class);
  PUTSTR(wpt->subclass);
  PUTPOS(wpt->posn);
  PUTU16(wpt->smbl);
  PUTVST(wpt->wpt_ident);
  PUTVST(wpt->lnk_ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.8   D107                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d107 ( D107 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTPOS(wpt->posn);
  SKIP(4);
  PUTSTR(wpt->cmnt);
  PUTU8(wpt->smbl);
  PUTU8(wpt->dspl);
  PUTF32(wpt->dst);
  PUTU8(wpt->color);
}


/* --------------------------------------------------------------------------*/
/* 7.4.9   D108                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d108 ( D108 * wpt, uint8 ** pos )
{
  PUTU8(wpt->wpt_class);
  PUTU8(wpt->color);
  PUTU8(wpt->dspl);
  PUTU8(wpt->attr);
  PUTU16(wpt->smbl);
  PUTSTR(wpt->subclass);
  PUTPOS(wpt->posn);
  PUTF32(wpt->alt);
  PUTF32(wpt->dpth);
  PUTF32(wpt->dist);
  PUTSTR(wpt->state);
  PUTSTR(wpt->cc);
  PUTVST(wpt->ident);
  PUTVST(wpt->comment);
  PUTVST(wpt->facility);
  PUTVST(wpt->city);
  PUTVST(wpt->addr);
  PUTVST(wpt->cross_road);
}


/* --------------------------------------------------------------------------*/
/* 7.4.10  D109                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d109 ( D109 * wpt, uint8 ** pos )
{
  PUTU8(wpt->dtyp);
  PUTU8(wpt->wpt_class);
  PUTU8(wpt->dspl_color);
  PUTU8(wpt->attr);
  PUTU16(wpt->smbl);
  PUTSTR(wpt->subclass);
  PUTPOS(wpt->posn);
  PUTF32(wpt->alt);
  PUTF32(wpt->dpth);
  PUTF32(wpt->dist);
  PUTSTR(wpt->state);
  PUTSTR(wpt->cc);
  PUTU32(wpt->ete);
  PUTVST(wpt->ident);
  PUTVST(wpt->comment);
  PUTVST(wpt->facility);
  PUTVST(wpt->city);
  PUTVST(wpt->addr);
  PUTVST(wpt->cross_road);
}


/* --------------------------------------------------------------------------*/
/* 7.4.11  D110                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d110 ( D110 * wpt, uint8 ** pos )
{
  PUTU8(wpt->dtyp);
  PUTU8(wpt->wpt_class);
  PUTU8(wpt->dspl_color);
  PUTU8(wpt->attr);
  PUTU16(wpt->smbl);
  PUTSTR(wpt->subclass);
  PUTPOS(wpt->posn);
  PUTF32(wpt->alt);
  PUTF32(wpt->dpth);
  PUTF32(wpt->dist);
  PUTSTR(wpt->state);
  PUTSTR(wpt->cc);
  PUTU32(wpt->ete);
  PUTF32(wpt->temp);
  PUTU32(wpt->time);
  PUTU16(wpt->wpt_cat);
  PUTVST(wpt->ident);
  PUTVST(wpt->comment);
  PUTVST(wpt->facility);
  PUTVST(wpt->city);
  PUTVST(wpt->addr);
  PUTVST(wpt->cross_road);
}


/* --------------------------------------------------------------------------*/
/* 7.4.12  D120                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d120 ( D120 * cat, uint8 ** pos )
{
  PUTSTR(cat->name);
}


/* --------------------------------------------------------------------------*/
/* 7.4.13  D150                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d150 ( D150 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTSTR(wpt->cc);
  PUTU8(wpt->wpt_class);
  PUTPOS(wpt->posn);
  PUTS16(wpt->alt);
  PUTSTR(wpt->city);
  PUTSTR(wpt->state);
  PUTSTR(wpt->name);
  PUTSTR(wpt->cmnt);
}


/* --------------------------------------------------------------------------*/
/* 7.4.14  D151                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d151 ( D151 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTPOS(wpt->posn);
  SKIP(4);
  PUTSTR(wpt->cmnt);
  PUTF32(wpt->dst);
  PUTSTR(wpt->name);
  PUTSTR(wpt->city);
  PUTSTR(wpt->state);
  PUTS16(wpt->alt);
  PUTSTR(wpt->cc);
  SKIP(1);
  PUTU8(wpt->wpt_class);
}


/* --------------------------------------------------------------------------*/
/* 7.4.15  D152                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d152 ( D152 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTPOS(wpt->posn);
  SKIP(4);
  PUTSTR(wpt->cmnt);
  PUTF32(wpt->dst);
  PUTSTR(wpt->name);
  PUTSTR(wpt->city);
  PUTSTR(wpt->state);
  PUTS16(wpt->alt);
  PUTSTR(wpt->cc);
  SKIP(1);
  PUTU8(wpt->wpt_class);
}


/* --------------------------------------------------------------------------*/
/* 7.4.16  D154                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d154 ( D154 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTPOS(wpt->posn);
  SKIP(4);
  PUTSTR(wpt->cmnt);
  PUTF32(wpt->dst);
  PUTSTR(wpt->name);
  PUTSTR(wpt->city);
  PUTSTR(wpt->state);
  PUTS16(wpt->alt);
  PUTSTR(wpt->cc);
  SKIP(1);
  PUTU8(wpt->wpt_class);
  PUTU16(wpt->smbl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.17  D155                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d155 ( D155 * wpt, uint8 ** pos )
{
  PUTSTR(wpt->ident);
  PUTPOS(wpt->posn);
  SKIP(4);
  PUTSTR(wpt->cmnt);
  PUTF32(wpt->dst);
  PUTSTR(wpt->name);
  PUTSTR(wpt->city);
  PUTSTR(wpt->state);
  PUTS16(wpt->alt);
  PUTSTR(wpt->cc);
  SKIP(1);
  PUTU8(wpt->wpt_class);
  PUTU16(wpt->smbl);  
  PUTU8(wpt->dspl);
}


/* --------------------------------------------------------------------------*/
/* 7.4.18  D200                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d200 ( D200 * hdr, uint8 ** pos )
{
  PUTU8(*hdr);
}


/* --------------------------------------------------------------------------*/
/* 7.4.19  D201                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d201 ( D201 * hdr, uint8 ** pos )
{
  PUTU8(hdr->nmbr);
  PUTSTR(hdr->cmnt);
}


/* --------------------------------------------------------------------------*/
/* 7.4.20  D202                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d202 ( D202 * hdr, uint8 ** pos )
{
  PUTVST(hdr->rte_ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.21  D210                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d210 ( D210 * link, uint8 ** pos )
{
  PUTU16(link->link_class);
  PUTSTR(link->subclass);
  PUTVST(link->ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.22  D300                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d300 ( D300 * point, uint8 ** pos )
{
  PUTPOS(point->posn);
  PUTU32(point->time);
  PUTU8(point->new_trk);
}


/* --------------------------------------------------------------------------*/
/* 7.4.23  D301                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d301 ( D301 * point, uint8 ** pos )
{ 
  PUTPOS(point->posn);
  PUTU32(point->time);
  PUTF32(point->alt);
  PUTF32(point->dpth);
  PUTU8(point->new_trk);
}


/* --------------------------------------------------------------------------*/
/* 7.4.24  D302                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d302 ( D302 * point, uint8 ** pos )
{
  PUTPOS(point->posn);
  PUTU32(point->time);
  PUTF32(point->alt);
  PUTF32(point->dpth);
  PUTF32(point->temp);
  PUTU8(point->new_trk);
}


/* --------------------------------------------------------------------------*/
/* 7.4.25  D303                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d303 ( D303 * point, uint8 ** pos )
{
  PUTPOS(point->posn);
  PUTU32(point->time);
  PUTF32(point->alt);
  PUTU8(point->heart_rate);
}


/* --------------------------------------------------------------------------*/
/* 7.4.26  D304                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d304 ( D304 * point, uint8 ** pos )
{
  PUTPOS(point->posn);
  PUTU32(point->time);
  PUTF32(point->alt);
  PUTF32(point->distance);
  PUTU8(point->heart_rate);
  PUTU8(point->cadence);
  PUTU8(point->sensor);
}


/* --------------------------------------------------------------------------*/
/* 7.4.27  D310                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d310 ( D310 * hdr, uint8 ** pos )
{
  PUTU8(hdr->dspl);
  PUTU8(hdr->color);
  PUTVST(hdr->trk_ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.28  D311                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d311 ( D311 * hdr, uint8 ** pos )
{
  PUTU16(hdr->index);
}


/* --------------------------------------------------------------------------*/
/* 7.4.29  D312                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d312 ( D312 * hdr, uint8 ** pos )
{
  PUTU8(hdr->dspl);
  PUTU8(hdr->color);
  PUTVST(hdr->trk_ident);
}


/* ------------------------------------------------------------------------- */
/* 7.4.30  D400                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d400 ( D400 * prx, uint8 ** pos )
{
  garmin_pack_d100(&prx->wpt,pos);
  SKIP(sizeof(D100));
  PUTF32(prx->dst);
}


/* ------------------------------------------------------------------------- */
/* 7.4.31  D403                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d403 ( D403 * prx, uint8 ** pos )
{
  garmin_pack_d103(&prx->wpt,pos);
  SKIP(sizeof(D103));
  PUTF32(prx->dst);
}


/* ------------------------------------------------------------------------- */
/* 7.4.32  D450                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d450 ( D450 * prx, uint8 ** pos )
{
  PUTU32(prx->idx);
  garmin_pack_d150(&prx->wpt,pos);
  SKIP(sizeof(D150));
  PUTF32(prx->dst);
}


/* ------------------------------------------------------------------------- */
/* 7.4.33  D500                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d500 ( D500 * alm, uint8 ** pos )
{
  PUTU16(alm->wn);
  PUTF32(alm->toa);
  PUTF32(alm->af0);
  PUTF32(alm->af1);
  PUTF32(alm->e);
  PUTF32(alm->sqrta);
  PUTF32(alm->m0);
  PUTF32(alm->w);
  PUTF32(alm->omg0);
  PUTF32(alm->odot);
  PUTF32(alm->i);
}


/* ------------------------------------------------------------------------- */
/* 7.4.34  D501                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d501 ( D501 * alm, uint8 ** pos )
{
  PUTU16(alm->wn);
  PUTF32(alm->toa);
  PUTF32(alm->af0);
  PUTF32(alm->af1);
  PUTF32(alm->e);
  PUTF32(alm->sqrta);
  PUTF32(alm->m0);
  PUTF32(alm->w);
  PUTF32(alm->omg0);
  PUTF32(alm->odot);
  PUTF32(alm->i);
  PUTU8(alm->hlth);
}


/* ------------------------------------------------------------------------- */
/* 7.4.35  D550                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d550 ( D550 * alm, uint8 ** pos )
{
  PUTU8(alm->svid);
  PUTU16(alm->wn);
  PUTF32(alm->toa);
  PUTF32(alm->af0);
  PUTF32(alm->af1);
  PUTF32(alm->e);
  PUTF32(alm->sqrta);
  PUTF32(alm->m0);
  PUTF32(alm->w);
  PUTF32(alm->omg0);
  PUTF32(alm->odot);
  PUTF32(alm->i);
}


/* ------------------------------------------------------------------------- */
/* 7.4.36  D551                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d551 ( D551 * alm, uint8 ** pos )
{
  PUTU8(alm->svid);
  PUTU16(alm->wn);
  PUTF32(alm->toa);
  PUTF32(alm->af0);
  PUTF32(alm->af1);
  PUTF32(alm->e);
  PUTF32(alm->sqrta);
  PUTF32(alm->m0);
  PUTF32(alm->w);
  PUTF32(alm->omg0);
  PUTF32(alm->odot);
  PUTF32(alm->i);
  PUTU8(alm->hlth);
}


/* ------------------------------------------------------------------------- */
/* 7.4.37  D600                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d600 ( D600 * dt, uint8 ** pos )
{
  PUTU8(dt->month);
  PUTU8(dt->day);
  PUTU16(dt->year);
  PUTU16(dt->hour);
  PUTU8(dt->minute);
  PUTU8(dt->second);
}


/* ------------------------------------------------------------------------- */
/* 7.4.38  D650                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d650 ( D650 * fbr, uint8 ** pos )
{
  PUTU32(fbr->takeoff_time);
  PUTU32(fbr->landing_time);
  PUTPOS(fbr->takeoff_posn);
  PUTPOS(fbr->landing_posn);
  PUTU32(fbr->night_time);
  PUTU32(fbr->num_landings);
  PUTF32(fbr->max_speed);
  PUTF32(fbr->max_alt);
  PUTF32(fbr->distance);
  PUTU8(fbr->cross_country_flag);
  PUTVST(fbr->departure_name);
  PUTVST(fbr->departure_ident);
  PUTVST(fbr->arrival_name);
  PUTVST(fbr->arrival_ident);
  PUTVST(fbr->ac_id);
}


/* ------------------------------------------------------------------------- */
/* 7.4.39  D700                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d700 ( D700 * pt, uint8 ** pos )
{
  PUTRPT(*pt);
}


/* ------------------------------------------------------------------------- */
/* 7.4.40  D800                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_pack_d800 ( D800 * pvt, uint8 ** pos )
{
  PUTF32(pvt->alt);
  PUTF32(pvt->epe);
  PUTF32(pvt->eph);
  PUTF32(pvt->epv);
  PUTU16(pvt->fix);
  PUTF64(pvt->tow);
  PUTRPT(pvt->posn);
  PUTF32(pvt->east);
  PUTF32(pvt->north);
  PUTF32(pvt->up);
  PUTF32(pvt->msl_hght);
  PUTS16(pvt->leap_scnds);
  PUTU32(pvt->wn_days);
}


/* --------------------------------------------------------------------------*/
/* 7.4.41  D906                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d906 ( D906 * lap, uint8 ** pos )
{
  PUTU32(lap->start_time);
  PUTU32(lap->total_time);
  PUTF32(lap->total_distance);
  PUTPOS(lap->begin);
  PUTPOS(lap->end);
  PUTU16(lap->calories);
  PUTU8(lap->track_index);
}


/* --------------------------------------------------------------------------*/
/* 7.4.42  D1000                                                             */
/* --------------------------------------------------------------------------*/


static void garmin_pack_d1002 ( D1002 * wkt, uint8 ** pos );


static void
garmin_pack_d1000 ( D1000 * run, uint8 ** pos )
{
  PUTU32(run->track_index);
  PUTU32(run->first_lap_index);
  PUTU32(run->last_lap_index);
  PUTU8(run->sport_type);
  PUTU8(run->program_type);
  SKIP(2);
  PUTU32(run->virtual_partner.time);
  PUTF32(run->virtual_partner.distance);
  garmin_pack_d1002(&run->workout,pos);
}


/* --------------------------------------------------------------------------*/
/* 7.4.43  D1001                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1001 ( D1001 * lap, uint8 ** pos )
{
  PUTU32(lap->index);
  PUTU32(lap->start_time);
  PUTU32(lap->total_time);
  PUTF32(lap->total_dist);
  PUTF32(lap->max_speed);
  PUTPOS(lap->begin);
  PUTPOS(lap->end);
  PUTU16(lap->calories);
  PUTU8(lap->avg_heart_rate);
  PUTU8(lap->max_heart_rate);
  PUTU8(lap->intensity);
}


/* --------------------------------------------------------------------------*/
/* 7.4.44  D1002                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1002 ( D1002 * wkt, uint8 ** pos )
{
  int i;

  PUTU32(wkt->num_valid_steps);
  for ( i = 0; i < 20; i++ ) {
    PUTSTR(wkt->steps[i].custom_name);
    PUTF32(wkt->steps[i].target_custom_zone_low);
    PUTF32(wkt->steps[i].target_custom_zone_high);
    PUTU16(wkt->steps[i].duration_value);
    PUTU8(wkt->steps[i].intensity);
    PUTU8(wkt->steps[i].duration_type);
    PUTU8(wkt->steps[i].target_type);
    PUTU8(wkt->steps[i].target_value);
    SKIP(2);
  }
  PUTSTR(wkt->name);
  PUTU8(wkt->sport_type);
}


/* --------------------------------------------------------------------------*/
/* 7.4.45  D1003                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1003 ( D1003 * occ, uint8 ** pos )
{
  PUTSTR(occ->workout_name);
  PUTU32(occ->day);
}


/* --------------------------------------------------------------------------*/
/* 7.4.46  D1004                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1004 ( D1004 * prof, uint8 ** pos )
{
  int i;
  int j;

  for ( i = 0; i < 3; i++ ) {
    for ( j = 0; j < 5; j++ ) {
      PUTU8(prof->activities[i].heart_rate_zones[j].low_heart_rate);
      PUTU8(prof->activities[i].heart_rate_zones[j].high_heart_rate);
      SKIP(2);
    }
    for ( j = 0; j < 10; j++ ) {
      PUTF32(prof->activities[i].speed_zones[j].low_speed);
      PUTF32(prof->activities[i].speed_zones[j].high_speed);
      PUTSTR(prof->activities[i].speed_zones[j].name);
    }
    PUTF32(prof->activities[i].gear_weight);
    PUTU8(prof->activities[i].max_heart_rate);
    SKIP(3);
  }
  PUTF32(prof->weight);
  PUTU16(prof->birth_year);
  PUTU8(prof->birth_month);
  PUTU8(prof->birth_day);
  PUTU8(prof->gender);
}


/* --------------------------------------------------------------------------*/
/* 7.4.47  D1005                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1005 ( D1005 * limits, uint8 ** pos )
{
  PUTU32(limits->max_workouts);
  PUTU32(limits->max_unscheduled_workouts);
  PUTU32(limits->max_occurrences);
}


/* --------------------------------------------------------------------------*/
/* 7.4.48  D1006                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1006 ( D1006 * course, uint8 ** pos )
{
  PUTU16(course->index);
  SKIP(2);
  PUTSTR(course->course_name);
  PUTU16(course->track_index);
}


/* --------------------------------------------------------------------------*/
/* 7.4.49  D1007                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1007 ( D1007 * lap, uint8 ** pos )
{
  PUTU16(lap->course_index);
  PUTU16(lap->lap_index);
  PUTU32(lap->total_time);
  PUTF32(lap->total_dist);
  PUTPOS(lap->begin);
  PUTPOS(lap->end);
  PUTU8(lap->avg_heart_rate);
  PUTU8(lap->max_heart_rate);
  PUTU8(lap->intensity);
  PUTU8(lap->avg_cadence);
}


/* --------------------------------------------------------------------------*/
/* 7.4.50  D1008                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1008 ( D1008 * wkt, uint8 ** pos )
{
  garmin_pack_d1002((D1002 *)wkt,pos);
}


/* --------------------------------------------------------------------------*/
/* 7.4.51  D1009                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1009 ( D1009 * run, uint8 ** pos )
{
  PUTU16(run->track_index);
  PUTU16(run->first_lap_index);
  PUTU16(run->last_lap_index);
  PUTU8(run->sport_type);
  PUTU8(run->program_type);
  PUTU8(run->multisport);
  SKIP(3);
  PUTU32(run->quick_workout.time);
  PUTF32(run->quick_workout.distance);
  garmin_pack_d1008(&run->workout,pos);
}


/* --------------------------------------------------------------------------*/
/* 7.4.52  D1010                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1010 ( D1010 * run, uint8 ** pos )
{
  PUTU32(run->track_index);
  PUTU32(run->first_lap_index);
  PUTU32(run->last_lap_index);
  PUTU8(run->sport_type);
  PUTU8(run->program_type);
  PUTU8(run->multisport);
  SKIP(1);
  PUTU32(run->virtual_partner.time);
  PUTF32(run->virtual_partner.distance);
  garmin_pack_d1002(&run->workout,pos);
}


/* --------------------------------------------------------------------------*/
/* 7.4.53  D1011                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1011 ( D1011 * lap, uint8 ** pos )
{
  PUTU16(lap->index);
  SKIP(2);
  PUTU32(lap->start_time);
  PUTU32(lap->total_time);
  PUTF32(lap->total_dist);
  PUTF32(lap->max_speed);
  PUTPOS(lap->begin);
  PUTPOS(lap->end);
  PUTU16(lap->calories);
  PUTU8(lap->avg_heart_rate);
  PUTU8(lap->max_heart_rate);
  PUTU8(lap->intensity);
  PUTU8(lap->avg_cadence);
  PUTU8(lap->trigger_method);
}


/* --------------------------------------------------------------------------*/
/* 7.4.54  D1012                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1012 ( D1012 * point, uint8 ** pos )
{
  PUTSTR(point->name);
  SKIP(1);
  PUTU16(point->course_index);
  SKIP(2);
  PUTU32(point->track_point_time);
  PUTU8(point->point_type);
}


/* --------------------------------------------------------------------------*/
/* 7.4.55  D1013                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1013 ( D1013 * limits, uint8 ** pos )
{
  PUTU32(limits->max_courses);
  PUTU32(limits->max_course_laps);
  PUTU32(limits->max_course_pnt);
  PUTU32(limits->max_course_trk_pnt);
}


/* --------------------------------------------------------------------------*/
/* 7.4.XX  D1015 (Undocumented)                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_pack_d1015 ( D1015 * lap, uint8 ** pos )
{
  PUTU16(lap->index);
  SKIP(2);
  PUTU32(lap->start_time);
  PUTU32(lap->total_time);
  PUTF32(lap->total_dist);
  PUTF32(lap->max_speed);
  PUTPOS(lap->begin);
  PUTPOS(lap->end);
  PUTU16(lap->calories);
  PUTU8(lap->avg_heart_rate);
  PUTU8(lap->max_heart_rate);
  PUTU8(lap->intensity);
  PUTU8(lap->avg_cadence);
  PUTU8(lap->trigger_method);

  /* Hopefully we'll know what this stuff actually is someday. */

  PUTU8(lap->unknown[0]);
  PUTU8(lap->unknown[1]);
  PUTU8(lap->unknown[2]);
  PUTU8(lap->unknown[3]);
  PUTU8(lap->unknown[4]);
}


/* List */

static void
garmin_pack_dlist ( garmin_list * list, uint8 ** pos )
{
  garmin_list_node * node;

  PUTU32(list->id);
  PUTU32(list->elements);
  for ( node = list->head; node != NULL; node = node->next ) {
    PUTU32(list->id);
    garmin_pack(node->data,pos);
  }
}


/* make a directory path (may require creation of multiple directories) */

static int
mkpath ( const char *path )
{
  struct stat sb;
  char        rpath[BUFSIZ];
  int         n = 0;
  int         j = 0;
  int         ok = 1;
  uid_t       owner = -1;
  gid_t       group = -1;
  int         already = 0;
  mode_t      mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
  
  /* check for obvious errors */

  if ( !path || *path != '/' ) return 0;

  /* 
     if the path already exists, return 1 if it is a directory and 0 
     otherwise 
  */
  
  if ( stat(path,&sb) != -1 ) {
    return (S_ISDIR(sb.st_mode)) ? 1 : 0;
  }

  /* 
     loop through the path, stopping at each slash to try and make the 
     directory. 
  */

  while ( path[n] ) {
    rpath[j++] = path[n++];
    if ( path[n] == '/' ) {
      rpath[j] = 0;
      if ( stat(rpath,&sb) != -1 ) {  /* this part already exists */
	if ( !S_ISDIR(sb.st_mode) ) { /* but is not a directory!  */
	  fprintf(stderr,"mkpath: %s exists but is not a directory",rpath);
	  ok = 0;
	  break;
	} else {
	  
	  /* figure out who owns the directory, what permissions they have */

	  owner   = sb.st_uid;
	  group   = sb.st_gid;
	  mode    = sb.st_mode;
	  already = 1;
	}
      } else {
	if ( mkdir(rpath,mode) != -1 ) {   /* have to make this part */
	  if ( already ) {
	    chown(rpath,owner,group);
	  }
	} else {
	  fprintf(stderr,"mkpath: mkdir(%s,%o): %s",path,mode,strerror(errno));
	  ok = 0;
	  break;
	}
      }
    }
  }
  
  /* make the final path */

  if ( mkdir(path,mode) != -1 ) {
    if ( already ) {
      chown(rpath,owner,group);
    }
  } else {
    fprintf(stderr,"mkpath: mkdir(%s,%o): %s",path,mode,strerror(errno));
    ok = 0;
  }

  return ok;
}


/* ========================================================================= */
/* garmin_save                                                               */
/* ========================================================================= */

uint32
garmin_save ( garmin_data * data, const char * filename, const char * dir )
{
  int         fd;
  uint8 *     buf;
  uint8 *     pos;
  uint8 *     marker;
  uint32      bytes  = 0;
  uint32      packed = 0;
  uint32      wrote  = 0;
  struct stat sb;
  uid_t       owner = -1;
  gid_t       group = -1;
  char        path[BUFSIZ];

  if ( (bytes = garmin_data_size(data)) != 0 ) {

    mkpath(dir);
    if ( stat(dir,&sb) != -1 ) {
      owner = sb.st_uid;
      group = sb.st_gid;
    }

    snprintf(path,sizeof(path)-1,"%s/%s",dir,filename);
    if ( stat(path,&sb) != -1 ) {
      /* Do NOT overwrite if the file is already there. */
      return 0;
    }

    if ( (fd = creat(path,0664)) != -1 ) {

      fchown(fd,owner,group);

      /* Allocate the memory and write the file header */

      if ( (buf = malloc(bytes + GARMIN_HEADER)) != NULL ) {

	/* write GARMIN_MAGIC, GARMIN_VERSION, and bytes. */

	pos = buf;
	memset(pos,0,GARMIN_HEADER);
	strncpy(pos,GARMIN_MAGIC,11);
	put_uint32(pos+12,GARMIN_VERSION);
	marker = pos+16;
	pos += GARMIN_HEADER;
	packed = GARMIN_HEADER;

	/* pack the rest of the data. */
	
	packed += garmin_pack(data,&pos);
	put_uint32(marker,packed-GARMIN_HEADER);

	/* Now write the data to the file and close the file. */

	if ( (wrote = write(fd,buf,packed)) != packed ) {
	  /* write error! */
	  printf("write of %d bytes returned %d: %s\n",
		 packed,wrote,strerror(errno));
	}
	close(fd);

	/* Free the buffer. */
	
	free(buf);

      } else {
	/* malloc error */
	printf("malloc(%d): %s\n",bytes + GARMIN_HEADER, strerror(errno));
      }
    } else {
      /* problem creating file. */
      printf("creat: %s: %s\n",path,strerror(errno));
    }
  } else {
    /* don't write empty data */
    printf("%s: garmin_data_size was 0\n",path);
  }

  return bytes;
}


/* ========================================================================= */
/* garmin_pack                                                               */
/*                                                                           */
/* Take an arbitrary garmin_data and pack it into a buffer.  The buffer is   */
/* allocated with malloc, and the number of bytes allocated is returned.     */
/* The buffer returned is suitable for transferring to a Garmin device or    */
/* for writing to disk.                                                      */
/* ========================================================================= */

uint32
garmin_pack ( garmin_data * data, uint8 ** buf )
{
  uint8 * start;
  uint8 * finish;
  uint8 * marker;
  uint32  bytes = 0;

  if ( garmin_data_size(data) == 0 ) return 0;

  /* OK, we must know how to serialize this data.  Let's go for it. */

#define CASE_DATA(x)                        \
  case data_D##x:                           \
    {                                       \
      put_uint32(*buf,data->type);          \
      *buf += 4;                            \
      marker = *buf;                        \
      *buf += 4;                            \
      start = *buf;                         \
      garmin_pack_d##x(data->data,buf);     \
      finish = *buf;                        \
      bytes = finish-start;                 \
      put_uint32(marker,bytes);             \
      bytes += 8;                           \
    }                                       \
    break
					   
  switch ( data->type ) {
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
    printf("garmin_pack: data type %d not supported\n",data->type);
    break;
  }
#undef CASE_DATA

  return bytes;
}
