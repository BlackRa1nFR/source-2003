//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef CL_MAIN_H
#define CL_MAIN_H
#ifdef _WIN32
#pragma once
#endif


#include "basetypes.h"
#include "networkstringtabledefs.h"
#include "const.h"


const struct CPrecacheUserData* CL_GetPrecacheUserData( TABLEID id, int index );

struct AddAngle
{
	float yawdelta;
	float starttime;
	float accum;
};

//-----------------------------------------------------------------------------
// Purpose: CClientState should hold all pieces of the client state
//   The client_state_t structure is wiped completely at every server signon
//-----------------------------------------------------------------------------
class CClientState
{
public:
	// Wipe all data
	void		Clear( void );

	int			servercount;	// server identification for prespawns, must match the svs.spawncount which
								// is incremented on server spawning.  This supercedes svs.spawn_issued, in that
								// we can now spend a fair amount of time sitting connected to the server
								// but downloading models, sounds, etc.  So much time that it is possible that the
								// server might change levels again and, if so, we need to know that.

	int			validsequence;	// this is the sequence number of the last good
								// world snapshot/update we got.  If this is 0, we can't
								// render a frame yet

	int			parsecount;		// server message counter
	int			parsecountmod;  // Modulo with network window
	double		parsecounttime; // Timestamp of parse

	QAngle		viewangles;
	CUtlVector< AddAngle >	addangle;
	
	qboolean	paused;			// send over by server
	
	float		mtime[2];		// the timestamp of last two messages
	
	float		gettime() const;
	float		getframetime() const;

	int			tickcount;			// client simulation tick counter
	float		tickremainder;
	bool		insimulation;

	int			oldtickcount;		// previous tick
	
	// These are the frames used to generate client side prediction with. The frames also contain the 
	//  state of all other entities and players, so that the visible entities list can be repopulated correctly.
	frame_t		frames[ MULTIPLAYER_BACKUP ];
	cmd_t		commands[ MULTIPLAYER_BACKUP ];

	//	Acknowledged sequence number
	int			delta_sequence;

	//
	// information that is static for the entire time connected to a server
	//
	int			playernum;	 // player entity.  skips world. Add 1 to get cl_entitites index;

	char		levelname[40];	// for display on solo scoreboard
	int			maxclients;
	
// refresh related state
	int			viewentity;		    // cl_entitites[cl.viewentity] == player point of view

	int			cdtrack;	// cd audio

	CRC32_t		serverCRC;              // To determine if client is playing hacked .map. (entities lump is skipped)
	CRC32_t		serverClientSideDllCRC; // To determine if client is playing on a hacked client dll.

	// Player information for self and other players.  Used for client side for prediction.
	player_info_t	players[MAX_CLIENTS];

	
	// This stuff manages the receiving of data tables and instantiating of client versions
	// of server-side classes.
	C_ServerClassInfo	*m_pServerClasses;
	int					m_nServerClasses;
	int					m_nServerClassBits; // log2 of number of classes
	byte				*pAreaBits;

	CEventState			events;

	// This is the usercmd slot that we predicted to last frame before we received new data
	//  from the server.  The prediction code will use this to copy some intermediate data
	//  back into the base slot based on predicted results
	int					last_prediction_start_state;
	int					last_entity_message_received_tick;

	int					last_incoming_sequence;
	int					last_command_ack;

	bool				force_send_usercmd;
	bool				continue_reading_packets;

public:

	void				DumpPrecacheStats( TABLEID table );

	TABLEID				GetModelPrecacheTable( void ) const;
	void				SetModelPrecacheTable( TABLEID id );
	
	TABLEID				GetGenericPrecacheTable( void ) const;
	void				SetGenericPrecacheTable( TABLEID id );

	TABLEID				GetSoundPrecacheTable( void ) const;
	void				SetSoundPrecacheTable( TABLEID id );

	TABLEID				GetDecalPrecacheTable( void ) const;
	void				SetDecalPrecacheTable( TABLEID id );

	TABLEID				GetInstanceBaselineTable() const;
	void				SetInstanceBaselineTable( TABLEID id );
	void				UpdateInstanceBaseline( int iStringID );

	// Public API to models
	model_t				*GetModel( int index );
	void				SetModel( int tableIndex );
	int					LookupModelIndex( char const *name );

	// Public API to generic
	char const			*GetGeneric( int index );
	void				SetGeneric( int tableIndex );
	int					LookupGenericIndex( char const *name );

	// Public API to sounds
	CSfxTable			*GetSound( int index );
	char const			*GetSoundName( int index );
	void				SetSound( int tableIndex );
	int					LookupSoundIndex( char const *name );

	// Public API to decals
	char const			*GetDecalName( int index );
	void				SetDecal( int tableIndex );

private:
	TABLEID				m_hModelPrecacheTable;	
	TABLEID				m_hGenericPrecacheTable;	
	TABLEID				m_hSoundPrecacheTable;
	TABLEID				m_hDecalPrecacheTable;
	TABLEID				m_hInstanceBaselineTable;

	CPrecacheItem		model_precache[ MAX_MODELS ];
	CPrecacheItem		generic_precache[ MAX_GENERIC ];
	CPrecacheItem		sound_precache[ MAX_SOUNDS ];
	CPrecacheItem		decal_precache[ MAX_BASE_DECALS ];

};  //CClientState

#endif // CL_MAIN_H
