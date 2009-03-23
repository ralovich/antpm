// copyright 2008 paul@ant.sbrk.co.uk. released under GPLv3
// vers 0.4t
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

#include "antdefs.h"

double round(double);

int gottype;
int sentauth;
int gotwatchid;
int nopairing;
int nowriteauth;
int reset;
int dbg = 0;
int seenphase0 = 0;
int lastphase;
int sentack2;
int newfreq = 0;
int period = 0x1000; // garmin specific broadcast period
int donebind = 0;
int sentgetv;
char *fname = "garmin";

static char  ANTSPT_KEY[] = "A8A423B9F55E63C1"; // ANT+Sport key

static uchar ebuf[MESG_DATA_SIZE]; // response event data gets stored here
static uchar cbuf[MESG_DATA_SIZE]; // channel event data gets stored here

int passive;
int semipassive;
int verbose;

int downloadfinished = 0;
int downloadstarted = 0;
int sentid = 0;

uint mydev = 0;
uint peerdev;
uint myid;
uint devid;
ulong myauth1;
ulong myauth2;
char authdata[32];
uint pairing;
uint isa50;
uint isa405;
uint waitauth;
int nphase0;

//char *getversion =	"440dffff00000000fe00000000000000";
//char *getgpsver =		"440dffff0000000006000200ff000000";
char *acks[] = {
	"fe00000000000000", // get version - 255, 248, 253
	"0e02000000000000", // device short name (fr405a) - 525
//"1c00020000000000", // no data
	"0a0002000e000000", // unit id - 38
	"0a000200ad020000", // 4 byte something? 0x10270000 = 10000 dec - 1523
	"0a000200c6010000", // 3 x 4 ints? - 994
	"0a00020035020000", // guessing this is # trackpoints per run - 1066
	"0a00020097000000", // load of software versions - 247
	"0a000200c2010000", // download runs - 27 (#runs), 990?, 12?
	"0a00020075000000", // download laps - 27 (#laps), 149 laps, 12?
	"0a00020006000000", // download trackpoints - 1510/99(run marker), ..1510,12
	"0a000200ac020000",
	""
};
int sentcmd;

uchar clientid[3][8];

int authfd = -1;
char *authfile;
int outfd; // output file
char *fn = "default_output_file";
char *progname;

#define BSIZE 8*100
uchar *burstbuf = 0;
uchar *blast = 0;
int blsize = 0;
int bused = 0;
int lseq = -1;

char *
timestamp(void)
{
	struct timeval tv;
	static char time[50];
	struct tm *tmp;

	gettimeofday(&tv, 0);
	tmp = gmtime(&tv.tv_sec);

	sprintf(time, "%02d:%02d:%02d.%02d",
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec, tv.tv_usec/10000);
	return time;
}

uint
randno(void)
{
	uint r;

	int fd = open("/dev/urandom", O_RDONLY);
	if (fd > 0) {
		read(fd, &r, sizeof r);
		close(fd);
	}
	return r;
}

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
	ulong auth1;
	ulong auth2;
	ulong fill1;
	ulong fill2;
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
#define ACKSIZE 8 // above structure must be this size
#define AUTHSIZE 24 // ditto
#define PAIRSIZE 16

decode(ushort bloblen, ushort pkttype, ushort pktlen, int dsize, uchar *data)
{
	int i;
	int j;
	int hr;
	int cad;
	int u1, u2;
	int doff = 20;
	char model[256];
	char gpsver[256];
	char devname[256];
	float alt;
	float dist;
	uint tv;
	time_t ttv;
	char tbuf[100];
	struct tm *tmp;
	double lat, lon;
	ushort part;
	ushort ver;
	uint unitid;
	uint nruns;
	printf("decode %d %d %d %d\n", bloblen, pkttype, pktlen, dsize);
	switch (pkttype) {
	case 255:
		memset(model, 0, sizeof model);
		memcpy(model, data+doff+4, dsize-4);
		part=data[doff]+data[doff+1]*256;
		ver=data[doff+2]+data[doff+3]*256;
		printf("%d Part#: %d ver: %d Name: %s\n", pkttype,
			part, ver, model);
	break;
	case 248:
		memset(gpsver, 0, sizeof gpsver);
		memcpy(gpsver, data+doff, dsize);
		printf("%d GPSver: %s\n", pkttype,
			gpsver);
	break;
	case 253:
		printf("%d Unknown\n", pkttype);
		for (i = 0; i < pktlen; i += 3)
			printf("%d.%d.%d\n", data[doff+i], data[doff+i+1], data[doff+i+2]);
	break;
	case 525:
		memset(devname, 0, sizeof devname);
		memcpy(devname, data+doff, dsize);
		printf("%d Devname %s\n", pkttype, devname);
	break;
	case 12:
	case 990:
		printf("%d shorts?", pkttype);
		for (i = 0; i < pktlen; i += 4)
			printf(" %u", data[doff+i] + data[doff+i+1]*256);
		printf("\n");
	break;
	case 38:
		unitid = data[doff] + data[doff+1]*256 +
			data[doff+2]*256*256 + data[doff+3]*256*256*256;
		printf("%d unitid %u\n", pkttype, unitid);
	break;
	case 27:
		nruns = data[doff] + data[doff+1] * 256;
		printf("%d nruns %u\n", pkttype, nruns);
	break;
	case 1523:
	case 994:
	case 1066:
		printf("%d ints?", pkttype);
		for (i = 0; i < pktlen; i += 4)
			printf(" %u", data[doff+i] + data[doff+i+1]*256 +
			data[doff+i+2]*256*256 + data[doff+i+3]*256*256*256);
		printf("\n");
	break;
	case 1510:
		printf("%d waypoints", pkttype);
		for (i = 0; i < 4 && i < pktlen; i += 4)
			printf(" %u", data[doff+i] + data[doff+i+1]*256 +
			data[doff+i+2]*256*256 + data[doff+i+3]*256*256*256);
		printf("\n");
		for (i = 4; i < pktlen; i += 24) {
			lat = (data[doff+i] + data[doff+i+1]*256 +
				data[doff+i+2]*256*256 + data[doff+i+3]*256*256*256)*180.0/0x80000000;
			lon = (data[doff+i+4] + data[doff+i+5]*256 +
				data[doff+i+6]*256*256 + data[doff+i+7]*256*256*256)*180.0/0x80000000;
			hr = data[doff+i+20];
			cad = data[doff+i+21];
			u1 = data[doff+i+22];
			u2 = data[doff+i+23];
			tv = (data[doff+i+8] + data[doff+i+9]*256 +
				data[doff+i+10]*256*256 + data[doff+i+11]*256*256*256);
			ttv = tv+631065600; // garmin epoch offset
			tmp = gmtime(&ttv);
			strftime(tbuf, sizeof tbuf, "%Y-%m-%dT%H:%M:%SZ", tmp);  // format for printing
			memcpy((void *)&alt, data+doff+i+12, 4);
			memcpy((void *)&dist, data+doff+i+16, 4);
			if (dbg) printf("lat %.10g lon %.10g hr %d cad %d u1 %d u2 %d tv %d %s alt %f dist %f\n", lat, lon,
				hr, cad, u1, u2, tv, tbuf, alt, dist);
			printf("          <Trackpoint>\n");
			printf("            <Time>%s</Time>\n",tbuf);
			printf("            <Position>\n");
			printf("              <LatitudeDegrees>%d.%07d</LatitudeDegrees>\n",
				(int)lat, (int)(round(10000000*fabs(lat-(int)lat))));
			printf("              <LongitudeDegrees>%d.%07d</LongitudeDegrees>\n",
				(int)lon, (int)(round(10000000*fabs(lon-(int)lon))));
			printf("            </Position>\n");
			printf("            <AltitudeMeters>%.7f</AltitudeMeters>\n", alt+0.000000005);
			printf("            <DistanceMeters>%.7f</DistanceMeters>\n", dist+0.000000005);
			if (hr > 0) {
				printf("            <HeartRateBpm xsi:type=\"HeartRateInBeatsPerMinute_t\">\n");
				printf("              <Value>%d</Value>\n", hr);
				printf("            </HeartRateBpm>\n");
			}
			printf("            <SensorState>%s</SensorState>\n",
				u1 ? "Present" : "Absent");
			printf("            <Extensions>\n");
			if (cad != 255) {
				printf("              <TPX xmlns=\"http://www.garmin.com/xmlschemas/ActivityExtension/v2\" CadenceSensor=\"Footpod\">\n");
				printf("                <RunCadence>%d</RunCadence>\n", cad);
				printf("              </TPX>\n");
			} else
				printf("              <TPX xmlns=\"http://www.garmin.com/xmlschemas/ActivityExtension/v2\" CadenceSensor=\"Footpod\"/>\n");
			printf("            </Extensions>\n");
			printf("          </Trackpoint>\n");
		}
	break;
	default:
		printf("don't know how to decode packet type %d\n", pkttype);
		for (i = doff; i < dsize && i < doff+pktlen; i++)
			printf("%02x", data[i]);
		printf("\n");
		for (i = doff; i < dsize && i < doff+pktlen; i++)
			if (isprint(data[i]))
				printf("%c", data[i]);
			else
				printf(".");
		printf("\n");
	}
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s -a authfile\n"
		"[ -o outfile ]\n"
		"[ -d devno ]\n"
		"[ -i id ]\n"
		"[ -m mydev ]\n"
		"[ -p ]\n",
		progname
	);
	exit(1);
}

uchar
chevent(uchar chan, uchar event)
{
	uchar seq;
	uchar last;
	uchar status;
	uchar phase;
	uint newdata;
	struct ack_msg ack;
	struct auth_msg auth;
	struct pair_msg pair;
	uint id;
	int i;
	uint cid;
	if (dbg) printf("chevent %02x %02x\n", chan, event);

	if (event == EVENT_RX_BROADCAST) {
		status = cbuf[1] & 0xd7;
		newdata = cbuf[1] & 0x20;
		phase = cbuf[2];
	}
	cid = cbuf[4]+cbuf[5]*256+cbuf[6]*256*256+cbuf[7]*256*256*256;
	memcpy((void *)&id, cbuf+4, 4);

	if (dbg)
		fprintf(stderr, "cid %08x myid %08x\n", cid, myid);
	if (dbg && event != EVENT_RX_BURST_PACKET) {
		fprintf(stderr, "chan %d event %02x channel open: ", chan, event);
		for (i = 0; i < 8; i++)
			fprintf(stderr, "%02x", cbuf[i]);
		fprintf(stderr, "\n");
	}
	
	switch (event) {
	case EVENT_RX_BROADCAST:
		lastphase = phase; // store the last phase we see the watch broadcast
		if (dbg) printf("lastphase %d\n", lastphase);
		if (!pairing && !nopairing)
			pairing = cbuf[1] & 8;
		if (!gottype) {
			gottype = 1;
			isa50 = cbuf[1] & 4;
			isa405 = cbuf[1] & 1;
			if ((isa50 && isa405) || (!isa50 && !isa405)) {
				fprintf(stderr, "50 %d and 405 %d\n", isa50, isa405);
				exit(1);
			}
		}
		if (verbose) {
			switch (phase) {
			case 0:
				fprintf(stderr, "%s BC0 %02x %d %d %d PID %d %d %d %c%c\n",
					timestamp(),
					cbuf[0], cbuf[1] & 0xd7, cbuf[2], cbuf[3],
					cbuf[4]+cbuf[5]*256, cbuf[6], cbuf[7],
					(cbuf[1] & 0x20) ? 'N' : ' ', (cbuf[1] & 0x08) ? 'P' : ' '
				);
				break;
			case 1:
				fprintf(stderr, "%s BC1 %02x %d %d %d CID %08x %c%c\n",
					timestamp(),
					cbuf[0], cbuf[1] & 0xd7, cbuf[2], cbuf[3], cid,
					(cbuf[1] & 0x20) ? 'N' : ' ', (cbuf[1] & 0x08) ? 'P' : ' '
				);
				break;
				fprintf(stderr, "%s BCX %02x %d %d %d PID %d %d %d %c%c\n",
					timestamp(),
					cbuf[0], cbuf[1] & 0xd7, cbuf[2], cbuf[3],
					cbuf[4]+cbuf[5]*256, cbuf[6], cbuf[7],
					(cbuf[1] & 0x20) ? 'N' : ' ', (cbuf[1] & 0x08) ? 'P' : ' '
				);
			default:
				break;
			}
		}

		if (dbg)
			printf("watch status %02x stage %d id %08x\n", status, phase, id);

		if (!sentid) {
			sentid = 1;
			ANT_RequestMessage(chan, MESG_CHANNEL_ID_ID); /* request sender id */
		}

		// if we don't see a phase 0 message first, reset the watch
		if (reset || (phase != 0 && !seenphase0)) {
			fprintf(stderr, "resetting\n");
			ack.code = 0x44; ack.atype = 3; ack.c1 = 0x00; ack.c2 = 0x00; ack.id = 0;
			ANT_SendAcknowledgedData(chan, (void *)&ack); // tell garmin we're finished
			sleep(1);
			exit(1);
		}
		switch (phase) {
		case 0:
			seenphase0 = 1;
			nphase0++;
			if (nphase0 % 10 == 0)
				donebind = 0;
			if (newfreq) {
				// switch to new frequency
				ANT_SetChannelPeriod(chan, period);
				ANT_SetChannelSearchTimeout(chan, 3);
				ANT_SetChannelRFFreq(chan, newfreq);
				newfreq = 0;
			}
			// phase 0 seen after reset at end of download
			if (downloadfinished) {
				fprintf(stderr, "finished\n");
				exit(0);
			}
			// generate a random id if pairing and user didn't specify one
			if (pairing && !myid) {
				myid = randno();
				fprintf(stderr, "pairing, using id %08x\n", myid);
			}
			// need id codes from auth file if not pairing
			// TODO: handle multiple watches
			// BUG: myauth1 should be allowed to be 0
			if (!pairing && !myauth1) {
				int nr;
				printf("reading auth data from %s\n", authfile);
				authfd = open(authfile, O_RDONLY);
				if (authfd < 0) {
					perror(authfile);
					fprintf(stderr, "No auth data. Need to pair first\n");
					exit(1);
				}
				nr = read(authfd, authdata, 32);
				close(authfd);
				if (nr != 32 && nr != 24) {
					fprintf(stderr, "bad auth file len %d != 32 or 24\n", nr);
					exit(1);
				}
				// BUG: auth file not portable
				memcpy((void *)&myauth1, authdata+16, 4);
				memcpy((void *)&myauth2, authdata+20, 4);
				memcpy((void *)&mydev, authdata+12, 4);
				memcpy((void *)&myid, authdata+4, 4);
				if (dbg)
					fprintf(stderr, "dev %08x auth %08x %08x id %08x\n",
						mydev, myauth1, myauth2, myid);
			}
			// bind to watch
			if (!donebind && devid) {
				donebind = 1;
				if (isa405)
					newfreq = 0x32;
				ack.code = 0x44; ack.atype = 2; ack.c1 = isa50 ? 0x32 : newfreq; ack.c2 = 0x04;
				ack.id = myid;
				ANT_SendAcknowledgedData(chan, (void *)&ack); // bind
			} else {
				if (dbg) printf("donebind %d devid %x\n", donebind, devid);
			}
			break;
		case 1:
			if (dbg) printf("case 1 %x\n", peerdev);
			if (peerdev) {
				if (dbg) printf("case 1 peerdev\n");
				// if watch has sent id
				if (mydev != 0 && peerdev != mydev) {
					fprintf(stderr, "Don't know this device %08x != %08x\n", peerdev, mydev);
				} else if (!sentauth && !waitauth) {
				if (dbg) printf("case 1 diffdev\n");
					assert(sizeof auth == AUTHSIZE);
					auth.code = 0x44; auth.atype = 4; auth.phase = 3; auth.u1 = 8;
					auth.id = myid; auth.auth1 = myauth1; auth.auth2 = myauth2;
					auth.fill1 = auth.fill2 = 0;
					sentauth = 1;
					ANT_SendBurstTransfer(chan, (void *)&auth, (sizeof auth)/8); // send our auth data
				}
			}
			if (dbg) printf("case 1 cid %x myid %x\n", cid, myid);
			if (!sentack2 && cid == myid && !waitauth) {
				sentack2 = 1;
				if (dbg) printf("sending ack2\n");
				// if it did bind to me before someone else
				ack.code = 0x44; ack.atype = 4; ack.c1 = 0x01; ack.c2 = 0x00;
				ack.id = myid;
				ANT_SendAcknowledgedData(chan, (void *)&ack); // request id
			}
			break;
		case 2:
			// successfully authenticated
			if (!downloadstarted) {
				downloadstarted = 1;
				if (dbg) printf("starting download\n");
				ack.code = 0x44; ack.atype = 6; ack.c1 = 0x01; ack.c2 = 0x00; ack.id = 0;
				//ANT_SendAcknowledgedData(chan, (void *)&ack); // tell garmin to start upload
			}
			if (downloadfinished) {
				if (dbg) printf("finished download\n");
				ack.code = 0x44; ack.atype = 3; ack.c1 = 0x00; ack.c2 = 0x00; ack.id = 0;
				if (!passive) ANT_SendAcknowledgedData(chan, (void *)&ack); // tell garmin we're finished
			}
			break;
		case 3:
			if (pairing) {
				// waiting for the user to pair
				printf("Please press \"View\" on watch to confirm pairing\n");
				waitauth = 2; // next burst data is auth data
			} else {
				if (dbg) printf("not sure why in phase 3\n");
				if (!sentgetv) {
					sentgetv = 1;
					//ANT_SendBurstTransferA(chan, getversion, strlen(getversion)/16);
				}
			}
			break;
		default:
			if (dbg) fprintf(stderr, "Unknown phase %d\n", phase);
			break;
		}
		break;
	case EVENT_RX_BURST_PACKET:
		// now handled in coalesced burst below
		if (dbg) printf("burst\n");
		break;
	case EVENT_RX_FAKE_BURST:
		if (dbg) printf("rxfake burst pairing %d blast %d waitauth %d\n",
			pairing, blast, waitauth);
		blsize = *(int *)(cbuf+4);
		memcpy(&blast, cbuf+8, 4);
		if (dbg) {
			printf("fake burst %d %lx ", blsize, blast);
			for (i = 0; i < blsize && i < 64; i++)
				printf("%02x", blast[i]);
			printf("\n");
			for (i = 0; i < blsize; i++)
				if (isprint(blast[i]))
					printf("%c", blast[i]);
				else
					printf(".");
			printf("\n");
		}
		if (sentauth) {
			static int nacksent = 0;
			char *ackdata;
			static uchar ackpkt[100];
			// ack the last packet
			ushort bloblen = blast[14]+256*blast[15];
			ushort pkttype = blast[16]+256*blast[17];
			ushort pktlen = blast[18]+256*blast[19];
			if (bloblen == 0) {
				if (dbg) printf("bloblen %d, get next data\n", bloblen);
				// request next set of data
				ackdata = acks[nacksent++];
				if (!strcmp(ackdata, "")) { // finished
					printf("acks finished, resetting\n");
					ack.code = 0x44; ack.atype = 3; ack.c1 = 0x00;
					ack.c2 = 0x00; ack.id = 0;
					ANT_SendAcknowledgedData(chan, (void *)&ack); // go to idle
					sleep(1);
					exit(1);
				}
				if (dbg) printf("got type 0, sending ack %s\n", ackdata);
				sprintf(ackpkt, "440dffff00000000%s", ackdata);
			} else if (bloblen == 65535) {
				// repeat last ack
				if (dbg) printf("repeating ack %s\n", ackpkt);
				ANT_SendBurstTransferA(chan, ackpkt, strlen(ackpkt)/16);
			} else {
				if (dbg) printf("non-0 bloblen %d\n", bloblen);
				decode(bloblen, pkttype, pktlen, blsize, blast);
				sprintf(ackpkt, "440dffff0000000006000200%02x%02x0000", pkttype%256, pkttype/256);
			}
			if (dbg) printf("received pkttype %d len %d\n", pkttype, pktlen);
			if (dbg) printf("acking %s\n", ackpkt);
			ANT_SendBurstTransferA(chan, ackpkt, strlen(ackpkt)/16);
		} else if (!nopairing && pairing && blast) {
			memcpy(&peerdev, blast+12, 4);
			if (dbg)
				printf("watch id %08x waitauth %d\n", peerdev, waitauth);
			if (mydev != 0 && peerdev != mydev) {
				fprintf(stderr, "Don't know this device %08x != %08x\n", peerdev, mydev);
				exit(1);
			}
			if (waitauth == 2) {
				int nw;
				// should be receiving auth data
				if (nowriteauth) {
					printf("Not overwriting auth data\n");
					exit(1);
				}
				printf("storing auth data in %s\n", authfile);
				authfd = open(authfile, O_WRONLY|O_CREAT, 0644);
				if (authfd < 0) {
					perror(authfile);
					exit(1);
				}
				nw = write(authfd, blast, blsize);
				if (nw != blsize) {
					fprintf(stderr, "auth write failed fd %d %d\n", authfd, nw);
					perror("write");
					exit(1);
				}
				close(authfd);
				//exit(1);
				pairing = 0;
				waitauth = 0;
				reset = 1;
			}
			if (pairing && !waitauth) {
				//assert(sizeof pair == PAIRSIZE);
				pair.code = 0x44; pair.atype = 4; pair.phase = 2; pair.id = myid;
				bzero(pair.devname, sizeof pair.devname);
				//if (peerdev <= 9999999) // only allow 7 digits
					//sprintf(pair.devname, "%u", peerdev);
					sprintf(pair.devname, fname);
				//else
				//	fprintf(stderr, "pair dev name too large %08x \"%d\"\n", peerdev, peerdev);
				pair.u1 = strlen(pair.devname);
				printf("sending pair data for dev %s\n", pair.devname);
				waitauth = 1;
				if (isa405 && pairing) {
					// go straight to storing auth data
					waitauth = 2;
				}
				ANT_SendBurstTransfer(chan, (void *)&pair, (sizeof pair)/8) ; // send pair data
			} else {
				if (dbg) printf("not pairing\n");
			}
		} else if (!gotwatchid && (lastphase == 1)) {
			static int once = 0;
			gotwatchid = 1;
			// garmin sending authentication/identification data
			if (!once) {
				int i;
				once = 1;
				if (dbg)
					fprintf(stderr, "id data: ");
			}
			if (dbg)
				for (i = 0; i < blsize; i++)
					fprintf(stderr, "%02x", blast[i]);
			if (dbg)
					fprintf(stderr, "\n");
			memcpy(&peerdev, blast+12, 4);
			if (dbg)
				printf("watch id %08x\n", peerdev);
			if (mydev != 0 && peerdev != mydev) {
				fprintf(stderr, "Don't know this device %08x != %08x\n", peerdev, mydev);
				exit(1);
			}
		} else if (lastphase == 2) {
			int nw;
			static int once = 0;
			printf("once %d\n", once);
			// garmin uploading in response to sendack3
			// in this state we're receiving the workout data
			if (!once) {
				printf("receiving\n");
				once = 1;
				outfd = open(fn, O_WRONLY|O_CREAT, 0644);
				if (outfd < 0) {
					perror(fn);
					exit(1);
				}
			}
			if (last) {
				nw = write(outfd, blast, blsize);
				if (nw != blsize) {
					fprintf(stderr, "data write failed fd %d %d\n", outfd, nw);
					perror("write");
					exit(1);
				}
				close(outfd);
				downloadfinished = 1;
			}
		} else if (0 && last) {
			if (dbg) {
				fprintf(stderr, "auth response: ");
				for (i = 0; i < blsize; i++)
					fprintf(stderr, "%02x", cbuf[i]);
				fprintf(stderr, "\n");
			}
			if (blast[10] == 2) {
				fprintf(stderr, "authentication failed\n");
				exit(1);
			}
		} else if (last) {
			fprintf(stderr, "data in state xx: ");
			int i;
			for (i = 0; i < blsize; i++)
				fprintf(stderr, "%02x", blast[i]);
			fprintf(stderr, "\n");
			sentcmd = 1000;
			switch (sentcmd) {
			case 1000:
				break;
			case 0:
				sentcmd++;
				//ANT_SendBurstTransferA(chan, getgpsver, strlen(getgpsver)/16);
				break;
			case 999:
				printf("finished\n");
				exit(1);
			default:
				sleep(1);
				sentcmd = 1;
				//printf("sending command %d %s\n", sentcmd-1, cmds[sentcmd-1]);
				//ANT_SendBurstTransferA(chan, cmds[sentcmd-1],
				//	strlen(cmds[sentcmd-1])/16);
				sentcmd++;
				//if(!strcmp(cmds[sentcmd-1], "END"))
				//	sentcmd = 999;
				break;
			}
		}
		if (dbg) printf("continuing after burst\n");
		break;
	}
	return 1;
}

uchar
revent(uchar chan, uchar event)
{
	struct ack_msg ack;
	int i;

	if (dbg) printf("revent %02x %02x\n", chan, event);
	switch (event) {
	case EVENT_TRANSFER_TX_COMPLETED:
		if (dbg) printf("Transfer complete %02x\n", ebuf[1]);
		break;
	case INVALID_MESSAGE:
		printf("Invalid message %02x\n", ebuf[1]);
		break;
	case RESPONSE_NO_ERROR:
		switch (ebuf[1]) {
		case MESG_ASSIGN_CHANNEL_ID:
			ANT_AssignChannelEventFunction(chan, chevent, cbuf);
			break;
		case MESG_OPEN_CHANNEL_ID:
			printf("channel open, waiting for broadcast\n");
			break;
		default:
			if (dbg) printf("Message %02x NO_ERROR\n", ebuf[1]);
			break;
		}
		break;
	case MESG_CHANNEL_ID_ID:
		devid = ebuf[1]+ebuf[2]*256;
		if (mydev == 0 || devid == mydev%65536) {
			if (dbg)
				printf("devid %08x myid %08x\n", devid, myid);
		} else {
			printf("Ignoring unknown device %08x, mydev %08x\n", devid, mydev);
			devid = sentid = 0; // reset
		}
		break;
	case MESG_NETWORK_KEY_ID:
	case MESG_SEARCH_WAVEFORM_ID:
	case MESG_OPEN_CHANNEL_ID:
		printf("response event %02x code %02x\n", event, ebuf[2]);
		for (i = 0; i < 8; i++)
			fprintf(stderr, "%02x", ebuf[i]);
		fprintf(stderr, "\n");
		break;
	case MESG_CAPABILITIES_ID:
		if (dbg)
			printf("capabilities chans %d nets %d opt %02x adv %02x\n",
				ebuf[0], ebuf[1], ebuf[2], ebuf[3]);
		break;
	case MESG_CHANNEL_STATUS_ID:
		if (dbg)
			printf("channel status %d\n", ebuf[1]);
		break;
	case EVENT_RX_FAIL:
		// ignore this
		break;
	default:
		printf("Unhandled response event %02x\n", event);
		break;
	}
	return 1;
}

main(int ac, char *av[])
{
	int devnum = 0;
	int chan = 0;
	int net = 0;
	int chtype = 0; // wildcard
	int devno = 0; // wildcard
	int devtype = 0; // wildcard
	int manid = 0; // wildcard
	int freq = 0x32; // garmin specific radio frequency
	int srchto = 255; // max timeout
	int waveform = 0x0053; // aids search somehow
	int c;
	extern char *optarg;
	extern int optind, opterr, optopt;

	// default auth file //
	if (getenv("HOME")) {
		authfile = malloc(strlen(getenv("HOME"))+strlen("/.gant")+1);
		if (authfile)
			sprintf(authfile, "%s/.gant", getenv("HOME"));
	}
	progname = av[0];
	while ((c = getopt(ac, av, "a:o:d:i:m:PpvDrnzf:")) != -1) {
		switch(c) {
			case 'a':
				authfile = optarg;
				break;
			case 'f':
				fname = optarg;
				break;
			case 'o':
				fn = optarg;
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
				semipassive = 0;
				break;
			case 'P':
				passive = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'D':
				dbg = 1;
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
			default:
				fprintf(stderr, "unknown option %s\n", optarg);
				usage();
		}
	}

	ac -= optind;
	av += optind;

	if ((!passive && !authfile) || ac)
		usage();
		
	if (!ANT_Init(devnum, 0)) { // should be 115200 but doesn't fit into a short
		fprintf(stderr, "open dev %d failed\n", devnum);
		exit(1);
	}
	ANT_ResetSystem();
	ANT_AssignResponseFunction(revent, ebuf);
	ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID); //informative
	ANT_SetNetworkKeya(net, ANTSPT_KEY);
	ANT_AssignChannel(chan, chtype, net);
	ANT_SetChannelId(chan, devno, devtype, manid);
	ANT_RequestMessage(chan, MESG_CAPABILITIES_ID); //informative
	ANT_SetChannelRFFreq(chan, freq);
	ANT_SetChannelPeriod(chan, period);
	ANT_SetChannelSearchTimeout(chan, srchto);
	ANT_SetSearchWaveform(chan, waveform);
	ANT_OpenChannel(chan);
	ANT_RequestMessage(chan, MESG_CHANNEL_STATUS_ID); //informative

	// everything handled in event functions
	for(;;)
		sleep(10);
}
