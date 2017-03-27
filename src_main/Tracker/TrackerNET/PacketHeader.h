//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PACKETHEADER_H
#define PACKETHEADER_H
#pragma once

//-----------------------------------------------------------------------------
// Purpose: basic packet header
//-----------------------------------------------------------------------------
#pragma pack(push, 1)
struct packet_header_t
{
	unsigned char protocolNumber;	// protocol version
	unsigned char headerSize;		// total size of header
	unsigned short packetSize;		// total size of packet, including header

	int targetID;			// target ID
	int sessionID;			// session ID
	int sequenceNumber;		// packet identification - 0 if it's an unreliable pack
	int sequenceReply;		// the sequence number of the packet this packet is replying to - 0 if it's not a reply

	unsigned char packetNum;		// the number of the current packet (for fragmentation/reassembly)
	unsigned char totalPackets;		// the total number of packets (for fragmentation/reassembly)
};
#pragma pack(pop)

//-----------------------------------------------------------------------------
// Purpose: bitfields for use in variable descriptors
//-----------------------------------------------------------------------------
enum
{
	PACKBIT_CONTROLBIT	= 0x01,		// this must always be set
	PACKBIT_INTNAME		= 0x02,		// if this is set then it's an int named variable, instead of a string
	PACKBIT_BINARYDATA  = 0x04,		// signifies the data in this variable is binary, it's not a string
};

//-----------------------------------------------------------------------------
// Purpose: Basic packet defines
//-----------------------------------------------------------------------------
enum 
{ 
	PROTOCOL_VERSION = 4,
	PACKET_DATA_MAX = 1300, 
	PACKET_HEADER_SIZE = sizeof(packet_header_t),
};

#endif // PACKETHEADER_H
