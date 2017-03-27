//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Precaches and defs for entities and other data that must always be available.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "soundent.h"
#include "client.h"
#include "decals.h"
#include "EnvMessage.h"
#include "player.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "physics.h"
#include "isaverestore.h"
#include "activitylist.h"
#include "eventqueue.h"
#include "ai_network.h"
#include "ai_schedule.h"
#include "ai_networkmanager.h"
#include "basetempentity.h"
#include "measure_section.h"
#include "world.h"
#include "mempool.h"
#include "igamesystem.h"
#include "engine/IEngineSound.h"
#include "globals.h"

extern CBaseEntity				*g_pLastSpawn;
void InitBodyQue(void);
extern void W_Precache(void);
extern void ActivityList_Free( void );
extern CMemoryPool g_EntityListPool;
extern ConVar game_speeds;
ConVar	host_speeds( "host_speeds","0" );
//

/*
==============================================================================

BODY QUE

==============================================================================
*/

#define SF_DECAL_NOTINDEATHMATCH		2048

class CDecal : public CPointEntity
{
public:
	DECLARE_CLASS( CDecal, CPointEntity );

	void	Spawn( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );

	// Need to apply static decals here to get them into the signon buffer for the server appropriately
	virtual void Activate();

	void	TriggerDecal( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// Input handlers.
	void	InputActivate( inputdata_t &inputdata );

	DECLARE_DATADESC();

public:
	int		m_nTexture;

private:

	void	StaticDecal( void );
};

BEGIN_DATADESC( CDecal )

	DEFINE_FIELD( CDecal, m_nTexture, FIELD_INTEGER ),

	// Function pointers
	DEFINE_FUNCTION( CDecal, StaticDecal ),
	DEFINE_FUNCTION( CDecal, TriggerDecal ),

	DEFINE_INPUTFUNC( CDecal, FIELD_VOID, "Activate", InputActivate ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( infodecal, CDecal );

// UNDONE:  These won't get sent to joining players in multi-player
void CDecal :: Spawn( void )
{
	if ( m_nTexture < 0 || 
		(gpGlobals->deathmatch && HasSpawnFlags( SF_DECAL_NOTINDEATHMATCH )) )
	{
		UTIL_Remove( this );
		return;
	} 
}

void CDecal::Activate()
{
	BaseClass::Activate();

	if ( !GetEntityName() )
	{
		StaticDecal();
	}
	else
	{
		// if there IS a targetname, the decal sprays itself on when it is triggered.
		SetThink ( &CDecal::SUB_DoNothing );
		SetUse(&CDecal::TriggerDecal);
	}
}

void CDecal::TriggerDecal ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// this is set up as a USE function for info_decals that have targetnames, so that the
	// decal doesn't get applied until it is fired. (usually by a scripted sequence)
	trace_t		trace;
	int			entityIndex;

	UTIL_TraceLine( GetAbsOrigin() - Vector(5,5,5), GetAbsOrigin() + Vector(5,5,5),  MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );

	entityIndex = trace.m_pEnt ? trace.m_pEnt->entindex() : 0;

	CBroadcastRecipientFilter filter;

	te->BSPDecal( filter, 0.0, 
		&GetAbsOrigin(), entityIndex, m_nTexture );

	SetThink( &CDecal::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


void CDecal::InputActivate( inputdata_t &inputdata )
{
	TriggerDecal( inputdata.pActivator, inputdata.pCaller, USE_ON, 0 );
}


void CDecal :: StaticDecal( void )
{
	trace_t trace;
	int entityIndex, modelIndex = 0;

	Vector position = GetAbsOrigin();
	UTIL_TraceLine( position - Vector(5,5,5), position + Vector(5,5,5),  MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );

	entityIndex = (short)trace.m_pEnt ? trace.m_pEnt->entindex() : 0;
	if ( entityIndex )
	{
		CBaseEntity *ent = trace.m_pEnt;
		if ( ent )
		{
			modelIndex = ent->GetModelIndex();
			VectorITransform( GetAbsOrigin(), ent->EntityToWorldTransform(), position );
		}
	}

	engine->StaticDecal( position, m_nTexture, entityIndex, modelIndex );

	// CRecipientFilter initFilter;
	// initFilter.MakeInitMessage();
	// TE_BSPDecal( initFilter, GetAbsOrigin(), entityIndex, (int)pev->skin );

	SUB_Remove();
}


bool CDecal::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "texture"))
	{
		// FIXME:  should decals all be preloaded?
		m_nTexture = UTIL_PrecacheDecal( szValue, true );
		
		// Found
		if (m_nTexture >= 0 )
			return true;
		Warning( "Can't find decal %s\n", szValue );
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================
LINK_ENTITY_TO_CLASS( worldspawn, CWorld );

BEGIN_DATADESC( CWorld )

	DEFINE_FIELD( CWorld, m_flWaveHeight, FIELD_FLOAT ),

	// keyvalues are parsed from map, but not saved/loaded
	DEFINE_KEYFIELD( CWorld, m_iszChapterTitle, FIELD_STRING, "chaptertitle" ),
	DEFINE_KEYFIELD( CWorld, m_bStartDark,		FIELD_BOOLEAN, "startdark" ),
	DEFINE_KEYFIELD( CWorld, m_bDisplayTitle,	FIELD_BOOLEAN, "gametitle" ),
	DEFINE_FIELD( CWorld, m_WorldMins, FIELD_VECTOR ),
	DEFINE_FIELD( CWorld, m_WorldMaxs, FIELD_VECTOR ),

END_DATADESC()


// SendTable stuff.
IMPLEMENT_SERVERCLASS_ST(CWorld, DT_WORLD)
	SendPropFloat	(SENDINFO(m_flWaveHeight),		8,	SPROP_ROUNDUP,	0.0f,	8.0f),
	SendPropVector	(SENDINFO(m_WorldMins),			-1,	SPROP_COORD),
	SendPropVector	(SENDINFO(m_WorldMaxs),			-1,	SPROP_COORD),
END_SEND_TABLE()

int CWorld::Save( ISave &save )
{
	if ( !BaseClass::Save(save) )
		return 0;

	// save out the message queue
	return g_EventQueue.Save( save );
}


int CWorld::Restore( IRestore &restore )
{
	if ( !BaseClass::Restore(restore) )
		return 0;

	// restore the event queue
	return g_EventQueue.Restore( restore );
}

//
// Just to ignore the "wad" field.
//
bool CWorld::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq(szKeyName, "skyname") )
	{
		// Sent over net now.
		ConVar *skyname = ( ConVar * )cvar->FindVar( "sv_skyname" );
		if ( skyname )
		{
			skyname->SetValue( szValue );
		}
	}
	else if ( FStrEq(szKeyName, "sounds") )
	{
		gpGlobals->cdAudioTrack = atoi(szValue);
	}
	else if ( FStrEq(szKeyName, "WaveHeight") )
	{
		// Sent over net now.
		m_flWaveHeight = atof(szValue) * (1.0/8.0);

		ConVar *wateramp = ( ConVar * )cvar->FindVar( "sv_wateramp" );
		if ( wateramp )
		{
			wateramp->SetValue( m_flWaveHeight );
		}
	}
	else if ( FStrEq(szKeyName, "newunit") )
	{
		// Single player only.  Clear save directory if set
		if ( atoi(szValue) )
		{
			extern void Game_SetOneWayTransition();
			Game_SetOneWayTransition();
		}
	}
	else if ( FStrEq(szKeyName, "world_mins") )
	{
		Vector vec;
		sscanf(	szValue, "%f %f %f", &vec.x, &vec.y, &vec.z );
		m_WorldMins = vec;
	}
	else if ( FStrEq(szKeyName, "world_maxs") )
	{
		Vector vec;
		sscanf(	szValue, "%f %f %f", &vec.x, &vec.y, &vec.z ); 
		m_WorldMaxs = vec;
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}


extern bool		g_fGameOver;
int g_iWeaponCheat; 

CWorld::CWorld( )
{
	CBaseEntity::AttachEdict( INDEXENT(RequiredEdictIndex()) );
	SetNextThink( gpGlobals->curtime + 0.05 );
	ActivityList_Init();
	NetworkStateManualMode( true );
	
	SetSolid( SOLID_BSP );
	SetMoveType( MOVETYPE_NONE );
}

CWorld::~CWorld( )
{
	ActivityList_Free();
	if ( g_pGameRules )
	{
		g_pGameRules->LevelShutdown();
	}
}

void CWorld::AttachEdict( edict_t *pRequiredEdict )
{
	// edict should already be attached by constructor

	// make sure classname is set
	pev->classname = m_iClassname;
}

//------------------------------------------------------------------------------
// Purpose : Add a decal to the world
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWorld::DecalTrace( trace_t *pTrace, char const *decalName)
{
	int index = decalsystem->GetDecalIndexForName( decalName );
	if ( index < 0 )
		return;

	CBroadcastRecipientFilter filter;

	te->WorldDecal( filter, 0.0, &pTrace->endpos, index );
}

void CWorld::RegisterSharedActivities( void )
{
	ActivityList_RegisterSharedActivities();
}


void CWorld::Spawn( void )
{
	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );
	// NOTE:  SHOULD NEVER BE ANYTHING OTHER THAN 1!!!
	SetModelIndex( 1 );
	// world model
	SetModelName( MAKE_STRING( modelinfo->GetModelName( GetModel() ) ) );
	AddFlag( FL_WORLDBRUSH );

	g_EventQueue.Init();
	Precache( );
}

// called after spawn, and after savegame load
void CWorld::Activate( void )
{
	BaseClass::Activate();

	ConVar const *cheats = cvar->FindVar( "sv_cheats" );

	// Is the impulse 101 command allowed?
	g_iWeaponCheat = cheats ? cheats->GetInt() : 0;
}

static CWorld *g_WorldEntity = NULL;

CWorld* GetWorldEntity()
{
	return g_WorldEntity;
}

void CWorld::Precache( void )
{
	g_WorldEntity = this;
	g_fGameOver = false;
	g_pLastSpawn = NULL;

	ConVar *gravity = ( ConVar * )cvar->FindVar( "sv_gravity" );
	if ( gravity )
	{
// UNDONE: Disabled this.  This makes sv_gravity reset of each level.  Is that what we want?
//		gravity->SetValue( 800 );// 67ft/sec^2
	}

	ConVar *stepsize = ( ConVar * )cvar->FindVar( "sv_stepsize" );
	if ( stepsize )
	{
		stepsize->SetValue( 18 );
	}

	ConVar *roomtype = ( ConVar * )cvar->FindVar( "room_type" );
	if ( roomtype )
	{
		roomtype->SetValue( 0 );
	}

	// Set up game rules
	Assert( !g_pGameRules );
	if (g_pGameRules)
	{
		delete g_pGameRules;
	}

	g_pGameRules = InstallGameRules( );
	Assert( g_pGameRules );
	g_pGameRules->LevelInit();

	CSoundEnt::InitSoundEnt();

	IGameSystem::LevelInitPreEntityAllSystems( STRING( GetModelName() ) );

	// UNDONE: Make most of these things server systems or precache_registers
	// =================================================
	//	Activities
	// =================================================
	ActivityList_Free();
	RegisterSharedActivities();

	InitBodyQue();
// init sentence group playback stuff from sentences.txt.
// ok to call this multiple times, calls after first are ignored.

	SENTENCEG_Init();

// the area based ambient sounds MUST be the first precache_sounds

// player precaches     
	W_Precache ();									// get weapon precaches
	ClientPrecache();
	g_pGameRules->Precache();
	// precache all temp ent stuff
	CBaseTempEntity::PrecacheTempEnts();

	// sounds used from C physics code
	enginesound->PrecacheSound("common/null.wav");				// clears sound channels

	ConVar const *language = cvar->FindVar( "sv_language" );

	g_Language = language ? language->GetInt() : LANGUAGE_ENGLISH;
	if ( g_Language == LANGUAGE_GERMAN )
	{
		engine->PrecacheModel( "models/germangibs.mdl" );
	}
	else
	{
		engine->PrecacheModel( "models/gibs/hgibs.mdl" );
	}

//
// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
//
	// 0 normal
	engine->LightStyle(0, "m");
	
	// 1 FLICKER (first variety)
	engine->LightStyle(1, "mmnmmommommnonmmonqnmmo");
	
	// 2 SLOW STRONG PULSE
	engine->LightStyle(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	
	// 3 CANDLE (first variety)
	engine->LightStyle(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	
	// 4 FAST STROBE
	engine->LightStyle(4, "mamamamamama");
	
	// 5 GENTLE PULSE 1
	engine->LightStyle(5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	
	// 6 FLICKER (second variety)
	engine->LightStyle(6, "nmonqnmomnmomomno");
	
	// 7 CANDLE (second variety)
	engine->LightStyle(7, "mmmaaaabcdefgmmmmaaaammmaamm");
	
	// 8 CANDLE (third variety)
	engine->LightStyle(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	
	// 9 SLOW STROBE (fourth variety)
	engine->LightStyle(9, "aaaaaaaazzzzzzzz");
	
	// 10 FLUORESCENT FLICKER
	engine->LightStyle(10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	engine->LightStyle(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");
	
	// 12 UNDERWATER LIGHT MUTATION
	// this light only distorts the lightmap - no contribution
	// is made to the brightness of affected surfaces
	engine->LightStyle(12, "mmnnmmnnnmmnn");
	
	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	engine->LightStyle(63, "a");

	// =================================================
	//	Load and Init AI Networks
	// =================================================
	CAI_NetworkManager::InitializeAINetworks();
	// =================================================
	//	Load and Init AI Schedules
	// =================================================
	g_AI_SchedulesManager.LoadAllSchedules();
	// =================================================
	//	Initialize NPC Relationships
	// =================================================
	g_pGameRules->InitDefaultAIRelationships();
	CBaseCombatCharacter::InitInteractionSystem();

	// Call all registered precachers.
	CPrecacheRegister::Precache();	

	if ( m_iszChapterTitle != NULL_STRING )
	{
		DevMsg( 2, "Chapter title: %s\n", STRING(m_iszChapterTitle) );
		CMessage *pMessage = (CMessage *)CBaseEntity::Create( "env_message", vec3_origin, vec3_angle, NULL );
		if ( pMessage )
		{
			pMessage->SetMessage( m_iszChapterTitle );
			m_iszChapterTitle = NULL_STRING;

			// send the message entity a play message command, delayed by 0.3 seconds
			pMessage->AddSpawnFlags( SF_MESSAGE_ONCE );
			pMessage->SetThink( &CMessage::SUB_CallUseToggle );
			pMessage->SetNextThink( gpGlobals->curtime + 0.3 );
		}
	}
}

double GetRealTime()
{
	return engine->Time();
}

//-----------------------------------------------------------------------------
// Purpose: Draw output overlays for any measure sections
// Input  : 
//-----------------------------------------------------------------------------
#include "ndebugoverlay.h"
void DrawMeasuredSections(void)
{
	int		row = 1;
	float	rowheight = 0.025;

	CMeasureSection *p = CMeasureSection::GetList();
	while ( p )
	{
		char str[256];
		Q_snprintf(str,sizeof(str),"%s",p->GetName());
		NDebugOverlay::ScreenText( 0.01,0.51+(row*rowheight),str, 255,255,255,255, 0.0 );
		
		Q_snprintf(str,sizeof(str),"%5.2f\n",p->GetTime().GetMillisecondsF());
		//Q_snprintf(str,sizeof(str),"%3.3f\n",p->GetTime().GetSeconds() * 100.0 / engine->Time());
		NDebugOverlay::ScreenText( 0.28,0.51+(row*rowheight),str, 255,255,255,255, 0.0 );

		Q_snprintf(str,sizeof(str),"%5.2f\n",p->GetMaxTime().GetMillisecondsF());
		//Q_snprintf(str,sizeof(str),"%3.3f\n",p->GetTime().GetSeconds() * 100.0 / engine->Time());
		NDebugOverlay::ScreenText( 0.34,0.51+(row*rowheight),str, 255,255,255,255, 0.0 );


		row++;

		p = p->GetNext();
	}

	bool sort_reset = false;

	// Time to redo sort?
	if ( measure_resort.GetFloat() > 0.0 &&
		engine->Time() >= CMeasureSection::m_dNextResort )
	{
		// Redo it
		CMeasureSection::SortSections();
		// Set next time
		CMeasureSection::m_dNextResort = engine->Time() + measure_resort.GetFloat();
		// Flag to reset sort accumulator, too
		sort_reset = true;
	}

	// Iterate through the sections now
	p = CMeasureSection::GetList();
	while ( p )
	{
		// Update max 
		p->UpdateMax();

		// Reset regular accum.
		p->Reset();
		// Reset sort accum less often
		if ( sort_reset )
		{
			p->SortReset();
		}
		p = p->GetNext();
	}

}

void CWorld::Think( void )
{
// Uncomment this is you want to see the world bbox displayed on the client
//
//	NDebugOverlay::Box(GetOrigin(), m_WorldMins, m_WorldMaxs, 0, 255, 0, 0, 0.1);
	Assert( GetAbsVelocity() == vec3_origin );
	Assert( GetLocalAngularVelocity() == vec3_angle );

	if (game_speeds.GetInt())
	{
		DrawMeasuredSections();
	}

	SetNextThink( gpGlobals->curtime + 0.05 );
}

bool CWorld::GetDisplayTitle() const
{
	return m_bDisplayTitle;
}

bool CWorld::GetStartDark() const
{
	return m_bStartDark;
}

void CWorld::SetDisplayTitle( bool display )
{
	m_bDisplayTitle = display;
}

void CWorld::SetStartDark( bool startdark )
{
	m_bStartDark = startdark;
}

