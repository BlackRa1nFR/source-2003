//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		This is the base version of the combine (not instanced only subclassed)
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include	"AI_Default.h"
#include	"AI_Task.h"
#include	"AI_Schedule.h"
#include	"AI_Hull.h"
#include	"AI_BaseNPC.h"
#include	"ndebugoverlay.h"
#include	"NPC_Roller.h"
#include	"CBaseSpriteProjectile.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

//=========================================================
// Custom schedules
//=========================================================
enum
{
	SCHED_ROLLERTURRET_OPEN = LAST_ROLLER_SCHED,
	SCHED_ROLLERTURRET_CLOSE,
	SCHED_ROLLERTURRET_RANGE_ATTACK1,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_ROLLERTURRET_OPEN = LAST_ROLLER_TASK,
	TASK_ROLLERTURRET_CLOSE,
};

#if 0
//=========================================================
// Custom Conditions
//=========================================================
enum 
{
	COND_ROLLERTURRET_ = LAST_ROLLER_CONDITION,
};
#endif 


//=========================================================
//=========================================================
class CNPC_RollerTurret : public CNPC_Roller
{
	DECLARE_CLASS( CNPC_RollerTurret, CNPC_Roller );
	DECLARE_DATADESC();

public:
	void Spawn( void );
	int RangeAttack1Conditions ( float flDot, float flDist );
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	int SelectSchedule ( void );

	void Precache( void );

	Class_T	Classify( void ) { return CLASS_COMBINE; }

	void Open( void );
	void Close( void );

	bool m_fIsOpen;

	void ShootLaser( const Vector &vecTarget );

	DEFINE_CUSTOM_AI;
};

LINK_ENTITY_TO_CLASS( npc_rollerturret, CNPC_RollerTurret );

IMPLEMENT_CUSTOM_AI( npc_rollerturret, CNPC_RollerTurret );


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_RollerTurret )

	DEFINE_FIELD( CNPC_RollerTurret, m_fIsOpen,	FIELD_BOOLEAN ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerTurret::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI( CNPC_RollerTurret );

	ADD_CUSTOM_SCHEDULE( CNPC_RollerTurret,		SCHED_ROLLERTURRET_OPEN );
	ADD_CUSTOM_SCHEDULE( CNPC_RollerTurret,		SCHED_ROLLERTURRET_CLOSE );
	ADD_CUSTOM_SCHEDULE( CNPC_RollerTurret,		SCHED_ROLLERTURRET_RANGE_ATTACK1 );

	ADD_CUSTOM_TASK( CNPC_RollerTurret,			TASK_ROLLERTURRET_OPEN );
	ADD_CUSTOM_TASK( CNPC_RollerTurret,			TASK_ROLLERTURRET_CLOSE );

	AI_LOAD_SCHEDULE( CNPC_RollerTurret,		SCHED_ROLLERTURRET_OPEN );
	AI_LOAD_SCHEDULE( CNPC_RollerTurret,		SCHED_ROLLERTURRET_CLOSE );
	AI_LOAD_SCHEDULE( CNPC_RollerTurret,		SCHED_ROLLERTURRET_RANGE_ATTACK1 );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerTurret::Precache( void )
{
	engine->PrecacheModel( "models/roller.mdl" );
	engine->PrecacheModel( "sprites/muzzleflash2.vmt" );
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerTurret::Spawn( void )
{
	BaseClass::Spawn();

	m_fIsOpen = false;

	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_RollerTurret::RangeAttack1Conditions( float flDot, float flDist )
{
	if( !m_fIsOpen )
	{
		if(flDist > 384  )
		{
			return COND_TOO_FAR_TO_ATTACK;
		}
	}
	else if( m_fIsOpen )
	{
		if(flDist > 512  )
		{
			return COND_TOO_FAR_TO_ATTACK;
		}
	}


	return COND_CAN_RANGE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerTurret::Open( void )
{
	Msg( "OPEN!\n" );
	PowerOff();
	m_fIsOpen = true;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerTurret::Close( void )
{
	Msg( "CLOSE!\n" );
	PowerOn();
	m_fIsOpen = false;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_RollerTurret::SelectSchedule ( void )
{
	switch( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if( HasCondition( COND_CAN_RANGE_ATTACK1 ) && !HasCondition( COND_ROLLER_PHYSICS ) )
		{
			if( m_fIsOpen )
			{
				return SCHED_ROLLERTURRET_RANGE_ATTACK1;
			}
			else
			{
				return SCHED_ROLLERTURRET_OPEN;
			}
		}
		
		if( m_fIsOpen )
		{
			return SCHED_ROLLERTURRET_CLOSE;
		}

		if( !HasCondition( COND_CAN_RANGE_ATTACK1 ) && !HasCondition( COND_ROLLER_PHYSICS ) )
		{
			return SCHED_CHASE_ENEMY;
		}

		break;

	default:
		if( m_fIsOpen )
		{
			return SCHED_ROLLERTURRET_CLOSE;
		}
		break;
	}

	return BaseClass::SelectSchedule();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerTurret::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLERTURRET_OPEN:
		PowerOff();
		Open();
		TaskComplete();
		break;

	case TASK_ROLLERTURRET_CLOSE:
		Close();
		TaskComplete();
		break;

	case TASK_RANGE_ATTACK1:
		if( GetEnemy() != NULL )
		{
			ShootLaser( GetEnemy()->WorldSpaceCenter() );
		}

		TaskComplete();
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerTurret::ShootLaser( const Vector &vecTarget )
{
	CBaseSpriteProjectile *pLaser; 
	Vector vecAim;

	EmitSound( "NPC_RollerTurret.Shoot" );

	vecAim = vecTarget - GetLocalOrigin();
	VectorNormalize( vecAim );

	vecAim.x += random->RandomFloat( -0.06, 0.06 );
	vecAim.y += random->RandomFloat( -0.06, 0.06 );
	vecAim.z += random->RandomFloat( -0.06, 0.06 );

	pLaser = (CBaseSpriteProjectile *)CreateEntityByName( "baseprojectile" );

	pLaser->Spawn( "sprites/muzzleflash2.vmt", GetLocalOrigin() + vecAim * 32, 
		vecAim * 1500, pev, MOVETYPE_FLY, MOVECOLLIDE_DEFAULT, 1, DMG_SHOCK );

	pLaser->SetScale( 0.15 );
	pLaser->m_nRenderMode = kRenderTransAdd;
	pLaser->SetRenderColor( 255, 255, 255, 255 );
	QAngle angles = pLaser->GetLocalAngles();
	angles.z = random->RandomFloat( 0, 360 );
	pLaser->SetLocalAngles( angles );
}

	
//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerTurret::RunTask( const Task_t *pTask )
{
/*
	switch( pTask->iTask )
	{
	default:
		BaseClass::RunTask( pTask );
		break;
	}
*/
	BaseClass::RunTask( pTask );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERTURRET_OPEN,

	"	Tasks"
	"		TASK_ROLLERTURRET_OPEN		0"
	"		TASK_WAIT					0.5"
	"	"
	"	Interrupts"
);

//=========================================================
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERTURRET_CLOSE,

	"	Tasks"
	"		TASK_ROLLERTURRET_CLOSE		0"
	"		TASK_WAIT					0.5"
	"	"
	"	Interrupts"
);

//=========================================================
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERTURRET_RANGE_ATTACK1,

	"	Tasks"
	"		TASK_RANGE_ATTACK1	0"
	"		TASK_WAIT			0.1"
	"		TASK_RANGE_ATTACK1	0"
	"		TASK_WAIT			0.1"
	"		TASK_RANGE_ATTACK1	0"
	"		TASK_WAIT			0.1"
	"		TASK_RANGE_ATTACK1	0"
	"		TASK_WAIT			0.1"
	"		TASK_RANGE_ATTACK1	0"
	"		TASK_WAIT			1.0"
	"	"
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_NEW_ENEMY"
	"		COND_ROLLER_PHYSICS"
);