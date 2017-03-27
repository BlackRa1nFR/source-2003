//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Crowbar - an old favorite
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hl1_basecombatweapon_shared.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib.h"
#include "in_buttons.h"
#include "soundent.h"
#include "vstdlib/random.h"

extern ConVar sk_plr_dmg_crowbar;

#define	CROWBAR_RANGE		32.0f
#define	CROWBAR_REFIRE_MISS	0.5f
#define	CROWBAR_REFIRE_HIT	0.25f


//-----------------------------------------------------------------------------
// CWeaponCrowbar
//-----------------------------------------------------------------------------

class CWeaponCrowbar : public CBaseHL1CombatWeapon
{
	DECLARE_CLASS( CWeaponCrowbar, CBaseHL1CombatWeapon );
public:

	CWeaponCrowbar();

	void			Precache( void );
	virtual void	ItemPostFrame( void );
	void			PrimaryAttack( void );

public:
	trace_t		m_traceHit;
	Activity	m_nHitActivity;

private:
	virtual void		Swing( void );
	virtual	void		Hit( void );
	virtual	void		ImpactEffect( void );
	virtual	void		ImpactSound( bool isWorld );
	virtual Activity	ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBasePlayer *pOwner );

public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( weapon_crowbar, CWeaponCrowbar );
PRECACHE_WEAPON_REGISTER( weapon_crowbar );

IMPLEMENT_SERVERCLASS_ST( CWeaponCrowbar, DT_WeaponCrowbar )
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponCrowbar )

	// UNDONE: Save/restore these?
//	trace_t			m_trLineHit;		// Used for decals - geometry hit
//	trace_t			m_trHullHit;		// Used for hitting NPCs
//	Activity		m_nHitActivity;

	// Function Pointers
	DEFINE_FUNCTION( CWeaponCrowbar, Hit ),

END_DATADESC()

static const Vector g_bludgeonMins(-16,-16,-16);
static const Vector g_bludgeonMaxs(16,16,16);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponCrowbar::CWeaponCrowbar()
{
	m_bFiresUnderwater = true;
}

//-----------------------------------------------------------------------------
// Purpose: Precache the weapon
//-----------------------------------------------------------------------------
void CWeaponCrowbar::Precache( void )
{
	//Call base class first
	BaseClass::Precache();
}

//------------------------------------------------------------------------------
// Purpose : Update weapon
//------------------------------------------------------------------------------
void CWeaponCrowbar::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		PrimaryAttack();
	} 
	else 
	{
		WeaponIdle();
		return;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeaponCrowbar::PrimaryAttack()
{
	Swing();
}


//------------------------------------------------------------------------------
// Purpose: Implement impact function
//------------------------------------------------------------------------------
void CWeaponCrowbar::Hit( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	//Make sound for the AI
	CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, m_traceHit.endpos, 400, 0.2f, pPlayer );

	CBaseEntity	*pHitEntity = m_traceHit.m_pEnt;

	//Apply damage to a hit target
	if ( pHitEntity != NULL )
	{
		Vector hitDirection;
		pPlayer->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );

		ClearMultiDamage();
		CTakeDamageInfo info( GetOwner(), GetOwner(), sk_plr_dmg_crowbar.GetFloat(), DMG_CLUB );
		CalculateMeleeDamageForce( &info, hitDirection, m_traceHit.endpos );
		pHitEntity->DispatchTraceAttack( info, hitDirection, &m_traceHit ); 
		ApplyMultiDamage();

		// Now hit all triggers along the ray that... 
		TraceAttackToTriggers( CTakeDamageInfo( GetOwner(), GetOwner(), sk_plr_dmg_crowbar.GetFloat(), DMG_CLUB ), m_traceHit.startpos, m_traceHit.endpos, hitDirection );

		//Play an impact sound	
		ImpactSound( pHitEntity->Classify() == CLASS_NONE || pHitEntity->Classify() == CLASS_MACHINE );
	}

	//Apply an impact effect
	ImpactEffect();
}

//-----------------------------------------------------------------------------
// Purpose: Play the impact sound
// Input  : isWorld - whether the hit was for the world or an entity
//-----------------------------------------------------------------------------
void CWeaponCrowbar::ImpactSound( bool isWorld )
{
	WeaponSound_t soundType = ( isWorld ) ? MELEE_HIT_WORLD : MELEE_HIT;

	WeaponSound( soundType );
}

Activity CWeaponCrowbar::ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBasePlayer *pOwner )
{
	int			i, j, k;
	float		distance;
	const float	*minmaxs[2] = {mins.Base(), maxs.Base()};
	trace_t		tmpTrace;
	Vector		vecHullEnd = hitTrace.endpos;
	Vector		vecEnd;

	distance = 1e6f;
	Vector vecSrc = hitTrace.startpos;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace );
	if ( tmpTrace.fraction == 1.0 )
	{
		for ( i = 0; i < 2; i++ )
		{
			for ( j = 0; j < 2; j++ )
			{
				for ( k = 0; k < 2; k++ )
				{
					vecEnd.x = vecHullEnd.x + minmaxs[i][0];
					vecEnd.y = vecHullEnd.y + minmaxs[j][1];
					vecEnd.z = vecHullEnd.z + minmaxs[k][2];

					UTIL_TraceLine( vecSrc, vecEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace );
					if ( tmpTrace.fraction < 1.0 )
					{
						float thisDistance = (tmpTrace.endpos - vecSrc).Length();
						if ( thisDistance < distance )
						{
							hitTrace = tmpTrace;
							distance = thisDistance;
						}
					}
				}
			}
		}
	}
	else
	{
		hitTrace = tmpTrace;
	}


	return ACT_VM_HITCENTER;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrowbar::ImpactEffect( void )
{
	//FIXME: need new decals
	UTIL_ImpactTrace( &m_traceHit, DMG_CLUB );
}

//------------------------------------------------------------------------------
// Purpose : Starts the swing of the weapon and determines the animation
//------------------------------------------------------------------------------
void CWeaponCrowbar::Swing( void )
{
	// Try a ray
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	Vector swingStart = pOwner->Weapon_ShootPosition( );
	Vector forward;

	pOwner->EyeVectors( &forward, NULL, NULL );

	UTIL_TraceLine( swingStart, swingStart + forward * CROWBAR_RANGE, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &m_traceHit );
	m_nHitActivity = ACT_VM_HITCENTER;

	if ( m_traceHit.fraction == 1.0 )
	{
		UTIL_TraceHull( swingStart, swingStart + forward * CROWBAR_RANGE, g_bludgeonMins, g_bludgeonMaxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &m_traceHit );
		if ( m_traceHit.fraction < 1.0 )
		{
			m_nHitActivity = ChooseIntersectionPointAndActivity( m_traceHit, g_bludgeonMins, g_bludgeonMaxs, pOwner );
		}
	}


	// -------------------------
	//	Miss
	// -------------------------
	if ( m_traceHit.fraction == 1.0f )
	{
		m_nHitActivity = ACT_VM_MISSCENTER;

		//Play swing sound
		WeaponSound( SINGLE );

		//Setup our next attack times
		m_flNextPrimaryAttack = gpGlobals->curtime + CROWBAR_REFIRE_MISS;
	}
	else
	{
		Hit();

		//Setup our next attack times
		m_flNextPrimaryAttack = gpGlobals->curtime + CROWBAR_REFIRE_HIT;
	}

	//Send the anim
	SendWeaponAnim( m_nHitActivity );
}
