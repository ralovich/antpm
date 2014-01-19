// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// copyright 2008-2009 paul@ant.sbrk.co.uk. released under GPLv3
// vers 0.6t
// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; coding: utf-8-unix -*-
// ***** BEGIN LICENSE BLOCK *****
//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2011-2014 RALOVICH, Kristóf                            //
//                                                                      //
// This program is free software; you can redistribute it and/or modify //
// it under the terms of the GNU General Public License as published by //
// the Free Software Foundation; either version 3 of the License, or    //
// (at your option) any later version.                                  //
//                                                                      //
// This program is distributed in the hope that it will be useful,      //
// but WITHOUT ANY WARRANTY; without even the implied warranty of       //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        //
// GNU General Public License for more details.                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
// ***** END LICENSE BLOCK *****

#pragma once

//#include <cstdint>
#include "stdintfwd.hpp"

namespace antpm {

typedef uint8_t uchar;
typedef uint16_t ushort;
typedef uint32_t uint;



//#define EVENT_RX_ACKNOWLEDGED   0x9b
//#define EVENT_RX_BROADCAST    0x9a
//#define EVENT_RX_BURST_PACKET   0x9c
//#define EVENT_RX_EXT_ACKNOWLEDGED 0x9e
//#define EVENT_RX_EXT_BROADCAST    0x9d
//#define EVENT_RX_EXT_BURST_PACKET 0x9f
//#define EVENT_RX_FAKE_BURST   0xdd





//#define MESG_INVALID_ID           0x00
#define MESG_EVENT_ID             0x01

#define MESG_VERSION_ID           0x3E

#define MESG_RESPONSE_EVENT_ID    0x40
#define MESG_UNASSIGN_CHANNEL_ID  0x41
#define MESG_ASSIGN_CHANNEL_ID    0x42
#define MESG_CHANNEL_MESG_PERIOD_ID 0x43
#define MESG_CHANNEL_SEARCH_TIMEOUT_ID  0x44
#define MESG_CHANNEL_RADIO_FREQ_ID  0x45
#define MESG_NETWORK_KEY_ID   0x46
#define MESG_SEARCH_WAVEFORM_ID   0x49
#define MESG_SYSTEM_RESET_ID    0x4a
#define MESG_OPEN_CHANNEL_ID    0x4b
#define MESG_CLOSE_CHANNEL_ID   0x4c
#define MESG_REQUEST_ID     0x4d
#define MESG_BROADCAST_DATA_ID    0x4e
#define MESG_ACKNOWLEDGED_DATA_ID 0x4f

#define MESG_BURST_DATA_ID    0x50
#define MESG_CAPABILITIES_ID    0x54
#define MESG_CHANNEL_ID_ID    0x51
#define MESG_CHANNEL_STATUS_ID    0x52
#define MESG_OPEN_RX_SCAN_ID    0x5b
#define MESG_EXT_BROADCAST_DATA_ID  0x5d
#define MESG_EXT_ACKNOWLEDGED_DATA_ID 0x5e
#define MESG_EXT_BURST_DATA_ID    0x5f

#define MESG_GET_SERIAL_NUM_ID    0x61

#define MESG_STARTUP_MSG_ID 0x6f


#define MESG_DATA_SIZE      30

#define MESG_RESPONSE_EVENT_SIZE  3

#define MESG_TX_SYNC      0xa4


#define RESPONSE_NO_ERROR   0x00
#define EVENT_RX_SEARCH_TIMEOUT  0x01
#define EVENT_RX_FAIL     0x02
#define EVENT_TX  0x03
#define EVENT_TRANSFER_RX_FAILED  0x04
#define EVENT_TRANSFER_TX_COMPLETED  0x05
#define EVENT_TRANSFER_TX_FAILED  0x06
#define EVENT_CHANNEL_CLOSED  7
#define EVENT_RX_FAIL_GO_TO_SEARCH  8
#define EVENT_CHANNEL_COLLISION    9
#define EVENT_TRANSFER_TX_START  10
#define CHANNEL_IN_WRONG_STATE    21
#define CHANNEL_NOT_OPENED    22
#define CHANNEL_ID_NOT_SET    24
#define CLOSE_ALL_CHANNELS  25
#define TRANSFER_IN_PROGRESS    31
#define TRANSFER_SEQUENCE_NUMBER_ERROR 32
#define TRANSFER_IN_ERROR  33

static const unsigned char ANTP_NETKEY_HR[8] = {0xB9,0xA5,0x21,0xFB,0xBD,0x72,0xC3,0x45};
static const unsigned char ANTP_NETKEY[8] = {0xA8,0xA4,0x23,0xB9,0xF5,0x5E,0x63,0xC1}; // ANT+Sport key



#define ANTFS_BeaconId          0x43
#define ANTFS_CommandResponseId 0x44

#define ANTFS_CmdLink 0x02
#define ANTFS_CmdDisconnect 0x03
#define ANTFS_CmdAuthenticate 0x04
#define ANTFS_CmdPing 0x05
#define ANTFS_ReqDownload 0x09
#define ANTFS_ReqUpload 0x0A
#define ANTFS_ReqErase 0x0B
#define ANTFS_UploadData 0x0C
#define ANTFS_CmdDirect 0x0D


#define ANTFS_RespAuthenticate 0x84
#define ANTFS_RespDownload 0x89
#define ANTFS_RespUpload 0x8A
#define ANTFS_RespErase 0x8B
#define ANTFS_RespUploadData 0x8C
#define ANTFS_RespDirect 0x8D



// $ date -u --date='@631065600'
#define GARMIN_EPOCH 631065600 // Sun Dec 31 00:00:00 UTC 1989
//#define GARMIN_EPOCH 627666624 // Tue Nov 21 15:50:24 UTC 1989


extern struct MsgNames { int i; const char* s; } msgNames[];
extern struct ResponseNames { int i; const char* s; } responseNames[];
extern struct AntFSCommandNames {int i; const char* s; } antFSCommandNames[];
extern struct AntFSResponseNames {int i; const char* s; } antFSResponseNames[];


// FIXME: message codes!





#define ENUMERATE1(id, val) ENUMERATE(id, val, #id)
#define ENUMERATE_LIST \
  ENUMERATE1(ST_ANTFS_0, 0) \
  ENUMERATE1(ST_ANTFS_NODATA, 999) \
  ENUMERATE1(ST_ANTFS_RESTART, 1000) \
  ENUMERATE1(ST_ANTFS_START0, 1001) \
  ENUMERATE1(ST_ANTFS_LINKING, 1005) \
  ENUMERATE1(ST_ANTFS_AUTH0_SN, 1012) \
  ENUMERATE1(ST_ANTFS_AUTH1_PASS, 1013) \
  ENUMERATE1(ST_ANTFS_AUTH1_PAIR, 1017) \
  ENUMERATE1(ST_ANTFS_DL_DIRECTORY, 1024) \
  ENUMERATE1(ST_ANTFS_DL_FILES, 1025) \
  ENUMERATE1(ST_ANTFS_DL_SINGLE_FILE, 1027) \
  ENUMERATE1(ST_ANTFS_GINTF_DL_CAPS, 1034) \
  ENUMERATE1(ST_ANTFS_ERASE_SINGLE_FILE, 500) \
  ENUMERATE1(ST_ANTFS_BAD, 1006) \
  ENUMERATE1(ST_ANTFS_LAST, 1007)

typedef enum StateFSWork {
  //enum {
#define ENUMERATE(id, val, name) id = val,
  ENUMERATE_LIST
#undef ENUMERATE
  //};
} StateFSWork;

//static const char* StateFSWorkNames {
//#define ENUMERATE(id, val, name) name,
//  ENUMERATE_LIST
//#undef ENUMERATE
//} StateFSWorkNames;

static inline
const char*
StateFSWork2Str(const int id)
{
  switch(id)
  {
    //case ST_ANTFS_0: return "ST_ANTFS_0";
#define ENUMERATE(id, val, str) case id: return str;
    ENUMERATE_LIST
#undef ENUMERATE
    default:
      return "???";
  }
}

//int
//string_to_enum(const char *in_str)
//{
//  if (0);
//#define ENUMERATE(id, val, str) else if (0 == strcmp(in_str, str)) return val;
//    ENUMERATE_LIST
//#undef ENUMERATE
//  return -1; /* Not found */
//}

#undef ENUMERATE_LIST
#undef ENUMERATE1



#define ENUMERATE1(id) ENUMERATE(id, #id)
#define ENUMERATE_LIST \
  ENUMERATE1(MD_DOWNLOAD_ALL) \
  ENUMERATE1(MD_DOWNLOAD_SINGLE_FILE) \
  ENUMERATE1(MD_DIRECTORY_LISTING) \
  ENUMERATE1(MD_ERASE_SINGLE_FILE) \
  ENUMERATE1(MD_ERASE_ALL_ACTIVITIES) \
  ENUMERATE1(MD_LAST)

typedef enum ModeOfOperation {
  //enum {
#define ENUMERATE(id, name) id,
  ENUMERATE_LIST
#undef ENUMERATE
  //};
} ModeOfOperation;

//static const char* StateFSWorkNames {
//#define ENUMERATE(id, val, name) name,
//  ENUMERATE_LIST
//#undef ENUMERATE
//} StateFSWorkNames;

static inline
const char*
ModeOfOperation2Str(const int id)
{
  switch(id)
  {
    //case ST_ANTFS_0: return "ST_ANTFS_0";
#define ENUMERATE(id, str) case id: return str;
    ENUMERATE_LIST
#undef ENUMERATE
    default:
      return "???";
  }
}

//int
//string_to_enum(const char *in_str)
//{
//  if (0);
//#define ENUMERATE(id, val, str) else if (0 == strcmp(in_str, str)) return val;
//    ENUMERATE_LIST
//#undef ENUMERATE
//  return -1; /* Not found */
//}

#undef ENUMERATE_LIST
#undef ENUMERATE1

}

