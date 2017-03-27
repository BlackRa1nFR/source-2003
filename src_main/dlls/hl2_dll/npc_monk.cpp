//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Father Grigori, a benevolent monk who is the last remaining human
//			in Ravenholm. He keeps to the rooftops and uses a big ole elephant
//			gun to send his zombified former friends to a peaceful death.
//
//=============================================================================

#include "cbase.h"
#include "NPCEvent.h"
#include "ai_baseactor.h"
#include "ai_hull.h"
#include "AmmoDef.h"
#include "gamerules.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "ai_behavior.h"


//-----------------------------------------------------------------------------
// Activities.
//-----------------------------------------------------------------------------
int ACT_MONK_GUN_IDLE;


//-----------------------------------------------------------------------------
// Bodygroups.
//-----------------------------------------------------------------------------
enum
{
	BODYGROUP_MONK_GUN = 1,
};


//-----------------------------------------------------------------------------
// Animation events.
//-----------------------------------------------------------------------------
enum
{
	AE_MONK_FIRE_GUN = 50,
};


class CNPC_Monk : public CAI_BaseActor
{
	typedef CAI_BaseActor BaseClass;

public:

	CNPC_Monk() {}
	void Spawn();
	void Precache();

	void BuildScheduleTestBits( void );
	Class_T	Classify( void );

	float MaxYawSpeed();
	int	TranslateSchedule( int scheduleType );
	int	SelectSchedule ();

	void HandleAnimEvent( animevent_t *pEvent );
	int RangeAttack1Conditions ( float flDot, float flDist );
	Activity NPC_TranslateActivity( Activity eNewActivity );

	void PrescheduleThink();

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	DECLARE_DATADESC();

private:

	DEFINE_CUSTOM_AI;
	
	bool m_bHasGun;
	int m_nAmmoType;
};

BEGIN_DATADESC( CNPC_Monk )

	DEFINE_KEYFIELD( CNPC_Monk, m_bHasGun, FIELD_BOOLEAN, "HasGun" ),
	DEFINE_FIELD( CNPC_Monk, m_nAmmoType, FIELD_INTEGER ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_monk, CNPC_Monk );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Monk::BuildScheduleTestBits( void )
{
	// FIXME: we need a way to make scenes non-interruptible
	if ( IsCurSchedule( SCHED_RANGE_ATTACK1 ) || IsCurSchedule( SCHED_SCENE_SEQUENCE ) || IsCurSchedule( SCHED_SCENE_WALK ) )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
		ClearCustomInterruptCondition( COND_NEW_ENEMY );
		ClearCustomInterruptCondition( COND_HEAR_DANGER );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Class_T	CNPC_Monk::Classify( void )
{
	return CLASS_CITIZEN_REBEL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CNPC_Monk::NPC_TranslateActivity( Activity eNewActivity )
{
	if (m_bHasGun)
	{
		//
		// He is carrying Annabelle.
		//
		if (eNewActivity == ACT_IDLE)
		{
			return (Activity)ACT_MONK_GUN_IDLE;
		}

		if ((eNewActivity == ACT_WALK) && (GetActiveWeapon() != NULL))
		{
			// HACK: Run when carrying two weapons. This is for the balcony scene. Remove once we
			//		 can specify movement activities between scene cue points.
			return (Activity)ACT_RUN;
		}
	}

	return eNewActivity;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Monk::Precache()
{
	engine->PrecacheModel( "models/Monk.mdl" );
	
	BaseClass::Precache();
}
 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Monk::Spawn()
{
	Precache();

	BaseClass::Spawn();

	SetModel( "models/Monk.mdl" );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_fEffects			= 0;
	m_iHealth			= 100;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;

	m_nAmmoType = GetAmmoDef()->Index("SniperRound");

	m_HackedGunPos = Vector ( 0, 0, 55 );

	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_CLIMB );
	CapabilitiesAdd( bits_CAP_USE_WEAPONS );
	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );

	if ( m_bHasGun )
	{
		CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 );
	}

	SetBodygroup( BODYGROUP_MONK_GUN, m_bHasGun );

	NPCInit();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
//-----------------------------------------------------------------------------
int CNPC_Monk::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist < 64)
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDist > 1024)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.5)
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CNPC_Monk::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
		case AE_MONK_FIRE_GUN:
		{
			Vector vecShootOrigin;
			QAngle vecAngles;
			GetAttachment( "muzzle", vecShootOrigin, vecAngles );

			Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

			CPASAttenuationFilter filter2( this, "NPC_Monk.Fire" );
			EmitSound( filter2, entindex(), "NPC_Monk.Fire" );

			UTIL_Smoke( vecShootOrigin, random->RandomInt(20, 30), 10 );
			FireBullets( 1, vecShootOrigin, vecShootDir, vec3_origin, MAX_TRACE_LENGTH, m_nAmmoType, 0 );
			m_fEffects |= EF_MUZZLEFLASH;
			break;
		}

		default:
		{
			BaseClass::HandleAnimEvent( pEvent );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_Monk::MaxYawSpeed()
{
	if( IsMoving() )
	{
		return 20;
	}

	switch( GetActivity() )
	{
	case ACT_180_LEFT:
		return 30;
		break;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 30;
		break;
	default:
		return 15;
		break;
	}
}

//-------------------------------------

int CNPC_Monk::TranslateSchedule( int scheduleType ) 
{
	return BaseClass::TranslateSchedule( scheduleType );
}


//-------------------------------------

void CNPC_Monk::PrescheduleThink()
{
	BaseClass::PrescheduleThink();
}	

//-------------------------------------

int CNPC_Monk::SelectSchedule ()
{
	return BaseClass::SelectSchedule();
}

//-------------------------------------

void CNPC_Monk::StartTask( const Task_t *pTask )
{
	BaseClass::StartTask( pTask );
}


void CNPC_Monk::RunTask( const Task_t *pTask )
{
	BaseClass::RunTask( pTask );
}


//-----------------------------------------------------------------------------
//
// CNPC_Monk Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_monk, CNPC_Monk )

	DECLARE_ACTIVITY( ACT_MONK_GUN_IDLE )

AI_END_CUSTOM_NPC()
