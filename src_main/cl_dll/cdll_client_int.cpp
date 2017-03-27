//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include <crtmemdebug.h>
#include "vgui_int.h"
#include "clientmode.h"
#include "cdll_convar.h"
#include "iinput.h"
#include "iviewrender.h"
#include "ivieweffects.h"
#include "ivmodemanager.h"
#include "prediction.h"
#include "clientsideeffects.h"
#include "particlemgr.h"
#include "initializer.h"
#include "smoke_fog_overlay.h"
#include "view.h"
#include "ienginevgui.h"
#include "inetgraph.h"
#include "iefx.h"
#include "enginesprite.h"
#include "networkstringtable_clientdll.h"
#include "voice_status.h"
#include "c_terrainmodmgr.h"
#include "FileSystem.h"
#include "c_te_legacytempents.h"
#include "c_rope.h"
#include "engine/IShadowMgr.h"
#include "engine/IStaticPropMgr.h"
#include "hud_chat.h"
#include "hud_crosshair.h"
#include "env_wind_shared.h"
#include "detailobjectsystem.h"
#include "clienteffectprecachesystem.h"
#include "soundEnvelope.h"
#include "c_basetempentity.h"
#include "vguimatsurface/IMatSystemSurface.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "c_soundscape.h"
#include "engine/IVDebugOverlay.h"
#include "vguicenterprint.h"
#include "imessagechars.h"
#include "iviewrender_beams.h"
#include "vgui_vprofpanel.h"
#include "tier0/vprof.h"
#include "engine/IEngineTrace.h"
#include "engine/ivmodelinfo.h"
#include "physics.h"
#include "usermessages.h"
#include "gamestringpool.h"
#include "c_user_message_register.h"
#include "igameuifuncs.h"
#include "saverestoretypes.h"
#include "saverestore.h"
#include "physics_saverestore.h"
#include "igameevents.h"

extern ConVar	cl_predict;

// IF YOU ADD AN INTERFACE, EXTERN IT IN THE HEADER FILE.
IVEngineClient	*engine = NULL;
IVModelRender *modelrender = NULL;
IVEfx *effects = NULL;
ICvar *cvar = NULL;
IVRenderView *render = NULL;
IVDebugOverlay *debugoverlay = NULL;
IMaterialSystem *materials = NULL;
IMaterialSystemHardwareConfig *g_pMaterialSystemHardwareConfig = NULL;
IVEngineCache *engineCache = NULL;
IVModelInfo *modelinfo = NULL;
IEngineVGui *enginevgui = NULL;
INetGraph *netgraph = NULL;
INetworkStringTableClient *networkstringtable = NULL;
ISpatialPartition* partition = NULL;
IFileSystem *filesystem = NULL;
IShadowMgr *shadowmgr = NULL;
IStaticPropMgrClient *staticpropmgr = NULL;
IEngineSound *enginesound = NULL;
IUniformRandomStream *random;
static CGaussianRandomStream s_GaussianRandomStream;
CGaussianRandomStream *randomgaussian = &s_GaussianRandomStream;
IMatSystemSurface *g_pMatSystemSurface = NULL;
ISharedGameRules *sharedgamerules = NULL;
IEngineTrace *enginetrace = NULL;
IGameUIFuncs *gameuifuncs = NULL;
IGameEventManager *gameeventmanager = NULL;

// String tables
TABLEID g_StringTableEffectDispatch = INVALID_STRING_TABLE;
TABLEID g_StringTableVguiScreen = INVALID_STRING_TABLE;
TABLEID g_StringTableMaterials = INVALID_STRING_TABLE;

static CGlobalVarsBase dummyvars( true );
// So stuff that might reference gpGlobals during DLL initialization won't have a NULL pointer.
// Once the engine calls Init on this DLL, this pointer gets assigned to the shared data in the engine
CGlobalVarsBase *gpGlobals = &dummyvars;

// Any entities that want an OnDataChanged during simulation register for it here.
class CDataChangedEvent
{
public:
	CDataChangedEvent() {}
	CDataChangedEvent( IClientNetworkable *ent, DataUpdateType_t updateType, int *pStoredEvent )
	{
		m_pEntity = ent;
		m_UpdateType = updateType;
		m_pStoredEvent = pStoredEvent;
	}

	IClientNetworkable	*m_pEntity;
	DataUpdateType_t	m_UpdateType;
	int					*m_pStoredEvent;
};

ISaveRestoreBlockHandler *GetEntitiySaveRestoreBlockHandler();

CUtlLinkedList<CDataChangedEvent, unsigned short> g_DataChangedEvents;
ClientFrameStage_t g_CurFrameStage = FRAME_UNDEFINED;


class IMoveHelper;


static ConVar g_CV_ShowParticleCounts("showparticlecounts", "0", 0, "Display number of particles drawn per frame");

// Console variable accessor.
static CDLL_ConVarAccessor g_ConVarAccessor;

// Physics system
bool	g_bLevelInitialized;


//-----------------------------------------------------------------------------
// Helper interface for voice.
//-----------------------------------------------------------------------------
class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	virtual void GetPlayerTextColor(int entindex, int color[3])
	{
		color[0] = color[1] = color[2] = 128;
	}

	virtual void UpdateCursorState()
	{
	}

	virtual bool			CanShowSpeakerLabels()
	{
		return true;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;




//-----------------------------------------------------------------------------
// Purpose: engine to client .dll interface
//-----------------------------------------------------------------------------
class CHLClient : public IBaseClientDLL
{
public:
	CHLClient();

	virtual int						Init( CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CGlobalVarsBase *pGlobals );

	virtual void					Shutdown( void );

	virtual void					LevelInitPreEntity( const char *pMapName );
	virtual void					LevelInitPostEntity();
	virtual void					LevelShutdown( void );

	virtual ClientClass				*GetAllClasses( void );

	virtual int						HudVidInit( void );
	virtual void					HudProcessInput( bool bActive );
	virtual void					HudUpdate( bool bActive );
	virtual void					HudReset( void );

	virtual int						ClientConnectionlessPacket( struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );

	// Mouse Input Interfaces
	virtual void					IN_ActivateMouse( void );
	virtual void					IN_DeactivateMouse( void );
	virtual void					IN_MouseEvent ( int mstate, bool down );
	virtual void					IN_Accumulate ( void );
	virtual void					IN_ClearStates ( void );
	virtual struct kbutton_s		*IN_FindKey( const char *name );
	// Raw signal
	virtual int						IN_KeyEvent( int eventcode, int keynum, const char *pszCurrentBinding );
	// Create movement command
	virtual void					CreateMove ( int command_number, int totalslots, float tick_frametime, float input_sample_frametime, int active, float packet_loss );
	virtual void					ExtraMouseSample( float frametime, int active );
	virtual void					WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand );	
	virtual void					EncodeUserCmdToBuffer( bf_write& buf, int buffersize, int slot );
	virtual void					DecodeUserCmdFromBuffer( bf_read& buf, int buffersize, int slot );


	virtual void					View_Render( vrect_t *rect );
	virtual void					RenderView( const CViewSetup &view, bool drawViewmodel );
	virtual void					View_Fade( ScreenFade_t *pSF );
	
	virtual void					SetCrosshairAngle( const QAngle& angle );

	virtual void					InitSprite( CEngineSprite *pSprite, const char *loadname );
	virtual void					ShutdownSprite( CEngineSprite *pSprite );

	virtual int						GetSpriteSize( void ) const;

	virtual void					VoiceStatus( int entindex, qboolean bTalking );

	virtual void					InstallStringTableCallback( const char *tableName );

	virtual void					FrameStageNotify( ClientFrameStage_t curStage );

	virtual bool					GetUserMessageInfo( int msg_type, char *name, int& size );
	virtual bool					DispatchUserMessage( const char *pszName, int iSize, void *pbuf );

	// Save/restore system hooks
	virtual CSaveRestoreData  *SaveInit( int size );
	virtual void			SaveWriteFields( CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int );
	virtual void			SaveReadFields( CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int );
	virtual void			PreSave( CSaveRestoreData * );
	virtual void			Save( CSaveRestoreData * );
	virtual void			WriteSaveHeaders( CSaveRestoreData * );
	virtual void			ReadRestoreHeaders( CSaveRestoreData * );
	virtual void			Restore( CSaveRestoreData *, bool );

public:
	void PrecacheMaterial( const char *pMaterialName );

private:
	void UncacheAllMaterials( );

	CUtlVector< IMaterial * > m_CachedMaterials;
};


CHLClient gHLClient;
IBaseClientDLL *clientdll = &gHLClient;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CHLClient, IBaseClientDLL, CLIENT_DLL_INTERFACE_VERSION, gHLClient );


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------

CHLClient::CHLClient() 
{
	// Kinda bogus, but the logic in the engine is too convoluted to put it there
	g_bLevelInitialized = false;
}


extern IGameSystem *ViewportClientSystem();

//-----------------------------------------------------------------------------
// Purpose: Called when the DLL is first loaded.
// Input  : engineFactory - 
// Output : int
//-----------------------------------------------------------------------------
int CHLClient::Init( CreateInterfaceFn appSystemFactory, CreateInterfaceFn physicsFactory, CGlobalVarsBase *pGlobals )
{
	InitCRTMemDebug();
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );

	// Hook up global variables
	gpGlobals = pGlobals;

	// We aren't happy unless we get all of our interfaces.
	if(
		!(engine = (IVEngineClient *)appSystemFactory( VENGINE_CLIENT_INTERFACE_VERSION, NULL )) ||
		!(modelrender = (IVModelRender *)appSystemFactory( VENGINE_HUDMODEL_INTERFACE_VERSION, NULL )) ||
		!(effects = (IVEfx *)appSystemFactory( VENGINE_EFFECTS_INTERFACE_VERSION, NULL )) ||
		!(cvar = (ICvar *)appSystemFactory( VENGINE_CVAR_INTERFACE_VERSION, NULL )) ||
		!(enginetrace = (IEngineTrace *)appSystemFactory( INTERFACEVERSION_ENGINETRACE_CLIENT, NULL )) ||
		!(render = (IVRenderView *)appSystemFactory( VENGINE_RENDERVIEW_INTERFACE_VERSION, NULL )) ||
		!(debugoverlay = (IVDebugOverlay *)appSystemFactory( VDEBUG_OVERLAY_INTERFACE_VERSION, NULL )) ||
		!(materials = (IMaterialSystem *)appSystemFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL )) ||
		!(engineCache = (IVEngineCache*)appSystemFactory(VENGINE_CACHE_INTERFACE_VERSION, NULL )) ||
		!(modelinfo = (IVModelInfo *)appSystemFactory(VMODELINFO_CLIENT_INTERFACE_VERSION, NULL )) ||
		!(netgraph = (INetGraph *)appSystemFactory(VNETGRAPH_INTERFACE_VERSION, NULL )) ||
		!(enginevgui = (IEngineVGui *)appSystemFactory(VENGINE_VGUI_VERSION, NULL )) ||
		!(networkstringtable = (INetworkStringTableClient *)appSystemFactory(INTERFACENAME_NETWORKSTRINGTABLECLIENT,NULL)) ||
		!(partition = (ISpatialPartition *)appSystemFactory(INTERFACEVERSION_SPATIALPARTITION, NULL)) ||
		!(shadowmgr = (IShadowMgr *)appSystemFactory(ENGINE_SHADOWMGR_INTERFACE_VERSION, NULL)) ||
		!(staticpropmgr = (IStaticPropMgrClient *)appSystemFactory(INTERFACEVERSION_STATICPROPMGR_CLIENT, NULL)) ||
		!(enginesound = (IEngineSound *)appSystemFactory(IENGINESOUND_CLIENT_INTERFACE_VERSION, NULL)) ||
		!(filesystem = (IFileSystem *)appSystemFactory(FILESYSTEM_INTERFACE_VERSION, NULL)) ||
		!(random = (IUniformRandomStream *)appSystemFactory(VENGINE_CLIENT_RANDOM_INTERFACE_VERSION, NULL)) ||
		!(gameuifuncs = (IGameUIFuncs * )appSystemFactory( VENGINE_GAMEUIFUNCS_VERSION, NULL )) ||
		!(gameeventmanager = (IGameEventManager *)appSystemFactory(INTERFACEVERSION_GAMEEVENTSMANAGER,NULL))
		)
	{
		return false;
	}

	g_pMaterialSystemHardwareConfig = materials->GetHardwareConfig( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, NULL );
	if( !g_pMaterialSystemHardwareConfig )
	{
		return false;
	}

	SetScreenSize();

	// Hook up the gaussian random number generator
	s_GaussianRandomStream.AttachToStream( random );

	// Initialize the console variables.
	ConCommandBaseMgr::OneTimeInit(&g_ConVarAccessor);

	if(!Initializer::InitializeAllObjects())
		return false;

	if(!g_ParticleMgr.Init(8192, materials))
		return false;

	// load used game events  
	gameeventmanager->LoadEventsFromFile("resource/gameevents.res");

	if(!VGui_Startup( appSystemFactory ))
		return false;

	g_pMatSystemSurface = (IMatSystemSurface*)vgui::surface()->QueryInterface( MAT_SYSTEM_SURFACE_INTERFACE_VERSION ); 
	if (!g_pMatSystemSurface)
		return false;

	// Add the client systems.	
	
	// Client Leaf System has to be initialized first, since DetailObjectSystem uses it
	IGameSystem::Add( GameStringSystem() );
	IGameSystem::Add( ClientLeafSystem() );
	IGameSystem::Add( DetailObjectSystem() );
	IGameSystem::Add( ViewportClientSystem() );
	IGameSystem::Add( ClientEffectPrecacheSystem() );
	IGameSystem::Add( g_pClientShadowMgr );
	IGameSystem::Add( ClientThinkList() );
	IGameSystem::Add( ClientSoundscapeSystem() );

#if defined( CLIENT_DLL ) && defined( COPY_CHECK_STRESSTEST )
	IGameSystem::Add( GetPredictionCopyTester() );
#endif

	modemanager->Init( );

	gHUD.Init();

	g_pClientMode->Init();

	if( !IGameSystem::InitAllSystems() )
		return false;

	g_pClientMode->Enable();

	view->Init();
	vieweffects->Init();

	C_BaseTempEntity::PrecacheTempEnts();

	input->Init_All();

	VGui_CreateGlobalPanels();

	InitSmokeFogOverlay();

	C_TerrainMod_Init();

	// Register user messages..
	CUserMessageRegister::RegisterAll();

	ClientVoiceMgr_Init();

	// Embed voice status icons inside chat element
	{
		CHudChat *chatElement = GET_HUDELEMENT( CHudChat );
		vgui::VPANEL parent = enginevgui->GetPanel( PANEL_CLIENTDLL );
		if ( chatElement )
		{
			parent = chatElement->GetVoiceArea()->GetVPanel();
		}
		
		GetClientVoiceMgr()->Init( &g_VoiceStatusHelper, parent );
	}

	if ( !PhysicsDLLInit( physicsFactory ) )
		return false;

	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetEntitiySaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetPhysSaveRestoreBlockHandler() );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Called when the client .dll is being dismissed
//-----------------------------------------------------------------------------
void CHLClient::Shutdown( void )
{
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetPhysSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetEntitiySaveRestoreBlockHandler() );

	ClientVoiceMgr_Shutdown();

	Initializer::FreeAllObjects();

	g_pClientMode->Disable();
	g_pClientMode->Shutdown();

	input->Shutdown_All();
	C_BaseTempEntity::ClearDynamicTempEnts();
	TermSmokeFogOverlay();
	view->Shutdown();
	UncacheAllMaterials();

	IGameSystem::ShutdownAllSystems();

	gHUD.Shutdown();
	VGui_Shutdown();

	g_pMatSystemSurface = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//  Called when the game initializes
//  and whenever the vid_mode is changed
//  so the HUD can reinitialize itself.
// Output : int
//-----------------------------------------------------------------------------
int CHLClient::HudVidInit( void )
{
	gHUD.VidInit();
	GetClientVoiceMgr()->VidInit();

	return 1;
}

//-----------------------------------------------------------------------------
// Method used to allow the client to filter input messages before the 
// move record is transmitted to the server
//-----------------------------------------------------------------------------

void CHLClient::HudProcessInput( bool bActive )
{
	// Q: Why isn't this done be the engine when it deals with all other input?
	input->ControllerCommands();

	g_pClientMode->ProcessInput( bActive );
}

//-----------------------------------------------------------------------------
// Purpose: Called when shared data gets changed, allows dll to modify data
// Input  : bActive - 
//-----------------------------------------------------------------------------
void CHLClient::HudUpdate( bool bActive )
{
	float frametime = gpGlobals->frametime;

	GetClientVoiceMgr()->Frame( frametime );

	gHUD.UpdateHud( bActive );

	IGameSystem::UpdateAllSystems( frametime );

	// I don't think this is necessary any longer, but I will leave it until
	// I can check into this further.
	C_BaseTempEntity::CheckDynamicTempEnts();
}

//-----------------------------------------------------------------------------
// Purpose: Called to restore to "non"HUD state.
//-----------------------------------------------------------------------------
void CHLClient::HudReset( void )
{
	gHUD.VidInit();
	PhysicsReset();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : ClientClass
//-----------------------------------------------------------------------------
ClientClass *CHLClient::GetAllClasses( void )
{
	return g_pClientClassHead;
}

//-----------------------------------------------------------------------------
// Purpose: Allows the HUD to process connectionless packets and send an optional response
// Input  : *net_from - address of remote sender
//			*args - string command sent by sender
//			*response_buffer - output response data here
//			*response_buffer_size - size of data you are returning
// Output : int
//-----------------------------------------------------------------------------
int CHLClient::ClientConnectionlessPacket( struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	// Parse stuff from args
	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLClient::IN_ActivateMouse( void )
{
	input->ActivateMouse();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLClient::IN_DeactivateMouse( void )
{
	input->DeactivateMouse();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : mstate - 
//-----------------------------------------------------------------------------
void CHLClient::IN_MouseEvent ( int mstate, bool down )
{
	input->MouseEvent( mstate, down );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLClient::IN_Accumulate ( void )
{
	input->AccumulateMouse();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLClient::IN_ClearStates ( void )
{
	input->ClearStates();
}

//-----------------------------------------------------------------------------
// Purpose: Engine can query for particular keys
// Input  : *name - 
// Output : struct kbutton_s
//-----------------------------------------------------------------------------
struct kbutton_s *CHLClient::IN_FindKey( const char *name )
{
	return input->FindKey( name );
}

//-----------------------------------------------------------------------------
// Purpose: Engine can issue a key event
// Input  : eventcode - 
//			keynum - 
//			*pszCurrentBinding - 
// Output : int
//-----------------------------------------------------------------------------
int CHLClient::IN_KeyEvent( int eventcode, int keynum, const char *pszCurrentBinding )
{
	return input->KeyEvent( eventcode, keynum, pszCurrentBinding );
}

void CHLClient::ExtraMouseSample( float frametime, int active )
{
	// FIXME: What should be happening here? Should we allow the partition to be queried here?
	// FIXME: We really need to be doing any work that's currently being done here
	// at the end of the frame (in PostRender)
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );
	C_BaseEntity::SetAbsQueriesValid( true );

	C_BaseAnimating::AllowBoneAccess( true, false );

	input->ExtraMouseSample( frametime, active );

	C_BaseAnimating::AllowBoneAccess( false, false );

	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );
	C_BaseEntity::SetAbsQueriesValid( false );
}

//-----------------------------------------------------------------------------
// Purpose: Fills in usercmd_s structure based on current view angles and key/controller inputs
// Input  : frametime - timestamp for last frame
//			*cmd - the command to fill in
//			active - whether the user is fully connected to a server
//-----------------------------------------------------------------------------
void CHLClient::CreateMove ( int command_number, int totalslots, float tick_frametime, float input_sample_frametime, int active, float packet_loss )
{
	// FIXME: What should be happening here? Should we allow the partition to be queried here?
	// FIXME: We really need to be doing any work that's currently being done here
	// at the end of the frame (in PostRender)
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );
	C_BaseEntity::SetAbsQueriesValid( true );

	C_BaseAnimating::AllowBoneAccess( true, false );

	input->CreateMove( command_number, totalslots, tick_frametime, input_sample_frametime, active, packet_loss );

	C_BaseAnimating::AllowBoneAccess( false, false );

	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );
	C_BaseEntity::SetAbsQueriesValid( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buf - 
//			from - 
//			to - 
//-----------------------------------------------------------------------------
void CHLClient::WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand )
{
	input->WriteUsercmdDeltaToBuffer( buf, from, to, isnewcommand );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			buffersize - 
//			slot - 
//-----------------------------------------------------------------------------
void CHLClient::EncodeUserCmdToBuffer( bf_write& buf, int buffersize, int slot )
{
	input->EncodeUserCmdToBuffer( buf, buffersize, slot );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : buf - 
//			buffersize - 
//			slot - 
//-----------------------------------------------------------------------------
void CHLClient::DecodeUserCmdFromBuffer( bf_read& buf, int buffersize, int slot )
{
	input->DecodeUserCmdFromBuffer( buf, buffersize, slot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLClient::View_Render( vrect_t *rect )
{
	VPROF( "View_Render" );
	// This is the main render entry point... 
	g_SmokeFogOverlayAlpha = 0;	// Reset the overlay alpha.

	view->Render( rect );

	// Draw an overlay to make it even harder to see inside smoke particle systems.
	g_SmokeFogOverlayColor = engine->GetLightForPoint(MainViewOrigin(), true);
	DrawSmokeFogOverlay();

//	Rope_ShowRSpeeds();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSF - 
//-----------------------------------------------------------------------------
void CHLClient::View_Fade( ScreenFade_t *pSF )
{
	vieweffects->Fade( NULL, 0, pSF );
}


//-----------------------------------------------------------------------------
// Purpose: Per level init
//-----------------------------------------------------------------------------
void CHLClient::LevelInitPreEntity( char const* pMapName )
{
	// HACK: Bogus, but the logic is too complicated in the engine
	if (g_bLevelInitialized)
		return;
	g_bLevelInitialized = true;

	// FIXME: Sucky, figure out logic for this crap
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

	vieweffects->LevelInit();
	
	// Tell mode manager that map is changing
	modemanager->LevelInit( pMapName );

	C_BaseTempEntity::ClearDynamicTempEnts();
	clienteffects->Flush();
	view->LevelInit();

	IGameSystem::LevelInitPreEntityAllSystems(pMapName);

	ResetWindspeed();

	// don't do prediction if single player!
	cl_predict.SetValue( engine->GetMaxClients() > 1 ? 1 : 0 );

	gHUD.LevelInit();

	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );
}


//-----------------------------------------------------------------------------
// Purpose: Per level init
//-----------------------------------------------------------------------------
void CHLClient::LevelInitPostEntity( )
{
	IGameSystem::LevelInitPostEntityAllSystems();
}


//-----------------------------------------------------------------------------
// Purpose: Per level de-init
//-----------------------------------------------------------------------------
void CHLClient::LevelShutdown( void )
{
	// HACK: Bogus, but the logic is too complicated in the engine
	if (!g_bLevelInitialized)
		return;
	g_bLevelInitialized = false;

	// Disable abs recomputations when everything is shutting down
	CBaseEntity::EnableAbsRecomputations( false );

	// Level shutdown sequence.
	// First do the pre-entity shutdown of all systems
	IGameSystem::LevelShutdownPreEntityAllSystems();

	modemanager->LevelShutdown();

	// Now release/delete the entities
	cl_entitylist->Release();

	// Now do the post-entity shutdown of all systems
	IGameSystem::LevelShutdownPostEntityAllSystems();

	tempents->LevelShutdown();
	g_ParticleMgr.RemoveAllEffects();

	gHUD.LevelShutdown();

	internalCenterPrint->Clear();
	messagechars->Clear();
	UncacheAllMaterials();
}


//-----------------------------------------------------------------------------
// Purpose: Engine can directly ask to render a view ( timerefresh and envmap creation, e.g. )
// Input  : &vs - 
//			drawViewmodel - 
//-----------------------------------------------------------------------------
void CHLClient::RenderView( const CViewSetup &vs, bool drawViewmodel )
{
	view->RenderView( vs, drawViewmodel );
}


//-----------------------------------------------------------------------------
// Purpose: Engine received crosshair offset ( autoaim )
// Input  : angle - 
//-----------------------------------------------------------------------------
void CHLClient::SetCrosshairAngle( const QAngle& angle )
{
	CHudCrosshair *crosshair = (CHudCrosshair *)GET_HUDELEMENT( CHudCrosshair );
	if ( crosshair )
	{
		crosshair->SetCrosshairAngle( angle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper to initialize sprite from .spr semaphor
// Input  : *pSprite - 
//			*loadname - 
//-----------------------------------------------------------------------------
void CHLClient::InitSprite( CEngineSprite *pSprite, const char *loadname )
{
	if ( pSprite )
	{
		pSprite->Init( loadname );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSprite - 
//-----------------------------------------------------------------------------
void CHLClient::ShutdownSprite( CEngineSprite *pSprite )
{
	if ( pSprite )
	{
		pSprite->Shutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tells engine how much space to allocate for sprite objects
// Output : int
//-----------------------------------------------------------------------------
int CHLClient::GetSpriteSize( void ) const
{
	return sizeof( CEngineSprite );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : entindex - 
//			bTalking - 
//-----------------------------------------------------------------------------
void CHLClient::VoiceStatus( int entindex, qboolean bTalking )
{
	GetClientVoiceMgr()->UpdateSpeakerStatus( entindex, !!bTalking );
}


//-----------------------------------------------------------------------------
// Called when the string table for materials changes
//-----------------------------------------------------------------------------
void OnMaterialStringTableChanged( void *object, TABLEID stringTable, int stringNumber, const char *newString, void const *newData )
{
	// Make sure this puppy is precached
	gHLClient.PrecacheMaterial( newString );
}

//-----------------------------------------------------------------------------
// Purpose: Hook up any callbacks here, the table definition has been parsed but 
//  no data has been added yet
//-----------------------------------------------------------------------------
void CHLClient::InstallStringTableCallback( const char *tableName )
{
	// Here, cache off string table IDs
	if (!Q_strcasecmp(tableName, "VguiScreen"))
	{
		// Look up the id 
		g_StringTableVguiScreen = networkstringtable->FindStringTable( tableName );
	}
	else if (!Q_strcasecmp(tableName, "Materials"))
	{
		// Look up the id 
		g_StringTableMaterials = networkstringtable->FindStringTable( tableName );

		// When the material list changes, we need to know immediately
		networkstringtable->SetStringChangedCallback( NULL, g_StringTableMaterials, OnMaterialStringTableChanged );
	}
	else if ( !Q_strcasecmp( tableName, "EffectDispatch" ) )
	{
		g_StringTableEffectDispatch = networkstringtable->FindStringTable( tableName );
	}
}


//-----------------------------------------------------------------------------
// Material precache
//-----------------------------------------------------------------------------
void CHLClient::PrecacheMaterial( const char *pMaterialName )
{
	bool bFound;
	IMaterial *pMaterial = materials->FindMaterial( pMaterialName, &bFound );
	if ( bFound && pMaterial )
	{
		pMaterial->IncrementReferenceCount();
		m_CachedMaterials.AddToTail( pMaterial );
	}
}

void CHLClient::UncacheAllMaterials( )
{
	for (int i = m_CachedMaterials.Count(); --i >= 0; )
	{
		m_CachedMaterials[i]->DecrementReferenceCount();
	}
	m_CachedMaterials.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_type - 
//			*name - 
//			size - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHLClient::GetUserMessageInfo( int msg_type, char *name, int& size )
{
	if ( !usermessages->IsValidIndex( msg_type ) )
		return false;

	Q_strcpy( name, usermessages->GetUserMessageName( msg_type ) );
	size = usermessages->GetUserMessageSize( msg_type );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHLClient::DispatchUserMessage( const char *pszName, int iSize, void *pbuf )
{
	return usermessages->DispatchUserMessage( pszName, iSize, pbuf );
}


//-----------------------------------------------------------------------------
// Converts precached material indices into strings
//-----------------------------------------------------------------------------
const char *GetMaterialNameFromIndex( int nIndex )
{
	if (nIndex != (networkstringtable->GetMaxStrings(g_StringTableMaterials) - 1))
	{
		return networkstringtable->GetString( g_StringTableMaterials, nIndex );
	}
	else
	{
		return NULL;
	}
}


void SimulateEntities()
{
	input->CAM_Think();

	// Service timer events (think functions).
  	ClientThinkList()->PerformThinkFunctions();

	// TODO: make an ISimulateable interface so C_BaseNetworkables can simulate?
	C_BaseEntityIterator iterator;
	C_BaseEntity *pEnt;
	while ( (pEnt = iterator.Next()) )
	{
		pEnt->Simulate();
	}
}


bool AddDataChangeEvent( IClientNetworkable *ent, DataUpdateType_t updateType, int *pStoredEvent )
{
	Assert( ent );
	// Make sure we don't already have an event queued for this guy.
	if ( *pStoredEvent >= 0 )
	{
		Assert( g_DataChangedEvents[*pStoredEvent].m_pEntity == ent );

		// DATA_UPDATE_CREATED always overrides DATA_UPDATE_CHANGED.
		if ( updateType == DATA_UPDATE_CREATED )
			g_DataChangedEvents[*pStoredEvent].m_UpdateType = updateType;
	
		return false;
	}
	else
	{
		*pStoredEvent = g_DataChangedEvents.AddToTail( CDataChangedEvent( ent, updateType, pStoredEvent ) );
		return true;
	}
}


void ClearDataChangedEvent( int iStoredEvent )
{
	if ( iStoredEvent != -1 )
		g_DataChangedEvents.Remove( iStoredEvent );
}


void ProcessOnDataChangedEvents()
{
	FOR_EACH_LL( g_DataChangedEvents, i )
	{
		CDataChangedEvent *pEvent = &g_DataChangedEvents[i];

		// Reset their stored event identifier.		
		*pEvent->m_pStoredEvent = -1;

		// Send the event.
		IClientNetworkable *pNetworkable = pEvent->m_pEntity;
		pNetworkable->OnDataChanged( pEvent->m_UpdateType );
	}

	g_DataChangedEvents.Purge();
}


void MarkAimEntsDirty()
{
	// Move any aiments to the appropriate origin
	C_BaseEntityIterator iterator;
	C_BaseEntity *pEnt;
	while ( (pEnt = iterator.Next()) )
	{
		pEnt->MoveAimEnt();
	}
}

void UpdateModelsAndAimEnts()
{
	VPROF_BUDGET( "UpdateModelsAndAimEnts", VPROF_BUDGETGROUP_OTHER_ANIMATION );
	// Move any aiments to the appropriate origin
	C_BaseEntityIterator iterator;
	C_BaseEntity *pEnt;
	while ( (pEnt = iterator.Next()) )
	{
		pEnt->UpdateClientSideAnimation();
		pEnt->Relink();
	}
}


void OnRenderStart()
{
	VPROF( "OnRenderStart" );

	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );
	C_BaseEntity::SetAbsQueriesValid( false );

	Rope_ResetCounters();

	// Interpolate server entities and move aiments.
	C_BaseEntity::InterpolateServerEntities();

	// Invalidate any bone information.
	C_BaseAnimating::InvalidateBoneCaches();

	C_BaseEntity::SetAbsQueriesValid( true );
	C_BaseEntity::EnableAbsRecomputations( true );

	// FIXME: This needs to be done before the player moves; it forces
	// aiments the player may be attached to to forcibly update their position
	MarkAimEntsDirty();

	// This will place the player + the view models + all parent
	// entities	at the correct abs position so that their attachment points
	// are at the correct location
	view->OnRenderStart();

	// This will place all entities in the correct position in world space and in the KD-tree
	UpdateModelsAndAimEnts();

	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );

	// Process OnDataChanged events.
	ProcessOnDataChangedEvents();

	// Simulate all the entities.
	SimulateEntities();
	PhysicsSimulate();

	// Update temp entities
	tempents->Update();

	// Update temp ent beams...
	beams->UpdateTempEntBeams();

	// Update particle effects (eventually, the effects should use Simulate() instead of having
	// their own update system).
	{
		VPROF_BUDGET( "g_ParticleMgr.Update", VPROF_BUDGETGROUP_PARTICLE_RENDERING );
		g_ParticleMgr.Update( gpGlobals->frametime );
	}

	// This creates things like temp entities.
	engine->FireEvents();

	// Finally, link all the entities into the leaf system right before rendering.
	C_BaseEntity::AddVisibleEntities();
}


void OnRenderEnd()
{
	// Disallow access to bones (access is enabled in CViewRender::SetUpView).
	C_BaseAnimating::AllowBoneAccess( false, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OnNetUpdateStart()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OnNetUpdatePostDataUpdateStart()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OnNetUpdatePostDataUpdateEnd()
{
	// Let prediction copy off pristine data
	prediction->PostEntityPacketReceived();
}

void CHLClient::FrameStageNotify( ClientFrameStage_t curStage )
{
	g_CurFrameStage = curStage;

	switch( curStage )
	{
	default:
		break;

	case FRAME_RENDER_START:
		{
			VPROF( "CHLClient::FrameStageNotify FRAME_RENDER_START" );
			// Last thing before rendering, run simulation.
			OnRenderStart();
		}
		break;
		
	case FRAME_RENDER_END:
		{
			VPROF( "CHLClient::FrameStageNotify FRAME_RENDER_END" );
			OnRenderEnd();
		}
		break;
		
	case FRAME_NET_UPDATE_START:
		{
			VPROF( "CHLClient::FrameStageNotify FRAME_NET_UPDATE_START" );
			C_BaseEntity::EnableAbsRecomputations( false );
			C_BaseEntity::SetAbsQueriesValid( false );
			partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, true );

			OnNetUpdateStart();
		}
		break;
	case FRAME_NET_UPDATE_END:
		{
		}
		break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_START:
		{
			VPROF( "CHLClient::FrameStageNotify FRAME_NET_UPDATE_POSTDATAUPDATE_START" );
			OnNetUpdatePostDataUpdateStart();
		}
		break;
	case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
		{
			VPROF( "CHLClient::FrameStageNotify FRAME_NET_UPDATE_POSTDATAUPDATE_END" );
			OnNetUpdatePostDataUpdateEnd();
		}
		break;
	case FRAME_START:
		{
			CVProfPanel *pVProfPanel = ( CVProfPanel * )GET_HUDELEMENT( CVProfPanel );
			pVProfPanel->UpdateProfile();
		}
		break;
	}
}

CSaveRestoreData *SaveInit( int size );

// Save/restore system hooks
CSaveRestoreData  *CHLClient::SaveInit( int size )
{
	return ::SaveInit(size);
}

void CHLClient::SaveWriteFields( CSaveRestoreData *pSaveData, const char *pname, void *pBaseData, datamap_t *pMap, typedescription_t *pFields, int fieldCount )
{
	CSave saveHelper( pSaveData );
	saveHelper.WriteFields( pname, pBaseData, pMap, pFields, fieldCount );
}

void CHLClient::SaveReadFields( CSaveRestoreData *pSaveData, const char *pname, void *pBaseData, datamap_t *pMap, typedescription_t *pFields, int fieldCount )
{
	CRestore restoreHelper( pSaveData );
	restoreHelper.ReadFields( pname, pBaseData, pMap, pFields, fieldCount );
}

void CHLClient::PreSave( CSaveRestoreData *s )
{
	g_pGameSaveRestoreBlockSet->PreSave( s );
}

void CHLClient::Save( CSaveRestoreData *s )
{
	CSave saveHelper( s );
	g_pGameSaveRestoreBlockSet->Save( &saveHelper );
}

void CHLClient::WriteSaveHeaders( CSaveRestoreData *s )
{
	CSave saveHelper( s );
	g_pGameSaveRestoreBlockSet->WriteSaveHeaders( &saveHelper );
	g_pGameSaveRestoreBlockSet->PostSave();
}

void CHLClient::ReadRestoreHeaders( CSaveRestoreData *s )
{
	CRestore restoreHelper( s );
	g_pGameSaveRestoreBlockSet->PreRestore();
	g_pGameSaveRestoreBlockSet->ReadRestoreHeaders( &restoreHelper );
}

void CHLClient::Restore( CSaveRestoreData *s, bool b )
{
	CRestore restore(s);
	g_pGameSaveRestoreBlockSet->Restore( &restore, b );
	g_pGameSaveRestoreBlockSet->PostRestore();
}
