//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "decals.h"
#include "basegrenade_shared.h"
#include "shake.h"
#include "engine/IEngineSound.h"

#if !defined( CLIENT_DLL )

#include "soundent.h"
#include "entitylist.h"

#endif

extern short	g_sModelIndexFireball;		// (in combatweapon.cpp) holds the index for the fireball 
extern short	g_sModelIndexWExplosion;	// (in combatweapon.cpp) holds the index for the underwater explosion
extern short	g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud
extern ConVar    sk_plr_dmg_grenade;

#if !defined( CLIENT_DLL )

// Global Savedata for friction modifier
BEGIN_DATADESC( CBaseGrenade )
	//					nextGrenade
	DEFINE_FIELD( CBaseGrenade, m_hOwner, FIELD_EHANDLE ),
	//					m_fRegisteredSound ???
	DEFINE_FIELD( CBaseGrenade, m_bIsLive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseGrenade, m_DmgRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseGrenade, m_flDetonateTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseGrenade, m_flDamage, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseGrenade, m_iszBounceSound, FIELD_STRING ),

	// Function Pointers
	DEFINE_FUNCTION( CBaseGrenade, Smoke ),
	DEFINE_FUNCTION( CBaseGrenade, BounceTouch ),
	DEFINE_FUNCTION( CBaseGrenade, SlideTouch ),
	DEFINE_FUNCTION( CBaseGrenade, ExplodeTouch ),
	DEFINE_FUNCTION( CBaseGrenade, DangerSoundThink ),
	DEFINE_FUNCTION( CBaseGrenade, PreDetonate ),
	DEFINE_FUNCTION( CBaseGrenade, Detonate ),
	DEFINE_FUNCTION( CBaseGrenade, DetonateUse ),
	DEFINE_FUNCTION( CBaseGrenade, TumbleThink ),

END_DATADESC()

void SendProxy_CropFlagsToPlayerFlagBitsLength( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID);

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( BaseGrenade, DT_BaseGrenade )

BEGIN_NETWORK_TABLE( CBaseGrenade, DT_BaseGrenade )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO( m_flDamage ), 10, SPROP_ROUNDDOWN, 0.0, 256.0f ),
	SendPropFloat( SENDINFO( m_DmgRadius ), 10, SPROP_ROUNDDOWN, 0.0, 1024.0f ),
	SendPropInt( SENDINFO( m_bIsLive ), 1, SPROP_UNSIGNED ),
//	SendPropTime( SENDINFO( m_flDetonateTime ) ),
	SendPropEHandle( SENDINFO( m_hOwner ) ),

	SendPropVector( SENDINFO( m_vecVelocity ), 0, SPROP_NOSCALE ), 
	// HACK: Use same flag bits as player for now
	SendPropInt			( SENDINFO(m_fFlags), PLAYER_FLAG_BITS, SPROP_UNSIGNED, SendProxy_CropFlagsToPlayerFlagBitsLength ),

	SendPropTime( SENDINFO( m_flNextAttack ) ),

#else
	RecvPropFloat( RECVINFO( m_flDamage ) ),
	RecvPropFloat( RECVINFO( m_DmgRadius ) ),
	RecvPropInt( RECVINFO( m_bIsLive ) ),
//	RecvPropTime( RECVINFO( m_flDetonateTime ) ),
	RecvPropEHandle( RECVINFO( m_hOwner ) ),

	// Need velocity from grenades to make animation system work correctly when running
	RecvPropVector( RECVINFO(m_vecVelocity) ),

	RecvPropInt( RECVINFO( m_fFlags ) ),

	RecvPropTime( RECVINFO( m_flNextAttack ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( grenade, CBaseGrenade );

BEGIN_PREDICTION_DATA( CBaseGrenade  )

	DEFINE_PRED_FIELD( CBaseGrenade, m_hOwner, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CBaseGrenade, m_bIsLive, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( CBaseGrenade, m_DmgRadius, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD_TOL( CBaseGrenade, m_flDetonateTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD( CBaseGrenade, m_flDamage, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD_TOL( CBaseGrenade, m_vecVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.5f ),
	DEFINE_PRED_FIELD_TOL( CBaseGrenade, m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),

//	DEFINE_FIELD( CBaseGrenade, m_fRegisteredSound, FIELD_BOOLEAN ),
//	DEFINE_FIELD( CBaseGrenade, m_iszBounceSound, FIELD_STRING ),

END_PREDICTION_DATA()

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE		0x0001

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CBaseGrenade::Explode( trace_t *pTrace, int bitsDamageType )
{
#if !defined( CLIENT_DLL )
	float		flRndSound;// sound randomizer

	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetLocalOrigin( pTrace->endpos + (pTrace->plane.normal * 0.6) );
	}

	UTIL_Relink( this );

	Vector vecAbsOrigin = GetAbsOrigin();
	int contents = UTIL_PointContents ( vecAbsOrigin );

	if ( pTrace->fraction != 1.0 )
	{
		Vector vecNormal = pTrace->plane.normal;
		surfacedata_t *pdata = physprops->GetSurfaceData( pTrace->surface.surfaceProps );	
		CPASFilter filter( vecAbsOrigin );
		te->Explosion( filter, 0.0, 
			&vecAbsOrigin,
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius * .03, 
			25,
			TE_EXPLFLAG_NONE,
			m_DmgRadius,
			m_flDamage,
			&vecNormal,
			(char) pdata->gameMaterial );
	}
	else
	{
		CPASFilter filter( vecAbsOrigin );
		te->Explosion( filter, 0.0,
			&vecAbsOrigin, 
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius * .03, 
			25,
			TE_EXPLFLAG_NONE,
			m_DmgRadius,
			m_flDamage );
	}

#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );
#endif

	// Use the owner's position as the reported position
	Vector vecReported = m_hOwner ? m_hOwner->GetAbsOrigin() : vec3_origin;
	
	CTakeDamageInfo info( this, m_hOwner, GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, 0, &vecReported );

	RadiusDamage( info, GetAbsOrigin(), m_DmgRadius, CLASS_NONE );

	UTIL_DecalTrace( pTrace, "Scorch" );

	flRndSound = random->RandomFloat( 0 , 1 );

	EmitSound( "BaseGrenade.Explode" );

	SetThink( &CBaseGrenade::SUB_Remove );
	SetTouch( NULL );
	
	m_fEffects		|= EF_NODRAW;
	SetAbsVelocity( vec3_origin );
	SetNextThink( gpGlobals->curtime );
#endif
}


void CBaseGrenade::Smoke( void )
{
	Vector vecAbsOrigin = GetAbsOrigin();
	if ( UTIL_PointContents ( vecAbsOrigin ) & MASK_WATER )
	{
		UTIL_Bubbles( vecAbsOrigin - Vector( 64, 64, 64 ), vecAbsOrigin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		CPVSFilter filter( vecAbsOrigin );

		te->Smoke( filter, 0.0, 
			&vecAbsOrigin, g_sModelIndexSmoke,
			m_DmgRadius * 0.03,
			24 );
	}
#if !defined( CLIENT_DLL )
	SetThink ( &CBaseGrenade::SUB_Remove );
#endif
	SetNextThink( gpGlobals->curtime );
}

void CBaseGrenade::Event_Killed( const CTakeDamageInfo &info )
{
	Detonate( );
}


// Timed grenade, this think is called when time runs out.
void CBaseGrenade::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CBaseGrenade::Detonate );
	SetNextThink( gpGlobals->curtime );
}

void CBaseGrenade::PreDetonate( void )
{
#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), 400, 0.5 );
#endif

	SetThink( &CBaseGrenade::Detonate );
	SetNextThink( gpGlobals->curtime + 1.5 );
}


void CBaseGrenade::Detonate( void )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, & tr);

	Explode( &tr, DMG_BLAST );

	if ( GetShakeAmplitude() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}
}


//
// Contact grenade, explode when it touches something
// 
void CBaseGrenade::ExplodeTouch( CBaseEntity *pOther )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	Assert( pOther );
	if ( !pOther->IsSolid() )
		return;

	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	Explode( &tr, DMG_BLAST );
}


void CBaseGrenade::DangerSoundThink( void )
{
	if (!IsInWorld())
	{
		Remove( );
		return;
	}

#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin() + GetAbsVelocity() * 0.5, GetAbsVelocity().Length( ), 0.2 );
#endif

	SetNextThink( gpGlobals->curtime + 0.2 );

	if (GetWaterLevel() != 0)
	{
		SetAbsVelocity( GetAbsVelocity() * 0.5 );
	}
}


void CBaseGrenade::BounceTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) )
		return;

	// don't hit the guy that launched this grenade
	if ( pOther == GetOwner() )
		return;

	// only do damage if we're moving fairly fast
	if ( (pOther->m_takedamage != DAMAGE_NO) && (m_flNextAttack < gpGlobals->curtime && GetAbsVelocity().Length() > 100))
	{
		if (m_hOwner)
		{
#if !defined( CLIENT_DLL )
			trace_t tr = CBaseEntity::GetTouchTrace( );
			ClearMultiDamage( );
			Vector forward;
			AngleVectors( GetLocalAngles(), &forward, NULL, NULL );
			CTakeDamageInfo info( this, m_hOwner, 1, DMG_CLUB );
			CalculateMeleeDamageForce( &info, GetAbsVelocity(), GetAbsOrigin() );
			pOther->DispatchTraceAttack( info, forward, &tr ); 
			ApplyMultiDamage();
#endif
		}
		m_flNextAttack = gpGlobals->curtime + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// m_vecAngVelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = GetAbsVelocity(); 
	vecTestVelocity.z *= 0.45;

	if ( !m_fRegisteredSound && vecTestVelocity.Length() <= 60 )
	{
		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// emit the danger sound.
		
		// register a radius louder than the explosion, so we make sure everyone gets out of the way
#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), m_flDamage / 0.4, 0.3 );
#endif
		m_fRegisteredSound = TRUE;
	}

	if (GetFlags() & FL_ONGROUND)
	{
		// add a bit of static friction
//		SetAbsVelocity( GetAbsVelocity() * 0.8 );

		// m_nSequence = random->RandomInt( 1, 1 ); // FIXME: missing tumble animations
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	m_flPlaybackRate = GetAbsVelocity().Length() / 200.0;
	if (m_flPlaybackRate > 1.0)
		m_flPlaybackRate = 1;
	else if (m_flPlaybackRate < 0.5)
		m_flPlaybackRate = 0;

}



void CBaseGrenade::SlideTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther == GetOwner() )
		return;

	// m_vecAngVelocity = Vector (300, 300, 300);

	if (GetFlags() & FL_ONGROUND)
	{
		// add a bit of static friction
//		SetAbsVelocity( GetAbsVelocity() * 0.95 );  

		if (GetAbsVelocity().x != 0 || GetAbsVelocity().y != 0)
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CBaseGrenade ::BounceSound( void )
{
	const char *pszBounceSound = ( m_iszBounceSound == NULL_STRING ) ? "BaseGrenade.BounceSound" : STRING( m_iszBounceSound );
	EmitSound( pszBounceSound );
}

void CBaseGrenade ::TumbleThink( void )
{
	if (!IsInWorld())
	{
		Remove( );
		return;
	}

	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	//
	// Emit a danger sound one second before exploding.
	//
	if (m_flDetonateTime - 1 < gpGlobals->curtime)
	{
#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin() + GetAbsVelocity() * (m_flDetonateTime - gpGlobals->curtime), 400, 0.1 );
#endif
	}

	if (m_flDetonateTime <= gpGlobals->curtime)
	{
		SetThink( &CBaseGrenade::Detonate );
	}

	if (GetWaterLevel() != 0)
	{
		SetAbsVelocity( GetAbsVelocity() * 0.5 );
		m_flPlaybackRate = 0.2;
	}
}

void CBaseGrenade::Precache( void )
{
	BaseClass::Precache( );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatCharacter
//-----------------------------------------------------------------------------
CBaseCombatCharacter *CBaseGrenade::GetOwner( void )
{
	CBaseCombatCharacter *pResult = ToBaseCombatCharacter( m_hOwner );
	if ( !pResult )
		Msg( "Warning: grenade has no owner\n" );
	if ( !pResult && GetOwnerEntity() != NULL )
	{
		pResult = ToBaseCombatCharacter( GetOwnerEntity() );
	}
	return pResult;
}

//-----------------------------------------------------------------------------

void CBaseGrenade::SetOwner( CBaseCombatCharacter *pOwner )
{
	m_hOwner = pOwner;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseGrenade::~CBaseGrenade(void)
{
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseGrenade::CBaseGrenade(void)
{
	m_hOwner			= NULL;
	m_bIsLive			= false;
	m_DmgRadius			= 100;
	m_flDetonateTime	= 0;

	SetSimulatedEveryTick( true );
};














