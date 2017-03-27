/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  cdll_int.h
//
// 4-23-98  
// JOHN:  client dll interface declarations
//

#ifndef CDLL_INT_H
#define CDLL_INT_H

#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "interface.h"
#include "mathlib.h"
#include "terrainmod.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class ClientClass;
struct model_t;
class CSentence;
struct vrect_t;
struct cmodel_t;
class IMaterial;
class CAudioSource;
class CMeasureSection;
class SurfInfo;
class ISpatialQuery;
struct cache_user_t;
class IMaterialSystem;
class VMatrix;
class bf_write;
class bf_read;
struct ScreenFade_t;
class CViewSetup;
//enum TerrainModType;
class CTerrainModParams;
class CSpeculativeTerrainModVert;
class CEngineSprite;
class CGlobalVarsBase;
class CPhysCollide;
class CSaveRestoreData;
struct datamap_t;
struct typedescription_t;

struct client_textmessage_t;

struct hud_player_info_t
{
	// True if this is the calling player
	bool thisplayer;  
	char *name;
	char *model;
	int	logo;
};


// The engine reports to the client DLL what stage it's entering so the DLL can latch events
// and make sure that certain operations only happen during the right stages.
// The value for each stage goes up as you move through the frame so you can check ranges of values
// and if new stages get added in-between, the range is still valid.
enum ClientFrameStage_t
{
	FRAME_UNDEFINED=-1,			// (haven't run any frames yet)
	FRAME_START,

	FRAME_NET_UPDATE_START,
		FRAME_NET_UPDATE_POSTDATAUPDATE_START,
		FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	FRAME_NET_UPDATE_END,		// client DLL does interpolation, prediction, etc..

	FRAME_RENDER_START,
	FRAME_RENDER_END
};


//-----------------------------------------------------------------------------
// Lightcache entry handle
//-----------------------------------------------------------------------------
DECLARE_POINTER_HANDLE( LightCacheHandle_t );


//-----------------------------------------------------------------------------
// Just an interface version name for the random number interface
// See vstdlib/random.h for the interface definition
// NOTE: If you change this, also change VENGINE_SERVER_RANDOM_INTERFACE_VERSION in eiface.h
//-----------------------------------------------------------------------------
#define VENGINE_CLIENT_RANDOM_INTERFACE_VERSION	"VEngineRandom001"


// --------------------------------------------------------------------------------- //
// IVEngineHud
// --------------------------------------------------------------------------------- //

// change this when the new version is incompatable with the old
#define VENGINE_CLIENT_INTERFACE_VERSION		"VEngineClient006"



// UNDONE: Split this interface into smaller pieces dealing with drawing, players, database queries
class IVEngineClient
{
public:
	virtual cmodel_t			*GetCModel(int modelindex) = 0;

	// Find the model's surfaces that intersect the given sphere.
	// Returns the number of surfaces filled in.
	virtual int					GetIntersectingSurfaces(
									const model_t *model,
									const Vector &vCenter, 
									const float radius,
									const bool bOnlyVisibleSurfaces,	// Only return surfaces visible to vCenter.
									SurfInfo *pInfos, 
									const int nMaxInfos) = 0;
	
	// Get the lighting value at the specified point.
	virtual Vector				GetLightForPoint(const Vector &pos, bool bClamp)=0;

	virtual char				*COM_ParseFile( char *data, char *token ) = 0;

	// screen handlers
	virtual void				GetScreenSize( int& width, int& height ) = 0;

	// command handlers
	virtual int					ServerCmd( const char *szCmdString, bool bReliable = true ) = 0;
	virtual int					ClientCmd( const char *szCmdString ) = 0;

	virtual void				GetPlayerInfo( int ent_num, hud_player_info_t *pinfo ) = 0;

	// Gets a unique ID for the specified player. This is the same even if you see the player on a different server.
	// iPlayer is an entity index, so client 0 would use iPlayer=1.
	// Returns false if there is no player on the server in the specified slot.
	virtual bool				GetPlayerUniqueID( int iPlayer, char playerID[16] ) = 0;

	// text message system
	virtual client_textmessage_t	*TextMessageGet( const char *pName ) = 0;
	virtual bool				Con_IsVisible( void ) = 0;

	// Get the local player index. Returns 0 if it can't be found.
	virtual int					GetPlayer( void )=0;

	// FIXME:  Move to modelinfo interface?
	// Client DLL is hooking model
	virtual const model_t		*LoadModel( const char *pName, bool bProp = false ) = 0;
	// Client DLL is unhooking model
	virtual void				UnloadModel( const model_t *model, bool bProp = false ) = 0;

	virtual float				Time( void ) = 0; // Get accurate clock
	virtual double				GetLastTimeStamp( void ) = 0; // Get last server time stamp ( cl.mtime[0] )

	virtual CSentence			*GetSentence( CAudioSource *pAudioSource ) = 0;
	virtual float				GetSentenceLength( CAudioSource *pAudioSource ) = 0;

	// User Input Processing
	virtual int					GetWindowCenterX( void ) = 0;
	virtual int					GetWindowCenterY( void ) = 0;
	virtual void				SetCursorPos( int x, int y ) = 0;
	virtual void				GetViewAngles( QAngle& va ) = 0;
	virtual void				SetViewAngles( QAngle& va ) = 0;
	
	virtual int					GetMaxClients( void ) = 0;

	virtual void				Key_Event( int key, int down ) = 0;
	virtual	const char			*Key_LookupBinding( const char *pBinding ) = 0;

	// Returns 1 if the player is fully connected and active in game
	virtual int					IsInGame( void ) = 0;
	// Returns 1 if the player is connected, but not necessarily active in game
	virtual int					IsConnected( void ) = 0;
	// Is the loading plaque being drawn ( overrides a bunch of other drawing code )
	virtual bool				IsDrawingLoadingImage( void ) = 0;

	// Printing functions
	virtual void				Con_NPrintf( int pos, char *fmt, ... ) = 0;
	virtual void				Con_NXPrintf( struct con_nprint_s *info, char *fmt, ... ) = 0;

	virtual int					Cmd_Argc( void ) = 0;	
	virtual char				*Cmd_Argv( int arg ) = 0;

	virtual void				COM_FileBase( const char *in, char *out ) = 0;
	virtual IMaterial			*TraceLineMaterialAndLighting( const Vector &start, const Vector &end, Vector &diffuseLightColor, Vector& baseColor ) = 0;
	virtual int					IsBoxVisible( const Vector& mins, const Vector& maxs ) = 0;
	virtual int					IsBoxInViewCluster( const Vector& mins, const Vector& maxs ) = 0;
	virtual bool				CullBox( const Vector& mins, const Vector& maxs ) = 0;
	virtual struct cmd_s		*GetCommand( int command_number ) = 0;

	// This returns *actual, real time*. Should only be used for statistics gathering
	virtual void				Sound_ExtraUpdate( void ) = 0;
	virtual void				SetFieldOfView( float fov ) = 0;
	virtual CMeasureSection		*GetMeasureSectionList( void ) = 0;
	virtual const char			*GetGameDirectory( void ) = 0;
	virtual const char			*GetVersionString( void ) = 0;

	// Fixme, use Vector& ???
	virtual const VMatrix& 		WorldToScreenMatrix() = 0;
	
	// Get the matrix to move a point from world space into view space
	// (translate and rotate so the camera is at the origin looking down X).
	virtual const VMatrix& 		WorldToViewMatrix() = 0;

	// Loads a game lump off disk
	virtual int					GameLumpVersion( int lumpId ) const = 0;
	virtual int					GameLumpSize( int lumpId ) const = 0;
	virtual bool				LoadGameLump( int lumpId, void* pBuffer, int size ) = 0;

	// Returns the number of leaves in the level
	virtual int					LevelLeafCount() const = 0;
	
	// Gets a way to perform spatial queries on the BSP tree
	virtual ISpatialQuery*		GetBSPTreeQuery() = 0;
	
	// Apply a modification to the terrain.
	virtual void		ApplyTerrainMod( TerrainModType type, CTerrainModParams const &params ) = 0;

	// This version doesn't actually apply the terrain mod - it tells you which terrain
	// verts would move where if you were to apply the terrain mod. This can be used
	// to show the user what will happen with a terrain mod.
	// Returns the number of verts filled into pVerts.
	virtual int			ApplyTerrainMod_Speculative( 
		TerrainModType type,
		CTerrainModParams const &params,
		CSpeculativeTerrainModVert *pVerts,
		int nMaxVerts ) = 0;

	// Convert texlight to gamma...
	virtual void		LinearToGamma( float* linear, float* gamma ) = 0;

	// Get the lightstyle value
	virtual float		LightStyleValue( int style ) = 0;

	// Draw portals if r_DrawPortals is set.
	virtual void		DrawPortals() = 0;

	// Computes light due to dynamic lighting at a point
	// If the normal isn't specified, then it'll return the maximum lighting
	virtual void		ComputeDynamicLighting( const Vector& pt, const Vector* pNormal, Vector& color ) = 0;

	// Returns the color of the ambient light
	virtual void		GetAmbientLightColor( Vector& color ) = 0;

	// Returns the dx support level
	virtual int			GetDXSupportLevel() = 0;

	// Replace the engine's material system pointer.
	virtual void		Mat_Stub( IMaterialSystem *pMatSys ) = 0;

	virtual char const	*GetLevelName( void ) = 0;
	virtual void		PlayerInfo_SetValueForKey( const char *key, const char *value ) = 0;
	virtual const char	*PlayerInfo_ValueForKey(int playerNum, const char *key) = 0;

	virtual int			GetTrackerIDForPlayer( int playerSlot ) = 0;
	virtual int			GetPlayerForTrackerID( int trackerID ) = 0;

	virtual struct IVoiceTweak_s *GetVoiceTweakAPI( void ) = 0;

	virtual void		EngineStats_BeginFrame( void ) = 0;
	virtual void		EngineStats_EndFrame( void ) = 0;
	
	// This can be used to notify test scripts that we're at a particular spot in the code.
	virtual void		CheckPoint( const char *pName ) = 0;

	// This tells the engine to fire any events that it has queued up this frame. It must be called
	// once per frame.
	virtual void		FireEvents() = 0;

	// Returns an area index if all the leaves are in the same area. If they span multple areas, then it returns -1.
	virtual int GetLeavesArea( int *pLeaves, int nLeaves ) = 0;

	// Returns true if the box touches the specified area's frustum.
	virtual bool DoesBoxTouchAreaFrustum( const Vector &mins, const Vector &maxs, int iArea ) = 0;

	// Sets the hearing origin
	virtual void SetHearingOrigin( const Vector &vecOrigin, const QAngle &angles ) = 0;

	// Sentences / sentence groups
	virtual int			SentenceGroupPick( int groupIndex, char *name, int nameBufLen ) = 0;
	virtual int			SentenceGroupPickSequential( int groupIndex, char *name, int nameBufLen, int sentenceIndex, int reset ) = 0;
	virtual int			SentenceIndexFromName( const char *pSentenceName ) = 0;
	virtual const char *SentenceNameFromIndex( int sentenceIndex ) = 0;
	virtual int			SentenceGroupIndexFromName( const char *pGroupName ) = 0;
	virtual const char *SentenceGroupNameFromIndex( int groupIndex ) = 0;
	virtual float		SentenceLength( int sentenceIndex ) = 0;

	// Computes light due to dynamic lighting at a point
	// If the normal isn't specified, then it'll return the maximum lighting
	virtual void		ComputeLighting( const Vector& pt, const Vector* pNormal, bool bClamp, Vector& color ) = 0;

	// slow routine to draw a physics model
	// NOTE: very slow code!!! just for debugging!
	virtual void		DebugDrawPhysCollide( const CPhysCollide *pCollide, IMaterial *pMaterial, matrix3x4_t& transform, const color32 &color ) = 0;

	// Activates/deactivates an occluder...
	virtual void		ActivateOccluder( int nOccluderIndex, bool bActive ) = 0;

	virtual void		*SaveAllocMemory( size_t num, size_t size ) = 0;
	virtual void		SaveFreeMemory( void *pSaveMem ) = 0;
	// GR - returns the HDR support status
	virtual bool        SupportsHDR() = 0;
};


//-----------------------------------------------------------------------------
// IVEngineCache
//-----------------------------------------------------------------------------

// change this when the new version is incompatable with the old
#define VENGINE_CACHE_INTERFACE_VERSION		"VEngineCache001"

class IVEngineCache
{
public:
	virtual void Flush( void ) = 0;
	
	// returns the cached data, and moves to the head of the LRU list
	// if present, otherwise returns NULL
	virtual void *Check( cache_user_t *c ) = 0;

	virtual void Free( cache_user_t *c ) = 0;
	
	// Returns NULL if all purgable data was tossed and there still
	// wasn't enough room.
	virtual void *Alloc( cache_user_t *c, int size, const char *name ) = 0;
	
	virtual void Report( void ) = 0;
	
	// all cache entries that subsequently allocated or successfully checked 
	// are considered "locked" and will not be freed when additional memory is needed
	virtual void EnterCriticalSection() = 0;

	// reset all protected blocks to normal
	virtual void ExitCriticalSection() = 0;
};

class IBaseClientDLL
{
public:
	// Called when the client DLL is loaded and unloaded
	virtual int				Init( CreateInterfaceFn appSystemFactory, 
									CreateInterfaceFn physicsFactory,
									CGlobalVarsBase *pGlobals ) = 0;

	virtual void			Shutdown( void ) = 0;
	
	// start of new level 
	virtual void			LevelInitPreEntity( char const* pMapName ) = 0;
	virtual void			LevelInitPostEntity( ) = 0;
	virtual void			LevelShutdown( void ) = 0;

	virtual ClientClass		*GetAllClasses( void ) = 0;

	virtual int				HudVidInit( void ) = 0;
	virtual void			HudProcessInput( bool bActive ) = 0;
	virtual void			HudUpdate( bool bActive ) = 0;
	virtual void			HudReset( void ) = 0;

	virtual int				ClientConnectionlessPacket( struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size ) = 0;

	// Mouse Input Interfaces
	virtual void			IN_ActivateMouse( void ) = 0;
	virtual void			IN_DeactivateMouse( void ) = 0;
	virtual void			IN_MouseEvent (int mstate, bool down) = 0;
	virtual void			IN_Accumulate (void) = 0;
	virtual void			IN_ClearStates (void) = 0;

	// Search for handler by name
	virtual struct kbutton_s *IN_FindKey( const char *name ) = 0;

	// Raw keyboard signal
	virtual int				IN_KeyEvent( int eventcode, int keynum, const char *pszCurrentBinding ) = 0;

	// Create CUserCmd
	virtual void			CreateMove ( int slot, int totalslots, float tick_frametime, float input_sample_frametime, int active, float packet_loss ) = 0;
	virtual void			ExtraMouseSample( float frametime, int active ) = 0;

	virtual void			WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand ) = 0;
	virtual void			EncodeUserCmdToBuffer( bf_write& buf, int buffersize, int slot ) = 0;
	virtual void			DecodeUserCmdFromBuffer( bf_read& buf, int buffersize, int slot ) = 0;

	// Set up and render one or more views
	virtual void			View_Render( vrect_t *rect ) = 0;

	// Allow engine to expressly render a view
	virtual void			RenderView( const CViewSetup &view, bool drawViewmodel ) = 0;

	// Apply screen fade directly from engine
	virtual void			View_Fade( ScreenFade_t *pSF ) = 0;

	virtual void			SetCrosshairAngle( const QAngle& angle ) = 0;

	virtual void			InitSprite( CEngineSprite *pSprite, const char *loadname ) = 0;
	virtual void			ShutdownSprite( CEngineSprite *pSprite ) = 0;
	virtual int				GetSpriteSize( void ) const = 0;

	// Called when a player starts or stops talking.
	// entindex is -1 to represent the local client talking (before the data comes back from the server). 
	// entindex is -2 to represent the local client's voice being acked by the server.
	// entindex is GetPlayer() when the server acknowledges that the local client is talking.
	virtual void			VoiceStatus( int entindex, qboolean bTalking ) = 0;

	// Networked string table definitions have arrived, allow client .dll to 
	//  hook string changes with callback function ( see INetworkStringTableClient.h for typedef )
	virtual void			InstallStringTableCallback( char const *tableName ) = 0;

	// Notification that we're moving into another stage during the frame.
	virtual void			FrameStageNotify( ClientFrameStage_t curStage ) = 0;

	virtual bool			GetUserMessageInfo( int msg_type, char *name, int& size ) = 0;
	virtual bool			DispatchUserMessage( const char *pszName, int iSize, void *pbuf ) = 0;


	// Save/restore system hooks
	virtual CSaveRestoreData  *SaveInit( int size ) = 0;
	virtual void			SaveWriteFields( CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int ) = 0;
	virtual void			SaveReadFields( CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int ) = 0;
	virtual void			PreSave( CSaveRestoreData * ) = 0;
	virtual void			Save( CSaveRestoreData * ) = 0;
	virtual void			WriteSaveHeaders( CSaveRestoreData * ) = 0;
	virtual void			ReadRestoreHeaders( CSaveRestoreData * ) = 0;
	virtual void			Restore( CSaveRestoreData *, bool ) = 0;

};

#define CLIENT_DLL_INTERFACE_VERSION		"VClient007"


#endif // CDLL_INT_H
