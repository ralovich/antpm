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
#include <stdlib.h>
#include <string.h>
#include "garmin.h"


#ifdef WORDS_BIGENDIAN
#define ENDIAN_FOR(i,x)     for ( i = sizeof(x)-1; i >= 0; i-- )
#define ENDIAN_GET(i,x,y,z) ENDIAN_FOR(i,x) *z++ = y[i]
#define ENDIAN_PUT(i,x,y,z) ENDIAN_FOR(i,x) z[i] = *y++
#else /* WORDS_BIGENDIAN */
#define ENDIAN_FOR(i,x)     for ( i = 0; i < (int)sizeof(x); i++ )
#define ENDIAN_GET(i,x,y,z) ENDIAN_FOR(i,x) *z++ = y[i]
#define ENDIAN_PUT(i,x,y,z) ENDIAN_FOR(i,x) z[i] = *y++
#endif /* WORDS_BIGENDIAN */

#define DEF_ENDIAN_GET(x)          \
  x                                \
  get_##x ( const uint8 * d )      \
  {                                \
    x       v;                     \
    uint8 * b;                     \
    int     i;                     \
                                   \
    b = (uint8 *)&v;               \
    ENDIAN_GET(i,x,d,b);           \
                                   \
    return v;                      \
  }


#define DEF_ENDIAN_PUT(x)          \
  void                             \
  put_##x ( uint8 * d, const x v ) \
  {                                \
    uint8 * b;                     \
    int     i;                     \
                                   \
    b = (uint8 *)&v;               \
    ENDIAN_PUT(i,x,b,d);           \
  }


/* Here are the definitions for the get/put functions. */

DEF_ENDIAN_GET(uint16)
DEF_ENDIAN_GET(sint16)
DEF_ENDIAN_GET(uint32)
DEF_ENDIAN_GET(sint32)
DEF_ENDIAN_GET(float32)
DEF_ENDIAN_GET(float64)

DEF_ENDIAN_PUT(uint16)
DEF_ENDIAN_PUT(sint16)
DEF_ENDIAN_PUT(uint32)
DEF_ENDIAN_PUT(sint32)
DEF_ENDIAN_PUT(float32)
DEF_ENDIAN_PUT(float64)


/* 
   Return a memory-allocated, NULL-terminated string and set the 'pos'
   argument to point to the next position. 
*/

char *
get_string ( garmin_packet * p, int * offset )
{
  char * start  = p->packet.data + *offset;
  char * cursor = start;
  int    allow  = garmin_packet_size(p) - *offset;
  char * ret    = NULL;
  int    bytes  = 0;

  /* early exit */

  if ( allow <= 0 ) return NULL;

  /* OK, we have space to work with. */

  do { bytes++; } while ( --allow && *cursor++ );
  
  ret = malloc(bytes);
  strncpy(ret,start,bytes-1);
  
  *offset += bytes;

  return ret;
}


/* 
   Same as above, but straight from the buffer until we hit a NULL, and
   update the buffer to point to the start of the next string.
*/

char *
get_vstring ( uint8 ** buf )
{
  char * start  = *buf;
  char * cursor = start;
  char * ret    = NULL;
  int    bytes  = 0;

  do { bytes++; } while ( *cursor++ );

  ret = malloc(bytes);
  strncpy(ret,start,bytes-1);

  *buf += bytes;

  return ret;
}


void
put_vstring ( uint8 ** buf, const char * x )
{
  if ( x != NULL ) {
    strcpy(*buf,x);
    *buf += strlen(x)+1;
  }
}


/* 
   Return a NULL-terminated list of strings, and set the 'pos' argument
   to point to the next position.
*/

char **
get_strings ( garmin_packet * p, int * offset )
{
  char *  start  = p->packet.data + *offset;
  char *  cursor = start;
  int     allow  = garmin_packet_size(p) - *offset;
  char ** ret    = NULL;
  char *  elem   = NULL;
  int     nstr   = 0;
  int     bytes  = 0;

  /* early exit */

  if ( allow <= 0 ) return NULL;

  /* OK, we have space to work with. */

  while ( allow ) {

    /* extract the next string from the buffer */

    do { bytes++; } while ( --allow && *cursor++ );

    elem = malloc(bytes);
    strncpy(elem,start,bytes-1);

    /* append it to the list of strings */

    if ( ret != NULL ) ret = realloc(ret,(nstr+2) * sizeof(char *));
    else               ret = malloc(2 * sizeof(char *));

    ret[nstr++] = elem;
    ret[nstr]   = NULL;

    /* update the offset */

    *offset += bytes;
  }

  return ret;
}
