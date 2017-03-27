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
#if !defined( EVENT_FLAGSH )
#define EVENT_FLAGSH
#ifdef _WIN32
#pragma once
#endif

// Skip local host for event send.
#define FEV_NOTHOST		(1<<0)    

// Send the event reliably.  You must specify the origin and angles and use
// PLAYBACK_EVENT_FULL for this to work correctly on the server for anything
// that depends on the event origin/angles.  I.e., the origin/angles are not
// taken from the invoking edict for reliable events.
#define FEV_RELIABLE	(1<<1)	 

// Don't restrict to PAS/PVS, send this event to _everybody_ on the server ( useful for stopping CHAN_STATIC
//  sounds started by client event when client is not in PVS anymore ( hwguy in TFC e.g. ).
#define FEV_GLOBAL		(1<<2)

// If this client already has one of these events in its queue, just update the event instead of sending it as a duplicate
//
#define FEV_UPDATE		(1<<3)

// Only send to entity specified as the invoker
#define	FEV_HOSTONLY	(1<<4)

// Only send if the event was created on the server.
#define FEV_SERVER		(1<<5)

// Only issue event client side ( from shared code )
#define FEV_CLIENT		(1<<6)

#endif