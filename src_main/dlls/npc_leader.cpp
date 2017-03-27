//---------------------------------------------------------
// NPCLeader.cpp
//---------------------------------------------------------

#include "cbase.h"

#if 0 // this class is unuses
#include "npc_talker.h"
#include "npc_leader.h"
#include "scripted.h"
#include "soundent.h"
#include "animation.h"
#include "EntityList.h"
#include "AI_Navigator.h"
#include "AI_Motor.h"
#include "engine/IEngineSound.h"

BEGIN_DATADESC( CNPCLeader )

	DEFINE_FIELD( CNPCLeader, m_hGoalEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPCLeader, m_hFollower, FIELD_EHANDLE ),
	DEFINE_FIELD( CNPCLeader, m_flFollowerDist, FIELD_FLOAT ),

	// Function Pointers
	DEFINE_USEFUNC( CNPCLeader, LeaderUse ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//
//-----------------------------------------------------------------------------
void CNPCLeader::LeaderUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Don't allow use during a scripted_sentence
	if ( m_useTime > gpGlobals->curtime )
		return;

	if ( IsLeading() )
	{
		Msg( "Come back when you are ready to continue!\n" );
		StopLeading( true );
	}
	else if ( pCaller != NULL && pCaller->IsPlayer() )
	{
		CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, "odell", pActivator );

		if ( pTarget )
		{
			Msg( "Follow me!\n" );
			StartLeading( pCaller, pTarget );

			//PlaySentence( "~OD_TEST", 0, 1, ATTN_NORM, pCaller );
		}
		else
		{
			Msg( "I've nothing else to show you! Blow.\n" );
			StopLeading( true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : clearSchedule - 
//
//-----------------------------------------------------------------------------
void CNPCLeader::StopLeading( bool clearSchedule )
{
	if ( IsLeading() )
	{
		Forget( bits_MEMORY_TOURGUIDE );

		m_hFollower = NULL;
		m_hGoalEnt = NULL;

		if ( clearSchedule )
			ClearSchedule();

		if ( IsMoving() && GetIdealActivity() == GetNavigator()->GetMovementActivity() )
		{
			SetIdealActivity( GetStoppedActivity() );
			GetNavigator()->ClearGoal();
		}

		if ( GetEnemy() != NULL )
		{
			m_IdealNPCState = NPC_STATE_COMBAT;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : *pFollower - 
//			*pDest - 
//
//-----------------------------------------------------------------------------
void CNPCLeader::StartLeading( CBaseEntity *pFollower, CBaseEntity *pDest )
{
	if ( GetEnemy() != NULL )
		m_IdealNPCState = NPC_STATE_ALERT;

	m_hGoalEnt = pDest;

	m_hFollower = pFollower;
	SetSpeechTarget( m_hFollower );

	Remember( bits_MEMORY_TOURGUIDE );

	ClearSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
int CNPCLeader::SelectSchedule ( void )
{
	switch( m_NPCState )
	{
	case NPC_STATE_IDLE:
	case NPC_STATE_ALERT:
		if ( IsLeading() )
		{
			if( HasInterruptCondition( COND_LEADER_FOLLOWERLOST ) )
			{
				return SCHED_LEADER_BACKTRACK;
			}

			if( HasInterruptCondition( COND_LEADER_FOLLOWERLAGGING ) )
			{
				return SCHED_LEADER_WAIT;
			}

			if( HasInterruptCondition( COND_LEADER_FOLLOWERFOUND ) )
			{
				return SCHED_LEADER_WAIT;
			}

			return SCHED_LEADER_LEAD;
		}
		else
		{
			return BaseClass::SelectSchedule();
		}
		break;
	default:
		return BaseClass::SelectSchedule();
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
#define DIST_FOLLOWERVERYNEAR	80
#define DIST_FOLLOWERNEAR		256
#define DIST_FOLLOWERFAR		400
#define DIST_FOLLOWERVERYFAR	512

void CNPCLeader::GatherConditions( void )
{
	BaseClass::GatherConditions();
	
	float flFollowerDist;

	if( IsLeading() )
	{
		flFollowerDist = (m_hFollower->GetLocalOrigin() - GetLocalOrigin()).Length();

		trace_t tr;
		UTIL_TraceLine( EyePosition(), 
			m_hFollower->EyePosition(), 
			MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0 )
		{
			// note that visible doesn't mean that the follower is in the leader's field of view,
			// only that a line from leader's eyes to follower's eyes isn't broken by the world.
			SetCondition( COND_LEADER_FOLLOWERVISIBLE );
		}
		else
		{
			ClearCondition( COND_LEADER_FOLLOWERVISIBLE );
		}

		// Clear and recalculate these each time.
		ClearCondition( COND_LEADER_FOLLOWERLOST );
		ClearCondition( COND_LEADER_FOLLOWERFOUND );
		ClearCondition( COND_LEADER_FOLLOWERLAGGING );
		ClearCondition( COND_LEADER_FOLLOWERCAUGHTUP );

		if( HasCondition( COND_LEADER_FOLLOWERVISIBLE ) )
		{
			// ***NOTE ***
			// it is important that COND_LEADER_FOLLOWERLAGGING and 
			// COND_LEADER_FOLLOWERCAUGHTUP be mutually exclusive!
			// ***********
			
			if( flFollowerDist > DIST_FOLLOWERVERYFAR )
			{
				// Follower is lost if visible, but very far away.
				SetCondition( COND_LEADER_FOLLOWERLOST );
			}
			else if( flFollowerDist > DIST_FOLLOWERFAR )
			{
				// Follower is lagging if visible, but falling far behind.
				SetCondition( COND_LEADER_FOLLOWERLAGGING );
			}
			else if ( flFollowerDist < DIST_FOLLOWERVERYNEAR )
			{
				// Follower is caught up if visible and very near.
				SetCondition( COND_LEADER_FOLLOWERCAUGHTUP );
			}
			else if ( flFollowerDist < DIST_FOLLOWERNEAR )
			{
				// Follower is found if visible and relatively near.
				SetCondition( COND_LEADER_FOLLOWERFOUND );
			}
		}
		else
		{
			// Checks made on an unseen follower. 

			// ***NOTE ***
			// it is important that COND_LEADER_FOLLOWERLOST and 
			// COND_LEADER_FOLLOWERLAGGING be mutually exclusive!
			// ***********
			if( flFollowerDist > DIST_FOLLOWERFAR )
			{
				// If the follower is unseen and far away, they are considered lost.
				SetCondition( COND_LEADER_FOLLOWERLOST );
			}
			else if( flFollowerDist > DIST_FOLLOWERNEAR )
			{
				// If the follower is unseen, but fairly near, they are considered lagging.
				SetCondition( COND_LEADER_FOLLOWERLAGGING );
			}
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : *pTask - 
//
//-----------------------------------------------------------------------------
void CNPCLeader::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_LEADER_GET_PATH_TO_GOAL:

		if (m_hGoalEnt == NULL)
		{
			TaskFail(FAIL_NO_GOAL);
		}
		else 
		{
			if ( GetNavigator()->SetGoal( m_hGoalEnt->GetLocalOrigin()) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				DevWarning( 2, "LeaderGetPathToGoal failed!!\n" );
				TaskFail(FAIL_NO_ROUTE);
			}
		}
		break;

	case TASK_LEADER_GET_PATH_TO_FOLLOWER:

		if (m_hGoalEnt == NULL)
		{
			TaskFail(FAIL_NO_GOAL);
		}
		else 
		{
			if ( GetNavigator()->SetGoal( m_hFollower->GetLocalOrigin() ) )
			{
				if( GetExpresser()->HasMonolog() && ShouldResumeMonolog() )
				{
					GetExpresser()->ResumeMonolog();
				}

				TaskComplete();
			}
			else
			{
				// no way to get there =(
				DevWarning( 2, "LeaderGetPathToFollower failed!!\n" );
				TaskFail(FAIL_NO_ROUTE);
			}
		}
		break;

	case TASK_LEADER_WALK_PATH_TO_FOLLOWER:
		if ( NPC_TranslateActivity( ACT_WALK ) != ACT_INVALID )
		{
			GetNavigator()->SetMovementActivity(ACT_WALK);
		}
		break;

	case TASK_LEADER_FACE_FOLLOWER:
		GetMotor()->SetIdealYawToTarget( m_hFollower->GetLocalOrigin() );
		SetTurnActivity(); 
		break;

	case TASK_LEADER_ANNOUNCE_END_TOUR:
	case TASK_LEADER_ANNOUNCE_BACKTRACK:
	case TASK_LEADER_REMIND_FOLLOWER:
		// No start task logic for these.
		break;

	case TASK_LEADER_STOP_LEADING:
		StopLeading( false );
		TaskComplete();
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : *pTask - 
//
//-----------------------------------------------------------------------------
void CNPCLeader::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_LEADER_WALK_PATH_TO_FOLLOWER:
		if ( HasCondition( COND_LEADER_FOLLOWERVISIBLE ) && HasCondition( COND_LEADER_FOLLOWERNEAR ) )
		{
			TaskComplete();
			GetNavigator()->ClearGoal();
		}
		else if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
		{
			TaskComplete();
			GetNavigator()->ClearGoal();
		}
		break;

	case TASK_LEADER_FACE_FOLLOWER:
		GetMotor()->SetIdealYawToTargetAndUpdate( m_hFollower->GetLocalOrigin(), AI_KEEP_YAW_SPEED );

		if ( FacingIdeal() )
		{
			TaskComplete();
		}
		break;

	case TASK_LEADER_ANNOUNCE_END_TOUR:
		if( IsOkToSpeak() )
		{
			Speak( TLK_TGENDTOUR );
			TaskComplete();
		}
		break;

	case TASK_LEADER_ANNOUNCE_BACKTRACK:
		if( IsOkToSpeak() )
		{
			Speak( TLK_TGSTAYPUT );
			TaskComplete();
		}
		break;

	case TASK_LEADER_REMIND_FOLLOWER:
		if( IsOkToSpeak() )
		{
			// wait until it's ok to speak, then remind the player to come on.
			Speak( TLK_TGCATCHUP );
			TaskComplete();
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether or not the follower is on the leader's left side
//
//
// Output : Returns true if yes, false if no.
//-----------------------------------------------------------------------------
bool CNPCLeader::FollowerOnLeft( void )
{
	Vector vecRight;
	Vector vecDirToFollower;

	AngleVectors( GetLocalAngles(), NULL, &vecRight, NULL );
	vecDirToFollower = m_hFollower->GetLocalOrigin() - GetLocalOrigin();
	VectorNormalize(vecDirToFollower);

	// make this planar. 
	vecDirToFollower.z = 0;
	vecRight.z = 0;

	if ( DotProduct ( vecDirToFollower, vecRight ) < 0 )
	{
		// follower is on the left side.
		return true;
	}
	else
	{
		// follower is on the right side.
		return false;
	}
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------


AI_BEGIN_CUSTOM_NPC(lead_monster, CNPCLeader)

	DECLARE_TASK(TASK_LEADER_GET_PATH_TO_GOAL)
	DECLARE_TASK(TASK_LEADER_GET_PATH_TO_FOLLOWER)
	DECLARE_TASK(TASK_LEADER_WALK_PATH_TO_FOLLOWER)
	DECLARE_TASK(TASK_LEADER_FACE_FOLLOWER)
	DECLARE_TASK(TASK_LEADER_WAIT_FOLLOWER_NEAR)
	DECLARE_TASK(TASK_LEADER_ANNOUNCE_BACKTRACK)
	DECLARE_TASK(TASK_LEADER_ANNOUNCE_END_TOUR)
	DECLARE_TASK(TASK_LEADER_REMIND_FOLLOWER)
	DECLARE_TASK(TASK_LEADER_STOP_LEADING)

	DECLARE_CONDITION(COND_LEADER_FOLLOWERNEAR)
	DECLARE_CONDITION(COND_LEADER_FOLLOWERVERYFAR)
	DECLARE_CONDITION(COND_LEADER_FOLLOWERVISIBLE)
	DECLARE_CONDITION(COND_LEADER_FOLLOWERLAGGING)
	DECLARE_CONDITION(COND_LEADER_FOLLOWERLOST)
	DECLARE_CONDITION(COND_LEADER_FOLLOWERFOUND)
	DECLARE_CONDITION(COND_LEADER_FOLLOWERCAUGHTUP)

	//=========================================================
	// > SCHED_LEADER_LEAD
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_LEADER_LEAD,

		"	Tasks"
		"		TASK_LEADER_GET_PATH_TO_GOAL		0"
		"		TASK_FACE_PATH						0"
		"		TASK_WALK_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_STOP_MOVING					0"
		"		TASK_LEADER_STOP_LEADING			0"
		"		TASK_LEADER_ANNOUNCE_END_TOUR		0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LEADER_FOLLOWERLAGGING"
		"		COND_LEADER_FOLLOWERLOST"
	)


	//=========================================================
	// > SCHED_LEADER_BACKTRACK
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_LEADER_BACKTRACK,


		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_LEADER_ANNOUNCE_BACKTRACK			0"
		"		TASK_LEADER_GET_PATH_TO_FOLLOWER		0"
		"		TASK_LEADER_WALK_PATH_TO_FOLLOWER		0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LEADER_FOLLOWERFOUND"
		"		COND_LEADER_FOLLOWERCAUGHTUP"
	)

	//=========================================================
	// > SCHED_LEADER_GET_CLOSER
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_LEADER_GET_CLOSER,

		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_LEADER_GET_PATH_TO_FOLLOWER		0"
		"		TASK_LEADER_WALK_PATH_TO_FOLLOWER		0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LEADER_FOLLOWERFOUND"
		"		COND_LEADER_FOLLOWERCAUGHTUP"
	)

	//=========================================================
	// > SCHED_LEADER_WAIT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_LEADER_WAIT,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_LEADER_FACE_FOLLOWER		0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_LEADER_REMIND_FOLLOWER		0"
		"		TASK_WAIT						5"
		"		TASK_WAIT_RANDOM				5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY "
		"		COND_LEADER_FOLLOWERCAUGHTUP "
	)
	
AI_END_CUSTOM_NPC()
#endif