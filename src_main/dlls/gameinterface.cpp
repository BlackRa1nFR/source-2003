//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: encapsulates and implements all the accessing of the game dll from external
//			sources (only the engine at the time of writing)
//			This files ONLY contains functions and data necessary to build an interface
//			to external modules
//=============================================================================

#include "cbase.h"
#include "gamestringpool.h"
#include "entitymapdata.h"
#include "game.h"
#include "entityapi.h"
#include "client.h"
#include "saverestore.h"
#include "entitylist.h"
#include "gamerules.h"
#include "soundent.h"
#include "player.h"
#include "server_class.h"
#include "ai_node.h"
#include "ai_link.h"
#include "ai_saverestore.h"
#include "ai_networkmanager.h"
#include "ndebugoverlay.h"
#include "ivoiceserver.h"
#include <stdarg.h>
#include "movehelper_server.h"
#include "networkstringtable_gamedll.h"
#include "filesystem.h"
#include "terrainmodmgr.h"
#include "func_areaportalwindow.h"
#include "igamesystem.h"
#include "init_factory.h"
#include "vstdlib/random.h"
#include "env_wind_shared.h"
#include "engine/IEngineSound.h"
#include "ispatialpartition.h"
#include "textstatsmgr.h"
#include "bitbuf.h"
#include "saverestoretypes.h"
#include "physics_saverestore.h"
#include "tier0/vprof.h"
#include "effect_dispatch_data.h"
#include "engine/IStaticPropMgr.h"
#include "TemplateEntities.h"
#include "ai_speech.h"
#include "soundenvelope.h"
#include "usermessages.h"
#include "physics.h"
#include "mapentities.h"
#include "igameevents.h"
#include "eventlog.h"

// Engine interfaces.
IVEngineServer	*engine = NULL;
IVoiceServer	*g_pVoiceServer = NULL;
ICvar			*cvar = NULL;
IFileSystem		*filesystem = NULL;
INetworkStringTableServer *networkstringtable = NULL;
IStaticPropMgrServer *staticpropmgr = NULL;
IUniformRandomStream *random = NULL;
IEngineSound *enginesound = NULL;
ISpatialPartition *partition = NULL;
IVModelInfo *modelinfo = NULL;
IEngineTrace *enginetrace = NULL;
IGameEventManager *gameeventmanager = NULL;

void SceneManager_ClientActive( CBasePlayer *player );

ConVar g_UseNetworkVars( "UseNetworkVars", "1", FCVAR_CHEAT, "For profiling, toggle network vars." );

// String tables
TABLEID g_StringTableEffectDispatch = INVALID_STRING_TABLE;
TABLEID g_StringTableVguiScreen = INVALID_STRING_TABLE;
TABLEID g_StringTableMaterials = INVALID_STRING_TABLE;

// Holds global variables shared between engine and game.
CGlobalVars *gpGlobals;
edict_t *g_pDebugEdictBase = 0;
static int		g_nCommandClientIndex = 0;
void *gGameDLLHandle;
void *GetGameModuleHandle();


static ConVar sv_showhitboxes( "sv_showhitboxes", "-1", FCVAR_CHEAT | FCVAR_SERVER, "Send server-side hitboxes for specified entity to client (NOTE:  this uses lots of bandwidth, use on listen server only)." );
//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int UTIL_GetCommandClientIndex( void )
{
	// -1 == unknown,dedicated server console
	// 0  == player 1

	// Convert to 1 based offset
	return (g_nCommandClientIndex+1);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBasePlayer
//-----------------------------------------------------------------------------
CBasePlayer *UTIL_GetCommandClient( void )
{
	int idx = UTIL_GetCommandClientIndex();
	if ( idx > 0 )
	{
		return UTIL_PlayerByIndex( idx );
	}

	// HLDS console issued command
	return NULL;
}

extern void InitializeCvars( void );

CBaseEntity*	FindPickerEntity( CBasePlayer* pPlayer );
CAI_Node*		FindPickerAINode( CBasePlayer* pPlayer, NodeType_e nNodeType );
CAI_Link*		FindPickerAILink( CBasePlayer* pPlayer );
float			GetFloorZ(const Vector &origin);
void			UpdateAllClientData( void );
void			DrawMessageEntities();

#include "ai_network.h"

// For now just using one big AI network
extern ConVar think_limit;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void DrawAllDebugOverlays( void ) 
{
	// If in debug select mode print the selection entities name or classname
	if (CBaseEntity::m_bInDebugSelect)
	{
		CBasePlayer* pPlayer =  UTIL_PlayerByIndex( CBaseEntity::m_nDebugPlayer );

		if (pPlayer)
		{
			// First try to trace a hull to an entity
			CBaseEntity *pEntity = FindPickerEntity( pPlayer );

			if ( pEntity ) 
			{
				pEntity->DrawDebugTextOverlays();
				pEntity->DrawBBoxOverlay();
				pEntity->SendDebugPivotOverlay();
			}
		}
	}

	// --------------------------------------------------------
	//  Draw debug overlay lines 
	// --------------------------------------------------------
	NDebugOverlay::DrawOverlayLines();

	// ------------------------------------------------------------------------
	// If in wc_edit mode draw a box to highlight which node I'm looking at
	// ------------------------------------------------------------------------
	if (engine->IsInEditMode())
	{
		CBasePlayer* pPlayer = UTIL_PlayerByIndex( CBaseEntity::m_nDebugPlayer );
		if (pPlayer) 
		{
			if (g_pAINetworkManager->GetEditOps()->m_bLinkEditMode)
			{
				CAI_Link* pAILink = FindPickerAILink(pPlayer);
				if (pAILink)
				{
					// For now just using one big AI network
					Vector startPos = g_pBigAINet->GetNode(pAILink->m_iSrcID)->GetPosition(g_pAINetworkManager->GetEditOps()->m_iHullDrawNum);
					Vector endPos	= g_pBigAINet->GetNode(pAILink->m_iDestID)->GetPosition(g_pAINetworkManager->GetEditOps()->m_iHullDrawNum);
					Vector linkDir	= startPos-endPos;
					float linkLen = VectorNormalize( linkDir );
					
					// Draw in green if link that's been turned off
					if (pAILink->m_LinkInfo & bits_LINK_OFF)
					{
						NDebugOverlay::BoxDirection(startPos, Vector(-4,-4,-4), Vector(-linkLen,4,4), linkDir, 0,255,0,40,0);
					}
					else
					{
						NDebugOverlay::BoxDirection(startPos, Vector(-4,-4,-4), Vector(-linkLen,4,4), linkDir, 255,0,0,40,0);
					}
				}
			}
			else
			{
				CAI_Node* pAINode;
				if (g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
				{
					pAINode = FindPickerAINode(pPlayer,NODE_AIR);
				}
				else
				{
					pAINode = FindPickerAINode(pPlayer,NODE_GROUND);
				}

				if (pAINode)
				{
					NDebugOverlay::Box(pAINode->GetPosition(g_pAINetworkManager->GetEditOps()->m_iHullDrawNum), Vector(-8,-8,-8), Vector(8,8,8), 255,0,0,40,0);
				}
			}
			// ------------------------------------
			// If in air edit mode draw guide line
			// ------------------------------------
			if (g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
			{
				NDebugOverlay::DrawPositioningOverlay(g_pAINetworkManager->GetEditOps()->m_flAirEditDistance);
			}
			else
			{
				NDebugOverlay::DrawGroundCrossHairOverlay();
			}
		}
	}

	// For not just using one big AI Network
	g_pAINetworkManager->GetEditOps()->DrawAINetworkOverlay();	

	// PERFORMANCE: only do this in developer mode
	if ( g_pDeveloper->GetInt() )
	{
		// iterate through all objects for debug overlays
		CBaseEntity *ent = NULL;
		while ( (ent = gEntList.NextEnt(ent)) != NULL )
		{
			// HACKHACK: to flag off these calls
			if ( ent->m_debugOverlays || ent->m_pTimedOverlay )
			{
				ent->DrawDebugGeometryOverlays();
			}
		}
	}

	// A hack to draw point_message entities w/o developer required
	DrawMessageEntities();
}

class CServerGameDLL : public IServerGameDLL
{
public:
	virtual bool			DLLInit(CreateInterfaceFn engineFactory, CreateInterfaceFn physicsFactory, 
										CreateInterfaceFn fileSystemFactory, CGlobalVars *pGlobals);
	virtual void			DLLShutdown( void );
	virtual bool			GameInit( void );
	virtual void			GameShutdown( void );
	virtual bool			LevelInit( const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame );
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			LevelShutdown( void );
	virtual void			GameFrame( bool simulating );
	virtual void			GameRenderDebugOverlays( bool simulating );

	virtual ServerClass*	GetAllServerClasses( void );
	virtual const char     *GetGameDescription( void );      
	virtual void			CreateNetworkStringTables( void );
	
	// Save/restore system hooks
	virtual CSaveRestoreData  *SaveInit( int size );
	virtual void			SaveWriteFields( CSaveRestoreData *, char const* , void *, datamap_t *, typedescription_t *, int );
	virtual void			SaveReadFields( CSaveRestoreData *, char const* , void *, datamap_t *, typedescription_t *, int );
	virtual void			SaveGlobalState( CSaveRestoreData * );
	virtual void			RestoreGlobalState( CSaveRestoreData * );
	virtual int				CreateEntityTransitionList( CSaveRestoreData *, int );
	virtual void			BuildAdjacentMapList( void );

	virtual void			PreSave( CSaveRestoreData * );
	virtual void			Save( CSaveRestoreData * );
	virtual void			GetSaveComment( char *comment, int maxlength );
	virtual void			WriteSaveHeaders( CSaveRestoreData * );

	virtual void			ReadRestoreHeaders( CSaveRestoreData * );
	virtual void			Restore( CSaveRestoreData *, bool );
};

EXPOSE_SINGLE_INTERFACE(CServerGameDLL, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);

bool CServerGameDLL::DLLInit(CreateInterfaceFn engineFactory, 
		CreateInterfaceFn physicsFactory, CreateInterfaceFn fileSystemFactory, 
		CGlobalVars *pGlobals)
{
	if(!(engine = (IVEngineServer*)engineFactory(INTERFACEVERSION_VENGINESERVER, NULL)) ||
		!(g_pVoiceServer = (IVoiceServer*)engineFactory(INTERFACEVERSION_VOICESERVER, NULL)) ||
		!(cvar = (ICvar*)engineFactory(VENGINE_CVAR_INTERFACE_VERSION, NULL)) ||
		!(networkstringtable = (INetworkStringTableServer *)engineFactory(INTERFACENAME_NETWORKSTRINGTABLESERVER,NULL)) ||
		!(staticpropmgr = (IStaticPropMgrServer *)engineFactory(INTERFACEVERSION_STATICPROPMGR_SERVER,NULL)) ||
		!(random = (IUniformRandomStream *)engineFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL)) ||
		!(enginesound = (IEngineSound *)engineFactory(IENGINESOUND_SERVER_INTERFACE_VERSION, NULL)) ||
		!(partition = (ISpatialPartition *)engineFactory(INTERFACEVERSION_SPATIALPARTITION, NULL)) ||
		!(modelinfo = (IVModelInfo *)engineFactory(VMODELINFO_SERVER_INTERFACE_VERSION, NULL)) ||
		!(enginetrace = (IEngineTrace *)engineFactory(INTERFACEVERSION_ENGINETRACE_SERVER,NULL)) ||
		!(filesystem = (IFileSystem *)fileSystemFactory(FILESYSTEM_INTERFACE_VERSION,NULL)) ||
		!(gameeventmanager = (IGameEventManager *)engineFactory(INTERFACEVERSION_GAMEEVENTSMANAGER,NULL))
	)
	{
		return false;
	}

	// cache the globals
	gpGlobals = pGlobals;

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
	gGameDLLHandle = (void *)GetGameModuleHandle();

	// save these in case other system inits need them
	factorylist_t factories;
	factories.engineFactory = engineFactory;
	factories.fileSystemFactory = fileSystemFactory;
	factories.physicsFactory = physicsFactory;
	FactoryList_Store( factories );

	// init the cvar list first in case inits want to reference them
	InitializeCvars();

	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetEntitiySaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetPhysSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetAISaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetTemplateSaveRestoreBlockHandler() );

	// The string system must init first + shutdown last
	IGameSystem::Add( GameStringSystem() );

	// Physics must occur before the sound envelope manager
	IGameSystem::Add( PhysicsGameSystem() );
	
	// Add game log system
	IGameSystem::Add( GameLogSystem() );

	if ( !IGameSystem::InitAllSystems() )
		return false;

	// load used game events  
	gameeventmanager->LoadEventsFromFile("resource/gameevents.res");

	return true;
}

void CServerGameDLL::DLLShutdown( void )
{
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetTemplateSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetAISaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetPhysSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetEntitiySaveRestoreBlockHandler() );

	char *pFilename = g_TextStatsMgr.GetStatsFilename();
	if ( !pFilename || !pFilename[0] )
	{
		g_TextStatsMgr.SetStatsFilename( "stats.txt" );
	}
	g_TextStatsMgr.WriteFile( filesystem );

	IGameSystem::ShutdownAllSystems();
}

// This is called when a new game is started. (restart, map)
bool CServerGameDLL::GameInit( void )
{
	ResetGlobalState();
	engine->ServerCommand( "exec game.cfg\n" );
	engine->ServerExecute( );
	// clear out any old game's temporary save data
//	engine->ClearSaveDir();
	return true;
}

// This is called when a game ends (server disconnect, death, restart, load)
// NOT on level transitions within a game
void CServerGameDLL::GameShutdown( void )
{
	ResetGlobalState();
}

static bool g_OneWayTransition = false;
void Game_SetOneWayTransition( void )
{
	g_OneWayTransition = true;
}

static CUtlVector<EHANDLE> g_RestoredEntities;
// just for debugging, assert that this is the only time this function is called
static bool g_InRestore = false;

void AddRestoredEntity( CBaseEntity *pEntity )
{
	Assert(g_InRestore);
	if ( !pEntity )
		return;

	g_RestoredEntities.AddToTail( EHANDLE(pEntity) );
}

void EndRestoreEntities()
{
	if ( !g_InRestore )
		return;

	// Call all entities' OnRestore handlers
	for ( int i = g_RestoredEntities.Count()-1; i >=0; --i )
	{
		CBaseEntity *pEntity = g_RestoredEntities[i].Get();
		if ( pEntity && !pEntity->IsDormant() )
		{
			pEntity->OnRestore();
		}
	}

	g_RestoredEntities.Purge();

	IGameSystem::OnRestoreAllSystems();

	g_InRestore = false;
}

void BeginRestoreEntities()
{
	if ( g_InRestore )
	{
		DevMsg( 2, "BeginRestoreEntities without previous EndRestoreEntities.\n" );
		EndRestoreEntities();
	}
	Assert(g_RestoredEntities.Count()==0);
	g_RestoredEntities.Purge();
	g_InRestore = true;
}


// Called any time a new level is started (after GameInit() also on level transitions within a game)
bool CServerGameDLL::LevelInit( const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame )
{
	ResetWindspeed();

	// IGameSystem::LevelInitPreEntityAllSystems() is called when the world is precached
	// That happens either in LoadGameState() or in MapEntity_ParseAllEntities()
	if ( loadGame )
	{
		BeginRestoreEntities();
		if ( !engine->LoadGameState( pMapName, 1 ) )
		{
			MapEntity_ParseAllEntities( pMapEntities );
		}

		if ( pOldLevel )
		{
			gpGlobals->eLoadType = MapLoad_Transition;
			engine->LoadAdjacentEnts( pOldLevel, pLandmarkName );
		}
		else
		{
			gpGlobals->eLoadType = MapLoad_LoadGame;
		}

		if ( g_OneWayTransition )
		{
			engine->ClearSaveDir();
		}
	}
	else
	{
		gpGlobals->eLoadType = MapLoad_NewGame;

		// This calls serversystem::LevelInitPreEntityAllSystems()
		MapEntity_ParseAllEntities( pMapEntities );
	}

	// Sometimes an ent will Remove() itself during its precache, so RemoveImmediate won't happen.
	// This makes sure those ents get cleaned up.
	gEntList.CleanupDeleteList();

	g_AITalkSemaphore.Release();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: called after every level change and load game, iterates through all the
//			active entities and gives them a chance to fix up their state
//-----------------------------------------------------------------------------
#ifdef DEBUG
bool g_bReceivedChainedActivate;
bool g_bCheckForChainedActivate;
#define BeginCheckChainedActivate() if (0) ; else { g_bCheckForChainedActivate = true; g_bReceivedChainedActivate = false; }
#define EndCheckChainedActivate()	if (0) ; else { AssertMsg( g_bReceivedChainedActivate == true, "Entity failed to call base class Activate()\n" ); g_bCheckForChainedActivate = false; }
#else
#define BeginCheckChainedActivate() ((void)0)
#define EndCheckChainedActivate()	((void)0)
#endif

void CServerGameDLL::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	if ( gEntList.ResetDeleteList() != 0 )
	{
		Msg( "ERROR: Entity delete queue not empty on level start!\n" );
	}

	for ( CBaseEntity *pClass = gEntList.FirstEnt(); pClass != NULL; pClass = gEntList.NextEnt(pClass) )
	{
		if ( pClass && !pClass->IsDormant() )
		{
			BeginCheckChainedActivate();
			pClass->Activate();
			EndCheckChainedActivate();
		}
	}

	IGameSystem::LevelInitPostEntityAllSystems();

	// Tell the game rules to activate
	g_pGameRules->Activate();

	// only display the think limit when the game is run with "developer" mode set
	if ( !g_pDeveloper->GetInt() )
	{
		think_limit.SetValue( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called at the start of every game frame
//-----------------------------------------------------------------------------

void CServerGameDLL::GameFrame( bool simulating )
{
	VPROF( "CServerGameDLL::GameFrame" );

	// For profiling.. let them enable/disable the networkvar manual mode stuff.
	g_bUseNetworkVars = g_UseNetworkVars.GetBool();

	extern void GameStartFrame( void );
	extern void ServiceEventQueue( void );
	extern void Physics_RunThinkFunctions( bool simulating );

	// Delete anything that was marked for deletion
	//  outside of server frameloop (e.g., in response to concommand)
	gEntList.CleanupDeleteList();

	IGameSystem::FrameUpdatePreEntityThinkAllSystems();
	GameStartFrame();

	Physics_RunThinkFunctions( simulating );
	
	IGameSystem::FrameUpdatePostEntityThinkAllSystems();

	// UNDONE: Make these systems IGameSystems and move these calls into FrameUpdatePostEntityThink()
	// service event queue, firing off any actions whos time has come
	ServiceEventQueue();

	// free all ents marked in think functions
	gEntList.CleanupDeleteList();

	// FIXME:  Should this only occur on the final tick?
	UpdateAllClientData();

	g_pGameRules->EndGameFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame even if not ticking
// Input  : simulating - 
//-----------------------------------------------------------------------------
void CServerGameDLL::GameRenderDebugOverlays( bool simulating )
{
	// Only render stuff in single player
	if ( gpGlobals->maxClients != 1 )
		return;

//#ifdef _DEBUG  - allow this in release for now
	DrawAllDebugOverlays();
//#endif

	if ( sv_showhitboxes.GetInt() == -1 )
		return;

	CBaseAnimating *anim = dynamic_cast< CBaseAnimating * >( CBaseEntity::Instance( engine->PEntityOfEntIndex( sv_showhitboxes.GetInt() ) ) );
	if ( !anim )
		return;

	anim->DrawServerHitboxes();
}

// Called when a level is shutdown (including changing levels)
void CServerGameDLL::LevelShutdown( void )
{
	IGameSystem::LevelShutdownPreEntityAllSystems();

	// YWB:
	// This entity pointer is going away now and is corrupting memory on level transitions/restarts
	CSoundEnt::ShutdownSoundEnt();

	gEntList.Clear();

	IGameSystem::LevelShutdownPostEntityAllSystems();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : ServerClass*
//-----------------------------------------------------------------------------
ServerClass* CServerGameDLL::GetAllServerClasses()
{
	return g_pServerClassHead;
}


const char *CServerGameDLL::GetGameDescription( void )
{
	return ::GetGameDescription();
}

void CServerGameDLL::CreateNetworkStringTables( void )
{
	// Create any shared string tables here (and only here!)
	// E.g.:  xxx = networkstringtable->CreateStringTable( "SceneStrings", 512 );
	g_StringTableEffectDispatch = networkstringtable->CreateStringTable( "EffectDispatch", MAX_EFFECT_DISPATCH_STRINGS );
	g_StringTableVguiScreen = networkstringtable->CreateStringTable( "VguiScreen", MAX_VGUI_SCREEN_STRINGS );
	g_StringTableMaterials = networkstringtable->CreateStringTable( "Materials", MAX_MATERIAL_STRINGS );

	// Need this so we have the error material always handy
	PrecacheMaterial( "debug/debugempty" );
	Assert( GetMaterialIndex( "debug/debugempty" ) == 0 );
}

CSaveRestoreData *CServerGameDLL::SaveInit( int size )
{
	return ::SaveInit(size);
}

//-----------------------------------------------------------------------------
// Purpose: Saves data from a struct into a saverestore object, to be saved to disk
// Input  : *pSaveData - the saverestore object
//			char *pname - the name of the data to write
//			*pBaseData - the struct into which the data is to be read
//			*pFields - pointer to an array of data field descriptions
//			fieldCount - the size of the array (number of field descriptions)
//-----------------------------------------------------------------------------
void CServerGameDLL::SaveWriteFields( CSaveRestoreData *pSaveData, const char *pname, void *pBaseData, datamap_t *pMap, typedescription_t *pFields, int fieldCount )
{
	CSave saveHelper( pSaveData );
	saveHelper.WriteFields( pname, pBaseData, pMap, pFields, fieldCount );
}


//-----------------------------------------------------------------------------
// Purpose: Reads data from a save/restore block into a structure
// Input  : *pSaveData - the saverestore object
//			char *pname - the name of the data to extract from
//			*pBaseData - the struct into which the data is to be restored
//			*pFields - pointer to an array of data field descriptions
//			fieldCount - the size of the array (number of field descriptions)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

void CServerGameDLL::SaveReadFields( CSaveRestoreData *pSaveData, const char *pname, void *pBaseData, datamap_t *pMap, typedescription_t *pFields, int fieldCount )
{
	CRestore restoreHelper( pSaveData );
	restoreHelper.ReadFields( pname, pBaseData, pMap, pFields, fieldCount );
}

//-----------------------------------------------------------------------------

void CServerGameDLL::SaveGlobalState( CSaveRestoreData *s )
{
	::SaveGlobalState(s);
}

void CServerGameDLL::RestoreGlobalState(CSaveRestoreData *s)
{
	::RestoreGlobalState(s);
}

void CServerGameDLL::Save( CSaveRestoreData *s )
{
	CSave saveHelper( s );
	g_pGameSaveRestoreBlockSet->Save( &saveHelper );
}

void CServerGameDLL::Restore( CSaveRestoreData *s, bool b)
{
	CRestore restore(s);
	g_pGameSaveRestoreBlockSet->Restore( &restore, b );
	g_pGameSaveRestoreBlockSet->PostRestore();
}

int	CServerGameDLL::CreateEntityTransitionList( CSaveRestoreData *s, int a)
{
	CRestore restoreHelper( s );
	// save off file base
	int base = restoreHelper.GetReadPos();

	int movedCount = ::CreateEntityTransitionList(s, a);
	if ( movedCount )
	{
		g_pGameSaveRestoreBlockSet->CallBlockHandlerRestore( GetPhysSaveRestoreBlockHandler(), base, &restoreHelper, false );
		g_pGameSaveRestoreBlockSet->CallBlockHandlerRestore( GetAISaveRestoreBlockHandler(), base, &restoreHelper, false );
	}

	GetPhysSaveRestoreBlockHandler()->PostRestore();
	GetAISaveRestoreBlockHandler()->PostRestore();

	return movedCount;
}

void CServerGameDLL::PreSave( CSaveRestoreData *s )
{
	g_pGameSaveRestoreBlockSet->PreSave( s );
}

#include "client_textmessage.h"

// This little hack lets me marry BSP names to messages in titles.txt
typedef struct
{
	char *pBSPName;
	char *pTitleName;
} TITLECOMMENT;

static TITLECOMMENT gTitleComments[] =
{
	{ "T0A0", "T0A0TITLE" },
	{ "C0A0", "C0A0TITLE" },
	{ "C1A0", "C0A1TITLE" },
	{ "C1A1", "C1A1TITLE" },
	{ "C1A2", "C1A2TITLE" },
	{ "C1A3", "C1A3TITLE" },
	{ "C1A4", "C1A4TITLE" },
	{ "C2A1", "C2A1TITLE" },
	{ "C2A2", "C2A2TITLE" },
	{ "C2A3", "C2A3TITLE" },
	{ "C2A4D", "C2A4TITLE2" },	// These must appear before "C2A4" so all other map names starting with C2A4 get that title
	{ "C2A4E", "C2A4TITLE2" },
	{ "C2A4F", "C2A4TITLE2" },
	{ "C2A4G", "C2A4TITLE2" },
	{ "C2A4", "C2A4TITLE1" },
	{ "C2A5", "C2A5TITLE" },
	{ "C3A1", "C3A1TITLE" },
	{ "C3A2", "C3A2TITLE" },
	{ "C4A1A", "C4A1ATITLE" },	// Order is important, see above
	{ "C4A1B", "C4A1ATITLE" },
	{ "C4A1C", "C4A1ATITLE" },
	{ "C4A1D", "C4A1ATITLE" },
	{ "C4A1E", "C4A1ATITLE" },
	{ "C4A1", "C4A1TITLE" },
	{ "C4A2", "C4A2TITLE" },
	{ "C4A3", "C4A3TITLE" },
	{ "C5A1", "C5TITLE" },
};

void CServerGameDLL::GetSaveComment( char *text, int maxlength )
{
	char comment[64];
	const char	*pName;
	int		i;

	char const *mapname = STRING( gpGlobals->mapname );

	pName = NULL;
	
	// Try to find a matching title comment for this mapname
	for ( i = 0; i < ARRAYSIZE(gTitleComments) && !pName; i++ )
	{
		if ( !strnicmp( mapname, gTitleComments[i].pBSPName, strlen(gTitleComments[i].pBSPName) ) )
		{
			// Found one, read the corresponding message from titles.txt (so it can be localized)
			client_textmessage_t *pMessage = engine->TextMessageGet( gTitleComments[i].pTitleName );
			if ( pMessage )
			{
				int j;

				// Got a message, post-process it to be save name friendly
				Q_strncpy( comment, pMessage->pMessage, 64 );
				pName = comment;
				j = 0;
				// Strip out CRs
				while ( j < 64 && comment[j] )
				{
					if ( comment[j] == '\n' || comment[j] == '\r' )
						comment[j] = 0;
					else
						j++;
				}
			}
		}
	}
	
	// If we didn't get one, use the designer's map name, or the BSP name itself
	if ( !pName )
	{
		pName = mapname;
	}

	Q_snprintf( text, maxlength, "%-64.64s %02d:%02d", pName, (int)(gpGlobals->curtime/60.0), (int)fmod(gpGlobals->curtime, 60.0) );
}

void CServerGameDLL::WriteSaveHeaders( CSaveRestoreData *s )
{
	CSave saveHelper( s );
	g_pGameSaveRestoreBlockSet->WriteSaveHeaders( &saveHelper );
	g_pGameSaveRestoreBlockSet->PostSave();
}

void CServerGameDLL::ReadRestoreHeaders( CSaveRestoreData *s )
{
	CRestore restoreHelper( s );
	g_pGameSaveRestoreBlockSet->PreRestore();
	g_pGameSaveRestoreBlockSet->ReadRestoreHeaders( &restoreHelper );
}


//-----------------------------------------------------------------------------
// Purpose: Called during a transition, to build a map adjacency list
//-----------------------------------------------------------------------------
void CServerGameDLL::BuildAdjacentMapList( void )
{
	// retrieve the pointer to the save data
	CSaveRestoreData *pSaveData = gpGlobals->pSaveData;

	if ( pSaveData )
		pSaveData->levelInfo.connectionCount = BuildChangeList( pSaveData->levelInfo.levelList, MAX_LEVEL_CONNECTIONS );
}


//-----------------------------------------------------------------------------
// Precaches a vgui screen overlay material
//-----------------------------------------------------------------------------
void PrecacheMaterial( const char *pMaterialName )
{
	networkstringtable->AddString( g_StringTableMaterials, pMaterialName );
}


//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetMaterialIndex( const char *pMaterialName )
{
	if (pMaterialName)
	{
		int nIndex = networkstringtable->FindStringIndex( g_StringTableMaterials, pMaterialName );
		Assert( nIndex >= 0 );
		if (nIndex >= 0)
			return nIndex;
	}

	// This is the invalid string index
	return 0;
}


class CServerGameEnts : public IServerGameEnts
{
public:
	virtual void			SetDebugEdictBase(edict_t *base);
	virtual void			MarkEntitiesAsTouching( edict_t *e1, edict_t *e2 );
	virtual void			FreeContainingEntity( edict_t * ); 
	virtual edict_t*		BaseEntityToEdict( CBaseEntity *pEnt );
	virtual CBaseEntity*	EdictToBaseEntity( edict_t *pEdict );
};
EXPOSE_SINGLE_INTERFACE(CServerGameEnts, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);

void CServerGameEnts::SetDebugEdictBase(edict_t *base)
{
	g_pDebugEdictBase = base;
}

//-----------------------------------------------------------------------------
// Purpose: Marks entities as touching
// Input  : *e1 - 
//			*e2 - 
//-----------------------------------------------------------------------------
void CServerGameEnts::MarkEntitiesAsTouching( edict_t *e1, edict_t *e2 )
{
	CBaseEntity *entity = GetContainingEntity( e1 );
	if ( entity )
	{
		// HACKHACK: UNDONE: Pass in the trace here??!?!?
		trace_t tr;
		UTIL_ClearTrace( tr );
		entity->PhysicsMarkEntitiesAsTouching( GetContainingEntity( e2 ), tr );
	}
}

void CServerGameEnts::FreeContainingEntity( edict_t *e )
{
	::FreeContainingEntity(e);
}

edict_t* CServerGameEnts::BaseEntityToEdict( CBaseEntity *pEnt )
{
	if ( pEnt )
		return pEnt->edict();
	else
		return NULL;
}

CBaseEntity* CServerGameEnts::EdictToBaseEntity( edict_t *pEdict )
{
	if ( pEdict )
		return CBaseEntity::Instance( pEdict );
	else
		return NULL;
}



// Player / Client related functions
class CServerGameClients : public IServerGameClients
{
public:
	virtual bool			ClientConnect( edict_t *pEntity, char const* pszName, char const* pszAddress, char szRejectReason[ 128 ] );
	virtual void			ClientActive( edict_t *pEntity );
	virtual void			ClientDisconnect( edict_t *pEntity );
	virtual void			ClientPutInServer( edict_t *pEntity, const char *playername );
	virtual void			ClientCommand( edict_t *pEntity );
	virtual void			ClientUserInfoChanged( edict_t *pEntity, char *infobuffer );
	virtual void			ClientSetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char *pvs );
	virtual float			ProcessUsercmds( edict_t *player, bf_read *buf, int numcmds, int totalcmds,
								int dropped_packets, bool ignore, bool paused );
	// Player is running a command
	virtual void			PostClientMessagesSent( void );
	virtual void			SetCommandClient( int index );
	virtual CPlayerState	*GetPlayerState( edict_t *player );
	virtual void			ClientEarPosition( edict_t *pEntity, Vector *pEarOrigin );
};
EXPOSE_SINGLE_INTERFACE(CServerGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);


//-----------------------------------------------------------------------------
// Purpose: called when a player tries to connect to the server
// Input  : *pEdict - the new player
//			char *pszName - the players name
//			char *pszAddress - the IP address of the player
//			szRejectReason[ 128 ] - output - fill in with the reason why 
//			the player was not allowed to connect.
// Output : Returns TRUE if player is allowed to join, FALSE if connection is denied.
//-----------------------------------------------------------------------------
bool CServerGameClients::ClientConnect( edict_t *pEdict, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  )
{	
	return g_pGameRules->ClientConnected( pEdict, pszName, pszAddress, szRejectReason );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a player is fully active (i.e. ready to receive messages)
// Input  : *pEntity - the player
//-----------------------------------------------------------------------------
void CServerGameClients::ClientActive( edict_t *pEdict )
{
	// If we just loaded from a save file, call OnRestore on valid entities
	EndRestoreEntities();

	// Tell the sound controller to check looping sounds
	CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	CSoundEnvelopeController::GetController().CheckLoopingSoundsForPlayer( pPlayer );
	SceneManager_ClientActive( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: called when a player disconnects from a server
// Input  : *pEdict - the player
//-----------------------------------------------------------------------------
void CServerGameClients::ClientDisconnect( edict_t *pEdict )
{
	extern bool	g_fGameOver;

	CBasePlayer *player = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	if ( player )
	{
		if ( !g_fGameOver )
		{
			player->SetMaxSpeed( 0.0f );

			char text[256];
			Q_snprintf( text,sizeof(text), "- %s has left the game\n", STRING(player->pl.netname) );

			CRelieableBroadcastRecipientFilter filter;

			UserMessageBegin( filter, "SayText" );
				WRITE_BYTE( ENTINDEX(pEdict) );
				WRITE_STRING( text );
			MessageEnd();

			CSound *pSound;
			pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( pEdict ) );
			{
				// since this client isn't around to think anymore, reset their sound. 
				if ( pSound )
				{
					pSound->Reset();
				}
			}

		// since the edict doesn't get deleted, fix it so it doesn't interfere.
			player->RemoveFlag( FL_AIMTARGET ); // don't attract autoaim
			player->AddFlag( FL_DONTTOUCH );	// stop it touching anything
			player->AddFlag( FL_NOTARGET );	// stop NPCs noticing it
			player->AddSolidFlags( FSOLID_NOT_SOLID );		// nonsolid

			UTIL_Relink( player );

			if ( g_pGameRules )
			{
				g_pGameRules->ClientDisconnected( pEdict );
			}
		}

		// Make sure all Untouch()'s are called for this client leaving
		CBaseEntity::PhysicsRemoveTouchedList( player );

		// Make sure anything we "own" is simulated by the server from now on
		player->ClearPlayerSimulationList();
	}
}


void CServerGameClients::ClientPutInServer( edict_t *pEntity, const char *playername )
{
	::ClientPutInServer( pEntity, playername );
}

void CServerGameClients::ClientCommand( edict_t *pEntity )
{
	CBasePlayer *player = ToBasePlayer( GetContainingEntity( pEntity ) );
	::ClientCommand(player);
}

//-----------------------------------------------------------------------------
// Purpose: called after the player changes userinfo - gives dll a chance to modify 
//			it before it gets sent into the rest of the engine->
// Input  : *pEdict - the player
//			*infobuffer - their infobuffer
//-----------------------------------------------------------------------------
void CServerGameClients::ClientUserInfoChanged( edict_t *pEdict, char *infobuffer )
{
	// Is the client spawned yet?
	if ( !pEdict->m_pEnt )
		return;

	CBasePlayer *player = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	if ( !player )
		return;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( player->pl.netname != NULL_STRING && 
		 STRING(player->pl.netname)[0] != 0 && 
		 Q_strcmp( STRING(player->pl.netname), engine->InfoKeyValue( infobuffer, "name" )) )
	{
		char text[256];
		Q_snprintf( text,sizeof(text), "%s changed name to %s\n", STRING(player->pl.netname), engine->InfoKeyValue( infobuffer, "name" ) );

		CRelieableBroadcastRecipientFilter filter;

		UserMessageBegin( filter, "SayText" );
			WRITE_BYTE( ENTINDEX(pEdict) );
			WRITE_STRING( text );
		MessageEnd();

		UTIL_LogPrintf( "\"%s<%i>\" changed name to \"%s<%i>\"\n", STRING( player->pl.netname ), engine->GetPlayerUserId( pEdict ), engine->InfoKeyValue( infobuffer, "name" ), engine->GetPlayerUserId( pEdict ) );

		player->pl.netname = AllocPooledString( engine->InfoKeyValue( infobuffer, "name" ) );
	}

	g_pGameRules->ClientUserInfoChanged( player, infobuffer );
}


//-----------------------------------------------------------------------------
// Purpose: A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
//  view entity.  If that's the case, then pViewEntity will be non-NULL and will be used.  Otherwise, the current
//  entity's origin is used.  Either is offset by the m_vecViewOffset to get the eye position.
// From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
//  override the actual PAS or PVS values, or use a different origin.
// NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
// Input  : *pViewEntity - 
//			*pClient - 
//			**pvs - 
//			**pas - 
//-----------------------------------------------------------------------------
void CServerGameClients::ClientSetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char *pvs )
{
	Vector org;

	// *pas = ENGINE_SET_PAS ( (float *)&org );
	// xxxYWB FIXME, enable this?
	byte* pas = NULL;

	engine->ResetPVS(pvs);

	// Find the client's PVS
	CBaseEntity *pVE = NULL;
	if ( pViewEntity )
	{
		pVE = GetContainingEntity( pViewEntity );
	}

	if ( pVE )
	{
		org = pVE->EyePosition();
		engine->AddOriginToPVS( org );
	}
	else
	{

		CBasePlayer *pPlayer = ( CBasePlayer * )GetContainingEntity( pClient );
		if ( pPlayer )
		{
			org = pPlayer->EyePosition();
			pPlayer->SetupVisibility( pvs, pas );
		}
	}

	for( unsigned short i = g_AreaPortals.Head(); i != g_AreaPortals.InvalidIndex(); i = g_AreaPortals.Next(i) )
	{
		CFuncAreaPortalBase *pCur = g_AreaPortals[i];
		pCur->UpdateVisibility( org );
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//			*buf - 
//			numcmds - 
//			totalcmds - 
//			dropped_packets - 
//			ignore - 
//			paused - 
// Output : float
//-----------------------------------------------------------------------------
float CServerGameClients::ProcessUsercmds( edict_t *player, bf_read *buf, int numcmds, int totalcmds,
	int dropped_packets, bool ignore, bool paused )
{
	int				i;
	CUserCmd		*from, *to;

	// We track last three command in case we drop some 
	//  packets but get them back.
	CUserCmd		cmds[ CMD_MAXBACKUP ];  

	CUserCmd		cmdNull;  // For delta compression
	
	Assert( numcmds >= 0 );
	Assert( ( totalcmds - numcmds ) >= 0 );

	CBasePlayer *pPlayer = NULL;
	CBaseEntity *pEnt = CBaseEntity::Instance(player);
	if ( pEnt && pEnt->IsPlayer() )
	{
		pPlayer = static_cast< CBasePlayer * >( pEnt );
	}
	// Too many commands?
	if ( totalcmds < 0 || totalcmds >= ( CMD_MAXBACKUP - 1 ) )
	{
		const char *name = "unknown";
		if ( pPlayer )
		{
			name = STRING( pPlayer->pl.netname );
		}

		Msg("CBasePlayer::ProcessUsercmds: too many cmds %i sent for player %s\n", totalcmds, name );
		// FIXME:  Need a way to drop the client from here
		//SV_DropClient ( host_client, false, "CMD_MAXBACKUP hit" );
		buf->SetOverflowFlag();
		return 0.0f;
	}

	// Initialize for reading delta compressed usercmds
	cmdNull.Reset();
	from = &cmdNull;
	for ( i = totalcmds - 1; i >= 0; i-- )
	{
		to = &cmds[ i ];
		ReadUsercmd( buf, to, from );
		from = to;
	}

	// Client not fully connected or server has gone inactive  or is paused, just ignore
	if ( ignore || !pPlayer )
	{
		return 0.0f;
	}

	pPlayer->ProcessUsercmds( cmds, numcmds, totalcmds, dropped_packets, paused );
	return cmds[ 0 ].frametime;
}

void CServerGameClients::PostClientMessagesSent( void )
{
	// iterate through all objects and run the cleanup code
	CBaseEntity *ent = NULL;
	while ( (ent = gEntList.NextEnt(ent)) != NULL )
	{
		ent->PostClientMessagesSent();
	}
}

// Sets the client index for the client who typed the command into his/her console
void CServerGameClients::SetCommandClient( int index )
{
	g_nCommandClientIndex = index;
}


//-----------------------------------------------------------------------------
// The client's userinfo data lump has changed
//-----------------------------------------------------------------------------
void CServerGameClients::ClientEarPosition( edict_t *pEdict, Vector *pEarOrigin )
{
	CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	if (pPlayer)
	{
		*pEarOrigin = pPlayer->EarPosition();
	}
	else
	{
		// Shouldn't happen
		Assert(0);
		*pEarOrigin = vec3_origin;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
// Output : CPlayerState
//-----------------------------------------------------------------------------
CPlayerState *CServerGameClients::GetPlayerState( edict_t *player )
{
	// Is the client spawned yet?
	if ( !player || !player->m_pEnt )
		return NULL;

	CBasePlayer *pBasePlayer = ( CBasePlayer * )CBaseEntity::Instance( player );
	if ( !pBasePlayer )
		return NULL;

	return &pBasePlayer->pl;
}

static bf_write *g_pMsgBuffer = NULL;

void MessageBegin( IRecipientFilter& filter, int msg_type /*= 0*/ ) 
{
	Assert( !g_pMsgBuffer );

	g_pMsgBuffer = engine->MessageBegin( filter, msg_type );
}

void UserMessageBegin( IRecipientFilter& filter, const char *messagename )
{
	Assert( !g_pMsgBuffer );

	Assert( messagename );

	int msg_type = usermessages->LookupUserMessage( messagename );
	if ( msg_type == -1 )
	{
		Error( "UserMessageBegin:  Unregistered message '%s'\n", messagename );
	}

	int msg_size = usermessages->GetUserMessageSize( msg_type );

	g_pMsgBuffer = engine->UserMessageBegin( filter, msg_type, msg_size, messagename );
}

void MessageEnd( void )
{
	Assert( g_pMsgBuffer );

	engine->MessageEnd();

	g_pMsgBuffer = NULL;
}

void MessageWriteByte( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WRITE_BYTE called with no active message\n" );

	g_pMsgBuffer->WriteByte( iValue );
}

void MessageWriteChar( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WRITE_CHAR called with no active message\n" );

	g_pMsgBuffer->WriteChar( iValue );
}

void MessageWriteShort( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WRITE_SHORT called with no active message\n" );

	g_pMsgBuffer->WriteShort( iValue );
}

void MessageWriteWord( int iValue )
{
	if (!g_pMsgBuffer)
		Error( "WRITE_WORD called with no active message\n" );

	g_pMsgBuffer->WriteWord( iValue );
}

void MessageWriteLong( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteLong called with no active message\n" );

	g_pMsgBuffer->WriteLong( iValue );
}

void MessageWriteFloat( float flValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteFloat called with no active message\n" );

	g_pMsgBuffer->WriteFloat( flValue );
}

void WriteAngle( float flValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteAngle called with no active message\n" );

	g_pMsgBuffer->WriteBitAngle( flValue, 8 );
}

void MessageWriteCoord( float flValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteCoord called with no active message\n" );

	g_pMsgBuffer->WriteBitCoord( flValue );
}

void MessageWriteVec3Coord( const Vector& rgflValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteVec3Coord called with no active message\n" );

	g_pMsgBuffer->WriteBitVec3Coord( rgflValue );
}

void MessageWriteVec3Normal( const Vector& rgflValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteVec3Normal called with no active message\n" );

	g_pMsgBuffer->WriteBitVec3Normal( rgflValue );
}

void MessageWriteAngles( const QAngle& rgflValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteVec3Normal called with no active message\n" );

	g_pMsgBuffer->WriteBitAngles( rgflValue );
}

void MessageWriteString( const char *sz )
{
	if (!g_pMsgBuffer)
		Error( "WriteString called with no active message\n" );

	g_pMsgBuffer->WriteString( sz );
}

void MessageWriteEntity( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteEntity called with no active message\n" );

	g_pMsgBuffer->WriteShort( iValue );
}

// bitwise
void MessageWriteBool( bool bValue )
{
	if (!g_pMsgBuffer)
		Error( "WriteBool called with no active message\n" );

	g_pMsgBuffer->WriteOneBit( bValue ? 1 : 0 );
}

void MessageWriteUBitLong( unsigned int data, int numbits )
{
	if (!g_pMsgBuffer)
		Error( "WriteUBitLong called with no active message\n" );

	g_pMsgBuffer->WriteUBitLong( data, numbits );
}

void MessageWriteSBitLong( int data, int numbits )
{
	if (!g_pMsgBuffer)
		Error( "WriteSBitLong called with no active message\n" );

	g_pMsgBuffer->WriteSBitLong( data, numbits );
}

void MessageWriteBits( const void *pIn, int nBits )
{
	if (!g_pMsgBuffer)
		Error( "WriteBits called with no active message\n" );

	g_pMsgBuffer->WriteBits( pIn, nBits );
}


