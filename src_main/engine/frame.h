//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#if !defined( FRAME_H )
#define FRAME_H
#ifdef _WIN32
#pragma once
#endif

#include "recventlist.h"


// Client's now store the command they sent to the server and the entire results set of
//  that command. 
typedef struct frame_s
{
	// Data received from server
	double		receivedtime;	 // Time message from server was received, or -1.
								 //  receivedtime - senttime = latency.
	double		latency;
	// Server timestamp
	double				time;			

	// The packet of entities sent from the server.
	CRecvEntList	packet_entities;

	unsigned short clientbytes;   // Client data ( local player only )
	unsigned short localplayerinfobytes;    // # of bytes of message that were occupied by local player's data
	unsigned short otherplayerinfobytes;    // # of bytes of message that were occupied by other players' data
	unsigned short packetentitybytes;  // # of bytes that were occupied by packet entities data.
	unsigned short soundbytes;
	unsigned short eventbytes;
	unsigned short usrbytes;
	unsigned short msgbytes;     // Total size of message
	unsigned short voicebytes;

	bool		invalid;		 // True if the packet_entities delta was invalid for some reason.
	bool		choked;
} frame_t;

#endif // FRAME_H