//=========================================================
// BaseBludgeonWeapon.cpp - the class from which all bludgeon-type
// player weapons are derived.
//=========================================================

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib.h"
#include "in_buttons.h"
#include "soundent.h"
#include "animation.h"
#include "ai_condition.h"
#include "basebludgeonweapon.h"

IMPLEMENT_SERVERCLASS_ST( CBaseHLBludgeonWeapon, DT_BaseHLBludgeonWeapon )
END_SEND_TABLE()

static const Vector g_bludgeonMins(-16,-16,-16);
static const Vector g_bludgeonMaxs(16,16,16);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CBaseHLBludgeonWeapon::CBaseHLBludgeonWeapon()
{
	m_bFiresUnderwater = true;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the weapon
//-----------------------------------------------------------------------------
void CBaseHLBludgeonWeapon::Spawn( void )
{
	//Call base class first
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache the weapon
//-----------------------------------------------------------------------------
void CBaseHLBludgeonWeapon::Precache( void )
{
	//Call base class first
	BaseClass::Precache();
}

int CBaseHLBludgeonWeapon::CapabilitiesGet()
{ 
	return bits_CAP_WEAPON_MELEE_ATTACK1; 
}


int CBaseHLBludgeonWeapon::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	if (flDist > 64)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}

//------------------------------------------------------------------------------
// Purpose : Update weapon
//------------------------------------------------------------------------------
void CBaseHLBludgeonWeapon::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		PrimaryAttack();
	} 
	else if ( (pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime) )
	{
		SecondaryAttack();
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
void CBaseHLBludgeonWeapon::PrimaryAttack()
{
	Swing( false );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHLBludgeonWeapon::SecondaryAttack()
{
	Swing( true );
}


//------------------------------------------------------------------------------
// Purpose: Implement impact function
//------------------------------------------------------------------------------
void CBaseHLBludgeonWeapon::Hit( trace_t &traceHit, Activity nHitActivity )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	//Do view kick
	AddViewKick();

	//Make sound for the AI
	CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, traceHit.endpos, 400, 0.2f, pPlayer );

	CBaseEntity	*pHitEntity = traceHit.m_pEnt;

	//Apply damage to a hit target
	if ( pHitEntity != NULL )
	{
		Vector hitDirection;
		pPlayer->EyeVectors( &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );

		CTakeDamageInfo info( GetOwner(), GetOwner(), GetDamageForActivity( nHitActivity ), DMG_CLUB );
		CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos );

		pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit ); 
		ApplyMultiDamage();

		// Now hit all triggers along the ray that... 
		TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );
		// Play an impact sound	
		ImpactSound( FClassnameIs( pHitEntity , "worldspawn" ) );
	}

	// Apply an impact effect
	ImpactEffect( traceHit );
}

//-----------------------------------------------------------------------------
// Purpose: Play the impact sound
// Input  : isWorld - whether the hit was for the world or an entity
//-----------------------------------------------------------------------------
void CBaseHLBludgeonWeapon::ImpactSound( bool isWorld )
{
	WeaponSound_t soundType = ( isWorld ) ? MELEE_HIT_WORLD : MELEE_HIT;

	WeaponSound( soundType );
}

Activity CBaseHLBludgeonWeapon::ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBasePlayer *pOwner )
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
void CBaseHLBludgeonWeapon::ImpactEffect( trace_t &traceHit )
{
	//FIXME: need new decals
	UTIL_ImpactTrace( &traceHit, DMG_CLUB );
}


//------------------------------------------------------------------------------
// Purpose : Starts the swing of the weapon and determines the animation
// Input   : bIsSecondary - is this a secondary attack?
//------------------------------------------------------------------------------
void CBaseHLBludgeonWeapon::Swing( int bIsSecondary )
{
	trace_t traceHit;

	// Try a ray
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	Vector swingStart = pOwner->Weapon_ShootPosition( );
	Vector forward;

	pOwner->EyeVectors( &forward, NULL, NULL );

	UTIL_TraceLine( swingStart, swingStart + forward * GetRange(), MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
	Activity nHitActivity = ACT_VM_HITCENTER;

	if ( traceHit.fraction == 1.0 )
	{
		UTIL_TraceHull( swingStart, swingStart + forward * GetRange(), g_bludgeonMins, g_bludgeonMaxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
		if ( traceHit.fraction < 1.0 )
		{
			nHitActivity = ChooseIntersectionPointAndActivity( traceHit, g_bludgeonMins, g_bludgeonMaxs, pOwner );
		}
	}


	// -------------------------
	//	Miss
	// -------------------------
	if ( traceHit.fraction == 1.0f )
	{
		nHitActivity = bIsSecondary ? ACT_VM_MISSCENTER2 : ACT_VM_MISSCENTER;
	}
	else
	{
		Hit( traceHit, nHitActivity );
	}

	// Send the anim
	SendWeaponAnim( nHitActivity );

	//Setup our next attack times
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();

	//Play swing sound
	WeaponSound( SINGLE );
}
