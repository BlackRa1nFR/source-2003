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
// $NoKeywords: $
//=============================================================================
#if !defined( HOST_H )
#define HOST_H
#ifdef _WIN32
#pragma once
#endif

#include "checksum_crc.h"
#include "convar.h"

#define SCRIPT_DIR			"scripts/"

struct model_t;

class CCommonHostState
{
public:
	model_t		*worldmodel;	// cl_entitites[0].model
	CRC32_t		worldmapCRC;      // For detecting that client has a hacked local copy of map, the client will be dropped if this occurs.
};

extern CCommonHostState host_state;


// Choke local client's/server's packets?
extern  ConVar		host_limitlocal;      
// Print a debug message when the client or server cache is missed
extern	ConVar		host_showcachemiss;

// Returns true if host is not single stepping/pausing through code/
// FIXME:  Remove from final, retail version of code.
bool Host_ShouldRun( void );
void Host_FreeToLowMark( bool server );
void Host_Disconnect( void );
int Host_RunFrame( float time, int iState );
void Host_DumpMemoryStats( void );
void Host_UpdateMapList( void );

// Force the voice stuff to send a packet out.
// bFinal is true when the user is done talking.
void CL_SendVoicePacket(bool bFinal);

// Accumulated filtered time on machine ( i.e., host_framerate can alter host_time )
extern double host_time;

extern Vector h_origin;
extern Vector h_forward, h_right, h_up;

typedef unsigned long CRC32_t;

CRC32_t Host_GetServerSendtableCRC();

// Total ticks run
extern int	host_tickcount;
// Number of ticks being run this frame
extern int	host_frameticks;
// Which tick are we currently on for this frame
extern int	host_currentframetick;

// Sub-tick remainder
extern float	host_remainder;

// PERFORMANCE INFO
#define MIN_FPS         0.1         // Host minimum fps value for maxfps.
#define MAX_FPS         1000.0        // Upper limit for maxfps.

#define MAX_FRAMETIME	0.1
#define MIN_FRAMETIME	0.001

#endif // HOST_H

