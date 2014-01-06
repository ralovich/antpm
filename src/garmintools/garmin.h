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

#ifndef __GARMIN_GARMIN_H__
#define __GARMIN_GARMIN_H__


#include <stdio.h>
#include <usb.h>
#include <math.h>


/* ------------------------------------------------------------------------- */
/* 7.3   Basic Data Types                                                    */
/* ------------------------------------------------------------------------- */

/* conversions */

#define DEGREES      180.0
#define SEMICIRCLES  0x80000000

#define SEMI2DEG(a)  (double)(a) * DEGREES / SEMICIRCLES
#define DEG2SEMI(a)  rint((double)(a) * SEMICIRCLES / DEGREES)

#define DEG2RAD(a)   (a) * M_PI / DEGREES
#define RAD2DEG(a)   (a) * DEGREES / M_PI


/* number of seconds since Dec 31, 1989, 12:00 AM (UTC) */

#define TIME_OFFSET  631065600


/* types */

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;
typedef short           sint16;
typedef int             sint32;
typedef float           float32;
typedef double          float64;
typedef unsigned char   gbool;


/* 2^31 semicircles = 180 degrees where N, E are positive, S and W negative. */

typedef struct position_type {
  sint32                lat;     /* latitude in semicircles  */
  sint32                lon;     /* longitude in semicircles */
} position_type;


typedef struct radian_position_type {
  float64               lat;     /* latitude in radians  */
  float64               lon;     /* longitude in radians */
} radian_position_type;


/* Time is the number of seconds since 12:00 am Dec 31, 1989 UTC. */

typedef uint32          time_type;


/* Symbol type for waypoints. */

typedef uint16          symbol_type;


/* Symbol values for waypoints */

typedef enum {

  /* Marine symbols */

  sym_anchor          =   0,  /* white anchor symbol              */
  sym_bell            =   1,  /* white bell symbol                */
  sym_diamond_grn     =   2,  /* green diamond symbol             */
  sym_diamond_red     =   3,  /* red diamond symbol               */
  sym_dive1           =   4,  /* diver down flag 1                */
  sym_dive2           =   5,  /* diver down flag 2                */
  sym_dollar          =   6,  /* white dollar symbol              */
  sym_fish            =   7,  /* white fish symbol                */
  sym_fuel            =   8,  /* white fuel symbol                */
  sym_horn            =   9,  /* white horn symbol                */
  sym_house           =  10,  /* white house symbol               */
  sym_knife           =  11,  /* white knife & fork symbol        */
  sym_light           =  12,  /* white light symbol               */
  sym_mug             =  13,  /* white mug symbol                 */
  sym_skull           =  14,  /* white skull and crossbones symbol*/
  sym_square_grn      =  15,  /* green square symbol              */
  sym_square_red      =  16,  /* red square symbol                */
  sym_wbuoy           =  17,  /* white buoy waypoint symbol       */
  sym_wpt_dot         =  18,  /* waypoint dot                     */
  sym_wreck           =  19,  /* white wreck symbol               */
  sym_null            =  20,  /* null symbol (transparent)        */
  sym_mob             =  21,  /* man overboard symbol             */
  sym_buoy_ambr       =  22,  /* amber map buoy symbol            */
  sym_buoy_blck       =  23,  /* black map buoy symbol            */
  sym_buoy_blue       =  24,  /* blue map buoy symbol             */
  sym_buoy_grn        =  25,  /* green map buoy symbol            */
  sym_buoy_grn_red    =  26,  /* green/red map buoy symbol        */
  sym_buoy_grn_wht    =  27,  /* green/white map buoy symbol      */
  sym_buoy_orng       =  28,  /* orange map buoy symbol           */
  sym_buoy_red        =  29,  /* red map buoy symbol              */
  sym_buoy_red_grn    =  30,  /* red/green map buoy symbol        */
  sym_buoy_red_wht    =  31,  /* red/white map buoy symbol        */
  sym_buoy_violet     =  32,  /* violet map buoy symbol           */
  sym_buoy_wht        =  33,  /* white map buoy symbol            */
  sym_buoy_wht_grn    =  34,  /* white/green map buoy symbol      */
  sym_buoy_wht_red    =  35,  /* white/red map buoy symbol        */
  sym_dot             =  36,  /* white dot symbol                 */
  sym_rbcn            =  37,  /* radio beacon symbol              */
  sym_boat_ramp       = 150,  /* boat ramp symbol                 */
  sym_camp            = 151,  /* campground symbol                */
  sym_restrooms       = 152,  /* restrooms symbol                 */
  sym_showers         = 153,  /* shower symbol                    */
  sym_drinking_wtr    = 154,  /* drinking water symbol            */
  sym_phone           = 155,  /* telephone symbol                 */
  sym_1st_aid         = 156,  /* first aid symbol                 */
  sym_info            = 157,  /* information symbol               */
  sym_parking         = 158,  /* parking symbol                   */
  sym_park            = 159,  /* park symbol                      */
  sym_picnic          = 160,  /* picnic symbol                    */
  sym_scenic          = 161,  /* scenic area symbol               */
  sym_skiing          = 162,  /* skiing symbol                    */
  sym_swimming        = 163,  /* swimming symbol                  */
  sym_dam             = 164,  /* dam symbol                       */
  sym_controlled      = 165,  /* controlled area symbol           */
  sym_danger          = 166,  /* danger symbol                    */
  sym_restricted      = 167,  /* restricted area symbol           */
  sym_null_2          = 168,  /* null symbol                      */
  sym_ball            = 169,  /* ball symbol                      */
  sym_car             = 170,  /* car symbol                       */
  sym_deer            = 171,  /* deer symbol                      */
  sym_shpng_cart      = 172,  /* shopping cart symbol             */
  sym_lodging         = 173,  /* lodging symbol                   */
  sym_mine            = 174,  /* mine symbol                      */
  sym_trail_head      = 175,  /* trail head symbol                */
  sym_truck_stop      = 176,  /* truck stop symbol                */
  sym_user_exit       = 177,  /* user exit symbol                 */
  sym_flag            = 178,  /* flag symbol                      */
  sym_circle_x        = 179,  /* circle with x in the center      */
  sym_open_24hr       = 180,  /* open 24 hours symbol             */
  sym_fhs_facility    = 181,  /* U Fishing Hot Spots(TM) Facility */
  sym_bot_cond        = 182,  /* Bottom Conditions                */
  sym_tide_pred_stn   = 183,  /* Tide/Current Prediction Station  */
  sym_anchor_prohib   = 184,  /* U anchor prohibited symbol       */
  sym_beacon          = 185,  /* U beacon symbol                  */
  sym_coast_guard     = 186,  /* U coast guard symbol             */
  sym_reef            = 187,  /* U reef symbol                    */
  sym_weedbed         = 188,  /* U weedbed symbol                 */
  sym_dropoff         = 189,  /* U dropoff symbol                 */
  sym_dock            = 190,  /* U dock symbol                    */
  sym_marina          = 191,  /* U marina symbol                  */
  sym_bait_tackle     = 192,  /* U bait and tackle symbol         */
  sym_stump           = 193,  /* U stump symbol                   */

  /* User customizable symbols */

  sym_begin_custom   = 7680,  /* first user customizable symbol   */
  sym_end_custom     = 8191,  /* last user customizable symbol    */
  
  /* Land symbols */

  sym_is_hwy         = 8192,  /* interstate hwy symbol            */
  sym_us_hwy         = 8193,  /* us hwy symbol                    */
  sym_st_hwy         = 8194,  /* state hwy symbol                 */
  sym_mi_mrkr        = 8195,  /* mile marker symbol               */
  sym_trcbck         = 8196,  /* TracBack (feet) symbol           */
  sym_golf           = 8197,  /* golf symbol                      */
  sym_sml_cty        = 8198,  /* small city symbol                */
  sym_med_cty        = 8199,  /* medium city symbol               */
  sym_lrg_cty        = 8200,  /* large city symbol                */
  sym_freeway        = 8201,  /* intl freeway hwy symbol          */
  sym_ntl_hwy        = 8202,  /* intl national hwy symbol         */
  sym_cap_cty        = 8203,  /* capitol city symbol (star)       */
  sym_amuse_pk       = 8204,  /* amusement park symbol            */
  sym_bowling        = 8205,  /* bowling symbol                   */
  sym_car_rental     = 8206,  /* car rental symbol                */
  sym_car_repair     = 8207,  /* car repair symbol                */
  sym_fastfood       = 8208,  /* fast food symbol                 */
  sym_fitness        = 8209,  /* fitness symbol                   */
  sym_movie          = 8210,  /* movie symbol                     */
  sym_museum         = 8211,  /* museum symbol                    */
  sym_pharmacy       = 8212,  /* pharmacy symbol                  */
  sym_pizza          = 8213,  /* pizza symbol                     */
  sym_post_ofc       = 8214,  /* post office symbol               */
  sym_rv_park        = 8215,  /* RV park symbol                   */
  sym_school         = 8216,  /* school symbol                    */
  sym_stadium        = 8217,  /* stadium symbol                   */
  sym_store          = 8218,  /* dept. store symbol               */
  sym_zoo            = 8219,  /* zoo symbol                       */
  sym_gas_plus       = 8220,  /* convenience store symbol         */
  sym_faces          = 8221,  /* live theater symbol              */
  sym_ramp_int       = 8222,  /* ramp intersection symbol         */
  sym_st_int         = 8223,  /* street intersection symbol       */
  sym_weigh_sttn     = 8226,  /* inspection/weigh station symbol  */
  sym_toll_booth     = 8227,  /* toll booth symbol                */
  sym_elev_pt        = 8228,  /* elevation point symbol           */
  sym_ex_no_srvc     = 8229,  /* exit without services symbol     */
  sym_geo_place_mm   = 8230,  /* Geographic place name, man-made  */
  sym_geo_place_wtr  = 8231,  /* Geographic place name, water     */
  sym_geo_place_lnd  = 8232,  /* Geographic place name, land      */
  sym_bridge         = 8233,  /* bridge symbol                    */
  sym_building       = 8234,  /* building symbol                  */
  sym_cemetery       = 8235,  /* cemetery symbol                  */
  sym_church         = 8236,  /* church symbol                    */
  sym_civil          = 8237,  /* civil location symbol            */
  sym_crossing       = 8238,  /* crossing symbol                  */
  sym_hist_town      = 8239,  /* historical town symbol           */
  sym_levee          = 8240,  /* levee symbol                     */
  sym_military       = 8241,  /* military location symbol         */
  sym_oil_field      = 8242,  /* oil field symbol                 */
  sym_tunnel         = 8243,  /* tunnel symbol                    */
  sym_beach          = 8244,  /* beach symbol                     */
  sym_forest         = 8245,  /* forest symbol                    */
  sym_summit         = 8246,  /* summit symbol                    */
  sym_lrg_ramp_int   = 8247,  /* large ramp intersection symbol   */
  sym_lrg_ex_no_srvc = 8248,  /* large exit without services smbl */
  sym_badge          = 8249,  /* police/official badge symbol     */
  sym_cards          = 8250,  /* gambling/casino symbol           */
  sym_snowski        = 8251,  /* snow skiing symbol               */
  sym_iceskate       = 8252,  /* ice skating symbol               */
  sym_wrecker        = 8253,  /* tow truck (wrecker) symbol       */
  sym_border         = 8254,  /* border crossing (port of entry)  */
  sym_geocache       = 8255,  /* geocache location                */
  sym_geocache_fnd   = 8256,  /* found geocache                   */
  sym_cntct_smiley   = 8257,  /* Rino contact symbol, "smiley"    */
  sym_cntct_ball_cap = 8258,  /* Rino contact symbol, "ball cap"  */
  sym_cntct_big_ears = 8259,  /* Rino contact symbol, "big ear"   */
  sym_cntct_spike    = 8260,  /* Rino contact symbol, "spike"     */
  sym_cntct_goatee   = 8261,  /* Rino contact symbol, "goatee"    */
  sym_cntct_afro     = 8262,  /* Rino contact symbol, "afro"      */
  sym_cntct_dreads   = 8263,  /* Rino contact symbol, "dreads"    */
  sym_cntct_female1  = 8264,  /* Rino contact symbol, "female 1"  */
  sym_cntct_female2  = 8265,  /* Rino contact symbol, "female 2"  */
  sym_cntct_female3  = 8266,  /* Rino contact symbol, "female 3"  */
  sym_cntct_ranger   = 8267,  /* Rino contact symbol, "ranger"    */
  sym_cntct_kung_fu  = 8268,  /* Rino contact symbol, "kung fu"   */
  sym_cntct_sumo     = 8269,  /* Rino contact symbol, "sumo"      */
  sym_cntct_pirate   = 8270,  /* Rino contact symbol, "pirate"    */
  sym_cntct_biker    = 8271,  /* Rino contact symbol, "biker"     */
  sym_cntct_alien    = 8272,  /* Rino contact symbol, "alien"     */
  sym_cntct_bug      = 8273,  /* Rino contact symbol, "bug"       */
  sym_cntct_cat      = 8274,  /* Rino contact symbol, "cat"       */
  sym_cntct_dog      = 8275,  /* Rino contact symbol, "dog"       */
  sym_cntct_pig      = 8276,  /* Rino contact symbol, "pig"       */
  sym_hydrant        = 8282,  /* water hydrant symbol             */
  sym_flag_blue      = 8284,  /* blue flag symbol                 */
  sym_flag_green     = 8285,  /* green flag symbol                */
  sym_flag_red       = 8286,  /* red flag symbol                  */
  sym_pin_blue       = 8287,  /* blue pin symbol                  */
  sym_pin_green      = 8288,  /* green pin symbol                 */
  sym_pin_red        = 8289,  /* red pin symbol                   */
  sym_block_blue     = 8290,  /* blue block symbol                */
  sym_block_green    = 8291,  /* green block symbol               */
  sym_block_red      = 8292,  /* red block symbol                 */
  sym_bike_trail     = 8293,  /* bike trail symbol                */
  sym_circle_red     = 8294,  /* red circle symbol                */
  sym_circle_green   = 8295,  /* green circle symbol              */
  sym_circle_blue    = 8296,  /* blue circle symbol               */
  sym_diamond_blue   = 8299,  /* blue diamond symbol              */
  sym_oval_red       = 8300,  /* red oval symbol                  */
  sym_oval_green     = 8301,  /* green oval symbol                */
  sym_oval_blue      = 8302,  /* blue oval symbol                 */
  sym_rect_red       = 8303,  /* red rectangle symbol             */
  sym_rect_green     = 8304,  /* green rectangle symbol           */
  sym_rect_blue      = 8305,  /* blue rectangle symbol            */
  sym_square_blue    = 8308,  /* blue square symbol               */
  sym_letter_a_red   = 8309,  /* red letter 'A' symbol            */
  sym_letter_b_red   = 8310,  /* red letter 'B' symbol            */
  sym_letter_c_red   = 8311,  /* red letter 'C' symbol            */
  sym_letter_d_red   = 8312,  /* red letter 'D' symbol            */
  sym_letter_a_green = 8313,  /* green letter 'A' symbol          */
  sym_letter_b_green = 8314,  /* green letter 'B' symbol          */
  sym_letter_c_green = 8315,  /* green letter 'C' symbol          */
  sym_letter_d_green = 8316,  /* green letter 'D' symbol          */
  sym_letter_a_blue  = 8317,  /* blue letter 'A' symbol           */
  sym_letter_b_blue  = 8318,  /* blue letter 'B' symbol           */
  sym_letter_c_blue  = 8319,  /* blue letter 'C' symbol           */
  sym_letter_d_blue  = 8320,  /* blue letter 'D' symbol           */  
  sym_number_0_red   = 8321,  /* red number '0' symbol            */
  sym_number_1_red   = 8322,  /* red number '1' symbol            */
  sym_number_2_red   = 8323,  /* red number '2' symbol            */
  sym_number_3_red   = 8324,  /* red number '3' symbol            */
  sym_number_4_red   = 8325,  /* red number '4' symbol            */
  sym_number_5_red   = 8326,  /* red number '5' symbol            */
  sym_number_6_red   = 8327,  /* red number '6' symbol            */
  sym_number_7_red   = 8328,  /* red number '7' symbol            */
  sym_number_8_red   = 8329,  /* red number '8' symbol            */
  sym_number_9_red   = 8330,  /* red number '9' symbol            */
  sym_number_0_green = 8331,  /* green number '0' symbol          */
  sym_number_1_green = 8332,  /* green number '1' symbol          */
  sym_number_2_green = 8333,  /* green number '2' symbol          */
  sym_number_3_green = 8334,  /* green number '3' symbol          */
  sym_number_4_green = 8335,  /* green number '4' symbol          */
  sym_number_5_green = 8336,  /* green number '5' symbol          */
  sym_number_6_green = 8337,  /* green number '6' symbol          */
  sym_number_7_green = 8338,  /* green number '7' symbol          */
  sym_number_8_green = 8339,  /* green number '8' symbol          */
  sym_number_9_green = 8340,  /* green number '9' symbol          */
  sym_number_0_blue  = 8341,  /* blue number '0' symbol           */
  sym_number_1_blue  = 8342,  /* blue number '1' symbol           */
  sym_number_2_blue  = 8343,  /* blue number '2' symbol           */
  sym_number_3_blue  = 8344,  /* blue number '3' symbol           */
  sym_number_4_blue  = 8345,  /* blue number '4' symbol           */
  sym_number_5_blue  = 8346,  /* blue number '5' symbol           */
  sym_number_6_blue  = 8347,  /* blue number '6' symbol           */
  sym_number_7_blue  = 8348,  /* blue number '7' symbol           */
  sym_number_8_blue  = 8349,  /* blue number '8' symbol           */
  sym_number_9_blue  = 8350,  /* blue number '9' symbol           */
  sym_triangle_blue  = 8351,  /* blue triangle symbol             */
  sym_triangle_green = 8352,  /* green triangle symbol            */
  sym_triangle_red   = 8353,  /* red triangle symbol              */
  sym_food_asian     = 8359,  /* asian food symbol                */
  sym_food_deli      = 8360,  /* deli symbol                      */
  sym_food_italian   = 8361,  /* italian food symbol              */
  sym_food_seafood   = 8362,  /* seafood food symbol              */
  sym_food_steak     = 8363,  /* steak symbol                     */
  
  /* Aviation symbols */

  sym_airport        = 16384, /* airport symbol                   */
  sym_int            = 16385, /* intersection symbol              */
  sym_ndb            = 16386, /* non-directional beacon symbol    */
  sym_vor            = 16387, /* VHF omni-range symbol            */
  sym_heliport       = 16388, /* heliport symbol                  */
  sym_private        = 16389, /* private field symbol             */
  sym_soft_fld       = 16390, /* soft field symbol                */
  sym_tall_tower     = 16391, /* tall tower symbol                */
  sym_short_tower    = 16392, /* short tower symbol               */
  sym_glider         = 16393, /* glider symbol                    */
  sym_ultralight     = 16394, /* ultralight symbol                */
  sym_parachute      = 16395, /* parachute symbol                 */
  sym_vortac         = 16396, /* VOR/TACAN symbol                 */
  sym_vordme         = 16397, /* VOR-DME symbol                   */
  sym_faf            = 16398, /* first approach fix               */
  sym_lom            = 16399, /* localizer outer marker           */
  sym_map            = 16400, /* missed approach point            */
  sym_tacan          = 16401, /* TACAN symbol                     */
  sym_seaplane       = 16402  /* Seaplane Base                    */
} symbol_value;


typedef struct D100 {
  char             ident[6];       /* identifier            */
  position_type    posn;           /* position              */
  uint32           unused;         /* should be set to zero */
  char             cmnt[40];       /* comment               */
} D100;


typedef struct D101 {
  char             ident[6];       /* identifier            */
  position_type    posn;           /* position              */
  uint32           unused;         /* should be set to zero */
  char             cmnt[40];       /* comment               */
  float32          dst;            /* proximity distance (meters) */
  uint8            smbl;           /* symbol id                   */
} D101;


typedef struct D102 {
  char             ident[6];      /* identifier                   */
  position_type    posn;          /* position                     */
  uint32           unused;        /* should be set to zero        */
  char             cmnt[40];      /* comment                      */
  float32          dst;           /* proximity distance (meters)  */
  symbol_type      smbl;          /* symbol id                    */
} D102;


typedef struct D103 {
  char             ident[6];      /* identifier                    */
  position_type    posn;          /* position                      */
  uint32           unused;        /* should be set to zero         */
  char             cmnt[40];      /* comment                       */
  uint8            smbl;          /* symbol id                     */
  uint8            dspl;          /* display option                */
} D103;


/* The enumerated values for the "smbl" member of the D103 are: */

typedef enum {
  D103_smbl_dot        =  0,   /* dot symbol                                */
  D103_smbl_house      =  1,   /* house symbol                              */
  D103_smbl_gas        =  2,   /* gas symbol                                */
  D103_smbl_car        =  3,   /* car symbol                                */
  D103_smbl_fish       =  4,   /* fish symbol                               */
  D103_smbl_boat       =  5,   /* boat symbol                               */
  D103_smbl_anchor     =  6,   /* anchor symbol                             */
  D103_smbl_wreck      =  7,   /* wreck symbol                              */
  D103_smbl_exit       =  8,   /* exit symbol                               */
  D103_smbl_skull      =  9,   /* skull symbol                              */
  D103_smbl_flag       = 10,   /* flag symbol                               */
  D103_smbl_camp       = 11,   /* camp symbol                               */
  D103_smbl_circle_x   = 12,   /* circle with x symbol                      */
  D103_smbl_deer       = 13,   /* deer symbol                               */
  D103_smbl_1st_aid    = 14,   /* first aid symbol                          */
  D103_smbl_back_track = 15    /* back track symbol                         */
} D103_smbl;


/* The enumerated values for the "dspl" member of the D103 are: */

typedef enum {
  D103_dspl_name       =  0,   /* Display symbol with waypoint name         */
  D103_dspl_none       =  1,   /* Display symbol by itself                  */
  D103_dspl_cmnt       =  2    /* Display symbol with comment               */
} D103_dspl;


typedef struct D104 {
  char              ident[6];        /* identifier                    */
  position_type     posn;            /* position                      */
  uint32            unused;          /* should be set to zero         */
  char              cmnt[40];        /* comment                       */
  float32           dst;             /* proximity distance (meters)   */
  symbol_type       smbl;            /* symbol id                     */
  uint8             dspl;            /* display option                */
} D104;


/* The enumerated values for the "dspl" member of the D104 are: */

typedef enum {
  D104_dspl_smbl_none    = 0,    /* Display symbol by itself             */
  D104_dspl_smbl_only    = 1,    /* Display symbol by itself             */
  D104_dspl_smbl_name    = 3,    /* Display symbol with waypoint name    */
  D104_dspl_smbl_cmnt    = 5     /* Display symbol with comment          */
} D104_dspl;


typedef struct D105 {
  position_type     posn;         /* position                    */
  symbol_type       smbl;         /* symbol id                   */
  char *            wpt_ident;    /* null-terminated string      */
} D105;


typedef struct D106 {
  uint8             wpt_class;      /* class                   */
  uint8             subclass[13];   /* subclass                */
  position_type     posn;           /* position                */
  symbol_type       smbl;           /* symbol id               */
  char *            wpt_ident;      /* null-terminated string  */
  char *            lnk_ident;      /* null-terminated string  */
} D106;


typedef struct D107 {
  char              ident[6];   /* identifier                   */
  position_type     posn;       /* position                     */
  uint32            unused;     /* should be set to zero        */
  char              cmnt[40];   /* comment                      */
  uint8             smbl;       /* symbol id                    */
  uint8             dspl;       /* display option               */
  float32           dst;        /* proximity distance (meters)  */
  uint8             color;      /* waypoint color               */
} D107;


typedef enum {
  D107_clr_default    = 0,              /* Default waypoint color        */
  D107_clr_red        = 1,              /* Red                           */
  D107_clr_green      = 2,              /* Green                         */
  D107_clr_blue       = 3               /* Blue                          */
} D107_clr;


typedef struct D108 {   /*                                 size   */
  uint8            wpt_class;    /* class (see below)               1      */
  uint8            color;        /* color (see below)               1      */
  uint8            dspl;         /* display options (see below)     1      */
  uint8            attr;         /* attributes (see below)          1      */
  symbol_type      smbl;         /* waypoint symbol                 2      */
  uint8            subclass[18]; /* subclass                        18     */
  position_type    posn;         /* 32 bit semicircle               8      */
  float32          alt;          /* altitude in meters              4      */
  float32          dpth;         /* depth in meters                 4      */
  float32          dist;         /* proximity distance in meters    4      */
  char             state[2];     /* state                           2      */
  char             cc[2];        /* country code                    2      */
  char *           ident;        /* variable length string          1-51   */
  char *           comment;      /* waypoint user comment           1-51   */
  char *           facility;     /* facility name                   1-31   */
  char *           city;         /* city name                       1-25   */
  char *           addr;         /* address number                  1-51   */
  char *           cross_road;   /* intersecting road label         1-51   */
} D108;


/* Enumerated values for the "wpt_class" member of the D108 are: */

typedef enum {
  D108_user_wpt           = 0x00,     /* User waypoint                     */
  D108_avtn_apt_wpt       = 0x40,     /* Aviation Airport waypoint         */
  D108_avtn_int_wpt       = 0x41,     /* Aviation Intersection waypoint    */
  D108_avtn_ndb_wpt       = 0x42,     /* Aviation NDB waypoint             */
  D108_avtn_vor_wpt       = 0x43,     /* Aviation VOR waypoint             */
  D108_avtn_arwy_wpt      = 0x44,     /* Aviation Airport Runway waypoint  */
  D108_avtn_aint_wpt      = 0x45,     /* Aviation Airport Intersection     */
  D108_avtn_andb_wpt      = 0x46,     /* Aviation Airport NDB waypoint     */
  D108_map_pnt_wpt        = 0x80,     /* Map Point waypoint                */
  D108_map_area_wpt       = 0x81,     /* Map Area waypoint                 */
  D108_map_int_wpt        = 0x82,     /* Map Intersection waypoint         */
  D108_map_adrs_wpt       = 0x83,     /* Map Address waypoint              */
  D108_map_label_wpt      = 0x84,     /* Map Label Waypoint                */
  D108_map_line_wpt       = 0x85      /* Map Line Waypoint                 */
} D108_wpt_class;


/* The "color" member can be one of the following values: */

typedef enum {
  D108_black          = 0x00,
  D108_dark_red       = 0x01,
  D108_dark_green     = 0x02,
  D108_dark_yellow    = 0x03,
  D108_dark_blue      = 0x04,
  D108_dark_magenta   = 0x05,
  D108_dark_cyan      = 0x06,
  D108_light_gray     = 0x07,
  D108_dark_gray      = 0x08,
  D108_red            = 0x09,
  D108_green          = 0x0a,
  D108_yellow         = 0x0b,
  D108_blue           = 0x0c,
  D108_magenta        = 0x0d,
  D108_cyan           = 0x0e,
  D108_white          = 0x0f,
  D108_default_color  = 0xff
} D108_color;


typedef struct D109 {    /*                                  size */
  uint8           dtyp;           /* data packet type (0x01 for D109) 1    */
  uint8           wpt_class;      /* class                            1    */
  uint8           dspl_color;     /* display & color (see below)      1    */
  uint8           attr;           /* attributes (0x70 for D109)       1    */
  symbol_type     smbl;           /* waypoint symbol                  2    */
  uint8           subclass[18];   /* subclass                         18   */
  position_type   posn;           /* 32 bit semicircle                8    */
  float32         alt;            /* altitude in meters               4    */
  float32         dpth;           /* depth in meters                  4    */
  float32         dist;           /* proximity distance in meters     4    */
  char            state[2];       /* state                            2    */
  char            cc[2];          /* country code                     2    */
  uint32          ete;            /* outbound link ete in seconds     4    */
  char *          ident;         /* variable length string           1-51 */
  char *          comment;       /* waypoint user comment            1-51 */
  char *          facility;      /* facility name                    1-31 */
  char *          city;          /* city name                        1-25 */
  char *          addr;          /* address number                   1-51 */
  char *          cross_road;    /* intersecting road label          1-51 */
} D109;


typedef struct D110 {    /*                                  size */
  uint8           dtyp;           /* data packet type (0x01 for D110) 1    */
  uint8           wpt_class;      /* class                            1    */
  uint8           dspl_color;     /* display & color (see below)      1    */
  uint8           attr;           /* attributes (0x80 for D110)       1    */
  symbol_type     smbl;           /* waypoint symbol                  2    */
  uint8           subclass[18];   /* subclass                         18   */
  position_type   posn;           /* 32 bit semicircle                8    */
  float32         alt;            /* altitude in meters               4    */
  float32         dpth;           /* depth in meters                  4    */
  float32         dist;           /* proximity distance in meters     4    */
  char            state[2];       /* state                            2    */
  char            cc[2];          /* country code                     2    */
  uint32          ete;            /* outbound link ete in seconds     4    */
  float32         temp;           /* temperature                      4    */
  time_type       time;           /* timestamp                        4    */
  uint16          wpt_cat;        /* category membership              2    */
  char *          ident;          /* variable length string           1-51 */
  char *          comment;        /* waypoint user comment            1-51 */
  char *          facility;       /* facility name                    1-31 */
  char *          city;           /* city name                        1-25 */
  char *          addr;           /* address number                   1-51 */
  char *          cross_road;     /* intersecting road label          1-51 */
} D110;


/* Enumerated values for the "wpt_class" member of the D108 are: */

typedef enum {
  D110_user_wpt           = 0x00,     /* User waypoint                     */
  D110_avtn_apt_wpt       = 0x40,     /* Aviation Airport waypoint         */
  D110_avtn_int_wpt       = 0x41,     /* Aviation Intersection waypoint    */
  D110_avtn_ndb_wpt       = 0x42,     /* Aviation NDB waypoint             */
  D110_avtn_vor_wpt       = 0x43,     /* Aviation VOR waypoint             */
  D110_avtn_arwy_wpt      = 0x44,     /* Aviation Airport Runway waypoint  */
  D110_avtn_aint_wpt      = 0x45,     /* Aviation Airport Intersection     */
  D110_avtn_andb_wpt      = 0x46,     /* Aviation Airport NDB waypoint     */
  D110_map_pnt_wpt        = 0x80,     /* Map Point waypoint                */
  D110_map_area_wpt       = 0x81,     /* Map Area waypoint                 */
  D110_map_int_wpt        = 0x82,     /* Map Intersection waypoint         */
  D110_map_adrs_wpt       = 0x83,     /* Map Address waypoint              */
  D110_map_line_wpt       = 0x84      /* Map Line Waypoint                 */
} D110_wpt_class;


typedef enum {
  D110_black          = 0x00,
  D110_dark_red       = 0x01,
  D110_dark_green     = 0x02,
  D110_dark_yellow    = 0x03,
  D110_dark_blue      = 0x04,
  D110_dark_magenta   = 0x05,
  D110_dark_cyan      = 0x06,
  D110_light_gray     = 0x07,
  D110_dark_gray      = 0x08,
  D110_red            = 0x09,
  D110_green          = 0x0a,
  D110_yellow         = 0x0b,
  D110_blue           = 0x0c,
  D110_magenta        = 0x0d,
  D110_cyan           = 0x0e,
  D110_white          = 0x0f,
  D110_transparent    = 0x10
} D110_color;


typedef enum {
  D110_symbol_name     = 0,    /* Display symbol with waypoint name    */
  D110_symbol_only     = 1,    /* Display symbol by itself             */
  D110_symbol_comment  = 2     /* Display symbol with comment          */
} D110_dspl;


typedef struct D120 {
  char              name[17];        /* category name */
} D120;


typedef struct D150 {
  char              ident[6];        /* identifier                */
  char              cc[2];           /* country code              */
  uint8             wpt_class;       /* class                     */
  position_type     posn;            /* position                  */
  sint16            alt;             /* altitude (meters)         */
  char              city[24];        /* city                      */
  char              state[2];        /* state                     */
  char              name[30];        /* facility name             */
  char              cmnt[40];        /* comment                   */
} D150;


/* Enumerated values for the "wpt_class" member of the D150 are: */

typedef enum {
  D150_apt_wpt_class    = 0,   /* airport waypoint class                   */
  D150_int_wpt_class    = 1,   /* intersection waypoint class              */
  D150_ndb_wpt_class    = 2,   /* NDB waypoint class                       */
  D150_vor_wpt_class    = 3,   /* VOR waypoint class                       */
  D150_usr_wpt_class    = 4,   /* user defined waypoint class              */
  D150_rwy_wpt_class    = 5,   /* airport runway threshold waypoint class  */
  D150_aint_wpt_class   = 6,   /* airport intersection waypoint class      */
  D150_locked_wpt_class = 7    /* locked waypoint class                    */
} D150_wpt_class;


typedef struct D151 {
  char             ident[6];        /* identifier                   */
  position_type    posn;            /* position                     */
  uint32           unused;          /* should be set to zero        */
  char             cmnt[40];        /* comment                      */
  float32          dst;             /* proximity distance (meters)  */
  char             name[30];        /* facility name                */
  char             city[24];        /* city                         */
  char             state[2];        /* state                        */
  sint16           alt;             /* altitude (meters)            */
  char             cc[2];           /* country code                 */
  char             unused2;         /* should be set to zero        */
  uint8            wpt_class;       /* class                        */
} D151;


/* 
   The enumerated values for the "wpt_class" member of the D151 are: 
*/

typedef enum {
  D151_apt_wpt_class     = 0,      /* airport waypoint class             */
  D151_vor_wpt_class     = 1,      /* VOR waypoint class                 */
  D151_usr_wpt_class     = 2,      /* user defined waypoint class        */
  D151_locked_wpt_class  = 3       /* locked waypoint class              */
} D151_wpt_class;


typedef struct D152 { 
  char            ident[6];        /* identifier                     */
  position_type   posn;            /* position                       */
  uint32          unused;          /* should be set to zero          */
  char            cmnt[40];        /* comment                        */
  float32         dst;             /* proximity distance (meters)    */
  char            name[30];        /* facility name                  */
  char            city[24];        /* city                           */
  char            state[2];        /* state                          */
  sint16          alt;             /* altitude (meters)              */
  char            cc[2];           /* country code                   */
  char            unused2;         /* should be set to zero          */
  uint8           wpt_class;       /* class                          */
} D152;


/* Enumerated values for the "wpt_class" member of the D152 are: */

typedef enum {
  D152_apt_wpt_class    = 0,             /* airport waypoint class         */
  D152_int_wpt_class    = 1,             /* intersection waypoint class    */
  D152_ndb_wpt_class    = 2,             /* NDB waypoint class             */
  D152_vor_wpt_class    = 3,             /* VOR waypoint class             */
  D152_usr_wpt_class    = 4,             /* user defined waypoint class    */
  D152_locked_wpt_class = 5              /* locked waypoint class          */
} D152_wpt_class;


typedef struct D154 {
  char              ident[6];        /* identifier                     */
  position_type     posn;            /* position                       */
  uint32            unused;          /* should be set to zero          */
  char              cmnt[40];        /* comment                        */
  float32           dst;             /* proximity distance (meters)    */
  char              name[30];        /* facility name                  */
  char              city[24];        /* city                           */
  char              state[2];        /* state                          */
  sint16            alt;             /* altitude (meters)              */
  char              cc[2];           /* country code                   */
  char              unused2;         /* should be set to zero          */
  uint8             wpt_class;       /* class                          */
  symbol_type       smbl;            /* symbol id                      */
} D154;


/* Enumerated values for the "wpt_class" member of the D154 are: */

typedef enum {
  D154_apt_wpt_class    = 0,     /* airport waypoint class                  */
  D154_int_wpt_class    = 1,     /* intersection waypoint class             */
  D154_ndb_wpt_class    = 2,     /* NDB waypoint class                      */
  D154_vor_wpt_class    = 3,     /* VOR waypoint class                      */
  D154_usr_wpt_class    = 4,     /* user defined waypoint class             */
  D154_rwy_wpt_class    = 5,     /* airport runway threshold waypoint class */
  D154_aint_wpt_class   = 6,     /* airport intersection waypoint class     */
  D154_andb_wpt_class   = 7,     /* airport NDB waypoint class              */
  D154_sym_wpt_class    = 8,     /* user defined symbol-only waypoint class */
  D154_locked_wpt_class = 9      /* locked waypoint class                   */
} D154_wpt_class;


typedef struct D155 {
  char              ident[6];        /* identifier                   */
  position_type     posn;            /* position                     */
  uint32            unused;          /* should be set to zero        */
  char              cmnt[40];        /* comment                      */
  float32           dst;             /* proximity distance (meters)  */
  char              name[30];        /* facility name                */
  char              city[24];        /* city                         */
  char              state[2];        /* state                        */
  sint16            alt;             /* altitude (meters)            */
  char              cc[2];           /* country code                 */
  char              unused2;         /* should be set to zero        */
  uint8             wpt_class;       /* class                        */
  symbol_type       smbl;            /* symbol id                    */
  uint8             dspl;            /* display option               */
} D155;


/* The enumerated values for the "dspl" member of the D155 are: */

typedef enum {
  D155_dspl_smbl_only   = 1,  /* Display symbol by itself               */
  D155_dspl_smbl_name   = 3,  /* Display symbol with waypoint name      */
  D155_dspl_smbl_cmnt   = 5   /* Display symbol with comment            */
} D155_dspl;


/* Enumerated values for the "wpt_class" member of the D155 are: */

typedef enum {
  D155_apt_wpt_class    = 0,  /* airport waypoint class                */
  D155_int_wpt_class    = 1,  /* intersection waypoint class           */
  D155_ndb_wpt_class    = 2,  /* NDB waypoint class                    */
  D155_vor_wpt_class    = 3,  /* VOR waypoint class                    */
  D155_usr_wpt_class    = 4,  /* user defined waypoint class           */
  D155_locked_wpt_class = 5   /* locked waypoint class                 */
} D155_wpt_class;


typedef uint8    D200;  /* route number  */


typedef struct D201 {
  uint8                 nmbr;       /* route number            */
  char                  cmnt[20];   /* comment                 */
} D201;


typedef struct D202 {
  char *                rte_ident;  /* null-terminated string  */
} D202;


typedef struct D210 {
  uint16                link_class;   /* link class; see below           */
  uint8                 subclass[18]; /* sublcass                        */
  char *                ident;        /* variable length string          */
} D210;


/* The "class" member can be one of the following values: */

typedef enum {
  D210_line           = 0,
  D210_link           = 1,
  D210_net            = 2,
  D210_direct         = 3,
  D210_snap           = 0xff
} D210_class;


typedef struct D300 {
  position_type     posn;      /* position                        */
  uint32            time;      /* time                            */
  gbool             new_trk;   /* new track segment?              */
} D300;


typedef struct D301 {
  position_type     posn;     /* position                  */
  uint32            time;     /* time                      */
  float32           alt;      /* altitude in meters        */
  float32           dpth;     /* depth in meters           */
  gbool             new_trk;  /* new track segment?        */
} D301;


typedef struct D302 {
  position_type     posn;
  uint32            time;
  float32           alt;
  float32           dpth;
  float32           temp;
  gbool             new_trk;
} D302;


typedef struct D303 {
  position_type     posn;
  uint32            time;
  float32           alt;
  uint8             heart_rate;
} D303;


typedef struct D304 {
  position_type     posn;
  uint32            time;
  float32           alt;
  float32           distance;
  uint8             heart_rate;
  uint8             cadence;
  gbool             sensor;
} D304;


typedef struct D310 {
  gbool         dspl;           /* display on the map?        */
  uint8         color;          /* color (same as D108)       */
  char *        trk_ident;      /* null-terminated string     */
} D310;


typedef struct D311 {
  uint16        index;   /* unique among all tracks received from device */
} D311;


typedef struct D312 {
  gbool         dspl;           /* display on the map?    */
  uint8         color;          /* color (same as D110)   */
  char *        trk_ident;      /* null-terminated string */
} D312;


typedef enum {
  D312_black          = 0x00,
  D312_dark_red       = 0x01,
  D312_dark_green     = 0x02,
  D312_dark_yellow    = 0x03,
  D312_dark_blue      = 0x04,
  D312_dark_magenta   = 0x05,
  D312_dark_cyan      = 0x06,
  D312_light_gray     = 0x07,
  D312_dark_gray      = 0x08,
  D312_red            = 0x09,
  D312_green          = 0x0a,
  D312_yellow         = 0x0b,
  D312_blue           = 0x0c,
  D312_magenta        = 0x0d,
  D312_cyan           = 0x0e,
  D312_white          = 0x0f,
  D312_transparent    = 0x10,
  D312_default_color  = 0xff
} D312_color;


typedef struct D400 {
  D100    wpt;  /* waypoint                       */
  float32          dst;  /* proximity distance (meters)    */
} D400;


typedef struct D403 { 
  D103   wpt;  /* waypoint                          */
  float32         dst;  /* proximity distance (meters)       */
} D403;


typedef struct D450 {
  sint16          idx;  /* proximity index                   */
  D150   wpt;  /* waypoint                          */
  float32         dst;  /* proximity distance (meters)       */
} D450;


typedef struct D500 {
  sint16        wn;    /* week number                          (weeks)    */
  float32       toa;   /* almanac data reference time              (s)    */
  float32       af0;   /* clock correction coefficient             (s)    */
  float32       af1;   /* clock correction coefficient           (s/s)    */
  float32       e;     /* eccentricity                             (-)    */
  float32       sqrta; /* square root of semi-major axis (a)  (m**1/2)    */
  float32       m0;    /* mean anomaly at reference time           (r)    */
  float32       w;     /* argument of perigee                      (r)    */
  float32       omg0;  /* right ascension                          (r)    */
  float32       odot;  /* rate of right ascension                (r/s)    */
  float32       i;     /* inclination angle                        (r)    */
} D500;


typedef struct D501 {
  sint16        wn;    /* week number                          (weeks)   */
  float32       toa;   /* almanac data reference time              (s)   */
  float32       af0;   /* clock correction coefficient             (s)   */
  float32       af1;   /* clock correction coefficient           (s/s)   */
  float32       e;     /* eccentricity                             (-)   */
  float32       sqrta; /* square root of semi-major axis (a)  (m**1/2)   */
  float32       m0;    /* mean anomaly at reference time           (r)   */
  float32       w;     /* argument of perigee                      (r)   */
  float32       omg0;  /* right ascension                          (r)   */
  float32       odot;  /* rate of right ascension                (r/s)   */
  float32       i;     /* inclination angle                        (r)   */
  uint8         hlth;  /* almanac health                                 */
} D501;


typedef struct D550 {
  char          svid;  /* satellite id                                   */
  sint16        wn;    /* week number                          (weeks)   */
  float32       toa;   /* almanac data reference time              (s)   */
  float32       af0;   /* clock correction coefficient             (s)   */
  float32       af1;   /* clock correction coefficient           (s/s)   */
  float32       e;     /* eccentricity                             (-)   */
  float32       sqrta; /* square root of semi-major axis (a)  (m**1/2)   */
  float32       m0;    /* mean anomaly at reference time           (r)   */
  float32       w;     /* argument of perigee                      (r)   */
  float32       omg0;  /* right ascension                          (r)   */
  float32       odot;  /* rate of right ascension                (r/s)   */
  float32       i;     /* inclination angle                        (r)   */
} D550;


typedef struct D551 {
  char          svid;  /* satellite id                                   */
  sint16        wn;    /* week number                          (weeks)   */
  float32       toa;   /* almanac data reference time              (s)   */
  float32       af0;   /* clock correction coefficient             (s)   */
  float32       af1;   /* clock correction coefficient           (s/s)   */
  float32       e;     /* eccentricity                             (-)   */
  float32       sqrta; /* square root of semi-major axis (a)  (m**1/2)   */
  float32       m0;    /* mean anomaly at reference time           (r)   */
  float32       w;     /* argument of perigee                      (r)   */
  float32       omg0;  /* right ascension                          (r)   */
  float32       odot;  /* rate of right ascension                (r/s)   */
  float32       i;     /* inclination angle                        (r)   */
  uint8         hlth;  /* almanac health bits 17:24            (coded)   */
} D551;


typedef struct D600 {
  uint8     month;            /* month  (1-12)                  */
  uint8     day;              /* day    (1-31)                  */
  uint16    year;             /* year   (1990 means 1990)       */
  sint16    hour;             /* hour   (0-23)                  */
  uint8     minute;           /* minute (0-59)                  */
  uint8     second;           /* second (0-59)                  */
} D600;


typedef struct D650 {
  time_type         takeoff_time;
  time_type         landing_time;
  position_type     takeoff_posn;
  position_type     landing_posn;
  uint32            night_time;
  uint32            num_landings;
  float32           max_speed;
  float32           max_alt;
  float32           distance;
  gbool             cross_country_flag;
  char *            departure_name;
  char *            departure_ident;
  char *            arrival_name;
  char *            arrival_ident;
  char *            ac_id;
} D650;


typedef radian_position_type  D700;


typedef struct D800 {
  float32               alt;        /* alt above WGS 84 ellipsoid (m)        */
  float32               epe;        /* est. position error, 2 sigma (m)      */
  float32               eph;        /* epe, but horizontal only (meters)     */
  float32               epv;        /* epe, but vertical only (meters)       */
  sint16                fix;        /* type of position fix                  */
  float64               tow;        /* time of week (seconds)                */
  radian_position_type  posn;       /* latitude and longitude (radians)      */
  float32               east;       /* velocity east  (meters/second)        */
  float32               north;      /* velocity north (meters/second)        */
  float32               up;         /* velocity up    (meters/second)        */
  float32               msl_hght;   /* ht. of WGS 84 ellipsoid above MSL (m) */
  sint16                leap_scnds; /* diff between GPS and UTC (seconds)    */
  sint32                wn_days;    /* week number days                      */
} D800;


typedef enum {
  D800_unusable  = 0,    /* failed integrity check                   */
  D800_invalid   = 1,    /* invalid or unavailable                   */
  D800_2D        = 2,    /* two dimensional                          */
  D800_3D        = 3,    /* three dimensional                        */
  D800_2D_diff   = 4,    /* two dimensional differential             */
  D800_3D_diff   = 5     /* three dimensional differential           */
} D800_fix;


typedef struct D906 {
  uint32           start_time;
  uint32           total_time;      /* In hundredths of a second */
  float32          total_distance;  /* In meters */
  position_type    begin;           /* Invalid if lat and lon are 0x7fffffff */
  position_type    end;             /* Invalid if lat and lon are 0x7fffffff */
  uint16           calories;
  uint8            track_index;     /* See below */
  uint8            unused;          /* Unused.  Set to 0. */
} D906;


typedef struct D1002 {
  uint32                       num_valid_steps;
  struct {
    char                       custom_name[16];
    float32                    target_custom_zone_low;
    float32                    target_custom_zone_high;
    uint16                     duration_value;
    uint8                      intensity;
    uint8                      duration_type;
    uint8                      target_type;
    uint8                      target_value;
    uint16                     unused;
  }                            steps[20];
  char                         name[16];
  uint8                        sport_type;
} D1002;


typedef enum {
  D1002_time = 0,
  D1002_distance,
  D1002_heart_rate_less_than,
  D1002_heart_rate_greater_than,
  D1002_calories_burned,
  D1002_open,
  D1002_repeat
} D1002_duration_type;


typedef struct D1000 {
  uint32                       track_index;
  uint32                       first_lap_index;
  uint32                       last_lap_index;
  uint8                        sport_type;
  uint8                        program_type;
  uint16                       unused;
  struct {
    uint32                     time;
    float32                    distance;
  }                            virtual_partner;
  D1002           workout;
} D1000;


typedef enum {
  D1000_running              = 0,
  D1000_biking               = 1,
  D1000_other                = 2
} D1000_sport_type;


typedef enum {
  D1000_none                 = 0,
  D1000_virtual_partner      = 1,
  D1000_workout              = 2
} D1000_program_type;


typedef struct D1001 {
  uint32                       index;
  time_type                    start_time;
  uint32                       total_time;
  float32                      total_dist;
  float32                      max_speed;
  position_type                begin;
  position_type                end;
  uint16                       calories;
  uint8                        avg_heart_rate;
  uint8                        max_heart_rate;
  uint8                        intensity;
} D1001;


typedef enum {
  D1001_active        = 0,
  D1001_rest          = 1
} D1001_intensity;


/* D1002 defined above D1000 */


typedef struct D1003 {
  char                         workout_name[16];
  time_type                    day;
} D1003;


typedef struct D1004 {
  struct {
    struct {
      uint8                    low_heart_rate;
      uint8                    high_heart_rate;
      uint16                   unused;
    }                          heart_rate_zones[5];
    struct {
      float32                  low_speed;
      float32                  high_speed;
      char                     name[16];
    }                          speed_zones[10];
    float32                    gear_weight;
    uint8                      max_heart_rate;
    uint8                      unused1;
    uint16                     unused2;
  }                            activities[3];
  float32                      weight;
  uint16                       birth_year;
  uint8                        birth_month;
  uint8                        birth_day;
  uint8                        gender;
} D1004;


typedef enum {
  D1004_female = 0,
  D1004_male   = 1
} D1004_gender;


typedef struct D1005 {
  uint32                       max_workouts;
  uint32                       max_unscheduled_workouts;
  uint32                       max_occurrences;
} D1005;


typedef struct D1006 {
  uint16                       index;
  uint16                       unused;
  char                         course_name[16];
  uint16                       track_index;
} D1006;


typedef struct D1007 {
  uint16                       course_index;
  uint16                       lap_index;
  uint32                       total_time;
  float32                      total_dist;
  position_type                begin;
  position_type                end;
  uint8                        avg_heart_rate;
  uint8                        max_heart_rate;
  uint8                        intensity;
  uint8                        avg_cadence;
} D1007;


typedef struct D1008 {
  uint32                       num_valid_steps;
  struct {
    char                       custom_name[16];
    float32                    target_custom_zone_low;
    float32                    target_custom_zone_high;
    uint16                     duration_value;
    uint8                      intensity;
    uint8                      duration_type;
    uint8                      target_type;
    uint8                      target_value;
    uint16                     unused;
  }                            steps[20];
  char                         name[16];
  uint8                        sport_type;
} D1008;


typedef struct D1009 {
  uint16                       track_index;
  uint16                       first_lap_index;
  uint16                       last_lap_index;
  uint8                        sport_type;
  uint8                        program_type;
  uint8                        multisport;
  uint8                        unused1;
  uint16                       unused2;
  struct {
    uint32                     time;
    float32                    distance;
  }                            quick_workout;
  D1008                        workout;
} D1009;


typedef enum {
  D1009_no                  = 0,
  D1009_yes                 = 1,
  D1009_yesAndLastInGroup   = 2
} D1009_multisport;


typedef struct D1010 {
  uint32                       track_index;
  uint32                       first_lap_index;
  uint32                       last_lap_index;
  uint8                        sport_type;
  uint8                        program_type;
  uint8                        multisport;
  uint8                        unused;
  struct {
    uint32                     time;
    float32                    distance;
  }                            virtual_partner;
  D1002                        workout;
} D1010;


typedef enum {
  D1010_none               = 0,
  D1010_virtual_partner    = 1,
  D1010_workout            = 2,
  D1010_auto_multisport    = 3
} D1010_program_type;


typedef struct D1011 {
  uint16                       index;
  uint16                       unused;
  time_type                    start_time;
  uint32                       total_time;
  float32                      total_dist;
  float32                      max_speed;
  position_type                begin;
  position_type                end;
  uint16                       calories;
  uint8                        avg_heart_rate;
  uint8                        max_heart_rate;
  uint8                        intensity;
  uint8                        avg_cadence;
  uint8                        trigger_method;
} D1011;


typedef enum {
  D1011_manual           = 0,
  D1011_distance         = 1,
  D1011_location         = 2,
  D1011_time             = 3,
  D1011_heart_rate       = 4
} D1011_trigger_method;


typedef struct D1012 {
  char                         name[11];
  uint8                        unused1;
  uint16                       course_index;
  uint16                       unused2;
  time_type                    track_point_time;
  uint8                        point_type;
} D1012;


typedef enum {
  D1012_generic              = 0x00,
  D1012_summit               = 0x01,
  D1012_valley               = 0x02,
  D1012_water                = 0x03,
  D1012_food                 = 0x04,
  D1012_danger               = 0x05,
  D1012_left                 = 0x06,
  D1012_right                = 0x07,
  D1012_straight             = 0x08,
  D1012_first_aid            = 0x09,
  D1012_fourth_category      = 0x0a,
  D1012_third_category       = 0x0b,
  D1012_second_category      = 0x0c,
  D1012_first_category       = 0x0d,
  D1012_hors_category        = 0x0e,
  D1012_sprint               = 0x0f
} D1012_point_type;


typedef struct D1013 {
  uint32                       max_courses;
  uint32                       max_course_laps;
  uint32                       max_course_pnt;
  uint32                       max_course_trk_pnt;
} D1013;


typedef struct D1015 {
  uint16                       index;
  uint16                       unused;
  time_type                    start_time;
  uint32                       total_time;
  float32                      total_dist;
  float32                      max_speed;
  position_type                begin;
  position_type                end;
  uint16                       calories;
  uint8                        avg_heart_rate;
  uint8                        max_heart_rate;
  uint8                        intensity;
  uint8                        avg_cadence;
  uint8                        trigger_method;
  /* FIXME - additional bytes are unknown */
  uint8                        unknown[5];
} D1015;


typedef enum {
  data_Dnil  =    0,
  data_Dlist =    1,      /* List of data */
  data_D100  =  100,      /* waypoint */
  data_D101  =  101,      /* waypoint */
  data_D102  =  102,      /* waypoint */
  data_D103  =  103,      /* waypoint */
  data_D104  =  104,      /* waypoint */
  data_D105  =  105,      /* waypoint */
  data_D106  =  106,      /* waypoint */
  data_D107  =  107,      /* waypoint */
  data_D108  =  108,      /* waypoint */
  data_D109  =  109,      /* waypoint */
  data_D110  =  110,      /* waypoint */
  data_D120  =  120,      /* waypoint category */
  data_D150  =  150,      /* waypoint */
  data_D151  =  151,      /* waypoint */
  data_D152  =  152,      /* waypoint */
  data_D154  =  154,      /* waypoint */
  data_D155  =  155,      /* waypoint */
  data_D200  =  200,      /* route header */
  data_D201  =  201,      /* route header */
  data_D202  =  202,      /* route header */
  data_D210  =  210,      /* route link */
  data_D300  =  300,      /* track point */
  data_D301  =  301,      /* track point */
  data_D302  =  302,      /* track point */
  data_D303  =  303,      /* track point */
  data_D304  =  304,      /* track point */
  data_D310  =  310,      /* track header */
  data_D311  =  311,      /* track header */
  data_D312  =  312,      /* track header */
  data_D400  =  400,      /* proximity waypoint */
  data_D403  =  403,      /* proximity waypoint */
  data_D450  =  450,      /* proximity waypoint */
  data_D500  =  500,      /* almanac */
  data_D501  =  501,      /* almanac */
  data_D550  =  550,      /* almanac */
  data_D551  =  551,      /* almanac */
  data_D600  =  600,      /* date/time */
  data_D601  =  601,      /* --- UNDOCUMENTED --- */
  data_D650  =  650,      /* flightbook record */
  data_D700  =  700,      /* position */
  data_D800  =  800,      /* position/velocity/time (PVT) */
  data_D801  =  801,      /* --- UNDOCUMENTED --- */
  data_D906  =  906,      /* lap */
  data_D907  =  907,      /* --- UNDOCUMENTED --- */
  data_D908  =  908,      /* --- UNDOCUMENTED --- */
  data_D909  =  909,      /* --- UNDOCUMENTED --- */
  data_D910  =  910,      /* --- UNDOCUMENTED --- */
  data_D1000 = 1000,      /* run */
  data_D1001 = 1001,      /* lap */
  data_D1002 = 1002,      /* workout */
  data_D1003 = 1003,      /* workout occurrence */
  data_D1004 = 1004,      /* fitness user profile */
  data_D1005 = 1005,      /* workout limits */
  data_D1006 = 1006,      /* course */
  data_D1007 = 1007,      /* course lap */
  data_D1008 = 1008,      /* workout */
  data_D1009 = 1009,      /* run */
  data_D1010 = 1010,      /* run */
  data_D1011 = 1011,      /* lap */
  data_D1012 = 1012,      /* course point */
  data_D1013 = 1013,      /* course limits */
  data_D1015 = 1015,      /* lap */
  data_NUM_DATATYPES
} garmin_datatype;


/* Garmin data of any type, including lists of {data, lists}. */

typedef struct garmin_data {
  garmin_datatype   type;
  void *            data;
} garmin_data;


/* A garmin list node (contains data and a 'next' pointer) */

typedef struct garmin_list_node {
  garmin_data *                      data;
  struct garmin_list_node *          next;
} garmin_list_node;


/* A singly linked list of garmin data (can be a list of lists) */

typedef struct garmin_list {
  int                                id;
  int                                elements;
  garmin_list_node *                 head;
  garmin_list_node *                 tail;
} garmin_list;


/* ------------------------------------------------------------------------- */
/* 3.2   USB Protocol                                                        */
/* ------------------------------------------------------------------------- */

#define GARMIN_USB_VID  0x091e
#define GARMIN_USB_PID  0x0003

#define GARMIN_DIR_NONE  0
#define GARMIN_DIR_READ  1
#define GARMIN_DIR_WRITE 2


/* ------------------------------------------------------------------------- */
/* 3.2.2 USB Packet Format                                                   */
/* ------------------------------------------------------------------------- */

#define GARMIN_PROTOCOL_USB   0x00
#define GARMIN_PROTOCOL_APP   0x14


/* Following the scheme of jeeps / gpsbabel... */


#define PACKET_HEADER_SIZE   12


typedef union garmin_packet {
  struct {
    uint8            type;         /*  byte 0      */
    uint8            reserved1;
    uint8            reserved2;
    uint8            reserved3;
    uint8            id[2];        /*  bytes 4-5   */ 
    uint8            reserved4;
    uint8            reserved5;
    uint8            size[4];      /*  bytes 8-11  */
    uint8            data[1];      /*  bytes 12+   */
  }                  packet;
  char               data[1024];
} garmin_packet;


/* ------------------------------------------------------------------------- */
/* 3.2.3 USB Protocol Layer Packet Ids                                       */
/* ------------------------------------------------------------------------- */

typedef enum {
  Pid_Data_Available       = 0x02,
  Pid_Start_Session        = 0x05,
  Pid_Session_Started      = 0x06
} USB_Pid;


/* ------------------------------------------------------------------------- */
/* 6.1   A000 - Product Data Protocol                                        */
/* ------------------------------------------------------------------------- */


typedef struct garmin_product {
  uint16          product_id;
  sint16          software_version;
  char *          product_description;
  char **         additional_data;
} garmin_product;


typedef struct garmin_extended_data {
  char **         ext_data;
} garmin_extended_data;


/* ------------------------------------------------------------------------- */
/* 6.2   A001 - Protocol Capability Protocol                                 */
/* ------------------------------------------------------------------------- */


typedef enum {
  Tag_Phys_Prot_Id    = 'P',
  Tag_Link_Prot_Id    = 'L',
  Tag_Appl_Prot_Id    = 'A',
  Tag_Data_Type_Id    = 'D'
} A001_tag;


/* ------------------------------------------------------------------------- */
/* 6.3.1 A010 - Device Command Protocol 1                                    */
/* ------------------------------------------------------------------------- */

typedef enum {
  A010_Cmnd_Abort_Transfer                = 0x0000,
  A010_Cmnd_Transfer_Alm                  = 0x0001,
  A010_Cmnd_Transfer_Posn                 = 0x0002,
  A010_Cmnd_Transfer_Prx                  = 0x0003,
  A010_Cmnd_Transfer_Rte                  = 0x0004,
  A010_Cmnd_Transfer_Time                 = 0x0005,
  A010_Cmnd_Transfer_Trk                  = 0x0006,
  A010_Cmnd_Transfer_Wpt                  = 0x0007,
  A010_Cmnd_Turn_Off_Pwr                  = 0x0008,
  A010_Cmnd_Start_Pvt_Data                = 0x0031,
  A010_Cmnd_Stop_Pvt_Data                 = 0x0032,
  A010_Cmnd_FlightBook_Transfer           = 0x005c,
  A010_Cmnd_Transfer_Laps                 = 0x0075,
  A010_Cmnd_Transfer_Wpt_Cats             = 0x0079,
  A010_Cmnd_Transfer_Runs                 = 0x01c2,
  A010_Cmnd_Transfer_Workouts             = 0x01c3,
  A010_Cmnd_Transfer_Workout_Occurrences  = 0x01c4,
  A010_Cmnd_Transfer_Fitness_User_Profile = 0x01c5,
  A010_Cmnd_Transfer_Workout_Limits       = 0x01c6,
  A010_Cmnd_Transfer_Courses              = 0x0231,
  A010_Cmnd_Transfer_Course_Laps          = 0x0232,
  A010_Cmnd_Transfer_Course_Points        = 0x0233,
  A010_Cmnd_Transfer_Course_Tracks        = 0x0234,
  A010_Cmnd_Transfer_Course_Limits        = 0x0235
} A010_command_id;


/* ------------------------------------------------------------------------- */
/* 6.3.2 A011 - Device Command Protocol 2                                    */
/* ------------------------------------------------------------------------- */

typedef enum {
  A011_Cmnd_Abort_Transfer                = 0x0000,
  A011_Cmnd_Transfer_Alm                  = 0x0004,
  A011_Cmnd_Transfer_Rte                  = 0x0008,
  A011_Cmnd_Transfer_Prx                  = 0x0011,
  A011_Cmnd_Transfer_Time                 = 0x0014,
  A011_Cmnd_Transfer_Wpt                  = 0x0015,
  A011_Cmnd_Turn_Off_Pwr                  = 0x001a,  
} A011_command_id;


/* Unified command enum */

typedef enum {

  /* A010 and A011 */

  Cmnd_Abort_Transfer,
  Cmnd_Transfer_Alm,
  Cmnd_Transfer_Prx,
  Cmnd_Transfer_Rte,
  Cmnd_Transfer_Time,
  Cmnd_Transfer_Wpt,
  Cmnd_Turn_Off_Pwr,

  /* A010 only */

  Cmnd_Transfer_Posn,
  Cmnd_Transfer_Trk,
  Cmnd_Start_Pvt_Data,
  Cmnd_Stop_Pvt_Data,
  Cmnd_FlightBook_Transfer,
  Cmnd_Transfer_Laps,
  Cmnd_Transfer_Wpt_Cats,
  Cmnd_Transfer_Runs,
  Cmnd_Transfer_Workouts,
  Cmnd_Transfer_Workout_Occurrences,
  Cmnd_Transfer_Fitness_User_Profile,
  Cmnd_Transfer_Workout_Limits,
  Cmnd_Transfer_Courses,
  Cmnd_Transfer_Course_Laps,
  Cmnd_Transfer_Course_Points,
  Cmnd_Transfer_Course_Tracks,
  Cmnd_Transfer_Course_Limits

} garmin_command;


/* ------------------------------------------------------------------------- */
/* 4.1   L000 - Basic Link Protocol                                          */
/* ------------------------------------------------------------------------- */

typedef enum {
  L000_Pid_Protocol_Array       = 0x00fd,
  L000_Pid_Product_Rqst         = 0x00fe,
  L000_Pid_Product_Data         = 0x00ff,
  L000_Pid_Ext_Product_Data     = 0x00f8
} L000_packet_id;


/* ------------------------------------------------------------------------- */
/* 4.2   L001 - Link Protocol 1                                              */
/* ------------------------------------------------------------------------- */

typedef enum {
  L001_Pid_Command_Data         = 0x000a,
  L001_Pid_Xfer_Cmplt           = 0x000c,
  L001_Pid_Date_Time_Data       = 0x000e,
  L001_Pid_Position_Data        = 0x0011,
  L001_Pid_Prx_Wpt_Data         = 0x0013,
  L001_Pid_Records              = 0x001b,
  /* L001_Pid_Undocumented_1    = 0x001c, */
  L001_Pid_Rte_Hdr              = 0x001d,
  L001_Pid_Rte_Wpt_Data         = 0x001e,
  L001_Pid_Almanac_Data         = 0x001f,
  L001_Pid_Trk_Data             = 0x0022,
  L001_Pid_Wpt_Data             = 0x0023,
  L001_Pid_Pvt_Data             = 0x0033,
  L001_Pid_Rte_Link_Data        = 0x0062,
  L001_Pid_Trk_Hdr              = 0x0063,
  L001_Pid_FlightBook_Record    = 0x0086,
  L001_Pid_Lap                  = 0x0095,
  L001_Pid_Wpt_Cat              = 0x0098,
  L001_Pid_Run                  = 0x03de,
  L001_Pid_Workout              = 0x03df,
  L001_Pid_Workout_Occurrence   = 0x03e0,
  L001_Pid_Fitness_User_Profile = 0x03e1,
  L001_Pid_Workout_Limits       = 0x03e2,
  L001_Pid_Course               = 0x0425,
  L001_Pid_Course_Lap           = 0x0426,
  L001_Pid_Course_Point         = 0x0427,
  L001_Pid_Course_Trk_Hdr       = 0x0428,
  L001_Pid_Course_Trk_Data      = 0x0429,
  L001_Pid_Course_Limits        = 0x042a
} L001_packet_id;


/* ------------------------------------------------------------------------- */
/* 4.3   Link Protocol 2                                                     */
/* ------------------------------------------------------------------------- */

typedef enum {
  L002_Pid_Almanac_Data         = 0x0004,
  L002_Pid_Command_Data         = 0x000b,
  L002_Pid_Xfer_Cmplt           = 0x000c,
  L002_Pid_Date_Time_Data       = 0x0014,
  L002_Pid_Position_Data        = 0x0018,
  L002_Pid_Prx_Wpt_Data         = 0x001b,
  L002_Pid_Records              = 0x0023,
  L002_Pid_Rte_Hdr              = 0x0025,
  L002_Pid_Rte_Wpt_Data         = 0x0027,
  L002_Pid_Wpt_Data             = 0x002b
} L002_packet_id;


/* Unified PID enum */

typedef enum {

  /* Invalid Pid */

  Pid_Nil,
  
  /* L000 Pids */

  Pid_Protocol_Array,
  Pid_Product_Rqst,
  Pid_Product_Data,
  Pid_Ext_Product_Data,

  /* L001 and L002 Pids */

  Pid_Almanac_Data,
  Pid_Command_Data,
  Pid_Xfer_Cmplt,
  Pid_Date_Time_Data,
  Pid_Position_Data,
  Pid_Prx_Wpt_Data,
  Pid_Records,
  Pid_Rte_Hdr,
  Pid_Rte_Wpt_Data,
  Pid_Wpt_Data,

  /* L001 only */

  Pid_Trk_Data,
  Pid_Pvt_Data,
  Pid_Rte_Link_Data,
  Pid_Trk_Hdr,
  Pid_FlightBook_Record,
  Pid_Lap,
  Pid_Wpt_Cat,
  Pid_Run,
  Pid_Workout,
  Pid_Workout_Occurrence,
  Pid_Fitness_User_Profile,
  Pid_Workout_Limits,
  Pid_Course,
  Pid_Course_Lap,
  Pid_Course_Point,
  Pid_Course_Trk_Hdr,
  Pid_Course_Trk_Data,
  Pid_Course_Limits

} garmin_pid;


/* Unified protocol enums */

typedef enum {
  phys_P000 = 0,
  phys_NUM_PROTOCOLS
} phys_protocol;


typedef enum {
  link_L000 = 0,      /* basic link protocol */
  link_L001 = 1,      /* link protocol 1 */
  link_L002 = 2,      /* link protocol 2 */
  link_NUM_PROTOCOLS
} link_protocol;


typedef enum {
  appl_Anil  =    0,
  appl_A000  =    0,      /* product data protocol */
  appl_A001  =    1,      /* protocol capability protocol */
  appl_A010  =   10,      /* device command protocol 1 */
  appl_A011  =   11,      /* device command protocol 2 */
  appl_A100  =  100,      /* waypoint transfer protocol */
  appl_A101  =  101,      /* waypoint category transfer protocol */
  appl_A200  =  200,      /* route transfer protocol */
  appl_A201  =  201,      /* route transfer protocol */
  appl_A300  =  300,      /* track log transfer protocol */
  appl_A301  =  301,      /* track log transfer protocol */
  appl_A302  =  302,      /* track log transfer protocol */
  appl_A400  =  400,      /* proximity waypoint transfer protocol */
  appl_A500  =  500,      /* almanac transfer protocol */
  appl_A600  =  600,      /* date and time initialization protocol */
  appl_A601  =  601,      /* --- UNDOCUMENTED --- */
  appl_A650  =  650,      /* flightbook transfer protocol */
  appl_A700  =  700,      /* position initialization protocol */
  appl_A800  =  800,      /* PVT protocol */
  appl_A801  =  801,      /* --- UNDOCUMENTED --- */
  appl_A902  =  902,      /* --- UNDOCUMENTED --- */
  appl_A903  =  903,      /* --- UNDOCUMENTED --- */
  appl_A906  =  906,      /* lap transfer protocol */
  appl_A907  =  907,      /* --- UNDOCUMENTED --- */
  appl_A1000 = 1000,     /* run transfer protocol */
  appl_A1002 = 1002,     /* workout transfer protocol */
  appl_A1003 = 1003,     /* workout occurrence transfer protocol */
  appl_A1004 = 1004,     /* fitness user profile transfer protocol */
  appl_A1005 = 1005,     /* workout limits transfer protocol */
  appl_A1006 = 1006,     /* course transfer protocol */
  appl_A1007 = 1007,     /* course lap transfer protocol */
  appl_A1008 = 1008,     /* course point transfer protocol */
  appl_A1009 = 1009,     /* course limits transfer protocol */
  appl_A1012 = 1012,     /* course track transfer protocol */
  appl_NUM_PROTOCOLS
} appl_protocol;


/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define GARMIN_MAGIC    "<@gArMiN@>"  /* appears at the start of all files. */
#define GARMIN_VERSION  100           /* version 1.00 */
#define GARMIN_HEADER   20            /* bytes needed for file header. */


/* ========================================================================= */
/* Data structures                                                           */
/* ========================================================================= */

typedef struct waypoint_protocols {
  appl_protocol              waypoint;
  appl_protocol              category;
  appl_protocol              proximity;
} waypoint_protocols;


typedef struct workout_protocols {
  appl_protocol              workout;
  appl_protocol              occurrence;
  appl_protocol              limits;
} workout_protocols;


typedef struct course_protocols {
  appl_protocol              course;
  appl_protocol              lap;
  appl_protocol              track;
  appl_protocol              point;
  appl_protocol              limits;
} course_protocols;


typedef struct garmin_protocols {
  phys_protocol              physical;  /* The physical protocol */
  link_protocol              link;      /* The link protocol */
  appl_protocol              command;   /* The device command protocol */
  waypoint_protocols         waypoint;  /* Application protocols */
  appl_protocol              route;
  appl_protocol              track;
  appl_protocol              almanac;
  appl_protocol              date_time;
  appl_protocol              flightbook;
  appl_protocol              position;
  appl_protocol              pvt;
  appl_protocol              lap;
  appl_protocol              run;
  workout_protocols          workout;
  appl_protocol              fitness;
  course_protocols           course;
} garmin_protocols;


typedef struct waypoint_datatypes {
  garmin_datatype            waypoint;
  garmin_datatype            category;
  garmin_datatype            proximity;
} waypoint_datatypes;


typedef struct route_datatypes {
  garmin_datatype            header;
  garmin_datatype            waypoint;
  garmin_datatype            link;
} route_datatypes;


typedef struct track_datatypes {
  garmin_datatype            header;
  garmin_datatype            data;
} track_datatypes;


typedef struct workout_datatypes {
  garmin_datatype            workout;
  garmin_datatype            occurrence;
  garmin_datatype            limits;
} workout_datatypes;


typedef struct course_datatypes {
  garmin_datatype            course;
  garmin_datatype            lap;
  track_datatypes            track;
  garmin_datatype            point;
  garmin_datatype            limits;
} course_datatypes;


typedef struct garmin_datatypes {
  waypoint_datatypes         waypoint;
  route_datatypes            route;
  track_datatypes            track;
  garmin_datatype            almanac;
  garmin_datatype            date_time;
  garmin_datatype            flightbook;
  garmin_datatype            position;
  garmin_datatype            pvt;
  garmin_datatype            lap;
  garmin_datatype            run;
  workout_datatypes          workout;
  garmin_datatype            fitness;
  course_datatypes           course;
} garmin_datatypes;


typedef struct garmin_usb {
  usb_dev_handle *          handle;
  int                       bulk_out;
  int                       bulk_in;
  int                       intr_in;
  int                       read_bulk;
} garmin_usb;


typedef struct garmin_unit {
  uint32                     id;
  garmin_product             product;
  garmin_extended_data       extended;
  garmin_protocols           protocol;
  garmin_datatypes           datatype;
  garmin_usb                 usb;
  int                        verbose;   /* this may become a 'flags' field. */
} garmin_unit;


typedef enum {
  GET_WAYPOINTS,
  GET_WAYPOINT_CATEGORIES,
  GET_ROUTES,
  GET_TRACKLOG,
  GET_PROXIMITY_WAYPOINTS,
  GET_ALMANAC,
  GET_FLIGHTBOOK,
  GET_RUNS,
  GET_WORKOUTS,
  GET_FITNESS_USER_PROFILE,
  GET_WORKOUT_LIMITS,
  GET_COURSES,
  GET_COURSE_LIMITS
} garmin_get_type;


/* ========================================================================= */
/* Function prototypes                                                       */
/* ========================================================================= */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ------------------------------------------------------------------------- */
/* packet_id.c                                                               */
/* ------------------------------------------------------------------------- */

uint16      garmin_lpid ( link_protocol     link,
			  garmin_pid        gpid );

garmin_pid  garmin_gpid ( link_protocol     link,
			  uint16            lpid );


/* ------------------------------------------------------------------------- */
/* unpack.c                                                                  */
/* ------------------------------------------------------------------------- */

garmin_data * garmin_load          ( const char *     filename );
garmin_data * garmin_unpack_packet ( garmin_packet *  p, 
				     garmin_datatype  type );
garmin_data * garmin_unpack        ( uint8 **         buf,
				     garmin_datatype  type );


/* ------------------------------------------------------------------------- */
/* pack.c                                                                    */
/* ------------------------------------------------------------------------- */

uint32 garmin_save ( garmin_data * data, 
		     const char *  filename, 
		     const char *  dir );
uint32 garmin_pack ( garmin_data * data, 
		     uint8 **      buf );


/* ------------------------------------------------------------------------- */
/* print.c                                                                   */
/* ------------------------------------------------------------------------- */

void garmin_print_data      ( garmin_data * data, FILE * fp, int spaces );
void garmin_print_protocols ( garmin_unit * unit, FILE * fp, int spaces );
void garmin_print_info      ( garmin_unit * unit, FILE * fp, int spaces );


/* ------------------------------------------------------------------------- */
/* command.c                                                                 */
/* ------------------------------------------------------------------------- */


int  garmin_command_supported ( garmin_unit *    garmin,
				garmin_command   cmd );

int  garmin_make_command_packet ( garmin_unit *    garmin, 
				  garmin_command   cmd,
				  garmin_packet *  packet );
     
int  garmin_send_command ( garmin_unit *    garmin, 
			   garmin_command   cmd );


/* ------------------------------------------------------------------------- */
/* protocol.c                                                                */
/* ------------------------------------------------------------------------- */

void garmin_read_a000_a001           ( garmin_unit *    garmin );
garmin_data * garmin_read_a100       ( garmin_unit *    garmin );
garmin_data * garmin_read_a101       ( garmin_unit *    garmin );
garmin_data * garmin_read_a200       ( garmin_unit *    garmin );
garmin_data * garmin_read_a201       ( garmin_unit *    garmin );
garmin_data * garmin_read_a300       ( garmin_unit *    garmin );
garmin_data * garmin_read_a301       ( garmin_unit *    garmin );
garmin_data * garmin_read_a302       ( garmin_unit *    garmin );
garmin_data * garmin_read_a400       ( garmin_unit *    garmin );
garmin_data * garmin_read_a500       ( garmin_unit *    garmin );
garmin_data * garmin_read_a600       ( garmin_unit *    garmin );
garmin_data * garmin_read_a650       ( garmin_unit *    garmin );
garmin_data * garmin_read_a700       ( garmin_unit *    garmin );
garmin_data * garmin_read_a800       ( garmin_unit *    garmin );
garmin_data * garmin_read_a906       ( garmin_unit *    garmin );
garmin_data * garmin_read_a1000      ( garmin_unit *    garmin );
garmin_data * garmin_read_a1002      ( garmin_unit *    garmin );
garmin_data * garmin_read_a1003      ( garmin_unit *    garmin );
garmin_data * garmin_read_a1004      ( garmin_unit *    garmin );
garmin_data * garmin_read_a1005      ( garmin_unit *    garmin );
garmin_data * garmin_read_a1006      ( garmin_unit *    garmin );
garmin_data * garmin_read_a1007      ( garmin_unit *    garmin );
garmin_data * garmin_read_a1008      ( garmin_unit *    garmin );
garmin_data * garmin_read_a1009      ( garmin_unit *    garmin );
garmin_data * garmin_read_a1012      ( garmin_unit *    garmin );

garmin_data * garmin_read_via        ( garmin_unit *    garmin, 
				       appl_protocol    protocol );
garmin_data * garmin_get             ( garmin_unit *    garmin, 
				       garmin_get_type  what );
int           garmin_init            ( garmin_unit *    garmin,
				       int              verbose );


/* ------------------------------------------------------------------------- */
/* usb_comm.c                                                                */
/* ------------------------------------------------------------------------- */

int     garmin_open           ( garmin_unit * garmin );
int     garmin_close          ( garmin_unit * garmin );
uint32  garmin_start_session  ( garmin_unit * garmin );
int     garmin_read           ( garmin_unit * garmin, garmin_packet * p );
int     garmin_write          ( garmin_unit * garmin, garmin_packet * p );
uint8   garmin_packet_type    ( garmin_packet * p );
uint16  garmin_packet_id      ( garmin_packet * p );
uint32  garmin_packet_size    ( garmin_packet * p );
uint8 * garmin_packet_data    ( garmin_packet * p );

void    garmin_print_packet   ( garmin_packet *  p, 
				int              dir, 
				FILE *           fp );
int     garmin_packetize      ( garmin_packet *  p,
				uint16           id, 
				uint32           size, 
				uint8 *          data );


/* ------------------------------------------------------------------------- */
/* byte_util.c                                                               */
/* ------------------------------------------------------------------------- */

uint16   get_uint16  ( const uint8 * d );
sint16   get_sint16  ( const uint8 * d );
uint32   get_uint32  ( const uint8 * d );
sint32   get_sint32  ( const uint8 * d );
float32  get_float32 ( const uint8 * d );
float64  get_float64 ( const uint8 * d );

void     put_uint16  ( uint8 * d, const uint16  v );
void     put_sint16  ( uint8 * d, const sint16  v );
void     put_uint32  ( uint8 * d, const uint32  v );
void     put_sint32  ( uint8 * d, const sint32  v );
void     put_float32 ( uint8 * d, const float32 v );
void     put_float64 ( uint8 * d, const float64 v );

char *   get_string  ( garmin_packet * p, int * offset );
char *   get_vstring ( uint8 ** buf );
void     put_vstring ( uint8 ** buf, const char * x );
char **  get_strings ( garmin_packet * p, int * offset );


/* ------------------------------------------------------------------------- */
/* datatype.c                                                                */
/* ------------------------------------------------------------------------- */

garmin_data * garmin_alloc_data     ( garmin_datatype type );
garmin_list * garmin_alloc_list     ( void );
garmin_list * garmin_list_append    ( garmin_list * list, 
				      garmin_data * data );
garmin_data * garmin_list_data      ( garmin_data * data, 
				      uint32        which );
void          garmin_free_list      ( garmin_list * l );
void          garmin_free_list_only ( garmin_list * l );
void          garmin_free_data      ( garmin_data * d );
uint32        garmin_data_size      ( garmin_data * d );


/* ------------------------------------------------------------------------- */
/* symbol_name.c                                                             */
/* ------------------------------------------------------------------------- */

char * garmin_symbol_name ( symbol_value s );


/* ------------------------------------------------------------------------- */
/* run.c                                                                     */
/* ------------------------------------------------------------------------- */

int           get_lap_index          ( garmin_data * lap, uint32 * lap_index );
int           get_lap_start_time     ( garmin_data * lap, time_type * start_time );
int           get_run_track_lap_info ( garmin_data * run,
                                       uint32      * track_index,
                                       uint32      * first_lap_index,
                                       uint32      * last_lap_index );

void          garmin_save_runs       ( garmin_unit * garmin );


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GARMIN_GARMIN_H__ */
