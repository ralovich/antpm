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
#include "garmin.h"


/* 
   This file contains functions to print Garmin datatypes in an XML-like
   form.  The functions aim to reproduce the data losslessly, including
   floating point data, such that the data can be scanned back from the
   printed output and reconstructed exactly.
*/


#define GARMIN_ENUM_NAME(x,y)                        \
  static char * garmin_d##x##_##y ( D##x##_##y z )   \
  {                                                  \
    char * name = "unknown";                         \
                                                     \
    switch ( z )

#define GARMIN_ENUM_CASE(x,y)                        \
    case D##x##_##y: name = #y; break

#define GARMIN_ENUM_DEFAULT                          \
    default: break;                                  \
    }                                                \
                                                     \
    return name

#define GARMIN_TAGFMT(w,x,y,z)                       \
  do {                                               \
    print_spaces(fp,spaces+x);                       \
    fprintf(fp,"<%s>" w "</%s>\n",y,z,y);            \
  } while ( 0 )

#define GARMIN_TAGFUN(w,x,y,z)                       \
  do {                                               \
    print_spaces(fp,spaces+x);                       \
    fprintf(fp,"<%s>",y);                            \
    w(z,fp);                                         \
    fprintf(fp,"</%s>\n",y);                         \
  } while ( 0 )

#define GARMIN_TAGPOS(x,y,z)                         \
  do {                                               \
    print_spaces(fp,spaces+x);                       \
    fprintf(fp,"<%s lat=\"%.8lf\" lon=\"%.8lf\"/>\n",\
            y,SEMI2DEG((z).lat),SEMI2DEG((z).lon));  \
  } while ( 0 )

#define GARMIN_TAGSYM(x,y,z)                         \
  do {                                               \
    print_spaces(fp,spaces+x);                       \
    fprintf(fp,"<%s value=\"0x%x\" name=\"%s\"/>\n", \
	    y,z,garmin_symbol_name(z));              \
  } while ( 0 )

#define GARMIN_TAGU8B(x,y,z,l)			     \
  do {                                               \
    int u8b;                                         \
                                                     \
    open_tag(y,fp,spaces+x);                         \
    print_spaces(fp,spaces+x);                       \
    for ( u8b = 0; u8b < l; u8b++ ) {                \
      fprintf(fp," 0x%02x",z[u8b]);                  \
    }                                                \
    fprintf(fp,"\n");                                \
    close_tag(y,fp,spaces+x);                        \
  } while ( 0 )

#define GARMIN_TAGSTR(x,y,z) GARMIN_TAGFMT("%s",x,y,z)
#define GARMIN_TAGINT(x,y,z) GARMIN_TAGFMT("%d",x,y,z)
#define GARMIN_TAGU32(x,y,z) GARMIN_TAGFMT("%u",x,y,z)
#define GARMIN_TAGF32(x,y,z) GARMIN_TAGFUN(garmin_print_float32,x,y,z)
#define GARMIN_TAGF64(x,y,z) GARMIN_TAGFUN(garmin_print_float64,x,y,z)
#define GARMIN_TAGHEX(x,y,z) GARMIN_TAGFMT("0x%x",x,y,z)


static void
print_spaces ( FILE * fp, int spaces )
{
  int i;

  for ( i = 0; i < spaces; i++ ) {
    fprintf(fp," ");
  }
}


static void
open_tag ( const char * tag, FILE * fp, int spaces ) 
{
  print_spaces(fp,spaces);
  fprintf(fp,"<%s>\n",tag);
}


static void
open_tag_with_type ( const char * tag, uint32 type, FILE * fp, int spaces ) 
{
  print_spaces(fp,spaces);
  fprintf(fp,"<%s type=\"%d\">\n",tag,type);
}


static void
close_tag ( const char * tag, FILE * fp, int spaces ) 
{
  print_spaces(fp,spaces);
  fprintf(fp,"</%s>\n",tag);
}


static void
garmin_print_dlist ( garmin_list * l, FILE * fp, int spaces )
{
  garmin_list_node * n;

  for ( n = l->head; n != NULL; n = n->next ) {
    garmin_print_data(n->data,fp,spaces);
  }
}


/* Support function to print a time value in ISO 8601 compliant format */

static void
garmin_print_dtime ( uint32 t, FILE * fp, const char * label )
{
  time_t     tval;
  struct tm  tmval;
  char       buf[128];
  int        len;

  /* 
                                  012345678901234567890123
     This will make, for example, 2007-04-20T23:55:01-0700, but that date
     isn't quite ISO 8601 compliant.  We need to stick a ':' in the time
     zone between the hours and the minutes.
  */

  tval = t + TIME_OFFSET;
  localtime_r(&tval,&tmval);
  strftime(buf,sizeof(buf)-1,"%FT%T%z",&tmval);

  /* 
     If the last character is a 'Z', don't do anything.  Otherwise, we 
     need to move the last two characters out one and stick a colon in 
     the vacated spot.  Let's not forget the trailing '\0' that needs to 
     be moved as well.
  */

  len = strlen(buf);
  if ( len > 0 && buf[len-1] != 'Z' ) {
    memmove(buf+len-1,buf+len-2,3);
    buf[len-2] = ':';
  }

  /* OK.  Done. */

  fprintf(fp," %s=\"%s\"",label,buf);
}


/* Support function to print a position type */

static void
garmin_print_dpos ( position_type * pos, FILE * fp )
{
  if ( pos->lat != 0x7fffffff ) {
    fprintf(fp," lat=\"%.8lf\"",SEMI2DEG(pos->lat));
  }
  if ( pos->lon != 0x7fffffff ) {
    fprintf(fp," lon=\"%.8lf\"",SEMI2DEG(pos->lon));
  }
}


/* 
   Print a float32 with enough precision such that it can be reconstructed
   exactly from its decimal representation.
*/

static void
garmin_print_float32 ( float32 f, FILE * fp )
{
  if ( f > 100000000.0 || f < -100000000.0 ) {
    fprintf(fp,"%.9e",f);
  } else if ( f > 10000000.0 || f < -10000000.0 ) {
    fprintf(fp,"%.1f",f);
  } else if ( f > 1000000.0 || f < -1000000.0 ) {
    fprintf(fp,"%.2f",f);
  } else if ( f > 100000.0 || f < -100000.0 ) {
    fprintf(fp,"%.3f",f);
  } else if ( f > 10000.0 || f < -10000.0 ) {
    fprintf(fp,"%.4f",f);
  } else if ( f > 1000.0 || f < -1000.0 ) {
    fprintf(fp,"%.5f",f);
  } else if ( f > 100.0 || f < -100.0 ) {
    fprintf(fp,"%.6f",f);
  } else if ( f > 10.0 || f < -10.0 ) {
    fprintf(fp,"%.7f",f);
  } else if ( f > 1.0 || f < -1.0 ) {
    fprintf(fp,"%.8f",f);
  } else if ( f > 0.1 || f < -0.1 ) {
    fprintf(fp,"%.9f",f);
  } else if ( f != 0 ) {
    fprintf(fp,"%.9e",f);
  } else {
    fprintf(fp,"%.8f",f);
  }
}


/* 
   Print a float64 with enough precision such that it can be reconstructed
   exactly from its decimal representation.
*/

static void
garmin_print_float64 ( float64 f, FILE * fp )
{
  if ( f > 10000000000000000.0 || f < -10000000000000000.0 ) {
    fprintf(fp,"%.17e",f);
  } else if ( f > 1000000000000000.0 || f < -1000000000000000.0 ) {
    fprintf(fp,"%.1f",f);
  } else if ( f > 100000000000000.0 || f < -100000000000000.0 ) {
    fprintf(fp,"%.2f",f);
  } else if ( f > 10000000000000.0 || f < -10000000000000.0 ) {
    fprintf(fp,"%.3f",f);
  } else if ( f > 1000000000000.0 || f < -1000000000000.0 ) {
    fprintf(fp,"%.4f",f);
  } else if ( f > 100000000000.0 || f < -100000000000.0 ) {
    fprintf(fp,"%.5f",f);
  } else if ( f > 10000000000.0 || f < -10000000000.0 ) {
    fprintf(fp,"%.6f",f);
  } else if ( f > 1000000000.0 || f < -1000000000.0 ) {
    fprintf(fp,"%.7f",f);
  } else if ( f > 100000000.0 || f < -100000000.0 ) {
    fprintf(fp,"%.8f",f);
  } else if ( f > 10000000.0 || f < -10000000.0 ) {
    fprintf(fp,"%.9f",f);
  } else if ( f > 1000000.0 || f < -1000000.0 ) {
    fprintf(fp,"%.10f",f);
  } else if ( f > 100000.0 || f < -100000.0 ) {
    fprintf(fp,"%.11f",f);
  } else if ( f > 10000.0 || f < -10000.0 ) {
    fprintf(fp,"%.12f",f);
  } else if ( f > 1000.0 || f < -1000.0 ) {
    fprintf(fp,"%.13f",f);
  } else if ( f > 100.0 || f < -100.0 ) {
    fprintf(fp,"%.14f",f);
  } else if ( f > 10.0 || f < -10.0 ) {
    fprintf(fp,"%.15f",f);
  } else if ( f > 1.0 || f < -1.0 ) {
    fprintf(fp,"%.16f",f);
  } else if ( f > 0.1 || f < -0.1 ) {
    fprintf(fp,"%.17f",f);
  } else if ( f != 0 ) {
    fprintf(fp,"%.17e",f);
  } else {
    fprintf(fp,"%.16f",f);
  }
}


/* 
   Print a float32 whose value is invalid (and should not be printed) if 
   greater than 1.0e24 
*/

static void
garmin_print_dfloat32 ( float32 f, FILE * fp, const char * label )
{
  if ( f < 1.0e24 ) {
    fprintf(fp," %s=\"",label);
    garmin_print_float32(f,fp);
    fprintf(fp,"\"");
  }
}


/* Print a duration and distance. */

static void
garmin_print_ddist ( uint32 dur, float32 dist, FILE * fp )
{
  int  hun;
  int  sec;
  int  min;
  int  hrs;
  
  hun  = dur % 100;
  dur -= hun;
  dur /= 100;
  sec  = dur % 60;
  dur -= sec;
  dur /= 60;
  min  = dur % 60;
  dur -= min;
  dur /= 60;
  hrs  = dur;

  fprintf(fp," duration=\"%d:%02d:%02d.%02d\" distance=\"",hrs,min,sec,hun);
  garmin_print_float32(dist,fp);
  fprintf(fp,"\"");
}


/* --------------------------------------------------------------------------*/
/* 7.4.1  D100                                                               */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d100 ( D100 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",100,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.2  D101                                                               */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d101 ( D101 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",101,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  GARMIN_TAGF32(1,"proximity_distance",x->dst);
  GARMIN_TAGSYM(1,"symbol",x->smbl);
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.3  D102                                                               */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d102 ( D102 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",102,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  GARMIN_TAGF32(1,"proximity_distance",x->dst);
  GARMIN_TAGSYM(1,"symbol",x->smbl);
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.4  D103                                                               */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(103,smbl) {
  GARMIN_ENUM_CASE(103,smbl_dot);
  GARMIN_ENUM_CASE(103,smbl_house);
  GARMIN_ENUM_CASE(103,smbl_gas);
  GARMIN_ENUM_CASE(103,smbl_car);
  GARMIN_ENUM_CASE(103,smbl_fish);
  GARMIN_ENUM_CASE(103,smbl_boat);
  GARMIN_ENUM_CASE(103,smbl_anchor);
  GARMIN_ENUM_CASE(103,smbl_wreck);
  GARMIN_ENUM_CASE(103,smbl_exit);
  GARMIN_ENUM_CASE(103,smbl_skull);
  GARMIN_ENUM_CASE(103,smbl_flag);
  GARMIN_ENUM_CASE(103,smbl_camp);
  GARMIN_ENUM_CASE(103,smbl_circle_x);
  GARMIN_ENUM_CASE(103,smbl_deer);
  GARMIN_ENUM_CASE(103,smbl_1st_aid);
  GARMIN_ENUM_CASE(103,smbl_back_track);
  GARMIN_ENUM_DEFAULT;
}


GARMIN_ENUM_NAME(103,dspl) {
  GARMIN_ENUM_CASE(103,dspl_name);
  GARMIN_ENUM_CASE(103,dspl_none);
  GARMIN_ENUM_CASE(103,dspl_cmnt);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d103 ( D103 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",103,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  GARMIN_TAGSTR(1,"symbol",garmin_d103_smbl(x->smbl));
  GARMIN_TAGSTR(1,"display",garmin_d103_dspl(x->dspl));
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.5  D104                                                               */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(104,dspl) {
  GARMIN_ENUM_CASE(104,dspl_smbl_none);
  GARMIN_ENUM_CASE(104,dspl_smbl_only);
  GARMIN_ENUM_CASE(104,dspl_smbl_name);
  GARMIN_ENUM_CASE(104,dspl_smbl_cmnt);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d104 ( D104 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",104,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  GARMIN_TAGF32(1,"proximity_distance",x->dst);
  GARMIN_TAGSYM(1,"symbol",x->smbl);
  GARMIN_TAGSTR(1,"display",garmin_d104_dspl(x->dspl));
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.6  D105                                                               */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d105 ( D105 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",105,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->wpt_ident);
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSYM(1,"symbol",x->smbl);
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.7  D106                                                               */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d106 ( D106 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",106,fp,spaces);
  GARMIN_TAGSTR(1,"class",(x->wpt_class)?"non-user":"user");
  if ( x->wpt_class != 0 ) {
    GARMIN_TAGU8B(1,"subclass",x->subclass,13);
  }
  GARMIN_TAGSTR(1,"ident",x->wpt_ident);
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSYM(1,"symbol",x->smbl);
  GARMIN_TAGSTR(1,"link",x->lnk_ident);
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.8  D107                                                               */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(107,clr) {
  GARMIN_ENUM_CASE(107,clr_default);
  GARMIN_ENUM_CASE(107,clr_red);
  GARMIN_ENUM_CASE(107,clr_green);
  GARMIN_ENUM_CASE(107,clr_blue);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d107 ( D107 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",107,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  GARMIN_TAGF32(1,"proximity_distance",x->dst);
  GARMIN_TAGSTR(1,"symbol",garmin_d103_smbl(x->smbl));
  GARMIN_TAGSTR(1,"display",garmin_d103_dspl(x->dspl));
  GARMIN_TAGSTR(1,"color",garmin_d107_clr(x->color));
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.9  D108                                                               */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(108,wpt_class) {
  GARMIN_ENUM_CASE(108,user_wpt);
  GARMIN_ENUM_CASE(108,avtn_apt_wpt);
  GARMIN_ENUM_CASE(108,avtn_int_wpt);
  GARMIN_ENUM_CASE(108,avtn_ndb_wpt);
  GARMIN_ENUM_CASE(108,avtn_vor_wpt);
  GARMIN_ENUM_CASE(108,avtn_arwy_wpt);
  GARMIN_ENUM_CASE(108,avtn_aint_wpt);
  GARMIN_ENUM_CASE(108,avtn_andb_wpt);
  GARMIN_ENUM_CASE(108,map_pnt_wpt);
  GARMIN_ENUM_CASE(108,map_area_wpt);
  GARMIN_ENUM_CASE(108,map_int_wpt);
  GARMIN_ENUM_CASE(108,map_adrs_wpt);
  GARMIN_ENUM_CASE(108,map_line_wpt);
  GARMIN_ENUM_DEFAULT;
}


GARMIN_ENUM_NAME(108,color) {
  GARMIN_ENUM_CASE(108,black);
  GARMIN_ENUM_CASE(108,dark_red);
  GARMIN_ENUM_CASE(108,dark_green);
  GARMIN_ENUM_CASE(108,dark_yellow);
  GARMIN_ENUM_CASE(108,dark_blue);
  GARMIN_ENUM_CASE(108,dark_magenta);
  GARMIN_ENUM_CASE(108,dark_cyan);
  GARMIN_ENUM_CASE(108,light_gray);
  GARMIN_ENUM_CASE(108,dark_gray);
  GARMIN_ENUM_CASE(108,red);
  GARMIN_ENUM_CASE(108,green);
  GARMIN_ENUM_CASE(108,yellow);
  GARMIN_ENUM_CASE(108,blue);
  GARMIN_ENUM_CASE(108,magenta);
  GARMIN_ENUM_CASE(108,cyan);
  GARMIN_ENUM_CASE(108,white);
  GARMIN_ENUM_CASE(108,default_color);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d108 ( D108 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",108,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->comment);
  GARMIN_TAGSYM(1,"symbol",x->smbl);
  GARMIN_TAGSTR(1,"display",garmin_d103_dspl(x->dspl));
  GARMIN_TAGSTR(1,"class",garmin_d108_wpt_class(x->wpt_class));
  GARMIN_TAGU8B(1,"subclass",x->subclass,18);
  GARMIN_TAGHEX(1,"attr",x->attr);
  GARMIN_TAGSTR(1,"color",garmin_d108_color(x->color));
  if ( x->alt  < 1.0e24 ) GARMIN_TAGF32(1,"altitude",x->alt);
  if ( x->dpth < 1.0e24 ) GARMIN_TAGF32(1,"depth",x->dpth);
  if ( x->dist < 1.0e24 ) GARMIN_TAGF32(1,"distance",x->dist);
  GARMIN_TAGSTR(1,"facility",x->facility);
  GARMIN_TAGSTR(1,"city",x->city);
  GARMIN_TAGSTR(1,"addr",x->addr);
  GARMIN_TAGSTR(1,"cross_road",x->cross_road);
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.10  D109                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d109 ( D109 * x, FILE * fp, int spaces )
{
  uint8 color = x->dspl_color & 0x1f;

  if ( color == 0x1f ) color = D108_default_color;

  open_tag_with_type("waypoint",109,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->comment);
  GARMIN_TAGSYM(1,"symbol",x->smbl);
  GARMIN_TAGSTR(1,"color",garmin_d108_color(color));
  GARMIN_TAGSTR(1,"display",garmin_d103_dspl((x->dspl_color >> 5) & 0x03));
  GARMIN_TAGSTR(1,"class",garmin_d108_wpt_class(x->wpt_class));
  GARMIN_TAGU8B(1,"subclass",x->subclass,18);
  GARMIN_TAGHEX(1,"attr",x->attr);
  GARMIN_TAGHEX(1,"dtyp",x->dtyp);
  GARMIN_TAGU32(1,"ete",x->ete);
  if ( x->alt  < 1.0e24 ) GARMIN_TAGF32(1,"altitude",x->alt);
  if ( x->dpth < 1.0e24 ) GARMIN_TAGF32(1,"depth",x->dpth);
  if ( x->dist < 1.0e24 ) GARMIN_TAGF32(1,"distance",x->dist);
  GARMIN_TAGSTR(1,"facility",x->facility);
  GARMIN_TAGSTR(1,"city",x->city);
  GARMIN_TAGSTR(1,"addr",x->addr);
  GARMIN_TAGSTR(1,"cross_road",x->cross_road);
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.11  D110                                                              */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(110,wpt_class) {
  GARMIN_ENUM_CASE(110,user_wpt);
  GARMIN_ENUM_CASE(110,avtn_apt_wpt);
  GARMIN_ENUM_CASE(110,avtn_int_wpt);
  GARMIN_ENUM_CASE(110,avtn_ndb_wpt);
  GARMIN_ENUM_CASE(110,avtn_vor_wpt);
  GARMIN_ENUM_CASE(110,avtn_arwy_wpt);
  GARMIN_ENUM_CASE(110,avtn_aint_wpt);
  GARMIN_ENUM_CASE(110,avtn_andb_wpt);
  GARMIN_ENUM_CASE(110,map_pnt_wpt);
  GARMIN_ENUM_CASE(110,map_area_wpt);
  GARMIN_ENUM_CASE(110,map_int_wpt);
  GARMIN_ENUM_CASE(110,map_adrs_wpt);
  GARMIN_ENUM_CASE(110,map_line_wpt);
  GARMIN_ENUM_DEFAULT;
}


GARMIN_ENUM_NAME(110,color) {
  GARMIN_ENUM_CASE(110,black);
  GARMIN_ENUM_CASE(110,dark_red);
  GARMIN_ENUM_CASE(110,dark_green);
  GARMIN_ENUM_CASE(110,dark_yellow);
  GARMIN_ENUM_CASE(110,dark_blue);
  GARMIN_ENUM_CASE(110,dark_magenta);
  GARMIN_ENUM_CASE(110,dark_cyan);
  GARMIN_ENUM_CASE(110,light_gray);
  GARMIN_ENUM_CASE(110,dark_gray);
  GARMIN_ENUM_CASE(110,red);
  GARMIN_ENUM_CASE(110,green);
  GARMIN_ENUM_CASE(110,yellow);
  GARMIN_ENUM_CASE(110,blue);
  GARMIN_ENUM_CASE(110,magenta);
  GARMIN_ENUM_CASE(110,cyan);
  GARMIN_ENUM_CASE(110,white);
  GARMIN_ENUM_CASE(110,transparent);
  GARMIN_ENUM_DEFAULT;
}


GARMIN_ENUM_NAME(110,dspl) {
  GARMIN_ENUM_CASE(110,symbol_name);
  GARMIN_ENUM_CASE(110,symbol_only);
  GARMIN_ENUM_CASE(110,symbol_comment);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d110 ( D110 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",110,fp,spaces);
  GARMIN_TAGHEX(1,"dtyp",x->dtyp);
  GARMIN_TAGSTR(1,"wpt_class",garmin_d110_wpt_class(x->wpt_class));
  GARMIN_TAGSTR(1,"color",garmin_d110_color((x->dspl_color) & 0x1f));
  GARMIN_TAGSTR(1,"display",garmin_d110_dspl((x->dspl_color >> 5) & 0x03));
  GARMIN_TAGHEX(1,"attr",x->attr);
  GARMIN_TAGSYM(1,"symbol",x->smbl);
  GARMIN_TAGPOS(1,"position",x->posn);
  if ( x->alt  < 1.0e24 ) GARMIN_TAGF32(1,"altitude",x->alt);
  if ( x->dpth < 1.0e24 ) GARMIN_TAGF32(1,"depth",x->dpth);
  if ( x->dist < 1.0e24 ) GARMIN_TAGF32(1,"distance",x->dist);
  if ( x->temp < 1.0e24 ) GARMIN_TAGF32(1,"temperature",x->temp);
  GARMIN_TAGSTR(1,"state",x->state);
  GARMIN_TAGSTR(1,"country_code",x->cc);
  GARMIN_TAGU32(1,"ete",x->ete);
  if ( x->time != 0xffffffff ) GARMIN_TAGU32(1,"time",x->time);
  GARMIN_TAGHEX(1,"category",x->wpt_cat);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGSTR(1,"comment",x->comment);
  GARMIN_TAGSTR(1,"facility",x->facility);
  GARMIN_TAGSTR(1,"city",x->city);
  GARMIN_TAGSTR(1,"address_number",x->addr);
  GARMIN_TAGSTR(1,"cross_road",x->cross_road);
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.12  D120                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d120 ( D120 * x, FILE * fp, int spaces )
{
  GARMIN_TAGSTR(0,"waypoint_category",x->name);
}

 
/* --------------------------------------------------------------------------*/
/* 7.4.13  D150                                                              */
/* --------------------------------------------------------------------------*/
 
 
GARMIN_ENUM_NAME(150,wpt_class) {
  GARMIN_ENUM_CASE(150,apt_wpt_class);
  GARMIN_ENUM_CASE(150,int_wpt_class);
  GARMIN_ENUM_CASE(150,ndb_wpt_class);
  GARMIN_ENUM_CASE(150,vor_wpt_class);
  GARMIN_ENUM_CASE(150,usr_wpt_class);
  GARMIN_ENUM_CASE(150,rwy_wpt_class);
  GARMIN_ENUM_CASE(150,aint_wpt_class);
  GARMIN_ENUM_CASE(150,locked_wpt_class);
  GARMIN_ENUM_DEFAULT;
}

		  
static void
garmin_print_d150 ( D150 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",150,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGSTR(1,"class",garmin_d150_wpt_class(x->wpt_class));
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  if ( x->wpt_class != D150_usr_wpt_class ) {
    GARMIN_TAGSTR(1,"city",x->city);
    GARMIN_TAGSTR(1,"state",x->state);
    GARMIN_TAGSTR(1,"facility_name",x->name);
    GARMIN_TAGSTR(1,"country_code",x->cc);
  }
  if ( x->wpt_class == D150_apt_wpt_class ) {
    GARMIN_TAGINT(1,"altitude",x->alt);
  }
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.14  D151                                                              */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(151,wpt_class) {
  GARMIN_ENUM_CASE(151,apt_wpt_class);
  GARMIN_ENUM_CASE(151,vor_wpt_class);
  GARMIN_ENUM_CASE(151,usr_wpt_class);
  GARMIN_ENUM_CASE(151,locked_wpt_class);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d151 ( D151 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",151,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGSTR(1,"class",garmin_d151_wpt_class(x->wpt_class));
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  GARMIN_TAGF32(1,"proximity_distance",x->dst);
  if ( x->wpt_class != D151_usr_wpt_class ) {
    GARMIN_TAGSTR(1,"city",x->city);
    GARMIN_TAGSTR(1,"state",x->state);
    GARMIN_TAGSTR(1,"facility_name",x->name);
    GARMIN_TAGSTR(1,"country_code",x->cc);
  }
  if ( x->wpt_class == D151_apt_wpt_class ) {
    GARMIN_TAGINT(1,"altitude",x->alt);
  }
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.15  D152                                                              */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(152,wpt_class) {
  GARMIN_ENUM_CASE(152,apt_wpt_class);
  GARMIN_ENUM_CASE(152,int_wpt_class);
  GARMIN_ENUM_CASE(152,ndb_wpt_class);
  GARMIN_ENUM_CASE(152,vor_wpt_class);
  GARMIN_ENUM_CASE(152,usr_wpt_class);
  GARMIN_ENUM_CASE(152,locked_wpt_class);
  GARMIN_ENUM_DEFAULT;
}

		  
static void
garmin_print_d152 ( D152 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",152,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGSTR(1,"class",garmin_d152_wpt_class(x->wpt_class));
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  GARMIN_TAGF32(1,"proximity_distance",x->dst);
  if ( x->wpt_class != D152_usr_wpt_class ) {
    GARMIN_TAGSTR(1,"city",x->city);
    GARMIN_TAGSTR(1,"state",x->state);
    GARMIN_TAGSTR(1,"facility_name",x->name);
    GARMIN_TAGSTR(1,"country_code",x->cc);
  }
  if ( x->wpt_class == D152_apt_wpt_class ) {
    GARMIN_TAGINT(1,"altitude",x->alt);
  }
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.16  D154                                                              */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(154,wpt_class) {
  GARMIN_ENUM_CASE(154,apt_wpt_class);
  GARMIN_ENUM_CASE(154,int_wpt_class);
  GARMIN_ENUM_CASE(154,ndb_wpt_class);
  GARMIN_ENUM_CASE(154,vor_wpt_class);
  GARMIN_ENUM_CASE(154,usr_wpt_class);
  GARMIN_ENUM_CASE(154,rwy_wpt_class);
  GARMIN_ENUM_CASE(154,aint_wpt_class);
  GARMIN_ENUM_CASE(154,andb_wpt_class);
  GARMIN_ENUM_CASE(154,sym_wpt_class);
  GARMIN_ENUM_CASE(154,locked_wpt_class);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d154 ( D154 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",154,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGSTR(1,"class",garmin_d154_wpt_class(x->wpt_class));
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  GARMIN_TAGF32(1,"proximity_distance",x->dst);
  if ( x->wpt_class != D154_usr_wpt_class ) {
    GARMIN_TAGSTR(1,"city",x->city);
    GARMIN_TAGSTR(1,"state",x->state);
    GARMIN_TAGSTR(1,"facility_name",x->name);
    GARMIN_TAGSTR(1,"country_code",x->cc);
  }
  if ( x->wpt_class == D154_apt_wpt_class ) {
    GARMIN_TAGINT(1,"altitude",x->alt);
  }
  GARMIN_TAGSYM(1,"symbol",x->smbl);
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.17  D155                                                              */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(155,wpt_class) {
  GARMIN_ENUM_CASE(155,apt_wpt_class);
  GARMIN_ENUM_CASE(155,int_wpt_class);
  GARMIN_ENUM_CASE(155,ndb_wpt_class);
  GARMIN_ENUM_CASE(155,vor_wpt_class);
  GARMIN_ENUM_CASE(155,usr_wpt_class);
  GARMIN_ENUM_CASE(155,locked_wpt_class);
  GARMIN_ENUM_DEFAULT;
}


GARMIN_ENUM_NAME(155,dspl) {
  GARMIN_ENUM_CASE(155,dspl_smbl_only);
  GARMIN_ENUM_CASE(155,dspl_smbl_name);
  GARMIN_ENUM_CASE(155,dspl_smbl_cmnt);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d155 ( D155 * x, FILE * fp, int spaces )
{
  open_tag_with_type("waypoint",155,fp,spaces);
  GARMIN_TAGSTR(1,"ident",x->ident);
  GARMIN_TAGSTR(1,"class",garmin_d155_wpt_class(x->wpt_class));
  GARMIN_TAGPOS(1,"position",x->posn);
  GARMIN_TAGSTR(1,"comment",x->cmnt);
  GARMIN_TAGF32(1,"proximity_distance",x->dst);
  if ( x->wpt_class != D155_usr_wpt_class ) {
    GARMIN_TAGSTR(1,"city",x->city);
    GARMIN_TAGSTR(1,"state",x->state);
    GARMIN_TAGSTR(1,"facility_name",x->name);
    GARMIN_TAGSTR(1,"country_code",x->cc);
  }
  if ( x->wpt_class == D155_apt_wpt_class ) {
    GARMIN_TAGINT(1,"altitude",x->alt);
  }
  GARMIN_TAGSYM(1,"symbol",x->smbl);
  GARMIN_TAGSTR(1,"display",garmin_d155_dspl(x->dspl));
  close_tag("waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.18  D200                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d200 ( D200 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<route_header type=\"200\" number=\"%d\"/>\n", *x);
}


/* --------------------------------------------------------------------------*/
/* 7.4.19  D201                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d201 ( D201 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<route_header type=\"201\" number=\"%d\">%s</route_header>\n",
	  x->nmbr,x->cmnt);
}


/* --------------------------------------------------------------------------*/
/* 7.4.20  D202                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d202 ( D202 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<route_header type=\"202\" ident=\"%s\"/>\n",
	  x->rte_ident);
}


/* --------------------------------------------------------------------------*/
/* 7.4.21  D210                                                              */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(210,class) {
  GARMIN_ENUM_CASE(210,line);
  GARMIN_ENUM_CASE(210,link);
  GARMIN_ENUM_CASE(210,net);
  GARMIN_ENUM_CASE(210,direct);
  GARMIN_ENUM_CASE(210,snap);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d210 ( D210 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<route_link type=\"210\" class=\"%s\" ident=\"%s\">\n",
	  garmin_d210_class(x->link_class),x->ident);
  GARMIN_TAGU8B(1,"route_link_subclass",x->subclass,18);
  close_tag("route_link",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.22  D300                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d300 ( D300 * p, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<point type=\"300\"");
  garmin_print_dtime(p->time,fp,"time");
  garmin_print_dpos(&p->posn,fp);
  if ( p->new_trk != 0 ) {
    fprintf(fp," new=\"true\"");
  }
  fprintf(fp,"/>\n");
}


/* --------------------------------------------------------------------------*/
/* 7.4.23  D301                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d301 ( D301 * p, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<point type=\"301\"");
  garmin_print_dtime(p->time,fp,"time");
  garmin_print_dpos(&p->posn,fp);
  garmin_print_dfloat32(p->alt,fp,"alt");
  garmin_print_dfloat32(p->dpth,fp,"depth");
  if ( p->new_trk != 0 ) {
    fprintf(fp," new=\"true\"");
  }
  fprintf(fp,"/>\n");
}


/* --------------------------------------------------------------------------*/
/* 7.4.24  D302                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d302 ( D302 * p, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<point type=\"302\"");
  garmin_print_dtime(p->time,fp,"time");
  garmin_print_dpos(&p->posn,fp);
  garmin_print_dfloat32(p->alt,fp,"alt");
  garmin_print_dfloat32(p->dpth,fp,"depth");
  garmin_print_dfloat32(p->temp,fp,"temperature");
  if ( p->new_trk != 0 ) {
    fprintf(fp," new=\"true\"");
  }
  fprintf(fp,"/>\n");

}


/* --------------------------------------------------------------------------*/
/* 7.4.25  D303                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d303 ( D303 * p, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<point type=\"303\"");
  garmin_print_dtime(p->time,fp,"time");
  garmin_print_dpos(&p->posn,fp);
  garmin_print_dfloat32(p->alt,fp,"alt");
  if ( p->heart_rate != 0 ) {
    fprintf(fp," hr=\"%d\"",p->heart_rate);
  }
  fprintf(fp,"/>\n");
}


/* --------------------------------------------------------------------------*/
/* 7.4.26  D304                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d304 ( D304 * p, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<point type=\"304\"");
  garmin_print_dtime(p->time,fp,"time");
  garmin_print_dpos(&p->posn,fp);
  garmin_print_dfloat32(p->alt,fp,"alt");
  garmin_print_dfloat32(p->distance,fp,"distance");
  if ( p->heart_rate != 0 ) {
    fprintf(fp," hr=\"%d\"", p->heart_rate);
  }
  if ( p->cadence != 0xff ) {
    fprintf(fp, " cadence=\"%d\"", p->cadence);
  }
  if ( p->sensor != 0 ) {
    fprintf(fp, " sensor=\"true\"");
  }
  fprintf(fp,"/>\n");
}


/* --------------------------------------------------------------------------*/
/* 7.4.27  D310                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d310 ( D310 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<track type=\"310\" ident=\"%s\" color=\"%s\" "
	  "display=\"%s\"/>\n",
	  x->trk_ident,garmin_d108_color(x->color),
	  (x->dspl) ? "true" : "false");
}


/* --------------------------------------------------------------------------*/
/* 7.4.28  D311                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d311 ( D311 * h, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<track type=\"311\" index=\"%d\"/>\n",h->index);
}


/* --------------------------------------------------------------------------*/
/* 7.4.29  D312                                                              */
/* --------------------------------------------------------------------------*/

GARMIN_ENUM_NAME(312,color) {
  GARMIN_ENUM_CASE(312,black);
  GARMIN_ENUM_CASE(312,dark_red);
  GARMIN_ENUM_CASE(312,dark_green);
  GARMIN_ENUM_CASE(312,dark_yellow);
  GARMIN_ENUM_CASE(312,dark_blue);
  GARMIN_ENUM_CASE(312,dark_magenta);
  GARMIN_ENUM_CASE(312,dark_cyan);
  GARMIN_ENUM_CASE(312,light_gray);
  GARMIN_ENUM_CASE(312,dark_gray);
  GARMIN_ENUM_CASE(312,red);
  GARMIN_ENUM_CASE(312,green);
  GARMIN_ENUM_CASE(312,yellow);
  GARMIN_ENUM_CASE(312,blue);
  GARMIN_ENUM_CASE(312,magenta);
  GARMIN_ENUM_CASE(312,cyan);
  GARMIN_ENUM_CASE(312,white);
  GARMIN_ENUM_CASE(312,transparent);
  GARMIN_ENUM_CASE(312,default_color);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d312 ( D312 * h,
		    FILE *              fp,
		    int                 spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<track type=\"312\" ident=\"%s\" color=\"%s\" "
	  "display=\"%s\"/>\n",
	  h->trk_ident,
	  garmin_d312_color(h->color),
	  (h->dspl) ? "true" : "false");
}


/* --------------------------------------------------------------------------*/
/* 7.4.30  D400                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d400 ( D400 * x, FILE * fp, int spaces )
{
  open_tag_with_type("proximity_waypoint",400,fp,spaces);
  garmin_print_d100(&x->wpt,fp,spaces+1);
  GARMIN_TAGF32(1,"distance",x->dst);
  close_tag("proximity_waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.31  D403                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d403 ( D403 * x, FILE * fp, int spaces )
{
  open_tag_with_type("proximity_waypoint",403,fp,spaces);
  garmin_print_d103(&x->wpt,fp,spaces+1);
  GARMIN_TAGF32(1,"distance",x->dst);
  close_tag("proximity_waypoint",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.32  D450                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d450 ( D450 * x, FILE * fp, int spaces )
{
  open_tag_with_type("proximity_waypoint",450,fp,spaces);
  GARMIN_TAGINT(1,"index",x->idx);
  garmin_print_d150(&x->wpt,fp,spaces+1);
  GARMIN_TAGF32(1,"distance",x->dst);
  close_tag("proximity_waypoint",fp,spaces);

}


/* --------------------------------------------------------------------------*/
/* 7.4.33  D500                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d500 ( D500 * x, FILE * fp, int spaces )
{
  open_tag_with_type("almanac",500,fp,spaces);
  GARMIN_TAGINT(1,"wn",x->wn);
  GARMIN_TAGF32(1,"toa",x->toa);
  GARMIN_TAGF32(1,"afo",x->af0);
  GARMIN_TAGF32(1,"af1",x->af1);
  GARMIN_TAGF32(1,"e",x->e);
  GARMIN_TAGF32(1,"sqrta",x->sqrta);
  GARMIN_TAGF32(1,"m0",x->m0);
  GARMIN_TAGF32(1,"w",x->w);
  GARMIN_TAGF32(1,"omg0",x->omg0);
  GARMIN_TAGF32(1,"odot",x->odot);
  GARMIN_TAGF32(1,"i",x->i);
  close_tag("almanac",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.34  D501                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d501 ( D501 * x, FILE * fp, int spaces )
{
  open_tag_with_type("almanac",501,fp,spaces);
  GARMIN_TAGINT(1,"wn",x->wn);
  GARMIN_TAGF32(1,"toa",x->toa);
  GARMIN_TAGF32(1,"afo",x->af0);
  GARMIN_TAGF32(1,"af1",x->af1);
  GARMIN_TAGF32(1,"e",x->e);
  GARMIN_TAGF32(1,"sqrta",x->sqrta);
  GARMIN_TAGF32(1,"m0",x->m0);
  GARMIN_TAGF32(1,"w",x->w);
  GARMIN_TAGF32(1,"omg0",x->omg0);
  GARMIN_TAGF32(1,"odot",x->odot);
  GARMIN_TAGF32(1,"i",x->i);
  GARMIN_TAGINT(1,"hlth",x->hlth);
  close_tag("almanac",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.35  D550                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d550 ( D550 * x, FILE * fp, int spaces )
{
  open_tag_with_type("almanac",550,fp,spaces);
  GARMIN_TAGINT(1,"svid",x->svid);
  GARMIN_TAGINT(1,"wn",x->wn);
  GARMIN_TAGF32(1,"toa",x->toa);
  GARMIN_TAGF32(1,"afo",x->af0);
  GARMIN_TAGF32(1,"af1",x->af1);
  GARMIN_TAGF32(1,"e",x->e);
  GARMIN_TAGF32(1,"sqrta",x->sqrta);
  GARMIN_TAGF32(1,"m0",x->m0);
  GARMIN_TAGF32(1,"w",x->w);
  GARMIN_TAGF32(1,"omg0",x->omg0);
  GARMIN_TAGF32(1,"odot",x->odot);
  GARMIN_TAGF32(1,"i",x->i);
  close_tag("almanac",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.36  D551                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d551 ( D551 * x, FILE * fp, int spaces )
{
  open_tag_with_type("almanac",551,fp,spaces);
  GARMIN_TAGINT(1,"svid",x->svid);
  GARMIN_TAGINT(1,"wn",x->wn);
  GARMIN_TAGF32(1,"toa",x->toa);
  GARMIN_TAGF32(1,"afo",x->af0);
  GARMIN_TAGF32(1,"af1",x->af1);
  GARMIN_TAGF32(1,"e",x->e);
  GARMIN_TAGF32(1,"sqrta",x->sqrta);
  GARMIN_TAGF32(1,"m0",x->m0);
  GARMIN_TAGF32(1,"w",x->w);
  GARMIN_TAGF32(1,"omg0",x->omg0);
  GARMIN_TAGF32(1,"odot",x->odot);
  GARMIN_TAGF32(1,"i",x->i);
  GARMIN_TAGINT(1,"hlth",x->hlth);
  close_tag("almanac",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.37  D600                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d600 ( D600 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<date_time type=\"600\">"
	  "%04d-%02d-%02d %02d:%02d:%02d</date_time>\n",
	  x->year,x->month,x->day,x->hour,x->minute,x->second);
}


/* --------------------------------------------------------------------------*/
/* 7.4.38  D650                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d650 ( D650 * x, FILE * fp, int spaces )
{
  open_tag("flightbook type=\"650\"",fp,spaces);
  GARMIN_TAGU32(1,"takeoff_time",x->takeoff_time + TIME_OFFSET);
  GARMIN_TAGU32(1,"landing_time",x->takeoff_time + TIME_OFFSET);
  GARMIN_TAGPOS(1,"takeoff_position",x->takeoff_posn);
  GARMIN_TAGPOS(1,"landing_position",x->takeoff_posn);
  GARMIN_TAGU32(1,"night_time",x->night_time);
  GARMIN_TAGU32(1,"num_landings",x->num_landings);
  GARMIN_TAGF32(1,"max_speed",x->max_speed);
  GARMIN_TAGF32(1,"max_alt",x->max_alt);
  GARMIN_TAGF32(1,"distance",x->distance);
  GARMIN_TAGSTR(1,"cross_country_flag",
		(x->cross_country_flag != 0) ? "true" : "false");
  GARMIN_TAGSTR(1,"departure_name",x->departure_name);
  GARMIN_TAGSTR(1,"departure_ident",x->departure_ident);
  GARMIN_TAGSTR(1,"arrival_name",x->arrival_name);
  GARMIN_TAGSTR(1,"arrival_ident",x->arrival_ident);
  GARMIN_TAGSTR(1,"ac_id",x->ac_id);
  close_tag("flightbook",fp,spaces);
}


/* ------------------------------------------------------------------------- */
/* 7.4.39  D700                                                              */
/* ------------------------------------------------------------------------- */

static void
garmin_print_d700 ( D700 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<position type=\"700\" lat=\"%f\" lon=\"%f\"/>\n",
	  RAD2DEG(x->lat),RAD2DEG(x->lon));
}


/* --------------------------------------------------------------------------*/
/* 7.4.40  D800                                                              */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(800,fix) {
  GARMIN_ENUM_CASE(800,unusable);
  GARMIN_ENUM_CASE(800,invalid);
  GARMIN_ENUM_CASE(800,2D);
  GARMIN_ENUM_CASE(800,3D);
  GARMIN_ENUM_CASE(800,2D_diff);
  GARMIN_ENUM_CASE(800,3D_diff);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d800 ( D800 * x, FILE * fp, int spaces )
{
  open_tag("pvt type=\"800\"",fp,spaces);
  GARMIN_TAGF32(1,"alt",x->alt);
  GARMIN_TAGF32(1,"epe",x->epe);
  GARMIN_TAGF32(1,"eph",x->eph);
  GARMIN_TAGF32(1,"epv",x->epv);
  GARMIN_TAGSTR(1,"position_fix",garmin_d800_fix(x->fix));
  garmin_print_d700(&x->posn,fp,spaces+1);
  print_spaces(fp,spaces+1);
  fprintf(fp,"<velocity east=\"");
  garmin_print_float32(x->east,fp);
  fprintf(fp,"\" north=\"");
  garmin_print_float32(x->north,fp);
  fprintf(fp,"\" up=\"");
  garmin_print_float32(x->up,fp);
  fprintf(fp,"\"/>\n");
  GARMIN_TAGF32(1,"msl_height",x->msl_hght);
  GARMIN_TAGINT(1,"leap_seconds",x->leap_scnds);
  GARMIN_TAGU32(1,"week_number_days",x->wn_days);
  GARMIN_TAGF64(1,"time_of_week",x->tow);
  close_tag("pvt",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.41  D906                                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d906 ( D906 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<lap type=\"906\"");
  garmin_print_dtime(x->start_time,fp,"start");
  garmin_print_ddist(x->total_time,x->total_distance,fp);
  fprintf(fp,">\n");

  if ( x->begin.lat != 0x7fffffff && x->begin.lon != 0x7fffffff ) {
    GARMIN_TAGPOS(1,"begin_pos",x->begin);
  }
  if ( x->end.lat != 0x7fffffff && x->end.lon != 0x7fffffff ) {
    GARMIN_TAGPOS(1,"end_pos",x->end);
  }

  GARMIN_TAGINT(1,"calories",x->calories);

  switch ( x->track_index ) {
  case 0xff:
    GARMIN_TAGSTR(1,"track_index","default");
    break;
  case 0xfe:
  case 0xfd:
    GARMIN_TAGSTR(1,"track_index","none");
    break;
  default:
    GARMIN_TAGINT(1,"track_index",x->track_index);
    break;
  }

  close_tag("lap",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.42  D1000                                                             */
/* --------------------------------------------------------------------------*/


static void garmin_print_d1002 ( D1002 * x, FILE * fp, int spaces );


GARMIN_ENUM_NAME(1000,sport_type) {
  GARMIN_ENUM_CASE(1000,running);
  GARMIN_ENUM_CASE(1000,biking);
  GARMIN_ENUM_CASE(1000,other);
  GARMIN_ENUM_DEFAULT;
}


GARMIN_ENUM_NAME(1000,program_type) {
  GARMIN_ENUM_CASE(1000,none);
  GARMIN_ENUM_CASE(1000,virtual_partner);
  GARMIN_ENUM_CASE(1000,workout);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d1000 ( D1000 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<run type=\"1000\" track=\"%d\" sport=\"%s\">\n",
	  x->track_index,garmin_d1000_sport_type(x->sport_type));
  print_spaces(fp,spaces+1);
  fprintf(fp,"<laps first=\"%u\" last=\"%u\"/>\n",
	  x->first_lap_index, x->last_lap_index);
  GARMIN_TAGSTR(1,"program_type",
		garmin_d1000_program_type(x->program_type));
  if ( x->program_type == D1000_virtual_partner ) {
    print_spaces(fp,spaces+1);
    fprintf(fp,"<virtual_partner time=\"%u\" distance=\"%f\"/>\n",
	    x->virtual_partner.time, x->virtual_partner.distance);
  }
  if ( x->program_type == D1000_workout ) {
    garmin_print_d1002(&x->workout,fp,spaces+1);
  }
  close_tag("run",fp,spaces);

  garmin_print_d1002(&x->workout,fp,spaces+1);
}


/* --------------------------------------------------------------------------*/
/* 7.4.43  D1001                                                             */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(1001,intensity) {
  GARMIN_ENUM_CASE(1001,active);
  GARMIN_ENUM_CASE(1001,rest);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d1001 ( D1001 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<lap type=\"1001\" index=\"%d\"",x->index);
  garmin_print_dtime(x->start_time,fp,"start");
  garmin_print_ddist(x->total_time,x->total_dist,fp);
  fprintf(fp,">\n");
  if ( x->begin.lat != 0x7fffffff && x->begin.lon != 0x7fffffff ) {
    GARMIN_TAGPOS(1,"begin_pos",x->begin);
  }
  if ( x->end.lat != 0x7fffffff && x->end.lon != 0x7fffffff ) {
    GARMIN_TAGPOS(1,"end_pos",x->end);
  }
  GARMIN_TAGF32(1,"max_speed",x->max_speed);
  GARMIN_TAGINT(1,"calories",x->calories);
  if ( x->avg_heart_rate != 0 ) {
    GARMIN_TAGINT(1,"avg_hr",x->avg_heart_rate);
  }
  if ( x->max_heart_rate != 0 ) {
    GARMIN_TAGINT(1,"max_hr",x->max_heart_rate);
  }
  GARMIN_TAGSTR(1,"intensity",garmin_d1001_intensity(x->intensity));
  close_tag("lap",fp,spaces);  
}


/* --------------------------------------------------------------------------*/
/* 7.4.44  D1002                                                             */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(1002,duration_type) {
  GARMIN_ENUM_CASE(1002,time);
  GARMIN_ENUM_CASE(1002,distance);
  GARMIN_ENUM_CASE(1002,heart_rate_less_than);
  GARMIN_ENUM_CASE(1002,heart_rate_greater_than);
  GARMIN_ENUM_CASE(1002,calories_burned);
  GARMIN_ENUM_CASE(1002,open);
  GARMIN_ENUM_CASE(1002,repeat);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d1002 ( D1002 * x, FILE * fp, int spaces )
{
  int i;

  print_spaces(fp,spaces);
  fprintf(fp,"<workout type=\"1002\" name=\"%s\" steps=\"%d\" "
	  "sport_type=\"%s\"",
	  x->name,x->num_valid_steps,garmin_d1000_sport_type(x->sport_type));
  if ( x->num_valid_steps > 0 ) {
    fprintf(fp,">\n");
    for ( i = 0; i < x->num_valid_steps; i++ ) {
      print_spaces(fp,spaces+1);
      fprintf(fp,"<step name=\"%s\">\n",x->steps[i].custom_name);
      GARMIN_TAGSTR(1,"intensity",
		    garmin_d1001_intensity(x->steps[i].intensity));
      print_spaces(fp,spaces+1);
      fprintf(fp,"<duration type=\"%s\">%d</duration>\n",
	      garmin_d1002_duration_type(x->steps[i].duration_type),
	      x->steps[i].duration_value);
      print_spaces(fp,spaces+1);
      if ( x->steps[i].duration_type == D1002_repeat ) {
	switch ( x->steps[i].target_type ) {
	case 0:
	  fprintf(fp,"<target type=\"speed_zone\" "
		  "value=\"%d\" low=\"%f m/s\" high=\"%f m/s\"/>\n",
		  x->steps[i].target_value,
		  x->steps[i].target_custom_zone_low,
		  x->steps[i].target_custom_zone_high);
	  break;
	case 1:
	  fprintf(fp,"<target type=\"heart_rate_zone\" "
		  "value=\"%d\" low=\"%f%s\" high=\"%f%s\"/>\n",
		  x->steps[i].target_value,
		  x->steps[i].target_custom_zone_low,
		  (x->steps[i].target_custom_zone_low <= 100) ? "%" : " bpm",
		  x->steps[i].target_custom_zone_high,
		  (x->steps[i].target_custom_zone_high <= 100) ? "%" : " bpm");
	  break;
	case 2:
	  fprintf(fp,"<target type=\"open\"/>\n");
	  break;
	default:
	  break;
	}
      } else {
	fprintf(fp,"<target type=\"repetitions\" value=\"%d\"/>\n",
		x->steps[i].target_value);
      }
      close_tag("step",fp,spaces+1);
    }
    close_tag("workout",fp,spaces);
  } else {
    fprintf(fp,"/>\n");
  }
}


/* --------------------------------------------------------------------------*/
/* 7.4.45  D1003                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d1003 ( D1003 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<workout_occurrence type=\"1003\" name=\"%s\" day=\"%u\"/>\n",
	  x->workout_name,x->day);
}


/* --------------------------------------------------------------------------*/
/* 7.4.46  D1004                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d1004 ( D1004 *  d, FILE * fp, int spaces )
{
  int i;
  int j;

  print_spaces(fp,spaces);
  fprintf(fp,
	  "<fitness_user_profile type=\"1004\" weight=\"%f\" "
	  "birth_date=\"%04d-%02d-%02d\" gender=\"%s\">\n",
	  d->weight,
	  d->birth_year,
	  d->birth_month,
	  d->birth_day,
	  (d->gender == D1004_male) ? "male" : "female");
  open_tag("activities",fp,spaces+1);
  for ( i = 0; i < 3; i++ ) {
    print_spaces(fp,spaces+2);
    fprintf(fp,"<activity gear_weight=\"%f\" max_hr=\"%d\">\n",
	    d->activities[i].gear_weight,
	    d->activities[i].max_heart_rate);
    open_tag("hr_zones",fp,spaces+3);
    for ( j = 0; j < 5; j++ ) {
      print_spaces(fp,spaces+4);
      fprintf(fp,"<hr_zone low=\"%d\" high=\"%d\"/>\n",
	      d->activities[i].heart_rate_zones[j].low_heart_rate,
	      d->activities[i].heart_rate_zones[j].high_heart_rate);
    }
    close_tag("hr_zones",fp,spaces+3);
    open_tag("speed_zones",fp,spaces+3);
    for ( j = 0; j < 10; j++ ) {
      print_spaces(fp,spaces+4);
      fprintf(fp,"<speed_zone low=\"%f\" high=\"%f\" name=\"%s\"/>\n",
	      d->activities[i].speed_zones[j].low_speed,
	      d->activities[i].speed_zones[j].high_speed,
	      d->activities[i].speed_zones[j].name);
    }
    close_tag("speed_zones",fp,spaces+3);
    close_tag("activity",fp,spaces+2);
  }
  close_tag("activities",fp,spaces+1);
  close_tag("fitness_user_profile",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.47  D1005                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d1005 ( D1005 * limits, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,
	  "<workout_limits type=\"1005\" workouts=\"%d\" unscheduled=\"%d\" "
	  "occurrences=\"%d\"/>\n",
	  limits->max_workouts,
	  limits->max_unscheduled_workouts,
	  limits->max_occurrences);
}


/* --------------------------------------------------------------------------*/
/* 7.4.48  D1006                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d1006 ( D1006 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<course type=\"1006\" index=\"%d\" name=\"%s\" "
	  "track_index=\"%d\"/>\n",
	  x->index,
	  x->course_name,
	  x->track_index);
}


/* --------------------------------------------------------------------------*/
/* 7.4.49  D1007                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d1007 ( D1007 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<course_lap type=\"1007\" course_index=\"%d\" lap_index=\"%d\"",
	  x->course_index,
	  x->lap_index);
  garmin_print_ddist(x->total_time,x->total_dist,fp);
  fprintf(fp,">\n");
  if ( x->begin.lat != 0x7fffffff && x->begin.lon != 0x7fffffff ) {
    GARMIN_TAGPOS(1,"begin_pos",x->begin);
  }
  if ( x->end.lat != 0x7fffffff && x->end.lon != 0x7fffffff ) {
    GARMIN_TAGPOS(1,"end_pos",x->end);
  }
  if ( x->avg_heart_rate != 0 ) GARMIN_TAGINT(1,"avg_hr",x->avg_heart_rate);
  if ( x->max_heart_rate != 0 ) GARMIN_TAGINT(1,"max_hr",x->max_heart_rate);
  if ( x->avg_cadence != 0xff ) GARMIN_TAGINT(1,"avg_cadence",x->avg_cadence);
  GARMIN_TAGSTR(1,"intensity",garmin_d1001_intensity(x->intensity));

  close_tag("course_lap",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.50  D1008                                                             */
/* --------------------------------------------------------------------------*/


static void
garmin_print_d1008 ( D1008 * w, FILE * fp, int spaces )
{
  /* For some reason, D1008 is identical to D1002. */

  garmin_print_d1002((D1002 *)w,fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.51  D1009                                                             */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(1009,multisport) {
  GARMIN_ENUM_CASE(1009,no);
  GARMIN_ENUM_CASE(1009,yes);
  GARMIN_ENUM_CASE(1009,yesAndLastInGroup);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d1009 ( D1009 * run, FILE * fp, int spaces )
{
  int npt = 0;

  print_spaces(fp,spaces);
  fprintf(fp,"<run type=\"1009\" track=\"%d\" sport=\"%s\" "
	  "multisport=\"%s\">\n",
	  run->track_index,garmin_d1000_sport_type(run->sport_type),
	  garmin_d1009_multisport(run->multisport));
  print_spaces(fp,spaces+1);
  fprintf(fp,"<laps first=\"%u\" last=\"%u\"/>\n",
	  run->first_lap_index, run->last_lap_index);

  if ( run->program_type != 0 ) {
    print_spaces(fp,spaces+1);
    fprintf(fp,"<program_type>");
    if ( run->program_type & 0x01 ) {
      fprintf(fp,"%s%s",(npt++) ? ", " : "", "virtual_partner");
    }
    if ( run->program_type & 0x02 ) {
      fprintf(fp,"%s%s",(npt++) ? ", " : "", "workout");
    }
    if ( run->program_type & 0x04 ) {
      fprintf(fp,"%s%s",(npt++) ? ", " : "", "quick_workout");
    } 
    if ( run->program_type & 0x08 ) {
      fprintf(fp,"%s%s",(npt++) ? ", " : "", "course");
    }
    if ( run->program_type & 0x10 ) {
      fprintf(fp,"%s%s",(npt++) ? ", " : "", "interval_workout");
    }
    if ( run->program_type & 0x20 ) {
      fprintf(fp,"%s%s",(npt++) ? ", " : "", "auto_multisport");
    }
    fprintf(fp,"</program_type>\n");
  }  

  if ( run->program_type & 0x02 ) {
    print_spaces(fp,spaces+1);
    fprintf(fp,"<quick_workout time=\"%u\" distance=\"%f\"/>\n",
	    run->quick_workout.time, run->quick_workout.distance);
  }

  if ( run->program_type & 0x01 ) {
    garmin_print_d1008(&run->workout,fp,spaces+1);
  }

  close_tag("run",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.52  D1010                                                             */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(1010,program_type) {
  GARMIN_ENUM_CASE(1010,none);
  GARMIN_ENUM_CASE(1010,virtual_partner);
  GARMIN_ENUM_CASE(1010,workout);
  GARMIN_ENUM_CASE(1010,auto_multisport);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d1010 ( D1010 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<run type=\"1010\" track=\"%d\" sport=\"%s\" "
	  "multisport=\"%s\">\n",
	  x->track_index,garmin_d1000_sport_type(x->sport_type),
	  garmin_d1009_multisport(x->multisport));
  print_spaces(fp,spaces+1);
  fprintf(fp,"<laps first=\"%u\" last=\"%u\"/>\n",
	  x->first_lap_index, x->last_lap_index);
  GARMIN_TAGSTR(1,"program_type",
		garmin_d1010_program_type(x->program_type));
  if ( x->program_type == D1010_virtual_partner ) {
    print_spaces(fp,spaces+1);
    fprintf(fp,"<virtual_partner time=\"%u\" distance=\"%f\"/>\n",
	    x->virtual_partner.time, x->virtual_partner.distance);
  }
  garmin_print_d1002(&x->workout,fp,spaces+1);
  close_tag("run",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.53  D1011                                                             */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(1011,trigger_method) {
  GARMIN_ENUM_CASE(1011,manual);
  GARMIN_ENUM_CASE(1011,distance);
  GARMIN_ENUM_CASE(1011,location);
  GARMIN_ENUM_CASE(1011,time);
  GARMIN_ENUM_CASE(1011,heart_rate);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d1011 ( D1011 * lap, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<lap type=\"1011\" index=\"%d\"",lap->index);
  garmin_print_dtime(lap->start_time,fp,"start");
  garmin_print_ddist(lap->total_time,lap->total_dist,fp);
  fprintf(fp," trigger=\"%s\">\n",
	  garmin_d1011_trigger_method(lap->trigger_method));
  if ( lap->begin.lat != 0x7fffffff && lap->begin.lon != 0x7fffffff ) {
    GARMIN_TAGPOS(1,"begin_pos",lap->begin);
  }
  if ( lap->end.lat != 0x7fffffff && lap->end.lon != 0x7fffffff ) {
    GARMIN_TAGPOS(1,"end_pos",lap->end);
  }
  GARMIN_TAGF32(1,"max_speed",lap->max_speed);
  GARMIN_TAGINT(1,"calories",lap->calories);
  if ( lap->avg_heart_rate != 0 ) {
    GARMIN_TAGINT(1,"avg_hr",lap->avg_heart_rate);
  }
  if ( lap->max_heart_rate != 0 ) {
    GARMIN_TAGINT(1,"max_hr",lap->max_heart_rate);
  }
  if ( lap->avg_cadence != 0xff ) {
    GARMIN_TAGINT(1,"avg_cadence",lap->avg_cadence);
  }
  GARMIN_TAGSTR(1,"intensity",garmin_d1001_intensity(lap->intensity));
  close_tag("lap",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.54  D1012                                                             */
/* --------------------------------------------------------------------------*/


GARMIN_ENUM_NAME(1012,point_type) {
  GARMIN_ENUM_CASE(1012,generic);
  GARMIN_ENUM_CASE(1012,summit);
  GARMIN_ENUM_CASE(1012,valley);
  GARMIN_ENUM_CASE(1012,water);
  GARMIN_ENUM_CASE(1012,food);
  GARMIN_ENUM_CASE(1012,danger);
  GARMIN_ENUM_CASE(1012,left);
  GARMIN_ENUM_CASE(1012,right);
  GARMIN_ENUM_CASE(1012,straight);
  GARMIN_ENUM_CASE(1012,first_aid);
  GARMIN_ENUM_CASE(1012,fourth_category);
  GARMIN_ENUM_CASE(1012,third_category);
  GARMIN_ENUM_CASE(1012,second_category);
  GARMIN_ENUM_CASE(1012,first_category);
  GARMIN_ENUM_CASE(1012,hors_category);
  GARMIN_ENUM_CASE(1012,sprint);
  GARMIN_ENUM_DEFAULT;
}


static void
garmin_print_d1012 ( D1012 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<course_point type=\"1012\" course_index=\"%d\" "
	  "name=\"%s\" type=\"%s\">\n",
	  x->course_index,x->name,
	  garmin_d1012_point_type(x->point_type));
  GARMIN_TAGU32(1,"track_point_time",x->track_point_time);
  close_tag("course_point",fp,spaces);
}


/* --------------------------------------------------------------------------*/
/* 7.4.55  D1013                                                             */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d1013 ( D1013 * x, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<course_limits type=\"1013\" courses=\"%d\" laps=\"%d\" "
	  "points=\"%d\" track_points=\"%d\"/>\n",
	  x->max_courses,
	  x->max_course_laps,
	  x->max_course_pnt,
	  x->max_course_trk_pnt);
}


/* --------------------------------------------------------------------------*/
/* 7.4.XX  D1015 (Undocumented)                                              */
/* --------------------------------------------------------------------------*/

static void
garmin_print_d1015 ( D1015 * lap, FILE * fp, int spaces )
{
  print_spaces(fp,spaces);
  fprintf(fp,"<lap type=\"1015\" index=\"%d\"",lap->index);
  garmin_print_dtime(lap->start_time,fp,"start");
  garmin_print_ddist(lap->total_time,lap->total_dist,fp);
  fprintf(fp," trigger=\"%s\">\n",
	  garmin_d1011_trigger_method(lap->trigger_method));
  if ( lap->begin.lat != 0x7fffffff && lap->begin.lon != 0x7fffffff ) {
    GARMIN_TAGPOS(1,"begin_pos",lap->begin);
  }
  if ( lap->end.lat != 0x7fffffff && lap->end.lon != 0x7fffffff ) {
    GARMIN_TAGPOS(1,"end_pos",lap->end);
  }
  GARMIN_TAGF32(1,"max_speed",lap->max_speed);
  GARMIN_TAGINT(1,"calories",lap->calories);
  if ( lap->avg_heart_rate != 0 ) {
    GARMIN_TAGINT(1,"avg_hr",lap->avg_heart_rate);
  }
  if ( lap->max_heart_rate != 0 ) {
    GARMIN_TAGINT(1,"max_hr",lap->max_heart_rate);
  }
  if ( lap->avg_cadence != 0xff ) {
    GARMIN_TAGINT(1,"avg_cadence",lap->avg_cadence);
  }
  GARMIN_TAGSTR(1,"intensity",garmin_d1001_intensity(lap->intensity));
  GARMIN_TAGU8B(1,"unknown",lap->unknown,5);  
  close_tag("lap",fp,spaces);
}


/* ========================================================================= */
/* garmin_print_data                                                         */
/* ========================================================================= */

void
garmin_print_data ( garmin_data * d, FILE * fp, int spaces )
{
#define CASE_PRINT(x) \
  case data_D##x: garmin_print_d##x(d->data,fp,spaces); break

  switch ( d->type ) {
  CASE_PRINT(list);
  CASE_PRINT(100);
  CASE_PRINT(101);
  CASE_PRINT(102);
  CASE_PRINT(103);
  CASE_PRINT(104);
  CASE_PRINT(105);
  CASE_PRINT(106);
  CASE_PRINT(107);
  CASE_PRINT(108);
  CASE_PRINT(109);
  CASE_PRINT(110);
  CASE_PRINT(120);
  CASE_PRINT(150);
  CASE_PRINT(151);
  CASE_PRINT(152);
  CASE_PRINT(154);
  CASE_PRINT(155);
  CASE_PRINT(200);
  CASE_PRINT(201);
  CASE_PRINT(202);
  CASE_PRINT(210);
  CASE_PRINT(300);
  CASE_PRINT(301);
  CASE_PRINT(302);
  CASE_PRINT(303);
  CASE_PRINT(304);
  CASE_PRINT(310);
  CASE_PRINT(311);
  CASE_PRINT(312);
  CASE_PRINT(400);
  CASE_PRINT(403);
  CASE_PRINT(450);
  CASE_PRINT(500);
  CASE_PRINT(501);
  CASE_PRINT(550);
  CASE_PRINT(551);
  CASE_PRINT(600);
  CASE_PRINT(650);
  CASE_PRINT(700);
  CASE_PRINT(800);
  CASE_PRINT(906);
  CASE_PRINT(1000);
  CASE_PRINT(1001);
  CASE_PRINT(1002);
  CASE_PRINT(1003);
  CASE_PRINT(1004);
  CASE_PRINT(1005);
  CASE_PRINT(1006);
  CASE_PRINT(1007);
  CASE_PRINT(1008);
  CASE_PRINT(1009);
  CASE_PRINT(1010);
  CASE_PRINT(1011);
  CASE_PRINT(1012);
  CASE_PRINT(1013);
  CASE_PRINT(1015);
  default:
    print_spaces(fp,spaces);
    fprintf(fp,"<data type=\"%d\"/>\n",d->type);
    break;
  }

#undef CASE_PRINT
}


/* ========================================================================= */
/* garmin_print_protocols                                                    */
/* ========================================================================= */

void
garmin_print_protocols ( garmin_unit * garmin, FILE * fp, int spaces )
{
#define PROTO1_AND_DATA(x)                                                \
  do {                                                                    \
    if ( garmin->protocol.x != appl_Anil ) {                              \
      print_spaces(fp,spaces+1);                                          \
      fprintf(fp,"<garmin_" #x                                            \
	      " protocol=\"A%03d\" " #x "=\"D%03d\"/>\n",                 \
	      garmin->protocol.x, garmin->datatype.x);                    \
    }                                                                     \
  } while ( 0 )

#define PROTO2_AND_DATA(x,y)                                              \
  do {                                                                    \
    if ( garmin->protocol.x.y != appl_Anil ) {                            \
      print_spaces(fp,spaces+2);                                          \
      fprintf(fp,"<garmin_" #x "_" #y                                     \
	      " protocol=\"A%03d\" " #y "=\"D%03d\"/>\n",                 \
	      garmin->protocol.x.y, garmin->datatype.x.y);                \
    }                                                                     \
  } while ( 0 )

  open_tag("garmin_protocols",fp,spaces);

  /* Physical */

  print_spaces(fp,spaces+1);
  fprintf(fp,"<garmin_physical protocol=\"P%03d\"/>\n",
	  garmin->protocol.physical);

  /* Link */

  print_spaces(fp,spaces+1);
  fprintf(fp,"<garmin_link protocol=\"L%03d\"/>\n",
	  garmin->protocol.link);

  /* Command */

  print_spaces(fp,spaces+1);
  fprintf(fp,"<garmin_command protocol=\"A%03d\"/>\n",
	  garmin->protocol.command);

  /* Waypoint */

  if ( garmin->protocol.waypoint.waypoint  != appl_Anil ||
       garmin->protocol.waypoint.category  != appl_Anil ||
       garmin->protocol.waypoint.proximity != appl_Anil ) {
    open_tag("garmin_waypoint",fp,spaces+1);
    PROTO2_AND_DATA(waypoint,waypoint);
    PROTO2_AND_DATA(waypoint,category);
    PROTO2_AND_DATA(waypoint,proximity);
    close_tag("garmin_waypoint",fp,spaces+1);
  }

  /* Route */

  if ( garmin->protocol.route != appl_Anil ) {
    print_spaces(fp,spaces+1);
    fprintf(fp,"<garmin_route protocol=\"A%03d\"",
	    garmin->protocol.route);
    if ( garmin->datatype.route.header != data_Dnil ) {
      fprintf(fp," header=\"D%03d\"",
	      garmin->datatype.route.header);
    }
    if ( garmin->datatype.route.waypoint != data_Dnil ) {
      fprintf(fp," waypoint=\"D%03d\"",
	      garmin->datatype.route.waypoint);
    }
    if ( garmin->datatype.route.link != data_Dnil ) {
      fprintf(fp," link=\"D%03d\"",
	      garmin->datatype.route.link);
    }
    fprintf(fp,"/>\n");
  }

  /* Track */
  
  if ( garmin->protocol.track != appl_Anil ) {
    print_spaces(fp,spaces+1);
    fprintf(fp,"<garmin_track protocol=\"A%03d\"",
	    garmin->protocol.track);
    if ( garmin->datatype.track.header != data_Dnil ) {
      fprintf(fp," header=\"D%03d\"",
	      garmin->datatype.track.header);
    }
    if ( garmin->datatype.track.data != data_Dnil ) {
      fprintf(fp," data=\"D%03d\"",
	      garmin->datatype.track.data);
    }
    fprintf(fp,"/>\n");
  }
  
  /* Almanac, Date/Time, FlightBook, Position, PVT, Lap, Run */
  
  PROTO1_AND_DATA(almanac);
  PROTO1_AND_DATA(date_time);
  PROTO1_AND_DATA(flightbook);
  PROTO1_AND_DATA(position);
  PROTO1_AND_DATA(pvt);
  PROTO1_AND_DATA(lap);
  PROTO1_AND_DATA(run);
  
  /* Workout */
  
  if ( garmin->protocol.workout.workout     != appl_Anil ||
       garmin->protocol.workout.occurrence  != appl_Anil ||
       garmin->protocol.workout.limits      != appl_Anil ) {
    open_tag("garmin_workout",fp,spaces+1);
    PROTO2_AND_DATA(workout,workout);
    PROTO2_AND_DATA(workout,occurrence);
    PROTO2_AND_DATA(workout,limits);
    close_tag("garmin_workout",fp,spaces+1);
  }
  
  /* Fitness user profile */
  
  PROTO1_AND_DATA(fitness);
  
  /* Course */
  
  if ( garmin->protocol.course.course != appl_Anil ||
       garmin->protocol.course.lap    != appl_Anil ||
       garmin->protocol.course.track  != appl_Anil ||
       garmin->protocol.course.point  != appl_Anil ||
       garmin->protocol.course.limits != appl_Anil ) {
    open_tag("garmin_course",fp,spaces+1);
    PROTO2_AND_DATA(course,course);
    PROTO2_AND_DATA(course,lap);
    
    if ( garmin->protocol.course.track != appl_Anil ) {
      print_spaces(fp,spaces+2);
      fprintf(fp,"<garmin_course_track protocol=\"A%03d\"",
	      garmin->protocol.course.track);
      if ( garmin->datatype.course.track.header != data_Dnil ) {
	fprintf(fp," header=\"D%03d\"",
		garmin->datatype.course.track.header);
      }
      if ( garmin->datatype.course.track.data != data_Dnil ) {
	fprintf(fp," data=\"D%03d\"",
		garmin->datatype.course.track.data);
      }
      close_tag("garmin_course_track",fp,spaces+1);      
    }
    
    PROTO2_AND_DATA(course,point);
    PROTO2_AND_DATA(course,limits);
    close_tag("garmin_course",fp,spaces+1);
  }
  
  /* All done. */
  
  close_tag("garmin_protocols",fp,spaces);

#undef PROTO1_AND_DATA
#undef PROTO2_AND_DATA
}


void
garmin_print_info ( garmin_unit * unit, FILE * fp, int spaces )
{
  char ** s;

  print_spaces(fp,spaces);
  fprintf(fp,"<garmin_unit id=\"%x\">\n",unit->id);
  print_spaces(fp,spaces+1);
  fprintf(fp,"<garmin_product id=\"%d\" software_version=\"%.2f\">\n",
	  unit->product.product_id,unit->product.software_version/100.0);
  GARMIN_TAGSTR(2,"product_description",unit->product.product_description);
  if ( unit->product.additional_data != NULL ) {
    open_tag("additional_data_list",fp,spaces+2);
    for ( s = unit->product.additional_data; s != NULL && *s != NULL; s++ ) {
      GARMIN_TAGSTR(3,"additional_data",*s);
    }
    close_tag("additional_data_list",fp,spaces+2);
  }
  close_tag("garmin_product",fp,spaces+1);
  if ( unit->extended.ext_data != NULL ) {
    open_tag("extended_data_list",fp,spaces+1);
    for ( s = unit->extended.ext_data; s != NULL && *s != NULL; s++ ) {
      GARMIN_TAGSTR(2,"extended_data",*s);
    }
    close_tag("extended_data_list",fp,spaces+1);
  }  
  garmin_print_protocols(unit,fp,spaces+1);
  close_tag("garmin_unit",fp,spaces);
}

