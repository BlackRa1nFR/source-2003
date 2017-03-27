//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "flashbang_projectile.h"
#include "shake.h"


#define GRENADE_MODEL "models/Weapons/w_grenade.mdl"


LINK_ENTITY_TO_CLASS( flashbang_projectile, CFlashbangProjectile );
PRECACHE_WEAPON_REGISTER( flashbang_projectile );

BEGIN_DATADESC( CFlashbangProjectile )
	DEFINE_FUNCTION( CFlashbangProjectile, FlashbangDetonate )
END_DATADESC()


// --------------------------------------------------------------------------------------------------- //
//
// RadiusDamage - this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
// 
// only damage ents that can clearly be seen by the explosion!
// --------------------------------------------------------------------------------------------------- //

void RadiusFlash( 
	Vector vecSrc, 
	CBaseEntity *pevInflictor, 
	CBaseEntity *pevAttacker, 
	float flDamage, 
	int iClassIgnore, 
	int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	trace_t tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;
	float		flRadius = 1500;


	if ( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0;

	int bInWater = (UTIL_PointContents ( vecSrc ) == CONTENTS_WATER);

	vecSrc.z += 1;// in case grenade is lying on the ground

	if ( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while ((pEntity = gEntList.FindEntityInSphere( pEntity, vecSrc, flRadius )) != NULL)
	{
		// get the heck out of here if it aint a player.
		if (pEntity->IsPlayer() == FALSE)
			continue;

		if (( pEntity->m_takedamage != DAMAGE_NO ) && (pEntity->m_lifeState != LIFE_DEAD))
		{
			// blast's don't tavel into or out of water
			if (bInWater && pEntity->GetWaterLevel() == 0)
				continue;
			if (!bInWater && pEntity->GetWaterLevel() == 3)
				continue;

			vecSpot = pEntity->BodyTarget( vecSrc );
			
			UTIL_TraceLine( vecSrc, vecSpot, MASK_SHOT, pevInflictor, COLLISION_GROUP_NONE, &tr );

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
				flAdjustedDamage = flDamage - flAdjustedDamage;
			
				if ( flAdjustedDamage < 0 )
				{
					flAdjustedDamage = 0;
				}
			
				Vector vecLOS;
				float flDot;
				Vector vForward;
				AngleVectors( pEntity->EyeAngles(), &vForward );

				vecLOS = ( vecSrc - (pEntity->EyePosition()));
					
				flDot = DotProduct (vecLOS, vForward);


				float fadeTime, fadeHold;
				int alpha;
				
				// if target is facing the bomb, the effect lasts longer
				if (flDot >= 0.0)
				{
					fadeTime = flAdjustedDamage * 3.0f;
					fadeHold = flAdjustedDamage / 1.5f;
					alpha = 255;
				}
				else
				{
					fadeTime = flAdjustedDamage * 1.75f;
					fadeHold = flAdjustedDamage / 3.5f;
					alpha = 200;
				}
				
				color32 clr = { 255, 255, 255, 255 };
				UTIL_ScreenFade( pEntity, clr, fadeTime, fadeHold, FFADE_IN );

				//MIKETODO: bots
				/*
				CBasePlayer *pPlayer = static_cast<CBasePlayer *>( pEntity );
				if (pPlayer->IsBot())
				{
					// blind the bot
					CCSBot *pBot = static_cast<CCSBot *>( pPlayer );
					pBot->Blind( 0.33f * fadeTime );
				}
				*/
			}
		}
	}
}


// --------------------------------------------------------------------------------------------------- //
// CFlashbangProjectile implementation.
// --------------------------------------------------------------------------------------------------- //

CFlashbangProjectile* CFlashbangProjectile::Create( 
	const Vector &position, 
	const QAngle &angles, 
	const Vector &velocity, 
	const AngularImpulse &angVelocity, 
	CBaseCombatCharacter *pOwner, 
	float timer )
{
	CFlashbangProjectile *pGrenade = (CFlashbangProjectile*)CBaseEntity::Create( "flashbang_projectile", position, angles, pOwner );
	
	// Set the timer for 1 second less than requested. We're going to issue a SOUND_DANGER
	// one second before detonation.
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY );
	pGrenade->SetMoveCollide( MOVECOLLIDE_FLY_BOUNCE );
	pGrenade->SetAbsVelocity( velocity );
	pGrenade->SetOwner( pOwner );
	pGrenade->m_flDamage = 100;
	pGrenade->ChangeTeam( pOwner->GetTeamNumber() );

	pGrenade->SetThink( &CFlashbangProjectile::FlashbangDetonate );
	pGrenade->SetNextThink( gpGlobals->curtime + timer );

	return pGrenade;
}

void CFlashbangProjectile::Spawn()
{
	SetModel( GRENADE_MODEL );

	BaseClass::Spawn();
}


void CFlashbangProjectile::Precache()
{
	engine->PrecacheModel( GRENADE_MODEL );
	BaseClass::Precache();
}


void CFlashbangProjectile::FlashbangDetonate()
{
	RadiusFlash ( GetAbsOrigin(), this, GetOwner(), 4, CLASS_NONE, DMG_BLAST );

	UTIL_Remove( this );
}

