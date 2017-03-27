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
// $NoKeywords: $
//=============================================================================
#if !defined( SERVER_H )
#define SERVER_H
#ifdef _WIN32
#pragma once
#endif


#include "basetypes.h"
#include "filesystem.h"
#include "packed_entity.h"
#include "bitbuf.h"
#include "netadr.h"
#include "checksum_crc.h"
#include "quakedef.h"
#include "engine/IEngineSound.h"
#include "precache.h"
#include "packedentityarray.h"


class CGameTrace;
class ITraceFilter;
typedef CGameTrace trace_t;
typedef int TABLEID;

//struct trace_t;

typedef struct svchannel_s    // Room associations for this server
{
	char		        szServerChannel[16];
	qboolean            bIsDefault;  // Can't be deleted if true (unless bLeaveDefault is false to clear channel func.)
	struct svchannel_s *pNext;
} svchannel_t;

#define	STATFRAMES	100
struct svstats_t
{
	double	active;
	double	idle;
	int		count;
	int		packets;

	double	latched_active;
	double	latched_idle;
	int		latched_packets;
};

struct modinfo_t
{
	qboolean bIsMod;
	char szInfo[ 256 ];
	char szDL  [ 256 ];
	char szHLVersion[ 32 ];
	int version;
	int size;
	qboolean svonly;
	qboolean cldll;
};

//-----------------------------------------------------------------------------
// Purpose: For players on the server, deals with logo download requests
//-----------------------------------------------------------------------------
struct CLogoRequest
{
	int				playerslot;
	int				playerid;
	CRC32_t			logo;
	int				servercount;
};

// server_static_t
struct server_static_t
{
	int			maxclients;         // Current max #
	int			maxclientslimit;    // Max allowed on server.
	int			spawncount;			// Number of servers spawned since start,
									// used to check late spawns (e.g., when d/l'ing lots of
									// data)

	qboolean    dll_initialized;    // Have we loaded the game dll.

	struct client_s	*clients;		// array of up to [maxclients] client slots.
	
	svstats_t	stats;

	CRC32_t		sendtable_crc;		// Checksum of networked parts of sendtable
};

//=============================================================================

// Max # of master servers this server can be associated with
enum server_state_t
{
	ss_dead,     // No server.
	ss_loading,  // Spawning
	ss_active   // Running
};


#define MAX_MULTICAST		2500

class CServerState
{
public:
	// Clear out memory for this structure
	void		Clear( void );

	// Data
public:
	server_state_t	state;			// some actions are only valid during load

	qboolean	active;				// false if only a net client
	qboolean	paused;
	qboolean	loadgame;			// handle connections specially
	
	float		gettime() const;

	int			tickcount;
	
	char		name[64];			// map name
	char		startspot[64];
	char		modelname[64];		// maps/<name>.bsp, for model_precache[0]


	CRC32_t		clientSideDllCRC; // The dll that this server is expecting clients to be using.

	char		*lightstyles[MAX_LIGHTSTYLES];
	int			num_edicts;
	int			max_edicts;
	edict_t		*edicts;			// Can array index now, edict_t is fixed
	
	PackedEntities	baselines;		// parallel array to sv.edicts, storing entity baselines
	

	// Unreliable data to send to clients.
	bf_write	datagram;
	byte		datagram_buf[NET_MAX_PAYLOAD];

	// Reliable data to send to clients.
	bf_write	reliable_datagram;	// copied to all clients at end of frame
	byte		reliable_datagram_buf[NET_MAX_PAYLOAD];

	bf_write	multicast;
	byte		multicast_buf[MAX_MULTICAST];	// Longest multicast message

	bool		allowsignonwrites;

	bf_write	signon;
	byte		signon_buffer[ NET_MAX_PAYLOAD ];

	bf_write	signon_fullsendtables;
	byte		signon_fullsendtables_buffer[ NET_MAX_PAYLOAD ];

	int			serverclasses;		// number of unique server classes
	int			serverclassbits;	// log2 of serverclasses

	// Reset every frame after sending messages to clients.  Allows clients to sort by
	//  svc_sound message to reconstruct the correct encoding order because we actually
	//  send the reliable buffer before the unreliable and start/stop sounds that occur
	//  same frame but are in different buffers can arrive and be processed out of order, otherwise.
	int			sound_sequence;

	CUtlVector< CLogoRequest >		pending_logos;

	// New style precache lists are done this way
	void		CreateEngineStringTables( void );

	TABLEID		GetModelPrecacheTable( void ) const;
	TABLEID		GetGenericPrecacheTable( void ) const;
	TABLEID		GetSoundPrecacheTable( void ) const;
	TABLEID		GetDecalPrecacheTable( void ) const;
	TABLEID		GetInstanceBaselineTable() const;

	// Accessors to model precaching stuff
	int			PrecacheModel( char const *name, int flags, model_t *model = NULL );
	model_t		*GetModel( int index );
	int			LookupModelIndex( char const *name );

	// Accessors to model precaching stuff
	int			PrecacheSound( char const *name, int flags );
	char const	*GetSound( int index );
	int			LookupSoundIndex( char const *name );

	int			PrecacheGeneric( char const *name, int flags );
	char const	*GetGeneric( int index );
	int			LookupGenericIndex( char const *name );

	int			PrecacheDecal( char const *name, int flags );
	int			LookupDecalIndex( char const *name );

	void		DumpPrecacheStats( TABLEID table );

private:
	CPrecacheItem	model_precache[ MAX_MODELS ];
	CPrecacheItem	generic_precache[ MAX_GENERIC ];
	CPrecacheItem	sound_precache[ MAX_SOUNDS ];
	CPrecacheItem	decal_precache[ MAX_BASE_DECALS ];

	TABLEID			m_hModelPrecacheTable;	
	TABLEID			m_hSoundPrecacheTable;
	TABLEID			m_hGenericPrecacheTable;
	TABLEID			m_hDecalPrecacheTable;
	TABLEID			m_hInstanceBaselineTable;
};


// -------------------------------------------------------------------------------------------------- //
// client_frame_t
// -------------------------------------------------------------------------------------------------- //

class CFrameSnapshot;

class client_frame_t
{
public:

						client_frame_t();
						~client_frame_t();

	// Accessors to snapshots. The data is protected because the snapshots are reference-counted.
	CFrameSnapshot*		GetSnapshot() const;
	void				SetSnapshot( CFrameSnapshot *pSnapshot );

	int					GetNumEntities() const;


public:

	// Time world sample was sent to client.
	double				senttime;       
	// Realtime when client ack'd the update.
	float				raw_ping;
	// Ping value adjusted for client/server framerate issues
	float				latency;     
	 // Internal lag (i.e., local client should = 0 ping)
	float               frame_time;    

	// State of entities this frame from the POV of the client.
	PackedEntities		entities;       

	// Used by server to indicate if the entity was in the player's pvs
	unsigned char 		entity_in_pvs[ PAD_NUMBER( MAX_EDICTS, 8 ) ];

private:

	// Index of snapshot entry that stores the entities that were active and the serial numbers
	//  for the frame number this packed entity corresponds to
	CFrameSnapshot		*m_pSnapshot;
};


inline CFrameSnapshot* client_frame_t::GetSnapshot() const
{
	return m_pSnapshot;
}

inline int client_frame_t::GetNumEntities() const
{
	return entities.num_entities;
}



// -------------------------------------------------------------------------------------------------- //
// client_t
// -------------------------------------------------------------------------------------------------- //

typedef struct client_s
{
	// ------------------------------------------------------------ //
	// Voice stuff.
	// ------------------------------------------------------------ //
	bool			m_bLoopback;		// Does this client want to hear his own voice?
	unsigned long	m_VoiceStreams[MAX_CLIENTS / 32 + 1];	// Which other clients does this guy's voice stream go to?



	qboolean		active;				// false = client is free
	qboolean		spawned;			// false = don't send datagrams
	qboolean        connected;          // On server, getting data.

//===== NETWORK ============
	netchan_t		netchan;            // The client's net connection.
	int				chokecount;         // Number of packets choked at the server because the client - server
										//  network channel is backlogged with too much data.
	int				delta_sequence;		// -1 = no compression.  This is where the server is creating the
										// compressed info from.
	int				acknowledged_tickcount; // tracks host_tickcount based on what client has ack'd receiving
	int				tabledef_acknowledged_tickcount; // tracks host_tickcount based on what client has ack'd receiving
	int				GetMaxAckTickCount() const;

	// This is used when we send out a nodelta packet to put the client in a state where we wait 
	// until we get an ack from them on this packet.
	// This is for 3 reasons:
	// 1. A client requesting a nodelta packet means they're screwed so no point in deluging them with data.
	//    Better to send the uncompressed data at a slow rate until we hear back from them (if at all).
	// 2. Since the nodelta packet deletes all client entities, we can't ever delta from a packet previous to it.
	// 3. It can eat up a lot of CPU on the server to keep building nodelta packets while waiting for
	//    a client to get back on its feet.
	int							m_ForceWaitForAck;
	bool						m_bResendNoDelta;

	// This is set each frame if we're using the single player optimized path so if they disable that path,
	// it knows to send an uncompressed packet.
	bool m_bUsedLocalNetworkBackdoor;
	
	qboolean		fakeclient;			// JAC: This client is a fake player controlled by the game DLL

	// The datagram is written to after every frame, but only cleared
	//  when it is sent out to the client.  overflow is tolerated.
	bf_write		datagram;
	byte			datagram_buf[NET_MAX_PAYLOAD];

	double			connection_started;	// Or time of disconnect for zombies
	
	// Time when we should send next world state update ( datagram )
	double          next_messagetime;   
	// Default time to wait for next message
	double          next_messageinterval;  

	qboolean		send_message;		// Set on frames a datagram arrived on
	qboolean		skip_message;		// Defer message sending for one frame

	client_frame_t	*frames; // updates can be deltad from here
	int				numframes;

	CEventState		events;   // Per edict events

	// Identity information.
	edict_t			*edict;				// EDICT_NUM(clientnum+1)

	const edict_t	*pViewEntity;		// View Entity (camera or the client itself)

	int				userid;				// identifying number on server
	char			userinfo[MAX_INFO_STRING];		// infostring
	qboolean		sendinfo;
	char            hashedcdkey[SIGNED_GUID_LEN + 1]; // MD5 hash is 32 hex #'s, plus trailing 0

	char			name[32];			// for printing to other people
		
	int				entityId;			// ID # in save structure

	// Spray point logo
	CRC32_t			logo;
	bool			request_logo;		// True if logo request should be made at SV_New_f

	// Client sends this during connection, so we can see if
	//  we need to send sendtable info or if the .dll matches
	CRC32_t			sendtable_crc;		
} client_t;


inline int client_t::GetMaxAckTickCount() const
{
	if ( acknowledged_tickcount > tabledef_acknowledged_tickcount )
		return acknowledged_tickcount;
	else
		return tabledef_acknowledged_tickcount;
}


enum sv_delta_t
{ 
	sv_packet_nodelta = 0, 
	sv_packet_delta
};

//============================================================================

class IServerGameDLL;
class IServerGameEnts;
class IServerGameClients;
extern IServerGameDLL	*serverGameDLL;
extern IServerGameEnts *serverGameEnts;
extern IServerGameClients *serverGameClients;

// Master server address struct for use in building heartbeats
extern	ConVar	skill;
extern	ConVar	deathmatch;
extern	ConVar	coop;
extern  ConVar  sv_lan;

extern	server_static_t	svs;				// persistant server info
extern	CServerState	sv;					// local server

extern	client_t	*host_client;

extern	edict_t		*sv_player;

extern modinfo_t gmodinfo;
//===========================================================

// sv_main.c

void SV_Frame( bool send_client_updates );
void SV_Init (void);
void SV_Shutdown( void );

void SV_InitGameDLL( void );
void SV_ShutdownGameDLL( void );

void SV_SendClientCommands (char *fmt, ...);
void SV_ReplicateConVarChange( ConVar const *var, char const *newValue );
void SV_ClearPacketEntities( PackedEntities *frame );
void SV_ExtractFromUserinfo (client_t *cl);
char *SV_ExtractNameFromUserinfo( client_t *cl );
void SV_ClearMemory( void );

void SV_ResetModInfo( void );

class IRecipientFilter;
void SV_StartSound ( IRecipientFilter& filter, edict_t *pSoundEmittingEntity, int iChannel, const char *pSample, 
	float flVolume, soundlevel_t iSoundLevel, int iFlags, int iPitch, const Vector *pOrigin = NULL, float soundtime = 0.0f );

void SV_DropClient (client_t *cl, qboolean crash, char *fmt, ... );
void SV_FullClientUpdate(client_t *cl, bf_write *sb);

void SV_CheckTimeouts (void);
float SV_CalcLatency (client_t *cl, int numsamples = 0 );
float SV_CalcPing( client_t *cl, int numsamples = 1 );
int SV_CalcPacketLoss (client_t *cl);

void SV_CountPlayers( int * );
void SV_New_f (void);

int SV_ModelIndex (const char *name);
int SV_FindOrAddModel (const char *name, bool preload );
int SV_SoundIndex (const char *name);
int SV_FindOrAddSound(const char *name, bool preload );
int SV_GenericIndex(const char *name);
int SV_FindOrAddGeneric(const char *name, bool preload );
int SV_DecalIndex(const char *name);
int SV_FindOrAddDecal(const char *name, bool preload );

void SV_ClientPrintf (const char *fmt, ...);
void SV_BroadcastPrintf (const char *fmt, ...);

void SV_Physics( bool simulating );
void SV_GameRenderDebugOverlays();

class IServerEntity;

void SV_ExecuteClientMessage (client_t *cl);

void SV_WriteClientdataToMessage( client_t *ent, bf_write *msg );

void SV_BroadcastCommand (char *fmt, ...);

int  SV_SpawnServer( char *level, char *startspot );

void SV_ActivateServer();
bool IsSinglePlayerGame( void );

void SV_DetermineMulticastRecipients( bool usepas, const Vector& origin, unsigned int& playerbits );

// sv_redirect.cpp
enum redirect_t
{
	RD_NONE = 0,
	RD_CLIENT,
	RD_PACKET
};

qboolean SV_RedirectActive( void );
void SV_RedirectAddText( const char *txt );
void SV_RedirectStart( redirect_t rd, netadr_t *addr );
void SV_RedirectEnd( void );

// sv_rcom.cpp
void SV_Rcom( netadr_t *net_from );

#endif // SERVER_H
