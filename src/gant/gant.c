/* copyright 2010 Klaus@Ethgen.de. released under GPLv3 */
/* copyright 2008-2009 paul@ant.sbrk.co.uk */
/* copyright 2009-2009 Wali */
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <ctype.h>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include "antlib.h"
#include "antdefs.h"

#define DEBUG_OUT(level, ...) {if (dbg && level <= dbg){fprintf(stderr, "DEBUG(%d): ", level); fprintf(stderr, __VA_ARGS__); fprintf(stderr, " in line %d.\n", __LINE__);}}
#define ERROR_OUT(...) {fprintf(stderr, "ERROR: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, " in line %d.\n", __LINE__);}
#define XML_ERROR_CHECK {if (rc < 0){ERROR_OUT("Error in XML creation"); exit(-1);}}

typedef enum {
   XML_OUTSIDE,
   XML_IN_TrainingCenterDatabase,
   XML_IN_Activities,
   XML_IN_Activity,
   XML_IN_Lap,
   XML_IN_Track,
   XML_IN_Trackpoint
} xml_pos;

struct _activity {
   unsigned short valid;
   unsigned short first_lap;
   unsigned short last_lap;
   unsigned short sport;
};

struct _lap {
   unsigned short valid;
   time_t timestamp;
   unsigned long duration;
   float distance;
   float max_speed;
   unsigned short calories;
   int hr_avg;
   int hr_max;
   int intensity;
   int cadence;
   int trigger;
};

/* all version numbering according ant agent for windows 2.2.1 */
char *releasetime = "Apr 25 2010, 10:00:00";
uint majorrelease = 2;
uint minorrelease = 2;
uint majorbuild = 7;
uint minorbuild = 0;

double round(double);

int gottype;
int sentauth;
int gotwatchid;
int nopairing = 0;
int nowriteauth = 0;
int reset = 0;
int dbg = 0;
int seenphase0 = 0;
int lastphase;
int sentack2;
int newfreq = 0;
int period = 0x1000;		/* garmin specific broadcast period */
int donebind = 0;
int sentgetv;
char *fname = "garmin";
time_t garmin_epoch = 631065600;

static const uchar ANTSPT_KEY[] = "A8A423B9F55E63C1";	// ANT+Sport key

static uchar ebuf[MESG_DATA_SIZE];	// response event data gets stored here
static uchar cbuf[MESG_DATA_SIZE];	// channel event data gets stored here

int passive = 0;
int verbose = 0;

int downloadfinished = 0;
int downloadstarted = 0;
int sentid = 0;

uint mydev = 0;
uint peerdev;
uint myid;
uint devid;
uint myauth1;
uint myauth2;
char authdata[32];
uint pairing;
uint isa50;
uint isa405;
uint waitauth;
int nphase0;
char modelname[256];
char devname[256];
ushort part = 0;
ushort ver = 0;
uint unitid = 0;

//char *getversion = "440dffff00000000fe00000000000000";
//char *getgpsver = "440dffff0000000006000200ff000000";
const char *acks[] = {
   "fe00000000000000",		// get version - 255, 248, 253
   "0e02000000000000",		// device short name (fr405a) - 525
// "1c00020000000000", // no data
   "0a0002000e000000",		// unit id - 38
   "0a00020002000000",		// send position
   "0a00020005000000",		// send time
   "0a000200ad020000",		// 4 byte something? 0x10270000 = 10000 dec - 1523
   "0a000200c6010000",		// 3 x 4 ints? - 994
   "0a00020035020000",		// guessing this is # trackpoints per run - 1066
   "0a00020097000000",		// load of software versions - 247
   "0a000200c2010000",		// download runs - 27 (#runs), 990?, 12?
   "0a00020075000000",		// download laps - 27 (#laps), 149 laps, 12?
   "0a00020006000000",		// download trackpoints - 1510/99(run marker), ..1510,12
   "0a000200ac020000",
   ""
};

int sentcmd;

uchar clientid[3][8];

int authfd = -1;
char *authfile;
char *progname;

#define BSIZE 8*10000
//uchar blast[BSIZE]; // should be like that, but not working
uchar *blast = 0;		// first time reading not initialized, but why it is working reading from adr 0?

int blsize = 0;
int bused = 0;
int lseq = -1;

/* round a float as garmin does it! */
/* shoot me for writing this! */
char *ground(double d)
{
   int neg = 0;
   static char res[30];
   ulong ival;
   ulong l;			/* hope it doesn't overflow */

   if (d < 0)
   {
      neg = 1;
      d = -d;
   }
   ival = floor(d);
   d -= ival;
   l = floor(d * 100000000);
   if (l % 10 >= 5)
      l = l / 10 + 1;
   else
      l = l / 10;
   snprintf(res, 30, "%s%ld.%07ld", neg ? "-" : "", ival, l);
   return res;
}

char *timestamp(void)
{
   struct timeval tv;
   static char time[50];
   struct tm *tmp;

   gettimeofday(&tv, 0);
   tmp = gmtime(&tv.tv_sec);

   snprintf(time, 50, "%02d:%02d:%02d.%02d", tmp->tm_hour, tmp->tm_min, tmp->tm_sec, (int)tv.tv_usec / 10000);
   return time;
}

uint randno(void)
{
   uint r;
   int fd = open("/dev/urandom", O_RDONLY);

   if (fd > 0)
   {
      read(fd, &r, sizeof r);
      close(fd);
   }
   return r;
}

void print_tcx_header(xmlTextWriterPtr tcxfile)
{
   int rc;

   /* Start the Document */
   rc = xmlTextWriterStartDocument(tcxfile, "1.0", "UTF-8", "no"); XML_ERROR_CHECK;

   /* Start the TrainingCenterDatabase */
   rc = xmlTextWriterStartElementNS(tcxfile, NULL, BAD_CAST "TrainingCenterDatabase", BAD_CAST "http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2"); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteAttributeNS(tcxfile, BAD_CAST "xsi", BAD_CAST "schemaLocation", BAD_CAST "http://www.w3.org/2001/XMLSchema-instance", BAD_CAST "http://www.garmin.com/xmlschemas/ActivityExtension/v2 http://www.garmin.com/xmlschemas/ActivityExtensionv2.xsd http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2 http://www.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd"); XML_ERROR_CHECK;

   /* Finally start the Activities element */
   rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Activities"); XML_ERROR_CHECK;

   return;
}

void print_tcx_footer(xmlTextWriterPtr tcxfile)
{
   int rc;

   /* *INDENT-OFF* */
   rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;		/* Track (Is it a good idea to do it here?) */
   rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;		/* Lap (Is it a good idea to do it here?) */
   rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Creator"); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "xsi:type", BAD_CAST "Device_t"); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "Name", "%s", devname); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "UnitId", "%u", unitid); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "ProductID", "%u", part); XML_ERROR_CHECK;
   rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Version"); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "VersionMajor", "%u", ver / 100); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "VersionMinor", "%u", ver % 100); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "BuildMajor", BAD_CAST "0"); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "BuildMinor", BAD_CAST "0"); XML_ERROR_CHECK;
   rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;		/* Version */
   rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;		/* Creator */
   rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;		/* Activity */
   rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;		/* Activities */
   rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Author"); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "xsi:type", BAD_CAST "Application_t"); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "Name", BAD_CAST "Garmin ANT Agent(tm)"); XML_ERROR_CHECK;
   rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Build"); XML_ERROR_CHECK;
   rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Version"); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "VersionMajor", "%u", majorrelease); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "VersionMinor", "%u", minorrelease); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "BuildMajor", "%u", majorbuild); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "BuildMinor", "%u", minorbuild); XML_ERROR_CHECK;
   rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;		/* Version */
   rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "Type", BAD_CAST "Release"); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "Time", "%s", releasetime); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "Builder", BAD_CAST "sqa"); XML_ERROR_CHECK;
   rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;		/* Build */
   rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "LangID", BAD_CAST "EN"); XML_ERROR_CHECK;
   rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "PartNumber", BAD_CAST "006-A0214-00"); XML_ERROR_CHECK;
   /* *INDENT-ON* */
   return;
}

void dump_data(FILE * out, uchar *data, size_t offset, size_t size)
{
   unsigned char buf[16];
   int i, j;
   for (i=0; i < (int)size; i+=16)
   {
      memset(buf, 0, sizeof buf);
      memcpy(buf, data + offset + i, (size-i >= 16)? 16 : (size-i));
      fprintf(out, "0x%04lx:", (unsigned long)(offset + i));

      for (j=0; j < 16; j++)
      {
	 if (j == 8)
	    fprintf(out, " ");

   if (i+j >= (int)size)
	    fprintf(out, "   ");
	 else
	    fprintf(out, " %02x", buf[j]);
      } // for (j=0; j < 16; j++)

      fprintf(out, " ");
      for (j=0; j < 16; j++)
      {
        if (i+j >= (int)size)
	    break;

	 if (j == 8)
	    fprintf(out, " ");

	 if (isprint(buf[j]))
	    fprintf(out, "%c", buf[j]);
	 else
	    fprintf(out, ".");
      } // for (j=0; j < 16; j++)

      fprintf(out, "\n");
   } // for (i=0; i < size; i+=16)

   return;
} // void dump_ata(void *data, siz...


#pragma pack(1)
struct ack_msg {
   uchar code;
   uchar atype;
   uchar c1;
   uchar c2;
   uint id;
};

struct auth_msg {
   uchar code;
   uchar atype;
   uchar phase;
   uchar u1;
   uint id;
   uint auth1;
   uint auth2;
   uint fill1;
   uint fill2;
};

struct pair_msg {
   uchar code;
   uchar atype;
   uchar phase;
   uchar u1;
   uint id;
   char devname[16];
};

#pragma pack()
#define ACKSIZE 8		// above structure must be this size
#define AUTHSIZE 24		// ditto
#define PAIRSIZE 16
#define MAXLAPS 256		// max of saving laps data before output with trackpoint data
#define MAXTRACK 256*256	// max number of tracks to be saved per download

void decode(ushort bloblen, ushort pkttype, ushort pktlen, int dsize, uchar * data)
{
   int i;
   int hr;
   int cad;
   int u1, u2;
   int doff = 20;
   char model[256];
   char gpsver[256];
   float alt;
   float dist;
   time_t tv;
   char tbuf[100];
   double lat, lon;
   uint ndata;
   time_t tv_lap;
   static xmlTextWriterPtr tcxfile = NULL;
   static ushort track_pause = 0;
   int rc;
   static xml_pos xml_position = XML_OUTSIDE;
   static struct _activity *activities = NULL;
   static struct _lap *laps = NULL;
   static unsigned short lap_id;
   static unsigned short activity_id;

   DEBUG_OUT(3, "decode %d %d %d %d", bloblen, pkttype, pktlen, dsize);
   switch (pkttype)
   {
      case 255:		// Identifier
	 memset(model, 0, sizeof model);
	 memcpy(model, data + doff + 4, pktlen - 4);
	 part = data[doff] + data[doff + 1] * 256;
	 ver = data[doff + 2] + data[doff + 3] * 256;
	 DEBUG_OUT(1, "Packet %d: Part#: %d Version: %d Name: %s", pkttype, part, ver, model);
	 break;
      case 248:		// GPS identifier
	 memset(gpsver, 0, sizeof gpsver);
	 memcpy(gpsver, data + doff, pktlen);
	 DEBUG_OUT(1, "Packet %d: GPS: %s", pkttype, gpsver);
	 break;
      case 253:
	 DEBUG_OUT(1, "Packet %d: Unknown", pkttype);
	 if (dbg >= 2)
	    dump_data(stderr, data+doff, 0, pktlen);
	 break;
      case 525:
	 memset(devname, 0, sizeof devname);
	 memcpy(devname, data + doff, pktlen);
	 DEBUG_OUT(1, "Packet %d: Devname: \"%s\"", pkttype, devname);
	 break;
      case 12:
	 DEBUG_OUT(1, "Packet %d: xfer complete (subtype %u)", pkttype, data[doff] + data[doff + 1] * 256);
	 if (dbg >= 2)
	    dump_data(stderr, data+doff, 2, pktlen-2);
	 switch (data[doff] + data[doff + 1] * 256)
	 {
	    case 6:		// Wayboint end
	       // last file completed, add footer and close file
	       if (tcxfile)
	       {
		  print_tcx_footer(tcxfile);
		  xml_position = XML_OUTSIDE;
		  rc = xmlTextWriterEndDocument(tcxfile); XML_ERROR_CHECK;
		  xmlFreeTextWriter(tcxfile);
		  tcxfile = NULL;
	       }
	       break;
	    case 117:		// Lap informations end
	       break;
	    case 450:		// Track informations end
	       break;
	    default:
	       break;
	 }
	 break;
      case 38:
	 unitid = data[doff] + data[doff + 1] * 256 + data[doff + 2] * 256 * 256 + data[doff + 3] * 256 * 256 * 256;
	 DEBUG_OUT(1, "Packet %d: UnitID: %u", pkttype, unitid);
	 break;
      case 27:			// Number of following packages
	 ndata = data[doff] + data[doff + 1] * 256;
	 DEBUG_OUT(2, "Packet %d: Number of following packages: %u", pkttype, ndata);
	 break;
      case 1523:		// Max Trackpoints (?) 10000
      case 994:		// Drei Limits (?) 200, 25, 200
      case 1066:		// 20, 200, 100, ????
	 DEBUG_OUT(1, "Packet %d", pkttype);
	 if (dbg >= 2)
	    dump_data(stderr, data+doff, 0, pktlen);
	 break;
      case 14:			// Current time (UTC)
	 DEBUG_OUT(1, "Packet %d: Current time: %04u-%02u-%02u %02u:%02u:%02u", pkttype, data[doff + 2] + data[doff + 3] * 256, data[doff + 1], data[doff], data[doff + 4], data[doff + 6], data[doff + 7]);
	 break;
      case 17:
	 DEBUG_OUT(1, "Packet %d: Position?", pkttype);
	 if (dbg >= 2)
	    dump_data(stderr, data+doff, 0, pktlen);
	 break;
      case 990:		// Activity specification
	 DEBUG_OUT(1, "Packet %d: Activity %u: laps %u-%u sport %u", pkttype, data[doff] + data[doff + 1] * 256, data[doff + 2] + data[doff + 3] * 256, data[doff + 4] + data[doff + 5] * 256, data[doff + 6]);

	 // Initialize memory if not done before
	 if (activities == NULL)
	 {
	    activities = (struct _activity *)calloc(256 * 256, sizeof(struct _activity));
	    if (activities == NULL)
	    {
	       ERROR_OUT("Cannot calloc enough memory");
	       exit(-1);
	    }
	    memset(activities, 0x00, sizeof(struct _activity) * 256 * 256);
	 }			// if (activities == NULL)

	 activity_id = data[doff] + data[doff + 1] * 256;
	 activities[activity_id].first_lap = data[doff + 2] + data[doff + 3] * 256;
	 activities[activity_id].last_lap = data[doff + 4] + data[doff + 5] * 256;
	 activities[activity_id].sport = data[doff + 6];
	 activities[activity_id].valid = 1;

	 // leftover (Most of!)
	 if (dbg >= 2)
	    dump_data(stderr, data+doff, 7, pktlen-7);
	 break;
      case 99:
	 DEBUG_OUT(1, "Packet %d: Activity index: %u", pkttype, data[doff] + data[doff + 1] * 256);

	 // close previous file
	 if (tcxfile)
	 {
	    // add xml footer and close file, the next file will be open further down
	    DEBUG_OUT(1, "Closing file");
	    print_tcx_footer(tcxfile);
	    xml_position = XML_OUTSIDE;
	    rc = xmlTextWriterEndDocument(tcxfile); XML_ERROR_CHECK;
	    xmlFreeTextWriter(tcxfile);
	    tcxfile = NULL;
	 }

	 activity_id = data[doff] + data[doff + 1] * 256;

	 if (laps == NULL || activities[activity_id].valid == 0 || laps[activities[activity_id].first_lap].valid == 0)
	 {
	    ERROR_OUT("Something is wrong in transfer");
	    exit(-1);
	 }

	 lap_id = activities[activity_id].first_lap;

	 // use first lap starttime as filename
	 tv_lap = laps[lap_id].timestamp;
	 strftime(tbuf, sizeof(tbuf), "%Y-%m-%d-%H%M%S.tcx", localtime(&tv_lap));
	 DEBUG_OUT(1, "Open file %s", tbuf);
	 // open file and start with header of xml file
	 tcxfile = xmlNewTextWriterFilename(tbuf, 0);
	 if (tcxfile == NULL)
	 {
	    ERROR_OUT("Error in XML creation");
	    exit(-1);
	 }
	 print_tcx_header(tcxfile);
	 xml_position = XML_IN_Activities;

	 rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Activity"); XML_ERROR_CHECK;
	 xml_position++;
	 switch (activities[activity_id].sport)
	 {
	    case 0:
	       rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "Sport", BAD_CAST "Running");
	       break;
	    case 1:
	       rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "Sport", BAD_CAST "Biking");
	       break;
	    case 2:
	       rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "Sport", BAD_CAST "Other");
	       break;
	    default:
	       rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "Notes", BAD_CAST "Unknown sport type"); XML_ERROR_CHECK;
	       rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "Sport", BAD_CAST "Other");
	 }
	 XML_ERROR_CHECK;

	 strftime(tbuf, sizeof tbuf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&tv_lap));
	 rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "Id", "%s", tbuf); XML_ERROR_CHECK;

	 // leftover (5)
	 if (dbg >= 2)
	    dump_data(stderr, data+doff, 2, pktlen-2);
	 break;
      case 1510:
	 DEBUG_OUT(1, "%d Track package with %u waypoints", pkttype, data[doff] + data[doff + 1] * 256);

	 // if trackpoints are split into more than one message 1510, do not add xml head again

	 // Do some sanity checks
	 if (activities[activity_id].valid == 0 || laps[lap_id].valid == 0 || lap_id < activities[activity_id].first_lap || lap_id > activities[activity_id].last_lap)
	 {
	    ERROR_OUT("Sanity check failed");
	    ERROR_OUT("Activity: %u, lap: %u", activity_id, lap_id);
	    exit(-1);
	 }			// if (activities[activity_id].va...

	 for (i = 4; i < pktlen; i += 24)
	 {
	    tv = data[doff + i + 8] + data[doff + i + 9] * 256 + data[doff + i + 10] * 256 * 256 + data[doff + i + 11] * 256 * 256 * 256 + garmin_epoch;

	    if (lap_id < activities[activity_id].last_lap && laps[lap_id + 1].valid && tv >= laps[lap_id + 1].timestamp)
	    {
	       lap_id++;
	       // Close track and lap if still open (Should ever be)
	       if (xml_position == XML_IN_Track)
	       {
		  rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;	/* Track */
		  xml_position--;
		  rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;	/* Lap */
		  xml_position--;
	       }		// if (xml_position == XML_IN_Tra...
	    }			// if (lap_id < activities[activi...

	    // New lap, handle here to only write the code once
	    if (xml_position == XML_IN_Activity)
	    {
	       strftime(tbuf, sizeof tbuf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&laps[lap_id].timestamp));
	       rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Lap"); XML_ERROR_CHECK;
	       xml_position++;
	       rc = xmlTextWriterWriteFormatAttribute(tcxfile, BAD_CAST "StartTime", "%s", tbuf); XML_ERROR_CHECK;
	       rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "TotalTimeSeconds", "%ld.%02ld00000", laps[lap_id].duration / 100, laps[lap_id].duration % 100); XML_ERROR_CHECK;
	       rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "DistanceMeters", "%s", ground(laps[lap_id].distance)); XML_ERROR_CHECK;
	       rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "MaximumSpeed", "%s", ground(laps[lap_id].max_speed)); XML_ERROR_CHECK;
	       rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "Calories", "%d", laps[lap_id].calories); XML_ERROR_CHECK;

	       if (laps[lap_id].hr_avg > 0)
	       {
		  rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "AverageHeartRateBpm"); XML_ERROR_CHECK;
		  rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "xsi:type", BAD_CAST "HeartRateInBeatsPerMinute_t"); XML_ERROR_CHECK;
		  rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "Value", "%d", laps[lap_id].hr_avg); XML_ERROR_CHECK;
		  rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;
	       }
	       if (laps[lap_id].hr_max > 0)
	       {
		  rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "MaximumHeartRateBpm"); XML_ERROR_CHECK;
		  rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "xsi:type", BAD_CAST "HeartRateInBeatsPerMinute_t"); XML_ERROR_CHECK;
		  rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "Value", "%d", laps[lap_id].hr_max); XML_ERROR_CHECK;
		  rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;
	       }

	       switch (laps[lap_id].intensity)
	       {
		  case 0:
		     rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "Intensity", BAD_CAST "Active");
		     break;
		  case 1:
		     rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "Intensity", BAD_CAST "Resting");
		     break;
		  default:
		     rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "Intensity", BAD_CAST "Active");
	       }
	       XML_ERROR_CHECK;

	       // for bike the average cadence of this lap is here
	       if (activities[activity_id].sport == 1)
	       {
		  if (laps[lap_id].cadence != 255)
		  {
		     rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "Cadence", "%d", laps[lap_id].cadence); XML_ERROR_CHECK;
		  }
	       }

	       switch (laps[lap_id].trigger)
	       {
		  case 4:
		     rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "TriggerMethod", BAD_CAST "HeartRate");
		     break;
		  case 3:
		     rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "TriggerMethod", BAD_CAST "Time");
		     break;
		  case 2:
		     rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "TriggerMethod", BAD_CAST "Location");
		     break;
		  case 1:
		     rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "TriggerMethod", BAD_CAST "Distance");
		     break;
		  case 0:
		     rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "TriggerMethod", BAD_CAST "Manual");
		     break;
		  default:
		     rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "TriggerMethod", BAD_CAST "Manual");
	       }
	       XML_ERROR_CHECK;

	       // I prefere the average run cadence here than at the end of this lap according windows ANTagent
	       if (activities[activity_id].sport == 0)
	       {
		  if (laps[lap_id].cadence != 255)
		  {
		     rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Extensions"); XML_ERROR_CHECK;
		     rc = xmlTextWriterStartElementNS(tcxfile, NULL, BAD_CAST "LX", BAD_CAST "http://www.garmin.com/xmlschemas/ActivityExtension/v2"); XML_ERROR_CHECK;
		     rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "AvgRunCadence", "%d", laps[lap_id].cadence); XML_ERROR_CHECK;
		     rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;	/* LX */
		     rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;	/* Extensions */
		  }
	       }
	       rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Track"); XML_ERROR_CHECK;
	       xml_position++;

	       track_pause = 0;
	    }			// if (xml_position == XML_IN_Act...

	    if (xml_position != XML_IN_Track)
	    {
	       ERROR_OUT("Need to write trackpoint but I am not in active track (Current: %d)", xml_position);
	       continue;
	    }

	    strftime(tbuf, sizeof tbuf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&tv));
	    memcpy((void *)&alt, data + doff + i + 12, 4);
	    memcpy((void *)&dist, data + doff + i + 16, 4);
	    lat = (data[doff + i] + data[doff + i + 1] * 256 + data[doff + i + 2] * 256 * 256 + data[doff + i + 3] * 256 * 256 * 256) * 180.0 / 0x80000000;
	    lon = (data[doff + i + 4] + data[doff + i + 5] * 256 + data[doff + i + 6] * 256 * 256 + data[doff + i + 7] * 256 * 256 * 256) * 180.0 / 0x80000000;
	    hr = data[doff + i + 20];
	    cad = data[doff + i + 21];
	    u1 = data[doff + i + 22];
	    u2 = data[doff + i + 23];
	    DEBUG_OUT(2, "lat %.10g lon %.10g hr %d cad %d u1 %d u2 %d tv %d %s alt %f dist %f %02x %02x%02x%02x%02x", lat, lon, hr, cad, u1, u2, (int)tv, tbuf, alt, dist, data[doff + i + 3], data[doff + i + 16], data[doff + i + 17], data[doff + i + 18], data[doff + i + 19]);
	    // track pause only if following trackpoint is aswell 'timemarker' with utopic distance
	    if (track_pause && dist > (float)40000000)
	    {
	       rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;	/* Track */
	       rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Track"); XML_ERROR_CHECK;
	    }

	    rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Trackpoint"); XML_ERROR_CHECK;
	    xml_position++;
	    rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "Time", "%s", tbuf); XML_ERROR_CHECK;

	    if (lat < 90)
	    {
	       rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Position"); XML_ERROR_CHECK;
	       rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "LatitudeDegrees", "%s", ground(lat)); XML_ERROR_CHECK;
	       rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "LongitudeDegrees", "%s", ground(lon)); XML_ERROR_CHECK;
	       rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;	/* Position */
	       rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "AltitudeMeters", "%s", ground(alt)); XML_ERROR_CHECK;
	    }

	    // last trackpoint has utopic distance, 40000km should be enough, hack?
	    if (dist < (float)40000000)
	    {
	       rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "DistanceMeters", "%s", ground(dist)); XML_ERROR_CHECK;
	    }

	    if (hr > 0)
	    {
	       rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "HeartRateBpm"); XML_ERROR_CHECK;
	       rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "xsi:type", BAD_CAST "HeartRateInBeatsPerMinute_t"); XML_ERROR_CHECK;
	       rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "Value", "%d", hr); XML_ERROR_CHECK;
	       rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;	/* HeartRateBpm */
	    }

	    // for bikes the cadence is written here and for the footpod in <Extensions>, why garmin?
	    if (activities[activity_id].sport == 1)
	    {
	       if (cad != 255)
	       {
		  rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "Cadence", "%d", cad); XML_ERROR_CHECK;
	       }
	    }

	    if (dist < (float)40000000)
	    {
	       rc = xmlTextWriterWriteElement(tcxfile, BAD_CAST "SensorState", u1 ? BAD_CAST "Present" : BAD_CAST "Absent"); XML_ERROR_CHECK;
	       if (u1 == 1 || cad != 255)
	       {
		  rc = xmlTextWriterStartElement(tcxfile, BAD_CAST "Extensions"); XML_ERROR_CHECK;
		  rc = xmlTextWriterStartElementNS(tcxfile, NULL, BAD_CAST "TPX", BAD_CAST "http://www.garmin.com/xmlschemas/ActivityExtension/v2"); XML_ERROR_CHECK;
		  rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "AvgRunCadence", "%d", cad); XML_ERROR_CHECK;
		  // get type of pod from data, could not figure it out, so using sporttyp of first track
		  if (activities[activity_id].sport == 1)
		  {
		     rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "CadenceSensor", BAD_CAST "Bike"); XML_ERROR_CHECK;
		  }
		  else
		  {
		     rc = xmlTextWriterWriteAttribute(tcxfile, BAD_CAST "CadenceSensor", BAD_CAST "Footpod"); XML_ERROR_CHECK;
		     if (cad != 255)
		     {
			rc = xmlTextWriterWriteFormatElement(tcxfile, BAD_CAST "RunCadence", "%d", cad); XML_ERROR_CHECK;
		     }
		  }
		  rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;	/* TPX */
		  rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;	/* Extensions */
	       }
	       track_pause = 0;
	    }

	    rc = xmlTextWriterEndElement(tcxfile); XML_ERROR_CHECK;	/* Trackpoint */
	    xml_position--;

	    // maybe if we recieve utopic position and distance this tells pause in the run (stop and go) if not begin or end of lap
	    if (dist > (float)40000000 && track_pause == 0)
	    {
	       track_pause = 1;
	       DEBUG_OUT(2, "Track pause (stop and go)");
	    }
	    else
	    {
	       track_pause = 0;
	    }
	 }			// for (i = 4; i < pktlen; i += 2...
	 break;
      case 149:		// Lap specification
	 DEBUG_OUT(1, "%d Lap data id: %u %u", pkttype, data[doff] + data[doff + 1] * 256, data[doff + 2] + data[doff + 3] * 256);

	 // Initialize memory if not done before
	 if (laps == NULL)
	 {
	    laps = (struct _lap *)calloc(256 * 256, sizeof(struct _lap));
	    if (laps == NULL)
	    {
	       ERROR_OUT("Cannot calloc enough memory");
	       exit(-1);
	    }
	    memset(laps, 0x00, sizeof(struct _lap) * 256 * 256);
	 }			// if (laps == NULL)

	 lap_id = data[doff] + data[doff + 1] * 256;
	 laps[lap_id].timestamp = data[doff + 4] + data[doff + 5] * 256 + data[doff + 6] * 256 * 256 + data[doff + 7] * 256 * 256 * 256 + garmin_epoch;
	 laps[lap_id].duration = data[doff + 8] + data[doff + 9] * 256 + data[doff + 10] * 256 * 256 + data[doff + 11] * 256 * 256 * 256;
	 memcpy(&laps[lap_id].distance, &data[doff + 12], 4);	// Dirty, but seems to work
	 memcpy(&laps[lap_id].max_speed, &data[doff + 16], 4);	// Dirty, but seems to work
	 laps[lap_id].calories = data[doff + 36] + data[doff + 37] * 256;
	 laps[lap_id].hr_avg = data[doff + 38];
	 laps[lap_id].hr_max = data[doff + 39];
	 laps[lap_id].intensity = data[doff + 40];
	 laps[lap_id].cadence = data[doff + 41];
	 laps[lap_id].trigger = data[doff + 42];

	 laps[lap_id].valid = 1;

	 if (dbg >= 2)
	 {
	    dump_data(stderr, data+doff, 20, 16);
	    dump_data(stderr, data+doff, 43, pktlen-43);
	 }

	 break;
      case 247:		// Software versions
	 memset(modelname, 0, sizeof modelname);
	 memcpy(modelname, data + doff + 88, dsize - 88);
	 DEBUG_OUT(1, "%d Device name \"%s\"\n", pkttype, modelname);
	 break;
      default:
	 DEBUG_OUT(1, "Don't know how to decode packet type %d", pkttype);
	 if (dbg >= 2)
	    dump_data(stderr, data+doff, 0, pktlen);
   }
}

void usage(void)
{
   /* *INDENT-OFF* */
   fprintf(stderr, "Usage: %s -a authfile\n"
	 "       [ -a authfile ] Authfile (default ~/.gant)\n"
	 "       [ -f name ] (default garmin)\n"
	 "       [ -d devno ] Device no. (default 0)\n"
	 "       [ -i id ] ID for pairing (default random)\n"
	 "       [ -m mydev ] (default 0)\n"
	 "       [ -p ] Passive\n"
	 "       [ -v ] Verbose\n"
	 "       [ -D level ] Debug\n"
	 "       [ -r ] Reset the device\n"
	 "       [ -n ] Do not write auth file\n"
	 "       [ -z ] Do not pair\n"
	 "       [ -h ] This help\n",
	 progname);
   /* *INDENT-ON* */
   exit(1);
}

uchar chevent(uchar chan, uchar event)
{
   uchar status;
   uchar phase;
   /*uint newdata;*/
   struct ack_msg ack;
   struct auth_msg auth;
   struct pair_msg pair;
   uint id;
   int i;
   uint cid;

   DEBUG_OUT(5, "chevent %02x %02x", chan, event);
   if (event == EVENT_RX_BROADCAST)
   {
      status = cbuf[1] & 0xd7;
      /*newdata = cbuf[1] & 0x20;*/
      phase = cbuf[2];
   }
   cid = cbuf[4] + cbuf[5] * 256 + cbuf[6] * 256 * 256 + cbuf[7] * 256 * 256 * 256;
   memcpy((void *)&id, cbuf + 4, 4);

   DEBUG_OUT(6, "cid %08x myid %08x", cid, myid);
   if (dbg && event != EVENT_RX_BURST_PACKET)
   {
      DEBUG_OUT(5, "chan %d event %02x channel open: ", chan, event);
      for (i = 0; i < 8; i++)
	 DEBUG_OUT(6, " -: %02x", cbuf[i]);
   }

   switch (event)
   {
      case EVENT_RX_BROADCAST:
	 lastphase = phase;	// store the last phase we see the watch broadcast
	 DEBUG_OUT(3, "Lastphase %d", lastphase);
	 if (!pairing && !nopairing)
	    pairing = cbuf[1] & 8;
	 if (!gottype)
	 {
	    gottype = 1;
	    isa50 = cbuf[1] & 4;
	    isa405 = cbuf[1] & 1;
	    if ((isa50 && isa405) || (!isa50 && !isa405))
	    {
	       ERROR_OUT("50 %d and 405 %d", isa50, isa405);
	       exit(1);
	    }
	 }
	 if (verbose)
	 {
	    switch (phase)
	    {
	       case 0:
		  DEBUG_OUT(4, "%s BC0 %02x %d %d %d PID %d %d %d %c%c", timestamp(), cbuf[0], cbuf[1] & 0xd7, cbuf[2], cbuf[3], cbuf[4] + cbuf[5] * 256, cbuf[6], cbuf[7], (cbuf[1] & 0x20) ? 'N' : ' ', (cbuf[1] & 0x08) ? 'P' : ' ');
		  break;
	       case 1:
		  DEBUG_OUT(4, "%s BC1 %02x %d %d %d CID %08x %c%c", timestamp(), cbuf[0], cbuf[1] & 0xd7, cbuf[2], cbuf[3], cid, (cbuf[1] & 0x20) ? 'N' : ' ', (cbuf[1] & 0x08) ? 'P' : ' ');
		  break;
		  DEBUG_OUT(4, "%s BCX %02x %d %d %d PID %d %d %d %c%c", timestamp(), cbuf[0], cbuf[1] & 0xd7, cbuf[2], cbuf[3], cbuf[4] + cbuf[5] * 256, cbuf[6], cbuf[7], (cbuf[1] & 0x20) ? 'N' : ' ', (cbuf[1] & 0x08) ? 'P' : ' ');
	       default:
		  break;
	    }
	 }

	 DEBUG_OUT(6, "Watch status %02x stage %d id %08x", status, phase, id);
	 if (!sentid)
	 {
	    sentid = 1;
	    ANT_RequestMessage(chan, MESG_CHANNEL_ID_ID);	/* request sender id */
	 }

	 // if we don't see a phase 0 message first, reset the watch
	 if (reset || (phase != 0 && !seenphase0))
	 {
	    DEBUG_OUT(1, "Resetting");
	    ack.code = 0x44;
	    ack.atype = 3;
	    ack.c1 = 0x00;
	    ack.c2 = 0x00;
	    ack.id = 0;
	    ANT_SendAcknowledgedData(chan, (uchar *)&ack);	// tell garmin we're finished
	    sleep(1);
	    exit(1);
	 }
	 switch (phase)
	 {
	    case 0:
	       seenphase0 = 1;
	       nphase0++;
	       if (nphase0 % 10 == 0)
		  donebind = 0;
	       if (newfreq)
	       {
		  // switch to new frequency
		  ANT_SetChannelPeriod(chan, period);
		  ANT_SetChannelSearchTimeout(chan, 3);
		  ANT_SetChannelRFFreq(chan, newfreq);
		  newfreq = 0;
	       }
	       // phase 0 seen after reset at end of download
	       if (downloadfinished)
	       {
		  DEBUG_OUT(1, "Download finished");
		  exit(0);
	       }
	       // generate a random id if pairing and user didn't specify one
	       if (pairing && !myid)
	       {
		  myid = randno();
		  DEBUG_OUT(1, "Pairing, using id %08x", myid);
	       }
	       // need id codes from auth file if not pairing
	       // TODO: handle multiple watches
	       // BUG: myauth1 should be allowed to be 0
	       if (!pairing && !myauth1)
	       {
		  int nr;

		  DEBUG_OUT(1, "Reading auth data from %s", authfile);
		  authfd = open(authfile, O_RDONLY);
		  if (authfd < 0)
		  {
		     perror(authfile);
		     ERROR_OUT("No auth data. Need to pair first");
		     exit(1);
		  }
		  nr = read(authfd, authdata, 32);
		  close(authfd);
		  if (nr != 32 && nr != 24)
		  {
		     ERROR_OUT("Bad auth file len %d != 32 or 24", nr);
		     exit(1);
		  }
		  // BUG: auth file not portable
		  memcpy((void *)&myauth1, authdata + 16, 4);
		  memcpy((void *)&myauth2, authdata + 20, 4);
		  memcpy((void *)&mydev, authdata + 12, 4);
		  memcpy((void *)&myid, authdata + 4, 4);
		  DEBUG_OUT(4, "dev %08x auth %08x %08x id %08x", mydev, myauth1, myauth2, myid);
	       }
	       // bind to watch
	       if (!donebind && devid)
	       {
		  donebind = 1;
		  if (isa405)
		     newfreq = 0x32;
		  ack.code = 0x44;
		  ack.atype = 2;
		  ack.c1 = isa50 ? 0x32 : newfreq;
		  ack.c2 = 0x04;
		  ack.id = myid;
		  ANT_SendAcknowledgedData(chan, (void *)&ack);	// bind
	       }
	       else
	       {
		  DEBUG_OUT(6, "Donebind %d devid %x", donebind, devid);
	       }
	       break;
	    case 1:
	       DEBUG_OUT(3, "Case 1 %x", peerdev);
	       if (peerdev)
	       {
		  DEBUG_OUT(4, " -: peerdev");
		  // if watch has sent id
		  if (mydev != 0 && peerdev != mydev)
		  {
		     DEBUG_OUT(1, "Don't know this device %08x != %08x", peerdev, mydev);
		  }
		  else if (!sentauth && !waitauth)
		  {
		     DEBUG_OUT(4, " -: diffdev");
		     assert(sizeof auth == AUTHSIZE);
		     auth.code = 0x44;
		     auth.atype = 4;
		     auth.phase = 3;
		     auth.u1 = 8;
		     auth.id = myid;
		     auth.auth1 = myauth1;
		     auth.auth2 = myauth2;
		     auth.fill1 = auth.fill2 = 0;
		     sentauth = 1;
		     ANT_SendBurstTransfer(chan, (void *)&auth, (sizeof auth) / 8);	// send our auth data
		  }
	       }
	       DEBUG_OUT(4, " -: cid %x myid %x", cid, myid);
	       if (!sentack2 && cid == myid && !waitauth)
	       {
		  sentack2 = 1;
		  DEBUG_OUT(4, " -: sending ack2");
		  // if it did bind to me before someone else
		  ack.code = 0x44;
		  ack.atype = 4;
		  ack.c1 = 0x01;
		  ack.c2 = 0x00;
		  ack.id = myid;
		  ANT_SendAcknowledgedData(chan, (void *)&ack);	// request id
	       }
	       break;
	    case 2:
	       // successfully authenticated
	       if (!downloadstarted)
	       {
		  downloadstarted = 1;
		  DEBUG_OUT(1, "Starting download");
		  ack.code = 0x44;
		  ack.atype = 6;
		  ack.c1 = 0x01;
		  ack.c2 = 0x00;
		  ack.id = 0;
		  //ANT_SendAcknowledgedData(chan, (void *)&ack); // tell garmin to start upload
	       }
	       if (downloadfinished)
	       {
		  DEBUG_OUT(1, "Finished download");
		  ack.code = 0x44;
		  ack.atype = 3;
		  ack.c1 = 0x00;
		  ack.c2 = 0x00;
		  ack.id = 0;
		  if (!passive)
		     ANT_SendAcknowledgedData(chan, (void *)&ack);	// tell garmin we're finished
	       }
	       break;
	    case 3:
	       if (pairing)
	       {
		  // waiting for the user to pair
		  printf("Please press \"View\" on watch to confirm pairing\n");
		  waitauth = 2;	// next burst data is auth data
	       }
	       else
	       {
		  DEBUG_OUT(1, "Not sure why in phase 3");
		  if (!sentgetv)
		  {
		     sentgetv = 1;
		     //ANT_SendBurstTransferA(chan, getversion, strlen(getversion)/16);
		  }
	       }
	       break;
	    default:
	       DEBUG_OUT(1, "Unknown phase %d", phase);
	       break;
	 }
	 break;
      case EVENT_RX_BURST_PACKET:
	 // now handled in coalesced burst below
	 DEBUG_OUT(5, "Burst");
	 break;
      case EVENT_RX_FAKE_BURST:
	 DEBUG_OUT(5, "rxfake burst pairing %d blast %ld waitauth %d", pairing, (long)blast, waitauth);
	 blsize = *(int *)(cbuf + 4);
	 memcpy(&blast, cbuf + 8, 4);
	 if (dbg)
	 {
	    DEBUG_OUT(6, "Fake burst %d %lx", blsize, (long)blast);
	    for (i = 0; i < blsize && i < 64; i++)
	       DEBUG_OUT(4, " -: %02x", blast[i]);
	    for (i = 0; i < blsize; i++)
	       if (isprint(blast[i]))
	       {
		  DEBUG_OUT(6, " --: %c", blast[i]);
	       }
	       else
		  DEBUG_OUT(6, " --: .");
	 }
	 if (sentauth)
	 {
	    static int nacksent = 0;
	    static char ackpkt[100];

	    // ack the last packet
	    ushort bloblen = blast[14] + 256 * blast[15];

	    ushort pkttype = blast[16] + 256 * blast[17];

	    ushort pktlen = blast[18] + 256 * blast[19];

	    if (bloblen == 0)
	    {
	       // request next set of data
	       const char* ackdata = acks[nacksent++];
	       DEBUG_OUT(2, "bloblen %d, get next data", bloblen);
	       if (!strcmp(ackdata, ""))
	       {		// finished
		  DEBUG_OUT(2, "ACKs finished, resetting");
		  ack.code = 0x44;
		  ack.atype = 3;
		  ack.c1 = 0x00;
		  ack.c2 = 0x00;
		  ack.id = 0;
		  ANT_SendAcknowledgedData(chan, (void *)&ack);	// go to idle
		  sleep(1);
		  exit(1);
	       }
	       DEBUG_OUT(2, "Got type 0, sending ACK %s", ackdata);
	       snprintf(ackpkt, 100, "440dffff00000000%s", ackdata);
	    }
	    else if (bloblen == 65535)
	    {
	       // repeat last ack
	       DEBUG_OUT(2, "Repeating ACK %s", ackpkt);
	       ANT_SendBurstTransferA(chan, (uchar *) ackpkt, strlen(ackpkt) / 16);
	    }
	    else
	    {
	       DEBUG_OUT(2, "Non-0 bloblen %d", bloblen);
	       decode(bloblen, pkttype, pktlen, blsize, blast);
	       snprintf(ackpkt, 100, "440dffff0000000006000200%02x%02x0000", pkttype % 256, pkttype / 256);
	    }
	    DEBUG_OUT(1, "Received pkttype %d len %d", pkttype, pktlen);
	    DEBUG_OUT(2, "Acking %s", ackpkt);
	    ANT_SendBurstTransferA(chan, (uchar *) ackpkt, strlen(ackpkt) / 16);
	 }
	 else if (!nopairing && pairing && blast)
	 {
	    memcpy(&peerdev, blast + 12, 4);
	    DEBUG_OUT(1, "Watch id %08x waitauth %d", peerdev, waitauth);
	    if (mydev != 0 && peerdev != mydev)
	    {
	       ERROR_OUT("Don't know this device %08x != %08x", peerdev, mydev);
	       exit(1);
	    }
	    if (waitauth == 2)
	    {
	       int nw;

	       // should be receiving auth data
	       if (nowriteauth)
	       {
		  ERROR_OUT("Not overwriting auth data");
		  exit(1);
	       }
	       DEBUG_OUT(1, "Storing auth data in %s", authfile);
	       authfd = open(authfile, O_WRONLY | O_CREAT, 0644);
	       if (authfd < 0)
	       {
		  perror(authfile);
		  exit(1);
	       }
	       nw = write(authfd, blast, blsize);
	       if (nw != blsize)
	       {
		  ERROR_OUT("Auth write failed fd %d %d", authfd, nw);
		  perror("write");
		  exit(1);
	       }
	       close(authfd);
	       //exit(1);
	       pairing = 0;
	       waitauth = 0;
	       reset = 1;
	    }
	    if (pairing && !waitauth)
	    {
	       //assert(sizeof pair == PAIRSIZE);
	       pair.code = 0x44;
	       pair.atype = 4;
	       pair.phase = 2;
	       pair.id = myid;
	       bzero(pair.devname, sizeof pair.devname);
	       //if (peerdev <= 9999999) // only allow 7 digits
	       //sprintf(pair.devname, "%u", peerdev);
	       strcpy(pair.devname, fname);
	       //else
	       //       DEBUG_OUT(1, "Pair dev name too large %08x \"%d\"\n", peerdev, peerdev)
	       pair.u1 = strlen(pair.devname);
	       DEBUG_OUT(1, "Sending pair data for dev %s", pair.devname);
	       waitauth = 1;
	       if (isa405 && pairing)
	       {
		  // go straight to storing auth data
		  waitauth = 2;
	       }
	       ANT_SendBurstTransfer(chan, (void *)&pair, (sizeof pair) / 8);	// send pair data
	    }
	    else
	    {
	       DEBUG_OUT(1, "Not pairing");
	    }
	 }
	 else if (!gotwatchid && (lastphase == 1))
	 {
	    static int once = 0;

	    gotwatchid = 1;
	    // garmin sending authentication/identification data
	    if (!once)
	    {
	       once = 1;
	       DEBUG_OUT(3, "ID data:");
	    }
	    if (dbg)
	       for (i = 0; i < blsize; i++)
		  DEBUG_OUT(4, " -: %02x", blast[i]);
	    memcpy(&peerdev, blast + 12, 4);
	    DEBUG_OUT(1, "watch id %08x", peerdev);
	    if (mydev != 0 && peerdev != mydev)
	    {
	       ERROR_OUT("Don't know this device %08x != %08x", peerdev, mydev);
	       exit(1);
	    }
	 }
	 else if (lastphase == 2)
	 {
	    static int once = 0;

	    DEBUG_OUT(2, "Once %d", once)
	       // garmin uploading in response to sendack3
	       // in this state we're receiving the workout data
	       if (!once)
	    {
	       DEBUG_OUT(2, "Receiving");
	       once = 1;
	    }
	 }
	 DEBUG_OUT(1, "Continuing after burst");
	 break;
   }
   return 1;
}

uchar revent(uchar chan, uchar event)
{
   int i;

   DEBUG_OUT(5, "Revent: %02x %02x", chan, event);
   switch (event)
   {
      case EVENT_TRANSFER_TX_COMPLETED:
	 DEBUG_OUT(4, "Transfer complete: %02x", ebuf[1]);
	 break;
      case INVALID_MESSAGE:
	 DEBUG_OUT(4, "Invalid message: %02x", ebuf[1]);
	 break;
      case RESPONSE_NO_ERROR:
	 switch (ebuf[1])
	 {
	    case MESG_ASSIGN_CHANNEL_ID:
	       ANT_AssignChannelEventFunction(chan, chevent, cbuf);
	       break;
	    case MESG_OPEN_CHANNEL_ID:
	       DEBUG_OUT(1, "Channel open, waiting for broadcast");
	       break;
	    default:
	       DEBUG_OUT(4, "Message: %02x NO_ERROR", ebuf[1]);
	       break;
	 }
	 break;
      case MESG_CHANNEL_ID_ID:
	 devid = ebuf[1] + ebuf[2] * 256;
	 if (mydev == 0 || devid == mydev % 65536)
	 {
	    DEBUG_OUT(4, "DevID %08x myid %08x", devid, myid);
	 }
	 else
	 {
	    DEBUG_OUT(3, "Ignoring unknown device %08x, mydev %08x", devid, mydev);
	    devid = sentid = 0;	// reset
	 }
	 break;
      case MESG_NETWORK_KEY_ID:
      case MESG_SEARCH_WAVEFORM_ID:
      case MESG_OPEN_CHANNEL_ID:
	 DEBUG_OUT(4, "Response event %02x code %02x", event, ebuf[2]);
	 for (i = 0; i < 8; i++)
	    DEBUG_OUT(4, " -: %02x", ebuf[i]);
	 break;
      case MESG_CAPABILITIES_ID:
	 DEBUG_OUT(4, "Capabilities chans %d nets %d opt %02x adv %02x", ebuf[0], ebuf[1], ebuf[2], ebuf[3]);
	 break;
      case MESG_CHANNEL_STATUS_ID:
	 DEBUG_OUT(4, "Channel status %d", ebuf[1]);
	 break;
      case EVENT_RX_FAIL:
      case EVENT_TRANSFER_TX_FAILED:
      case EVENT_TRANSFER_RX_FAILED:
	 // ignore this
	 break;
      case EVENT_RX_SEARCH_TIMEOUT:
	 printf("Timeout, please make sure the device is not in standby.\n");
	 break;
      default:
	 DEBUG_OUT(3, "Unhandled response event %02x", event);
	 break;
   }
   return 1;
}

int main(int ac, char *av[])
{
   int devnum = 0;
   int chan = 0;
   int net = 0;
   int chtype = 0;		// wildcard
   int devno = 0;		// wildcard
   int devtype = 0;		// wildcard
   int manid = 0;		// wildcard
   int freq = 0x32;		// garmin specific radio frequency
   int srchto = 255;		// max timeout
   int waveform = 0x0053;	// aids search somehow
   int c;
   extern char *optarg;
   extern int optind;

   // default auth file //
   if (getenv("HOME"))
   {
      authfile = malloc(strlen(getenv("HOME")) + strlen("/.gant") + 1);
      if (authfile)
	 sprintf(authfile, "%s/.gant", getenv("HOME"));
   }
   progname = av[0];
   while ((c = getopt(ac, av, "a:f:d:i:m:pvD:rnzh")) != -1)
   {
      switch (c)
      {
	 case 'a':
	    authfile = optarg;
	    break;
	 case 'f':
	    fname = optarg;
	    break;
	 case 'd':
	    devnum = atoi(optarg);
	    break;
	 case 'i':
	    myid = atoi(optarg);
	    break;
	 case 'm':
	    mydev = atoi(optarg);
	    break;
	 case 'p':
	    passive = 1;
	    break;
	 case 'v':
	    verbose = 1;
	    break;
	 case 'D':
	    dbg = atoi(optarg);
	    break;
	 case 'r':
	    reset = 1;
	    break;
	 case 'n':
	    nowriteauth = 1;
	    break;
	 case 'z':
	    nopairing = 1;
	    break;
	 case 'h':
	    usage();
	 default:
	    ERROR_OUT("unknown option %s", optarg);
	    usage();
      }
   }

   ac -= optind;
   av += optind;

   if ((!passive && !authfile) || ac)
      usage();

   if (!ANT_Init(devnum))	// should be 115200 but doesn't fit into a short
   {
      ERROR_OUT("Open /dev/ttyUSB%d failed", devnum);
      exit(1);
   }
   ANT_ResetSystem();
   ANT_AssignResponseFunction(revent, ebuf);
   ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID);	//informative
   ANT_SetNetworkKeya(net, ANTSPT_KEY);
   ANT_AssignChannel(chan, chtype, net);
   // Wali: changed order of the following seq. according windows
   ANT_SetChannelPeriod(chan, period);
   ANT_SetChannelSearchTimeout(chan, srchto);
   ANT_RequestMessage(chan, MESG_CAPABILITIES_ID);	//informative
   ANT_SetChannelRFFreq(chan, freq);
   ANT_SetSearchWaveform(chan, waveform);
   ANT_SetChannelId(chan, devno, devtype, manid);
   ANT_OpenChannel(chan);
   ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID);	//informative

   // everything handled in event functions
   for (;;)
      sleep(10);
}

/* vim: set shiftwidth=3 softtabstop=3: */
