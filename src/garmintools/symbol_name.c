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

#include "garmin.h"


#define SYMBOL_NAME                     \
  char *                                \
  garmin_symbol_name ( symbol_value s ) \
  {                                     \
    char * name = "unknown";            \
                                        \
    switch ( s )

#define SYMBOL_CASE(x) case x: name = #x; break

#define SYMBOL_DEFAULT                  \
    default: break;                     \
    }                                   \
                                        \
    return name


SYMBOL_NAME {
  SYMBOL_CASE(sym_anchor);
  SYMBOL_CASE(sym_bell);
  SYMBOL_CASE(sym_diamond_grn);
  SYMBOL_CASE(sym_diamond_red);
  SYMBOL_CASE(sym_dive1);
  SYMBOL_CASE(sym_dive2);
  SYMBOL_CASE(sym_dollar);
  SYMBOL_CASE(sym_fish);
  SYMBOL_CASE(sym_fuel);
  SYMBOL_CASE(sym_horn);
  SYMBOL_CASE(sym_house);
  SYMBOL_CASE(sym_knife);
  SYMBOL_CASE(sym_light);
  SYMBOL_CASE(sym_mug);
  SYMBOL_CASE(sym_skull);
  SYMBOL_CASE(sym_square_grn);
  SYMBOL_CASE(sym_square_red);
  SYMBOL_CASE(sym_wbuoy);
  SYMBOL_CASE(sym_wpt_dot);
  SYMBOL_CASE(sym_wreck);
  SYMBOL_CASE(sym_null);
  SYMBOL_CASE(sym_mob);
  SYMBOL_CASE(sym_buoy_ambr);
  SYMBOL_CASE(sym_buoy_blck);
  SYMBOL_CASE(sym_buoy_blue);
  SYMBOL_CASE(sym_buoy_grn);
  SYMBOL_CASE(sym_buoy_grn_red);
  SYMBOL_CASE(sym_buoy_grn_wht);
  SYMBOL_CASE(sym_buoy_orng);
  SYMBOL_CASE(sym_buoy_red);
  SYMBOL_CASE(sym_buoy_red_grn);
  SYMBOL_CASE(sym_buoy_red_wht);
  SYMBOL_CASE(sym_buoy_violet);
  SYMBOL_CASE(sym_buoy_wht);
  SYMBOL_CASE(sym_buoy_wht_grn);
  SYMBOL_CASE(sym_buoy_wht_red);
  SYMBOL_CASE(sym_dot);
  SYMBOL_CASE(sym_rbcn);
  SYMBOL_CASE(sym_boat_ramp);
  SYMBOL_CASE(sym_camp);
  SYMBOL_CASE(sym_restrooms);
  SYMBOL_CASE(sym_showers);
  SYMBOL_CASE(sym_drinking_wtr);
  SYMBOL_CASE(sym_phone);
  SYMBOL_CASE(sym_1st_aid);
  SYMBOL_CASE(sym_info);
  SYMBOL_CASE(sym_parking);
  SYMBOL_CASE(sym_park);
  SYMBOL_CASE(sym_picnic);
  SYMBOL_CASE(sym_scenic);
  SYMBOL_CASE(sym_skiing);
  SYMBOL_CASE(sym_swimming);
  SYMBOL_CASE(sym_dam);
  SYMBOL_CASE(sym_controlled);
  SYMBOL_CASE(sym_danger);
  SYMBOL_CASE(sym_restricted);
  SYMBOL_CASE(sym_null_2);
  SYMBOL_CASE(sym_ball);
  SYMBOL_CASE(sym_car);
  SYMBOL_CASE(sym_deer);
  SYMBOL_CASE(sym_shpng_cart);
  SYMBOL_CASE(sym_lodging);
  SYMBOL_CASE(sym_mine);
  SYMBOL_CASE(sym_trail_head);
  SYMBOL_CASE(sym_truck_stop);
  SYMBOL_CASE(sym_user_exit);
  SYMBOL_CASE(sym_flag);
  SYMBOL_CASE(sym_circle_x);
  SYMBOL_CASE(sym_open_24hr);
  SYMBOL_CASE(sym_fhs_facility);
  SYMBOL_CASE(sym_bot_cond);
  SYMBOL_CASE(sym_tide_pred_stn);
  SYMBOL_CASE(sym_anchor_prohib);
  SYMBOL_CASE(sym_beacon);
  SYMBOL_CASE(sym_coast_guard);
  SYMBOL_CASE(sym_reef);
  SYMBOL_CASE(sym_weedbed);
  SYMBOL_CASE(sym_dropoff);
  SYMBOL_CASE(sym_dock);
  SYMBOL_CASE(sym_marina);
  SYMBOL_CASE(sym_bait_tackle);
  SYMBOL_CASE(sym_stump);
  SYMBOL_CASE(sym_begin_custom);
  SYMBOL_CASE(sym_end_custom);
  SYMBOL_CASE(sym_is_hwy);
  SYMBOL_CASE(sym_us_hwy);
  SYMBOL_CASE(sym_st_hwy);
  SYMBOL_CASE(sym_mi_mrkr);
  SYMBOL_CASE(sym_trcbck);
  SYMBOL_CASE(sym_golf);
  SYMBOL_CASE(sym_sml_cty);
  SYMBOL_CASE(sym_med_cty);
  SYMBOL_CASE(sym_lrg_cty);
  SYMBOL_CASE(sym_freeway);
  SYMBOL_CASE(sym_ntl_hwy);
  SYMBOL_CASE(sym_cap_cty);
  SYMBOL_CASE(sym_amuse_pk);
  SYMBOL_CASE(sym_bowling);
  SYMBOL_CASE(sym_car_rental);
  SYMBOL_CASE(sym_car_repair);
  SYMBOL_CASE(sym_fastfood);
  SYMBOL_CASE(sym_fitness);
  SYMBOL_CASE(sym_movie);
  SYMBOL_CASE(sym_museum);
  SYMBOL_CASE(sym_pharmacy);
  SYMBOL_CASE(sym_pizza);
  SYMBOL_CASE(sym_post_ofc);
  SYMBOL_CASE(sym_rv_park);
  SYMBOL_CASE(sym_school);
  SYMBOL_CASE(sym_stadium);
  SYMBOL_CASE(sym_store);
  SYMBOL_CASE(sym_zoo);
  SYMBOL_CASE(sym_gas_plus);
  SYMBOL_CASE(sym_faces);
  SYMBOL_CASE(sym_ramp_int);
  SYMBOL_CASE(sym_st_int);
  SYMBOL_CASE(sym_weigh_sttn);
  SYMBOL_CASE(sym_toll_booth);
  SYMBOL_CASE(sym_elev_pt);
  SYMBOL_CASE(sym_ex_no_srvc);
  SYMBOL_CASE(sym_geo_place_mm);
  SYMBOL_CASE(sym_geo_place_wtr);
  SYMBOL_CASE(sym_geo_place_lnd);
  SYMBOL_CASE(sym_bridge);
  SYMBOL_CASE(sym_building);
  SYMBOL_CASE(sym_cemetery);
  SYMBOL_CASE(sym_church);
  SYMBOL_CASE(sym_civil);
  SYMBOL_CASE(sym_crossing);
  SYMBOL_CASE(sym_hist_town);
  SYMBOL_CASE(sym_levee);
  SYMBOL_CASE(sym_military);
  SYMBOL_CASE(sym_oil_field);
  SYMBOL_CASE(sym_tunnel);
  SYMBOL_CASE(sym_beach);
  SYMBOL_CASE(sym_forest);
  SYMBOL_CASE(sym_summit);
  SYMBOL_CASE(sym_lrg_ramp_int);
  SYMBOL_CASE(sym_lrg_ex_no_srvc);
  SYMBOL_CASE(sym_badge);
  SYMBOL_CASE(sym_cards);
  SYMBOL_CASE(sym_snowski);
  SYMBOL_CASE(sym_iceskate);
  SYMBOL_CASE(sym_wrecker);
  SYMBOL_CASE(sym_border);
  SYMBOL_CASE(sym_geocache);
  SYMBOL_CASE(sym_geocache_fnd);
  SYMBOL_CASE(sym_cntct_smiley);
  SYMBOL_CASE(sym_cntct_ball_cap);
  SYMBOL_CASE(sym_cntct_big_ears);
  SYMBOL_CASE(sym_cntct_spike);
  SYMBOL_CASE(sym_cntct_goatee);
  SYMBOL_CASE(sym_cntct_afro);
  SYMBOL_CASE(sym_cntct_dreads);
  SYMBOL_CASE(sym_cntct_female1);
  SYMBOL_CASE(sym_cntct_female2);
  SYMBOL_CASE(sym_cntct_female3);
  SYMBOL_CASE(sym_cntct_ranger);
  SYMBOL_CASE(sym_cntct_kung_fu);
  SYMBOL_CASE(sym_cntct_sumo);
  SYMBOL_CASE(sym_cntct_pirate);
  SYMBOL_CASE(sym_cntct_biker);
  SYMBOL_CASE(sym_cntct_alien);
  SYMBOL_CASE(sym_cntct_bug);
  SYMBOL_CASE(sym_cntct_cat);
  SYMBOL_CASE(sym_cntct_dog);
  SYMBOL_CASE(sym_cntct_pig);
  SYMBOL_CASE(sym_hydrant);
  SYMBOL_CASE(sym_flag_blue);
  SYMBOL_CASE(sym_flag_green);
  SYMBOL_CASE(sym_flag_red);
  SYMBOL_CASE(sym_pin_blue);
  SYMBOL_CASE(sym_pin_green);
  SYMBOL_CASE(sym_pin_red);
  SYMBOL_CASE(sym_block_blue);
  SYMBOL_CASE(sym_block_green);
  SYMBOL_CASE(sym_block_red);
  SYMBOL_CASE(sym_bike_trail);
  SYMBOL_CASE(sym_circle_red);
  SYMBOL_CASE(sym_circle_green);
  SYMBOL_CASE(sym_circle_blue);
  SYMBOL_CASE(sym_diamond_blue);
  SYMBOL_CASE(sym_oval_red);
  SYMBOL_CASE(sym_oval_green);
  SYMBOL_CASE(sym_oval_blue);
  SYMBOL_CASE(sym_rect_red);
  SYMBOL_CASE(sym_rect_green);
  SYMBOL_CASE(sym_rect_blue);
  SYMBOL_CASE(sym_square_blue);
  SYMBOL_CASE(sym_letter_a_red);
  SYMBOL_CASE(sym_letter_b_red);
  SYMBOL_CASE(sym_letter_c_red);
  SYMBOL_CASE(sym_letter_d_red);
  SYMBOL_CASE(sym_letter_a_green);
  SYMBOL_CASE(sym_letter_b_green);
  SYMBOL_CASE(sym_letter_c_green);
  SYMBOL_CASE(sym_letter_d_green);
  SYMBOL_CASE(sym_letter_a_blue);
  SYMBOL_CASE(sym_letter_b_blue);
  SYMBOL_CASE(sym_letter_c_blue);
  SYMBOL_CASE(sym_letter_d_blue);
  SYMBOL_CASE(sym_number_0_red);
  SYMBOL_CASE(sym_number_1_red);
  SYMBOL_CASE(sym_number_2_red);
  SYMBOL_CASE(sym_number_3_red);
  SYMBOL_CASE(sym_number_4_red);
  SYMBOL_CASE(sym_number_5_red);
  SYMBOL_CASE(sym_number_6_red);
  SYMBOL_CASE(sym_number_7_red);
  SYMBOL_CASE(sym_number_8_red);
  SYMBOL_CASE(sym_number_9_red);
  SYMBOL_CASE(sym_number_0_green);
  SYMBOL_CASE(sym_number_1_green);
  SYMBOL_CASE(sym_number_2_green);
  SYMBOL_CASE(sym_number_3_green);
  SYMBOL_CASE(sym_number_4_green);
  SYMBOL_CASE(sym_number_5_green);
  SYMBOL_CASE(sym_number_6_green);
  SYMBOL_CASE(sym_number_7_green);
  SYMBOL_CASE(sym_number_8_green);
  SYMBOL_CASE(sym_number_9_green);
  SYMBOL_CASE(sym_number_0_blue);
  SYMBOL_CASE(sym_number_1_blue);
  SYMBOL_CASE(sym_number_2_blue);
  SYMBOL_CASE(sym_number_3_blue);
  SYMBOL_CASE(sym_number_4_blue);
  SYMBOL_CASE(sym_number_5_blue);
  SYMBOL_CASE(sym_number_6_blue);
  SYMBOL_CASE(sym_number_7_blue);
  SYMBOL_CASE(sym_number_8_blue);
  SYMBOL_CASE(sym_number_9_blue);
  SYMBOL_CASE(sym_triangle_blue);
  SYMBOL_CASE(sym_triangle_green);
  SYMBOL_CASE(sym_triangle_red);
  SYMBOL_CASE(sym_food_asian);
  SYMBOL_CASE(sym_food_deli);
  SYMBOL_CASE(sym_food_italian);
  SYMBOL_CASE(sym_food_seafood);
  SYMBOL_CASE(sym_food_steak);
  SYMBOL_CASE(sym_airport);
  SYMBOL_CASE(sym_int);
  SYMBOL_CASE(sym_ndb);
  SYMBOL_CASE(sym_vor);
  SYMBOL_CASE(sym_heliport);
  SYMBOL_CASE(sym_private);
  SYMBOL_CASE(sym_soft_fld);
  SYMBOL_CASE(sym_tall_tower);
  SYMBOL_CASE(sym_short_tower);
  SYMBOL_CASE(sym_glider);
  SYMBOL_CASE(sym_ultralight);
  SYMBOL_CASE(sym_parachute);
  SYMBOL_CASE(sym_vortac);
  SYMBOL_CASE(sym_vordme);
  SYMBOL_CASE(sym_faf);
  SYMBOL_CASE(sym_lom);
  SYMBOL_CASE(sym_map);
  SYMBOL_CASE(sym_tacan);
  SYMBOL_CASE(sym_seaplane);
  SYMBOL_DEFAULT;
}

