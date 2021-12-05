#ifndef __ANTLIB_H__
#define __ANTLIB_H__

#include "antdefs.h"
uchar ANT_ResetSystem(void);
uchar ANT_Cmd55(uchar chan);
uchar ANT_OpenRxScanMode(uchar chan);
uchar ANT_Initf(const char *devname);
uchar ANT_Init(uchar devno);
uchar ANT_RequestMessage(uchar chan, uchar mesg);
uchar ANT_SetNetworkKeya(uchar net, const uchar *key);
uchar ANT_SetNetworkKey(uchar net, const uchar *key);
uchar ANT_AssignChannel(uchar chan, uchar chtype, uchar net);
uchar ANT_UnAssignChannel(uchar chan);
uchar ANT_SetChannelId(uchar chan, ushort dev, uchar devtype, uchar manid);
uchar ANT_SetChannelRFFreq(uchar chan, uchar freq);
uchar ANT_SetChannelPeriod(uchar chan, ushort period);
uchar ANT_SetChannelSearchTimeout(uchar chan, uchar timeout);
uchar ANT_SetSearchWaveform(uchar chan, ushort waveform);
uchar ANT_SendAcknowledgedDataA(uchar chan, uchar *data); /* ascii version */
uchar ANT_SendAcknowledgedData(uchar chan, uchar *data);
ushort ANT_SendBurstTransferA(uchar chan, uchar *data, ushort numpkts);
ushort ANT_SendBurstTransfer(uchar chan, uchar *data, ushort numpkts);
uchar ANT_OpenChannel(uchar chan);
uchar ANT_CloseChannel(uchar chan);
void ANT_AssignResponseFunction(RESPONSE_FUNC rf, uchar* rbuf);
void ANT_AssignChannelEventFunction(uchar chan, CHANNEL_EVENT_FUNC rf, uchar* rbuf);
int ANT_fd();

#endif
