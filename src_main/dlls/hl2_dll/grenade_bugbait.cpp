//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Bugbait weapon to summon and direct antlions
//
//=============================================================================

#include "cbase.h"
#include "grenade_bugbait.h"
#include "decals.h"
#include "smoke_trail.h"
#include "soundent.h"
#include "engine/IEngineSound.h"
#include "npc_bullseye.h"
#include "EntityList.h"

#define	SF_BUGBAIT_SUPPRESS_CALL	0x00000001

//=============================================================================
// Bugbait sensor
//=============================================================================

class CBugBaitSensor : public CPointEntity 
{
public:

	DECLARE_CLASS( CBugBaitSensor, CPointEntity );

	DECLARE_DATADESC();

	void Baited( CBaseEntity *pOther )
	{
		if ( m_bEnabled )
		{
			m_OnBaited.FireOutput( pOther, this );
		}
	}

	void InputEnable( inputdata_t &data )
	{
		m_bEnabled = true;
	}

	void InputDisable( inputdata_t &data )
	{
		m_bEnabled = false;
	}

	void InputToggle( inputdata_t &data )
	{
		m_bEnabled = !m_bEnabled;
	}

	bool SuppressCall( void )
	{
		return ( ( m_spawnflags & SF_BUGBAIT_SUPPRESS_CALL ) == true );
	}

	float GetRadius( void ) const 
	{ 
		if ( m_flRadius == 0 )
			return bugbait_radius.GetFloat();

		return m_flRadius; 
	}

protected:

	float			m_flRadius;
	bool			m_bEnabled;
	COutputEvent	m_OnBaited;
};

BEGIN_DATADESC( CBugBaitSensor )

	DEFINE_KEYFIELD( CBugBaitSensor, m_bEnabled, FIELD_BOOLEAN, "Enabled" ),
	DEFINE_KEYFIELD( CBugBaitSensor, m_flRadius, FIELD_FLOAT, "radius" ),

	DEFINE_INPUTFUNC( CBugBaitSensor, FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( CBugBaitSensor, FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( CBugBaitSensor, FIELD_VOID, "Toggle", InputToggle ),

	// Function Pointers
	DEFINE_OUTPUT( CBugBaitSensor, m_OnBaited, "OnBaited" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( point_bugbait, CBugBaitSensor );

//=============================================================================
// Bugbait grenade
//=============================================================================

#define GRENADE_MODEL "models/spore.mdl"

BEGIN_DATADESC( CGrenadeBugBait )

	DEFINE_FIELD( CGrenadeBugBait, m_pSporeTrail, FIELD_CLASSPTR ),

	// Function Pointers
	DEFINE_FUNCTION( CGrenadeBugBait, BugBaitTouch ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( npc_grenade_bugbait, CGrenadeBugBait );

//Radius of the bugbait's effect on other creatures
ConVar bugbait_radius( "bugbait_radius", "512" );
ConVar bugbait_hear_radius( "bugbait_hear_radius", "2500" );
ConVar bugbait_distract_time( "bugbait_distract_time", "5" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeBugBait::Spawn( void )
{
	Precache();

	SetModel( GRENADE_MODEL );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX ); 

	UTIL_SetSize( this, Vector( -2, -2, -2), Vector( 2, 2, 2 ) );
	Relink();

	SetTouch( BugBaitTouch );

	m_takedamage = DAMAGE_NO;

	m_pSporeTrail = NULL;

	/*
	m_pSporeTrail = SporeTrail::CreateSporeTrail();
	
	m_pSporeTrail->m_bEmit				= true;
	m_pSporeTrail->m_flSpawnRate		= 100.0f;
	m_pSporeTrail->m_flParticleLifetime	= 1.0f;
	m_pSporeTrail->m_flStartSize		= 1.0f;
	m_pSporeTrail->m_flEndSize			= 1.0f;
	m_pSporeTrail->m_flSpawnRadius		= 8.0f;

	m_pSporeTrail->m_vecEndColor		= Vector( 0, 0, 0 );
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeBugBait::Precache( void )
{
	engine->PrecacheModel( GRENADE_MODEL );

	BaseClass::Precache();
}

#define	NUM_SPLASHES	6

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeBugBait::BugBaitTouch( CBaseEntity *pOther )
{
	Assert( pOther );
	if ( !pOther->IsSolid() )
		return;

	if ( m_pSporeTrail != NULL )
	{
		m_pSporeTrail->m_bEmit = false;
	}

	//Do effect for the hit
	SporeExplosion *pSporeExplosion = SporeExplosion::CreateSporeExplosion();

	if ( pSporeExplosion )
	{
		Vector	dir = -GetAbsVelocity();
		VectorNormalize( dir );

		QAngle	angles;
		VectorAngles( dir, angles );

		pSporeExplosion->SetLocalAngles( angles );
		pSporeExplosion->SetLocalOrigin( GetAbsOrigin() );
		pSporeExplosion->m_flSpawnRate			= 1.0f;
		pSporeExplosion->m_flParticleLifetime	= 2.0f;
		pSporeExplosion->SetRenderColor( 0.0f, 0.5f, 0.25f, 0.15f );

		pSporeExplosion->m_flStartSize			= 0.0f;
		pSporeExplosion->m_flEndSize			= 32.0f;
		pSporeExplosion->m_flSpawnRadius		= 2.0f;

		pSporeExplosion->SetLifetime( bugbait_distract_time.GetFloat() );
		
		UTIL_Relink( pSporeExplosion );
	}

	trace_t	tr;
	Vector	traceDir, tracePos;

	Vector	directions[ NUM_SPLASHES ] =
	{
		Vector(  1,  0,  0 ),
		Vector( -1,  0,  0 ),
		Vector(  0,  1,  0 ),
		Vector(  0, -1,  0 ),
		Vector(  0,  0,  1 ),
		Vector(  0,  0, -1 ),
	};

	//Splatter decals everywhere
	for ( int i = 0; i < NUM_SPLASHES; i++ )
	{
		traceDir = directions[i];
		tracePos = GetAbsOrigin() + ( traceDir * 64.0f );

		UTIL_TraceLine( GetAbsOrigin(), tracePos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction < 1.0f )
		{
			UTIL_DecalTrace( &tr, "BeerSplash" );	//TODO: Use real decal
		}
	}

	//Make a splat sound
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), CHAN_WEAPON, "weapons/bugbait/splat.wav", 1.0f, ATTN_NORM );

	//Attempt to activate any spawners in a radius around the bugbait
	CBaseEntity*	pList[100];
	Vector			delta( 256, 256, 256 );
	bool			suppressCall = false;

	int count = UTIL_EntitiesInBox( pList, 100, GetAbsOrigin() - delta, GetAbsOrigin() + delta, 0 );
	
	//Iterate over all the possible targets
	for ( i = 0; i < count; i++ )
	{
		//See if this is a bugbait sensor
		if ( FClassnameIs( pList[i], "point_bugbait" ) )
		{
			CBugBaitSensor *pSensor = dynamic_cast<CBugBaitSensor *>(pList[i]);

			if ( pSensor == NULL )
				continue;

			//Make sure we're within range of the sensor
			if ( pSensor->GetRadius() > ( pSensor->GetAbsOrigin() - GetAbsOrigin() ).Length() )
			{
				//Tell the sensor it's been hit
				pSensor->Baited( GetOwner() );

				//If we're suppressing the call to antlions, then don't make a bugbait sound
				if ( pSensor->SuppressCall() )
				{
					suppressCall = true;
				}
			}
		}
	}

	//Make sure we want to call antlions
	if ( suppressCall == false )
	{
		//Alert any antlions around
		CSoundEnt::InsertSound( SOUND_BUGBAIT, GetAbsOrigin(), bugbait_hear_radius.GetInt(), bugbait_distract_time.GetFloat(), GetOwner() );
	}

	//Go away
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &position - 
//			&angles - 
//			&velocity - 
//			&angVelocity - 
//			*owner - 
// Output : CBaseGrenade
//-----------------------------------------------------------------------------
CBaseGrenade *BugBaitGrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const QAngle &angVelocity, CBaseEntity *owner )
{
	CGrenadeBugBait *pGrenade = (CGrenadeBugBait *) CBaseEntity::Create( "npc_grenade_bugbait", position, angles, owner );
	
	if ( pGrenade != NULL )
	{
		pGrenade->SetLocalAngularVelocity( angVelocity );
		pGrenade->SetAbsVelocity( velocity );
		pGrenade->SetOwner( ToBaseCombatCharacter( owner ) );
	}

	return pGrenade;
}

