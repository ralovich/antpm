// copyright 2008 paul@ant.sbrk.co.uk. released under GPLv3
// vers 0.4t
typedef unsigned char uchar;
typedef uchar (*RESPONSE_FUNC)(uchar chan, uchar msgid);
typedef uchar (*CHANNEL_EVENT_FUNC)(uchar chan, uchar event);

#define EVENT_RX_ACKNOWLEDGED		0x9b
#define EVENT_RX_BROADCAST		0x9a
#define EVENT_RX_BURST_PACKET		0x9c
#define EVENT_RX_EXT_ACKNOWLEDGED	0x9e
#define EVENT_RX_EXT_BROADCAST		0x9d
#define EVENT_RX_EXT_BURST_PACKET	0x9f
#define EVENT_RX_FAKE_BURST		0xdd
#define EVENT_TRANSFER_TX_COMPLETED	0x05
#define INVALID_MESSAGE			0x28
#define MESG_ACKNOWLEDGED_DATA_ID	0x4f
#define MESG_ASSIGN_CHANNEL_ID		0x42
#define MESG_BROADCAST_DATA_ID		0x4e
#define MESG_BURST_DATA_ID		0x50
#define MESG_CAPABILITIES_ID		0x54
#define MESG_CHANNEL_ID_ID		0x51
#define MESG_CHANNEL_MESG_PERIOD_ID	0x43
#define MESG_CHANNEL_RADIO_FREQ_ID	0x45
#define MESG_CHANNEL_SEARCH_TIMEOUT_ID	0x44
#define MESG_CHANNEL_STATUS_ID		0x52
#define MESG_CLOSE_CHANNEL_ID		0x4c
#define MESG_DATA_SIZE			13 // due to dodgy msg from suunto
#define MESG_EXT_ACKNOWLEDGED_DATA_ID	0x5e
#define MESG_EXT_BROADCAST_DATA_ID	0x5d
#define MESG_EXT_BURST_DATA_ID		0x5f
#define MESG_NETWORK_KEY_ID		0x46
#define MESG_OPEN_CHANNEL_ID		0x4b
#define MESG_OPEN_RX_SCAN_ID		0x5b
#define MESG_REQUEST_ID			0x4d
#define MESG_RESPONSE_EVENT_ID		0x40
#define MESG_RESPONSE_EVENT_SIZE	3
#define MESG_SEARCH_WAVEFORM_ID		0x49
#define MESG_SYSTEM_RESET_ID		0x4a
#define MESG_TX_SYNC			0xa4
#define MESG_UNASSIGN_CHANNEL_ID	0x41
#define RESPONSE_NO_ERROR		0x00
#define EVENT_RX_FAIL			0x02

/* vim: se sw=8 ts=8: */
