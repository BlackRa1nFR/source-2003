//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Functions and data pertaining to the NPCs' AI scheduling system.
//			Implements default NPC tasks and schedules.
//
//=============================================================================

#include "cbase.h"
#include "ai_default.h"
#include "animation.h"
#include "scripted.h"
#include "soundent.h"
#include "entitylist.h"
#include "basecombatweapon.h"
#include "bitstring.h"
#include "ai_task.h"
#include "ai_network.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_motor.h"
#include "ai_hint.h"
#include "ai_memory.h"
#include "ai_navigator.h"
#include "ai_tacticalservices.h"
#include "game.h"
#include "IEffects.h"
#include "vstdlib/random.h"

#define MAX_TASKS_RUN 10

struct TaskTimings
{
	const char *pszTask;
	CFastTimer selectSchedule;
	CFastTimer startTimer;
	CFastTimer runTimer;
};

TaskTimings g_AITaskTimings[MAX_TASKS_RUN];
int			g_nAITasksRun;

void CAI_BaseNPC::DumpTaskTimings()
{
	Msg(" Tasks timings:\n" );
	for ( int i = 0; i < g_nAITasksRun; ++i )
	{
		Msg( "   %32s -- select %5.2f, start %5.2f, run %5.2f\n", g_AITaskTimings[i].pszTask,
			 g_AITaskTimings[i].selectSchedule.GetDuration().GetMillisecondsF(),
			 g_AITaskTimings[i].startTimer.GetDuration().GetMillisecondsF(),
			 g_AITaskTimings[i].runTimer.GetDuration().GetMillisecondsF() );
			
	}

}

//=========================================================
// FHaveSchedule - Returns true if NPC's GetCurSchedule()
// is anything other than NULL.
//=========================================================
bool CAI_BaseNPC::FHaveSchedule( void )
{
	if ( GetCurSchedule() == NULL )
	{
		return false;
	}

	return true;
}

//=========================================================
// ClearSchedule - blanks out the caller's schedule pointer
// and index.
//=========================================================
void CAI_BaseNPC::ClearSchedule( void )
{
	m_ScheduleState.timeCurTaskStarted = m_ScheduleState.timeStarted = 0;
	SetTaskStatus( TASKSTATUS_NEW );
	m_pSchedule =  NULL;
	ResetScheduleCurTaskIndex();
	m_InverseIgnoreConditions.SetAllBits();
}

//=========================================================
// FScheduleDone - Returns true if the caller is on the
// last task in the schedule
//=========================================================
bool CAI_BaseNPC::FScheduleDone ( void )
{
	Assert( GetCurSchedule() != NULL );
	
	if ( GetScheduleCurTaskIndex() == GetCurSchedule()->NumTasks() )
	{
		return true;
	}

	return false;
}

//=========================================================
// SetSchedule - replaces the NPC's schedule pointer
// with the passed pointer, and sets the ScheduleIndex back
// to 0
//=========================================================
void CAI_BaseNPC::SetSchedule ( CAI_Schedule *pNewSchedule )
{
	Assert( pNewSchedule != NULL );
	
	m_ScheduleState.timeCurTaskStarted = m_ScheduleState.timeStarted = gpGlobals->curtime;
	
	m_pSchedule = pNewSchedule ;
	ResetScheduleCurTaskIndex();
	SetTaskStatus( TASKSTATUS_NEW );
	m_failSchedule = SCHED_NONE;
	m_Conditions.ClearAllBits();
	m_bConditionsGathered = false;
	GetNavigator()->ClearGoal();
	m_InverseIgnoreConditions.SetAllBits();

/*
#if _DEBUG
	if ( !ScheduleFromName( pNewSchedule->GetName() ) )
	{
		Msg( "Schedule %s not in table!!!\n", pNewSchedule->GetName() );
	}
#endif
*/	
// this is very useful code if you can isolate a test case in a level with a single NPC. It will notify
// you of every schedule selection the NPC makes.

	if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
	{
		Msg(this, AIMF_IGNORE_SELECTED, "Schedule: %s\n", pNewSchedule->GetName() );
	}

#if 0
	if ( FClassnameIs( this, "npc_barney" ) )
	{
		const Task_t *pTask = GetTask();
		
		if ( pTask )
		{
			const char *pName = NULL;

			if ( GetCurSchedule() )
			{
				pName = GetCurSchedule()->GetName();
			}
			else
			{
				pName = "No Schedule";
			}
			
			if ( !pName )
			{
				pName = "Unknown";
			}

			DevMsg( 2, "%s: picked schedule %s\n", GetClassname(), pName );
		}
	}
#endif// 0

}

//=========================================================
// NextScheduledTask - increments the ScheduleIndex
//=========================================================
void CAI_BaseNPC::NextScheduledTask ( void )
{
	Assert( GetCurSchedule() != NULL );

	SetTaskStatus( TASKSTATUS_NEW );
	IncScheduleCurTaskIndex();

	if ( FScheduleDone() )
	{
		// Reset memory of failed schedule 
		m_failedSchedule   = NULL;
		m_interuptSchedule = NULL;

		// just completed last task in schedule, so make it invalid by clearing it.
		SetCondition( COND_SCHEDULE_DONE );
	}
}


//-----------------------------------------------------------------------------
// Purpose: This function allows NPCs to modify the interrupt mask for the
//			current schedule. This enables them to use base schedules but with
//			different interrupt conditions. Implement this function in your
//			derived class, and Set or Clear condition bits as you please.
//
//			NOTE: Always call the base class in your implementation, but be
//				  aware of the difference between changing the bits before vs.
//				  changing the bits after calling the base implementation.
//
// Input  : pBitString - Receives the updated interrupt mask.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::BuildScheduleTestBits( void )
{
	//NOTENOTE: Always defined in the leaf classes
}


//=========================================================
// IsScheduleValid - returns true as long as the current
// schedule is still the proper schedule to be executing,
// taking into account all conditions
//=========================================================
bool CAI_BaseNPC::IsScheduleValid ( void )
{
	if ( GetCurSchedule() == NULL || GetCurSchedule()->NumTasks() == 0 )
	{
		// schedule is empty, and therefore not valid.
		return false;
	}

	//Start out with the base schedule's set interrupt conditions
	GetCurSchedule()->GetInterruptMask( &m_CustomInterruptConditions );

	//Let the leaf class modify our interrupt test bits
	BuildScheduleTestBits();

	//Any conditions set here will always be forced on the interrupt conditions
	SetCustomInterruptCondition( COND_NPC_FREEZE );

	// This is like: m_CustomInterruptConditions &= m_Conditions;
	CAI_ScheduleBits testBits;
	m_CustomInterruptConditions.And( m_Conditions, &testBits  );

	if (!testBits.IsAllClear()) 
	{
		// If in developer mode save the interrupt text for debug output
		if (g_pDeveloper->GetInt()) 
		{
			// Reset memory of failed schedule 
			m_failedSchedule   = NULL;
			m_interuptSchedule = GetCurSchedule();

			// Find the first non-zero bit
			for (int i=0;i<MAX_CONDITIONS;i++)
			{
				if (testBits.GetBit(i))
				{
					m_interruptText = ConditionName( AI_RemapToGlobal( i ) );
					if (!m_interruptText)
					{
						static const char *pError = "ERROR: Unknown condition!";
						Msg("%s\n", pError);
						m_interruptText = pError;
					}

					if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
					{
						Msg( this, AIMF_IGNORE_SELECTED, "      Break condition -> %s\n", m_interruptText );
					}

					break;
				}
			}
		}

		return false;
	}

	if ( HasCondition(COND_SCHEDULE_DONE) || 
		 HasCondition(COND_TASK_FAILED)   )
	{
#ifdef DEBUG
		if ( HasCondition ( COND_TASK_FAILED ) && m_failSchedule == SCHED_NONE )
		{
			// fail! Send a visual indicator.
			DevWarning( 2, "Schedule: %s Failed\n", GetCurSchedule()->GetName() );

			Vector tmp = GetLocalOrigin();
			tmp.z = GetAbsMaxs().z + 16;
			g_pEffects->Sparks( tmp );
		}
#endif // DEBUG

		// some condition has interrupted the schedule, or the schedule is done
		return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether or not SelectIdealState() should be called before
//			a NPC selects a new schedule. 
//
//			NOTE: This logic was a source of pure, distilled trouble in Half-Life.
//			If you change this function, please supply good comments.
//
// Output : Returns true if yes, false if no
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ShouldSelectIdealState( void )
{
/*

	HERE's the old Half-Life code that used to control this.

	if ( m_IdealNPCState != NPC_STATE_DEAD && 
		 (m_IdealNPCState != NPC_STATE_SCRIPT || m_IdealNPCState == m_NPCState) )
	{
		if (	(m_afConditions && !HasConditions(bits_COND_SCHEDULE_DONE)) ||
				(GetCurSchedule() && (GetCurSchedule()->iInterruptMask & bits_COND_SCHEDULE_DONE)) ||
				((m_NPCState == NPC_STATE_COMBAT) && (GetEnemy() == NULL))	)
		{
			GetIdealState();
		}
	}
*/
	
	if ( m_IdealNPCState == NPC_STATE_DEAD )
	{
		// Don't get ideal state if you are supposed to be dead.
		return false;
	}

	if ( (m_IdealNPCState == NPC_STATE_SCRIPT) && (m_NPCState != NPC_STATE_SCRIPT) )
	{
		// If I'm supposed to be in scripted state, but i'm not yet, do not allow 
		// SelectIdealState() to be called, because it doesn't know how to determine 
		// that a NPC should be in SCRIPT state and will stomp it with some other 
		// state. (Most likely ALERT)
		return false;
	}

	if ( !HasCondition(COND_SCHEDULE_DONE) )
	{
		// If the NPC has any current conditions, and one of those conditions indicates
		// that the previous schedule completed successfully, then don't run SelectIdealState(). 
		// Paths between states only exist for interrupted schedules, or when a schedule 
		// contains a task that suggests that the NPC change state.
		return true;
	}

	if ( GetCurSchedule() && GetCurSchedule()->HasInterrupt(COND_SCHEDULE_DONE) )
	{
		// This seems like some sort of hack...
		// Currently no schedule that I can see in the AI uses this feature, but if a schedule
		// interrupt mask contains bits_COND_SCHEDULE_DONE, then force a call to SelectIdealState().
		// If we want to keep this feature, I suggest we create a new condition with a name that
		// indicates exactly what it does. 
		return true;
	}

	if ( (m_NPCState == NPC_STATE_COMBAT) && (GetEnemy() == NULL) )
	{
		// Don't call SelectIdealState if a NPC in combat state has a valid enemy handle. Otherwise,
		// we need to change state immediately because something unexpected happened to the enemy 
		// entity (it was blown apart by someone else, for example), and we need the NPC to change
		// state. THE REST OF OUR CODE should be robust enough that this can go away!!
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a new schedule based on current condition bits.
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_BaseNPC::GetNewSchedule( void )
{
	int scheduleType;

	//
	// Schedule selection code here overrides all leaf schedule selection.
	//
	if (HasCondition(COND_NPC_FREEZE))
	{
		scheduleType = SCHED_NPC_FREEZE;
	}
	else
	{

		if ( !m_bConditionsGathered ) // occurs if a schedule is exhausted within one think
			GatherConditions();

		scheduleType = SelectSchedule();
	}

	return GetScheduleOfType( scheduleType );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_BaseNPC::GetFailSchedule( void )
{
	int prevSchedule;
	int failedTask;

	if ( GetCurSchedule() )
		prevSchedule = GetLocalScheduleId( GetCurSchedule()->GetId() );
	else
		prevSchedule = SCHED_NONE;
		
	const Task_t *pTask = GetTask();
	if ( pTask )
		failedTask = pTask->iTask;
	else
		failedTask = TASK_INVALID;

	Assert( AI_IdIsLocal( prevSchedule ) );
	Assert( AI_IdIsLocal( failedTask ) );

	int scheduleType = SelectFailSchedule( prevSchedule, failedTask, m_ScheduleState.taskFailureCode );
	return GetScheduleOfType( scheduleType );
}


//=========================================================
// MaintainSchedule - does all the per-think schedule maintenance.
// ensures that the NPC leaves this function with a valid
// schedule!
//=========================================================
void CAI_BaseNPC::MaintainSchedule ( void )
{
	AI_PROFILE_SCOPE(CAI_BaseNPC_RunAI_MaintainSchedule);
	extern CFastTimer g_AIMaintainScheduleTimer;
	CTimeScope timeScope(&g_AIMaintainScheduleTimer);

	//---------------------------------

	CAI_Schedule	*pNewSchedule;
	int			i;
	bool		runTask = true;

	memset( g_AITaskTimings, sizeof(g_AITaskTimings), 0 );
	
	g_nAITasksRun = 0;
	
	static bool 	   fInitializedTimeLimit;
	static CCycleCount timeLimit;
	
	if ( !fInitializedTimeLimit )
	{
		timeLimit.Init( 8 ); //  leave time for executing move/animation
		fInitializedTimeLimit = true;
	}
	
	CFastTimer taskTime;
	taskTime.Start();

	// UNDONE: Tune/fix this MAX_TASKS_RUN... This is just here so infinite loops are impossible
	for ( i = 0; i < MAX_TASKS_RUN; i++ )
	{
		if ( GetCurSchedule() != NULL && TaskIsComplete() )
		{
			// Schedule is valid, so advance to the next task if the current is complete.
			NextScheduledTask();

			// --------------------------------------------------------
			//	If debug stepping advance when I complete a task
			// --------------------------------------------------------
			if (CAI_BaseNPC::m_nDebugBits & bits_debugStepAI)
			{
				m_nDebugCurIndex++;
				return;
			}
		}
		
		int curTiming = g_nAITasksRun;
		g_nAITasksRun++;

		// validate existing schedule 
		if ( !IsScheduleValid() || m_NPCState != m_IdealNPCState )
		{
			// Notify the NPC that his schedule is changing
			OnScheduleChange();

			if ( ShouldSelectIdealState() )
			{
				SelectIdealState();
			}

			if ( HasCondition( COND_TASK_FAILED ) && m_NPCState == m_IdealNPCState )
			{
				// Get a fail schedule if the previous schedule failed during execution and 
				// the NPC is still in its ideal state. Otherwise, the NPC would immediately
				// select the same schedule again and fail again.
				if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
				{
					Msg( this, AIMF_IGNORE_SELECTED, "      (failed)\n" );
				}
				pNewSchedule = GetFailSchedule();
				DevWarning( 2, "Schedule (%s) Failed at %d!\n", GetCurSchedule() ? GetCurSchedule()->GetName() : "GetCurSchedule() == NULL", GetScheduleCurTaskIndex() );
				SetSchedule( pNewSchedule );
			}
			else
			{
				// If the NPC is supposed to change state, it doesn't matter if the previous
				// schedule failed or completed. Changing state means selecting an entirely new schedule.
				SetState( m_IdealNPCState );
				
				g_AITaskTimings[curTiming].selectSchedule.Start();
				if ( m_NPCState == NPC_STATE_SCRIPT || m_NPCState == NPC_STATE_DEAD )
				{
					pNewSchedule = CAI_BaseNPC::GetNewSchedule();
				}
				else
				{
					pNewSchedule = GetNewSchedule();
				}

				g_AITaskTimings[curTiming].selectSchedule.End();

				SetSchedule( pNewSchedule );
			}
		}

		if (!GetCurSchedule())
		{
			g_AITaskTimings[curTiming].selectSchedule.Start();
			
			pNewSchedule = GetNewSchedule();
			
			g_AITaskTimings[curTiming].selectSchedule.End();

			if (pNewSchedule)
			{
				SetSchedule( pNewSchedule );
			}
		}

		if ( !GetCurSchedule() || GetCurSchedule()->NumTasks() == 0 )
		{
			Msg("ERROR: Missing or invalid schedule!\n");
			SetActivity ( ACT_IDLE );
			return;
		}
		
		if ( GetTaskStatus() == TASKSTATUS_NEW )
		{	
			g_AITaskTimings[curTiming].startTimer.Start();
			const Task_t *pTask = GetTask();
			Assert( pTask != NULL );
			g_AITaskTimings[i].pszTask = TaskName( pTask->iTask );

			if (m_debugOverlays & OVERLAY_TASK_TEXT_BIT)
			{
				Msg(this, AIMF_IGNORE_SELECTED, "  Task: %s\n", TaskName(pTask->iTask) );
			}

			OnStartTask();
			
			m_ScheduleState.taskFailureCode    = NO_TASK_FAILURE;
			m_ScheduleState.timeCurTaskStarted = gpGlobals->curtime;
			
			StartTask( pTask );

			if ( TaskIsRunning() && !HasCondition(COND_TASK_FAILED) )
				StartTaskOverlay();

			g_AITaskTimings[curTiming].startTimer.End();
			// Msg( "%.2f StartTask( %s )\n", gpGlobals->curtime, m_pTaskSR->GetStringText( pTask->iTask ) );
		}

		// UNDONE: Twice?!!!
		MaintainActivity();
		
		if ( !TaskIsComplete() && GetTaskStatus() != TASKSTATUS_NEW )
		{
			if ( TaskIsRunning() && !HasCondition(COND_TASK_FAILED) && runTask )
			{
				const Task_t *pTask = GetTask();
				Assert( pTask != NULL );
				g_AITaskTimings[i].pszTask = TaskName( pTask->iTask );
				// Msg( "%.2f RunTask( %s )\n", gpGlobals->curtime, m_pTaskSR->GetStringText( pTask->iTask ) );
				g_AITaskTimings[curTiming].runTimer.Start();

				RunTask( pTask );
				if ( TaskIsRunning() && !HasCondition(COND_TASK_FAILED) )
					RunTaskOverlay();

				g_AITaskTimings[curTiming].runTimer.End();

				// don't do this again this frame
				// FIXME: RunTask() should eat some of the clock, depending on what it has done
				// runTask = false;

				if ( !TaskIsComplete() )
					break;
			}
			else
			{
				break;
			}

		}
		
		if ( timeLimit.IsLessThan( taskTime.GetDurationInProgress() ) )
			break;
	}

	// UNDONE: We have to do this so that we have an animation set to blend to if RunTask changes the animation
	// RunTask() will always change animations at the end of a script!
	// Don't do this twice
	MaintainActivity();

	// --------------------------------------------------------
	//	If I'm stopping to debug step, don't animate unless
	//  I'm in motion
	// --------------------------------------------------------
	if (CAI_BaseNPC::m_nDebugBits & bits_debugStepAI)
	{
		if (!GetNavigator()->IsGoalActive() && 
			m_nDebugCurIndex >= CAI_BaseNPC::m_nDebugPauseIndex)
		{
			m_flPlaybackRate = 0;
		}
	}
}


//=========================================================

bool CAI_BaseNPC::FindCoverPos( CBaseEntity *pEntity, Vector *pResult)
{
	if ( !GetTacticalServices()->FindLateralCover( pEntity->EyePosition(), pResult ) )
	{
		if ( !GetTacticalServices()->FindCoverPos( pEntity->GetAbsOrigin(), pEntity->EyePosition(), 0, CoverRadius(), pResult ) ) 
		{
			return false;
		}
	}
	return true;
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule. 
//=========================================================
void CAI_BaseNPC::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RESET_ACTIVITY:
		m_Activity = ACT_RESET;
		TaskComplete();
		break;

	case TASK_ANNOUNCE_ATTACK:
		{
			// Default case just completes.  Override in sub-classes
			// to play sound / animation / or pause
			TaskComplete();
			break;
		}

	case TASK_TURN_RIGHT:
		{
			float flCurrentYaw;
			
			flCurrentYaw = UTIL_AngleMod( GetLocalAngles().y );
			GetMotor()->SetIdealYaw( UTIL_AngleMod( flCurrentYaw - pTask->flTaskData ) );
			SetTurnActivity();
			break;
		}
	case TASK_TURN_LEFT:
		{
			float flCurrentYaw;
			
			flCurrentYaw = UTIL_AngleMod( GetLocalAngles().y );
			GetMotor()->SetIdealYaw( UTIL_AngleMod( flCurrentYaw + pTask->flTaskData ) );
			SetTurnActivity();
			break;
		}
	case TASK_REMEMBER:
		{
			Remember ( (int)pTask->flTaskData );
			TaskComplete();
			break;
		}
	case TASK_FORGET:
		{
			Forget ( (int)pTask->flTaskData );
			TaskComplete();
			break;
		}
	case TASK_FIND_HINTNODE:
	case TASK_FIND_LOCK_HINTNODE:
		{
			if (!m_pHintNode)
			{
				m_pHintNode = CAI_Hint::FindHint( this, HINT_NONE, pTask->flTaskData, 2000 );
			}
			if (m_pHintNode)
			{
				TaskComplete();
			}
			else
			{
				TaskFail(FAIL_NO_HINT_NODE);
			}
			if ( pTask->iTask == TASK_FIND_HINTNODE )
				break;
		}
		// Fall through on TASK_FIND_LOCK_HINTNODE...
		
	case TASK_LOCK_HINTNODE:
	{
		if (!m_pHintNode)
		{
			TaskFail(FAIL_NO_HINT_NODE);
		}
		else if( m_pHintNode->Lock(this) )
		{
			TaskComplete();
		}
		else
		{
			TaskFail(FAIL_ALREADY_LOCKED);
			m_pHintNode = NULL;
		}
		break;
	}

	case TASK_STORE_LASTPOSITION:
		{
			m_vecLastPosition = GetLocalOrigin();
			TaskComplete();
			break;
		}
	case TASK_CLEAR_LASTPOSITION:
		{
			m_vecLastPosition = vec3_origin;
			TaskComplete();
			break;
		}

	case TASK_STORE_POSITION_IN_SAVEPOSITION:
		{
			m_vSavePosition = GetLocalOrigin();
			TaskComplete();
			break;
		}

	case TASK_STORE_BESTSOUND_IN_SAVEPOSITION:
		{
			CSound *pBestSound = GetBestSound();
			if ( !pBestSound )
			{
				TaskFail("No Sound!");
				break;
			}

			m_vSavePosition = pBestSound->GetSoundOrigin();
			CBaseEntity *pSoundEnt = pBestSound->m_hOwner;
			if ( pSoundEnt )
			{
				Vector vel;
				pSoundEnt->GetVelocity( &vel, NULL );
				// HACKHACK: run away from cars in the right direction
				m_vSavePosition += vel * 2;	// add in 2 seconds of velocity
			}
			TaskComplete();
			break;
		}

	case TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION:
		{
			if ( GetEnemy() == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
			m_vSavePosition = GetEnemy()->GetAbsOrigin();
			TaskComplete();
			break;
		}

	case TASK_CLEAR_HINTNODE:
		{
			m_pHintNode->Unlock(0);
			m_pHintNode = NULL;
			TaskComplete();
			break;
		}
	case TASK_STOP_MOVING:
		{
			if (GetNavigator()->IsGoalActive())
			{
				GetNavigator()->ClearGoal();
				// E3 Hack
				if (LookupPoseParameter( "move_yaw") >= 0)
				{
					SetPoseParameter( "move_yaw", 0 );
				}
			}
			else
			{
				TaskComplete();
			}
		}
		break;
	case TASK_PLAY_PRIVATE_SEQUENCE:
	case TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY:
		{
			SetIdealActivity( (Activity)(int)pTask->flTaskData );
			break;
		}
		break;

	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_TARGET:
	case TASK_PLAY_SEQUENCE:
		{
			Activity goalActivity = (Activity)((int)pTask->flTaskData);
			SetIdealActivity( goalActivity );

			break;
		}
	case TASK_PLAY_HINT_ACTIVITY:
		{	
			SetIdealActivity( GetHintActivity(m_pHintNode->HintType()) );
			break;
		}
	case TASK_SET_SCHEDULE:
		{
			CAI_Schedule *pNewSchedule;

			pNewSchedule = GetScheduleOfType( (int)pTask->flTaskData );
			
			if ( pNewSchedule )
			{
				SetSchedule( pNewSchedule );
			}
			else
			{
				TaskFail(FAIL_SCHEDULE_NOT_FOUND);
			}

			break;
		}
	case TASK_FIND_BACKAWAY_FROM_SAVEPOSITION:
		{
			if ( GetEnemy() == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			Vector backPos;

			if ( !GetTacticalServices()->FindBackAwayPos( m_vSavePosition, &backPos ) )
			{
				// no place to backaway
				TaskFail(FAIL_NO_BACKAWAY_NODE);
			}
			else 
			{
				if (GetNavigator()->SetGoal( AI_NavGoal_t( backPos, ACT_RUN ) ) )
				{
					TaskComplete();
				}
				else
				{
					// no place to backaway
					TaskFail(FAIL_NO_ROUTE);
				}
			}
			break;
		}
	case TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY:
		{
			if ( GetEnemy() == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			Vector coverPos;

			if ( GetTacticalServices()->FindCoverPos( GetEnemy()->GetAbsOrigin(), GetEnemy()->EyePosition(), 0, pTask->flTaskData, &coverPos ) ) 
			{
				AI_NavGoal_t goal(GOALTYPE_COVER, coverPos, ACT_RUN);
				GetNavigator()->SetGoal( goal );
				// FIXME: add to goal
				if (m_pHintNode)
				{
					GetNavigator()->SetArrivalActivity( GetCoverActivity( m_pHintNode ) );
					GetNavigator()->SetArrivalDirection( m_pHintNode->GetDirection() );
				}
			}
			else
			{
				// no coverwhatsoever.
				TaskFail(FAIL_NO_COVER);
			}
			break;
		}
	case TASK_FIND_FAR_NODE_COVER_FROM_ENEMY:
		{
			if ( GetEnemy() == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			Vector coverPos;

			if ( GetTacticalServices()->FindCoverPos( GetEnemy()->GetAbsOrigin(), GetEnemy()->EyePosition(), pTask->flTaskData, CoverRadius(), &coverPos ) ) 
			{
				AI_NavGoal_t goal(GOALTYPE_COVER, coverPos, ACT_RUN, AIN_HULL_TOLERANCE);
				GetNavigator()->SetGoal( goal );
				// FIXME: add to goal
				if (m_pHintNode)
				{
					GetNavigator()->SetArrivalActivity( GetCoverActivity( m_pHintNode ) );
					GetNavigator()->SetArrivalDirection( m_pHintNode->GetDirection() );
				}

			}
			else
			{
				// no coverwhatsoever.
				TaskFail(FAIL_NO_COVER);
			}
			break;
		}
	case TASK_FIND_NODE_COVER_FROM_ENEMY:
		{
			if ( GetEnemy() == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			Vector coverPos;

			if ( GetTacticalServices()->FindCoverPos( GetEnemy()->GetAbsOrigin(), GetEnemy()->EyePosition(), 0, CoverRadius(), &coverPos ) ) 
			{
				AI_NavGoal_t goal(GOALTYPE_COVER, coverPos, ACT_RUN);
				GetNavigator()->SetGoal( goal );
				// FIXME: add to goal
				if (m_pHintNode)
				{
					GetNavigator()->SetArrivalActivity( GetCoverActivity( m_pHintNode ) );
					GetNavigator()->SetArrivalDirection( m_pHintNode->GetDirection() );
				}
			}
			else
			{
				// no coverwhatsoever.
				TaskFail(FAIL_NO_COVER);
			}
			break;
		}
	case TASK_FIND_COVER_FROM_ENEMY:
		{
			CBaseEntity *pEntity = GetEnemy();

			if ( pEntity == NULL )
			{
				// Find cover from self if no enemy available
				pEntity = this;
			}

			Vector coverPos;

			if ( m_pHintNode )
			{
				m_pHintNode->Unlock(0);
				m_pHintNode = NULL;
			}

			bool success = false;

			if ( FindCoverPos( pEntity, &coverPos ) )
			{
				AI_NavGoal_t goal(GOALTYPE_COVER, coverPos, ACT_RUN, AIN_HULL_TOLERANCE, AIN_DEF_FLAGS);

				if ( GetNavigator()->SetGoal( goal ) )
				{
					// FIXME: add to goal
					if (m_pHintNode)
					{
						GetNavigator()->SetArrivalActivity( GetCoverActivity( m_pHintNode ) );
						GetNavigator()->SetArrivalDirection( m_pHintNode->GetDirection() );
					}
 					m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
					success = true;
				}
			}

			if ( success )
				TaskComplete();
			else
				TaskFail(FAIL_NO_COVER);

			break;
		}

	case TASK_FIND_COVER_FROM_ORIGIN:
		{
			Vector coverPos;

			if ( GetTacticalServices()->FindCoverPos( GetLocalOrigin(), EyePosition(), 0, CoverRadius(), &coverPos ) ) 
			{
				AI_NavGoal_t goal(coverPos, ACT_RUN, AIN_HULL_TOLERANCE);
				GetNavigator()->SetGoal( goal );

				m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
			}
			else
			{
				// no coverwhatsoever.
				TaskFail(FAIL_NO_COVER);
			}
		}
		break;
	case TASK_FIND_COVER_FROM_BEST_SOUND:
		{
			CSound *pBestSound;

			pBestSound = GetBestSound();

			Assert( pBestSound != NULL );
			/*
			if ( pBestSound && FindLateralCover( pBestSound->m_vecOrigin, vec3_origin) )
			{
				// try lateral first
				m_flMoveWaitFinished = gpGlobals->curtime + pTask->flData;
				TaskComplete();
			}
			*/

			if (!pBestSound)
			{
				TaskFail("No sound in list");
				return;
			}

			Vector coverPos;

			// UNDONE: Back away if cover fails?  Grunts do this.
			if ( GetTacticalServices()->FindCoverPos( pBestSound->GetSoundOrigin(), pBestSound->GetSoundOrigin(), pBestSound->Volume(), CoverRadius(), &coverPos ) ) 
			{
				AI_NavGoal_t goal(coverPos, ACT_RUN, AIN_HULL_TOLERANCE);
				GetNavigator()->SetGoal( goal );
				m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
			}
			else
			{
				// no coverwhatsoever.
				TaskFail(FAIL_NO_COVER);
			}
		}
		break;
	case TASK_FACE_HINTNODE:
		{	
			GetMotor()->SetIdealYaw( m_pHintNode->Yaw() );
			SetTurnActivity();
			
			break;
		}
	
	case TASK_FACE_LASTPOSITION:
		GetMotor()->SetIdealYawToTarget( m_vecLastPosition );
		SetTurnActivity(); 
		break;

	case TASK_SET_IDEAL_YAW_TO_CURRENT:
		GetMotor()->SetIdealYaw( UTIL_AngleMod( GetLocalAngles().y ) );
		TaskComplete();
		break;

	case TASK_FACE_TARGET:
		if ( m_hTargetEnt != NULL )
		{
			GetMotor()->SetIdealYawToTarget( m_hTargetEnt->GetAbsOrigin() );
			SetTurnActivity(); 
		}
		else
			TaskFail(FAIL_NO_TARGET);
		break;
		
	case TASK_FACE_PLAYER:
		// track head to the client for a while.
		m_flWaitFinished = gpGlobals->curtime + pTask->flTaskData;
		break;

	case TASK_FACE_ENEMY:
		{
			Vector vecEnemyLKP = GetEnemyLKP();
			if (!FInAimCone( vecEnemyLKP ))
			{
				GetMotor()->SetIdealYawToTarget( vecEnemyLKP );
				SetTurnActivity(); 
			}
			else
			{
				TaskComplete();
			}
			break;
		}
	case TASK_FACE_IDEAL:
		{
			SetTurnActivity();
			break;
		}
	case TASK_FACE_PATH:
		{
			if (!GetNavigator()->IsGoalActive())
			{
				DevWarning( 2, "No route to face!\n");
				TaskFail(FAIL_NO_ROUTE);
			}
			else
			{
				const float NPC_TRIVIAL_TURN = 15;	// (Degrees). Turns this small or smaller, don't bother with a transition.

				GetMotor()->SetIdealYawToTarget( GetNavigator()->GetCurWaypointPos());

				if( fabs( GetMotor()->DeltaIdealYaw() ) <= NPC_TRIVIAL_TURN )
				{
					// This character is already facing the path well enough that 
					// moving will look fairly natural. Don't bother with a transitional
					// turn animation.
					TaskComplete();
					break;
				}

				SetTurnActivity();
			}
			break;
		}
	case TASK_WAIT_PVS:
	case TASK_WAIT_INDEFINITE:
		{
			// don't do anything.
			break;
		}
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
		{// set a future time that tells us when the wait is over.
			m_flWaitFinished = gpGlobals->curtime + pTask->flTaskData;
			break;
		}
	case TASK_WAIT_RANDOM:
	case TASK_WAIT_FACE_ENEMY_RANDOM:
		{// set a future time that tells us when the wait is over.
			m_flWaitFinished = gpGlobals->curtime + random->RandomFloat( 0.1, pTask->flTaskData );
			break;
		}
	case TASK_MOVE_TO_TARGET_RANGE:
		{
			if ( m_hTargetEnt == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else if ( (m_hTargetEnt->GetAbsOrigin() - GetLocalOrigin()).Length() < 1 )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_TARGET_PLAYER:
		{
			CBaseEntity *pPlayer = gEntList.FindEntityByName( NULL, "!player", this );
			if ( pPlayer )
			{
				SetTarget( pPlayer );
				TaskComplete();
			}
			else
				TaskFail( FAIL_NO_PLAYER );
			break;
		}

	case TASK_RUN_TO_TARGET:
	case TASK_WALK_TO_TARGET:
	case TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET:
		{
			Activity newActivity;

			if ( m_hTargetEnt == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else if ( (m_hTargetEnt->GetAbsOrigin() - GetLocalOrigin()).Length() < 1 )
			{
				TaskComplete();
			}
			else
			{
				//
				// Select the appropriate activity.
				//
				if ( pTask->iTask == TASK_WALK_TO_TARGET )
				{
					newActivity = ACT_WALK;
				}
				else if ( pTask->iTask == TASK_RUN_TO_TARGET )
				{
					newActivity = ACT_RUN;
				}
				else
				{
					newActivity = GetScriptCustomMoveActivity();
				}

				if ( ( newActivity != ACT_SCRIPT_CUSTOM_MOVE ) && TranslateActivity( newActivity ) == ACT_INVALID )
				{
					Assert( 0 );
					// This NPC can't do this!
					TaskComplete();
				}
				else 
				{
					if (m_hTargetEnt == NULL)
					{
						TaskFail(FAIL_NO_TARGET);
					}
					else 
					{

						AI_NavGoal_t goal( GOALTYPE_TARGETENT, newActivity );
						
						if ( GetState() == NPC_STATE_SCRIPT && 
							 ( m_ScriptArrivalActivity != AIN_DEF_ACTIVITY || 
							   m_strScriptArrivalSequence != NULL_STRING ) )
						{
							if ( m_ScriptArrivalActivity != AIN_DEF_ACTIVITY )
							{
								goal.arrivalActivity = m_ScriptArrivalActivity;
							}
							else
							{
								goal.arrivalSequence = LookupSequence( m_strScriptArrivalSequence.ToCStr() );
							}
						}
							
						if (!GetNavigator()->SetGoal( goal, AIN_DISCARD_IF_FAIL ))
						{
							TaskFail(FAIL_NO_ROUTE);
						}
						else
						{
							GetNavigator()->SetArrivalDirection( m_hTargetEnt->GetAbsAngles() );
						}
					}
				}
			}

			m_ScriptArrivalActivity = AIN_DEF_ACTIVITY;
			m_strScriptArrivalSequence = NULL_STRING;

			TaskComplete();
			break;
		}
	case TASK_CLEAR_MOVE_WAIT:
		{
			m_flMoveWaitFinished = gpGlobals->curtime;
			TaskComplete();
			break;
		}
	case TASK_MELEE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK1:
		{
			m_flLastAttackTime = gpGlobals->curtime;
			ResetIdealActivity( ACT_MELEE_ATTACK1 );
			break;
		}
	case TASK_MELEE_ATTACK2_NOTURN:
	case TASK_MELEE_ATTACK2:
		{
			m_flLastAttackTime = gpGlobals->curtime;
			ResetIdealActivity( ACT_MELEE_ATTACK2 );
			break;
		}
	case TASK_RANGE_ATTACK1_NOTURN:
	case TASK_RANGE_ATTACK1:
		{
			m_flLastAttackTime = gpGlobals->curtime;
			ResetIdealActivity( ACT_RANGE_ATTACK1 );
			break;
		}
	case TASK_RANGE_ATTACK2_NOTURN:
	case TASK_RANGE_ATTACK2:
		{
			m_flLastAttackTime = gpGlobals->curtime;
			ResetIdealActivity( ACT_RANGE_ATTACK2 );
			break;
		}
	case TASK_RELOAD_NOTURN:
	case TASK_RELOAD:
		{
			ResetIdealActivity( ACT_RELOAD );
			break;
		}
	case TASK_SPECIAL_ATTACK1:
		{
			ResetIdealActivity( ACT_SPECIAL_ATTACK1 );
			break;
		}
	case TASK_SPECIAL_ATTACK2:
		{
			ResetIdealActivity( ACT_SPECIAL_ATTACK2 );
			break;
		}
	case TASK_SET_ACTIVITY:
		{
			Activity goalActivity = (Activity)((int)pTask->flTaskData);
			if (goalActivity != ACT_RESET)
			{
				SetIdealActivity( goalActivity );
			}
			else
			{
				m_Activity = ACT_RESET;
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY_LKP:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if (!pEnemy || IsUnreachable(pEnemy))
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}
			AI_NavGoal_t goal( GetEnemyLKP() );

			TranslateEnemyChasePosition( pEnemy, goal.dest, goal.tolerance );

			if ( GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				DevWarning( 2, "GetPathToEnemyLKP failed!!\n" );
				RememberUnreachable(GetEnemy());
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}
	
	case TASK_GET_PATH_TO_ENEMY_LOS:
	case TASK_GET_PATH_TO_ENEMY_LKP_LOS:
		{
			if ( GetEnemy() == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
		
			Vector vecEnemy 	= ( pTask->iTask == TASK_GET_PATH_TO_ENEMY_LOS ) ? GetEnemy()->GetAbsOrigin() : GetEnemyLKP();
			Vector vecEnemyEye	= vecEnemy + GetEnemy()->GetViewOffset();

			float flMaxRange = 2000;
			float flMinRange = 0;
			
			if ( GetActiveWeapon() )
			{
				flMaxRange = max( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
				flMinRange = min( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
			}

			//Check against NPC's max range
			if ( flMaxRange > m_flDistTooFar )
			{
				flMaxRange = m_flDistTooFar;
			}

			Vector posLos;
			bool found = false;

			if ( GetTacticalServices()->FindLateralLos( vecEnemyEye, &posLos ) )
			{
				float dist = ( posLos - vecEnemyEye ).Length();
				if ( dist < flMaxRange && dist > flMinRange )
					found = true;
			}

			if ( !found && GetTacticalServices()->FindLos( vecEnemy, vecEnemyEye, flMinRange, flMaxRange, 1.0, &posLos ) )
			{
				found = true;
			}

			if ( found )
			{
				AI_NavGoal_t goal( posLos, ACT_RUN, AIN_HULL_TOLERANCE );

				GetNavigator()->SetGoal( goal, AIN_CLEAR_TARGET );
				GetNavigator()->SetArrivalDirection( vecEnemy - goal.dest );
			}
			else
			{
				TaskFail( FAIL_NO_SHOOT );
			}
		}
		break;

//==================================================
// TASK_SET_GOAL
//==================================================

	case TASK_SET_GOAL:

		switch ( (int) pTask->flTaskData )
		{
		case GOAL_ENEMY:	//Enemy
			
			if ( GetEnemy() == NULL )
			{
				TaskFail( FAIL_NO_ENEMY );
				return;
			}
			
			//Setup our stored info
			m_vecStoredPathGoal = GetEnemy()->GetAbsOrigin();
			m_nStoredPathType	= GOALTYPE_ENEMY;
			m_fStoredPathFlags	= 0;
			m_hStoredPathTarget	= GetEnemy();
			GetNavigator()->SetMovementActivity(ACT_RUN);
			break;
		
		case GOAL_ENEMY_LKP:		//Enemy's last known position

			if ( GetEnemy() == NULL )
			{
				TaskFail( FAIL_NO_ENEMY );
				return;
			}
			
			//Setup our stored info
			m_vecStoredPathGoal = GetEnemyLKP();
			m_nStoredPathType	= GOALTYPE_LOCATION;
			m_fStoredPathFlags	= 0;
			m_hStoredPathTarget	= NULL;
			GetNavigator()->SetMovementActivity(ACT_RUN);
			break;
		
		case GOAL_TARGET:			//Target entity
			
			if ( m_hTargetEnt == NULL )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}
			
			//Setup our stored info
			m_vecStoredPathGoal = m_hTargetEnt->GetAbsOrigin();
			m_nStoredPathType	= GOALTYPE_TARGETENT;
			m_fStoredPathFlags	= 0;
			m_hStoredPathTarget	= m_hTargetEnt;
			GetNavigator()->SetMovementActivity(ACT_RUN);
			break;

		case GOAL_SAVED_POSITION:	//Saved position
			
			//Setup our stored info
			m_vecStoredPathGoal = m_vSavePosition;
			m_nStoredPathType	= GOALTYPE_LOCATION;
			m_fStoredPathFlags	= 0;
			m_hStoredPathTarget	= NULL;
			GetNavigator()->SetMovementActivity(ACT_RUN);
			break;
		}

		TaskComplete();

		break;

//==================================================
// TASK_GET_PATH_TO_GOAL
//==================================================

	case TASK_GET_PATH_TO_GOAL:
		{
			AI_NavGoal_t goal( m_nStoredPathType, 
							   AIN_DEF_ACTIVITY, 
							   AIN_HULL_TOLERANCE,
							   AIN_DEF_FLAGS,
							   m_hStoredPathTarget );
			
			bool	foundPath = false;

			//Find our path
			switch ( (int) pTask->flTaskData )
			{
			case PATH_TRAVEL:	//A land path to our goal
				goal.dest = m_vecStoredPathGoal;
				foundPath = GetNavigator()->SetGoal( goal );
				break;

			case PATH_LOS:		//A path to get LOS to our goal
				{
					float flMaxRange = 2000.0f;
					float flMinRange = 0.0f;

					if ( GetActiveWeapon() )
					{
						flMaxRange = max( GetActiveWeapon()->m_fMaxRange1, GetActiveWeapon()->m_fMaxRange2 );
						flMinRange = min( GetActiveWeapon()->m_fMinRange1, GetActiveWeapon()->m_fMinRange2 );
					}

					// Check against NPC's max range
					if ( flMaxRange > m_flDistTooFar )
					{
						flMaxRange = m_flDistTooFar;
					}

					Vector	eyePosition = ( m_hStoredPathTarget != NULL ) ? m_hStoredPathTarget->EyePosition() : m_vecStoredPathGoal;

					Vector posLos;

					// See if we've found it
					if ( GetTacticalServices()->FindLos( m_vecStoredPathGoal, eyePosition, flMinRange, flMaxRange, 1.0f, &posLos ) )
					{
						goal.dest = posLos;
						foundPath = GetNavigator()->SetGoal( goal );
					}
					else
					{
						// No LOS to goal
						TaskFail( FAIL_NO_SHOOT );
						return;
					}
				}
				
				break;

			case PATH_COVER:	//Get a path to cover FROM our goal
				{
					CBaseEntity *pEntity = ( m_hStoredPathTarget == NULL ) ? this : m_hStoredPathTarget;

					//Find later cover first
					Vector coverPos;

					if ( GetTacticalServices()->FindLateralCover( pEntity->EyePosition(), &coverPos ) )
					{
						AI_NavGoal_t goal( coverPos, ACT_RUN );
						GetNavigator()->SetGoal( goal, AIN_CLEAR_PREVIOUS_STATE );
						
 						//FIXME: What exactly is this doing internally?
						m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
						TaskComplete();
						return;
					}
					else
					{
						//Try any cover
						if ( GetTacticalServices()->FindCoverPos( pEntity->GetAbsOrigin(), pEntity->EyePosition(), 0, CoverRadius(), &coverPos ) ) 
						{
							//If we've found it, find a safe route there
							AI_NavGoal_t coverGoal( GOALTYPE_COVER, 
													coverPos,
													ACT_RUN,
													AIN_HULL_TOLERANCE,
													AIN_DEF_FLAGS,
													m_hStoredPathTarget );
							
							foundPath = GetNavigator()->SetGoal( goal );

							m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
						}
						else
						{
							TaskFail( FAIL_NO_COVER );
						}
					}
				}

				break;
			}

			//Now validate our try
			if ( foundPath )
			{
				TaskComplete();
			}
			else
			{
				TaskFail( FAIL_NO_ROUTE );
			}
		}
		break;

	case TASK_GET_DROPSHIP_DEPLOY_PATH:
		{
			Vector	vecForward;
			Vector	vecGoal;
			trace_t	tr;

			GetVectors( &vecForward, NULL, NULL );

			vecGoal = GetLocalOrigin() + vecForward * 256;

			AI_TraceLine( vecGoal, vecGoal - Vector( 0, 0, 500 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			vecGoal = tr.endpos;

			if( GetNavigator()->SetGoal( vecGoal, AIN_CLEAR_TARGET ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail("gah");
			}

			TaskComplete();
		}
		break;

	case TASK_GET_PATH_TO_ENEMY:
		{
			if (IsUnreachable(GetEnemy()))
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}

			CBaseEntity *pEnemy = GetEnemy();

			if ( pEnemy == NULL )
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}
						
			if ( GetNavigator()->SetGoal( GOALTYPE_ENEMY ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =( 
				DevWarning( 2, "GetPathToEnemy failed!!\n" );
				RememberUnreachable(GetEnemy());
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY_CORPSE:
		{
			Vector forward;
			AngleVectors( GetLocalAngles(), &forward );
			Vector vecEnemyLKP = GetEnemyLKP();

			GetNavigator()->SetGoal( vecEnemyLKP - forward * 64, AIN_CLEAR_TARGET);
		}
		break;

	case TASK_GET_PATH_TO_PLAYER:
		{
			CBaseEntity *pPlayer = gEntList.FindEntityByName( NULL, "!player", this );

			AI_NavGoal_t goal;

			goal.type = GOALTYPE_LOCATION;
			goal.dest = pPlayer->WorldSpaceCenter();
			goal.pTarget = pPlayer;

			GetNavigator()->SetGoal( goal );
			break;
		}

	case TASK_GET_PATH_TO_SAVEPOSITION_LOS:
	{
		if ( GetEnemy() == NULL )
		{
			TaskFail(FAIL_NO_ENEMY);
			return;
		}
	
		float flMaxRange = 2000;
		float flMinRange = 0;
		if (GetActiveWeapon())
		{
			flMaxRange = max(GetActiveWeapon()->m_fMaxRange1,GetActiveWeapon()->m_fMaxRange2);
			flMinRange = min(GetActiveWeapon()->m_fMinRange1,GetActiveWeapon()->m_fMinRange2);
		}

		// Check against NPC's max range
		if (flMaxRange > m_flDistTooFar)
		{
			flMaxRange = m_flDistTooFar;
		}

		Vector posLos;

		if (GetTacticalServices()->FindLos(m_vSavePosition,m_vSavePosition, flMinRange, flMaxRange, 1.0, &posLos))
		{
			GetNavigator()->SetGoal( AI_NavGoal_t( posLos, ACT_RUN, AIN_HULL_TOLERANCE ) );
		}
		else
		{
			// no coverwhatsoever.
			TaskFail(FAIL_NO_SHOOT);
		}
		break;
	}

	case TASK_GET_PATH_TO_TARGET:
		{
			if (m_hTargetEnt == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else 
			{
				AI_NavGoal_t goal( m_hTargetEnt->EyePosition() );
				goal.pTarget = m_hTargetEnt;
				GetNavigator()->SetGoal( goal );
			}
			break;
		}

	case TASK_GET_PATH_TO_HINTNODE:// for active idles!
		{
			if (!m_pHintNode)
			{
				TaskFail(FAIL_NO_HINT_NODE);
			}
			else
			{
				Vector vHintPos;
				m_pHintNode->GetPosition(this, &vHintPos);

				GetNavigator()->SetGoal( AI_NavGoal_t( vHintPos, ACT_RUN ) );
			}
			break;
		}

	case TASK_GET_PATH_TO_COMMAND_GOAL:
		{
			if (!GetNavigator()->SetGoal( m_vecCommandGoal ))
			{
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}

	case TASK_CLEAR_COMMAND_TARGET:
		m_hCommandTarget.Set( NULL );
		break;

	case TASK_CLEAR_COMMAND_GOAL:
		m_vecCommandGoal = vec3_invalid;
		TaskComplete();
		break;
		
	case TASK_GET_PATH_TO_LASTPOSITION:
		{
			if (!GetNavigator()->SetGoal( m_vecLastPosition ))
			{
				TaskFail(FAIL_NO_ROUTE);
			}
			break;
		}

	case TASK_GET_PATH_TO_SAVEPOSITION:
		{
			GetNavigator()->SetGoal( m_vSavePosition );
			break;
		}


	case TASK_GET_PATH_TO_RANDOM_NODE:  // Task argument is lenth of path to build
		{
			if ( GetNavigator()->SetRandomGoal( pTask->flTaskData ) )
				TaskComplete();
			else
				TaskFail(FAIL_NO_REACHABLE_NODE);
		
			break;
		}

	case TASK_GET_PATH_TO_BESTSOUND:
		{
			CSound *pSound;

			pSound = GetBestSound();

			if (!pSound)
			{
				TaskFail(FAIL_NO_SOUND);
			}
			else
			{
				GetNavigator()->SetGoal( pSound->GetSoundOrigin() );
			}
			break;
		}
	case TASK_GET_PATH_TO_BESTSCENT:
		{
			CSound *pScent;

			pScent = GetBestScent();

			if (!pScent) 
			{
				TaskFail(FAIL_NO_SCENT);
			}
			else
			{
				GetNavigator()->SetGoal( pScent->GetSoundOrigin() );
			}
			break;
		}
	case TASK_MOVE_AWAY_PATH:
		{
			QAngle ang = GetLocalAngles();
			ang.y = GetMotor()->GetIdealYaw() + 180;
			Vector move;

			AngleVectors( ang, &move );
			if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, min(36,pTask->flTaskData), true ) )
			{
				TaskComplete();
			}
			else
			{
				ang.y = GetMotor()->GetIdealYaw() + 90;
				AngleVectors( ang, &move );

				if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, min(24,pTask->flTaskData), true ) )
				{
					TaskComplete();
				}
				else
				{
					ang.y = GetMotor()->GetIdealYaw() + 270;
					AngleVectors( ang, &move );

					if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, min(24,pTask->flTaskData), true ) )
					{
						TaskComplete();
					}
					else
					{
						Vector coverPos;

						if ( GetTacticalServices()->FindCoverPos( GetLocalOrigin(), EyePosition(), 0, CoverRadius(), &coverPos ) ) 
						{
							GetNavigator()->SetGoal( AI_NavGoal_t( coverPos, ACT_RUN ) );
							m_flMoveWaitFinished = gpGlobals->curtime + 2;
						}
						else
						{
							// no coverwhatsoever.
							TaskFail(FAIL_NO_COVER);
						}
					}
				}
			}
		}
		break;
	case TASK_WEAPON_RUN_PATH:
		GetNavigator()->SetMovementActivity(ACT_RUN);
		break;

	case TASK_RUN_PATH:
		{
			// UNDONE: This is in some default AI and some NPCs can't run? -- walk instead?
			if ( TranslateActivity( ACT_RUN ) != ACT_INVALID )
			{
				GetNavigator()->SetMovementActivity( ACT_RUN );
			}
			else
			{
				GetNavigator()->SetMovementActivity(ACT_WALK);
			}
			// Cover is void once I move
			Forget( bits_MEMORY_INCOVER );
			TaskComplete();
			break;
		}

	case TASK_WALK_PATH:
		{
			bool bIsFlying = (GetMoveType() == MOVETYPE_FLY) || (GetMoveType() == MOVETYPE_FLYGRAVITY);
			if ( bIsFlying && ( TranslateActivity( ACT_FLY ) != ACT_INVALID) )
			{
				GetNavigator()->SetMovementActivity(ACT_FLY);
			}
			else if ( TranslateActivity( ACT_WALK ) != ACT_INVALID )
			{
				GetNavigator()->SetMovementActivity(ACT_WALK);
			}
			else
			{
				GetNavigator()->SetMovementActivity(ACT_RUN);
			}
			// Cover is void once I move
			Forget( bits_MEMORY_INCOVER );
			TaskComplete();
			break;
		}
	case TASK_WALK_PATH_WITHIN_DIST:
		{
			GetNavigator()->SetMovementActivity(ACT_WALK);
			break;
		}
	case TASK_RUN_PATH_WITHIN_DIST:
		{
			GetNavigator()->SetMovementActivity(ACT_RUN);
			break;
		}
	case TASK_WALK_PATH_TIMED:
		{
			GetNavigator()->SetMovementActivity(ACT_WALK);
			m_flWaitFinished = gpGlobals->curtime + pTask->flTaskData;
			break;
		}
	case TASK_RUN_PATH_TIMED:
		{
			GetNavigator()->SetMovementActivity(ACT_RUN);
			m_flWaitFinished = gpGlobals->curtime + pTask->flTaskData;
			break;
		}
	case TASK_STRAFE_PATH:
		{
			Vector2D vec2DirToPoint; 
			Vector2D vec2RightSide;

			// to start strafing, we have to first figure out if the target is on the left side or right side
			Vector right;
			AngleVectors( GetLocalAngles(), NULL, &right, NULL );

			vec2DirToPoint = ( GetNavigator()->GetCurWaypointPos() - GetLocalOrigin() ).AsVector2D();
			Vector2DNormalize(vec2DirToPoint);
			vec2RightSide = right.AsVector2D();
			Vector2DNormalize(vec2RightSide);

			if ( DotProduct2D ( vec2DirToPoint, vec2RightSide ) > 0 )
			{
				// strafe right
				GetNavigator()->SetMovementActivity(ACT_STRAFE_RIGHT);
			}
			else
			{
				// strafe left
				GetNavigator()->SetMovementActivity(ACT_STRAFE_LEFT);
			}
			TaskComplete();
			break;
		}

	case TASK_WAIT_FOR_MOVEMENT_STEP:
		{
			if(!GetNavigator()->IsGoalActive())
			{
				TaskComplete();
				return;
			}

			if ( IsActivityFinished() )
			{
				TaskComplete();
				return;
			}
			ValidateNavGoal();
			break;
		}

	case TASK_WAIT_FOR_MOVEMENT:
		{
			if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				TaskComplete();
				GetNavigator()->ClearGoal();		// Stop moving
			}
			else if (!GetNavigator()->IsGoalActive())
			{
				SetIdealActivity( GetStoppedActivity() );
			}
			else
			{
				// Check validity of goal type
				ValidateNavGoal();
			}
			break;
		}
	case TASK_SMALL_FLINCH:
		{
			SetIdealActivity( GetSmallFlinchActivity() );
			break;
		}
	case TASK_DIE:
		{
			GetNavigator()->ClearGoal();	
			SetIdealActivity( GetDeathActivity() );
			m_lifeState = LIFE_DYING;

			break;
		}
	case TASK_SOUND_WAKE:
		{
			AlertSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_DIE:
		{
			DeathSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_IDLE:
		{
			IdleSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_PAIN:
		{
			PainSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_ANGRY:
		{
			// sounds are complete as soon as we get here, cause we've already played them.
			DevMsg( 2, "SOUND\n" );			
			TaskComplete();
			break;
		}
	case TASK_SPEAK_SENTENCE:
		{
			SpeakSentence(pTask->flTaskData);	
			TaskComplete();
			break;
		}
	case TASK_WAIT_FOR_SCRIPT:
		{
			if ( !m_hCine )
			{
				Msg( "Scripted sequence destroyed while in use\n" );
				TaskFail( FAIL_SCHEDULE_NOT_FOUND );
				break;
			}

			//
			// Start waiting to play a script. Play the script's pre idle animation if one
			// is specified, otherwise just go to our default idle activity.
			//
			if ( m_hCine->m_iszPreIdle != NULL_STRING )
			{
				m_hCine->StartSequence( ( CAI_BaseNPC * )this, m_hCine->m_iszPreIdle, false );
				if ( FStrEq( STRING( m_hCine->m_iszPreIdle ), STRING( m_hCine->m_iszPlay ) ) )
				{
					m_flPlaybackRate = 0;
				}
			}
			else if ( m_scriptState != SCRIPT_CUSTOM_MOVE_TO_MARK )
			{
				SetIdealActivity( ACT_IDLE );
			}

			break;
		}
	case TASK_PUSH_SCRIPT_ARRIVAL_ACTIVITY:
		{
			if ( !m_hCine )
			{
				Msg( "Scripted sequence destroyed while in use\n" );
				TaskFail( FAIL_SCHEDULE_NOT_FOUND );
				break;
			}

			string_t iszArrivalText;

			if ( m_hCine->m_iszPlay != NULL_STRING )
			{
				iszArrivalText = m_hCine->m_iszPlay;
			}
			else if ( m_hCine->m_iszPostIdle != NULL_STRING )
			{
				iszArrivalText = m_hCine->m_iszPostIdle;
			}
			else
				iszArrivalText = NULL_STRING;

			m_ScriptArrivalActivity = AIN_DEF_ACTIVITY;
			m_strScriptArrivalSequence = NULL_STRING;

			if ( iszArrivalText != NULL_STRING )
			{
				m_ScriptArrivalActivity = (Activity)GetActivityID( STRING( iszArrivalText ) );
				if ( m_ScriptArrivalActivity == ACT_INVALID )
					m_strScriptArrivalSequence = iszArrivalText;
			}

			TaskComplete();
			break;
		}

	case TASK_PLAY_SCRIPT:
		{
			if (!HasMovement( GetSequence() ))
			{
#if 0
				// @Note (toml 04-07-03): removed at Ken's recommendation
				// FIXME: the movement is set to fly so 
				SetMoveType( MOVETYPE_FLY );
				RemoveFlag( FL_ONGROUND );
#endif
			}
			else
			{


			}
			//
			// Start playing a scripted sequence.
			//
			m_scriptState = SCRIPT_PLAYING;
			break;
		}
	case TASK_PLAY_SCRIPT_POST_IDLE:
		{
			//
			// Start playing a scripted post idle.
			//
			m_scriptState = SCRIPT_POST_IDLE;
			break;
		}
	case TASK_ENABLE_SCRIPT:
		{
			if ( !m_hCine )
			{
				Msg( "Scripted sequence destroyed while in use\n" );
				TaskFail( FAIL_SCHEDULE_NOT_FOUND );
				break;
			}

			m_hCine->DelayStart( 0 );
			TaskComplete();
			break;
		}
	case TASK_PLANT_ON_SCRIPT:
		{
			if ( m_hTargetEnt != NULL )
			{
				SetLocalOrigin( m_hTargetEnt->GetAbsOrigin() );	// Plant on target
			}

			TaskComplete();
			break;
		}
	case TASK_FACE_SCRIPT:
		{
			if ( m_hTargetEnt != NULL )
			{
				GetMotor()->SetIdealYaw( UTIL_AngleMod( m_hTargetEnt->GetLocalAngles().y ) );
			}

			if ( m_scriptState != SCRIPT_CUSTOM_MOVE_TO_MARK )
			{
				SetTurnActivity();
			}

			GetNavigator()->ClearGoal();
			break;
		}

	case TASK_SUGGEST_STATE:
		{
			m_IdealNPCState = (NPC_STATE)(int)pTask->flTaskData;
			TaskComplete();
			break;
		}

	case TASK_SET_FAIL_SCHEDULE:
		m_failSchedule = (int)pTask->flTaskData;
		TaskComplete();
		break;

	case TASK_SET_TOLERANCE_DISTANCE:
		GetNavigator()->SetGoalTolerance( (int)pTask->flTaskData );
		TaskComplete();
		break;

	case TASK_SET_ROUTE_SEARCH_TIME:
		GetNavigator()->SetMaxRouteRebuildTime( (int)pTask->flTaskData );
		TaskComplete();
		break;

	case TASK_CLEAR_FAIL_SCHEDULE:
		m_failSchedule = SCHED_NONE;
		TaskComplete();
		break;

	case TASK_WEAPON_FIND:
		{
			m_hTargetEnt = Weapon_FindUsable( Vector(1000,1000,1000) );
			if (m_hTargetEnt)
			{
				TaskComplete();
			}
			else
			{
				TaskFail(FAIL_WEAPON_NO_FIND);
			}
		}
		break;

	case TASK_WEAPON_PICKUP:
		{
			SetIdealActivity( ACT_PICKUP_GROUND );
		}
		break;

	case TASK_USE_SMALL_HULL:
		{
			SetHullSizeSmall();
			TaskComplete();
		}
		break;
	
	case TASK_FALL_TO_GROUND:
		break;
		
	case TASK_WANDER:
		{
			// This task really uses 2 parameters, so we have to extract
			// them from a single integer. To send both parameters, the
			// formula is MIN_DIST * 10000 + MAX_DIST
			{
				int iMinDist, iMaxDist, iParameter;

				iParameter = pTask->flTaskData;

				iMinDist = iParameter / 10000;
				iMaxDist = iParameter - (iMinDist * 10000);

				if ( GetNavigator()->SetWanderGoal( iMinDist, iMaxDist ) )
					TaskComplete();
				else
					TaskFail(FAIL_NO_REACHABLE_NODE);
			}
		}
		break;

	case TASK_FREEZE:
		m_flPlaybackRate = 0;
		break;

	case TASK_GATHER_CONDITIONS:
		GatherConditions();
		TaskComplete();
		break;

	case TASK_IGNORE_OLD_ENEMIES:
		m_flAcceptableTimeSeenEnemy = gpGlobals->curtime;
		if ( GetEnemy() && GetEnemyLastTimeSeen() < m_flAcceptableTimeSeenEnemy )
		{
			CBaseEntity *pNewEnemy = BestEnemy();

			Assert( pNewEnemy != GetEnemy() );

			if( pNewEnemy != NULL )
			{
				//New enemy! Clear the timers and set conditions.
				SetCondition( COND_NEW_ENEMY );
				SetEnemy( pNewEnemy );
				SetState( NPC_STATE_COMBAT );
			}
			else
			{
				SetEnemy( NULL );
				ClearAttackConditions();
			}
		}
		TaskComplete();
		break;

	case TASK_ADD_HEALTH:
		TakeHealth( (int)pTask->flTaskData, DMG_GENERIC );
		TaskComplete();
		break;

	default:
		{
			Msg( "No StartTask entry for %s\n", TaskName( pTask->iTask ) );
		}
		break;
	}
}

void CAI_BaseNPC::StartTaskOverlay()
{
	if ( IsCurTaskContinuousMove() )
	{
		if ( ShouldMoveAndShoot() )
		{
			m_MoveAndShootOverlay.StartShootWhileMove();
		}
		else
		{
			m_MoveAndShootOverlay.NoShootWhileMove();
		}
	}
}

//=========================================================
// RunTask 
//=========================================================
void CAI_BaseNPC::RunTask( const Task_t *pTask )
{
	VPROF_BUDGET( "CAI_BaseNPC::RunTask", VPROF_BUDGETGROUP_NPCS );
	switch ( pTask->iTask )
	{
	case TASK_GET_PATH_TO_RANDOM_NODE:
		{
			break;
		}
	case TASK_TURN_RIGHT:
	case TASK_TURN_LEFT:
		{
			GetMotor()->UpdateYaw();

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}

	case TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_TARGET:
		{
			CBaseEntity *pTarget;

			if ( pTask->iTask == TASK_PLAY_SEQUENCE_FACE_TARGET )
				pTarget = m_hTargetEnt;
			else
				pTarget = GetEnemy();
			if ( pTarget )
			{
				GetMotor()->SetIdealYawAndUpdate( pTarget->GetAbsOrigin() - GetLocalOrigin() , AI_KEEP_YAW_SPEED );
			}

			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
		}		
		break;

	case TASK_PLAY_HINT_ACTIVITY:
		{
			if (!m_pHintNode)
			{
				TaskFail(FAIL_NO_HINT_NODE);
			}

			// Put a debugging check in here
			if (m_pHintNode->User() != this)
			{
				Msg("Hint node (%s) being used by non-owner!\n",m_pHintNode->GetDebugName());
			}

			if ( IsActivityFinished() )
			{
				TaskComplete();
			}

			break;
		}

	case TASK_STOP_MOVING:
		{
			// if they're jumping, wait until they land
			if (GetNavType() == NAV_JUMP)
			{
				if (GetFlags() & FL_ONGROUND)
				{
					SetNavType( NAV_GROUND ); // this assumes that NAV_JUMP only happens with npcs that use NAV_GROUND as base movement
				}
				else if (GetSmoothedVelocity().Length() > 0.01) // use an EPSILON damnit!!
				{
					// wait until you land
					break;
				}
				else
				{
					// stopped and stuck!
					SetNavType( NAV_GROUND );
					TaskFail( FAIL_STUCK_ONTOP );						
				}
			}

			// @TODO (toml 10-30-02): this is unacceptable, but needed until navigation can handle commencing
			// 						  a navigation while in the middle of a climb
			if (GetNavType() == NAV_CLIMB)
			{
				// wait until you reach the end
				break;
			}

			SetIdealActivity( GetStoppedActivity() );

			TaskComplete();
			break;
		}

	case TASK_PLAY_SEQUENCE:
	case TASK_PLAY_PRIVATE_SEQUENCE:
		{
			AutoMovement( );
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}

	case TASK_SET_ACTIVITY:
		{
			if ( IsActivityStarted() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_FACE_ENEMY:
		{
			Vector vecEnemyLKP = GetEnemyLKP();
			GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP );

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_FACE_PLAYER:
		{
			// Get edict for one player
			CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( engine->PEntityOfEntIndex( 1 ) );
			Assert( pPlayer );
			if ( pPlayer )
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( pPlayer->GetAbsOrigin(), AI_KEEP_YAW_SPEED );
				SetTurnActivity();
				if ( gpGlobals->curtime > m_flWaitFinished && GetMotor()->DeltaIdealYaw() < 10 )
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail(FAIL_NO_PLAYER);
			}
		}
		break;

	case TASK_FACE_HINTNODE:
	case TASK_FACE_LASTPOSITION:
	case TASK_FACE_TARGET:
	case TASK_FACE_IDEAL:
	case TASK_FACE_SCRIPT:
	case TASK_FACE_PATH:
		{
			GetMotor()->UpdateYaw();

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_PVS:
		{
			if ( ShouldAlwaysThink() || UTIL_FindClientInPVS(edict()) )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_INDEFINITE:
		{
			// don't do anything.
			break;
		}
	case TASK_WAIT:
	case TASK_WAIT_RANDOM:
		{
			if ( gpGlobals->curtime >= m_flWaitFinished )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_FACE_ENEMY:
	case TASK_WAIT_FACE_ENEMY_RANDOM:
		{
			Vector vecEnemyLKP = GetEnemyLKP();
			if (!FInAimCone( vecEnemyLKP ))
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP , AI_KEEP_YAW_SPEED );
			}

			if ( gpGlobals->curtime >= m_flWaitFinished )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_MOVE_TO_TARGET_RANGE:
		{
			float distance;

			if ( m_hTargetEnt == NULL )
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				distance = ( GetNavigator()->GetGoalPos() - GetLocalOrigin() ).Length2D();
				// Re-evaluate when you think your finished, or the target has moved too far
				if ( (distance < pTask->flTaskData) || (GetNavigator()->GetGoalPos() - m_hTargetEnt->GetAbsOrigin()).Length() > pTask->flTaskData * 0.5 )
				{
					distance = ( m_hTargetEnt->GetAbsOrigin() - GetLocalOrigin() ).Length2D();
					GetNavigator()->UpdateGoalPos( m_hTargetEnt->GetAbsOrigin() );
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				// BUGBUG: this is checking linear distance (ie. through walls) and not path distance or even visibility
				if ( distance < pTask->flTaskData )
				{
					TaskComplete();
					GetNavigator()->ClearGoal();		// Stop moving
				}
				else
				{
					// Pick the right movement activity.
					Activity followActivity = GetFollowActivity( distance );

					GetNavigator()->SetMovementActivity(followActivity);
					m_IdealActivity = followActivity;
				}
			}
			break;
		}
	case TASK_WEAPON_RUN_PATH:
		{
			CBaseEntity *pTarget = m_hTargetEnt;
			if ( pTarget )
			{
				if ( pTarget->GetOwnerEntity() )
				{
					TaskFail(FAIL_WEAPON_OWNED);
				}
				else if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
				{
					TaskComplete();
					GetNavigator()->ClearGoal();		// Stop moving
				}
			}
			else
			{
				TaskFail(FAIL_WEAPON_NO_FIND);
			}
		}
		break;

	case TASK_WAIT_FOR_MOVEMENT_STEP:
	case TASK_WAIT_FOR_MOVEMENT:
		{
			bool fTimeExpired = ( pTask->flTaskData != 0 && pTask->flTaskData < gpGlobals->curtime - GetTimeTaskStarted() );
			
			if (fTimeExpired || GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				TaskComplete();
				GetNavigator()->ClearGoal();		// Stop moving
			}
			else if (!GetNavigator()->IsGoalActive())
			{
				SetIdealActivity( GetStoppedActivity() );
			}
			else
			{
				// Check validity of goal type
				ValidateNavGoal();
			}
			break;
		}
	case TASK_DIE:
		{
			if ( IsActivityFinished() && m_flCycle >= 1.0f )
			{
				m_lifeState = LIFE_DEAD;
				
				SetThink ( NULL );
				StopAnimation();

				if ( !BBoxFlat() )
				{
					// a bit of a hack. If a corpses' bbox is positioned such that being left solid so that it can be attacked will
					// block the player on a slope or stairs, the corpse is made nonsolid. 
//					SetSolid( SOLID_NOT );
					UTIL_SetSize ( this, Vector ( -4, -4, 0 ), Vector ( 4, 4, 1 ) );
				}
				else // !!!HACKHACK - put NPC in a thin, wide bounding box until we fix the solid type/bounding volume problem
					UTIL_SetSize ( this, WorldAlignMins(), Vector ( WorldAlignMaxs().x, WorldAlignMaxs().y, WorldAlignMins().z + 1 ) );

				if ( ShouldFadeOnDeath() )
				{
					// this NPC was created by a NPCmaker... fade the corpse out.
					SUB_StartFadeOut();
				}
				else
				{
					// body is gonna be around for a while, so have it stink for a bit.
					CSoundEnt::InsertSound ( SOUND_CARCASS, GetLocalOrigin(), 384, 30 );
				}
			}
			break;
		}
	case TASK_RANGE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK2_NOTURN:
	case TASK_RANGE_ATTACK2_NOTURN:
	case TASK_RELOAD_NOTURN:
		{
			AutoMovement( );

			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_RANGE_ATTACK1:
	case TASK_MELEE_ATTACK1:
	case TASK_MELEE_ATTACK2:
	case TASK_RANGE_ATTACK2:
	case TASK_SPECIAL_ATTACK1:
	case TASK_SPECIAL_ATTACK2:
	case TASK_RELOAD:
		{
			AutoMovement( );

			Vector vecEnemyLKP = GetEnemyLKP();

			if ( ( pTask->iTask == TASK_RANGE_ATTACK1 || pTask->iTask == TASK_RELOAD ) && 
				 ( CapabilitiesGet() & bits_CAP_AIM_GUN ) && 
				 FInAimCone( vecEnemyLKP ) )
			{
				// Arms will aim, so leave body yaw as is
				GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
			}
			else
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
			}

			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_SMALL_FLINCH:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
		}
		break;
	case TASK_WAIT_FOR_SCRIPT:
		{
			//
			// Waiting to play a script. If the script is ready, start playing the sequence.
			//
			if ( m_hCine && m_hCine->IsTimeToStart() )
			{
				TaskComplete();
				m_hCine->OnBeginSequence();
				m_hCine->StartSequence( (CAI_BaseNPC *)this, m_hCine->m_iszPlay, true );
				if ( IsSequenceFinished() )
				{
					ClearSchedule();
				}

				m_flPlaybackRate = 1.0;
				//DevMsg( 2, "Script %s has begun for %s\n", STRING( m_hCine->m_iszPlay ), GetClassname() );
			}
			else if (!m_hCine)
			{
				Msg( "Cine died!\n");
				TaskComplete();
			}
			break;
		}
	case TASK_PLAY_SCRIPT:
		{
			//
			// Playing a scripted sequence.
			//
			AutoMovement( );

			if ( IsSequenceFinished() )
			{
				if (m_hCine)
				{
					m_hCine->SequenceDone( this );
				}

				TaskComplete();
			}
			break;
		}

	case TASK_PLAY_SCRIPT_POST_IDLE:
		{
			if ( !m_hCine )
			{
				Msg( "Scripted sequence destroyed while in use\n" );
				TaskFail( FAIL_SCHEDULE_NOT_FOUND );
				break;
			}
			//
			// Playing a scripted post idle sequence. Clean up if another sequence has grabbed the NPC.
			//
			if ( IsSequenceFinished() || ( m_hCine->m_hNextCine != NULL ) )
			{
				m_hCine->PostIdleDone( this );
			}
			break;
		}

	case TASK_RUN_PATH_WITHIN_DIST:
	case TASK_WALK_PATH_WITHIN_DIST:
		{
			Vector vecDiff;

			vecDiff = GetLocalOrigin() - GetNavigator()->GetGoalPos();

			if( vecDiff.Length() <= pTask->flTaskData )
			{
				TaskComplete();
				GetNavigator()->ClearGoal();
			}
			break;
		}

	case TASK_WALK_PATH_TIMED:
	case TASK_RUN_PATH_TIMED:
		{
			if ( gpGlobals->curtime > m_flWaitFinished || 
				 GetNavigator()->GetGoalType() == GOALTYPE_NONE )
			{
				TaskComplete();
				GetNavigator()->ClearGoal();
			}
		}
		break;

	case TASK_WEAPON_PICKUP:
		{
			if ( IsActivityFinished() )
			{
				CBaseCombatWeapon	 *pWeapon = dynamic_cast<CBaseCombatWeapon *>(	(CBaseEntity *)m_hTargetEnt);
				CBaseCombatCharacter *pOwner  = pWeapon->GetOwner();
				if ( !pOwner )
				{
					TaskComplete();
				}
				else
				{
					TaskFail(FAIL_WEAPON_OWNED);
				}
			}
			break;
		}
		break;

	case TASK_FALL_TO_GROUND:
		if ( GetFlags() & FL_ONGROUND )
		{
			TaskComplete();
		}
		break;

	case TASK_WANDER:
		break;

	case TASK_FREEZE:
		break;

	default:
		{
			Msg( "No RunTask entry for %s\n", TaskName( pTask->iTask ) );
			TaskComplete();
		}
		break;
	}
}

void CAI_BaseNPC::RunTaskOverlay()
{
	if ( IsCurTaskContinuousMove() )
	{
		m_MoveAndShootOverlay.RunShootWhileMove();
	}
}

void CAI_BaseNPC::EndTaskOverlay()
{
	m_MoveAndShootOverlay.EndShootWhileMove();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the activity this character should use to follow an object or character.
//			
// Input  : Distance to the object to follow.
//-----------------------------------------------------------------------------
Activity CAI_BaseNPC::GetFollowActivity( float flDistance )
{
	if ( flDistance < 190 && m_NPCState != NPC_STATE_COMBAT )
	{
		return ACT_WALK;
	}
	else
	{
		return ACT_RUN;
	}
}


//=========================================================
// SetTurnActivity - measures the difference between the way
// the NPC is facing and determines whether or not to
// select one of the 180 turn animations.
//=========================================================
void CAI_BaseNPC::SetTurnActivity ( void )
{
	float flYD;
	flYD = GetMotor()->DeltaIdealYaw();

	if( flYD <= -80 && flYD >= -100 && SelectWeightedSequence( ACT_90_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
	{
		// 90 degree right.
		Remember( bits_MEMORY_TURNING );
		SetIdealActivity( ACT_90_RIGHT );
		return;
	}
	if( flYD >= 80 && flYD <= 100 && SelectWeightedSequence( ACT_90_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{
		// 90 degree left.
		Remember( bits_MEMORY_TURNING );
		SetIdealActivity( ACT_90_LEFT );
		return;
	}
	if( fabs( flYD ) >= 160 && SelectWeightedSequence ( ACT_180_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{
		Remember( bits_MEMORY_TURNING );
		SetIdealActivity( ACT_180_LEFT );
		return;
	}

	if ( flYD <= -45 && SelectWeightedSequence ( ACT_TURN_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
	{// big right turn
		SetIdealActivity( ACT_TURN_RIGHT );
		return;
	}
	if ( flYD >= 45 && SelectWeightedSequence ( ACT_TURN_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{// big left turn
		SetIdealActivity( ACT_TURN_LEFT );
		return;
	}
	SetIdealActivity( ACT_IDLE ); // failure case
}


//-----------------------------------------------------------------------------
// Purpose: For non-looping animations that may be replayed sequentially (like attacks)
//			Set the activity to ACT_RESET if this is a replay, otherwise just set ideal activity
// Input  : newIdealActivity - desired ideal activity
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ResetIdealActivity( Activity newIdealActivity )
{
	if ( m_Activity == newIdealActivity )
	{
		m_Activity = ACT_RESET;
	}

	SetIdealActivity( newIdealActivity );
}

			
void CAI_BaseNPC::TranslateEnemyChasePosition( CBaseEntity *pEnemy, Vector &chasePosition, float &tolerance )
{
	if ( GetNavType() == NAV_FLY )
	{
		// UNDONE: Cache these per enemy instead?
		Vector offset = pEnemy->EyePosition() - pEnemy->GetAbsOrigin();
		chasePosition += offset;
		tolerance = GetHullWidth();
	}
	else
	{
		// otherwise, leave chasePosition as is
		tolerance = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns the custom movement activity for the script that this NPC
//			is running.
// Output : Returns the activity, or ACT_INVALID is the sequence is unknown.
//-----------------------------------------------------------------------------
Activity CAI_BaseNPC::GetScriptCustomMoveActivity( void )
{
	Activity eActivity = ACT_WALK;

	if ( ( m_hCine != NULL ) && ( m_hCine->m_iszCustomMove != NULL_STRING ) )
	{
		//
		// We have a valid script. Look up the custom movement activity.
		//
		eActivity = ( Activity )LookupActivity( STRING( m_hCine->m_iszCustomMove ) );
		if ( eActivity == ACT_INVALID )
		{
			// Not an activity, at least make sure it's a valid sequence.
			if ( LookupSequence( STRING( m_hCine->m_iszCustomMove ) ) != ACT_INVALID )
			{
				eActivity = ACT_SCRIPT_CUSTOM_MOVE;
			}
			else
			{
				eActivity = ACT_WALK;
			}
		}
	}

	return eActivity;
}


//=========================================================
// GetTask - returns a pointer to the current 
// scheduled task. NULL if there's a problem.
//=========================================================
const Task_t *CAI_BaseNPC::GetTask ( void ) 
{
	int iScheduleIndex = GetScheduleCurTaskIndex();
	if ( !GetCurSchedule() ||  iScheduleIndex < 0 || iScheduleIndex >= GetCurSchedule()->NumTasks() )
		// iScheduleIndex is not within valid range for the NPC's current schedule.
		return NULL;

	return &GetCurSchedule()->GetTaskList()[ iScheduleIndex ];
}


//-----------------------------------------------------------------------------

bool CAI_BaseNPC::IsInterruptable()
{
	if ( GetState() == NPC_STATE_SCRIPT )
	{
		if ( m_hCine && !m_hCine->CanInterrupt() )
			return false;
	}
	
	return IsAlive();
}

//-----------------------------------------------------------------------------
// Purpose: Decides which type of schedule best suits the NPC's current 
// state and conditions. Then calls NPC's member function to get a pointer 
// to a schedule of the proper type.
//
//
//-----------------------------------------------------------------------------
int CAI_BaseNPC::SelectSchedule( void )
{
	switch	( m_NPCState )
	{
	case NPC_STATE_NONE:
		{
			DevWarning( 2, "NPC_STATE IS NONE!\n" );
			break;
		}
	case NPC_STATE_PRONE:
		{
			return SCHED_IDLE_STAND;
		}
	case NPC_STATE_IDLE:
		{
			if ( HasCondition ( COND_HEAR_DANGER ) ||
				 HasCondition ( COND_HEAR_COMBAT ) ||
				 HasCondition ( COND_HEAR_WORLD  ) ||
				 HasCondition ( COND_HEAR_BULLET_IMPACT ) ||
				 HasCondition ( COND_HEAR_PLAYER ) )
			{
				return SCHED_ALERT_FACE;
			}
			// Get out of someone's way
			else if ( HasCondition ( COND_GIVE_WAY ) )
			{
				return SCHED_GIVE_WAY;
			}
			else if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				// no valid route!
				return SCHED_IDLE_STAND;
			}

			else if (HasCondition(COND_LIGHT_DAMAGE) && SelectWeightedSequence( ACT_SMALL_FLINCH ) != -1 )
			{
				return SCHED_SMALL_FLINCH;
			}
			else
			{
				// valid route. Get moving
				return SCHED_IDLE_WALK;
			}
			break;
		}
	case NPC_STATE_ALERT:
		{
			if ( HasCondition( COND_ENEMY_DEAD ) && SelectWeightedSequence( ACT_VICTORY_DANCE ) != ACTIVITY_NOT_AVAILABLE )
			{
				// Scan around for new enemies
				return SCHED_ALERT_SCAN;
			}

			if ( HasCondition(COND_LIGHT_DAMAGE) ||
				 HasCondition(COND_HEAVY_DAMAGE) )
			{
				if ( fabs( GetMotor()->DeltaIdealYaw() ) < (1.0 - m_flFieldOfView) * 60 ) // roughly in the correct direction
				{
					return SCHED_TAKE_COVER_FROM_ORIGIN;
				}
				else if ( SelectWeightedSequence( ACT_SMALL_FLINCH ) != -1 )
				{
					return SCHED_ALERT_SMALL_FLINCH;
				}
				// Not facing the correct dir, and I have no flinch animation
				return SCHED_ALERT_FACE;
			}
			else if ( HasCondition ( COND_HEAR_DANGER ) ||
					  HasCondition ( COND_HEAR_PLAYER ) ||
					  HasCondition ( COND_HEAR_WORLD  ) ||
					  HasCondition ( COND_HEAR_BULLET_IMPACT ) ||
					  HasCondition ( COND_HEAR_COMBAT ) )
			{
				return SCHED_ALERT_FACE;
			}
			else
			{
				return SCHED_ALERT_STAND;
			}
			break;
		}
	case NPC_STATE_COMBAT:
		{
			if ( HasCondition(COND_NEW_ENEMY) )
			{
				return SCHED_WAKE_ANGRY;
			}
			else if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// clear the current (dead) enemy and try to find another.
				SetEnemy( NULL );
				 
				if ( ChooseEnemy() )
				{
					ClearCondition( COND_ENEMY_DEAD );
					return SelectSchedule();
				}

				SetState( NPC_STATE_ALERT );
				return SelectSchedule();
			}
			else if ( ( HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE) ) && !HasMemory( bits_MEMORY_FLINCHED) && SelectWeightedSequence( ACT_SMALL_FLINCH ) != -1 )
			{
				return SCHED_SMALL_FLINCH;
			}
			// ---------------------------------------------------
			//  If I'm scared of this enemy run away
			// ---------------------------------------------------
			else if (IRelationType( GetEnemy() ) == D_FR)
			{
				if (HasCondition( COND_SEE_ENEMY )	|| 
					HasCondition( COND_LIGHT_DAMAGE )|| 
					HasCondition( COND_HEAVY_DAMAGE))
				{
					FearSound();
					ClearCommandGoal();
					return SCHED_RUN_FROM_ENEMY;
				}
				else
				{
					return SCHED_FEAR_FACE;
				}
			}
			else if ( !HasCondition(COND_SEE_ENEMY) )
			{
				// we can't see the enemy
				if ( !HasCondition(COND_ENEMY_OCCLUDED) )
				{
					// enemy is unseen, but not occluded!
					// turn to face enemy
					return SCHED_COMBAT_FACE;
				}
				else
				{
					// chase!
					if ( GetActiveWeapon() || (CapabilitiesGet() & (bits_CAP_INNATE_MELEE_ATTACK1|bits_CAP_INNATE_MELEE_ATTACK2)))
						// if we can see enemy but can't use either attack type, we must need to get closer to enemy
						return SCHED_CHASE_ENEMY;
					else
						return SCHED_TAKE_COVER_FROM_ENEMY;
				}
			}
			else if ( HasCondition(COND_TOO_CLOSE_TO_ATTACK) ) 
			{
				return SCHED_BACK_AWAY_FROM_ENEMY;
			}
			else if ( HasCondition( COND_WEAPON_PLAYER_IN_SPREAD ))
			{
				return SCHED_ESTABLISH_LINE_OF_FIRE;
			}
			else  
			{
				// we can see the enemy
				if ( HasCondition(COND_CAN_RANGE_ATTACK1) )
				{
					return SCHED_RANGE_ATTACK1;
				}
				if ( HasCondition(COND_CAN_RANGE_ATTACK2) )
				{
					return SCHED_RANGE_ATTACK2;
				}
				if ( HasCondition(COND_CAN_MELEE_ATTACK1) )
				{
					return SCHED_MELEE_ATTACK1;
				}
				if ( HasCondition(COND_CAN_MELEE_ATTACK2) )
				{
					return SCHED_MELEE_ATTACK2;
				}
				if ( HasCondition(COND_NOT_FACING_ATTACK) )
				{
					return SCHED_COMBAT_FACE;
				}
				else if ( !HasCondition(COND_CAN_RANGE_ATTACK1) && 
						  !HasCondition(COND_CAN_MELEE_ATTACK1) )
				{
					if ( GetActiveWeapon() || (CapabilitiesGet() & (bits_CAP_INNATE_MELEE_ATTACK1|bits_CAP_INNATE_MELEE_ATTACK2)))
						// if we can see enemy but can't use either attack type, we must need to get closer to enemy
						return SCHED_CHASE_ENEMY;
					else
						return SCHED_TAKE_COVER_FROM_ENEMY;
				}

				else
				{
					DevWarning( 2, "No suitable combat schedule!\n" );
				}
			}
			break;
		}
	case NPC_STATE_DEAD:
		{
			if ( BecomeRagdollOnClient( vec3_origin ) )
			{
				CleanupOnDeath();
				return SCHED_DIE_RAGDOLL;
			}

			// Adrian - Alread dead (by animation event maybe?)
			// Is it safe to set it to SCHED_NONE?
			if ( m_lifeState == LIFE_DEAD )
				 return SCHED_NONE;

			CleanupOnDeath();
			return SCHED_DIE;

			break;
		}
	case NPC_STATE_SCRIPT:
		{
			Assert( m_hCine != NULL );
			if ( !m_hCine )
			{
				DevWarning( 2, "Script failed for %s\n", GetClassname() );
				CineCleanup();
				return SCHED_IDLE_STAND;
			}

			return SCHED_AISCRIPT;
		}
	default:
		{
			DevWarning( 2, "Invalid State for SelectSchedule!\n" );
			break;
		}
	}

	return SCHED_FAIL;
}

//-----------------------------------------------------------------------------

int CAI_BaseNPC::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	return ( m_failSchedule != SCHED_NONE ) ? m_failSchedule : SCHED_FAIL;
}
