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
#if !defined( EVENT_SYSTEM_H )
#define EVENT_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif

// 64 simultaneous events, max
#define MAX_EVENT_QUEUE 64
// 192 bytes of data per event max
#define EVENT_DATA_LEN_BITS	11			// ( 1<<8 bits == 256, but only using 192 below )
#define MAX_EVENT_DATA 192

#include "event_flags.h"

class CEventInfo
{
public:
	// 0 implies not in use
	short index;			  
	// If non-zero, the time when the event should be fired ( fixed up on the client )
	float fire_time;       
	
	// Length of data bits
	int	  bits;
	// Raw event data
	byte data[ MAX_EVENT_DATA ];
	// CLIENT ONLY Reliable or not, etc.
	int	  flags;			
};

class CEventState
{
public:
	CEventInfo ei[ MAX_EVENT_QUEUE ];
};

#endif // EVENT_SYSTEM_H