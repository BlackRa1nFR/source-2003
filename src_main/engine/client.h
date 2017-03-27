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
#if !defined( CLIENT_H )
#define CLIENT_H
#ifdef _WIN32
#pragma once
#endif


#include "convar.h"
#include "info.h"
#include "protocol.h"
#include "dlight.h"
#include "event_system.h"
#include "checksum_crc.h"
#include "precache.h"
#include "globalvars_base.h"

#define	MAX_SCOREBOARDNAME	32                 // have it (yet, ever?).  If we do put it in, we can readjust.
#define	MAX_STYLESTRING	64

struct model_t;

class ClientClass;
class CSfxTable;

#include "client_command.h"
#include "frame.h"

typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
} lightstyle_t;

// Player info.
typedef struct player_info_s
{
	// scoreboard information
	char			name[MAX_SCOREBOARDNAME];
	// Settings for player
	char			userinfo[MAX_INFO_STRING];
	// skin information
	char			model[MAX_QPATH];

	// hashed cd key
	char			hashedcdkey[16];

	// tracker identification number
	int				trackerID;

	// Spray paint logo
	CRC32_t			logo;
} player_info_t;

#define	SIGNONS		3			// signon messages to receive before connected

#define	MAX_DLIGHTS		32
#define	MAX_ELIGHTS		64		// entity only point lights

#define	MAX_DEMOS		32
#define	MAX_DEMONAME	32

typedef enum
{
	ca_dedicated, 		// A dedicated server with no ability to start a client
	ca_disconnected, 	// Full screen console with no connection
	ca_connecting,      // Challenge requested, waiting for response or to resend connection request.
	ca_connected,		// valid netcon, talking to a server, waiting for server data
	ca_active			// d/l complete, ready game views should be displayed
} cactive_t;

//
// the client_static_t structure is persistant through an arbitrary number
// of server connections
//
typedef struct
{
	cactive_t	state;              // Current state.

// Our sequenced channel to the remote server.
	netchan_t   netchan;

	// Unreliable stuff. Gets sent in CL_Move about cl_cmdrate times per second.
	bf_write	datagram;
	byte		datagram_buf[NET_MAX_PAYLOAD];

// Connection to server.
	double      connect_time;       // If gap of connect_time to realtime > 3000, then resend connect packet

// connection information
	int			signon;			    // 0 to SIGNONS, for the signon sequence.
	char		servername[MAX_OSPATH];	// name of server from original connect command

	// personalization data sent to server	
	char		userinfo[MAX_INFO_STRING];

	// When can we send the next command packet?
	float		nextcmdtime;                
	// Sequence number of last outgoing command
	int			lastoutgoingcommand;

// demo loop control
	int			demonum;		                  // -1 = don't play demos
	char		demos[MAX_DEMOS][MAX_DEMONAME];	  // when not playing

	float		latency;		// Rolling average of frame latencey (receivedtime - senttime) values.

	int			otherplayerbits;   // For profiling
	int			localplayerbits;

	float		packet_loss;
	double		packet_loss_recalc_time;

	int			snapshotnumber;
	char		retry_address[ MAX_OSPATH ];
} client_static_t;

extern client_static_t	cls;

// This represents a server's 
class C_ServerClassInfo
{
public:
				C_ServerClassInfo();
				~C_ServerClassInfo();


public:

	ClientClass	*m_pClientClass;
	char		*m_ClassName;
	char		*m_DatatableName;

	// This is an index into the network string table (cl.GetInstanceBaselineTable()).
	int			m_InstanceBaselineIndex; // INVALID_STRING_INDEX if not initialized yet.
};


#include "cl_main.h"


// ConVars
extern	ConVar	cl_name;
extern	ConVar	cl_shownet;
extern	ConVar	cl_interp;

extern	CClientState	cl;

extern	lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
extern	dlight_t		cl_dlights[MAX_DLIGHTS];
extern	dlight_t		cl_elights[MAX_ELIGHTS];

extern CGlobalVarsBase g_ClientGlobalVariables;


#define EndGameAssertMsg( assertion, msg ) \
	if ( !(assertion) )\
		Host_EndGame msg


//=============================================================================

// cl_main.cpp
//
void CL_DecayLights (void);

void CL_Init (void);
void CL_Shutdown( void );

void CL_FireEvents( void );
void CL_Disconnect (void);
void CL_NextDemo (void);

// Resource
void CL_ParseAuthenticationMessage( unsigned char cType );
void CL_RegisterResources ( void );

//
// cl_input
//
void CL_Move( float accumulated_extra_samples );
void CL_ExtraMouseUpdate( float remainder );

void CL_ClearState (void);
void CL_ReadPackets ( bool framefinished );        // Read packets from server and other sources (ping requests, etc.)
void CL_SendConnectPacket (void);  // Send the actual connection packet, after server supplies us a challenge value.
void CL_CheckForResend (void);     // If we are in ca_connecting state and it has been cl_resend.value seconds, 
								   //  request a challenge value again if we aren't yet connected. 
//
// cl_main.cpp
//
void CL_HudMessage( const char *pMessage );
void CL_CheckClientState( void );
void CL_TakeSnapshotAndSwap();

//
// cl_parse.cpp
//
void CL_ParseServerMessage ( qboolean normal_message = true );
void CL_WriteMessageHistory( void );
int DispatchDirectUserMsg(const char *pszName, int iSize, void *pBuf);
void CL_PostReadMessages();

//
// view
//
void V_RenderView (void);

//
// cl_tent
//
void CL_SignonReply (void);

// cl_ents_parse.cpp
bool CL_IsPlayerIndex( int index );

// cl_parse_tent.cpp
void CL_ParseTEnt (void);

#endif // CLIENT_H