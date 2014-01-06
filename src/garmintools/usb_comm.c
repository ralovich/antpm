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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <usb.h>
#include "garmin.h"


#define INTR_TIMEOUT  3000
#define BULK_TIMEOUT  3000


/* Close the USB connection with the Garmin device. */

int
garmin_close ( garmin_unit * garmin )
{
  if ( garmin->usb.handle != NULL ) {
    usb_release_interface(garmin->usb.handle,0);
    usb_close(garmin->usb.handle);
    garmin->usb.handle = NULL;
  }

  return 0;
}


/* 
   Open the USB connection with the first Garmin device we find.  Eventually,
   I'd like to add the ability to select a particular device.  Returns 1 on
   success, 0 on failure.  Prints diagnostic information and errors to stdout.
*/

int
garmin_open ( garmin_unit * garmin )
{
  struct usb_bus *     bi;
  struct usb_device *  di;
  int                  err = 0;
  int                  i;

  if ( garmin->usb.handle == NULL ) {
    usb_init();
    usb_find_busses();
    usb_find_devices();
    
    for ( bi = usb_busses; bi != NULL; bi = bi->next ) {
      for ( di = bi->devices; di != NULL; di = di->next ) {
	if ( di->descriptor.idVendor  == GARMIN_USB_VID &&
	     di->descriptor.idProduct == GARMIN_USB_PID ) {

	  if ( garmin->verbose != 0 ) {
	    printf("[garmin] found VID %04x, PID %04x on %s/%s\n",
		   di->descriptor.idVendor,
		   di->descriptor.idProduct,
		   bi->dirname,
		   di->filename);
	  }

	  garmin->usb.handle = usb_open(di);
	  garmin->usb.read_bulk = 0;

	  err = 0;

	  if ( garmin->usb.handle == NULL ) {
	    printf("usb_open failed: %s\n",usb_strerror());
	    err = 1;
	  } else if ( !err && garmin->verbose != 0 ) {
	    printf("[garmin] usb_open = %p\n",garmin->usb.handle);
	  }

	  if ( !err && usb_set_configuration(garmin->usb.handle,1) < 0 ) {
	    printf("usb_set_configuration failed: %s\n",usb_strerror());
	    err = 1;
	  } else if ( !err && garmin->verbose != 0 ) {
	    printf("[garmin] usb_set_configuration[1] succeeded\n");
	  }

	  if ( !err && usb_claim_interface(garmin->usb.handle,0) < 0 ) {
	    printf("usb_claim_interface failed: %s\n",usb_strerror());
	    err = 1;
	  } else if ( !err && garmin->verbose != 0 ) {
	    printf("[garmin] usb_claim_interface[0] succeeded\n");
	  }

	  if ( !err ) {

	    /* 
	       We've succeeded in opening and claiming the interface 
	       Let's set the bulk and interrupt in and out endpoints. 
	    */

	    for ( i = 0; 
		  i < di->config->interface->altsetting->bNumEndpoints; 
		  i++ ) {
	      struct usb_endpoint_descriptor * ep;
	      
	      ep = &di->config->interface->altsetting->endpoint[i];
	      switch ( ep->bmAttributes & USB_ENDPOINT_TYPE_MASK ) {
	      case USB_ENDPOINT_TYPE_BULK:
		if ( ep->bEndpointAddress & USB_ENDPOINT_DIR_MASK ) {
		  garmin->usb.bulk_in = 
		    ep->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
		  if ( garmin->verbose != 0 ) {
		    printf("[garmin] bulk IN  = %d\n",garmin->usb.bulk_in);
		  }
		} else {
		  garmin->usb.bulk_out = 
		    ep->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
		  if ( garmin->verbose != 0 ) {
		    printf("[garmin] bulk OUT = %d\n",garmin->usb.bulk_out);
		  }
		}
		break;
	      case USB_ENDPOINT_TYPE_INTERRUPT:
		if ( ep->bEndpointAddress & USB_ENDPOINT_DIR_MASK ) {
		  garmin->usb.intr_in = 
		    ep->bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
		  if ( garmin->verbose != 0 ) {
		    printf("[garmin] intr IN  = %d\n",garmin->usb.intr_in);
		  }
		}
		break;
	      default:
		break;
	      }
	    }
	  }

	  /* We've found what should be the Garmin interface. */

	  break;
	}
      }

      if ( garmin->usb.handle != NULL ) break;
    }
  }

  /* 
     If the USB handle is open but we experienced an error in setting the
     configuration or claiming the interface, close the USB handle and set
     it to NULL.
  */

  if ( garmin->usb.handle != NULL && err != 0 ) {
    if ( garmin->verbose != 0 ) {
      printf("[garmin] (err = %d) usb_close(%p)\n",err,garmin->usb.handle);
    }
    usb_close(garmin->usb.handle);
    garmin->usb.handle = NULL;
  }

  return (garmin->usb.handle != NULL);
}


uint8
garmin_packet_type ( garmin_packet * p )
{
  return p->packet.type;
}


uint16
garmin_packet_id ( garmin_packet * p )
{
  return get_uint16(p->packet.id);
}


uint32
garmin_packet_size ( garmin_packet * p )
{
  return get_uint32(p->packet.size);
}


uint8 *
garmin_packet_data ( garmin_packet * p )
{
  return p->packet.data;
}


int
garmin_packetize ( garmin_packet *  p,
		   uint16           id, 
		   uint32           size, 
		   uint8 *          data )
{
  int ok = 0;

  if ( size + PACKET_HEADER_SIZE < sizeof(garmin_packet) ) {
    p->packet.type       = GARMIN_PROTOCOL_APP;
    p->packet.reserved1  = 0;
    p->packet.reserved2  = 0;
    p->packet.reserved3  = 0;
    p->packet.id[0]      = id;
    p->packet.id[1]      = id >> 8;
    p->packet.reserved4  = 0;
    p->packet.reserved5  = 0;
    p->packet.size[0]    = size;
    p->packet.size[1]    = size >> 8;
    p->packet.size[2]    = size >> 16;
    p->packet.size[3]    = size >> 24;
    if ( size > 0 && data != NULL ) {
      memcpy(p->packet.data,data,size);
    }
    ok = 1;
  }

  return ok;
}


int
garmin_read ( garmin_unit * garmin, garmin_packet * p )
{
  int r = -1;

  garmin_open(garmin);

  if ( garmin->usb.handle != NULL ) {
    if ( garmin->usb.read_bulk == 0 ) {
      r = usb_interrupt_read(garmin->usb.handle,
			     garmin->usb.intr_in,
			     p->data,
			     sizeof(garmin_packet),
			     INTR_TIMEOUT);
      /* 
	 If the packet is a "Pid_Data_Available" packet, we need to read
	 from the bulk endpoint until we get an empty packet.
      */
      
      if ( garmin_packet_type(p) == GARMIN_PROTOCOL_USB &&
	   garmin_packet_id(p) == Pid_Data_Available ) {
	
	/* FIXME!!! */
	
	printf("Received a Pid_Data_Available from the unit!\n");
      }
      
    } else {
      r = usb_bulk_read(garmin->usb.handle,
			garmin->usb.bulk_in,
			p->data,
			sizeof(garmin_packet),
			BULK_TIMEOUT);
    }
  }

  if ( garmin->verbose != 0 && r >= 0 ) {
    garmin_print_packet(p,GARMIN_DIR_READ,stdout);
  }

  return r;
}


int
garmin_write ( garmin_unit * garmin, garmin_packet * p )
{
  int r = -1;
  int s = garmin_packet_size(p) + PACKET_HEADER_SIZE;

  garmin_open(garmin);

  if ( garmin->usb.handle != NULL ) {

    if ( garmin->verbose != 0 ) {
      garmin_print_packet(p,GARMIN_DIR_WRITE,stdout);
    }

    r = usb_bulk_write(garmin->usb.handle,
		       garmin->usb.bulk_out,
		       p->data,
		       s,
		       BULK_TIMEOUT);
    if ( r != s ) {
      printf("usb_bulk_write failed: %s\n",usb_strerror());
      exit(1);
    }
  }
  
  return r;
}


uint32
garmin_start_session ( garmin_unit * garmin )
{
  garmin_packet p;

  garmin_packetize(&p,Pid_Start_Session,0,NULL);
  p.packet.type = GARMIN_PROTOCOL_USB;

  garmin_write(garmin,&p);
  garmin_write(garmin,&p);
  garmin_write(garmin,&p);

  if ( garmin_read(garmin,&p) == 16 ) {
    garmin->id = get_uint32(p.packet.data);
  } else {
    garmin->id = 0;
  }
  
  return garmin->id;
}


void
garmin_print_packet ( garmin_packet * p, int dir, FILE * fp )
{
  int    i;
  int    j;
  uint32 s;
  char   hex[128];
  char   dec[128];

  s = garmin_packet_size(p);

  switch ( dir ) {
  case GARMIN_DIR_READ:   fprintf(fp,"<read");   break;
  case GARMIN_DIR_WRITE:  fprintf(fp,"<write");  break;
  default:                fprintf(fp,"<packet");        break;
  }

  fprintf(fp," type=\"0x%02x\" id=\"0x%04x\" size=\"%u\"",
	  garmin_packet_type(p),garmin_packet_id(p),s);
  if ( s > 0 ) {
    fprintf(fp,">\n");
    for ( i = 0, j = 0; i < s; i++ ) {
      sprintf(&hex[(3*(i&0x0f))]," %02x",p->packet.data[i]);
      sprintf(&dec[(i&0x0f)],"%c",
	      (isalnum(p->packet.data[i]) || 
	       ispunct(p->packet.data[i]) ||
	       p->packet.data[i] == ' ') ?
	      p->packet.data[i] : '_');
      if ( (i & 0x0f) == 0x0f ) {
	j = 0;
	fprintf(fp,"[%04x] %-54s %s\n",i-15,hex,dec);
      } else {
	j++;
      }
    }
    if ( j > 0 ) {
      fprintf(fp,"[%04x] %-54s %s\n",s-(s & 0x0f),hex,dec);
    }
    switch ( dir ) {
    case GARMIN_DIR_READ:   fprintf(fp,"</read>\n");   break;
    case GARMIN_DIR_WRITE:  fprintf(fp,"</write>\n");  break;
    default:                fprintf(fp,"</packet>\n"); break;
    }
  } else {
    fprintf(fp,"/>\n");
  }
}
