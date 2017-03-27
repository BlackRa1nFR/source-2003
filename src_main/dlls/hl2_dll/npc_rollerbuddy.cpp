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
#include "AI_Default.h"
#include "AI_Task.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Node.h"
#include "AI_BaseNPC.h"
#include "AI_Navigator.h"
#include "ndebugoverlay.h"
#include "NPC_Roller.h"
#include "entitylist.h"
#include "player.h"
#include "npc_rollerbuddy.h"
#include "engine/IEngineSound.h"


//=========================================================
// Custom schedules
//=========================================================
enum
{
	ROLLERBUDDY_MODE_FOLLOW = 0,
	ROLLERBUDDY_MODE_COMMAND,
};


//=========================================================
// Custom schedules
//=========================================================
enum
{
	SCHED_ROLLERBUDDY_IDLE_STAND = LAST_ROLLER_SCHED,
	SCHED_ROLLERBUDDY_MOVE_TO_MASTER,
	SCHED_ROLLERBUDDY_MOVE_TO_LOCATION,
};

//=========================================================
// Custom conditions
//=========================================================
enum
{
	COND_ROLLERBUDDY_APPROACH_MASTER = LAST_ROLLER_CONDITION,
	COND_ROLLERBUDDY_COMMAND_MOVE,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_ROLLERBUDDY_GET_PATH_TO_MASTER = LAST_ROLLER_TASK,
	TASK_ROLLERBUDDY_GET_PATH_TO_LOCATION,
	TASK_ROLLERBUDDY_ACTIVATE_COMMAND_ENTITY,
};


LINK_ENTITY_TO_CLASS( npc_rollerbuddy, CNPC_RollerBuddy );

IMPLEMENT_CUSTOM_AI( npc_rollerbuddy, CNPC_RollerBuddy );


#define ROLLERBUDDY_SOUND_MOVE			"npc/roller/roll.wav"

static const char *pCodeSounds[] =
{
	"npc/roller/code1.wav",
	"npc/roller/code2.wav",
	"npc/roller/code3.wav",
	"npc/roller/code4.wav",
	"npc/roller/code5.wav",
	"npc/roller/code6.wav",
};



//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_RollerBuddy )

	DEFINE_FIELD( CNPC_RollerBuddy, m_iMode,				FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_RollerBuddy, m_vecCommandLocation,	FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_RollerBuddy, m_hCommandEntity,		FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_RollerBuddy, m_pMaster,				FIELD_CLASSPTR ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBuddy::Spawn( void )
{
	BaseClass::Spawn();

	m_flForwardSpeed = -2000;
	m_pMaster = NULL;

	m_iMode = ROLLERBUDDY_MODE_COMMAND;

	// Get the rotor sound started up.
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CPASAttenuationFilter filter( this );
	m_pRollSound = controller.SoundCreate( filter, entindex(), CHAN_STATIC, ROLLERBUDDY_SOUND_MOVE, ATTN_NORM );
}


//-----------------------------------------------------------------------------
// Purpose: Find the player in the world and make him my master. This is
//			usually done only once, following spawn.
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBuddy::FindMaster( void )
{
	CBasePlayer *pPlayer; 
	
	pPlayer = (CBasePlayer *)gEntList.FindEntityByClassname( NULL, "player" );	

	if( pPlayer )
	{
		m_pMaster = pPlayer;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_RollerBuddy::SelectSchedule ( void )
{
	if( HasCondition( COND_ROLLERBUDDY_APPROACH_MASTER ) )
	{
		return SCHED_ROLLERBUDDY_MOVE_TO_MASTER;
	}

	if( HasCondition( COND_ROLLERBUDDY_COMMAND_MOVE ) )
	{
		ClearCondition( COND_ROLLERBUDDY_COMMAND_MOVE );
		return SCHED_ROLLERBUDDY_MOVE_TO_LOCATION;
	}

	return SCHED_IDLE_STAND;

	return BaseClass::SelectSchedule();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
int CNPC_RollerBuddy::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_IDLE_STAND:
		return SCHED_ROLLERBUDDY_IDLE_STAND;
		break;
	}
	
	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBuddy::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if( m_pMaster )
	{
		if( m_iMode == ROLLERBUDDY_MODE_FOLLOW )
		{
			float flDistance;

			flDistance = UTIL_DistApprox( GetLocalOrigin(), m_pMaster->GetLocalOrigin() );

			if( !FVisible( m_pMaster ) || flDistance > 256 )
			{
				SetCondition( COND_ROLLERBUDDY_APPROACH_MASTER );
			}
			else
			{
				ClearCondition( COND_ROLLERBUDDY_APPROACH_MASTER );
			}
		}
		else
		{
			// Don't care if we can see the player if we are not 
			// in follow mode. The player may have moved us out of sight
			// and expects us to stay until called.
			ClearCondition( COND_ROLLERBUDDY_APPROACH_MASTER );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBuddy::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	if( !m_pMaster )
	{
		FindMaster();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBuddy::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI( CNPC_RollerBuddy );

	ADD_CUSTOM_SCHEDULE( CNPC_RollerBuddy, SCHED_ROLLERBUDDY_IDLE_STAND );
	ADD_CUSTOM_SCHEDULE( CNPC_RollerBuddy, SCHED_ROLLERBUDDY_MOVE_TO_MASTER );
	ADD_CUSTOM_SCHEDULE( CNPC_RollerBuddy, SCHED_ROLLERBUDDY_MOVE_TO_LOCATION );

	ADD_CUSTOM_TASK( CNPC_RollerBuddy, TASK_ROLLERBUDDY_GET_PATH_TO_MASTER );
	ADD_CUSTOM_TASK( CNPC_RollerBuddy, TASK_ROLLERBUDDY_GET_PATH_TO_LOCATION );
	ADD_CUSTOM_TASK( CNPC_RollerBuddy, TASK_ROLLERBUDDY_ACTIVATE_COMMAND_ENTITY );

	ADD_CUSTOM_CONDITION( CNPC_RollerBuddy,	COND_ROLLERBUDDY_APPROACH_MASTER );
	ADD_CUSTOM_CONDITION( CNPC_RollerBuddy,	COND_ROLLERBUDDY_COMMAND_MOVE );

	AI_LOAD_SCHEDULE( CNPC_RollerBuddy, SCHED_ROLLERBUDDY_IDLE_STAND );
	AI_LOAD_SCHEDULE( CNPC_RollerBuddy, SCHED_ROLLERBUDDY_MOVE_TO_MASTER );
	AI_LOAD_SCHEDULE( CNPC_RollerBuddy, SCHED_ROLLERBUDDY_MOVE_TO_LOCATION );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBuddy::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLERBUDDY_ACTIVATE_COMMAND_ENTITY:
		
		m_iCodeProgress = 0;
		m_flWaitFinished = gpGlobals->curtime + 1.0;

		if( m_hCommandEntity == NULL || FClassnameIs( m_hCommandEntity, "worldspawn" ) )
		{
			TaskComplete();
		}
		break;

	case TASK_ROLLERBUDDY_GET_PATH_TO_LOCATION:
		{
			if ( GetNavigator()->SetGoal( m_vecCommandLocation, AIN_CLEAR_TARGET ) )
			{
				EmitSound( "NPC_RollerBuddy.Confirm" );

				// Make a sound from remote, too.
				CPASAttenuationFilter filternoatten( this, "NPC_RollerBuddy.RemoteYes" );
				EmitSound( filternoatten, entindex(), "NPC_RollerBuddy.RemoteYes" );

				TaskComplete();
			}
			else
			{
				// no way to get there
				// Make a sound from remote, too.
				CPASAttenuationFilter filternoatten( this, "NPC_RollerBuddy.RemoteNo" );
				EmitSound( filternoatten, entindex(), "NPC_RollerBuddy.RemoteNo" );
				TaskFail(FAIL_NO_ROUTE);
			}
		}
		break;

	case TASK_ROLLERBUDDY_GET_PATH_TO_MASTER:
		{
			if ( GetNavigator()->SetGoal( m_pMaster->GetLocalOrigin(), AIN_CLEAR_TARGET ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there
				TaskFail(FAIL_NO_ROUTE);
			}
		}
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
void CNPC_RollerBuddy::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLERBUDDY_ACTIVATE_COMMAND_ENTITY:
		if( gpGlobals->curtime >= m_flWaitFinished )
		{
			if( m_iCodeProgress == ROLLER_CODE_DIGITS )
			{
				m_hCommandEntity->Use( this, this, USE_TOGGLE, 0 );
				TaskComplete();
			}
			else
			{
				if( m_iCodeProgress == ROLLER_CODE_DIGITS - 1 )
				{
					m_flWaitFinished = gpGlobals->curtime + 1;
				}
				else
				{
					m_flWaitFinished = gpGlobals->curtime + ROLLER_TONE_TIME;
				}

				CPASAttenuationFilter filter( this );
				EmitSound( filter, entindex(), CHAN_BODY, pCodeSounds[ m_iAccessCode[ m_iCodeProgress ] ], 1.0, ATTN_NORM );
				m_iCodeProgress++;
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBuddy::Precache( void )
{
	BaseClass::Precache();

	PRECACHE_SOUND_ARRAY( pCodeSounds );

	enginesound->PrecacheSound( ROLLERBUDDY_SOUND_MOVE );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBuddy::ToggleBuddyMode( bool fAcknowledge )
{
	if( m_iMode == ROLLERBUDDY_MODE_FOLLOW )
	{
		m_iMode = ROLLERBUDDY_MODE_COMMAND;
	}
	else
	{
		m_iMode = ROLLERBUDDY_MODE_FOLLOW;
	}

	// Remote should say yes
	if( fAcknowledge )
	{
		// No attenuation, since this is supposed to come from the remote.
		EmitSound( "NPC_RollerBuddy.RemoteYes" );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBuddy::CommandMoveToLocation( const Vector &vecLocation, CBaseEntity *pCommandEntity )
{
	m_vecCommandLocation = vecLocation;
	m_hCommandEntity = pCommandEntity;

	Msg( "%s\n", pCommandEntity->GetClassname() );

	SetCondition( COND_ROLLERBUDDY_COMMAND_MOVE );
}


void CNPC_RollerBuddy::PowerOn( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.Play( m_pRollSound, 1.0, 100 );

	BaseClass::PowerOn();
}


void CNPC_RollerBuddy::PowerOff( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundFadeOut( m_pRollSound, 0.5 );

	BaseClass::PowerOff();
}



//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//==================================================
//==================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERBUDDY_IDLE_STAND,

	"	Tasks"
	"		TASK_ROLLER_OFF					0"
	"		TASK_WAIT_INDEFINITE			0"
	"	"
	"	Interrupts"
	"		COND_ROLLER_PHYSICS"
	"		COND_ROLLERBUDDY_APPROACH_MASTER"
	"		COND_ROLLERBUDDY_COMMAND_MOVE"
);

//==================================================
//==================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERBUDDY_MOVE_TO_MASTER,

	"	Tasks"
	"		TASK_ROLLERBUDDY_GET_PATH_TO_MASTER		0"
	"		TASK_ROLLER_ON							0"
	"		TASK_WALK_PATH							0"
	"	"
	"	Interrupts"
	"		COND_ROLLER_PHYSICS"
	"		COND_ROLLERBUDDY_COMMAND_MOVE"
);

//==================================================
//==================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERBUDDY_MOVE_TO_LOCATION,

	"	Tasks"
	"		TASK_ROLLERBUDDY_GET_PATH_TO_LOCATION		0"
	"		TASK_ROLLER_ON								0"
	"		TASK_WALK_PATH								0"
	"		TASK_ROLLER_OFF								0"
	"		TASK_ROLLERBUDDY_ACTIVATE_COMMAND_ENTITY	0"
	"	"
	"	Interrupts"
	"		COND_ROLLER_PHYSICS"
	"		COND_ROLLERBUDDY_COMMAND_MOVE"
);