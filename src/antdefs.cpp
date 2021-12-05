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

#include "antdefs.hpp"

namespace antpm {

#define ANTP_PAIR(x) {x,#x}

MsgNames msgNames[]={
  ANTP_PAIR(MESG_EVENT_ID),
  ANTP_PAIR(MESG_ACKNOWLEDGED_DATA_ID),
  ANTP_PAIR(MESG_ASSIGN_CHANNEL_ID),
  ANTP_PAIR(MESG_BROADCAST_DATA_ID),
  ANTP_PAIR(MESG_BURST_DATA_ID),
  ANTP_PAIR(MESG_CAPABILITIES_ID),
  ANTP_PAIR(MESG_CHANNEL_ID_ID),
  ANTP_PAIR(MESG_CHANNEL_MESG_PERIOD_ID),
  ANTP_PAIR(MESG_CHANNEL_RADIO_FREQ_ID),
  ANTP_PAIR(MESG_CHANNEL_SEARCH_TIMEOUT_ID),
  ANTP_PAIR(MESG_CHANNEL_STATUS_ID),
  ANTP_PAIR(MESG_VERSION_ID),
  ANTP_PAIR(MESG_CLOSE_CHANNEL_ID),
  ANTP_PAIR(MESG_EXT_ACKNOWLEDGED_DATA_ID),
  ANTP_PAIR(MESG_EXT_BROADCAST_DATA_ID),
  ANTP_PAIR(MESG_EXT_BURST_DATA_ID),
  ANTP_PAIR(MESG_NETWORK_KEY_ID),
  ANTP_PAIR(MESG_OPEN_CHANNEL_ID),
  ANTP_PAIR(MESG_OPEN_RX_SCAN_ID),
  ANTP_PAIR(MESG_REQUEST_ID),
  ANTP_PAIR(MESG_RESPONSE_EVENT_ID),
  ANTP_PAIR(MESG_SEARCH_WAVEFORM_ID),
  ANTP_PAIR(MESG_SYSTEM_RESET_ID),
  ANTP_PAIR(MESG_UNASSIGN_CHANNEL_ID),
  ANTP_PAIR(MESG_GET_SERIAL_NUM_ID),
  ANTP_PAIR(MESG_STARTUP_MSG_ID),
  {-1,"UNKNOWN"}
};


ResponseNames responseNames[]={
  ANTP_PAIR(RESPONSE_NO_ERROR),
  ANTP_PAIR(EVENT_RX_SEARCH_TIMEOUT),
  ANTP_PAIR(EVENT_RX_FAIL),
  ANTP_PAIR(EVENT_TX),
  ANTP_PAIR(EVENT_TRANSFER_RX_FAILED),
  ANTP_PAIR(EVENT_TRANSFER_TX_COMPLETED),
  ANTP_PAIR(EVENT_TRANSFER_TX_FAILED),
  ANTP_PAIR(EVENT_CHANNEL_CLOSED),
  ANTP_PAIR(EVENT_RX_FAIL_GO_TO_SEARCH),
  ANTP_PAIR(EVENT_CHANNEL_COLLISION),
  ANTP_PAIR(EVENT_TRANSFER_TX_START),
  ANTP_PAIR(CHANNEL_IN_WRONG_STATE),
  ANTP_PAIR(CHANNEL_NOT_OPENED),
  ANTP_PAIR(CHANNEL_ID_NOT_SET),
  ANTP_PAIR(CLOSE_ALL_CHANNELS),
  ANTP_PAIR(TRANSFER_IN_PROGRESS),
  ANTP_PAIR(TRANSFER_SEQUENCE_NUMBER_ERROR),
  ANTP_PAIR(TRANSFER_IN_ERROR),
  {-1,"UNKNOWN"}
};

AntFSCommandNames antFSCommandNames[]={
  ANTP_PAIR(ANTFS_CmdLink),
  ANTP_PAIR(ANTFS_CmdDisconnect),
  ANTP_PAIR(ANTFS_CmdAuthenticate),
  ANTP_PAIR(ANTFS_CmdPing),
  ANTP_PAIR(ANTFS_ReqDownload),
  ANTP_PAIR(ANTFS_ReqUpload),
  ANTP_PAIR(ANTFS_ReqErase),
  ANTP_PAIR(ANTFS_UploadData),
  ANTP_PAIR(ANTFS_CmdDirect),
  {-1,"UNKNOWN"}
};

AntFSResponseNames  antFSResponseNames[]={
  ANTP_PAIR(ANTFS_RespAuthenticate),
  ANTP_PAIR(ANTFS_RespDownload),
  ANTP_PAIR(ANTFS_RespUpload),
  ANTP_PAIR(ANTFS_RespErase),
  ANTP_PAIR(ANTFS_RespUploadData),
  ANTP_PAIR(ANTFS_RespDirect),
  {-1,"UNKNOWN"}
};


#undef ANTP_PAIR



}

