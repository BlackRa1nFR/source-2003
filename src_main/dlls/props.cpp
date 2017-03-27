//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: static_prop - don't move, don't animate, don't do anything.
//			physics_prop - move, take damage, but don't animate
//
//=============================================================================

#include "cbase.h"
#include "BasePropDoor.h"
#include "ai_basenpc.h"
#include "npcevent.h"
#include "animation.h"
#include "engine/IEngineSound.h"
#include "locksounds.h"
#include "filters.h"
#include "physics.h"
#include "vphysics_interface.h"
#include "entityoutput.h"
#include "vcollide_parse.h"
#include "bone_setup.h"
#include "studio.h"
#include "explode.h"
#include "igamesystem.h"
#include "utlrbtree.h"
#include "vstdlib/strtools.h"
#include "physics_impact_damage.h"
#include "keyvalues.h"
#include "filesystem.h"
#include "igamesystem.h"

const char *GetMassEquivalent(float flMass);


//=============================================================================================================
// PROP DATA
//=============================================================================================================
//-----------------------------------------------------------------------------
// Purpose: Gamesystem that parses the prop data file
//-----------------------------------------------------------------------------
class CPropData : public CAutoGameSystem
{
public:
	CPropData( void );

	// Inherited from IAutoServerSystem
	virtual void LevelInitPreEntity( void );
	virtual void ShutdownAllSystems( void );

	// Read in the data from the prop data file
	void ParsePropDataFile( void );

	// Parse a keyvalues section into the prop
	int ParsePropFromKV( CBaseProp *pProp, KeyValues *pSection );

	// Fill out a prop's with base data parsed from the propdata file
	int ParsePropFromBase( CBaseProp *pProp, const char *pszPropData );

private:
	KeyValues	*m_pKVPropData;
	bool		m_bPropDataLoaded;
};

static CPropData g_PropDataSystem;

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CPropData::CPropData( void )
{
	m_bPropDataLoaded = false;
	m_pKVPropData = NULL;
}

//-----------------------------------------------------------------------------
// Inherited from IAutoServerSystem
//-----------------------------------------------------------------------------
void CPropData::LevelInitPreEntity( void )
{
	ParsePropDataFile();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropData::ShutdownAllSystems( void )
{
	if ( m_pKVPropData )
	{
		m_pKVPropData->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Clear out the stats + their history
//-----------------------------------------------------------------------------
void CPropData::ParsePropDataFile( void )
{
	m_pKVPropData = new KeyValues( "PropDatafile" );
	if ( !m_pKVPropData->LoadFromFile( filesystem, "scripts/propdata.txt" ) )
	{
		m_pKVPropData->deleteThis();
		return;
	}

	m_bPropDataLoaded = true;
}

//-----------------------------------------------------------------------------
// Purpose: Parse a keyvalues section into the prop
//-----------------------------------------------------------------------------
int CPropData::ParsePropFromKV( CBaseProp *pProp, KeyValues *pSection )
{
	// Do we have a base?
	char const *pszBase = pSection->GetString( "base" );
	if ( pszBase && pszBase[0] )
	{
		int iResult = ParsePropFromBase( pProp, pszBase );
		if ( iResult != PARSE_SUCCEEDED )
			return iResult;
	}

	// Only breakable props need to get health data
	CBreakableProp *pBreakable = dynamic_cast<CBreakableProp *>(pProp);
	if ( pBreakable )
	{
		// Get damage modifiers, but only if they're specified, because our base may have already overridden them.
		pBreakable->SetDmgModBullet( pSection->GetFloat( "dmg.bullets", pBreakable->GetDmgModBullet() ) );
		pBreakable->SetDmgModClub( pSection->GetFloat( "dmg.club", pBreakable->GetDmgModClub() ) );
		pBreakable->SetDmgModExplosive( pSection->GetFloat( "dmg.explosive", pBreakable->GetDmgModExplosive() ) );

		// Get the health (unless this is an override prop)
		if ( !FClassnameIs( pProp, "prop_physics_override" ) && !FClassnameIs( pProp, "prop_dynamic_override" ) )
		{
			pBreakable->SetHealth( pSection->GetInt( "health", pBreakable->GetHealth() ) );
		}
	}

	return PARSE_SUCCEEDED;
}

//-----------------------------------------------------------------------------
// Purpose: Fill out a prop's with base data parsed from the propdata file
//-----------------------------------------------------------------------------
int CPropData::ParsePropFromBase( CBaseProp *pProp, const char *pszPropData )
{
	if ( !m_bPropDataLoaded )
		return PARSE_FAILED_NO_DATA;

	// Find the specified propdata
	KeyValues *pSection = m_pKVPropData->FindKey( pszPropData );
	if ( !pSection )
	{
		Warning("%s '%s' has a base specified as '%s', but there is no matching entry in propdata.txt.\n", pProp->GetClassname(), pProp->GetModelName(), pszPropData );
		return PARSE_FAILED_BAD_DATA;
	}

	// Store off the first base data for debugging
	if ( pProp->m_iszBasePropData == NULL_STRING )
	{
		pProp->m_iszBasePropData = AllocPooledString( pszPropData );
	}

	return ParsePropFromKV( pProp, pSection );
}

// Damage type modifiers for breakable objects.
ConVar func_breakdmg_bullet( "func_breakdmg_bullet", "0.5" );
ConVar func_breakdmg_club( "func_breakdmg_club", "1.5" );
ConVar func_breakdmg_explosive( "func_breakdmg_explosive", "1.25" );

//-----------------------------------------------------------------------------
// Purpose: Breakable objects take different levels of damage based upon the damage type.
//			This isn't contained by CBaseProp, because func_breakables use it as well.
//-----------------------------------------------------------------------------
float GetBreakableDamage( const CTakeDamageInfo &inputInfo, CBreakableProp *pProp )
{
	float flDamage = inputInfo.GetDamage();
	int iDmgType = inputInfo.GetDamageType();

	// Bullet damage?
	if ( iDmgType & DMG_BULLET )
	{
		if ( pProp )
		{
			flDamage *= pProp->GetDmgModBullet();
		}
		else
		{
			// Bullets do little damage to breakables
			flDamage *= func_breakdmg_bullet.GetFloat();
		}
	}

	// Club damage?
	if ( iDmgType & DMG_CLUB )
	{
		if ( pProp )
		{
			flDamage *= pProp->GetDmgModClub();
		}
		else
		{
			// Club does extra damage
			flDamage *= func_breakdmg_club.GetFloat();
		}
	}

	// Explosive damage?
	if ( iDmgType & DMG_BLAST )
	{
		if ( pProp )
		{
			flDamage *= pProp->GetDmgModExplosive();
		}
		else
		{
			// Explosions do extra damage
			flDamage *= func_breakdmg_explosive.GetFloat();
		}
	}

	// Poison & other timebased damage types do no damage
	if ( iDmgType & DMG_TIMEBASED )
	{
		flDamage = 0;
	}

	return flDamage;
}

//=============================================================================================================
// BASE PROP
//=============================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProp::Spawn( void )
{
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		Warning( "prop at %.0f %.0f %0.f missing modelname\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove( this );
		return;
	}

	Precache();
	SetModel( szModel );

	// Load this prop's data from the propdata file
	int iResult = ParsePropData();
	if ( !OverridePropdata() )
	{
		if ( iResult == PARSE_FAILED_BAD_DATA )
		{
			Warning( "%s at %.0f %.0f %0.f uses model %s, which has an invalid prop_data type. DELETED.\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, szModel );
			UTIL_Remove( this );
			return;
		}
		else if ( iResult == PARSE_FAILED_NO_DATA )
		{
			// If we don't have data, but we're a prop_physics, fail
			if ( FClassnameIs( this, "prop_physics" ) )
			{
				Warning( "%s at %.0f %.0f %0.f uses model %s, which requires that it be used on a prop_static. DELETED.\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, szModel );
				UTIL_Remove( this );
				return;
			}
		}
		else if ( iResult == PARSE_SUCCEEDED )
		{
			// If we have data, and we're a static prop, fail
			if ( !FClassnameIs( this, "prop_physics" ) && !FClassnameIs( this, "prop_physics_override" ) && !FClassnameIs( this, "prop_dynamic_override" ) )
			{
				Warning( "%s at %.0f %.0f %0.f uses model %s, which requires that it be used on a prop_physics. DELETED.\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, szModel );
				UTIL_Remove( this );
				return;
			}
		}
	}

	SetMoveType( MOVETYPE_NONE );
	m_takedamage = DAMAGE_NO;
	SetNextThink( TICK_NEVER_THINK );

	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	m_flCycle = 0;
	Relink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProp::Precache( void )
{
	engine->PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProp::Activate( void )
{
	BaseClass::Activate();
	
	// Make sure mapmakers haven't used the wrong prop type.
	if ( m_takedamage == DAMAGE_NO && m_iHealth != 0 )
	{
		Warning("%s has a health specified in model '%s'. Use prop_physics or prop_dynamic instead.\n", GetClassname(), GetModelName() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles keyvalues from the BSP. Called before spawning.
//-----------------------------------------------------------------------------
bool CBaseProp::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq(szKeyName, "health") )
	{
		// Only override props are allowed to override health.
		if ( FClassnameIs( this, "prop_physics_override" ) || FClassnameIs( this, "prop_dynamic_override" ) )
			return BaseClass::KeyValue( szKeyName, szValue );

		return true;
	}
	else
	{ 
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Parse this prop's data from the model, if it has a keyvalues section.
//			Returns true only if this prop is using a model that has a prop_data section that's invalid.
//-----------------------------------------------------------------------------
int CBaseProp::ParsePropData( void )
{
	KeyValues *modelKeyValues = new KeyValues("");
	if ( !modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		modelKeyValues->deleteThis();
		return PARSE_FAILED_NO_DATA;
	}

	// Do we have a props section?
	KeyValues *pkvPropData = modelKeyValues->FindKey("prop_data");
	if ( !pkvPropData )
	{
		modelKeyValues->deleteThis();
		return PARSE_FAILED_NO_DATA;
	}

	int iResult = g_PropDataSystem.ParsePropFromKV( this, pkvPropData );
	modelKeyValues->deleteThis();
	return iResult;
}

// HACK: Re-use NPC bit for prop debug
#define OVERLAY_PROP_DEBUG		OVERLAY_NPC_STEERING_REGULATIONS

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProp::DrawDebugGeometryOverlays( void )
{
	BaseClass::DrawDebugGeometryOverlays();

	if ( m_debugOverlays & OVERLAY_PROP_DEBUG )  
	{
		if ( m_takedamage == DAMAGE_NO )
		{
			NDebugOverlay::EntityBounds(this, 255, 0, 0, 0, 0 );
		}
		else if ( m_takedamage == DAMAGE_EVENTS_ONLY )
		{
			NDebugOverlay::EntityBounds(this, 255, 255, 255, 0, 0 );
		}
		else
		{
			// Remap health to green brightness
			float flG = RemapVal( m_iHealth, 0, 100, 64, 255 );
			flG = clamp( flG, 0, 255 );
			NDebugOverlay::EntityBounds(this, 0, flG, 0, 0, 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turn on prop debugging mode
//-----------------------------------------------------------------------------
void CC_Prop_Debug( void )
{
	// Toggle the prop debug bit on all props
	for ( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt(pEntity) )
	{
		CBaseProp *pProp = dynamic_cast<CBaseProp*>(pEntity);
		if ( pProp )
		{
			if ( pProp->m_debugOverlays )
			{
				pProp->m_debugOverlays &= ~OVERLAY_PROP_DEBUG;
			}
			else
			{
				pProp->m_debugOverlays |= OVERLAY_PROP_DEBUG;
			}
		}
	}
}
static ConCommand prop_debug("prop_debug", CC_Prop_Debug, "Toggle prop debug mode. If on, props will show colorcoded bounding boxes. Red means ignore all damage. White means respond physically to damage but never break. Green maps health in the range of 100 down to 1.", FCVAR_CHEAT);

//=============================================================================================================
// BREAKABLE PROPS
//=============================================================================================================
IMPLEMENT_SERVERCLASS_ST(CBreakableProp, DT_BreakableProp)
END_SEND_TABLE()

BEGIN_DATADESC( CBreakableProp )

	DEFINE_KEYFIELD( CBreakableProp, m_explodeDamage, FIELD_FLOAT, "ExplodeDamage"),	
	DEFINE_KEYFIELD( CBreakableProp, m_explodeRadius, FIELD_FLOAT, "ExplodeRadius"),	
	DEFINE_KEYFIELD( CBreakableProp, m_iMinHealthDmg, FIELD_INTEGER, "minhealthdmg" ),
	DEFINE_FIELD( CBreakableProp, m_createTick, FIELD_INTEGER ),
	DEFINE_FIELD( CBreakableProp, m_hBreaker, FIELD_EHANDLE ),

	DEFINE_FIELD( CBreakableProp, m_flDmgModBullet, FIELD_FLOAT ),
	DEFINE_FIELD( CBreakableProp, m_flDmgModClub, FIELD_FLOAT ),
	DEFINE_FIELD( CBreakableProp, m_flDmgModExplosive, FIELD_FLOAT ),

	DEFINE_KEYFIELD( CBreakableProp, m_flPressureDelay, FIELD_FLOAT, "PressureDelay" ),

	// Inputs
	DEFINE_INPUTFUNC( CBreakableProp, FIELD_VOID, "Break", InputBreak ),
	DEFINE_INPUTFUNC( CBreakableProp, FIELD_INTEGER, "SetHealth", InputSetHealth ),
	DEFINE_INPUTFUNC( CBreakableProp, FIELD_INTEGER, "AddHealth", InputAddHealth ),
	DEFINE_INPUTFUNC( CBreakableProp, FIELD_INTEGER, "RemoveHealth", InputRemoveHealth ),
	DEFINE_INPUT( CBreakableProp, m_impactEnergyScale, FIELD_FLOAT, "physdamagescale" ),


	// Outputs
	DEFINE_OUTPUT( CBreakableProp, m_OnBreak, "OnBreak" ),
	DEFINE_OUTPUT( CBreakableProp, m_OnHealthChanged, "OnHealthChanged" ),

	// Function Pointers
	DEFINE_FUNCTION( CBreakableProp, FadeOut ),
	DEFINE_FUNCTION( CBreakableProp, BreakThink ),
	DEFINE_FUNCTION( CBreakableProp, BreakablePropTouch ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBreakableProp::Spawn()
{
	// Initialize damage modifiers. Must be done before baseclass spawn.
	m_flDmgModBullet = 1.0;
	m_flDmgModClub = 1.0;
	m_flDmgModExplosive = 1.0;

	BaseClass::Spawn();

	// Setup takedamage based upon the health we parsed earlier
	if ( m_iHealth == 0 )
	{
		m_takedamage = DAMAGE_EVENTS_ONLY;
	}
	else
	{
		m_takedamage = DAMAGE_YES;
	}

	m_createTick = gpGlobals->tickcount;
	if ( m_impactEnergyScale == 0 )
	{
		m_impactEnergyScale = 0.1f;
	}

	m_hBreaker = NULL;
	SetTouch( &CBreakableProp::BreakablePropTouch );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBreakableProp::BreakablePropTouch( CBaseEntity *pOther )
{
	if ( HasSpawnFlags( SF_PHYSPROP_TOUCH ) )
	{
		// can be broken when run into 
		float flDamage = pOther->GetAbsVelocity().Length() * 0.01;

		if ( flDamage >= m_iHealth )
		{
			// Make sure we can take damage
			m_takedamage = DAMAGE_YES;
			OnTakeDamage( CTakeDamageInfo( pOther, pOther, flDamage, DMG_CRUSH ) );

			// do a little damage to player if we broke glass or computer
			CTakeDamageInfo info( pOther, pOther, flDamage/4, DMG_SLASH );
			CalculateMeleeDamageForce( &info, (pOther->GetAbsOrigin() - GetAbsOrigin()), GetAbsOrigin() );
			pOther->TakeDamage( info );
		}
	}

	if ( HasSpawnFlags( SF_PHYSPROP_PRESSURE ) && pOther->GetGroundEntity() == this )
	{
		// can be broken when stood upon
		// play creaking sound here.
		// DamageSound();

		m_hBreaker = pOther;

		if ( m_pfnThink != (void (CBaseEntity::*)())&CBreakableProp::BreakThink )
		{
			SetThink( &CBreakableProp::BreakThink );
			//SetTouch( NULL );
		
			// Add optional delay 
			SetNextThink( gpGlobals->curtime + m_flPressureDelay );
		}

	}
}

//-----------------------------------------------------------------------------
// UNDONE: Time stamp the object's creation so that an explosion or something doesn't break the parent object
// and then break the children who spawn afterward ?
// Explosions should use entities in box before they start to do damage.  Make sure nothing traverses the list
// in a way that would hose this.
int CBreakableProp::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	// If attacker can't do at least the min required damage to us, don't take any damage from them
 	if ( info.GetDamage() < m_iMinHealthDmg )
		return 0;

	if (!PassesDamageFilter( info.GetAttacker() ))
	{
		return 1;
	}

	float flPropDamage = GetBreakableDamage( info, this );
	info.SetDamage( flPropDamage );

	// UNDONE: Do this?
#if 0
	// Make a shard noise each time func breakable is hit.
	// Don't play shard noise if being burned.
	// Don't play shard noise if cbreakable actually died.
	if ( ( bitsDamageType & DMG_BURN ) == false )
	{
		DamageSound();
	}
#endif

	// don't take damage on the same frame you were created 
	// (avoids a set of explosions progressively vaporizing a compound breakable)
	if ( m_createTick == (unsigned int)gpGlobals->tickcount )
	{
		int saveFlags = m_takedamage;
		m_takedamage = DAMAGE_EVENTS_ONLY;
		int ret = BaseClass::OnTakeDamage( info );
		m_takedamage = saveFlags;

		return ret;
	}

	int ret = BaseClass::OnTakeDamage( info );
	m_OnHealthChanged.Set( m_iHealth, info.GetAttacker(), this );

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBreakableProp::Event_Killed( const CTakeDamageInfo &info )
{
	IPhysicsObject *pPhysics = VPhysicsGetObject();
	if ( pPhysics && !pPhysics->IsMoveable() )
	{
		pPhysics->EnableMotion( true );
		VPhysicsTakeDamage( info );
	}
	Break( info.GetInflictor(), &info );
	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for breaking the breakable immediately.
//-----------------------------------------------------------------------------
void CBreakableProp::InputBreak( inputdata_t &inputdata )
{
	Break( inputdata.pActivator, NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for adding to the breakable's health.
// Input  : Integer health points to add.
//-----------------------------------------------------------------------------
void CBreakableProp::InputAddHealth( inputdata_t &inputdata )
{
	m_iHealth += inputdata.value.Int();
	m_OnHealthChanged.Set( m_iHealth, inputdata.pActivator, this );

	if ( m_iHealth <= 0 )
	{
		Break( inputdata.pActivator, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for removing health from the breakable.
// Input  : Integer health points to remove.
//-----------------------------------------------------------------------------
void CBreakableProp::InputRemoveHealth( inputdata_t &inputdata )
{
	m_iHealth -= inputdata.value.Int();
	m_OnHealthChanged.Set( m_iHealth, inputdata.pActivator, this );

	if ( m_iHealth <= 0 )
	{
		Break( inputdata.pActivator, NULL );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the breakable's health.
//-----------------------------------------------------------------------------
void CBreakableProp::InputSetHealth( inputdata_t &inputdata )
{
	m_iHealth = inputdata.value.Int();
	m_OnHealthChanged.Set( m_iHealth, inputdata.pActivator, this );

	if ( m_iHealth <= 0 )
	{
		Break( inputdata.pActivator, NULL );
	}
}

void CBreakableProp::StartFadeOut( float delay )
{
	SetThink( &CBreakableProp::FadeOut );
	SetNextThink( gpGlobals->curtime + delay );
	SetRenderColorA( 255 );
	m_nRenderMode = kRenderNormal;
}

void CBreakableProp::FadeOut( void )
{
	float dt = gpGlobals->frametime;
	if ( dt > 0.1f )
	{
		dt = 0.1f;
	}
	m_nRenderMode = kRenderTransTexture;
	int speed = max(1,256*dt); // fade out over 1 second
	SetRenderColorA( UTIL_Approach( 0, m_clrRender->a, speed ) );
	NetworkStateChanged();

	if ( m_clrRender->a == 0 )
	{
		UTIL_Remove(this);
	}
	else
	{
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBreakableProp::BreakThink( void )
{
	Break( m_hBreaker, NULL );
}

void CBreakableProp::Precache()
{
	PropBreakablePrecacheAll( GetModelName() );
	BaseClass::Precache();
}

void CBreakableProp::Break( CBaseEntity *pBreaker, const CTakeDamageInfo *pDamageInfo )
{
	m_takedamage = DAMAGE_NO;
	m_OnBreak.FireOutput( pBreaker, this );

	Vector velocity;
	AngularImpulse angVelocity;
	IPhysicsObject *pPhysics = VPhysicsGetObject();
	Vector origin;
	QAngle angles;
	AddSolidFlags( FSOLID_NOT_SOLID );
	if ( pPhysics )
	{
		pPhysics->GetVelocity( &velocity, &angVelocity );
		pPhysics->GetPosition( &origin, &angles );
		pPhysics->RecheckCollisionFilter();
	}
	else
	{
		velocity = GetAbsVelocity();
		QAngleToAngularImpulse( GetLocalAngularVelocity(), angVelocity );
		origin = GetAbsOrigin();
		angles = GetAbsAngles();
	}
	UTIL_Relink(this);

	if ( m_explodeDamage > 0 || m_explodeRadius > 0 )
	{
		ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), NULL, m_explodeDamage, m_explodeRadius, true );
	}

	breakablepropparams_t params( GetAbsOrigin(), GetAbsAngles(), velocity, angVelocity );
	params.impactEnergyScale = m_impactEnergyScale;
	params.defCollisionGroup = GetCollisionGroup();
	if ( params.defCollisionGroup == COLLISION_GROUP_NONE )
	{
		// don't automatically make anything COLLISION_GROUP_NONE or it will
		// collide with debris being ejected by breaking
		params.defCollisionGroup = COLLISION_GROUP_INTERACTIVE;
	}

	// no damage/damage force? set a burst of 100 for some movement
	params.defBurstScale = pDamageInfo ? 0 : 100;
	PropBreakableCreateAll( GetModelIndex(), pPhysics, params );

	UTIL_Remove(this);
}

//=============================================================================================================
// DYNAMIC PROPS
//=============================================================================================================
LINK_ENTITY_TO_CLASS( dynamic_prop, CDynamicProp );
LINK_ENTITY_TO_CLASS( prop_dynamic, CDynamicProp );	
LINK_ENTITY_TO_CLASS( prop_dynamic_override, CDynamicProp );	

BEGIN_DATADESC( CDynamicProp )

	// Fields
	DEFINE_KEYFIELD( CDynamicProp, m_bRandomAnimator, FIELD_BOOLEAN, "RandomAnimation"),	
	DEFINE_FIELD(	 CDynamicProp, m_flNextRandAnim, FIELD_TIME ),
	DEFINE_KEYFIELD( CDynamicProp, m_flMinRandAnimTime, FIELD_FLOAT, "MinAnimTime"),
	DEFINE_KEYFIELD( CDynamicProp, m_flMaxRandAnimTime, FIELD_FLOAT, "MaxAnimTime"),
		
	// Inputs
	DEFINE_INPUTFUNC( CDynamicProp,	FIELD_STRING,	"SetAnimation",	InputSetAnimation ),
	DEFINE_INPUTFUNC( CDynamicProp,	FIELD_VOID,		"TurnOn",		InputTurnOn ),
	DEFINE_INPUTFUNC( CDynamicProp,	FIELD_VOID,		"TurnOff",		InputTurnOff ),

	// Outputs
	DEFINE_OUTPUT( CDynamicProp, m_pOutputAnimBegun, "OnAnimationBegun" ),
	DEFINE_OUTPUT( CDynamicProp, m_pOutputAnimOver, "OnAnimationDone" ),

	// Function Pointers
	DEFINE_FUNCTION( CDynamicProp, AnimThink ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CDynamicProp, DT_DynamicProp)
END_SEND_TABLE()


CDynamicProp::CDynamicProp()
{
	UseClientSideAnimation();
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CDynamicProp::Spawn( )
{
	// Condense classname's to one, except for "prop_dynamic_override"
	if ( FClassnameIs( this, "dynamic_prop" ) )
	{
		SetClassname( "prop_dynamic" );
	}

	BaseClass::Spawn();

	if ( IsMarkedForDeletion() )
		return;

	// Now condense all classnames to one
	SetClassname("prop_dynamic");

	AddFlag( FL_STATICPROP );
	Relink();
	NetworkStateManualMode( true );

	if ( m_bRandomAnimator )
	{
		RemoveFlag( FL_STATICPROP );
		SetThink( &CDynamicProp::AnimThink );
		m_flNextRandAnim = gpGlobals->curtime + random->RandomFloat( m_flMinRandAnimTime, m_flMaxRandAnimTime );
		SetNextThink( gpGlobals->curtime + m_flNextRandAnim + 0.1 );
	}

	CreateVPhysics();
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CDynamicProp::CreateVPhysics( void )
{
	if ( GetSolid() != SOLID_NONE )
	{
		VPhysicsInitStatic();
	}
	return true;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CDynamicProp::AnimThink( void )
{
	if ( m_bRandomAnimator && m_flNextRandAnim < gpGlobals->curtime )
	{
		ResetSequence( SelectWeightedSequence( ACT_IDLE ) );
		ResetClientsideFrame();

		// Fire output
		m_pOutputAnimBegun.FireOutput( NULL,this );

		m_flNextRandAnim = gpGlobals->curtime + random->RandomFloat( m_flMinRandAnimTime, m_flMaxRandAnimTime );
	}

	StudioFrameAdvance();
	DispatchAnimEvents(this);

	if ( IsSequenceFinished() && !SequenceLoops() )
	{
		// Fire output
		m_pOutputAnimOver.FireOutput(NULL,this);

		// If I'm a random animator, think again when it's time to change sequence
		if ( m_bRandomAnimator )
		{
			SetNextThink( gpGlobals->curtime + m_flNextRandAnim + 0.1 );
		}
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CDynamicProp::InputSetAnimation( inputdata_t &inputdata )
{
	int nSequence = LookupSequence ( inputdata.value.String() );

	// Set to the desired anim, or default anim if the desired is not present
	if ( nSequence > ACTIVITY_NOT_AVAILABLE )
	{
		PropSetSequence( nSequence );

		// Fire output
		m_pOutputAnimBegun.FireOutput( NULL,this );
	}
	else
	{
		// Not available try to get default anim
		Msg( "Dynamic prop no sequence named:%s\n", inputdata.value.String() );
		SetSequence( 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the sequence and starts thinking.
// Input  : nSequence - 
//-----------------------------------------------------------------------------
void CDynamicProp::PropSetSequence( int nSequence )
{
	m_flCycle = 0;
	ResetSequence( nSequence );
	ResetClientsideFrame();

	RemoveFlag( FL_STATICPROP );
	SetThink( AnimThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


// NOTE: To avoid risk, currently these do nothing about collisions, only visually on/off
void CDynamicProp::InputTurnOn( inputdata_t &inputdata )
{
	m_fEffects &= ~EF_NODRAW;
}

void CDynamicProp::InputTurnOff( inputdata_t &inputdata )
{
	m_fEffects |= EF_NODRAW;
}

//-----------------------------------------------------------------------------
// Purpose: Ornamental prop that follows a studio
//-----------------------------------------------------------------------------
class COrnamentProp : public CDynamicProp
{
	DECLARE_CLASS( COrnamentProp, CDynamicProp );
public:
	DECLARE_DATADESC();

	void Spawn();
	void Activate();
	void AttachTo( const char *pAttachEntity, CBaseEntity *pActivator );
	void DetachFromOwner();

	// Input handlers
	void InputSetAttached( inputdata_t &inputdata );
	void InputDetach( inputdata_t &inputdata );

private:
	string_t	m_initialOwner;
};

LINK_ENTITY_TO_CLASS( prop_dynamic_ornament, COrnamentProp );	

BEGIN_DATADESC( COrnamentProp )

	DEFINE_KEYFIELD( COrnamentProp, m_initialOwner, FIELD_STRING, "InitialOwner" ),
	// Inputs
	DEFINE_INPUTFUNC( COrnamentProp,	FIELD_STRING,	"SetAttached",	InputSetAttached ),
	DEFINE_INPUTFUNC( COrnamentProp,	FIELD_VOID,		"Detach",	InputDetach ),

END_DATADESC()

void COrnamentProp::Spawn()
{
	BaseClass::Spawn();
	DetachFromOwner();
}

void COrnamentProp::DetachFromOwner()
{
	SetOwnerEntity( NULL );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	m_fEffects |= EF_NODRAW;
	UTIL_Relink(this);
}

void COrnamentProp::Activate()
{
	BaseClass::Activate();
	
	if ( m_initialOwner != NULL_STRING )
	{
		AttachTo( STRING(m_initialOwner), this );
	}
}

void COrnamentProp::InputSetAttached( inputdata_t &inputdata )
{
	AttachTo( inputdata.value.String(), inputdata.pActivator );
}

void COrnamentProp::AttachTo( const char *pAttachName, CBaseEntity *pActivator )
{
	// find and notify the new parent
	CBaseEntity *pAttach = gEntList.FindEntityByName( NULL, pAttachName, pActivator );
	if ( pAttach )
	{
		m_fEffects &= ~EF_NODRAW;
		FollowEntity( pAttach );
	}
}

void COrnamentProp::InputDetach( inputdata_t &inputdata )
{
	DetachFromOwner();
}


//=============================================================================================================
// PHYSICS PROPS
//=============================================================================================================
LINK_ENTITY_TO_CLASS( physics_prop, CPhysicsProp );
LINK_ENTITY_TO_CLASS( prop_physics, CPhysicsProp );	
LINK_ENTITY_TO_CLASS( prop_physics_override, CPhysicsProp );	

BEGIN_DATADESC( CPhysicsProp )

	DEFINE_INPUTFUNC( CPhysicsProp, FIELD_VOID, "EnableMotion", InputEnableMotion ),
	DEFINE_INPUTFUNC( CPhysicsProp, FIELD_VOID, "DisableMotion", InputDisableMotion ),
	DEFINE_INPUTFUNC( CPhysicsProp, FIELD_VOID, "Wake", InputWake ),
	DEFINE_INPUTFUNC( CPhysicsProp, FIELD_VOID, "Sleep", InputSleep ),
	DEFINE_INPUT( CPhysicsProp, m_fadeMinDist, FIELD_FLOAT, "fademindist" ),
	DEFINE_INPUT( CPhysicsProp, m_fadeMaxDist, FIELD_FLOAT, "fademaxdist" ),

	DEFINE_KEYFIELD( CPhysicsProp, m_massScale, FIELD_FLOAT, "massscale" ),
	DEFINE_KEYFIELD( CPhysicsProp, m_inertiaScale, FIELD_FLOAT, "inertiascale" ),
	DEFINE_KEYFIELD( CPhysicsProp, m_damageType, FIELD_INTEGER, "Damagetype" ),
	DEFINE_KEYFIELD( CPhysicsProp, m_iszOverrideScript, FIELD_STRING, "overridescript" ),

	DEFINE_KEYFIELD( CPhysicsProp, m_damageToEnableMotion, FIELD_INTEGER, "damagetoenablemotion" ), 
	DEFINE_KEYFIELD( CPhysicsProp, m_flForceToEnableMotion, FIELD_FLOAT, "forcetoenablemotion" ), 
	DEFINE_OUTPUT( CPhysicsProp, m_MotionEnabled, "OnMotionEnabled" ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPhysicsProp, DT_PhysicsProp )

	SendPropFloat( SENDINFO( m_fadeMinDist ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_fadeMaxDist ), 0, SPROP_NOSCALE ),

END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Purpose: Create a physics object for this prop
//-----------------------------------------------------------------------------
void CPhysicsProp::Spawn( )
{
	// Condense classname's to one, except for "prop_physics_override"
	if ( FClassnameIs( this, "physics_prop" ) )
	{
		SetClassname( "prop_physics" );
	}

	BaseClass::Spawn();

	if ( IsMarkedForDeletion() )
		return;

	// Now condense all classnames to one
	SetClassname( "prop_physics" );

	if ( HasSpawnFlags( SF_PHYSPROP_DEBRIS ) )
	{
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	}

	//m_fadeMinDist = 0; m_fadeMaxDist = 1300;

	CreateVPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPhysicsProp::CreateVPhysics()
{
	// Create the object in the physics system
	bool asleep = HasSpawnFlags( SF_PHYSPROP_START_ASLEEP ) ? true : false;

	solid_t tmpSolid;
	PhysModelParseSolid( tmpSolid, this, GetModelIndex() );
	
	if ( m_massScale > 0 )
	{
		float mass = tmpSolid.params.mass * m_massScale;
		mass = clamp( mass, 0.5, 1e6 );
		tmpSolid.params.mass = mass;
	}

	if ( m_inertiaScale > 0 )
	{
		tmpSolid.params.inertia *= m_inertiaScale;
		if ( tmpSolid.params.inertia < 0.5 )
			tmpSolid.params.inertia = 0.5;
	}

	PhysSolidOverride( tmpSolid, m_iszOverrideScript );

	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal( SOLID_VPHYSICS, 0, asleep, &tmpSolid );

	if ( !pPhysicsObject )
	{
		SetSolid( SOLID_NONE );
		SetMoveType( MOVETYPE_NONE );
		Warning("ERROR!: Can't create physics object for %s\n", STRING( GetModelName() ) );
		// update engine data
		Relink();
	}
	else
	{
		if ( m_damageType == 1 )
		{
			PhysSetGameFlags( pPhysicsObject, FVPHYSICS_DMG_SLICE );
		}
		if ( HasSpawnFlags( SF_PHYSPROP_MOTIONDISABLED ) || m_damageToEnableMotion > 0 || m_flForceToEnableMotion > 0 )
		{
			pPhysicsObject->EnableMotion( false );
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Input handler to start the physics prop simulating.
//-----------------------------------------------------------------------------
void CPhysicsProp::InputWake( inputdata_t &inputdata )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->Wake();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler to stop the physics prop simulating.
//-----------------------------------------------------------------------------
void CPhysicsProp::InputSleep( inputdata_t &inputdata )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->Sleep();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Enable physics motion and collision response (on by default)
//-----------------------------------------------------------------------------
void CPhysicsProp::InputEnableMotion( inputdata_t &inputdata )
{
	EnableMotion();
}

//-----------------------------------------------------------------------------
// Purpose: Disable any physics motion or collision response
//-----------------------------------------------------------------------------
void CPhysicsProp::InputDisableMotion( inputdata_t &inputdata )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->EnableMotion( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsProp::EnableMotion( void )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->Wake();
		pPhysicsObject->EnableMotion( true );

		m_MotionEnabled.FireOutput( this, this, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsProp::OnPhysGunPickup( CBasePlayer *pPhysGunUser )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject && !pPhysicsObject->IsMoveable() )
	{
		EnableMotion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsProp::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	// If we have a force to enable motion, and we're still disabled, check to see if this should enable us
	if ( m_flForceToEnableMotion )
	{
		// Large enough to enable motion?
		float flForce = pEvent->collisionSpeed * pEvent->pObjects[!index]->GetMass();
		if ( flForce >= m_flForceToEnableMotion )
		{
			EnableMotion();
			m_flForceToEnableMotion = 0;
		}
	}

	if ( !HasSpawnFlags( SF_PHYSPROP_DONT_TAKE_PHYSICS_DAMAGE ) )
	{
		int damageType = 0;
		float damage = CalculateDefaultPhysicsDamage( index, pEvent, m_impactEnergyScale, true, damageType );
		if ( damage > 0 )
		{
			CBaseEntity *pHitEntity = pEvent->pEntities[!index];
			if ( !pHitEntity )
			{
				// hit world
				pHitEntity = GetContainingEntity( INDEXENT(0) );
			}
			Vector damagePos;
			pEvent->pInternalData->GetContactPoint( damagePos );
			Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
			if ( damageForce == vec3_origin )
			{
				// This can happen if this entity is motion disabled, and can't move.
				// Use the velocity of the entity that hit us instead.
				damageForce = pEvent->postVelocity[!index] * pEvent->pObjects[!index]->GetMass();
			}

			PhysCallbackDamage( this, CTakeDamageInfo( pHitEntity, pHitEntity, damageForce, damagePos, damage, damageType ) );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPhysicsProp::OnTakeDamage( const CTakeDamageInfo &info )
{
	// note: if motion is disabled, OnTakeDamage can't apply physics force
	int ret = BaseClass::OnTakeDamage( info );
	
	// If we have a force to enable motion, and we're still disabled, check to see if this should enable us
	if ( m_flForceToEnableMotion )
	{
		// Large enough to enable motion?
		float flForce = info.GetDamageForce().Length();
		if ( flForce >= m_flForceToEnableMotion )
		{
			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				pPhysicsObject->Wake();
				pPhysicsObject->EnableMotion( true );
			}
			m_flForceToEnableMotion = 0;
		}
	}

	// Check our health against the threshold:
	if( m_damageToEnableMotion > 0 && GetHealth() < m_damageToEnableMotion )
	{
		// only do this once
		m_damageToEnableMotion = 0;

		EnableMotion();
		VPhysicsTakeDamage( info );
	}
	
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CPhysicsProp::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		if (VPhysicsGetObject())
		{
			char tempstr[512];
			Q_snprintf(tempstr, sizeof(tempstr),"Mass: %.2f kg / %.2f lb (%s)", VPhysicsGetObject()->GetMass(), kg2lbs(VPhysicsGetObject()->GetMass()), GetMassEquivalent(VPhysicsGetObject()->GetMass()));
			NDebugOverlay::EntityText(entindex(), text_offset, tempstr, 0);
			text_offset++;

			if ( !VPhysicsGetObject()->IsMoveable() )
			{
				Q_snprintf(tempstr, sizeof(tempstr),"Motion Disabled" );
				NDebugOverlay::EntityText(entindex(), text_offset, tempstr, 0);
				text_offset++;
			}

			if ( m_iszBasePropData != NULL_STRING )
			{
				Q_snprintf(tempstr, sizeof(tempstr),"Base PropData: %s", STRING(m_iszBasePropData) );
				NDebugOverlay::EntityText(entindex(), text_offset, tempstr, 0);
				text_offset++;
			}
		}
	}

	return text_offset;
}


//-----------------------------------------------------------------------------
// breakable prop functions
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// list of models to break into
struct breakmodel_t
{
	Vector		offset;
	char		modelName[1024];
	float		fadeTime;
	float		health;
	float		burstScale;
	int			collisionGroup;
};

class CBreakParser : public IVPhysicsKeyHandler
{
public:
	CBreakParser( float defaultBurstScale, int defaultCollisionGroup ) 
		: m_defaultBurstScale(defaultBurstScale), m_defaultCollisionGroup(defaultCollisionGroup) {}

	virtual void ParseKeyValue( void *pData, const char *pKey, const char *pValue )
	{
		breakmodel_t *pModel = (breakmodel_t *)pData;
		if ( !strcmp( pKey, "model" ) )
		{
			char tmp[1024];
			Q_strncpy( tmp, pValue, sizeof(pModel->modelName) );
			if ( strnicmp( tmp, "models/", 7 ) )
			{
				Q_strncpy( pModel->modelName, "models/" ,sizeof(pModel->modelName));
				Q_strncat( pModel->modelName, tmp, sizeof(pModel->modelName) );
			}
			else
			{
				Q_strncpy( pModel->modelName, tmp ,sizeof(pModel->modelName));
			}
			int len = strlen(pModel->modelName);
			if ( len < 4 || strcmpi( pModel->modelName + (len-4), ".mdl" ) )
			{
				Q_strncat( pModel->modelName, ".mdl", sizeof(pModel->modelName) );
			}
		}
		else if ( !strcmpi( pKey, "offset" ) )
		{
			UTIL_StringToVector( pModel->offset.Base(), pValue );
		}
		else if ( !strcmpi( pKey, "health" ) )
		{
			pModel->health = atof(pValue);
		}
		else if ( !strcmpi( pKey, "fadetime" ) )
		{
			pModel->fadeTime = atof(pValue);
			if ( !m_wroteCollisionGroup )
			{
				pModel->collisionGroup = COLLISION_GROUP_DEBRIS;
			}
		}
		else if ( !strcmpi( pKey, "debris" ) )
		{
			pModel->collisionGroup = atoi(pValue) > 0 ? COLLISION_GROUP_DEBRIS : COLLISION_GROUP_INTERACTIVE;
			m_wroteCollisionGroup = true;
		}
		else if ( !strcmpi( pKey, "burst" ) )
		{
			pModel->burstScale = atof( pValue );
		}
	}
	virtual void SetDefaults( void *pData ) 
	{
		breakmodel_t *pModel = (breakmodel_t *)pData;
		pModel->modelName[0] = 0;
		pModel->offset = vec3_origin;
		pModel->health = 1;
		pModel->fadeTime = 0;
		pModel->burstScale = m_defaultBurstScale;
		pModel->collisionGroup = m_defaultCollisionGroup;
		m_wroteCollisionGroup = false;
	}

private:
	int		m_defaultCollisionGroup;
	float	m_defaultBurstScale;
	bool	m_wroteCollisionGroup;
};

static void BreakModelList( CUtlVector<breakmodel_t> &list, int modelindex, float defBurstScale, int defCollisionGroup )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( modelindex );
	if ( !pCollide )
		return;

	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
	while ( !pParse->Finished() )
	{
		CBreakParser breakParser( defBurstScale, defCollisionGroup );
		
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcmpi( pBlock, "break" ) )
		{
			int index = list.AddToTail();
			breakmodel_t &breakModel = list[index];
			pParse->ParseCustom( &breakModel, &breakParser );
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );
}

static void BreakModelCreateSingle( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, 
	const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, int nSkin, const breakablepropparams_t &params )
{
	CBreakableProp *pEntity = (CBreakableProp *)CBaseEntity::CreateNoSpawn( "prop_physics", position, angles, pOwner );
	if ( pEntity )
	{
		pEntity->m_nSkin = nSkin;
		pEntity->SetModelName( AllocPooledString( pModel->modelName ) );
		pEntity->SetModel( STRING(pEntity->GetModelName()) );
		pEntity->m_iHealth = pModel->health;
		// UNDONE: Allow .qc to override spawnflags for child pieces
		if ( pOwner )
		{
			pEntity->AddSpawnFlags( pOwner->GetSpawnFlags() );

			// We never want to be motion disabled
			pEntity->RemoveSpawnFlags( SF_PHYSPROP_MOTIONDISABLED );
		}
		pEntity->m_impactEnergyScale = params.impactEnergyScale;	// assume the same material
		pEntity->SetCollisionGroup( pModel->collisionGroup );
		
		// Inherit the base object's damage modifiers
		CBreakableProp *pBreakableOwner = dynamic_cast<CBreakableProp *>(pOwner);
		if ( pBreakableOwner )
		{
			pEntity->SetDmgModBullet( pBreakableOwner->GetDmgModBullet() );
			pEntity->SetDmgModClub( pBreakableOwner->GetDmgModClub() );
			pEntity->SetDmgModExplosive( pBreakableOwner->GetDmgModExplosive() );
		}
		pEntity->Spawn();
		if ( pModel->fadeTime )
		{
			pEntity->StartFadeOut( pModel->fadeTime );
		}

		IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();
		if ( pPhysics )
		{
			pPhysics->SetVelocity( &velocity, &angVelocity );
		}
		else
		{
			// failed to create a physics object
			UTIL_Remove( pEntity );
		}
	}
}

class CBreakModelsPrecached : public CAutoGameSystem
{
public:
	CBreakModelsPrecached()
	{
		m_modelList.SetLessFunc( BreakLessFunc );
	}

	static bool BreakLessFunc( const string_t &lhs, const string_t &rhs )
	{
		return ( lhs.ToCStr() < rhs.ToCStr() );
	}

	bool IsInList( string_t modelName )
	{
		if ( m_modelList.Find(modelName) != m_modelList.InvalidIndex() )
			return true;

		return false;
	}

	void AddToList( string_t modelName )
	{
		m_modelList.Insert( modelName );
	}

	void LevelShutdownPostEntity()
	{
		m_modelList.RemoveAll();
	}

private:
	CUtlRBTree<string_t>	m_modelList;
};

static CBreakModelsPrecached g_BreakModelsPrecached;

void PropBreakablePrecacheAll( string_t modelName )
{
	if ( g_BreakModelsPrecached.IsInList( modelName ) )
		return;

	g_BreakModelsPrecached.AddToList( modelName );
	int modelIndex = engine->PrecacheModel( STRING(modelName) );

	CUtlVector<breakmodel_t> list;

	BreakModelList( list, modelIndex, COLLISION_GROUP_NONE, 0 );
	for ( int i = 0; i < list.Count(); i++ )
	{
		string_t modelName = AllocPooledString(list[i].modelName);
		PropBreakablePrecacheAll( modelName );
	}
}

void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const breakablepropparams_t &params )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( modelindex );
	if ( !pCollide )
		return;

	int nSkin = 0;
	CBaseEntity *pOwnerEntity = NULL; 
	if ( pPhysics )
	{
		pOwnerEntity = static_cast<CBaseEntity *>(pPhysics->GetGameData());
		CBaseAnimating *pAnim = dynamic_cast<CBaseAnimating*>(pOwnerEntity);
		if ( pAnim )
		{
			nSkin = pAnim->m_nSkin;
		}

	}
	matrix3x4_t localToWorld;

	studiohdr_t *pStudioHdr = NULL;
	const model_t *model = modelinfo->GetModel( modelindex );
	if ( model )
	{
		pStudioHdr = static_cast< studiohdr_t * >( modelinfo->GetModelExtraData( model ) );
	}

	Vector parentOrigin = vec3_origin;
	int parentAttachment = 	Studio_FindAttachment( pStudioHdr, "placementOrigin" ) + 1;
	if ( parentAttachment > 0 )
	{
		GetAttachmentLocalSpace( pStudioHdr, parentAttachment, localToWorld );
		MatrixGetColumn( localToWorld, 3, parentOrigin );
	}
	else
	{
		AngleMatrix( vec3_angle, localToWorld );
	}
	
	matrix3x4_t matrix;
	AngleMatrix( params.angles, params.origin, matrix );
	CUtlVector<breakmodel_t> list;

	BreakModelList( list, modelindex, params.defBurstScale, params.defCollisionGroup );

	for ( int i = 0; i < list.Count(); i++ )
	{
		int modelIndex = modelinfo->GetModelIndex( list[i].modelName );
		if ( modelIndex <= 0 )
			continue;
		
		studiohdr_t *pStudioHdr = NULL;
		const model_t *model = modelinfo->GetModel( modelIndex );
		if ( model )
		{
			pStudioHdr = static_cast< studiohdr_t * >( modelinfo->GetModelExtraData( model ) );
		}

		int placementIndex = Studio_FindAttachment( pStudioHdr, "placementOrigin" ) + 1;
		Vector placementOrigin = parentOrigin;
		if ( placementIndex > 0 )
		{
			GetAttachmentLocalSpace( pStudioHdr, placementIndex, localToWorld );
			MatrixGetColumn( localToWorld, 3, placementOrigin );
			placementOrigin -= parentOrigin;
		}

		Vector position;
		VectorTransform( list[i].offset - placementOrigin, matrix, position );
		Vector objectVelocity = params.velocity;

		if (pPhysics)
		{
			pPhysics->GetVelocityAtPoint( position, objectVelocity );
		}
		if( list[i].burstScale != 0.0 )
		{
			// If burst scale is set, this piece should 'burst' away from
			// the origin in addition to travelling in the wished velocity.
			Vector vecBurstDir;

			vecBurstDir = position - params.origin;

			VectorNormalize( vecBurstDir );

			objectVelocity += vecBurstDir * list[i].burstScale;
		}

		int nActualSkin = nSkin;
		if ( nActualSkin > pStudioHdr->numskinfamilies )
			nActualSkin = 0;

		BreakModelCreateSingle( pOwnerEntity, &list[i], position, params.angles, objectVelocity, params.angularVelocity, nActualSkin, params );
	}
}

void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const Vector &origin, const QAngle &angles, const Vector &velocity, const AngularImpulse &angularVelocity, float impactEnergyScale, float defBurstScale, int defCollisionGroup )
{
	breakablepropparams_t params( origin, angles, velocity, angularVelocity );
	params.impactEnergyScale = impactEnergyScale;
	params.defBurstScale = defBurstScale;
	params.defCollisionGroup = defCollisionGroup;
	PropBreakableCreateAll( modelindex, pPhysics, params );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a string describing a real-world equivalent mass.
// Input  : flMass - mass in kg
//-----------------------------------------------------------------------------
const char *GetMassEquivalent(float flMass)
{
	static struct
	{
		float flMass;
		char *sz;
	} masstext[] =
	{
		{ 5e-6,		"snowflake" },
		{ 2.5e-3,	"ping-pong ball" },
		{ 5e-3,		"penny" },
		{ 0.2,		"mouse" },
		{ 0.05,		"golf ball" },
		{ 0.17,		"billard ball" },
		{ 2,		"bag of sugar" },
		{ 7,		"male cat" },
		{ 10,		"bowling ball" },
		{ 30,		"dog" },
		{ 60,		"cheetah" },
		{ 90,		"adult male human" },
		{ 250,		"refrigerator" },
		{ 600,		"race horse" },
		{ 1000,		"small car" },
		{ 1650,		"medium car" },
		{ 2500,		"large car" },
		{ 6000,		"t-rex" },
		{ 7200,		"elephant" },
		{ 8e4,		"space shuttle" },
		{ 2e5,		"loaded boxcar" },
		{ 7e5,		"locomotive" },
		{ 1036,		"Eiffel tower" },
		{ 6e24,		"the Earth" },
		{ 7e24,		"really freaking heavy" },
	};

	for (int i = 0; i < sizeof(masstext) / sizeof(masstext[0]) - 1; i++)
	{
		if (flMass < masstext[i].flMass)
		{
			return masstext[i].sz;
		}
	}

	return masstext[ sizeof(masstext) / sizeof(masstext[0]) - 1 ].sz;
}


//=============================================================================================================
// BASE PROP DOOR
//=============================================================================================================
#define	SF_DOOR_START_OPEN		1		// Door is initially open (by default, doors start closed).
#define SF_DOOR_LOCKED			2048	// Door is initially locked.
#define SF_DOOR_SILENT			4096	// Door makes no sounds, despite the settings for sound files.
#define	SF_DOOR_USE_CLOSES		8192	// Door can be +used to close before its autoreturn delay has expired.

//
// Private activities.
//
static int ACT_DOOR_OPEN = 0;
static int ACT_DOOR_LOCKED = 0;

//
// Anim events.
//
enum
{
	AE_DOOR_OPEN = 1,	// The door should start opening.
};


void PlayLockSounds(CBaseEntity *pEdict, locksound_t *pls, int flocked, int fbutton);

BEGIN_DATADESC_NO_BASE(locksound_t)

	DEFINE_FIELD( locksound_t, sLockedSound,	FIELD_STRING),
	DEFINE_FIELD( locksound_t, sLockedSentence,	FIELD_STRING ),
	DEFINE_FIELD( locksound_t, sUnlockedSound,	FIELD_STRING ),
	DEFINE_FIELD( locksound_t, sUnlockedSentence, FIELD_STRING ),
	DEFINE_FIELD( locksound_t, iLockedSentence, FIELD_INTEGER ),
	DEFINE_FIELD( locksound_t, iUnlockedSentence, FIELD_INTEGER ),
	DEFINE_FIELD( locksound_t, flwaitSound,		FIELD_FLOAT ),
	DEFINE_FIELD( locksound_t, flwaitSentence,	FIELD_FLOAT ),
	DEFINE_FIELD( locksound_t, bEOFLocked,		FIELD_CHARACTER ),
	DEFINE_FIELD( locksound_t, bEOFUnlocked,	FIELD_CHARACTER ),

END_DATADESC()


BEGIN_DATADESC(CBasePropDoor)
	//DEFINE_FIELD(CBasePropDoor, m_bLockedSentence, FIELD_CHARACTER),
	//DEFINE_FIELD(CBasePropDoor, m_bUnlockedSentence, FIELD_CHARACTER),	
	DEFINE_KEYFIELD(CBasePropDoor, m_nHardwareType, FIELD_INTEGER, "hardware"),
	DEFINE_KEYFIELD(CBasePropDoor, m_flAutoReturnDelay, FIELD_FLOAT, "returndelay"),
	DEFINE_FIELD( CBasePropDoor, m_hActivator, FIELD_EHANDLE ),
	DEFINE_KEYFIELD(CBasePropDoor, m_SoundMoving, FIELD_SOUNDNAME, "soundmove"),
	DEFINE_KEYFIELD(CBasePropDoor, m_SoundOpen, FIELD_SOUNDNAME, "soundopen"),
	DEFINE_KEYFIELD(CBasePropDoor, m_SoundClose, FIELD_SOUNDNAME, "soundclose"),
	DEFINE_KEYFIELD(CBasePropDoor, m_ls.sLockedSound, FIELD_SOUNDNAME, "soundlocked"),
	DEFINE_KEYFIELD(CBasePropDoor, m_ls.sUnlockedSound, FIELD_SOUNDNAME, "soundunlocked"),
	DEFINE_FIELD(CBasePropDoor, m_bLocked, FIELD_BOOLEAN),
	//DEFINE_KEYFIELD(CBasePropDoor, m_flBlockDamage, FIELD_FLOAT, "dmg"),
	DEFINE_FIELD(CBasePropDoor, m_eDoorState, FIELD_INTEGER),

	DEFINE_INPUTFUNC(CBasePropDoor, FIELD_VOID, "Open", InputOpen),
	DEFINE_INPUTFUNC(CBasePropDoor, FIELD_VOID, "Close", InputClose),
	DEFINE_INPUTFUNC(CBasePropDoor, FIELD_VOID, "Toggle", InputToggle),
	DEFINE_INPUTFUNC(CBasePropDoor, FIELD_VOID, "Lock", InputLock),
	DEFINE_INPUTFUNC(CBasePropDoor, FIELD_VOID, "Unlock", InputUnlock),

	DEFINE_OUTPUT(CBasePropDoor, m_OnBlockedOpening, "OnBlockedOpening"),
	DEFINE_OUTPUT(CBasePropDoor, m_OnBlockedClosing, "OnBlockedClosing"),
	DEFINE_OUTPUT(CBasePropDoor, m_OnUnblockedOpening, "OnUnblockedOpening"),
	DEFINE_OUTPUT(CBasePropDoor, m_OnUnblockedClosing, "OnUnblockedClosing"),
	DEFINE_OUTPUT(CBasePropDoor, m_OnFullyClosed, "OnFullyClosed"),
	DEFINE_OUTPUT(CBasePropDoor, m_OnFullyOpen, "OnFullyOpen"),
	DEFINE_OUTPUT(CBasePropDoor, m_OnClose, "OnClose"),
	DEFINE_OUTPUT(CBasePropDoor, m_OnOpen, "OnOpen"),

	DEFINE_EMBEDDED( CBasePropDoor, m_ls ),
	DEFINE_FIELD( CBasePropDoor, m_hOpeningNPC, FIELD_EHANDLE ),

	// Function Pointers
	DEFINE_FUNCTION(CBasePropDoor, DoorOpen),
	DEFINE_FUNCTION(CBasePropDoor, DoorClose),
	DEFINE_FUNCTION(CBasePropDoor, DoorOpened),
	DEFINE_FUNCTION(CBasePropDoor, DoorClosed),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::Spawn()
{
	BaseClass::Spawn();

	if (HasSpawnFlags(SF_DOOR_START_OPEN))
	{
		SetDoorOpen();
	}
	else
	{
		SetDoorClosed();
	}

	if (HasSpawnFlags(SF_DOOR_LOCKED))
	{
		m_bLocked = true;
	}

	SetMoveType(MOVETYPE_PUSH);
	Relink();
	
	if (m_flSpeed == 0)
	{
		m_flSpeed = 100;
	}
	
	RemoveFlag(FL_STATICPROP);

	SetSolid(SOLID_VPHYSICS);
	VPhysicsInitShadow(false, false);

	if (m_nHardwareType != 0)
	{
		SetBodygroup( m_nHardwareType, true );
	}
	else if (!HasSpawnFlags(SF_DOOR_LOCKED))
	{
		// Doors with no hardware must always be locked.
		DevWarning(1, "Unlocked prop_door '%s' at (%.0f %.0f %.0f) has no hardware. All openable doors must have hardware!\n", GetDebugName(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::Precache(void)
{
	BaseClass::Precache();

	RegisterPrivateActivities();

	UTIL_ValidateSoundName( m_SoundMoving, "common/null.wav" );
	UTIL_ValidateSoundName( m_SoundOpen, "common/null.wav" );
	UTIL_ValidateSoundName( m_SoundClose, "common/null.wav" );
	UTIL_ValidateSoundName( m_ls.sLockedSound, "common/null.wav" );
	UTIL_ValidateSoundName( m_ls.sUnlockedSound, "common/null.wav" );

	enginesound->PrecacheSound(STRING(m_SoundMoving));
	enginesound->PrecacheSound(STRING(m_SoundOpen));
	enginesound->PrecacheSound(STRING(m_SoundClose));
	enginesound->PrecacheSound(STRING(m_ls.sLockedSound));
	enginesound->PrecacheSound(STRING(m_ls.sUnlockedSound));
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::RegisterPrivateActivities(void)
{
	static bool bRegistered = false;

	if (bRegistered)
		return;

	REGISTER_PRIVATE_ACTIVITY( ACT_DOOR_OPEN );
	REGISTER_PRIVATE_ACTIVITY( ACT_DOOR_LOCKED );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::Activate(void)
{
	BaseClass::Activate();
	UpdateAreaPortals(IsDoorOpen());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::HandleAnimEvent(animevent_t *pEvent)
{
	// Opening is called here via an animation event if the open sequence has one,
	// otherwise it is called immediately when the open sequence is set.
	if (pEvent->event == AE_DOOR_OPEN)
	{
		DoorActivate();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : isOpen - 
//-----------------------------------------------------------------------------
void CBasePropDoor::UpdateAreaPortals(bool isOpen)
{
	string_t name = GetEntityName();
	if (!name)
		return;
	
	CBaseEntity *pPortal = NULL;
	while (pPortal = gEntList.FindEntityByClassname(pPortal, "func_areaportal"))
	{
		if (pPortal->HasTarget(name))
		{
			// USE_ON means open the portal, off means close it
			pPortal->Use(this, this, isOpen?USE_ON:USE_OFF, 0);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called when the player uses the door.
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CBasePropDoor::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	m_hActivator = pActivator;

	if (IsDoorClosed() || (IsDoorOpen() && HasSpawnFlags(SF_DOOR_USE_CLOSES)))
	{
		// Ready to be opened or closed.
		if (m_bLocked)
		{
			PropSetSequence(SelectWeightedSequence((Activity)ACT_DOOR_LOCKED));
			PlayLockSounds(this, &m_ls, TRUE, FALSE);
		}
		else
		{
			int nSequence = SelectWeightedSequence((Activity)ACT_DOOR_OPEN);
			PropSetSequence(nSequence);

			if ((nSequence == -1) || !HasAnimEvent(nSequence, AE_DOOR_OPEN))
			{
				// No open anim event, we need to open the door here.
				DoorActivate();
			}
		}
	}
	else if (IsDoorOpening())
	{
		DoorClose();
	}
	else if (IsDoorClosing())
	{
		DoorOpen();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Closes the door if it is not already closed.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputClose(inputdata_t &inputdata)
{
	if (!IsDoorClosed())
	{	
		m_OnClose.FireOutput(inputdata.pActivator, this);
		DoorClose();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that locks the door.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputLock(inputdata_t &inputdata)
{
	Lock();
}


//-----------------------------------------------------------------------------
// Purpose: Opens the door if it is not already open.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputOpen(inputdata_t &inputdata)
{
	// I'm locked, can't open
	if (m_bLocked)
		return; 

	if (!IsDoorOpen() && !IsDoorOpening())
	{	
		// Play door unlock sounds.
		PlayLockSounds(this, &m_ls, false, false);
		m_OnOpen.FireOutput(inputdata.pActivator, this);
		DoorOpen();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Opens the door if it is not already open.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputToggle(inputdata_t &inputdata)
{
	if (IsDoorClosed())
	{	
		// I'm locked, can't open
		if (m_bLocked)
			return; 

		DoorOpen();
	}
	else if (IsDoorOpen())
	{
		DoorClose();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that unlocks the door.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputUnlock(inputdata_t &inputdata)
{
	Unlock();
}


//-----------------------------------------------------------------------------
// Purpose: Locks the door so that it cannot be opened.
//-----------------------------------------------------------------------------
void CBasePropDoor::Lock(void)
{
	m_bLocked = true;
}


//-----------------------------------------------------------------------------
// Purpose: Unlocks the door so that it can be opened.
//-----------------------------------------------------------------------------
void CBasePropDoor::Unlock(void)
{
	if (!m_nHardwareType)
	{
		// Doors with no hardware must always be locked.
		DevWarning(1, "Unlocking prop_door '%s' at (%.0f %.0f %.0f) with no hardware. All openable doors must have hardware!\n", GetDebugName(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
	}

	m_bLocked = false;
}


//-----------------------------------------------------------------------------
// Purpose: Causes the door to "do its thing", i.e. start moving, and cascade activation.
//-----------------------------------------------------------------------------
bool CBasePropDoor::DoorActivate()
{
	if (IsDoorOpen())
	{
		DoorClose();
	}
	else
	{
		PlayLockSounds(this, &m_ls, FALSE, FALSE);
		DoorOpen();
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Starts the door opening.
//-----------------------------------------------------------------------------
void CBasePropDoor::DoorOpen()
{
	UpdateAreaPortals(true);

	// It could be going-down, if blocked.
	ASSERT(IsDoorClosed() || IsDoorClosing());

	// Emit door moving and stop sounds on CHAN_STATIC so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		CPASAttenuationFilter filter(this);
		filter.MakeReliable();
		EmitSound(filter, entindex(), CHAN_STATIC, (char *)STRING(m_SoundMoving), 1, ATTN_NORM);
	}

	m_eDoorState = DOOR_STATE_OPENING;
	
	SetMoveDone(&CBasePropDoor::DoorOpened);
	BeginOpening();
	m_OnOpen.FireOutput(this, this);
}


//-----------------------------------------------------------------------------
// Purpose: The door has reached the "up" position.  Either go back down, or
//			wait for another activation.
//-----------------------------------------------------------------------------
void CBasePropDoor::DoorOpened(void)
{
	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		CPASAttenuationFilter filter(this);
		filter.MakeReliable();
		StopSound(entindex(), CHAN_STATIC, (char*)STRING(m_SoundMoving));
		EmitSound(filter, entindex(), CHAN_STATIC, (char *)STRING(m_SoundOpen), 1, ATTN_NORM);
	}

	ASSERT(IsDoorOpening());
	m_eDoorState = DOOR_STATE_OPEN;
	
	if (WillAutoReturn())
	{
		// In flWait seconds, DoorClose will fire, unless wait is -1, then door stays open
		SetMoveDoneTime(m_flAutoReturnDelay + 0.1);
		SetMoveDone(&CBasePropDoor::DoorClose);

		if (m_flAutoReturnDelay == -1)
		{
			SetNextThink( TICK_NEVER_THINK );
		}
	}

	CAI_BaseNPC *pNPC = m_hOpeningNPC;
	if (pNPC)
	{
		// Notify the NPC that opened us.
		pNPC->OnDoorFullyOpen(this);
	}

	m_OnFullyOpen.FireOutput(this, this);
}


//-----------------------------------------------------------------------------
// Purpose: Starts the door closing.
//-----------------------------------------------------------------------------
void CBasePropDoor::DoorClose(void)
{
	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		CPASAttenuationFilter filter(this);
		filter.MakeReliable();
		EmitSound(filter, entindex(), CHAN_STATIC, (char *)STRING(m_SoundMoving), 1, ATTN_NORM);
	}
	
	ASSERT(IsDoorOpen() || IsDoorOpening());
	m_eDoorState = DOOR_STATE_CLOSING;

	SetMoveDone(&CBasePropDoor::DoorClosed);

	BeginClosing();

	m_OnClose.FireOutput(this, this);
}


//-----------------------------------------------------------------------------
// Purpose: The door has reached the "down" position.  Back to quiescence.
//-----------------------------------------------------------------------------
void CBasePropDoor::DoorClosed(void)
{
	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		CPASAttenuationFilter filter(this);
		filter.MakeReliable();
	
		StopSound(entindex(), CHAN_STATIC, (char *)STRING(m_SoundMoving));
		EmitSound(filter, entindex(), CHAN_STATIC, (char *)STRING(m_SoundClose), 1, ATTN_NORM);
	}

	ASSERT(IsDoorClosing());
	m_eDoorState = DOOR_STATE_CLOSED;

	m_OnFullyClosed.FireOutput(m_hActivator, this);
	UpdateAreaPortals(false);
}


//-----------------------------------------------------------------------------
// Purpose: Called the first frame that the door is blocked while opening or closing.
// Input  : pOther - The blocking entity.
//-----------------------------------------------------------------------------
void CBasePropDoor::StartBlocked(CBaseEntity *pOther)
{
	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		CPASAttenuationFilter filter(this);
		filter.MakeReliable();
		StopSound(entindex(), CHAN_STATIC, (char*)STRING(m_SoundMoving));
	}

	//
	// Fire whatever events we need to due to our blocked state.
	//
	if (IsDoorClosing())
	{
		m_OnBlockedClosing.FireOutput(pOther, this);
	}
	else
	{
		CAI_BaseNPC *pNPC = m_hOpeningNPC;
		if (pNPC)
		{
			// Notify the NPC that tried to open us.
			pNPC->OnDoorBlocked(this);
		}

		m_OnBlockedOpening.FireOutput(pOther, this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame when the door is blocked while opening or closing.
// Input  : pOther - The blocking entity.
//-----------------------------------------------------------------------------
void CBasePropDoor::Blocked(CBaseEntity *pOther)
{
	// dvs: TODO: will prop_door apply any blocking damage?
	// Hurt the blocker a little.
	//if (m_flBlockDamage)
	//{
	//	pOther->TakeDamage(CTakeDamageInfo(this, this, m_flBlockDamage, DMG_CRUSH));
	//}

	// If a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast.
//	if (m_flAutoReturnDelay >= 0)
//	{
//		if (IsDoorClosing())
//		{
//			DoorOpen();
//		}
//		else
//		{
//			DoorClose();
//		}
//	}

	// Block all door pieces with the same targetname here.
//	if (GetEntityName() != NULL_STRING)
//	{
//		CBaseEntity pTarget = NULL;
//		for (;;)
//		{
//			pTarget = gEntList.FindEntityByName(pTarget, GetEntityName(), NULL);
//
//			if (pTarget != this)
//			{
//				if (!pTarget)
//					break;
//
//				if (FClassnameIs(pTarget, "prop_door_rotating"))
//				{
//					CPropDoorRotating *pDoor = (CPropDoorRotating *)pTarget;
//
//					if (pDoor->m_fAutoReturnDelay >= 0)
//					{
//						if (pDoor->GetAbsVelocity() == GetAbsVelocity() && pDoor->GetLocalAngularVelocity() == GetLocalAngularVelocity())
//						{
//							// this is the most hacked, evil, bastardized thing I've ever seen. kjb
//							if (FClassnameIs(pTarget, "prop_door_rotating"))
//							{
//								// set angles to realign rotating doors
//								pDoor->SetLocalAngles(GetLocalAngles());
//								pDoor->SetLocalAngularVelocity(vec3_angle);
//							}
//							else
//							//{
//							//	// set origin to realign normal doors
//							//	pDoor->SetLocalOrigin(GetLocalOrigin());
//							//	pDoor->SetAbsVelocity(vec3_origin);// stop!
//							//}
//						}
//
//						if (IsDoorClosing())
//						{
//							pDoor->DoorOpen();
//						}
//						else
//						{
//							pDoor->DoorClose();
//						}
//					}
//				}
//			}
//		}
//	}
}


//-----------------------------------------------------------------------------
// Purpose: Called the first frame that the door is unblocked while opening or closing.
//-----------------------------------------------------------------------------
void CBasePropDoor::EndBlocked(void)
{
	// Emit door moving and stop sounds on CHAN_STATIC so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		CPASAttenuationFilter filter(this);
		filter.MakeReliable();
		EmitSound(filter, entindex(), CHAN_STATIC, (char *)STRING(m_SoundMoving), 1, ATTN_NORM);
	}

	//
	// Fire whatever events we need to due to our unblocked state.
	//
	if (IsDoorClosing())
	{
		m_OnUnblockedClosing.FireOutput(this, this);
	}
	else
	{
		m_OnUnblockedOpening.FireOutput(this, this);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pNPC - 
//-----------------------------------------------------------------------------
bool CBasePropDoor::NPCOpenDoor(CAI_BaseNPC *pNPC)
{
	// dvs: TODO: use activator filter here
	// dvs: TODO: outboard entity containing rules for whether door is operable?
	if (IsDoorClosed())
	{
		// Remember who is opening us so we can notify them.
		m_hOpeningNPC = pNPC;
		DoorOpen();
	}

	return true;
}


class CPropDoorRotating : public CBasePropDoor
{
	typedef CBasePropDoor BaseClass;

	void Spawn();

	void MoveDone();

	void BeginOpening();
	void BeginClosing();

	void SetDoorOpen();
	void SetDoorClosed();

	void GetNPCOpenData(CAI_BaseNPC *pNPC, opendata_t &opendata);
	float GetOpenInterval();

	DECLARE_DATADESC();
private:

	void AngularMove(const QAngle &vecDestAngle, float flSpeed);

	Vector m_vecAxis;				// The axis of rotation.
	float m_flDistance;				// How many degrees we rotate between open and closed.
	QAngle m_angRotationClosed;		// Our angles when we are fully closed.
	QAngle m_angRotationOpen;		// Our angles when we are fully open.

	QAngle m_angGoal;

};


BEGIN_DATADESC(CPropDoorRotating)
	DEFINE_KEYFIELD(CPropDoorRotating, m_vecAxis, FIELD_VECTOR, "axis"),
	DEFINE_KEYFIELD(CPropDoorRotating, m_flDistance, FIELD_FLOAT, "distance"),
	DEFINE_FIELD( CPropDoorRotating, m_angRotationClosed, FIELD_VECTOR ),
	DEFINE_FIELD( CPropDoorRotating, m_angRotationOpen, FIELD_VECTOR ),
	DEFINE_FIELD( CPropDoorRotating, m_angGoal, FIELD_VECTOR ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::Spawn()
{
	// Doors are built closed, so save the current angles as the closed angles.
	m_angRotationClosed = GetLocalAngles();

	// The axis of rotation must be axial for now.
	// dvs: TODO: finalize data definition of hinge axis
	// HACK: convert the axis of rotation to dPitch dYaw dRoll
	m_vecAxis = Vector(0, 0, 1);
	VectorNormalize(m_vecAxis);
	ASSERT((m_vecAxis.x == 0 && m_vecAxis.y == 0) ||
			(m_vecAxis.y == 0 && m_vecAxis.z == 0) ||
			(m_vecAxis.z == 0 && m_vecAxis.x == 0));
	Vector vecMoveDir(m_vecAxis.y, m_vecAxis.z, m_vecAxis.x);

	if (m_flDistance == 0)
	{
		m_flDistance = 90;
	}

	// Calculate our orientation when we are fully open.
	m_angRotationOpen.x = m_angRotationClosed.x + (vecMoveDir.x * m_flDistance);
	m_angRotationOpen.y = m_angRotationClosed.y + (vecMoveDir.y * m_flDistance);
	m_angRotationOpen.z = m_angRotationClosed.z + (vecMoveDir.z * m_flDistance);

	SetLocalAngularVelocity(QAngle(0, 100, 0));

	// Call this last! It relies on stuff we calculated above.
	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::SetDoorOpen()
{
	SetLocalAngles(m_angRotationOpen);
	m_eDoorState = DOOR_STATE_OPEN;

	// Doesn't relink; that's done in Spawn.
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::SetDoorClosed()
{
	SetLocalAngles(m_angRotationClosed);
	m_eDoorState = DOOR_STATE_CLOSED;

	// Doesn't relink; that's done in Spawn.
}


//-----------------------------------------------------------------------------
// Purpose: After rotating, set angle to exact final angle, call "move done" function.
//-----------------------------------------------------------------------------
void CPropDoorRotating::MoveDone()
{
	SetLocalAngles(m_angGoal);
	SetLocalAngularVelocity(vec3_angle);
	SetMoveDoneTime(-1);
	BaseClass::MoveDone();
}


//-----------------------------------------------------------------------------
// Purpose: Calculate m_vecVelocity and m_flNextThink to reach vecDest from
//			GetLocalOrigin() traveling at flSpeed. Just like LinearMove, but rotational.
// Input  : vecDestAngle - 
//			flSpeed - 
//-----------------------------------------------------------------------------
void CPropDoorRotating::AngularMove(const QAngle &vecDestAngle, float flSpeed)
{
	ASSERTSZ(flSpeed != 0, "AngularMove:  no speed is defined!");
	
	m_angGoal = vecDestAngle;

	// Already there?
	if (vecDestAngle == GetLocalAngles())
	{
		MoveDone();
		return;
	}
	
	// Set destdelta to the vector needed to move.
	QAngle vecDestDelta = vecDestAngle - GetLocalAngles();
	
	// Divide by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// Call MoveDone when destination angles are reached.
	SetMoveDoneTime(flTravelTime);

	// Scale the destdelta vector by the time spent traveling to get velocity.
	SetLocalAngularVelocity(vecDestDelta * (1.0 / flTravelTime));
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::BeginOpening()
{
	if (m_hActivator != NULL)
	{
		// dvs: TODO: two-way doors should use their forward vector to determine open dir
		//float sign = 1.0;
		//if (!HasSpawnFlags(SF_DOOR_ONEWAY) && vecMoveDir.y) 		// Y axis rotation, move away from the player
		//{
		//	Vector vec = m_hActivator->GetLocalOrigin() - GetLocalOrigin();
		//	QAngle angles = m_hActivator->GetLocalAngles();
		//	angles.x = 0;
		//	angles.z = 0;
		//	Vector forward;
		//	AngleVectors(angles, &forward );
		//	Vector vnext = (m_hActivator->GetLocalOrigin() + (forward * 10)) - GetLocalOrigin();
		//	if ((vec.x*vnext.y - vec.y*vnext.x) < 0)
		//	{
		//		sign = -1.0;
		//	}
		//}
	}

	AngularMove(m_angRotationOpen, m_flSpeed);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::BeginClosing()
{
	AngularMove(m_angRotationClosed, m_flSpeed);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vecMoveDir - 
//			opendata - 
//-----------------------------------------------------------------------------
void CPropDoorRotating::GetNPCOpenData(CAI_BaseNPC *pNPC, opendata_t &opendata)
{
	// dvs: TODO: finalize open position, direction, activity
	Vector vecForward;
	Vector vecRight;
	AngleVectors(GetAbsAngles(), &vecForward, &vecRight, NULL);

	//
	// Figure out where the NPC should stand to open this door,
	// and what direction they should face.
	//
	opendata.vecStandPos = GetAbsOrigin() - (vecRight * 24);
	opendata.vecStandPos.z -= 54;

	Vector vecNPCOrigin = pNPC->GetAbsOrigin();

	if (pNPC->GetAbsOrigin().Dot(vecForward) > GetAbsOrigin().Dot(vecForward))
	{
		// In front of the door relative to the door's forward vector.
		opendata.vecStandPos += vecForward * 64;
		opendata.vecFaceDir = -vecForward;
	}
	else
	{
		// Behind the door relative to the door's forward vector.
		opendata.vecStandPos -= vecForward * 64;
		opendata.vecFaceDir = vecForward;
	}

	opendata.eActivity = ACT_MELEE_ATTACK_SWING;
}


//-----------------------------------------------------------------------------
// Purpose: Returns how long it will take this door to open.
//-----------------------------------------------------------------------------
float CPropDoorRotating::GetOpenInterval()
{
	// set destdelta to the vector needed to move
	QAngle vecDestDelta = m_angRotationOpen - GetLocalAngles();
	
	// divide by speed to get time to reach dest
	return vecDestDelta.Length() / m_flSpeed;
}


LINK_ENTITY_TO_CLASS(prop_door_rotating, CPropDoorRotating);


// Debug sphere
class CPhysSphere : public CPhysicsProp
{
	DECLARE_CLASS( CPhysSphere, CPhysicsProp );
public:
	virtual bool OverridePropdata() { return true; }
	bool CreateVPhysics()
	{
		SetSolid( SOLID_BBOX );
		SetCollisionBounds( -Vector(12,12,12), Vector(12,12,12) );
		objectparams_t params = g_PhysDefaultObjectParams;
		params.pGameData = static_cast<void *>(this);
		IPhysicsObject *pPhysicsObject = physenv->CreateSphereObject( 12, 0, GetAbsOrigin(), GetAbsAngles(), &params, false );
		if ( pPhysicsObject )
		{
			VPhysicsSetObject( pPhysicsObject );
			SetMoveType( MOVETYPE_VPHYSICS );
			pPhysicsObject->Wake();
			Relink();
		}
	
		return true;
	}
};

LINK_ENTITY_TO_CLASS( prop_sphere, CPhysSphere );

