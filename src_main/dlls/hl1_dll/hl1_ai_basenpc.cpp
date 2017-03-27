/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#include "cbase.h"
#include "hl1_ai_basenpc.h"
#include "scripted.h"
#include "soundent.h"
#include "animation.h"
#include "EntityList.h"
#include "AI_Navigator.h"
#include "AI_Motor.h"
#include "player.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "NPCevent.h"

#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "cplane.h"
#include "ai_squad.h"

#define HUMAN_GIBS 1
#define ALIEN_GIBS 2

//=========================================================
// NoFriendlyFire - checks for possibility of friendly fire
//
// Builds a large box in front of the grunt and checks to see 
// if any squad members are in that box. 
//=========================================================
bool CHL1BaseNPC::NoFriendlyFire( void )
{
	if ( !m_pSquad )
	{
		return true;
	}

	CPlane	backPlane;
	CPlane  leftPlane;
	CPlane	rightPlane;

	Vector	vecLeftSide;
	Vector	vecRightSide;
	Vector	v_left;

	Vector  vForward, vRight, vUp;
	QAngle  vAngleToEnemy;

	VectorAngles( ( GetEnemy()->WorldSpaceCenter() - GetAbsOrigin() ), vAngleToEnemy );

	//!!!BUGBUG - to fix this, the planes must be aligned to where the monster will be firing its gun, not the direction it is facing!!!

	if ( GetEnemy() != NULL )
	{
		AngleVectors ( vAngleToEnemy, &vForward, &vRight, &vUp );
	}
	else
	{
		// if there's no enemy, pretend there's a friendly in the way, so the grunt won't shoot.
		return false;
	}
	
	vecLeftSide = GetAbsOrigin() - ( vRight * ( WorldAlignSize().x * 1.5 ) );
	vecRightSide = GetAbsOrigin() + ( vRight * ( WorldAlignSize().x * 1.5 ) );
	v_left = vRight * -1;

	leftPlane.InitializePlane ( vRight, vecLeftSide );
	rightPlane.InitializePlane ( v_left, vecRightSide );
	backPlane.InitializePlane ( vForward, GetAbsOrigin() );

	AISquadIter_t iter;
	for ( CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
	{
		if ( pSquadMember == NULL )
			 continue;

		if ( pSquadMember == this )
			 continue;

		if ( backPlane.PointInFront  ( pSquadMember->GetAbsOrigin() ) &&
				 leftPlane.PointInFront  ( pSquadMember->GetAbsOrigin() ) && 
				 rightPlane.PointInFront ( pSquadMember->GetAbsOrigin()) )
			{
				// this guy is in the check volume! Don't shoot!
				return false;
			}
	}

	return true;
}

void CHL1BaseNPC::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( info.GetDamage() >= 1.0 && !(info.GetDamageType() & DMG_SHOCK ) )
	{
		UTIL_BloodSpray( ptr->endpos, vecDir, BloodColor(), 4, FX_BLOODSPRAY_ALL );	
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}


bool CHL1BaseNPC::ShouldGib( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & DMG_NEVERGIB )
		 return false;

	if ( ( info.GetDamageType() & DMG_GIB_CORPSE && m_iHealth < GIB_HEALTH_VALUE ) || ( info.GetDamageType() & DMG_ALWAYSGIB ) )
		 return true;
	
	return false;
	
}

bool CHL1BaseNPC::HasHumanGibs( void )
{
	Class_T myClass = Classify();

	if ( myClass == CLASS_HUMAN_MILITARY ||
		 myClass == CLASS_PLAYER_ALLY	||
		 myClass == CLASS_HUMAN_PASSIVE  ||
		 myClass == CLASS_PLAYER )

		 return true;

	return false;
}


bool CHL1BaseNPC::HasAlienGibs( void )
{
	Class_T myClass = Classify();

	if ( myClass == CLASS_ALIEN_MILITARY ||
		 myClass == CLASS_ALIEN_MONSTER	||
		 myClass == CLASS_INSECT  ||
		 myClass == CLASS_ALIEN_PREDATOR  ||
		 myClass == CLASS_ALIEN_PREY )

		 return true;

	return false;
}


void CHL1BaseNPC::Precache( void )
{
	engine->PrecacheModel( "models/agibs.mdl" );
	engine->PrecacheModel( "models/hgibs.mdl" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL1BaseNPC::CorpseGib( const CTakeDamageInfo &info )
{
	CEffectData	data;
	
	data.m_vOrigin = WorldSpaceCenter();
	data.m_vNormal = data.m_vOrigin - info.GetDamagePosition();
	VectorNormalize( data.m_vNormal );
	
	data.m_flScale = RemapVal( m_iHealth, 0, -500, 1, 3 );
	data.m_flScale = clamp( data.m_flScale, 1, 3 );

	if ( HasAlienGibs() )
		 data.m_nMaterial = ALIEN_GIBS;
	else if ( HasHumanGibs() )
		 data.m_nMaterial = HUMAN_GIBS;
	
	data.m_nColor = BloodColor();

	DispatchEffect( "HL1Gib", data );

	CSoundEnt::InsertSound( SOUND_MEAT, GetAbsOrigin(), 256, 0.5f, this );

///	BaseClass::CorpseGib( info );

	return true;
}

int	CHL1BaseNPC::IRelationPriority( CBaseEntity *pTarget )
{
	return BaseClass::IRelationPriority( pTarget );
}