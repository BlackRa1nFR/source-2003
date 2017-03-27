/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// GameRules.cpp
//=========================================================

#include "cbase.h"
#include "gamerules.h"
#include "ammodef.h"

#ifdef CLIENT_DLL

#else

	#include "player.h"
	#include "teamplay_gamerules.h"
	#include "game.h"
	#include "entitylist.h"
	#include "basecombatweapon.h"
	#include "voice_gamemgr.h"
	#include "globalstate.h"

#endif



IMPLEMENT_NETWORKCLASS_ALIASED( GameRules, DT_GameRules )

// Don't send any of the CBaseEntity stuff..
BEGIN_NETWORK_TABLE_NOBASE( CGameRules, DT_GameRules )
END_NETWORK_TABLE()



#ifdef CLIENT_DLL

	CGameRules::CGameRules()
	{
		Assert( !g_pGameRules );
		g_pGameRules = this;
	}	

#else

	// In tf_gamerules.cpp or hl_gamerules.cpp.
	extern IVoiceGameMgrHelper *g_pVoiceGameMgrHelper;


	CGameRules*	g_pGameRules = NULL;
	extern bool	g_fGameOver;

	//-----------------------------------------------------------------------------
	// constructor, destructor
	//-----------------------------------------------------------------------------
	CGameRules::CGameRules()
	{
		Assert( !g_pGameRules );
		g_pGameRules = this;

		GetVoiceGameMgr()->Init( g_pVoiceGameMgrHelper, gpGlobals->maxClients );
		ClearMultiDamage();
	}

	//-----------------------------------------------------------------------------
	// Purpose: Return true if the specified player can carry any more of the ammo type
	//-----------------------------------------------------------------------------
	bool CGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex )
	{
		if ( iAmmoIndex > -1 )
		{
			// Get the max carrying capacity for this ammo
			int iMaxCarry = GetAmmoDef()->MaxCarry( iAmmoIndex );

			// Does the player have room for more of this type of ammo?
			if ( pPlayer->GetAmmoCount( iAmmoIndex ) < iMaxCarry )
				return true;
		}

		return false;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Return true if the specified player can carry any more of the ammo type
	//-----------------------------------------------------------------------------
	bool CGameRules::CanHaveAmmo( CBaseCombatCharacter *pPlayer, const char *szName )
	{
		return CanHaveAmmo( pPlayer, GetAmmoDef()->Index(szName) );
	}

	//=========================================================
	//=========================================================
	CBaseEntity *CGameRules :: GetPlayerSpawnSpot( CBasePlayer *pPlayer )
	{
		CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();
		Assert( pSpawnSpot );

		pPlayer->SetLocalOrigin( pSpawnSpot->GetAbsOrigin() + Vector(0,0,1) );
		pPlayer->SetAbsVelocity( vec3_origin );
		pPlayer->SetLocalAngles( pSpawnSpot->GetLocalAngles() );
		pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
		pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
		pPlayer->SnapEyeAngles( pSpawnSpot->GetLocalAngles() );
		UTIL_Relink( pPlayer );

		return pSpawnSpot;
	}

	//=========================================================
	//=========================================================
	bool CGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
	{
	/*
		if ( pWeapon->m_pszAmmo1 )
		{
			if ( !CanHaveAmmo( pPlayer, pWeapon->m_iPrimaryAmmoType ) )
			{
				// we can't carry anymore ammo for this gun. We can only 
				// have the gun if we aren't already carrying one of this type
				if ( pPlayer->Weapon_OwnsThisType( pWeapon ) )
				{
					return FALSE;
				}
			}
		}
		else
		{
			// weapon doesn't use ammo, don't take another if you already have it.
			if ( pPlayer->Weapon_OwnsThisType( pWeapon ) )
			{
				return FALSE;
			}
		}
	*/
		// note: will fall through to here if GetItemInfo doesn't fill the struct!
		return TRUE;
	}

	bool CGameRules::ShouldTransmit( const edict_t *recipient, const void *pvs, int clientArea )
	{
		// ALWAYS transmit to all clients.
		return true;
	}

	//=========================================================
	// load the SkillData struct with the proper values based on the skill level.
	//=========================================================
	void CGameRules::RefreshSkillData ( bool forceUpdate )
	{
#ifndef CLIENT_DLL
		if ( !forceUpdate )
		{
			if ( GlobalEntity_IsInTable( "skill.cfg" ) )
				return;
		}
		GlobalEntity_Add( "skill.cfg", STRING(gpGlobals->mapname), GLOBAL_ON );
		char	szExec[256];

		ConVar const *skill = cvar->FindVar( "skill" );

		g_iSkillLevel = skill ? skill->GetInt() : 1;

		if ( g_iSkillLevel < 1 )
		{
			g_iSkillLevel = 1;
		}
		else if ( g_iSkillLevel > 3 )
		{
			g_iSkillLevel = 3; 
		}

		Q_snprintf( szExec,sizeof(szExec), "exec skill%d.cfg\n", g_iSkillLevel );

		engine->ServerCommand( szExec );
		engine->ServerExecute();
#endif
	}


	//-----------------------------------------------------------------------------
	// Default implementation of radius damage
	//-----------------------------------------------------------------------------
	void CGameRules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore )
	{
		CBaseEntity *pEntity = NULL;
		trace_t		tr;
		float		flAdjustedDamage, falloff;
		Vector		vecSpot;

		Vector vecSrc = vecSrcIn;

		if ( flRadius )
			falloff = info.GetDamage() / flRadius;
		else
			falloff = 1.0;

		int bInWater = (UTIL_PointContents ( vecSrc ) & MASK_WATER) ? true : false;
		
		vecSrc.z += 1;// in case grenade is lying on the ground

		// iterate on all entities in the vicinity.
		for ( CEntitySphereQuery sphere( vecSrc, flRadius ); pEntity = sphere.GetCurrentEntity(); sphere.NextEntity() )
		{
			if ( pEntity->m_takedamage != DAMAGE_NO )
			{
				// UNDONE: this should check a damage mask, not an ignore
				if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
				{// houndeyes don't hurt other houndeyes with their attack
					continue;
				}

				// blast's don't tavel into or out of water
				if (bInWater && pEntity->GetWaterLevel() == 0)
					continue;
				if (!bInWater && pEntity->GetWaterLevel() == 3)
					continue;

				// radius damage can only be blocked by the world
				vecSpot = pEntity->BodyTarget( vecSrc );

 				CTraceFilterWorldAndPropsOnly traceFilter;
  				UTIL_TraceLine( vecSrc, vecSpot, CONTENTS_SOLID, &traceFilter, &tr );
				
				if ( tr.fraction == 1.0 || tr.m_pEnt == pEntity )
				{// the explosion can 'see' this entity, so hurt them!
					if (tr.startsolid)
					{
						// if we're stuck inside them, fixup the position and distance
						tr.endpos = vecSrc;
						tr.fraction = 0.0;
					}
					
					// decrease damage for an ent that's farther from the bomb.
					flAdjustedDamage = ( vecSrc - tr.endpos ).Length() * falloff;
					flAdjustedDamage = info.GetDamage() - flAdjustedDamage;
				
					if ( flAdjustedDamage > 0 )
					{
						CTakeDamageInfo adjustedInfo = info;
						adjustedInfo.SetDamage( flAdjustedDamage );

						Vector dir = tr.endpos - vecSrc;
						VectorNormalize( dir );

						// If we don't have a damage force, manufacture one
						if ( adjustedInfo.GetDamagePosition() == vec3_origin || adjustedInfo.GetDamageForce() == vec3_origin )
						{
							CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc );
						}
						else
						{
							// Assume the force passed in is the maximum force. Decay it based on falloff.
							float flForce = adjustedInfo.GetDamageForce().Length() * falloff;
							adjustedInfo.SetDamageForce( dir * flForce );
							adjustedInfo.SetDamagePosition( vecSrc );
						}

						if (tr.fraction != 1.0)
						{
							ClearMultiDamage( );
							pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
							ApplyMultiDamage();
						}
						else
						{
							pEntity->TakeDamage( adjustedInfo );
						}
						// Now hit all triggers along the way that respond to damage... 
						pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, tr.endpos, dir );
					}
				}
			}
		}
	}


	bool CGameRules::ClientCommand( const char *pcmd, CBaseEntity *pEdict )
	{
		if( pEdict->IsPlayer() )
		{
			if( GetVoiceGameMgr()->ClientCommand( static_cast<CBasePlayer*>(pEdict), pcmd ) )
				return true;
		}

		return false;
	}


	void CGameRules::Think()
	{
		GetVoiceGameMgr()->Update( gpGlobals->frametime );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Called at the end of GameFrame (i.e. after all game logic has run this frame)
	//-----------------------------------------------------------------------------
	void CGameRules::EndGameFrame( void )
	{
		// If you hit this assert, it means something called AddMultiDamage() and didn't ApplyMultiDamage().
		// The g_MultiDamage.m_hAttacker & g_MultiDamage.m_hInflictor should give help you figure out the culprit.
		Assert( g_MultiDamage.IsClear() );
	}

	//-----------------------------------------------------------------------------
	// trace line rules
	//-----------------------------------------------------------------------------
	float CGameRules::WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd,
						 unsigned int mask, trace_t *ptr )
	{
		UTIL_TraceEntity( pEntity, vecStart, vecEnd, mask, ptr );
		return 1.0f;
	}


#endif


// ----------------------------------------------------------------------------- //
// Shared CGameRules implementation.
// ----------------------------------------------------------------------------- //

CGameRules::~CGameRules()
{
	Assert( g_pGameRules == this );
	g_pGameRules = NULL;
}

bool CGameRules::SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	return false;
}

CBaseCombatWeapon *CGameRules::GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
	return false;
}

bool CGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		int tmp = collisionGroup0;
		collisionGroup0 = collisionGroup1;
		collisionGroup1 = tmp;
	}
	
	// --------------------------------------------------------------------------
	// NOTE: All of this code assumes the collision groups have been sorted!!!!
	// NOTE: Don't change their order without rewriting this code !!!
	// --------------------------------------------------------------------------


	// Don't bother if either is in a vehicle...
	if (( collisionGroup0 == COLLISION_GROUP_IN_VEHICLE ) || ( collisionGroup1 == COLLISION_GROUP_IN_VEHICLE ))
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS )
	{
		// put exceptions here, right now this will only collide with COLLISION_GROUP_NONE
		return false;
	}
	// doesn't collide with other members of this group
	// or debris, but that's handled above
	if ( collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && collisionGroup1 == COLLISION_GROUP_INTERACTIVE_DEBRIS )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_BREAKABLE_GLASS && collisionGroup1 == COLLISION_GROUP_BREAKABLE_GLASS )
		return false;

	// interactive objects collide with everything except debris & interactive debris
	if ( collisionGroup1 == COLLISION_GROUP_INTERACTIVE && collisionGroup0 != COLLISION_GROUP_NONE )
		return false;

	// Don't let vehicles collide with weapons
	// Don't let players collide with weapons...
	// Don't let NPCs collide with weapons
	// Weapons are triggers, too, so they should still touch because of that
	if ( collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		if ( collisionGroup0 == COLLISION_GROUP_VEHICLE || 
			collisionGroup0 == COLLISION_GROUP_PLAYER ||
			collisionGroup0 == COLLISION_GROUP_NPC )
		{
			return false;
		}
	}

	// collision with vehicle clip entity??
	if ( collisionGroup0 == COLLISION_GROUP_VEHICLE_CLIP || collisionGroup1 == COLLISION_GROUP_VEHICLE_CLIP )
	{
		// yes then if it's a vehicle, collide, otherwise no collision
		// vehicle sorts lower than vehicle clip, so must be in 0
		if ( collisionGroup0 == COLLISION_GROUP_VEHICLE )
			return true;
		// vehicle clip against non-vehicle, no collision
		return false;
	}

	return true;
}




