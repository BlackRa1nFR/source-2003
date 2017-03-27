//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_csbase.h"
#include "decals.h"
#include "cs_gamerules.h"
#include "weapon_c4.h"

#ifdef CLIENT_DLL
	#include "c_cs_player.h"
#else
	#include "cs_player.h"
#endif

#include "cs_playeranimstate.h"
#include "basecombatweapon_shared.h"
#include "util_shared.h"
#include "takedamageinfo.h"


void WeaponImpact( trace_t *tr, Vector vecDir, bool bHurt, CBaseEntity *pEntity, int iDamageType )
{
	Assert( pEntity );
	
	// Client waits for server version 
//	#ifndef CLIENT_DLL
		// Make sure the server sends to us, even though we're predicting
//		CDisablePredictionFiltering dpf;
		UTIL_ImpactTrace( tr, iDamageType, "Impact" );
//	#endif
}


void CCSPlayer::GetBulletTypeParameters( 
	int iBulletType, 
	int &iPenetrationPower, 
	float &flPenetrationDistance,
	int &iSparksAmount, 
	int &iCurrentDamage )
{
	//MIKETODO: make ammo types come from a script file.
	if ( IsAmmoType( iBulletType, BULLET_PLAYER_50AE ) )
	{
		iPenetrationPower = 30;
		flPenetrationDistance = 1000.0;
		iSparksAmount = 20;
		iCurrentDamage += (-4 + SHARED_RANDOMINT(0,10));
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_762MM ) )
	{
		iPenetrationPower = 39;
		flPenetrationDistance = 5000.0;
		iSparksAmount = 30;
		iCurrentDamage += (-2 + SHARED_RANDOMINT(0,4));
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_556MM ) )
	{
		iPenetrationPower = 35;
		flPenetrationDistance = 4000.0;
		iSparksAmount = 30;
		iCurrentDamage += (-3 + random->RandomInt(0,6));
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_338MAG ) )
	{
		iPenetrationPower = 45;
		flPenetrationDistance = 8000.0;
		iSparksAmount = 30;
		iCurrentDamage += (-4 + random->RandomInt(0,8));
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_9MM ) )
	{
		iPenetrationPower = 21;
		flPenetrationDistance = 800.0;
		iSparksAmount = 15;
		iCurrentDamage += (-4 + random->RandomInt(0,10));
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_BUCKSHOT ) )
	{
		iPenetrationPower = 0;
		flPenetrationDistance = 0.0;
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_45ACP ) )
	{
		iPenetrationPower = 15;
		flPenetrationDistance = 500.0;
		iSparksAmount = 20;
		iCurrentDamage += (-2 + random->RandomInt(0,4));
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_357SIG ) )
	{
		iPenetrationPower = 25;
		flPenetrationDistance = 800.0;
		iSparksAmount = 20;
		iCurrentDamage += (-4 + random->RandomInt(0,10));
	}
	else if ( IsAmmoType( iBulletType, BULLET_PLAYER_57MM ) )
	{
		iPenetrationPower = 30;
		flPenetrationDistance = 2000.0;
		iSparksAmount = 20;
		iCurrentDamage += (-4 + random->RandomInt(0,10));
	}
	else
	{
		// What kind of ammo is this?
		Assert( false );
		iPenetrationPower = 0;
		flPenetrationDistance = 0.0;
	}

	/*
	switch ( iBulletType )
	{
		case BULLET_PLAYER_9MM		: iPenetrationPower = 21; flPenetrationDistance = 800.0; iSparksAmount = 15; iCurrentDamage += (-4 + random->RandomInt(0,10)); break;
		case BULLET_PLAYER_45ACP	: iPenetrationPower = 15;  flPenetrationDistance = 500.0; iSparksAmount = 20; iCurrentDamage += (-2 + random->RandomInt(0,4)); break;
		case BULLET_PLAYER_50AE		: iPenetrationPower = 30;  flPenetrationDistance = 1000.0; iSparksAmount = 20;  iCurrentDamage += (-4 + random->RandomInt(0,10)); break;
		case BULLET_PLAYER_762MM	: iPenetrationPower = 39; flPenetrationDistance = 5000.0; iSparksAmount = 30;  iCurrentDamage += (-2 + random->RandomInt(0,4)); break;
		case BULLET_PLAYER_556MM	: iPenetrationPower = 35; flPenetrationDistance = 4000.0; iSparksAmount = 30;  iCurrentDamage += (-3 + random->RandomInt(0,6));break;
		case BULLET_PLAYER_338MAG	: iPenetrationPower = 45; flPenetrationDistance = 8000.0; iSparksAmount = 30; iCurrentDamage += (-4 + random->RandomInt(0,8));break;
		case BULLET_PLAYER_57MM		: iPenetrationPower = 30; flPenetrationDistance = 2000.0; iSparksAmount = 20; iCurrentDamage += (-4 + random->RandomInt(0,10));break;
		case BULLET_PLAYER_357SIG	: iPenetrationPower = 25; flPenetrationDistance = 800.0; iSparksAmount = 20; iCurrentDamage += (-4 + random->RandomInt(0,10));break;
		default : iPenetrationPower = 0; flPenetrationDistance = 0.0; break;
	}
	*/
}


Vector CCSPlayer::FireBullets3( 
	Vector vecSrc, 
	const QAngle &shootAngles, 
	float vecSpread, 
	float flDistance, 
	int iPenetration, 
	int iBulletType, 
	int iDamage, 
	float flRangeModifier, 
	CBaseEntity *pevAttacker )
{
	int iCurrentDamage = iDamage;   // damage of the bullet at it's current trajectory
	float flCurrentDistance = 0.0;  //distance that the bullet has traveled so far
	trace_t tr, tr2;
	CBaseEntity *pEntity;
	bool bHitMetal = FALSE;

	Vector vecDirShooting, vecRight, vecUp;
	AngleVectors( shootAngles, &vecDirShooting, &vecRight, &vecUp );

	
	// MIKETODO: put all the ammo parameters into a script file and allow for CS-specific params.
	int iPenetrationPower;			// thickness of a wall that this bullet can penetrate
	float flPenetrationDistance;		// distance at which the bullet is capable of penetrating a wall
	int iSparksAmount;
	GetBulletTypeParameters( iBulletType, iPenetrationPower, flPenetrationDistance, iSparksAmount, iCurrentDamage );


	if ( !pevAttacker )
		pevAttacker = this;  // the default attacker is ourselves

	// get circular gaussian spread
	float x, y, z;

	// Adrian - Use the player's random seed to have both .dlls sync shots.

	if ( IsPlayer() )
	{
		x = SHARED_RANDOMFLOAT( -0.5, 0.5 ) + SHARED_RANDOMFLOAT( -0.5, 0.5 );
		y = SHARED_RANDOMFLOAT( -0.5, 0.5 ) + SHARED_RANDOMFLOAT( -0.5, 0.5 );
		z = x * x + y * y;
	}
	else
	{
		do {
			x = SHARED_RANDOMFLOAT(-0.5,0.5) + SHARED_RANDOMFLOAT(-0.5,0.5);
			y = SHARED_RANDOMFLOAT(-0.5,0.5) + SHARED_RANDOMFLOAT(-0.5,0.5);
			z = x * x + y * y;
		} while (z > 1);
	}

	Vector vecDir = vecDirShooting +
	      x * vecSpread * vecRight +
		  y * vecSpread * vecUp;

	Vector vecEnd;
	Vector vecOldSrc;
	Vector vecNewSrc;

	vecEnd = vecSrc + vecDir * flDistance;

	float flDamageModifier = 0.5;  // default modification of bullets power after they go through a wall.
	while (iPenetration != 0)
	{
		ClearMultiDamage();
		UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &tr );

		/************* MATERIAL DETECTION ***********/
		surfacedata_t *pSurfaceData = physprops->GetSurfaceData( tr.surface.surfaceProps );
		int iMaterial = pSurfaceData->gameMaterial;
		bool bSparks = FALSE;

		switch ( iMaterial )
		{
			case CHAR_TEX_METAL		: bSparks = TRUE; bHitMetal = TRUE;
				iPenetrationPower = (int) (iPenetrationPower * 0.15);  // If we hit metal, reduce the thickness of the brush we can't penetrate
				flDamageModifier = 0.2;
			break;
			case CHAR_TEX_CONCRETE	: //bSparks = TRUE; bHitMetal = TRUE;
				iPenetrationPower = (int) (iPenetrationPower * 0.25);break;
				flDamageModifier = 0.25;
			case CHAR_TEX_GRATE		: bSparks = TRUE; bHitMetal = TRUE; 
				iPenetrationPower = (int) (iPenetrationPower * 0.5);
				flDamageModifier = 0.4;
				break;
			case CHAR_TEX_VENT		: bSparks = TRUE; bHitMetal = TRUE;
				iPenetrationPower = (int) (iPenetrationPower * 0.5);
				flDamageModifier = 0.45;
				break;
			case CHAR_TEX_TILE		: //bSparks = TRUE; 
				iPenetrationPower = (int) (iPenetrationPower * 0.65);
				flDamageModifier = 0.3;
				break;
			case CHAR_TEX_COMPUTER	: bSparks = TRUE; bHitMetal = TRUE; 
				iPenetrationPower = (int) (iPenetrationPower * 0.4);
				flDamageModifier = 0.45;
				break;
			case CHAR_TEX_WOOD		:
				iPenetrationPower = (int) (iPenetrationPower * 1);
				flDamageModifier = 0.6;
				break;
			default : bSparks = FALSE; break;
		}


		if ( tr.fraction != 1.0 )
		{
			pEntity = tr.m_pEnt;
			iPenetration--;
			//calculate the damage based on the distance the bullet travelled.
			flCurrentDistance = tr.fraction * flDistance;
			iCurrentDamage = iCurrentDamage * (pow (flRangeModifier, (flCurrentDistance / 500)));

		
			if (flCurrentDistance > flPenetrationDistance)
				iPenetration = 0;
		
			//Hit a shield, AND IT'S MIGHTY STRONG!. Don't penetrate.
			//MIKETODO: get shields working..
			/*
			if ( tr.iHitgroup == HITGROUP_SHIELD )
			{
				 iPenetration = 0;

				 if (tr.fraction != 1.0)
				 {
					 if ( RANDOM_LONG( 0, 1 ) )
						EMIT_SOUND(ENT(pEntity->pev), CHAN_VOICE, "weapons/ric_metal-1.wav", 1, ATTN_NORM );
					 else
						EMIT_SOUND(ENT(pEntity->pev), CHAN_VOICE, "weapons/ric_metal-2.wav", 1, ATTN_NORM );

					 UTIL_Sparks( tr.vecEndPos );
								
					 pEntity->pev->punchangle.x = iCurrentDamage * RANDOM_FLOAT ( -0.15, 0.15 );
					 pEntity->pev->punchangle.z = iCurrentDamage * RANDOM_FLOAT ( -0.15, 0.15 );

	 				 if ( pEntity->pev->punchangle.x < 4 )
						  pEntity->pev->punchangle.x = -4;

					 if ( pEntity->pev->punchangle.z < -5 )
						  pEntity->pev->punchangle.z = -5;
					 else if ( pEntity->pev->punchangle.z > 5 )
						  pEntity->pev->punchangle.z = 5;
				 }

				 break;
			}
			*/

			int iDamageType = DMG_BULLET | DMG_NEVERGIB;
			//int iDamageType = CSGameRules()->GetAmmoDef()->DamageType( iBulletType );
			UTIL_ImpactTrace( &tr, iDamageType );

			// Apply damage. Server only.
			#ifndef CLIENT_DLL			
				if ( (tr.m_pEnt->GetSolid() == SOLID_BSP) && ( iPenetration != 0 ) )
				{
					vecSrc = tr.endpos + (vecDir * iPenetrationPower);
					flDistance = (flDistance - flCurrentDistance) * 0.5;
					vecEnd = vecSrc + (vecDir * flDistance);

					CTakeDamageInfo info( pevAttacker, pevAttacker, iCurrentDamage, iDamageType );
					pEntity->DispatchTraceAttack( info, vecDir, &tr );
					
					iCurrentDamage *= flDamageModifier;
				}
				else
				{
					vecSrc = tr.endpos + (vecDir * 42);
					flDistance = (flDistance - flCurrentDistance) * 0.75;
					vecEnd = vecSrc + (vecDir * flDistance);

					CTakeDamageInfo info( pevAttacker, pevAttacker, iCurrentDamage, iDamageType );
					pEntity->DispatchTraceAttack( info, vecDir, &tr );
					
					iCurrentDamage *= 0.75;
				} 
			#endif
		}
		else
		{
			iPenetration = 0;
		}

		ApplyMultiDamage();
	}

	return Vector( x * vecSpread, y * vecSpread, 0 );
}


bool CCSPlayer::CanMove() const
{
	if ( IsObserver() )
		return true; // observers can move all the time

	bool bValidMoveState = (State_Get() == STATE_JOINED);
			
	if ( m_bIsDefusing || !bValidMoveState || CSGameRules()->IsFreezePeriod() )
	{
		return false;
	}
	else
	{
		// Can't move while planting C4.
		CC4 *pC4 = dynamic_cast< CC4* >( GetActiveWeapon() );
		if ( pC4 && pC4->m_bStartedArming )
			return false;

		return true;
	}
}


bool CCSPlayer::HasC4() const
{
	return Weapon_OwnsThisType( "weapon_c4" ) != NULL;
}


