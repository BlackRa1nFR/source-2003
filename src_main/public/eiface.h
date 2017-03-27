//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Defines entity interface between engine and DLLs.
// This header file included by engine files and DLL files.
//
// Before including this header, DLLs must:
//		include edict.h
// This is conveniently done for them in extdll.h
//
// $NoKeywords: $
//=============================================================================

#ifndef EIFACE_H
#define EIFACE_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "icvar.h"
#include "edict.h"
#include "terrainmod.h"
#include "vplane.h"
#include "iserverentity.h"
#include "engine/ivmodelinfo.h"
#include "soundflags.h"

//
// Defines entity interface between engine and DLLs.
// This header file included by engine files and DLL files.
//
// Before including this header, DLLs must:
//		include edict.h
// This is conveniently done for them in extdll.h
//

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class	SendTable;
class	ServerClass;
class	IMoveHelper;
struct  Ray_t;
class	CGameTrace;
typedef	CGameTrace trace_t;
struct	typedescription_t;
class	CSaveRestoreData;
struct	datamap_t;
class	SendTable;
class	ServerClass;
class	IMoveHelper;
struct  Ray_t;
struct	studiohdr_t;
class	CBaseEntity;
class	CRestore;
class	CSave;
class	variant_t;
struct	vcollide_t;
class	bf_read;
class	bf_write;
class	IRecipientFilter;
//enum	soundlevel_t;
class	CBaseEntity;
class	ITraceFilter;
struct	client_textmessage_t;

//-----------------------------------------------------------------------------
// enums + defines
//-----------------------------------------------------------------------------

#ifdef _WIN32
#define DLLEXPORT __stdcall
#else
#define DLLEXPORT /* */
#endif


//-----------------------------------------------------------------------------
// Interface the engine exposes to the game DLL
//-----------------------------------------------------------------------------
#define INTERFACEVERSION_VENGINESERVER	"VEngineServer015"
class IVEngineServer
{
public:
	virtual				~IVEngineServer() {}

	// tell engine to change level
	virtual void		ChangeLevel( char *s1, char *s2 ) = 0;
	
	// Ask engine whether the specified map is a valid map file
	virtual int			IsMapValid( char *filename ) = 0;
	
	// Is this a dedicated server?
	virtual int			IsDedicatedServer( void ) = 0;
	
	// Is in wc editing mode?
	virtual int			IsInEditMode( void ) = 0;
	
	// Add to the server/client lookup/precache table
	// NOTE: The indices for PrecacheModel are 1 based
	// a 0 returned from those methods indicates the model or sound was not
	// correctly precached
	// Generic and decal are 0 based
	virtual int			PrecacheModel( const char *s, bool preload = false ) = 0;
	virtual int			PrecacheGeneric( const char *s, bool preload = false ) = 0;
	virtual int			PrecacheSentenceFile( const char *s, bool preload = false ) = 0;
	virtual int			PrecacheDecal( const char *name, bool preload = false ) = 0;
	
	virtual void		RelinkEntity( edict_t *e, bool bFireTriggers = false, const Vector *pPrevAbsOrigin = 0 ) = 0;
	virtual int			FastUnlink( edict_t *e ) = 0;
	virtual void		FastRelink( edict_t *e, int tempHandle ) = 0;
	
	// Special purpose PVS checking
	virtual int			GetClusterForOrigin( const Vector &org ) = 0;
	virtual int			GetPVSForCluster( int cluster, int outputpvslength, unsigned char *outputpvs ) = 0;
	virtual bool		CheckOriginInPVS( const Vector &org, const unsigned char *checkpvs ) = 0;
	virtual bool		CheckBoxInPVS( const Vector &mins, const Vector &maxs, const unsigned char *checkpvs ) = 0;

	// Returns the server assigned userid for this player.  useful for logging frags, etc.  returns -1 if the edict couldn't be found in the list of clients
	virtual int			GetPlayerUserId( edict_t *e ) = 0; 
	virtual int			IndexOfEdict( const edict_t *pEdict ) = 0;
	virtual edict_t		*PEntityOfEntIndex( int iEntIndex ) = 0;
	virtual int			GetEntityCount( void ) = 0;

	// Get a client index from its (player's) entity index.
	virtual int			EntityToClientIndex(int iEntIndex) = 0;
	virtual void		GetPlayerConnectionInfo( int playerIndex, int &ping, int &packetloss ) = 0;
	virtual float		GetPlayerPing( int playerIndex, int numsamples ) = 0;
	
	// Allocate space for string and return index/offset of string in global string list
	virtual edict_t		*CreateEdict( void ) = 0;
	virtual void		RemoveEdict( edict_t *e ) = 0;
	
	// memory allocation
	virtual void		*PvAllocEntPrivateData( long cb ) = 0;
	virtual void		FreeEntPrivateData( void *pEntity ) = 0;
	virtual void		*SaveAllocMemory( size_t num, size_t size ) = 0;
	virtual void		SaveFreeMemory( void *pSaveMem ) = 0;
	
	// Sound engine
	virtual void		EmitAmbientSound( edict_t *entity, const Vector &pos, const char *samp, float vol, soundlevel_t soundlevel, int fFlags, int pitch, float delay = 0.0f ) = 0;
	virtual void        FadeClientVolume( const edict_t *pEdict, float fadePercent, float fadeOutSeconds, float holdTime, float fadeInSeconds ) = 0;
	
	// Sentences / sentence groups
	virtual int			SentenceGroupPick( int groupIndex, char *name, int nameBufLen ) = 0;
	virtual int			SentenceGroupPickSequential( int groupIndex, char *name, int nameBufLen, int sentenceIndex, int reset ) = 0;
	virtual int			SentenceIndexFromName( const char *pSentenceName ) = 0;
	virtual const char *SentenceNameFromIndex( int sentenceIndex ) = 0;
	virtual int			SentenceGroupIndexFromName( const char *pGroupName ) = 0;
	virtual const char *SentenceGroupNameFromIndex( int groupIndex ) = 0;
	virtual float		SentenceLength( int sentenceIndex ) = 0;
	

	virtual void		ServerCommand( char *str ) = 0;
	virtual void		ServerExecute( void ) = 0;
	virtual void		ClientCommand( edict_t *pEdict, char *szFmt, ... ) = 0;
	virtual void		LightStyle( int style, char *val ) = 0;
	virtual void		StaticDecal( const Vector &originInEntitySpace, int decalIndex, int entityIndex, int modelIndex ) = 0;
	
	virtual void		Message_DetermineMulticastRecipients( bool usepas, const Vector& origin, unsigned int& playerbits ) = 0;

	virtual bf_write	*MessageBegin( IRecipientFilter& filter, int msg_type = 0 ) = 0;
	virtual bf_write	*UserMessageBegin( IRecipientFilter& filter, int msg_index, int msg_size, char const *messagename /*for debugging spews*/ ) = 0;
	virtual void		MessageEnd( void ) = 0;

	virtual void		ClientPrintf( edict_t *pEdict, const char *szMsg ) = 0;
	virtual char		*Cmd_Args( void ) = 0;		// these 3 added 
	virtual char		*Cmd_Argv( int argc ) = 0;	// so game DLL can easily 
	virtual int			Cmd_Argc( void ) = 0;		// access client 'cmd' strings
	virtual void		SetView( const edict_t *pClient, const edict_t *pViewent ) = 0;
	virtual float		Time( void ) = 0;
	virtual void		CrosshairAngle( const edict_t *pClient, float pitch, float yaw ) = 0;
	virtual void        GetGameDir( char *szGetGameDir ) = 0;
	virtual byte		*LoadFileForMe( const char *filename, int *pLength ) = 0;
	virtual void        FreeFile( byte *buffer ) = 0;
	virtual int 		CompareFileTime( char *filename1, char *filename2, int *iCompare ) = 0;
	virtual edict_t		*CreateFakeClient( const char *netname ) = 0;	// returns NULL if fake client can't be created
	virtual char		*GetInfoKeyBuffer( edict_t *e ) = 0;	// passing in NULL gets the serverinfo
	virtual char		*InfoKeyValue( char *infobuffer, const char *key ) = 0;
	virtual void		SetClientKeyValue( int clientIndex, char *infobuffer, const char *key, const char *value ) = 0;
	virtual char		*COM_ParseFile( char *data, char *token ) = 0;
	virtual byte		*COM_LoadFile( const char *path, int usehunk, int *pLength ) = 0;
	// Set the pvs based on the specified origin ( not that the pvs has an 8 unit fudge factor )
	// Reset the pvs ( call right before any calls to addorigintopvs
	virtual void		ResetPVS( byte *pvs ) = 0;
	virtual void		AddOriginToPVS( const Vector &origin ) = 0;
	virtual void		SetAreaPortalState( int portalNumber, int isOpen ) = 0;
	// Queue a temp entity for transmission
	virtual void		PlaybackTempEntity( IRecipientFilter& filter, float delay, const void *pSender, const SendTable *pST, int classID  ) = 0;
	virtual int			CheckHeadnodeVisible( int nodenum, byte *visbits ) = 0;
	virtual int			CheckAreasConnected( int area1, int area2 ) = 0;

	// Get area portal bit set
	virtual int			GetArea( const Vector &origin ) = 0;
	virtual void		GetAreaBits( int area, unsigned char *bits ) = 0;

	// Given a view origin (which tells us the area to start looking in) and a portal key,
	// fill in the plane that leads out of this area (it points into whatever area it leads to).
	virtual bool		GetAreaPortalPlane( Vector const &vViewOrigin, int portalKey, VPlane *pPlane ) = 0;

	// Apply a modification to the terrain.
	virtual void		ApplyTerrainMod( TerrainModType type, CTerrainModParams const &params ) = 0;

	// HACKHACK: Save/restore wrapper - Move this to a different interface
	virtual bool		LoadGameState( char const *pMapName, bool createPlayers ) = 0;
	virtual void		LoadAdjacentEnts( const char *pOldLevel, const char *pLandmarkName ) = 0;
	virtual void		ClearSaveDir() = 0;

	// Used by CS to reload the map entities when restarting the round.
	virtual const char*	GetMapEntitiesString() = 0;

	// text message system
	virtual client_textmessage_t	*TextMessageGet( const char *pName ) = 0;
	
	// load text message
	virtual void		LogPrint(const char * msg) = 0;

};


//-----------------------------------------------------------------------------
// interface the game DLL exposes to the engine
//-----------------------------------------------------------------------------

#define INTERFACEVERSION_SERVERGAMEDLL			"ServerGameDLL002"

class IServerGameDLL
{
public:
	virtual					~IServerGameDLL()	{}

	// Initialize the game (one-time call when the DLL is first loaded )
	// Return false if there is an error.
	virtual bool			DLLInit(CreateInterfaceFn engineFactory, CreateInterfaceFn physicsFactory, 
										CreateInterfaceFn fileSystemFactory, CGlobalVars *pGlobals) = 0;

	// This is called when a new game is started. (restart, map)
	virtual bool			GameInit( void ) = 0;

	// Called any time a new level is started (after GameInit() also on level transitions within a game)
	virtual bool			LevelInit( char const *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame ) = 0;

	// The server is about to activate
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax ) = 0;

	// The server should run physics/think on all edicts
	virtual void			GameFrame( bool simulating ) = 0;

	// Called once per simulation frame on the final tick
	virtual void			GameRenderDebugOverlays( bool simulating ) = 0;

	// Called when a level is shutdown (including changing levels)
	virtual void			LevelShutdown( void ) = 0;
	// This is called when a game ends (server disconnect, death, restart, load)
	// NOT on level transitions within a game
	virtual void			GameShutdown( void ) = 0;

	virtual void			DLLShutdown( void ) = 0;

	// Give the list of classes to the engine.  The engine matches class names from here with
	//  edict_t::classname to figure out how to encode a class's data.	
	virtual ServerClass*	GetAllServerClasses( void ) = 0;

	// Returns string describing current .dll.  E.g., TeamFortress 2, Half-Life 2.  Hey, it's more descriptive than just the name of the game directory
	virtual const char     *GetGameDescription( void ) = 0;      
	
	// Let the game .dll allocate shared string tables
	virtual void			CreateNetworkStringTables( void ) = 0;
	
	// Save/restore system hooks
	virtual CSaveRestoreData  *SaveInit( int size ) = 0;
	virtual void			SaveWriteFields( CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int ) = 0;
	virtual void			SaveReadFields( CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int ) = 0;
	virtual void			SaveGlobalState( CSaveRestoreData * ) = 0;
	virtual void			RestoreGlobalState( CSaveRestoreData * ) = 0;

	virtual void			PreSave( CSaveRestoreData * ) = 0;
	virtual void			Save( CSaveRestoreData * ) = 0;
	virtual void			GetSaveComment( char *comment, int maxlength ) = 0;
	
	virtual void			WriteSaveHeaders( CSaveRestoreData * ) = 0;

	virtual void			ReadRestoreHeaders( CSaveRestoreData * ) = 0;
	virtual void			Restore( CSaveRestoreData *, bool ) = 0;

	// Returns the number of entities moved across the transition
	virtual int				CreateEntityTransitionList( CSaveRestoreData *, int ) = 0;
	// Build the list of maps adjacent to the current map
	virtual void			BuildAdjacentMapList( void ) = 0;
};

//-----------------------------------------------------------------------------
// Just an interface version name for the random number interface
// See vstdlib/random.h for the interface definition
// NOTE: If you change this, also change VENGINE_CLIENT_RANDOM_INTERFACE_VERSION in cdll_int.h
//-----------------------------------------------------------------------------
#define VENGINE_SERVER_RANDOM_INTERFACE_VERSION	"VEngineRandom001"


//-----------------------------------------------------------------------------
// Interface to get at server entities
//-----------------------------------------------------------------------------
#define INTERFACEVERSION_SERVERGAMEENTS			"ServerGameEnts001"

class IServerGameEnts
{
public:
	virtual					~IServerGameEnts()	{}

	// Only for debugging. Set the edict base so you can get an edict's index in the debugger.
	virtual void			SetDebugEdictBase(edict_t *base) = 0;
	
	virtual void			MarkEntitiesAsTouching( edict_t *e1, edict_t *e2 ) = 0;

	// Frees the entity attached to this edict
	virtual void			FreeContainingEntity( edict_t * ) = 0; 

	// This allows the engine to get at edicts in a CGameTrace.
	virtual edict_t*		BaseEntityToEdict( CBaseEntity *pEnt ) = 0;
	virtual CBaseEntity*	EdictToBaseEntity( edict_t *pEdict ) = 0;
};


#define INTERFACEVERSION_SERVERGAMECLIENTS		"ServerGameClients002"

// Player / Client related functions
class IServerGameClients
{
public:
	virtual					~IServerGameClients()	{}

	// Client is connecting to server ( return false to reject the connection )
	virtual bool			ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] ) = 0;

	// Client is active
	virtual void			ClientActive( edict_t *pEntity ) = 0;
	
	// Client is disconnecting from server
	virtual void			ClientDisconnect( edict_t *pEntity ) = 0;
	
	// Client is connected and should be put in the game
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername ) = 0;
	
	// The client has typed a command at the console
	virtual void			ClientCommand( edict_t *pEntity ) = 0;
	
	// The client's userinfo data lump has changed
	virtual void			ClientUserInfoChanged( edict_t *pEntity, char *infobuffer ) = 0;
	
	// Determine PVS origin and set PVS for the player/viewentity
	virtual void			ClientSetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char *pvs ) = 0;
	
	virtual float			ProcessUsercmds( edict_t *player, bf_read *buf, int numcmds, int totalcmds,
								int dropped_packets, bool ignore, bool paused ) = 0;
	
	// Let the game .dll do stuff after messages have been sent to clients after the frame is run
	virtual void			PostClientMessagesSent( void ) = 0;

	// Sets the client index for the client who typed the command into his/her console
	virtual void			SetCommandClient( int index ) = 0;

	// For players, looks up the player_edict_t structure
	virtual CPlayerState	*GetPlayerState( edict_t *player ) = 0;

	// The client's userinfo data lump has changed
	virtual void			ClientEarPosition( edict_t *pEntity, Vector *pEarOrigin ) = 0;
};

#endif // EIFACE_H
