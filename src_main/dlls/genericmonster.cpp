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
//=========================================================
// Generic NPC - purely for scripted sequence work.
//=========================================================
#include "cbase.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "engine/IEngineSound.h"


// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICNPC_NOTSOLID					4 

//=========================================================
// NPC's Anim Events Go Here
//=========================================================

class CGenericNPC : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CGenericNPC, CAI_BaseNPC );

	void	Spawn( void );
	void	Precache( void );
	float	MaxYawSpeed( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests ( void );

	void	TempGunEffect( void );
};

LINK_ENTITY_TO_CLASS( monster_generic, CGenericNPC );

//=========================================================
// Classify - indicates this NPC's place in the 
// relationship table.
//=========================================================
Class_T	CGenericNPC :: Classify ( void )
{
	return	CLASS_NONE;
}


//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CGenericNPC :: MaxYawSpeed ( void )
{
	return 90;
}

//---------------------------------------------------------
// !!!TEMP
// !!!TEMP
// !!!TEMP
// !!!TEMP
//
// (sjb)
//---------------------------------------------------------
void CGenericNPC::TempGunEffect( void )
{
	QAngle vecAngle;
	Vector vecDir, vecShot;
	Vector vecMuzzle, vecButt;

	GetAttachment( 2, vecMuzzle, vecAngle );
	GetAttachment( 3, vecButt, vecAngle );

	vecDir = vecMuzzle - vecButt;
	VectorNormalize( vecDir );

	// CPVSFilter filter( GetAbsOrigin() );
	//te->ShowLine( filter, 0.0, vecSpot, vecSpot + vecForward );
	//UTIL_Sparks( vecMuzzle );

	bool fSound = false;
	
	if( random->RandomInt( 0, 3 ) == 0 )
	{
		fSound = true;
	}

	Vector start = vecMuzzle + vecDir * 64;
	Vector end = vecMuzzle + vecDir * 4096;
	UTIL_Tracer( start, end, 0, TRACER_DONT_USE_ATTACHMENT, 5500, fSound );
	CPASAttenuationFilter filter2( this, "GenericNPC.GunSound" );
	EmitSound( filter2, entindex(), "GenericNPC.GunSound" );
}


//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericNPC :: HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		// TEMPORARLY. Makes the May 2001 sniper demo work (sjb)
		TempGunEffect();
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// GetSoundInterests - generic NPC can't hear.
//=========================================================
int CGenericNPC :: GetSoundInterests ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CGenericNPC :: Spawn()
{
	Precache();

	SetModel( STRING( GetModelName() ) );

/*
	if ( FStrEq( STRING( GetModelName() ), "models/player.mdl" ) )
		UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if ( FStrEq( STRING( GetModelName() ), "models/player.mdl" ) || FStrEq( STRING( GetModelName() ), "models/holo.mdl" ) )
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
	else
		UTIL_SetSize(this, NAI_Hull::Mins(HULL_HUMAN), NAI_Hull::Maxs(HULL_HUMAN));

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_RED;
	m_iHealth			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS );

	NPCInit();
	if ( !HasSpawnFlags(SF_GENERICNPC_NOTSOLID) )
	{
		trace_t tr;
		UTIL_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin(), MASK_SOLID, &tr );
		if ( tr.startsolid )
		{
			Msg("Placed npc_generic in solid!!! (%s)\n", STRING(GetModelName()) );
			m_spawnflags |= SF_GENERICNPC_NOTSOLID;
		}
	}

	if ( HasSpawnFlags(SF_GENERICNPC_NOTSOLID) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_takedamage = DAMAGE_NO;
		VPhysicsDestroyObject();
	}

	Relink();
}

//=========================================================
// Precache - precaches all resources this NPC needs
//=========================================================
void CGenericNPC :: Precache()
{
	engine->PrecacheModel( STRING( GetModelName() ) );
}	

//=========================================================
// AI Schedules Specific to this NPC
//=========================================================
