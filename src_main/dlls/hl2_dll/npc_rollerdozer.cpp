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
#include "AI_Hint.h"
#include	"AI_Node.h"
#include	"AI_BaseNPC.h"
#include "AI_Navigator.h"
#include	"ndebugoverlay.h"
#include	"NPC_Roller.h"
#include "vstdlib/random.h"

#define ROLLERDOZER_DEBRIS_FREQUENCY	5

#define ROLLERDOZER_DEBRIS_RADIUS	300

#define ROLLERDOZER_FORWARD_SPEED	-900.0

//=========================================================
// Custom schedules
//=========================================================
enum
{
	SCHED_ROLLERDOZER_CLEAR_DEBRIS = LAST_ROLLER_SCHED,
	SCHED_ROLLERDOZER_IDLE_STAND,
};


//=========================================================
// Custom conditions
//=========================================================
enum
{
	COND_ROLLERDOZER_FOUND_DEBRIS = LAST_ROLLER_CONDITION,
};


//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_ROLLERDOZER_FIND_CLEANUP_NODE = LAST_ROLLER_TASK,
	TASK_ROLLERDOZER_GET_PATH_TO_CLEANUP_POINT,
	TASK_ROLLERDOZER_CLEAR_DEBRIS		
};


//=========================================================
//=========================================================
class CNPC_RollerDozer : public CNPC_Roller
{
	DECLARE_CLASS( CNPC_RollerDozer, CNPC_Roller );
	DECLARE_DATADESC();

public:

#if 0
	Class_T	Classify( void ) { return CLASS_NONE; }
#endif

	void RunTask( const Task_t *pTask );
	void StartTask( const Task_t *pTask );

	DEFINE_CUSTOM_AI;

	void Spawn( void );
	void GatherConditions( void );
	bool FValidateHintType(CAI_Hint *pHint);

	int SelectSchedule ( void );
	int TranslateSchedule( int scheduleType );

	CBaseEntity *FindDebris( void );

	void TaskFail( AI_TaskFailureCode_t code );
	void TaskFail( const char *pszGeneralFailText )	{ TaskFail( MakeFailCode( pszGeneralFailText ) ); }

	float m_flTimeDebrisSearch;

	EHANDLE m_hDebris;
	Vector m_vecCleanupPoint;
};

LINK_ENTITY_TO_CLASS( npc_rollerdozer, CNPC_RollerDozer );
IMPLEMENT_CUSTOM_AI( npc_rollerdozer, CNPC_RollerDozer );


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_RollerDozer )

	DEFINE_FIELD( CNPC_RollerDozer, m_flTimeDebrisSearch,	FIELD_TIME ),
	DEFINE_FIELD( CNPC_RollerDozer, m_hDebris,				FIELD_EHANDLE ),
	DEFINE_FIELD( CNPC_RollerDozer, m_vecCleanupPoint,		FIELD_VECTOR ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerDozer::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI( CNPC_RollerDozer );

	ADD_CUSTOM_SCHEDULE( CNPC_RollerDozer,	SCHED_ROLLERDOZER_CLEAR_DEBRIS );
	ADD_CUSTOM_SCHEDULE( CNPC_RollerDozer,	SCHED_ROLLERDOZER_IDLE_STAND );
	
	ADD_CUSTOM_CONDITION( CNPC_RollerDozer,	COND_ROLLERDOZER_FOUND_DEBRIS );

	ADD_CUSTOM_TASK( CNPC_RollerDozer, TASK_ROLLERDOZER_FIND_CLEANUP_NODE );
	ADD_CUSTOM_TASK( CNPC_RollerDozer, TASK_ROLLERDOZER_GET_PATH_TO_CLEANUP_POINT );
	ADD_CUSTOM_TASK( CNPC_RollerDozer, TASK_ROLLERDOZER_CLEAR_DEBRIS );
	
	AI_LOAD_SCHEDULE( CNPC_RollerDozer,	SCHED_ROLLERDOZER_CLEAR_DEBRIS );
	AI_LOAD_SCHEDULE( CNPC_RollerDozer,	SCHED_ROLLERDOZER_IDLE_STAND );
	
#if 0
	ADD_CUSTOM_ACTIVITY(CNPC_Roller,	ACT_MYCUSTOMACTIVITY);
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
bool CNPC_RollerDozer::FValidateHintType(CAI_Hint *pHint)
{
	return(pHint->HintType() == HINT_ROLLER_CLEANUP_POINT);
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_RollerDozer::FindDebris( void )
{
	if( !m_pHintNode )
	{
		// Detect rubbish near a hint node.
		CAI_Hint* pHintNode = CAI_Hint::FindHint( this, HINT_ROLLER_CLEANUP_POINT, 0, 1024 ); 


		if( pHintNode)
		{
			// Search around the hint node for debris that should be cleared.
			Vector vecHintNodeOrigin;
			
			// Get hint node position
			vecHintNodeOrigin;
			pHintNode->GetPosition(this,&vecHintNodeOrigin);

			CBaseEntity *pList[ 16 ];
			Vector vecDeltaUp( 200, 200, 64 );
			Vector vecDeltaDown( 200, 200, 10 );
			int i;
			IPhysicsObject	*pPhysObj;

			int count = UTIL_EntitiesInBox( pList, 16, vecHintNodeOrigin - vecDeltaDown, vecHintNodeOrigin + vecDeltaUp, 0 );
			
			float m_flHeaviestMass = 0;
			CBaseEntity *pHeaviest = NULL;
			
			for( i = 0 ; i < count ; i++ )
			{
				pPhysObj = pList[ i ]->VPhysicsGetObject();

				if( !pPhysObj || FClassnameIs( pList[ i ], "npc_rollerdozer" ) )
				{
					// Only consider physics objects. Exclude rollers.
					continue;
				}

				if( pPhysObj->GetMass() <= 400 )
				{
					if( pPhysObj->GetMass() > m_flHeaviestMass )
					{
						m_flHeaviestMass = pPhysObj->GetMass();
						pHeaviest = pList[ i ];
					}

/*
					// Report to the cleanup point and doze this piece of debris away.
					SetCondition( COND_ROLLERDOZER_FOUND_DEBRIS );
					m_vecCleanupPoint = vecHintNodeOrigin;


					return pList[ i ];
*/
				}
			}

			if( pHeaviest )
			{
				SetCondition( COND_ROLLERDOZER_FOUND_DEBRIS );
				//NDebugOverlay::Line( GetLocalOrigin(), pHeaviest->GetLocalOrigin(), 255,255,0, true, 3 );
				m_vecCleanupPoint = vecHintNodeOrigin;
				return pHeaviest;
			}
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
void CNPC_RollerDozer::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if( gpGlobals->curtime > m_flTimeDebrisSearch && m_hDebris == NULL )
	{
		m_flTimeDebrisSearch = gpGlobals->curtime + ROLLERDOZER_DEBRIS_FREQUENCY;
		m_hDebris = FindDebris();

		if( m_hDebris == NULL)
		{
			ClearCondition( COND_ROLLERDOZER_FOUND_DEBRIS );
		}
		else
		{
			SetCondition( COND_ROLLERDOZER_FOUND_DEBRIS );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
void CNPC_RollerDozer::Spawn( void )
{
	BaseClass::Spawn();

	m_flTimeDebrisSearch = gpGlobals->curtime + ( ROLLERDOZER_DEBRIS_FREQUENCY * random->RandomFloat( 0, 1 ) );
	m_hDebris = NULL;
	m_vecCleanupPoint = vec3_origin;
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
int CNPC_RollerDozer::SelectSchedule ( void )
{
	if( HasCondition( COND_ROLLERDOZER_FOUND_DEBRIS ) )
	{
		return SCHED_ROLLERDOZER_CLEAR_DEBRIS;
	}

	if( HasCondition( COND_ROLLER_PHYSICS ) )
	{
		m_hDebris = NULL;
	}
	
	return BaseClass::SelectSchedule();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
int CNPC_RollerDozer::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_IDLE_STAND:
		return SCHED_ROLLERDOZER_IDLE_STAND;
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
void CNPC_RollerDozer::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLERDOZER_GET_PATH_TO_CLEANUP_POINT:
		if ( GetNavigator()->SetGoal( m_vecCleanupPoint, AIN_CLEAR_TARGET ) )
		{
			TaskComplete();
		}
		else
		{
			// no way to get there
			TaskFail(FAIL_NO_ROUTE);
		}
		break;

	case TASK_ROLLERDOZER_CLEAR_DEBRIS:
		GetNavigator()->ClearGoal();
		m_flWaitFinished = gpGlobals->curtime + 5;
		break;

	case TASK_ROLLERDOZER_FIND_CLEANUP_NODE:
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
void CNPC_RollerDozer::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLERDOZER_CLEAR_DEBRIS:
		if( gpGlobals->curtime > m_flWaitFinished )
		{
			m_hDebris = NULL;
			m_flTimeDebrisSearch = gpGlobals->curtime;
			TaskComplete();
		}
		else if( m_hDebris != NULL )
		{
			float yaw = UTIL_VecToYaw( m_hDebris->GetLocalOrigin() - GetLocalOrigin() );
			Vector vecRight, vecForward;

			AngleVectors( QAngle( 0, yaw, 0 ), &vecForward, &vecRight, NULL );

			//Stop pushing if I'm going to push this object sideways or back towards the center of the cleanup area.
			Vector vecCleanupDir = m_hDebris->GetLocalOrigin() - m_vecCleanupPoint;
			VectorNormalize( vecCleanupDir );
			if( DotProduct( vecForward, vecCleanupDir ) < -0.5 )
			{
				// HACKHACK !!!HACKHACK - right now forcing an unstick. Do this better (sjb)

				// Clear the debris, suspend the search for debris, trick base class into unsticking me.
				m_hDebris = NULL;
				m_flTimeDebrisSearch = gpGlobals->curtime + 4;
				m_iFail = 10;
				TaskFail("Pushing Wrong Way");
			}

			m_RollerController.m_vecAngular = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, ROLLERDOZER_FORWARD_SPEED * 2 );
		}
		else
		{
			TaskFail("No debris!!");
		}

		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_RollerDozer::TaskFail( AI_TaskFailureCode_t code )
{
	m_hDebris = NULL;
	m_flTimeDebrisSearch = gpGlobals->curtime + ROLLERDOZER_DEBRIS_FREQUENCY / 2;

	BaseClass::TaskFail( code );
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
	SCHED_ROLLERDOZER_IDLE_STAND,

	"	Tasks"
	"		TASK_ROLLER_OFF		0"
	"		TASK_WAIT_INDEFINITE	0"
	"	"
	"	Interrupts"
	"		COND_ROLLERDOZER_FOUND_DEBRIS"
	"		COND_ROLLER_PHYSICS"
);

//=========================================================
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERDOZER_CLEAR_DEBRIS,

	"	Tasks"
	"		TASK_ROLLER_ON					0"
	"		TASK_ROLLERDOZER_GET_PATH_TO_CLEANUP_POINT	0"
	"		TASK_WALK_PATH					0"
	"		TASK_ROLLERDOZER_CLEAR_DEBRIS			0"
	"		"
	"	Interrupts"
	"		COND_ROLLER_PHYSICS"
);



