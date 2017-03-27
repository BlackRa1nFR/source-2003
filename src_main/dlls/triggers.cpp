//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Spawn and use functions for editor-placed triggers.
//
//=============================================================================

#include "cbase.h"
#include "ai_basenpc.h"
#include "player.h"
#include "saverestore.h"
#include "gamerules.h"
#include "entityapi.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "globalstate.h"
#include "filters.h"
#include "vstdlib/random.h"
#include "triggers.h"
#include "saverestoretypes.h"
#include "hierarchy.h"
#include "bspfile.h"
#include "saverestore_utlvector.h"
#include "physics_saverestore.h"
#include "te_effect_dispatch.h"
#include "ammodef.h"
#include "iservervehicle.h"

#if _DEBUG
//#define DEBUG_LEVELTRANSITION 1
#endif

// Global list of triggers that care about weapon fire
// Doesn't need saving, the triggers re-add themselves on restore.
CUtlVector< CHandle<CTriggerMultiple> >	g_hWeaponFireTriggers;

//
// Spawnflags
//
const int SF_TRIGGER_ALLOW_CLIENTS		= 0x01;	// Players can fire this trigger
const int SF_TRIGGER_ALLOW_NPCS			= 0x02;	// NPCS can fire this trigger
const int SF_TRIGGER_ALLOW_PUSHABLES	= 0x04;	// Pushables can fire this trigger
const int SF_TRIGGER_ALLOW_PHYSICS		= 0x08;	// Physics objects can fire this trigger
const int SF_TRIG_PUSH_ONCE				= 0x80; // trigger_push removes itself after firing once


extern bool		g_fGameOver;
ConVar	showtriggers( "showtriggers","0");


// Global Savedata for base trigger
BEGIN_DATADESC( CBaseTrigger )

	// Keyfields
	DEFINE_KEYFIELD( CBaseTrigger, m_iFilterName,	FIELD_STRING,	"filtername" ),
	DEFINE_FIELD( CBaseTrigger, m_hFilter,	FIELD_EHANDLE ),
	DEFINE_KEYFIELD( CBaseTrigger, m_bDisabled,		FIELD_BOOLEAN,	"StartDisabled" ),
	DEFINE_UTLVECTOR( CBaseTrigger, m_hTouchingEntities, FIELD_EHANDLE ),

	// Inputs	
	DEFINE_INPUTFUNC( CBaseTrigger, FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( CBaseTrigger, FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( CBaseTrigger, FIELD_VOID, "Toggle", InputToggle ),

	// Outputs
	DEFINE_OUTPUT( CBaseTrigger, m_OnStartTouch, "OnStartTouch"),
	DEFINE_OUTPUT( CBaseTrigger, m_OnEndTouch, "OnEndTouch"),
	DEFINE_OUTPUT( CBaseTrigger, m_OnEndTouchAll, "OnEndTouchAll"),

END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger, CBaseTrigger );


//------------------------------------------------------------------------------
// Purpose: Input handler to turn on this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::InputEnable( inputdata_t &inputdata )
{ 
	Enable();
}


//------------------------------------------------------------------------------
// Purpose: Input handler to turn off this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::InputDisable( inputdata_t &inputdata )
{ 
	Disable();
}


//------------------------------------------------------------------------------
// Purpose: Turns on this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::Enable( void )
{ 
	if (!IsSolidFlagSet( FSOLID_TRIGGER ))
	{
		AddSolidFlags( FSOLID_TRIGGER ); 
		engine->RelinkEntity( edict(), true );
	}
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CBaseTrigger::Activate( void ) 
{ 
	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName, NULL ));
	}
	BaseClass::Activate();
}


//------------------------------------------------------------------------------
// Purpose: Turns off this trigger.
//------------------------------------------------------------------------------
void CBaseTrigger::Disable( void )
{ 
	if (IsSolidFlagSet(FSOLID_TRIGGER))
	{
		RemoveSolidFlags( FSOLID_TRIGGER ); 
		engine->RelinkEntity( edict(), true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CBaseTrigger::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print Target
		// --------------
		char tempstr[255];
		if (IsSolidFlagSet(FSOLID_TRIGGER)) 
		{
			Q_strncpy(tempstr,"State: Enabled",sizeof(tempstr));
		}
		else
		{
			Q_strncpy(tempstr,"State: Disabled",sizeof(tempstr));
		}
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseTrigger::InitTrigger( )
{
	SetSolid( GetParent() ? SOLID_VPHYSICS : SOLID_BSP );	
	AddSolidFlags( FSOLID_NOT_SOLID );
	if (m_bDisabled)
	{
		RemoveSolidFlags( FSOLID_TRIGGER );	
	}
	else
	{
		AddSolidFlags( FSOLID_TRIGGER );	
	}

	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );    // set size and link into world
	if ( showtriggers.GetInt() == 0 )
	{
		SETBITS( m_fEffects, EF_NODRAW );
	}

	m_hTouchingEntities.Purge();

	Relink();
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if this entity passes the filter criterea, false if not.
// Input  : pOther - The entity to be filtered.
//-----------------------------------------------------------------------------
bool CBaseTrigger::PassesTriggerFilters(CBaseEntity *pOther)
{
	// First test spawn flag filters
	if ((HasSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS) && (pOther->GetFlags() & FL_CLIENT)) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_NPCS) && (pOther->GetFlags() & FL_NPC)) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_PUSHABLES) && FClassnameIs(pOther, "func_pushable")) ||
		(HasSpawnFlags(SF_TRIGGER_ALLOW_PHYSICS) && pOther->GetMoveType() == MOVETYPE_VPHYSICS))
	{
		CBaseFilter *pFilter = m_hFilter.Get();
		return (!pFilter) ? true : pFilter->PassesFilter(pOther);
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Called when an entity starts touching us.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::StartTouch(CBaseEntity *pOther)
{
	if (PassesTriggerFilters(pOther))
	{
		EHANDLE hOther;
		hOther = pOther;

		m_hTouchingEntities.AddToTail( hOther );
		m_OnStartTouch.FireOutput(pOther, this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when an entity stops touching us.
// Input  : pOther - The entity that was touching us.
//-----------------------------------------------------------------------------
void CBaseTrigger::EndTouch(CBaseEntity *pOther)
{
	if ( IsTouching( pOther ) )
	{
		EHANDLE hOther;
		hOther = pOther;
		m_hTouchingEntities.FindAndRemove( hOther );
		m_OnEndTouch.FireOutput(pOther, this);

		// If there are no more entities touching this trigger, fire the lost all touches
		// Loop through the touching entities backwards. Clean out old ones, and look for existing
		bool bFoundOtherTouchee = false;
		int iSize = m_hTouchingEntities.Count();
		for ( int i = iSize-1; i >= 0; i-- )
		{
			EHANDLE hOther;
			hOther = m_hTouchingEntities[i];

			if ( !hOther )
			{
				m_hTouchingEntities.Remove( i );
			}
			else
			{
				bFoundOtherTouchee = true;
			}
		}

		// Didn't find one?
		if ( !bFoundOtherTouchee )
		{
			m_OnEndTouchAll.FireOutput(pOther, this);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified entity is touching us
//-----------------------------------------------------------------------------
bool CBaseTrigger::IsTouching( CBaseEntity *pOther )
{
	EHANDLE hOther;
	hOther = pOther;
	return ( m_hTouchingEntities.Find( hOther ) != m_hTouchingEntities.InvalidIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: Return a pointer to the first entity of the specified type being touched by this trigger
//-----------------------------------------------------------------------------
CBaseEntity *CBaseTrigger::GetTouchedEntityOfType( const char *sClassName )
{
	int iCount = m_hTouchingEntities.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CBaseEntity *pEntity = m_hTouchingEntities[i];
		if ( FClassnameIs( pEntity, sClassName ) )
			return pEntity;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Toggles this trigger between enabled and disabled.
//-----------------------------------------------------------------------------
void CBaseTrigger::InputToggle( inputdata_t &inputdata )
{
	if (IsSolidFlagSet( FSOLID_TRIGGER ))
	{
		RemoveSolidFlags(FSOLID_TRIGGER);
	}
	else
	{
		AddSolidFlags(FSOLID_TRIGGER);
	}

	engine->RelinkEntity( edict(), true );
}


//-----------------------------------------------------------------------------
// Purpose: Hurts anything that touches it. If the trigger has a targetname,
//			firing it will toggle state.
//-----------------------------------------------------------------------------
class CTriggerHurt : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerHurt, CBaseTrigger );

	void Spawn( void );
	void RadiationThink( void );
	void HurtThink( void );
	void Touch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );
	bool HurtEntity( CBaseEntity *pOther, float damage );
	int HurtAllTouchers( float dt );
	
	DECLARE_DATADESC();
	
	float	m_flDamage;			// Damage per second.
	float	m_flLastDmgTime;	// Time that we last applied damage.
	int		m_bitsDamageInflict;	// DMG_ damage type that the door or tigger does

	// Outputs
	COutputEvent m_OnHurt;
	COutputEvent m_OnHurtPlayer;

	CUtlVector<EHANDLE>	m_hurtEntities;
};

BEGIN_DATADESC( CTriggerHurt )

	// Function Pointers
	DEFINE_FUNCTION( CTriggerHurt, RadiationThink ),
	DEFINE_FUNCTION( CTriggerHurt, HurtThink ),

	// Fields
	DEFINE_KEYFIELD( CTriggerHurt, m_flDamage, FIELD_FLOAT, "damage" ),
	DEFINE_KEYFIELD( CTriggerHurt, m_bitsDamageInflict, FIELD_INTEGER, "damagetype" ),

	DEFINE_FIELD( CTriggerHurt, m_flLastDmgTime, FIELD_TIME ),
	DEFINE_UTLVECTOR( CTriggerHurt, m_hurtEntities, FIELD_EHANDLE ),

	// Inputs
	DEFINE_INPUT( CTriggerHurt, m_flDamage, FIELD_FLOAT, "SetDamage" ),

	// Outputs
	DEFINE_OUTPUT( CTriggerHurt, m_OnHurt, "OnHurt" ),
	DEFINE_OUTPUT( CTriggerHurt, m_OnHurtPlayer, "OnHurtPlayer" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger_hurt, CTriggerHurt );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerHurt::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
	if (m_bitsDamageInflict & DMG_RADIATION)
	{
		SetThink ( &CTriggerHurt::RadiationThink );
		SetNextThink( gpGlobals->curtime + random->RandomFloat(0.0, 0.5) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Trigger hurt that causes radiation will do a radius check and set
//			the player's geiger counter level according to distance from center
//			of trigger.
//-----------------------------------------------------------------------------
void CTriggerHurt::RadiationThink( void )
{

	edict_t *pentPlayer;
	CBasePlayer *pPlayer = NULL;
	float flRange;
	edict_t *pevTarget;
	Vector vecSpot1;
	Vector vecSpot2;
	Vector vecRange;
	Vector origin;
	Vector view_ofs;

	// check to see if a player is in pvs
	// if not, continue	

	// set origin to center of trigger so that this check works
	origin = GetLocalOrigin();
	view_ofs = GetViewOffset();

	SetLocalOrigin( (GetAbsMins() + GetAbsMaxs()) * 0.5 );
	SetViewOffset( vec3_origin );

	pentPlayer = UTIL_FindClientInPVS(edict());

	SetLocalOrigin( origin );
	SetViewOffset( view_ofs );

	// reset origin

	if (pentPlayer)
	{
 
		pPlayer = (CBasePlayer *)CBaseEntity::Instance( pentPlayer );

		pevTarget = pentPlayer;

		// get range to player;

		vecSpot1 = (GetAbsMins() + GetAbsMaxs()) * 0.5;
		vecSpot2 = (pPlayer->GetAbsMins() + pPlayer->GetAbsMaxs()) * 0.5;
		
		vecRange = vecSpot1 - vecSpot2;
		flRange = vecRange.Length();

		pPlayer->NotifyNearbyRadiationSource(flRange);
	}

	float dt = gpGlobals->curtime - m_flLastDmgTime;
	if ( dt >= 0.5 )
	{
		HurtAllTouchers( dt );
	}
	SetNextThink( gpGlobals->curtime + 0.25 );
}


//-----------------------------------------------------------------------------
// Purpose: When touched, a hurt trigger does m_flDamage points of damage each half-second.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
bool CTriggerHurt::HurtEntity( CBaseEntity *pOther, float damage )
{
	if ( !pOther->m_takedamage || !PassesTriggerFilters(pOther) )
		return false;

	if ( damage < 0 )
	{
		pOther->TakeHealth( -damage, m_bitsDamageInflict );
	}
	else
	{
		CTakeDamageInfo info( this, this, damage, m_bitsDamageInflict );
		GuessDamageForce( &info, (pOther->GetAbsOrigin() - GetAbsOrigin()), pOther->GetAbsOrigin() );
		pOther->TakeDamage( info );
	}

	if (pOther->IsPlayer())
	{
		m_OnHurtPlayer.FireOutput(pOther, this);
	}
	else
	{
		m_OnHurt.FireOutput(pOther, this);
	}
	m_hurtEntities.AddToTail( EHANDLE(pOther) );
	//NDebugOverlay::Box( pOther->GetAbsOrigin(), pOther->WorldAlignMins(), pOther->WorldAlignMaxs(), 255,0,0,0,0.5 );
	return true;
}

void CTriggerHurt::HurtThink()
{
	// if I hurt anyone, think again
	if ( HurtAllTouchers( 0.5 ) <= 0 )
	{
		SetThink(NULL);
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
}

void CTriggerHurt::EndTouch( CBaseEntity *pOther )
{
	if (PassesTriggerFilters(pOther))
	{
		EHANDLE hOther;
		hOther = pOther;

		// if this guy has never taken damage, hurt him now
		if ( !m_hurtEntities.HasElement( hOther ) )
		{
			HurtEntity( pOther, m_flDamage * 0.5 );
		}
	}
	BaseClass::EndTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: called from RadiationThink() as well as HurtThink()
//			This function applies damage to any entities currently touching the
//			trigger
// Input  : dt - time since last call
// Output : int - number of entities actually hurt
//-----------------------------------------------------------------------------
int CTriggerHurt::HurtAllTouchers( float dt )
{
	int hurtCount = 0;
	// half second worth of damage
	float fldmg = m_flDamage * dt;
	m_flLastDmgTime = gpGlobals->curtime;

	m_hurtEntities.RemoveAll();
	for ( touchlink_t *link = m_EntitiesTouched.nextLink; link != &m_EntitiesTouched; link = link->nextLink )
	{
		CBaseEntity *pTouch = link->entityTouched;
		if ( pTouch )
		{
			if ( HurtEntity( pTouch, fldmg ) )
			{
				hurtCount++;
			}
		}
	}

	return hurtCount;
}

void CTriggerHurt::Touch( CBaseEntity *pOther )
{
	if ( m_pfnThink == NULL )
	{
		SetThink( &CTriggerHurt::HurtThink );
		SetNextThink( gpGlobals->curtime );
	}
}


// ##################################################################################
//	>> TriggerMultiple
// ##################################################################################
LINK_ENTITY_TO_CLASS( trigger_multiple, CTriggerMultiple );


BEGIN_DATADESC( CTriggerMultiple )

	// Function Pointers
	DEFINE_FUNCTION(CTriggerMultiple, MultiTouch),
	DEFINE_FUNCTION(CTriggerMultiple, MultiWaitOver ),

	// Outputs
	DEFINE_OUTPUT(CTriggerMultiple, m_OnTrigger, "OnTrigger")

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerMultiple::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	if (m_flWait == 0)
	{
		m_flWait = 0.2;
	}

	ASSERTSZ(m_iHealth == 0, "trigger_multiple with health");
	SetTouch( &CTriggerMultiple::MultiTouch );
}


//-----------------------------------------------------------------------------
// Purpose: Touch function. Activates the trigger.
// Input  : pOther - The thing that touched us.
//-----------------------------------------------------------------------------
void CTriggerMultiple::MultiTouch(CBaseEntity *pOther)
{
	if (PassesTriggerFilters(pOther))
	{
		ActivateMultiTrigger( pOther );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pActivator - 
//-----------------------------------------------------------------------------
void CTriggerMultiple::ActivateMultiTrigger(CBaseEntity *pActivator)
{
	if (GetNextThink() > gpGlobals->curtime)
		return;         // still waiting for reset time

	m_hActivator = pActivator;

	m_OnTrigger.FireOutput(m_hActivator, this);

	if (m_flWait > 0)
	{
		SetThink( &CTriggerMultiple::MultiWaitOver );
		SetNextThink( gpGlobals->curtime + m_flWait );
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch( NULL );
		SetNextThink( gpGlobals->curtime + 0.1f );
		SetThink(  &CTriggerMultiple::SUB_Remove );
	}
}


//-----------------------------------------------------------------------------
// Purpose: The wait time has passed, so set back up for another activation
//-----------------------------------------------------------------------------
void CTriggerMultiple::MultiWaitOver( void )
{
	SetThink( NULL );
}

// ##################################################################################
//	>> TriggerOnce
// ##################################################################################
class CTriggerOnce : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerOnce, CTriggerMultiple );
public:

	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_once, CTriggerOnce );

void CTriggerOnce::Spawn( void )
{
	BaseClass::Spawn();

	m_flWait = -1;
}


// ##################################################################################
//	>> TriggerLook
//
//  Triggers once when player is looking at m_target
//
// ##################################################################################
#define SF_TRIGGERLOOK_FIREONCE		128
#define SF_TRIGGERLOOK_USEVELOCITY	256

class CTriggerLook : public CTriggerOnce
{
	DECLARE_CLASS( CTriggerLook, CTriggerOnce );
public:

	EHANDLE	m_hLookTarget;
	float	m_flFieldOfView;
	float   m_flLookTime;			// How long must I look for
	float   m_flLookTimeTotal;		// How long have I looked
	float   m_flLookTimeLast;		// When did I last look

	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );
	int	 DrawDebugTextOverlays(void);

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( trigger_look, CTriggerLook );
BEGIN_DATADESC( CTriggerLook )

	DEFINE_FIELD( CTriggerLook, m_hLookTarget,		FIELD_EHANDLE),
	DEFINE_FIELD( CTriggerLook, m_flLookTimeTotal,	FIELD_FLOAT),
	DEFINE_FIELD( CTriggerLook, m_flLookTimeLast,	FIELD_TIME),

	// Inputs
	DEFINE_INPUT( CTriggerLook, m_flFieldOfView,		FIELD_FLOAT,	"FieldOfView" ),
	DEFINE_INPUT( CTriggerLook, m_flLookTime,			FIELD_FLOAT,	"LookTime" ),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CTriggerLook::Spawn( void )
{
	m_hLookTarget		= NULL;
	m_flLookTimeTotal	= -1;
	BaseClass::Spawn();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CTriggerLook::EndTouch(CBaseEntity *pOther)
{
	m_flLookTimeTotal	= -1;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CTriggerLook::Touch(CBaseEntity *pOther)
{
	// --------------------------------
	// Make sure we have a look target
	// --------------------------------
	if (m_hLookTarget == NULL)
	{
		m_hLookTarget = GetNextTarget();
		if (m_hLookTarget == NULL)
		{
			return;
		}
	}

	// This is designed for single player only
	// so we'll always have the same player
	if (pOther->IsPlayer())
	{
		// ----------------------------------------
		// Check that toucher is facing the target
		// ----------------------------------------
		Vector vLookDir;
		if ( HasSpawnFlags( SF_TRIGGERLOOK_USEVELOCITY ) )
		{
			vLookDir = pOther->GetAbsVelocity();
			if ( vLookDir == vec3_origin )
			{
				// See if they're in a vehicle
				CBasePlayer *pPlayer = (CBasePlayer *)pOther;
				if ( pPlayer->IsInAVehicle() )
				{
					vLookDir = pPlayer->GetVehicle()->GetVehicleEnt()->GetSmoothedVelocity();
				}
			}
			VectorNormalize( vLookDir );
		}
		else
		{
			vLookDir = ((CBaseCombatCharacter*)pOther)->EyeDirection3D( );
		}

		Vector vTargetDir = m_hLookTarget->GetAbsOrigin() - pOther->EyePosition();
		VectorNormalize(vTargetDir);

		float fDotPr = DotProduct(vLookDir,vTargetDir);
		if (fDotPr > m_flFieldOfView)
		{
			// Is it the first time I'm looking?
			if (m_flLookTimeTotal == -1)
			{
				m_flLookTimeLast	= gpGlobals->curtime;
				m_flLookTimeTotal	= 0;
			}
			else
			{
				m_flLookTimeTotal	+= gpGlobals->curtime - m_flLookTimeLast;
				m_flLookTimeLast	=  gpGlobals->curtime;
			}

			if (m_flLookTimeTotal >= m_flLookTime)
			{
				m_OnTrigger.FireOutput(pOther, this);
				m_flLookTimeTotal = -1;

				if ( HasSpawnFlags( SF_TRIGGERLOOK_FIREONCE ) )
				{
					SetThink( &CTriggerLook::SUB_Remove );
					SetNextThink( gpGlobals->curtime );
				}
			}
		}
		else
		{
			m_flLookTimeTotal	= -1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTriggerLook::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// ----------------
		// Print Look time
		// ----------------
		char tempstr[255];
		Q_snprintf(tempstr,sizeof(tempstr),"Time:   %3.2f",m_flLookTime - max(0,m_flLookTimeTotal));
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

// ##################################################################################
//	>> TriggerVolume
// ##################################################################################
class CTriggerVolume : public CPointEntity	// Derive from point entity so this doesn't move across levels
{
public:
	DECLARE_CLASS( CTriggerVolume, CPointEntity );

	void		Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_transition, CTriggerVolume );

// Define space that travels across a level transition
void CTriggerVolume::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );    // set size and link into world
	SetModelName( NULL_STRING );
	SetModelIndex( 0 );
	Relink();
}

#define SF_CHANGELEVEL_NOTOUCH		0x0002
#define cchMapNameMost 32

enum
{
	TRANSITION_VOLUME_SCREENED_OUT = 0,
	TRANSITION_VOLUME_NOT_FOUND = 1,
	TRANSITION_VOLUME_PASSED = 2,
};


class CChangeLevel : public CBaseTrigger
{
public:
	DECLARE_CLASS( CChangeLevel, CBaseTrigger );

	void Spawn( void );
	void Activate( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	void TouchChangeLevel( CBaseEntity *pOther );
	void ChangeLevelNow( CBaseEntity *pActivator );

	void InputChangeLevel( inputdata_t &inputdata );

	static CBaseEntity *FindLandmark( const char *pLandmarkName );
	static int ChangeList( levellist_t *pLevelList, int maxList );
	static int AddTransitionToList( levellist_t *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark );
	static int InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName );

	DECLARE_DATADESC();

	char m_szMapName[cchMapNameMost];		// trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost];		// trigger_changelevel only:  landmark on next map
	float	m_touchTime;

	// Outputs
	COutputEvent m_OnChangeLevel;
};
LINK_ENTITY_TO_CLASS( trigger_changelevel, CChangeLevel );

// Global Savedata for changelevel trigger
BEGIN_DATADESC( CChangeLevel )

	DEFINE_ARRAY( CChangeLevel, m_szMapName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_ARRAY( CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, cchMapNameMost ),
//	DEFINE_FIELD( CChangeLevel, m_touchTime, FIELD_TIME ),	// don't save

	// Function Pointers
	DEFINE_FUNCTION( CChangeLevel, TouchChangeLevel ),

	DEFINE_INPUTFUNC( CChangeLevel, FIELD_VOID, "ChangeLevel", InputChangeLevel ),

	// Outputs
	DEFINE_OUTPUT( CChangeLevel, m_OnChangeLevel, "OnChangeLevel"),

END_DATADESC()


//
// Cache user-entity-field values until spawn is called.
//

bool CChangeLevel::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "map"))
	{
		if (strlen(szValue) >= cchMapNameMost)
		{
			Warning( "Map name '%s' too long (32 chars)\n", szValue );
			Assert(0);
		}
		Q_strncpy(m_szMapName, szValue, sizeof(m_szMapName));
	}
	else if (FStrEq(szKeyName, "landmark"))
	{
		if (strlen(szValue) >= cchMapNameMost)
		{
			Warning( "Landmark name '%s' too long (32 chars)\n", szValue );
			Assert(0);
		}
		
		Q_strncpy(m_szLandmarkName, szValue, sizeof( m_szLandmarkName ));
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}



void CChangeLevel::Spawn( void )
{
	if ( FStrEq( m_szMapName, "" ) )
	{
		Msg( "a trigger_changelevel doesn't have a map" );
	}

	if ( FStrEq( m_szLandmarkName, "" ) )
	{
		Msg( "trigger_changelevel to %s doesn't have a landmark", m_szMapName );
	}

	InitTrigger();
	if ( !HasSpawnFlags(SF_CHANGELEVEL_NOTOUCH) )
	{
		SetTouch( &CChangeLevel::TouchChangeLevel );
	}

//	Msg( "TRANSITION: %s (%s)\n", m_szMapName, m_szLandmarkName );
}

void CChangeLevel::Activate( void )
{
	BaseClass::Activate();

	// Level transitions will bust if they are in solid
	CBaseEntity *pLandmark = FindLandmark( m_szLandmarkName );
	if ( pLandmark )
	{
		int clusterIndex = engine->GetClusterForOrigin( pLandmark->GetAbsOrigin() );
		if ( clusterIndex < 0 )
		{
			Warning( "trigger_changelevel to map %s has a landmark embedded in solid!\n"
					"This will break level transitions!\n", m_szMapName ); 
		}
	}
}


static char st_szNextMap[cchMapNameMost];
static char st_szNextSpot[cchMapNameMost];

CBaseEntity *CChangeLevel::FindLandmark( const char *pLandmarkName )
{
	CBaseEntity *pentLandmark;

	pentLandmark = gEntList.FindEntityByName( NULL, pLandmarkName, NULL );
	while ( pentLandmark )
	{
		// Found the landmark
		if ( FClassnameIs( pentLandmark, "info_landmark" ) )
			return pentLandmark;
		else
			pentLandmark = gEntList.FindEntityByName( pentLandmark, pLandmarkName, NULL );
	}
	Warning( "Can't find landmark %s\n", pLandmarkName );
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Allows level transitions to be triggered by buttons, etc.
//-----------------------------------------------------------------------------
void CChangeLevel::InputChangeLevel( inputdata_t &inputdata )
{
	ChangeLevelNow( inputdata.pActivator );
}


//-----------------------------------------------------------------------------
// Purpose: Performs the level change and fires targets.
// Input  : pActivator - 
//-----------------------------------------------------------------------------
void CChangeLevel::ChangeLevelNow( CBaseEntity *pActivator )
{
	CBaseEntity	*pLandmark;
	levellist_t	levels[16];

	Assert(!FStrEq(m_szMapName, ""));

	// Don't work in deathmatch
	if ( g_pGameRules->IsDeathmatch() )
		return;

	// Some people are firing these multiple times in a frame, disable
	if ( gpGlobals->curtime == m_touchTime )
		return;

	m_touchTime = gpGlobals->curtime;


	CBaseEntity *pPlayer = (pActivator && pActivator->IsPlayer()) ? pActivator : UTIL_PlayerByIndex(1);
	int transitionState = InTransitionVolume(pPlayer, m_szLandmarkName);
	if ( transitionState == TRANSITION_VOLUME_SCREENED_OUT )
	{
		DevMsg( 2, "Player isn't in the transition volume %s, aborting\n", m_szLandmarkName );
		return;
	}

	// look for a landmark entity		
	pLandmark = FindLandmark( m_szLandmarkName );

	if ( !pLandmark )
		return;

	// no transition volumes, check PVS of landmark
	if ( transitionState == TRANSITION_VOLUME_NOT_FOUND )
	{
		byte pvs[MAX_MAP_CLUSTERS/8];
		int clusterIndex = engine->GetClusterForOrigin( pLandmark->GetAbsOrigin() );
		engine->GetPVSForCluster( clusterIndex, sizeof(pvs), pvs );
		if ( pPlayer )
		{
			bool playerInPVS = engine->CheckBoxInPVS( pPlayer->GetAbsMins(), pPlayer->GetAbsMaxs(), pvs );
			//Assert( playerInPVS );

			if ( !playerInPVS )
			{
				Warning( "Player isn't in the landmark's (%s) PVS, aborting\n", m_szLandmarkName );
#ifndef HL1_DLL
				// HL1 works even with these errors!
				return;
#endif
			}
		}
	}

	st_szNextSpot[0] = 0;	// Init landmark to NULL
	Q_strncpy(st_szNextSpot, m_szLandmarkName,sizeof(st_szNextSpot));
	// This object will get removed in the call to engine->ChangeLevel, copy the params into "safe" memory
	Q_strncpy(st_szNextMap, m_szMapName, sizeof(st_szNextMap));

	m_hActivator = pActivator;

	m_OnChangeLevel.FireOutput(pActivator, this);


////	Msg( "Level touches %d levels\n", ChangeList( levels, 16 ) );
#if DEBUG_LEVELTRANSITION
	Msg( "CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot );
#endif
	engine->ChangeLevel( st_szNextMap, st_szNextSpot );
}

//
// GLOBALS ASSUMED SET:  st_szNextMap
//
void CChangeLevel::TouchChangeLevel( CBaseEntity *pOther )
{
	CBasePlayer *pPlayer = ToBasePlayer(pOther);
	if ( !pPlayer )
		return;

	if ( !pPlayer->IsInAVehicle() && pPlayer->GetMoveType() == MOVETYPE_NOCLIP )
	{
		DevMsg("In level transition: %s %s\n", st_szNextMap, st_szNextSpot );
		return;
	}

	ChangeLevelNow( pOther );
}


// Add a transition to the list, but ignore duplicates 
// (a designer may have placed multiple trigger_changelevels with the same landmark)
int CChangeLevel::AddTransitionToList( levellist_t *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark )
{
	int i;

	if ( !pLevelList || !pMapName || !pLandmarkName || !pentLandmark )
		return 0;

	for ( i = 0; i < listCount; i++ )
	{
		if ( pLevelList[i].pentLandmark == pentLandmark && stricmp( pLevelList[i].mapName, pMapName ) == 0 )
			return 0;
	}
	Q_strncpy( pLevelList[listCount].mapName, pMapName, sizeof(pLevelList[listCount].mapName) );
	Q_strncpy( pLevelList[listCount].landmarkName, pLandmarkName, sizeof(pLevelList[listCount].landmarkName) );
	pLevelList[listCount].pentLandmark = pentLandmark;

	CBaseEntity *ent = CBaseEntity::Instance( pentLandmark );
	Assert( ent );

	pLevelList[listCount].vecLandmarkOrigin = ent->GetAbsOrigin();

	return 1;
}

int BuildChangeList( levellist_t *pLevelList, int maxList )
{
	return CChangeLevel::ChangeList( pLevelList, maxList );
}


int CChangeLevel::InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName )
{
	CBaseEntity *pVolume;

	if ( pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION )
		return TRANSITION_VOLUME_PASSED;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	pEntity = GetHighestParent( pEntity );

	int inVolume = TRANSITION_VOLUME_NOT_FOUND;	// Unless we find a trigger_transition, everything is in the volume

	pVolume = gEntList.FindEntityByName( NULL, pVolumeName, NULL );
	while ( pVolume )
	{
		if ( pVolume && FClassnameIs( pVolume, "trigger_transition" ) )
		{
			if ( pVolume->Intersects( pEntity ) )	// It touches one, it's in the volume
				return TRANSITION_VOLUME_PASSED;
			else
				inVolume = TRANSITION_VOLUME_SCREENED_OUT;	// Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
		pVolume = gEntList.FindEntityByName( pVolume, pVolumeName, NULL );
	}

	return inVolume;
}

// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512
// This has grown into a complicated beast
// Can we make this more elegant?
// This builds the list of all transitions on this level and which entities are in their PVS's and can / should
// be moved across.
int CChangeLevel::ChangeList( levellist_t *pLevelList, int maxList )
{
	CBaseEntity *pentChangelevel, *pentLandmark;
	int	i, count;

	count = 0;

	// Find all of the possible level changes on this BSP
	pentChangelevel = gEntList.FindEntityByClassname( NULL, "trigger_changelevel" );
	if ( !pentChangelevel )
		return 0;
	while ( pentChangelevel )
	{
		CChangeLevel *pTrigger;
		
		pTrigger = dynamic_cast<CChangeLevel *> (pentChangelevel);
		if ( pTrigger )
		{
			// Find the corresponding landmark
			pentLandmark = FindLandmark( pTrigger->m_szLandmarkName );
			if ( pentLandmark )
			{
				// Build a list of unique transitions
				if ( AddTransitionToList( pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark->pev ) )
				{
					count++;
					if ( count >= maxList )		// FULL!!
						break;
				}
			}
		}
		pentChangelevel = gEntList.FindEntityByClassname( pentChangelevel, "trigger_changelevel" );
	}

	if ( gpGlobals->pSaveData && ((CSaveRestoreData *)gpGlobals->pSaveData)->NumEntities() )
	{
		CSave saveHelper( (CSaveRestoreData *)gpGlobals->pSaveData );

		for ( i = 0; i < count; i++ )
		{
			int j, iEntity = 0;
			CBaseEntity *pEntList[ MAX_ENTITY ];
			int			 entityFlags[ MAX_ENTITY ];

			// Follow the linked list of entities in the PVS of the transition landmark
			CBaseEntity *pEntity = NULL; 
			CBaseEntity *pLandmarkEntity = CBaseEntity::Instance( pLevelList[i].pentLandmark );
			while ( (pEntity = UTIL_EntitiesInPVS( pLandmarkEntity, pEntity)) != NULL )
			{
#if DEBUG_LEVELTRANSITION
				Msg( "Trying %s\n", pEntity->GetClassname() );
#endif
				int caps = pEntity->ObjectCaps();
				if ( !(caps & FCAP_DONT_SAVE) )
				{
					int flags = 0;

					// If this entity can be moved or is global, mark it
					if ( caps & FCAP_ACROSS_TRANSITION )
					{
						flags |= FENTTABLE_MOVEABLE;
					}
					if ( pEntity->m_iGlobalname != NULL_STRING && !pEntity->IsDormant() )
					{
						flags |= FENTTABLE_GLOBAL;
					}

					if ( flags )
					{
						if ( iEntity >= MAX_ENTITY )
						{
							Warning( "Too many entities across a transition!" );
							Assert( 0 );
							break;
						}

						pEntList[ iEntity ] = pEntity;
						entityFlags[ iEntity ] = flags;
						iEntity++;
					}
#if DEBUG_LEVELTRANSITION
					else
						Msg( "Failed %s\n", pEntity->GetClassname() );
#endif
				}
#if DEBUG_LEVELTRANSITION
				else
					Msg( "DON'T SAVE %s\n", pEntity->GetClassname() );
#endif
			}

			for ( j = 0; j < iEntity; j++ )
			{
				// Check to make sure the entity isn't screened out by a trigger_transition
				if ( entityFlags[j] && InTransitionVolume( pEntList[j], pLevelList[i].landmarkName ) )
				{
					// Mark entity table with 1<<i
					int index = saveHelper.EntityIndex( pEntList[j] );
					// Flag it with the level number
					saveHelper.EntityFlagsSet( index, entityFlags[j] | (1<<i) );
				}
#if DEBUG_LEVELTRANSITION
				else
					Msg( "Screened out %s\n", pEntList[j]->GetClassname() );
#endif
			}
		}
	}

	return count;
}


//-----------------------------------------------------------------------------
// Purpose: A trigger that pushes the player, NPCs, or objects.
//-----------------------------------------------------------------------------
class CTriggerPush : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerPush, CBaseTrigger );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void Untouch( CBaseEntity *pOther );

	Vector m_vecPushDir;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CTriggerPush )
	DEFINE_KEYFIELD( CTriggerPush, m_vecPushDir, FIELD_VECTOR, "pushdir" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_push, CTriggerPush );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerPush::Spawn( )
{
	// Convert pushdir from angles to a vector
	QAngle angPushDir = QAngle(m_vecPushDir.x, m_vecPushDir.y, m_vecPushDir.z);
	AngleVectors(angPushDir, &m_vecPushDir);

	Assert( !GetMoveParent() );
	if ( GetMoveParent() )
	{
		Warning("trigger_push cannot be hierarchically attached !!!\n");
	}
	BaseClass::Spawn();

	
	InitTrigger();

	if (m_flSpeed == 0)
	{
		m_flSpeed = 100;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CTriggerPush::Touch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || (pOther->GetMoveType() == MOVETYPE_PUSH || pOther->GetMoveType() == MOVETYPE_NONE ) )
		return;

	if (!PassesTriggerFilters(pOther))
		return;

	// FIXME: If something is hierarchically attached, should we try to push the parent?
	if (pOther->GetMoveParent())
		return;

	// Instant trigger, just transfer velocity and remove
	if (HasSpawnFlags(SF_TRIG_PUSH_ONCE))
	{
		pOther->ApplyAbsVelocityImpulse( m_flSpeed * m_vecPushDir );

		if ( pOther->GetAbsVelocity().z > 0 )
			pOther->RemoveFlag( FL_ONGROUND );
		UTIL_Remove( this );
		return;
	}

	switch( pOther->GetMoveType() )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
		break;

	case MOVETYPE_VPHYSICS:
		{
			IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
			if ( pPhys )
			{
				// UNDONE: Assume the velocity is for a 100kg object, scale with mass
				pPhys->ApplyForceCenter( m_flSpeed * m_vecPushDir * 100.0f * gpGlobals->frametime );
				return;
			}
		}
		break;

	default:
		{
			Vector vecPush = (m_flSpeed * m_vecPushDir);
			if ( pOther->GetFlags() & FL_BASEVELOCITY )
			{
				vecPush = vecPush + pOther->GetBaseVelocity();
			}
			if ( vecPush.z > 0 && (pOther->GetFlags() & FL_ONGROUND) )
			{
				pOther->RemoveFlag( FL_ONGROUND );
				Vector origin = pOther->GetLocalOrigin();
				origin.z += 1.0f;
				pOther->SetLocalOrigin( origin );
			}

			pOther->SetBaseVelocity( vecPush );
			pOther->AddFlag( FL_BASEVELOCITY );
		}
		break;
	}
}



class CTriggerTeleport : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerTeleport, CBaseTrigger );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );

	string_t m_iLandmark;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( trigger_teleport, CTriggerTeleport );

BEGIN_DATADESC( CTriggerTeleport )

	DEFINE_KEYFIELD( CTriggerTeleport, m_iLandmark, FIELD_STRING, "landmark" ),

END_DATADESC()



void CTriggerTeleport::Spawn( void )
{
	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: Teleports the entity that touched us to the location of our target,
//			setting the toucher's angles to our target's angles if they are a
//			player.
//
//			If a landmark was specified, the toucher is offset from the target
//			by their initial offset from the landmark and their angles are
//			left alone.
//
// Input  : pOther - The entity that touched us.
//-----------------------------------------------------------------------------
void CTriggerTeleport::Touch( CBaseEntity *pOther )
{
	CBaseEntity	*pentTarget = NULL;

	if (!PassesTriggerFilters(pOther))
	{
		return;
	}

	pentTarget = gEntList.FindEntityByName( pentTarget, m_target, pOther );
	if (!pentTarget)
	{
	   return;
	}
	
	//
	// If a landmark was specified, offset the player relative to the landmark.
	//
	CBaseEntity	*pentLandmark = NULL;
	Vector vecLandmarkOffset(0, 0, 0);
	if (m_iLandmark != NULL_STRING)
	{
		pentLandmark = gEntList.FindEntityByName(pentLandmark, m_iLandmark, pOther );
		if (pentLandmark)
		{
			vecLandmarkOffset = pOther->GetAbsOrigin() - pentLandmark->GetAbsOrigin();
		}
	}

	pOther->RemoveFlag( FL_ONGROUND );
	
	Vector tmp = pentTarget->GetAbsOrigin();

	if (!pentLandmark && pOther->IsPlayer())
	{
		// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
		tmp.z -= pOther->WorldAlignMins().z;
	}

	//
	// Only modify the toucher's angles and zero their velocity if no landmark was specified.
	//
	const QAngle *pAngles = NULL;
	Vector *pVelocity = NULL;

	if (!pentLandmark)
	{
		pAngles = &pentTarget->GetAbsAngles();
		pVelocity = NULL;
	}

	tmp += vecLandmarkOffset;
	pOther->Teleport( &tmp, pAngles, pVelocity );
}


LINK_ENTITY_TO_CLASS( info_teleport_destination, CPointEntity );


//-----------------------------------------------------------------------------
// Purpose: Saves the game when the player touches the trigger.
//-----------------------------------------------------------------------------
class CTriggerSave : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerSave, CBaseTrigger );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_autosave, CTriggerSave );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been set.
//-----------------------------------------------------------------------------
void CTriggerSave::Spawn( void )
{
	if ( g_pGameRules->IsDeathmatch() )
	{
		UTIL_Remove( this );
		return;
	}

	InitTrigger();
}


//-----------------------------------------------------------------------------
// Purpose: Performs the autosave when the player touches us.
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CTriggerSave::Touch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsPlayer() )
		return;
    
	UTIL_Remove( this );
	engine->ServerCommand( "autosave\n" );
}


class CTriggerGravity : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerGravity, CBaseTrigger );
	DECLARE_DATADESC();

	void Spawn( void );
	void GravityTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_gravity, CTriggerGravity );

BEGIN_DATADESC( CTriggerGravity )

	// Function Pointers
	DEFINE_FUNCTION(CTriggerGravity, GravityTouch),

END_DATADESC()

void CTriggerGravity::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
	SetTouch( &CTriggerGravity::GravityTouch );
}

void CTriggerGravity::GravityTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsPlayer() )
		return;

	pOther->SetGravity( GetGravity() );
}


// this is a really bad idea.
class CAI_ChangeTarget : public CBaseEntity
{
public:
	DECLARE_CLASS( CAI_ChangeTarget, CBaseEntity );

	// Input handlers.
	void InputActivate( inputdata_t &inputdata );

	int ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	DECLARE_DATADESC();

private:
	string_t	m_iszNewTarget;
};
LINK_ENTITY_TO_CLASS( ai_changetarget, CAI_ChangeTarget );

BEGIN_DATADESC( CAI_ChangeTarget )

	DEFINE_KEYFIELD( CAI_ChangeTarget, m_iszNewTarget, FIELD_STRING, "m_iszNewTarget" ),

	// Inputs
	DEFINE_INPUTFUNC( CAI_ChangeTarget, FIELD_VOID, "Activate", InputActivate ),

END_DATADESC()


void CAI_ChangeTarget::InputActivate( inputdata_t &inputdata )
{
	CBaseEntity *pTarget = NULL;

	while ((pTarget = gEntList.FindEntityByName( pTarget, m_target, inputdata.pActivator )) != NULL)
	{
		pTarget->m_target = m_iszNewTarget;
		CAI_BaseNPC *pNPC = pTarget->MyNPCPointer( );
		if (pNPC)
		{
			pNPC->m_pGoalEnt = NULL;
		}
	}
}








//-----------------------------------------------------------------------------
// Purpose: Change an NPC's hint group to something new
//-----------------------------------------------------------------------------
class CAI_ChangeHintGroup : public CBaseEntity
{
public:
	DECLARE_CLASS( CAI_ChangeHintGroup, CBaseEntity );

	int ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	// Input handlers.
	void InputActivate( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	CAI_BaseNPC *FindQualifiedNPC( CAI_BaseNPC *pPrev );

	int			m_iSearchType;
	string_t	m_strSearchName;
	string_t	m_strNewHintGroup;
	float		m_flRadius;
	bool		m_bHintGroupNavLimiting;
};
LINK_ENTITY_TO_CLASS( ai_changehintgroup, CAI_ChangeHintGroup );

BEGIN_DATADESC( CAI_ChangeHintGroup )

	DEFINE_KEYFIELD( CAI_ChangeHintGroup, m_iSearchType, FIELD_INTEGER, "SearchType" ),
	DEFINE_KEYFIELD( CAI_ChangeHintGroup, m_strSearchName, FIELD_STRING, "SearchName" ),
	DEFINE_KEYFIELD( CAI_ChangeHintGroup, m_strNewHintGroup, FIELD_STRING, "NewHintGroup" ),
	DEFINE_KEYFIELD( CAI_ChangeHintGroup, m_flRadius, FIELD_FLOAT, "Radius" ),
	DEFINE_KEYFIELD( CAI_ChangeHintGroup, m_bHintGroupNavLimiting,	FIELD_BOOLEAN, "hintlimiting" ),

	DEFINE_INPUTFUNC( CAI_ChangeHintGroup, FIELD_VOID, "Activate", InputActivate ),

END_DATADESC()

CAI_BaseNPC *CAI_ChangeHintGroup::FindQualifiedNPC( CAI_BaseNPC *pPrev )
{
	CBaseEntity *pEntity = pPrev;
	CAI_BaseNPC *pResult = NULL;
	const char *pszSearchName = STRING(m_strSearchName);
	while ( !pResult )
	{
		// Find a candidate
		switch ( m_iSearchType )
		{
			case 0:
			{
				pEntity = gEntList.FindEntityByNameWithin( pEntity, pszSearchName, GetLocalOrigin(), m_flRadius );
				break;
			}
			
			case 1:
			{
				pEntity = gEntList.FindEntityByClassnameWithin( pEntity, pszSearchName, GetLocalOrigin(), m_flRadius );
				break;
			}

			case 2:
			{
				pEntity = gEntList.FindEntityInSphere( pEntity, GetLocalOrigin(), ( m_flRadius != 0.0 ) ? m_flRadius : FLT_MAX );
				break;
			}
		}

		if ( !pEntity )
			return NULL;

		// Qualify
		pResult = pEntity->MyNPCPointer();
		if ( pResult && m_iSearchType == 2 && (!FStrEq( STRING(pResult->GetHintGroup()), pszSearchName ) ) )
		{
			pResult = NULL;
		}
	}

	return pResult;
}

void CAI_ChangeHintGroup::InputActivate( inputdata_t &inputdata )
{
	CAI_BaseNPC *pTarget = NULL;

	while((pTarget = FindQualifiedNPC( pTarget )) != NULL)
	{
		pTarget->SetHintGroup( m_strNewHintGroup, m_bHintGroupNavLimiting );
	}
}




#define SF_CAMERA_PLAYER_POSITION	1
#define SF_CAMERA_PLAYER_TARGET		2
#define SF_CAMERA_PLAYER_TAKECONTROL 4

class CTriggerCamera : public CBaseEntity
{
public:
	DECLARE_CLASS( CTriggerCamera, CBaseEntity );

	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void FollowTarget( void );
	void Move(void);

	// Always transmit to clients so they know where to move the view to
	virtual bool ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
	{
		return true;
	}

	DECLARE_DATADESC();

	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;
	CBaseEntity *m_pPath;
	string_t m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int	  m_state;
	Vector m_vecMoveDir;

private:
	COutputEvent m_OnEndFollow;
};
LINK_ENTITY_TO_CLASS( trigger_camera, CTriggerCamera );

BEGIN_DATADESC( CTriggerCamera )

	DEFINE_FIELD( CTriggerCamera, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_pPath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTriggerCamera, m_sPath, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCamera, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_flReturnTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_flStopTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_moveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_targetSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_initialSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_acceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_deceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_state, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerCamera, m_vecMoveDir, FIELD_VECTOR ),

	// Function Pointers
	DEFINE_FUNCTION( CTriggerCamera, FollowTarget ),
	DEFINE_OUTPUT( CTriggerCamera, m_OnEndFollow, "OnEndFollow" ),

END_DATADESC()


void CTriggerCamera::Spawn( void )
{
	BaseClass::Spawn();

	SetMoveType( MOVETYPE_NOCLIP );
	SetSolid( SOLID_NONE );								// Remove model & collisions
	SetRenderColorA( 0 );								// The engine won't draw this model if this is set to 0 and blending is on
	m_nRenderMode = kRenderTransTexture;

	m_initialSpeed = m_flSpeed;
	if ( m_acceleration == 0 )
		m_acceleration = 500;
	if ( m_deceleration == 0 )
		m_deceleration = 500;
	Relink();

	// set manual mode on networking because these ents never change
	NetworkStateManualMode( true );
}


bool CTriggerCamera::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "wait"))
	{
		m_flWait = atof(szValue);
	}
	else if (FStrEq(szKeyName, "moveto"))
	{
		m_sPath = AllocPooledString( szValue );
	}
	else if (FStrEq(szKeyName, "acceleration"))
	{
		m_acceleration = atof( szValue );
	}
	else if (FStrEq(szKeyName, "deceleration"))
	{
		m_deceleration = atof( szValue );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}



void CTriggerCamera::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_state ) )
		return;

	// Toggle state
	m_state = !m_state;
	if (m_state == 0)
	{
		m_flReturnTime = gpGlobals->curtime;
		return;
	}
	if ( !pActivator || !pActivator->IsPlayer() )
	{
		pActivator = CBaseEntity::Instance(engine->PEntityOfEntIndex( 1 ));
	}
		
	m_hPlayer = pActivator;

	m_flReturnTime = gpGlobals->curtime + m_flWait;
	m_flSpeed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	if (HasSpawnFlags(SF_CAMERA_PLAYER_TARGET ) )
	{
		m_hTarget = m_hPlayer;
	}
	else
	{
		m_hTarget = GetNextTarget();
	}

	// Nothing to look at!
	if ( m_hTarget == NULL )
	{
		return;
	}


	if (HasSpawnFlags(SF_CAMERA_PLAYER_TAKECONTROL ) )
	{
		((CBasePlayer *)pActivator)->EnableControl(FALSE);
	}

	if ( m_sPath != NULL_STRING )
	{
		m_pPath = gEntList.FindEntityByName( NULL, m_sPath, pActivator );
	}
	else
	{
		m_pPath = NULL;
	}

	m_flStopTime = gpGlobals->curtime;
	if ( m_pPath )
	{
		if ( m_pPath->m_flSpeed != 0 )
			m_targetSpeed = m_pPath->m_flSpeed;
		
		m_flStopTime += m_pPath->GetDelay();
	}

	// copy over player information
	if (HasSpawnFlags(SF_CAMERA_PLAYER_POSITION ) )
	{
		UTIL_SetOrigin( this, pActivator->EyePosition() );
		SetLocalAngles( QAngle( pActivator->GetLocalAngles().x,
			pActivator->GetLocalAngles().y, 0 ) );
		SetAbsVelocity( pActivator->GetAbsVelocity() );
	}
	else
	{
		SetAbsVelocity( vec3_origin );
	}

	engine->SetView( pActivator->edict(), edict() );

	SetModel( STRING(pActivator->GetModelName()) );

	// follow the player down
	SetThink( &CTriggerCamera::FollowTarget );
	SetNextThink( gpGlobals->curtime );

	m_moveDistance = 0;
	Move();
}


void CTriggerCamera::FollowTarget( )
{
	if (m_hPlayer == NULL)
		return;

	if (m_hTarget == NULL || m_flReturnTime < gpGlobals->curtime)
	{
		if (m_hPlayer->IsAlive( ))
		{
			engine->SetView( m_hPlayer->edict(), m_hPlayer->edict() );
			((CBasePlayer *)((CBaseEntity *)m_hPlayer))->EnableControl(TRUE);
		}

		m_OnEndFollow.FireOutput(this, this); // dvsents2: what is the best name for this output?

		SetLocalAngularVelocity( vec3_angle );
		m_state = 0;
		return;
	}

	QAngle vecGoal;
	VectorAngles( m_hTarget->GetLocalOrigin() - GetLocalOrigin(), vecGoal );
	
	// UNDONE: Is this still necessary?
	vecGoal.x = -vecGoal.x;

	// UNDONE: Can't we just use UTIL_AngleDiff here?
	QAngle angles = GetLocalAngles();

	if (angles.y > 360)
		angles.y -= 360;

	if (angles.y < 0)
		angles.y += 360;

	SetLocalAngles( angles );

	float dx = vecGoal.x - GetLocalAngles().x;
	float dy = vecGoal.y - GetLocalAngles().y;

	if (dx < -180) 
		dx += 360;
	if (dx > 180) 
		dx = dx - 360;
	
	if (dy < -180) 
		dy += 360;
	if (dy > 180) 
		dy = dy - 360;

	QAngle vecAngVel;
	vecAngVel.Init( dx * 40 * gpGlobals->frametime, dy * 40 * gpGlobals->frametime, GetLocalAngularVelocity().z );
	SetLocalAngularVelocity(vecAngVel);

	if (!HasSpawnFlags(SF_CAMERA_PLAYER_TAKECONTROL))	
	{
		SetAbsVelocity( GetAbsVelocity() * 0.8 );
		if (GetAbsVelocity().Length( ) < 10.0)
		{
			SetAbsVelocity( vec3_origin );
		}
	}

	SetNextThink( gpGlobals->curtime );

	Move();
}

void CTriggerCamera::Move()
{
	// Not moving on a path, return
	if (!m_pPath)
		return;

	// Subtract movement from the previous frame
	m_moveDistance -= m_flSpeed * gpGlobals->frametime;

	// Have we moved enough to reach the target?
	if ( m_moveDistance <= 0 )
	{
		variant_t emptyVariant;
		m_pPath->AcceptInput( "InPass", this, this, emptyVariant, 0 );
		// Time to go to the next target
		m_pPath = m_pPath->GetNextTarget();

		// Set up next corner
		if ( !m_pPath )
		{
			SetAbsVelocity( vec3_origin );
		}
		else 
		{
			if ( m_pPath->m_flSpeed != 0 )
				m_targetSpeed = m_pPath->m_flSpeed;

			m_vecMoveDir = m_pPath->GetLocalOrigin() - GetLocalOrigin();
			m_moveDistance = VectorNormalize( m_vecMoveDir );
			m_flStopTime = gpGlobals->curtime + m_pPath->GetDelay();
		}
	}

	if ( m_flStopTime > gpGlobals->curtime )
		m_flSpeed = UTIL_Approach( 0, m_flSpeed, m_deceleration * gpGlobals->frametime );
	else
		m_flSpeed = UTIL_Approach( m_targetSpeed, m_flSpeed, m_acceleration * gpGlobals->frametime );

	float fraction = 2 * gpGlobals->frametime;
	SetAbsVelocity( ((m_vecMoveDir * m_flSpeed) * fraction) + (GetAbsVelocity() * (1-fraction)) );
}


//-----------------------------------------------------------------------------
// Purpose: Starts/stops cd audio tracks
//-----------------------------------------------------------------------------
class CTriggerCDAudio : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerCDAudio, CBaseTrigger );

	void Spawn( void );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void PlayTrack( void );
	void Touch ( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_cdaudio, CTriggerCDAudio );


//-----------------------------------------------------------------------------
// Purpose: Changes tracks or stops CD when player touches
// Input  : pOther - The entity that touched us.
//-----------------------------------------------------------------------------
void CTriggerCDAudio::Touch ( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
	{
		return;
	}

	PlayTrack();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCDAudio::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}


void CTriggerCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	PlayTrack();
}


//-----------------------------------------------------------------------------
// Purpose: Issues a client command to play a given CD track. Called from
//			trigger_cdaudio and target_cdaudio.
// Input  : iTrack - Track number to play.
//-----------------------------------------------------------------------------
static void PlayCDTrack( int iTrack )
{
	edict_t *pClient;
	
	// manually find the single player. 
	pClient = engine->PEntityOfEntIndex( 1 );
	
	// Can't play if the client is not connected!
	if ( !pClient )
		return;

	// UNDONE: Move this to engine sound
	if ( iTrack < -1 || iTrack > 30 )
	{
		Warning( "TriggerCDAudio - Track %d out of range\n" );
		return;
	}

	if ( iTrack == -1 )
	{
		engine->ClientCommand ( pClient, "cd pause\n");
	}
	else
	{
		char string [ 64 ];

		Q_snprintf( string,sizeof(string), "cd play %3d\n", iTrack );
		engine->ClientCommand ( pClient, string);
	}
}


// only plays for ONE client, so only use in single play!
void CTriggerCDAudio::PlayTrack( void )
{
	PlayCDTrack( (int)m_iHealth );
	
	SetTouch( NULL );
	UTIL_Remove( this );
}


//-----------------------------------------------------------------------------
// Purpose: Measures the proximity to a specified entity of any entities within
//			the trigger, provided they are within a given radius of the specified
//			entity. The nearest entity distance is output as a number from [0 - 1].
//-----------------------------------------------------------------------------
class CTriggerProximity : public CBaseTrigger
{
public:

	DECLARE_CLASS( CTriggerProximity, CBaseTrigger );

	virtual void Spawn(void);
	virtual void Activate(void);
	virtual void StartTouch(CBaseEntity *pOther);
	virtual void EndTouch(CBaseEntity *pOther);

	void MeasureThink(void);

protected:

	EHANDLE m_hMeasureTarget;
	string_t m_iszMeasureTarget;		// The entity from which we measure proximities.
	float m_fRadius;			// The radius around the measure target that we measure within.
	int m_nTouchers;			// Number of entities touching us.

	// Outputs
	COutputFloat m_NearestEntityDistance;

	DECLARE_DATADESC();
};


BEGIN_DATADESC( CTriggerProximity )

	// Functions
	DEFINE_FUNCTION(CTriggerProximity, MeasureThink),

	// Keys
	DEFINE_KEYFIELD(CTriggerProximity, m_iszMeasureTarget, FIELD_STRING, "measuretarget"),
	DEFINE_FIELD( CTriggerProximity, m_hMeasureTarget, FIELD_EHANDLE ),
	DEFINE_KEYFIELD(CTriggerProximity, m_fRadius, FIELD_FLOAT, "radius"),
	DEFINE_FIELD( CTriggerProximity, m_nTouchers, FIELD_INTEGER ),

	// Outputs
	DEFINE_OUTPUT(CTriggerProximity, m_NearestEntityDistance, "NearestEntityDistance"),

END_DATADESC()



LINK_ENTITY_TO_CLASS(trigger_proximity, CTriggerProximity);
LINK_ENTITY_TO_CLASS(logic_proximity, CPointEntity);


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerProximity::Spawn(void)
{
	// Avoid divide by zero in MeasureThink!
	if (m_fRadius == 0)
	{
		m_fRadius = 32;
	}

	InitTrigger();
	UTIL_Relink(this);
}


//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned and after a load game.
//			Finds the reference point from which to measure.
//-----------------------------------------------------------------------------
void CTriggerProximity::Activate(void)
{
	BaseClass::Activate();
	m_hMeasureTarget = gEntList.FindEntityByName(NULL, m_iszMeasureTarget, NULL);

	//
	// Disable our Touch function if we were given a bad measure target.
	//
	if ((m_hMeasureTarget == NULL) || (m_hMeasureTarget->pev == NULL))
	{
		Warning( "TriggerProximity - Missing measure target or measure target with no origin!\n");
	}
}


//-----------------------------------------------------------------------------
// Purpose: Decrements the touch count and cancels the think if the count reaches
//			zero.
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CTriggerProximity::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch( pOther );

	if ( PassesTriggerFilters( pOther ) )
	{
		m_nTouchers++;	

		SetThink( &CTriggerProximity::MeasureThink );
		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Decrements the touch count and cancels the think if the count reaches
//			zero.
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CTriggerProximity::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch( pOther );

	if ( PassesTriggerFilters( pOther ) )
	{
		m_nTouchers--;

		if ( m_nTouchers == 0 )
		{
			SetThink( NULL );
			SetNextThink( TICK_NEVER_THINK );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Think function called every frame as long as we have entities touching
//			us that we care about. Finds the closest entity to the measure
//			target and outputs the distance as a normalized value from [0..1].
//-----------------------------------------------------------------------------
void CTriggerProximity::MeasureThink( void )
{
	if ( ( m_hMeasureTarget == NULL ) || ( m_hMeasureTarget->pev == NULL ) )
	{
		SetThink(NULL);
		SetNextThink( TICK_NEVER_THINK );
		return;
	}

	//
	// Traverse our list of touchers and find the entity that is closest to the
	// measure target.
	//
	float fMinDistance = m_fRadius + 100;
	CBaseEntity *pNearestEntity = NULL;

	touchlink_t *pLink = m_EntitiesTouched.nextLink;
	while ( pLink != &m_EntitiesTouched )
	{
		CBaseEntity *pEntity = pLink->entityTouched;

		// If this is an entity that we care about, check its distance.
		if ( ( pEntity != NULL ) && PassesTriggerFilters( pEntity ) )
		{
			float flDistance = (pEntity->GetLocalOrigin() - m_hMeasureTarget->GetLocalOrigin()).Length();
			if (flDistance < fMinDistance)
			{
				fMinDistance = flDistance;
				pNearestEntity = pEntity;
			}
		}

		pLink = pLink->nextLink;
	}

	// Update our output with the nearest entity distance, normalized to [0..1].
	if ( fMinDistance <= m_fRadius )
	{
		fMinDistance /= m_fRadius;
		if ( fMinDistance != m_NearestEntityDistance.Get() )
		{
			m_NearestEntityDistance.Set( fMinDistance, pNearestEntity, this );
		}
	}

	SetNextThink( gpGlobals->curtime );
}


// ##################################################################################
//	>> TriggerFog
//
//  Creates fog billows when collided with
//
// ##################################################################################
extern short g_sModelIndexSmoke;
extern float	GetFloorZ(const Vector &origin);

class CTriggerFog : public CTriggerMultiple
{
public:
	DECLARE_CLASS( CTriggerFog, CTriggerMultiple );

	void Spawn( void );
	void FogTouch( CBaseEntity *pOther );

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( trigger_fog, CTriggerFog );
BEGIN_DATADESC( CTriggerFog )

	// Function Pointers
	DEFINE_FUNCTION(CTriggerFog, FogTouch),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CTriggerFog::Spawn( void )
{
	BaseClass::Spawn();

	SetTouch(&CTriggerFog::FogTouch);
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CTriggerFog::FogTouch(CBaseEntity *pOther)
{
	if (PassesTriggerFilters(pOther))
	{
		Vector vVelocity;
		pOther->GetVelocity( &vVelocity, NULL );

		if (vVelocity.Length() > 5)
		{
			// Move at 1/4 the velocity of the colliding object
			vVelocity *= 0.25;
			Vector vPos = pOther->GetLocalOrigin();

			// Raise up off the floor a bit
			vPos.z = GetFloorZ(vPos)+12;
			CPVSFilter filter( vPos );
			te->FogRipple( filter, 0.0, 
				&vPos, &vVelocity);
		}
	}
}


// ##################################################################################
//	>> TriggerWind
//
//  Blows physics objects in the trigger
//
// ##################################################################################

#define MAX_WIND_CHANGE		5.0f

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
class CPhysicsWind : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:
	simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
	{
		// Get a cosine modulated noise between 5 and 20 that is object specific
		int nNoiseMod = 5+(int)pObject%15; // 

		// Turn wind yaw direction into a vector and add noise
		QAngle vWindAngle = vec3_angle;	
		vWindAngle[1] = m_nWindYaw+(30*cos(nNoiseMod * gpGlobals->curtime + nNoiseMod));
		Vector vWind;
		AngleVectors(vWindAngle,&vWind);

		// Add lift with noise
		vWind.z = 1.1 + (1.0 * sin(nNoiseMod * gpGlobals->curtime + nNoiseMod));

		linear = 3*vWind*m_flWindSpeed;
		angular = vec3_origin;
		return IMotionEvent::SIM_GLOBAL_FORCE;	
	}

	int		m_nWindYaw;
	float	m_flWindSpeed;
};

BEGIN_SIMPLE_DATADESC( CPhysicsWind )

	DEFINE_FIELD( CPhysicsWind,	m_nWindYaw,		FIELD_INTEGER ),
	DEFINE_FIELD( CPhysicsWind,	m_flWindSpeed,	FIELD_FLOAT ),

END_DATADESC()


extern short g_sModelIndexSmoke;
extern float	GetFloorZ(const Vector &origin);

class CTriggerWind : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerWind, CTriggerMultiple );
public:
	DECLARE_DATADESC();

	int		m_nSpeedBase;	// base line for how hard the wind blows
	int		m_nSpeedNoise;	// noise added to wind speed +/-
	int		m_nSpeedCurrent;// current wind speed
	int		m_nSpeedTarget;	// wind speed I'm approaching

	int		m_nDirBase;		// base line for direction the wind blows (yaw)
	int		m_nDirNoise;	// noise added to wind direction
	int		m_nDirCurrent;	// the current wind direction
	int		m_nDirTarget;	// wind direction I'm approaching

	int		m_nHoldBase;	// base line for how long to wait before changing wind
	int		m_nHoldNoise;	// noise added to how long to wait before changing wind

	bool	m_bSwitch;		// when does wind change

	virtual void OnRestore();
	void	Spawn( void );
	void	WindThink( void );
	void	StartTouch( CBaseEntity *pOther );
	void	EndTouch( CBaseEntity *pOther );
	int		DrawDebugTextOverlays(void);

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputSetSpeed( inputdata_t &inputdata );

	IPhysicsMotionController*	m_pWindController;
	CPhysicsWind				m_WindCallback;

};

LINK_ENTITY_TO_CLASS( trigger_wind, CTriggerWind );

BEGIN_DATADESC( CTriggerWind )

	DEFINE_FIELD( CTriggerWind,		m_nSpeedCurrent, FIELD_INTEGER),
	DEFINE_FIELD( CTriggerWind,		m_nSpeedTarget,	FIELD_INTEGER),
	DEFINE_FIELD( CTriggerWind,		m_nDirBase,		FIELD_INTEGER),
	DEFINE_FIELD( CTriggerWind,		m_nDirCurrent,	FIELD_INTEGER),
	DEFINE_FIELD( CTriggerWind,		m_nDirTarget,	FIELD_INTEGER),
	DEFINE_FIELD( CTriggerWind,		m_bSwitch,		FIELD_BOOLEAN),

	DEFINE_KEYFIELD( CTriggerWind,	m_nSpeedBase,	FIELD_INTEGER, "Speed"),
	DEFINE_KEYFIELD( CTriggerWind,	m_nSpeedNoise,	FIELD_INTEGER, "SpeedNoise"),
	DEFINE_KEYFIELD( CTriggerWind,	m_nDirNoise,	FIELD_INTEGER, "DirectionNoise"),
	DEFINE_KEYFIELD( CTriggerWind,	m_nHoldBase,	FIELD_INTEGER, "HoldTime"),
	DEFINE_KEYFIELD( CTriggerWind,	m_nHoldNoise,	FIELD_INTEGER, "HoldNoise"),

	DEFINE_PHYSPTR( CTriggerWind,	m_pWindController ),
	DEFINE_EMBEDDED( CTriggerWind,	m_WindCallback ),

	DEFINE_FUNCTION( CTriggerWind, WindThink),

	DEFINE_INPUTFUNC( CTriggerWind, FIELD_VOID, "SetSpeed", InputSetSpeed ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::Spawn( void )
{
	m_bSwitch			= true;
	m_pWindController	= physenv->CreateMotionController( &m_WindCallback );
	m_nDirBase			= GetLocalAngles().y;

	Relink();

	BaseClass::Spawn();

	m_nSpeedCurrent  = m_nSpeedBase;
	m_nDirCurrent	 = m_nDirBase;

	SetThink(&CTriggerWind::WindThink);
	SetNextThink( gpGlobals->curtime );
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::OnRestore()
{
	BaseClass::OnRestore();
	if ( m_pWindController )
	{
		m_pWindController->SetEventHandler( &m_WindCallback );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::StartTouch(CBaseEntity *pOther)
{
	if (!m_pWindController || pOther->GetMoveType() != MOVETYPE_VPHYSICS || pOther->IsPlayer())
	{
		return;
	}

	IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
	if ( pPhys)
	{
		m_pWindController->AttachObject( pPhys );
		pPhys->Wake();
	}
}



//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::EndTouch(CBaseEntity *pOther)
{
	if (!m_pWindController || pOther->GetMoveType() != MOVETYPE_VPHYSICS || pOther->IsPlayer())
	{
		return;
	}

	IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
	if ( pPhys)
	{
		m_pWindController->DetachObject( pPhys );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::InputEnable( inputdata_t &inputdata )
{
	BaseClass::InputEnable( inputdata );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::WindThink( void )
{
	// If I'm not enabled, don't blow anything
	if (!IsSolidFlagSet(FSOLID_TRIGGER))
	{
		m_WindCallback.m_flWindSpeed	= 0;
		m_bSwitch						= true;
		return;
	}

	CBaseEntity *pPlayer;

	pPlayer = (CBaseEntity *)UTIL_PlayerByIndex( 1 );
	if ( !pPlayer )
	{
		// player isn't valid right now - defer the think
		SetNextThink( gpGlobals->curtime + 0.2 );
		return;
	}

	// By default...
	SetNextThink( gpGlobals->curtime + 0.1f );

	// Is it time to change the wind?
	if (m_bSwitch)
	{
		m_bSwitch = false;

		// Set new target direction and speed
		m_nSpeedTarget = m_nSpeedBase + random->RandomInt( -m_nSpeedNoise, m_nSpeedNoise );
		m_nDirTarget	= UTIL_AngleMod( m_nDirBase + random->RandomInt(-m_nDirNoise, m_nDirNoise) );
	}
	else
	{
		bool bDone = true;
		// either ramp up, or sleep till change
		if (fabs(m_nSpeedTarget - m_nSpeedCurrent) > MAX_WIND_CHANGE)
		{
			m_nSpeedCurrent += (m_nSpeedTarget > m_nSpeedCurrent) ? MAX_WIND_CHANGE : -MAX_WIND_CHANGE;
			bDone = false;
		}

		if (fabs(m_nDirTarget - m_nDirCurrent) > MAX_WIND_CHANGE)
		{

			m_nDirCurrent = UTIL_ApproachAngle( m_nDirTarget, m_nDirCurrent, MAX_WIND_CHANGE );
			bDone = false;
		}
		
		if (bDone)
		{
			m_nSpeedCurrent  = m_nSpeedTarget;
			SetNextThink( m_nHoldBase + random->RandomFloat(-m_nHoldNoise,m_nHoldNoise) );
			m_bSwitch		  = true;
		}
	}

	// store the wind data in the controller callback
	m_WindCallback.m_nWindYaw	 = m_nDirCurrent;
	m_WindCallback.m_flWindSpeed = m_nSpeedCurrent;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerWind::InputSetSpeed( inputdata_t &inputdata )
{
	// Set new speed and mark to switch
	m_nSpeedBase = inputdata.value.Int();
	m_bSwitch	 = true;
	m_pWindController->WakeObjects();
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTriggerWind::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print Target
		// --------------
		char tempstr[255];
		Q_snprintf(tempstr,sizeof(tempstr),"Dir: %i (%i)",m_nDirCurrent,m_nDirTarget);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Speed: %i (%i)",m_nSpeedCurrent,m_nSpeedTarget);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


// ##################################################################################
//	>> TriggerImpact
//
//  Blows physics objects in the trigger
//
// ##################################################################################
#define TRIGGERIMPACT_VIEWKICK_SCALE 0.1

class CTriggerImpact : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerImpact, CTriggerMultiple );
public:
	DECLARE_DATADESC();

	float	m_flMagnitude;
	float	m_flNoise;
	float	m_flViewkick;

	void	Spawn( void );
	void	StartTouch( CBaseEntity *pOther );

	// Inputs
	void InputSetMagnitude( inputdata_t &inputdata );
	void InputImpact( inputdata_t &inputdata );

	// Outputs
	COutputVector	m_pOutputForce;		// Output force in case anyone else wants to use it

	// Debug
	int		DrawDebugTextOverlays(void);
};

LINK_ENTITY_TO_CLASS( trigger_impact, CTriggerImpact );

BEGIN_DATADESC( CTriggerImpact )

	DEFINE_KEYFIELD( CTriggerImpact, m_flMagnitude,	FIELD_FLOAT, "Magnitude"),
	DEFINE_KEYFIELD( CTriggerImpact, m_flNoise,		FIELD_FLOAT, "Noise"),
	DEFINE_KEYFIELD( CTriggerImpact, m_flViewkick,	FIELD_FLOAT, "Viewkick"),

	// Inputs
	DEFINE_INPUTFUNC( CTriggerImpact, FIELD_VOID,  "Impact", InputImpact ),
	DEFINE_INPUTFUNC( CTriggerImpact, FIELD_FLOAT, "SetMagnitude", InputSetMagnitude ),

	// Outputs
	DEFINE_OUTPUT(CTriggerImpact, m_pOutputForce, "ImpactForce"),

	// Function Pointers
	DEFINE_FUNCTION( CTriggerImpact, Disable ),


END_DATADESC()


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::Spawn( void )
{	
	// Clamp date in case user made an error
	m_flNoise = clamp(m_flNoise,0,1);
	m_flViewkick = clamp(m_flViewkick,0,1);

	// Always start disabled
	m_bDisabled = true;
	BaseClass::Spawn();
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::InputImpact( inputdata_t &inputdata )
{
	// Output the force vector in case anyone else wants to use it
	Vector vDir;
	AngleVectors( GetLocalAngles(),&vDir );
	m_pOutputForce.Set( m_flMagnitude * vDir, inputdata.pActivator, inputdata.pCaller);

	// Enable long enough to throw objects inside me
	Enable();
	SetNextThink( gpGlobals->curtime + 0.1f );
	SetThink(&CTriggerImpact::Disable);
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::StartTouch(CBaseEntity *pOther)
{
	//If the entity is valid and has physics, hit it
	if ( ( pOther != NULL  ) && ( pOther->VPhysicsGetObject() != NULL ) )
	{
		Vector vDir;
		AngleVectors( GetLocalAngles(),&vDir );
		vDir += RandomVector(-m_flNoise,m_flNoise);
		pOther->VPhysicsGetObject()->ApplyForceCenter( m_flMagnitude * vDir );
	}

	// If the player, so a view kick
	if (pOther->IsPlayer() && fabs(m_flMagnitude)>0 )
	{
		Vector vDir;
		AngleVectors( GetLocalAngles(),&vDir );

		float flPunch = -m_flViewkick*m_flMagnitude*TRIGGERIMPACT_VIEWKICK_SCALE;
		pOther->ViewPunch( QAngle( vDir.y * flPunch, 0, vDir.x * flPunch ) );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerImpact::InputSetMagnitude( inputdata_t &inputdata )
{
	m_flMagnitude = inputdata.value.Float();
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTriggerImpact::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[255];
		Q_snprintf(tempstr,sizeof(tempstr),"Magnitude: %3.2f",m_flMagnitude);
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Disables auto movement on players that touch it
//-----------------------------------------------------------------------------

const int SF_TRIGGER_MOVE_AUTODISABLE				= 0x10; // disable auto movement

class CTriggerPlayerMovement : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerPlayerMovement, CBaseTrigger );
public:

	void Spawn( void );
	void StartTouch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );
	
	DECLARE_DATADESC();

};

BEGIN_DATADESC( CTriggerPlayerMovement )

END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger_playermovement, CTriggerPlayerMovement );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerPlayerMovement::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();
}


// UNDONE: This will not support a player touching more than one of these
// UNDONE: Do we care?  If so, ref count automovement in the player?
void CTriggerPlayerMovement::StartTouch( CBaseEntity *pOther )
{	
	if (!PassesTriggerFilters(pOther))
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );

	if ( !pPlayer )
		return;

	// UNDONE: Currently this is the only operation this trigger can do
	if ( HasSpawnFlags(SF_TRIGGER_MOVE_AUTODISABLE) )
	{
		pPlayer->m_Local.m_bAllowAutoMovement = false;
	}
}

void CTriggerPlayerMovement::EndTouch( CBaseEntity *pOther )
{
	if (!PassesTriggerFilters(pOther))
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOther );

	if ( !pPlayer )
		return;

	if ( HasSpawnFlags(SF_TRIGGER_MOVE_AUTODISABLE) )
	{
		pPlayer->m_Local.m_bAllowAutoMovement = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTriggerWateryDeath : public CBaseTrigger
{
	DECLARE_CLASS( CTriggerWateryDeath, CBaseTrigger );
public:
	DECLARE_DATADESC();

	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	
	// Ignore non-living entities
	virtual bool PassesTriggerFilters(CBaseEntity *pOther)
	{
		if ( !BaseClass::PassesTriggerFilters(pOther) )
			return false;

		return (pOther->m_takedamage == DAMAGE_YES);
	}

	virtual void StartTouch(CBaseEntity *pOther);
	virtual void EndTouch(CBaseEntity *pOther);

private:
	// Kill times for entities I'm touching
	CUtlVector< float >	m_flEntityKillTimes;
	float				m_flNextPullSound;
};

BEGIN_DATADESC( CTriggerWateryDeath )
	DEFINE_UTLVECTOR( CTriggerWateryDeath, m_flEntityKillTimes, FIELD_TIME ),
	DEFINE_FIELD( CTriggerWateryDeath, m_flNextPullSound, FIELD_TIME ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger_waterydeath, CTriggerWateryDeath );

// Stages of the waterydeath trigger, in time offsets from the initial touch
#define	WD_KILLTIME_WARN_TIME		6
#define WD_KILLTIME_DRAGDOWN_TIME	1.5

//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerWateryDeath::Spawn( void )
{
	BaseClass::Spawn();

	m_flNextPullSound = 0;
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerWateryDeath::Touch( CBaseEntity *pOther )
{	
	if (!PassesTriggerFilters(pOther))
		return;

	float flDistanceBelow = 128;

	// Find our index
	EHANDLE hOther;
	hOther = pOther;
	int iIndex = m_hTouchingEntities.Find( hOther );
	if ( iIndex == m_hTouchingEntities.InvalidIndex() )
		return;

	float flKillTime = m_flEntityKillTimes[iIndex];
	
	// Time to kill it?
	if ( gpGlobals->curtime > flKillTime )
	{
		// Chomp!
		CPASAttenuationFilter filter( pOther );
		EmitSound( filter, entindex(), "WateryDeath.Bite", &pOther->GetAbsOrigin() );

		// Kill it
		pOther->TakeDamage( CTakeDamageInfo( this, this, pOther->GetHealth(), DMG_DROWN ) );
	}
	else
	{
		// Warn the target?
		if ( (gpGlobals->curtime + WD_KILLTIME_DRAGDOWN_TIME) < flKillTime )
		{
			// We're still giving the player some warning time
		}
		else if ( pOther->GetAbsMaxs().z > (GetAbsMaxs().z - flDistanceBelow) )
		{
			if ( m_flNextPullSound < gpGlobals->curtime )
			{
				m_flNextPullSound = gpGlobals->curtime + 5.0;

				// Play a "you're about to die" sound
				CPASAttenuationFilter filter( pOther );
				EmitSound( filter, entindex(), "WateryDeath.Pull", &pOther->GetAbsOrigin() );

				// Make splashes if we're being pulled down from the surface
				if ( pOther->GetAbsMaxs().z > GetAbsMaxs().z )
				{
					CEffectData	data;
					data.m_vOrigin = pOther->GetAbsOrigin();
					data.m_vNormal = RandomVector(-1,1);
					data.m_vNormal.z = 1;
					VectorAngles( data.m_vNormal, data.m_vAngles );
					data.m_flScale = random->RandomFloat( 10, 20 );
					DispatchEffect( "watersplash", data );
				}
			}

			// It's above the desired distance below, pull it down
			switch( pOther->GetMoveType() )
			{
			case MOVETYPE_NONE:
			case MOVETYPE_PUSH:
			case MOVETYPE_NOCLIP:
				break;

			case MOVETYPE_VPHYSICS:
				{
					IPhysicsObject *pPhys = pOther->VPhysicsGetObject();
					if ( pPhys )
					{
						pPhys->ApplyForceCenter( Vector(0,0,-1024) * pPhys->GetMass() * gpGlobals->frametime );
					}
				}
				break;

			default:
				{
					Vector vecPush = Vector(0,0,-1024);
					if ( pOther->GetFlags() & FL_BASEVELOCITY )
					{
						vecPush = vecPush + pOther->GetBaseVelocity();
					}
					pOther->SetBaseVelocity( vecPush );
					pOther->AddFlag( FL_BASEVELOCITY );
				}
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when an entity starts touching us.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
void CTriggerWateryDeath::StartTouch(CBaseEntity *pOther)
{
	BaseClass::StartTouch( pOther );

	// If we added him to our list, store the start time
	EHANDLE hOther;
	hOther = pOther;
	if ( m_hTouchingEntities.Find( hOther ) != m_hTouchingEntities.InvalidIndex() )
	{
		// Always added to the end
		// Players get warned, everything else gets et quick.
		if ( pOther->IsPlayer() )
		{
			// Warn the player
			CPASAttenuationFilter filter( pOther );
			EmitSound( filter, entindex(), "WateryDeath.Warn", &pOther->GetAbsOrigin() );

			m_flEntityKillTimes.AddToTail( gpGlobals->curtime + WD_KILLTIME_WARN_TIME );
		}
		else
		{
			m_flEntityKillTimes.AddToTail( gpGlobals->curtime + WD_KILLTIME_DRAGDOWN_TIME );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when an entity stops touching us.
// Input  : pOther - The entity that was touching us.
//-----------------------------------------------------------------------------
void CTriggerWateryDeath::EndTouch( CBaseEntity *pOther )
{
	if ( IsTouching( pOther ) )
	{
		EHANDLE hOther;
		hOther = pOther;

		// Remove the time from our list
		int iIndex = m_hTouchingEntities.Find( hOther );
		if ( iIndex != m_hTouchingEntities.InvalidIndex() )
		{
			m_flEntityKillTimes.Remove( iIndex );
		}
	}

	BaseClass::EndTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Triggers whenever an RPG is fired within it
//-----------------------------------------------------------------------------
class CTriggerRPGFire : public CTriggerMultiple
{
	DECLARE_CLASS( CTriggerRPGFire, CTriggerMultiple );
public:
	void Spawn( void );
	void OnRestore( void );
};

LINK_ENTITY_TO_CLASS( trigger_rpgfire, CTriggerRPGFire );

//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerRPGFire::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	g_hWeaponFireTriggers.AddToTail( this );

	// Stomp the touch function, because we don't want to respond to touch
	SetTouch( NULL );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CTriggerRPGFire::OnRestore()
{
	BaseClass::OnRestore();

	g_hWeaponFireTriggers.AddToTail( this );
}

#ifdef HL1_DLL
//----------------------------------------------------------------------------------
// func_friction
//----------------------------------------------------------------------------------
class CFrictionModifier : public CBaseTrigger
{
	DECLARE_CLASS( CFrictionModifier, CBaseTrigger );

public:
	void		Spawn( void );
	bool		KeyValue( const char *szKeyName, const char *szValue );

	virtual void StartTouch(CBaseEntity *pOther);
	virtual void EndTouch(CBaseEntity *pOther);

	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	float		m_frictionFraction;		// Sorry, couldn't resist this name :)

	DECLARE_DATADESC();

};

LINK_ENTITY_TO_CLASS( func_friction, CFrictionModifier );

BEGIN_DATADESC( CFrictionModifier )
	DEFINE_FIELD( CFrictionModifier, m_frictionFraction, FIELD_FLOAT ),
END_DATADESC()

// Modify an entity's friction
void CFrictionModifier::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();
}

// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
bool CFrictionModifier::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "modifier"))
	{
		m_frictionFraction = atof(szValue) / 100.0;
	}
	else
	{
		BaseClass::KeyValue( szKeyName, szValue );
	}
	return true;
}

void CFrictionModifier::StartTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )		// ignore player
	{
		pOther->SetFriction( m_frictionFraction );
	}
}

void CFrictionModifier::EndTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )		// ignore player
	{
		pOther->SetFriction( 1.0f );
	}
}

#endif //HL1_DLL