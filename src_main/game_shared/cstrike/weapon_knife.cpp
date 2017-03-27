//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "util.h"
#include "weapon_knife.h"

#if defined( CLIENT_DLL )
	#include "c_cs_player.h"
#else
	#include "cs_player.h"
#endif


#define	KNIFE_BODYHIT_VOLUME 128
#define	KNIFE_WALLHIT_VOLUME 512


Vector head_hull_mins( -16, -16, -18 );
Vector head_hull_maxs( 16, 16, 18 );


// ----------------------------------------------------------------------------- //
// CKnife tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED( Knife, DT_WeaponKnife )

BEGIN_NETWORK_TABLE( CKnife, DT_WeaponKnife )
	#ifdef CLIENT_DLL
		RecvPropInt( RECVINFO( m_iSwing ) )
	#else
		SendPropInt( SENDINFO( m_iSwing ), 1, SPROP_UNSIGNED )
	#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CKnife )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_knife, CKnife );
PRECACHE_WEAPON_REGISTER( weapon_knife );

#ifndef CLIENT_DLL

	BEGIN_DATADESC( CKnife )
		DEFINE_FUNCTION( CKnife, Smack )
	END_DATADESC()

#endif


int FilterKnifeActivity( int iActivity )
{
	if ( iActivity == ACT_SPECIAL_ATTACK1 || iActivity == ACT_SPECIAL_ATTACK2 || iActivity == ACT_VM_SWINGHIT )
		return ACT_VM_PRIMARYATTACK;
	else
		return iActivity;
}


// ----------------------------------------------------------------------------- //
// CKnife implementation.
// ----------------------------------------------------------------------------- //

CKnife::CKnife()
{
	m_iSwing = 0;
}


bool CKnife::HasPrimaryAmmo()
{
	return true;
}


bool CKnife::CanBeSelected()
{
	return true;
}


void CKnife::Spawn()
{
	m_iClip1 = -1;
	BaseClass::Spawn();
}


bool CKnife::Deploy()
{
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();
	EmitSound( filter, entindex(), "Weapon_Knife.Deploy" );

	m_iSwing = 0;
	
	return BaseClass::Deploy();
}

void CKnife::Holster( int skiplocal )
{
	GetPlayerOwner()->m_flNextAttack = gpGlobals->curtime + 0.5;
}

void CKnife::WeaponAnimation ( int iAnimation )
{
	/*
	int flag;
	#if defined( CLIENT_WEAPONS )
		flag = FEV_NOTHOST;
	#else
		flag = 0;
	#endif

	PLAYBACK_EVENT_FULL( flag, pPlayer->edict(), m_usKnife,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 
		0.0,
		0.0,
		iAnimation, 2, 3, 4 );
	*/
}

void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, CBaseEntity *pEntity )
{
	int			i, j, k;
	float		distance;
	Vector minmaxs[2] = {mins, maxs};
	trace_t tmpTrace;
	Vector		vecHullEnd = tr.endpos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace );
	if ( tmpTrace.fraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace );
				if ( tmpTrace.fraction < 1.0 )
				{
					float thisDistance = (tmpTrace.endpos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}


void CKnife::PrimaryAttack()
{
	Swing( 1 );
}

void CKnife::SecondaryAttack()
{
	Stab (1);
	SetNextThink( gpGlobals->curtime + 0.35 );
}


void CKnife::Smack()
{
	UTIL_ImpactTrace( &m_trHit, DMG_BULLET );
	//DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR, false, pPlayer->pev, false );
}


void CKnife::SwingAgain()
{
	Swing( 0 );
}


void CKnife::WeaponIdle()
{
	//ResetEmptySound();

	CCSPlayer *pPlayer = GetPlayerOwner();

	pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	if ( pPlayer->IsShieldDrawn() )
		 return;

	m_flTimeWeaponIdle = gpGlobals->curtime + 20;

	// only idle if the slid isn't back
	SendWeaponAnim( ACT_VM_IDLE );
}


int CKnife::Swing( int fFirst )
{
	CCSPlayer *pPlayer = GetPlayerOwner();

	int fDidHit = false;

	Vector vForward;
	AngleVectors( pPlayer->EyeAngles(), &vForward );
	Vector vecSrc	= pPlayer->Weapon_ShootPosition();
	Vector vecEnd	= vecSrc + vForward * 48;

	trace_t tr;
	UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, head_hull_mins, head_hull_maxs, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = tr.m_pEnt;
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, pPlayer );
			vecEnd = tr.endpos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	if ( tr.fraction >= 1.0 )
	{
		if (fFirst)
		{
			if ( pPlayer->HasShield() == false )
			{
				m_iSwing = (m_iSwing + 1) % 2;
				switch( m_iSwing )
				{
				case 0:
					// KNIFE_MIDATTACK1HIT
					SendWeaponAnim( FilterKnifeActivity( ACT_VM_PRIMARYATTACK ) );
					break;
				case 1:
					// KNIFE_MIDATTACK2HIT
					SendWeaponAnim( FilterKnifeActivity( ACT_SPECIAL_ATTACK1 ) ); 
					break;
				}

				m_flNextPrimaryAttack = gpGlobals->curtime + 0.35;
				m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
			}
			else
			{
				// KNIFE_SHIELD_ATTACKHIT
				SendWeaponAnim( FilterKnifeActivity( ACT_SPECIAL_ATTACK2 ) );

				m_flNextPrimaryAttack = gpGlobals->curtime + 1;
				m_flNextSecondaryAttack = gpGlobals->curtime + 1.2;
			}

			m_flTimeWeaponIdle = gpGlobals->curtime + 2;
		
			
			// play wiff or swish sound
			CPASAttenuationFilter filter( this );
			filter.UsePredictionRules();
			EmitSound( filter, entindex(), "Weapon_Knife.Slash" );

			// player "shoot" animation
			//pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		// hit
		fDidHit = true;

		if ( pPlayer->HasShield() == false )
		{
			m_iSwing = (m_iSwing + 1) % 2;
			switch ( m_iSwing )
			{
			case 0:
				// KNIFE_MIDATTACK1HIT
				SendWeaponAnim( FilterKnifeActivity( ACT_VM_PRIMARYATTACK ) );
				//SendWeaponAnim( KNIFE_MIDATTACK1HIT, UseDecrement() ? 1: 0 ); 
				break;

			case 1:
				// KNIFE_MIDATTACK2HIT
				SendWeaponAnim( FilterKnifeActivity( ACT_SPECIAL_ATTACK2 ) ); 
				//SendWeaponAnim( KNIFE_MIDATTACK2HIT, UseDecrement() ? 1: 0 ); 
				break;
			}

			m_flNextPrimaryAttack = gpGlobals->curtime + 0.4;
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
		}
		else
		{
			// KNIFE_SHIELD_ATTACKHIT
			SendWeaponAnim( FilterKnifeActivity( ACT_SPECIAL_ATTACK2 ) );
			//SendWeaponAnim( KNIFE_SHIELD_ATTACKHIT, UseDecrement() ? 1 : 0 );

			m_flNextPrimaryAttack = gpGlobals->curtime + 1;
			m_flNextSecondaryAttack = gpGlobals->curtime + 1.2;
		}

		m_flTimeWeaponIdle = gpGlobals->curtime + 2;

	
#ifndef CLIENT_DLL

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = true;


		CBaseEntity *pEntity = tr.m_pEnt;
		
		//SetPlayerShieldAnim();
		
		// player "shoot" animation
		pPlayer->SetAnimation( PLAYER_ATTACK1 );

		ClearMultiDamage();
		CTakeDamageInfo info( pPlayer, pPlayer, 20, DMG_BULLET | DMG_NEVERGIB );
		if ( (m_flNextPrimaryAttack + 0.4 < gpGlobals->curtime) )
		{
			// first swing does full damage
			info.SetDamage( 20 );
		}
		else
		{
			// subsequent swings do less
			info.SetDamage( 15 );
		}	
		CalculateMeleeDamageForce( &info, vForward, tr.endpos );
		pEntity->DispatchTraceAttack( info, vForward, &tr ); 
		ApplyMultiDamage();

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_PLAYER )
			{
				CPASAttenuationFilter filter( this );
				filter.UsePredictionRules();
				EmitSound( filter, entindex(), "Weapon_Knife.Hit" );
				
				//pPlayer->m_iWeaponVolume = KNIFE_BODYHIT_VOLUME;
				if (!pEntity->IsAlive() )
					return true;
				else
					flVol = 0.1;

				fHitWorld = false;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if ( fHitWorld )
		{
			//float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR);
			//fvolbar = 1;

			CPASAttenuationFilter filter( this );
			filter.UsePredictionRules();
			EmitSound( filter, entindex(), "Weapon_Knife.HitWall" );
		}

#endif

		// delay the decal a bit
		m_trHit = tr;
		SetThink( &CKnife::Smack );
		SetNextThink( gpGlobals->curtime + 0.2 );

		//pPlayer->m_iWeaponVolume = flVol * KNIFE_WALLHIT_VOLUME;
		//ResetPlayerShieldAnim();
	}
	
	return fDidHit;
}



int CKnife::Stab( int fFirst )
{
	CCSPlayer *pPlayer = GetPlayerOwner();


	int fDidHit = false;

	trace_t tr;

	Vector vForward;
	AngleVectors( pPlayer->EyeAngles(), &vForward );
	Vector vecSrc	= pPlayer->Weapon_ShootPosition();
	Vector vecEnd	= vecSrc + vForward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, head_hull_mins, head_hull_maxs, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = tr.m_pEnt;
			if ( !pHit || pHit->IsBSPModel() )
			{
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, pPlayer );
			}

			vecEnd = tr.endpos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	if ( tr.fraction >= 1.0 )
	{
		if (fFirst)
		{
			//SendWeaponAnim( KNIFE_STABMISS, UseDecrement() ? 1: 0 );
			SendWeaponAnim( FilterKnifeActivity( ACT_VM_SWINGMISS ) );
			
			m_flNextSecondaryAttack = gpGlobals->curtime + 1;
			m_flNextPrimaryAttack = gpGlobals->curtime + 1;
			
			// play wiff or swish sound
			CPASAttenuationFilter filter( this );
			filter.UsePredictionRules();
			EmitSound( filter, entindex(), "Weapon_Knife.Slash" );

			// player "shoot" animation
			//pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		// hit
		fDidHit = true;
		
		//SendWeaponAnim( KNIFE_STABHIT, UseDecrement() ? 1: 0 );
		SendWeaponAnim( FilterKnifeActivity( ACT_VM_SWINGHIT ) );

		m_flNextSecondaryAttack = gpGlobals->curtime + 1.1;
		m_flNextPrimaryAttack = gpGlobals->curtime + 1.1;
		
		
#ifndef CLIENT_DLL

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = true;

		CBaseEntity *pEntity = tr.m_pEnt;
		
		// player "shoot" animation
		pPlayer->SetAnimation( PLAYER_ATTACK1 );

		float flDamage = 65.0f;

		if ( pEntity && pEntity->IsPlayer() )
		{
			Vector2D	vec2LOS;
			float		flDot;
			Vector      vMyForward = vForward;

			AngleVectors( pEntity->GetAbsAngles(), &vForward );
			
			vec2LOS = vMyForward.AsVector2D();
			Vector2DNormalize( vec2LOS );

			flDot = vec2LOS.Dot( vForward.AsVector2D() );

			//Triple the damage if we are stabbing them in the back.
			if ( flDot > 0.80f )
				 flDamage *= 3;
		}

		AngleVectors( pPlayer->EyeAngles(), &vForward );

		ClearMultiDamage();
		CTakeDamageInfo info( pPlayer, pPlayer, flDamage, DMG_BULLET | DMG_NEVERGIB );
		CalculateMeleeDamageForce( &info, vForward, tr.endpos );
		pEntity->DispatchTraceAttack( info, vForward, &tr ); 
		ApplyMultiDamage();

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_PLAYER )
			{
				CPASAttenuationFilter filter( this );
				filter.UsePredictionRules();
				EmitSound( filter, entindex(), "Weapon_Knife.Stab" );

				//pPlayer->m_iWeaponVolume = KNIFE_BODYHIT_VOLUME;
				if (!pEntity->IsAlive() )
					return true;
				else
					flVol = 0.1;

				fHitWorld = false;

			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			//float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR);
			//fvolbar = 1;

			// also play knife strike
			CPASAttenuationFilter filter( this );
			filter.UsePredictionRules();
			EmitSound( filter, entindex(), "Weapon_Knife.HitWall" );
		}

#endif
		// delay the decal a bit
		m_trHit = tr;
		SetThink( &CKnife::Smack );
		SetNextThink( gpGlobals->curtime + 0.2 );

		//pPlayer->m_iWeaponVolume = flVol * KNIFE_WALLHIT_VOLUME;
	}
	return fDidHit;
}


bool CKnife::CanDrop()
{
	return false;
}


