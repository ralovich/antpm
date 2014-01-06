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


static uint32 gListId = 0;


garmin_data *
garmin_alloc_data ( garmin_datatype type )
{
  garmin_data * d = malloc(sizeof(garmin_data));

  d->type = type;

#define CASE_DATA(x) \
  case data_D##x: d->data = calloc(1,sizeof(D##x)); break

  switch ( type ) {
  case data_Dlist: d->data = garmin_alloc_list(); break;
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
  default: d->data = NULL; break;
  }

  return d;
}


garmin_list *
garmin_alloc_list ( void )
{
  garmin_list * l;

  l = calloc(1,sizeof(garmin_list));
  l->id = ++gListId;

  return l;
}


garmin_list *
garmin_list_append ( garmin_list * list, garmin_data * data )
{
  garmin_list *      l = list;
  garmin_list_node * n;

  if ( data != NULL ) {
    if ( l == NULL ) l = garmin_alloc_list();
    n = malloc(sizeof(garmin_list_node));

    n->data = data;
    n->next = NULL;
    
    if ( l->head == NULL ) l->head = n;
    if ( l->tail != NULL ) l->tail->next = n;
    l->tail = n;
    
    l->elements++;
  }

  return l;
}


garmin_data *
garmin_list_data ( garmin_data * data, uint32 which )
{
  garmin_data *       ret = NULL;
  garmin_list *       list;
  garmin_list_node *  n;
  int                 i;

  if ( data                 != NULL       && 
       data->type           == data_Dlist && 
       (list = data->data)  != NULL ) {
    for ( i = 0, n = list->head; i < which && n != NULL; i++, n = n->next );
    if ( n != NULL ) ret = n->data;
  }

  return ret;
}


void
garmin_free_list ( garmin_list * l )
{
  garmin_list_node * n;
  garmin_list_node * x;

  if ( l != NULL ) {
    for ( n = l->head; n != NULL; n = x ) {
      x = n->next;
      garmin_free_data(n->data);
      free(n);
    }
    free(l);
  }
}


void
garmin_free_list_only ( garmin_list * l )
{
  garmin_list_node * n;
  garmin_list_node * x;

  if ( l != NULL ) {
    for ( n = l->head; n != NULL; n = x ) {
      x = n->next;
      free(n);
    }
    free(l);
  }
}


#define TRYFREE(x) if ( x != NULL ) free(x)


void
garmin_free_data ( garmin_data * d )
{
  D105 *   d105;
  D106 *   d106;
  D108 *   d108;
  D109 *   d109;
  D110 *   d110;
  D202 *   d202;
  D210 *   d210;
  D310 *   d310;
  D312 *   d312;
  D650 *   d650;

  if ( d != NULL ) {
    if ( d->data != NULL ) {
      if ( d->type == data_Dlist ) {
	garmin_free_list((garmin_list *)d->data);
      } else {
	switch ( d->type ) {
	case data_D105:
	  d105 = d->data;
	  TRYFREE(d105->wpt_ident);
	  break;
	case data_D106:
	  d106 = d->data;
	  TRYFREE(d106->wpt_ident);
	  TRYFREE(d106->lnk_ident);
	  break;
	case data_D108:
	  d108 = d->data;
	  TRYFREE(d108->ident);
	  TRYFREE(d108->comment);
	  TRYFREE(d108->facility);
	  TRYFREE(d108->city);
	  TRYFREE(d108->addr);
	  TRYFREE(d108->cross_road);
	  break;
	case data_D109:
	  d109 = d->data;
	  TRYFREE(d109->ident);
	  TRYFREE(d109->comment);
	  TRYFREE(d109->facility);
	  TRYFREE(d109->city);
	  TRYFREE(d109->addr);
	  TRYFREE(d109->cross_road);
	  break;
	case data_D110:
	  d110 = d->data;
	  TRYFREE(d110->ident);
	  TRYFREE(d110->comment);
	  TRYFREE(d110->facility);
	  TRYFREE(d110->city);
	  TRYFREE(d110->addr);
	  TRYFREE(d110->cross_road);
	  break;
	case data_D202:
	  d202 = d->data;
	  TRYFREE(d202->rte_ident);
	  break;
	case data_D210:
	  d210 = d->data;
	  TRYFREE(d210->ident);
	  break;
	case data_D310:
	  d310 = d->data;
	  TRYFREE(d310->trk_ident);
	  break;
	case data_D312:
	  d312 = d->data;
	  TRYFREE(d312->trk_ident);
	  break;
	case data_D650:
	  d650 = d->data;
	  TRYFREE(d650->departure_name);
	  TRYFREE(d650->departure_ident);
	  TRYFREE(d650->arrival_name);
	  TRYFREE(d650->arrival_ident);
	  TRYFREE(d650->ac_id);
	  break;
	default:
	  break;
	}
	free(d->data);
      }
    }
    free(d);
  }
}


/* 
   Returns the number of bytes needed in order to serialize the data.  Note
   that this is an upper bound!  Some of the Garmin data structures do not
   align to word boundaries.  Their memory representation will take up more
   bytes than the serialized representation will.
*/

uint32
garmin_data_size ( garmin_data * d )
{
  garmin_list *       list;
  garmin_list_node *  node;
  uint32              bytes = 0;

  /* 
     The number of bytes needed in order to serialize a Garmin data structure
     is almost equal to the size of its data structure - but not quite.  If
     we have variable length strings, we need to add their string lengths
     (including space for the terminating '\0') and subtract the size of the 
     char * placeholders.  We also need 4 bytes for the data type, and 4
     additional bytes in which we store the number of bytes that we should
     seek forward in order to skip this record.  This allows us to handle
     files that include new, unrecognized data types but still conform to
     the file format rules (i.e. we can skip data records that we don't
     yet know about).
  */

#define ADDBYTES(x,y) \
  bytes = sizeof(D##x) + 8 - 3 * y

#define ADDSTRING(x,y) \
  if (d##x->y != NULL) bytes += strlen(d##x->y)

#define ADDBYTES1(x,a) \
  ADDBYTES(x,1);  \
  ADDSTRING(x,a)

#define ADDBYTES2(x,a,b) \
  ADDBYTES(x,2);  \
  ADDSTRING(x,a); \
  ADDSTRING(x,b)

#define ADDBYTES5(x,a,b,c,d,e) \
  ADDBYTES(x,5);  \
  ADDSTRING(x,a); \
  ADDSTRING(x,b); \
  ADDSTRING(x,c); \
  ADDSTRING(x,d); \
  ADDSTRING(x,e)

#define ADDBYTES6(x,a,b,c,d,e,f) \
  ADDBYTES(x,6);  \
  ADDSTRING(x,a); \
  ADDSTRING(x,b); \
  ADDSTRING(x,c); \
  ADDSTRING(x,d); \
  ADDSTRING(x,e); \
  ADDSTRING(x,f)

#define DATASIZE0(x)              \
  case data_D##x:                 \
    {                             \
      ADDBYTES(x,0);              \
    }                             \
    break

#define DATASIZE(x,y)             \
  case data_D##x:                 \
    {                             \
      D##x * d##x = d->data;      \
                                  \
      y;                          \
    }                             \
    break

#define DATASIZE1(x,a)             DATASIZE(x,ADDBYTES1(x,a))
#define DATASIZE2(x,a,b)           DATASIZE(x,ADDBYTES2(x,a,b))
#define DATASIZE5(x,a,b,c,d,e)     DATASIZE(x,ADDBYTES5(x,a,b,c,d,e))
#define DATASIZE6(x,a,b,c,d,e,f)   DATASIZE(x,ADDBYTES6(x,a,b,c,d,e,f))


  if ( d != NULL ) {
    if ( d->data != NULL ) {
      if ( d->type == data_Dlist ) {
	list = d->data;
	bytes += 16;  /* { datatype, bytes, list ID, element count } */
	for ( node = list->head; node != NULL; node = node->next ) {
	  bytes += 4; /* list ID */
	  bytes += garmin_data_size(node->data);
	}
      } else {
	switch ( d->type ) {
        DATASIZE0(100);
        DATASIZE0(101);
        DATASIZE0(102);
        DATASIZE0(103);
        DATASIZE0(104);
	DATASIZE1(105,wpt_ident);
	DATASIZE2(106,wpt_ident,lnk_ident);
	DATASIZE0(107);
	DATASIZE6(108,ident,comment,facility,city,addr,cross_road);
	DATASIZE6(109,ident,comment,facility,city,addr,cross_road);
	DATASIZE6(110,ident,comment,facility,city,addr,cross_road);
	DATASIZE0(120);
	DATASIZE0(150);
	DATASIZE0(151);
	DATASIZE0(152);
	DATASIZE0(154);
	DATASIZE0(155);
	DATASIZE0(200);
	DATASIZE0(201);
	DATASIZE1(202,rte_ident);
	DATASIZE1(210,ident);
	DATASIZE0(300);
	DATASIZE0(301);
	DATASIZE0(302);
	DATASIZE0(303);
	DATASIZE0(304);
	DATASIZE1(310,trk_ident);
	DATASIZE0(311);
	DATASIZE1(312,trk_ident);
	DATASIZE0(400);
	DATASIZE0(403);
	DATASIZE0(450);
	DATASIZE0(500);
	DATASIZE0(501);
	DATASIZE0(550);
	DATASIZE0(551);
	DATASIZE0(600);
	DATASIZE5(650,departure_name,departure_ident,
		  arrival_name,arrival_ident,ac_id);
	DATASIZE0(700);
	DATASIZE0(800);
	DATASIZE0(906);
	DATASIZE0(1000);
	DATASIZE0(1001);
	DATASIZE0(1002);
	DATASIZE0(1003);
	DATASIZE0(1004);
	DATASIZE0(1005);
	DATASIZE0(1006);
	DATASIZE0(1007);
	DATASIZE0(1008);
	DATASIZE0(1009);
	DATASIZE0(1010);
	DATASIZE0(1011);
	DATASIZE0(1012);
	DATASIZE0(1013);
	DATASIZE0(1015);
	default:
	  printf("garmin_data_size: data type %d not supported\n",d->type);
	  break;
	}
      }
    }
  }

  return bytes;
}
