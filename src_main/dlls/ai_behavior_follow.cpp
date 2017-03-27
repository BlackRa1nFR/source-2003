//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#ifdef _WIN32
// using C++ standard list because CUtlLinkedList is not a suitable structure (memory moves)
#pragma warning(push)
#include <yvals.h>	// warnings get enabled in yvals.h 
#pragma warning(disable:4245)
#pragma warning(disable:4530)
#include <list> 	
using namespace std;
#pragma warning(pop)
#elif _LINUX
#undef time
#undef sprintf
#undef strncpy
#undef use_Q_snprintf_instead_of_sprintf
#undef use_Q_strncpy_instead
#undef use_Q_snprintf_instead
#include <string>
#include <time.h>
#include <list> 	
using namespace std;
#else
#include <list> 	
using namespace std;
#endif

#include "bitstring.h"
#include "utlvector.h"
#include "utllinkedlist.h"
#include "ai_navigator.h"
#include "scripted.h"
#include "ai_hint.h"
#include "ai_behavior_follow.h"
#include "ai_memory.h"
#include "npc_talker.h"
#include "ndebugoverlay.h"

//-----------------------------------------------------------------------------
//
// Purpose: Formation management
//
//			Right now, this is in a very preliminary sketch state. (toml 03-03-03)
//-----------------------------------------------------------------------------

struct AI_FollowSlot_t;
struct AI_FollowFormation_t;
struct AI_FollowGroup_t;

struct AI_Follower_t
{
	AI_Follower_t()
	{
		memset(this, 0, sizeof(*this) );
	}

	EHANDLE 			hFollower;
	int					slot;
	AI_FollowNavInfo_t	navInfo;
	AI_FollowGroup_t *	pGroup;	// backpointer for efficiency
};

struct AI_FollowGroup_t
{
	AI_FollowFormation_t *	pFormation;
	EHANDLE 				hFollowTarget;
	list<AI_Follower_t>		followers;
	CBitString				slotUsage;
};

typedef list<AI_Follower_t>::iterator FollowerListIter_t;

//-------------------------------------

class CAI_FollowManager
{
public:
	~CAI_FollowManager()
	{
		for ( int i = 0; i < m_groups.Count(); i++ )
			delete m_groups[i];
	}

	AI_FollowManagerInfoHandle_t AddFollower( CBaseEntity *pTarget, CAI_BaseNPC *pFollower );
	void RemoveFollower( AI_FollowManagerInfoHandle_t );
	bool CalcFollowPosition( AI_FollowManagerInfoHandle_t, AI_FollowNavInfo_t *pNavInfo );

	int CountFollowersInGroup( CAI_BaseNPC *pMember )
	{
		AI_FollowGroup_t *pGroup = FindFollowerGroup( pMember );

		if( !pGroup )
		{
			return 0;
		}

		return pGroup->followers.size();
	}

	int GetFollowerSlot( CAI_BaseNPC *pFollower )
	{
		AI_FollowGroup_t *pGroup = FindFollowerGroup( pFollower );

		if( !pGroup )
		{
			return 0;
		}

		FollowerListIter_t it = pGroup->followers.begin();

		while( it != pGroup->followers.end() )
		{
			if( it->hFollower.Get() == pFollower )
			{
				return it->slot;
			}

			it++;
		}

		return 0;
	}
	
private:
	int FindBestSlot( AI_FollowGroup_t *pGroup, CAI_BaseNPC *pFollower );
	void CalculateFieldsFromSlot( AI_FollowSlot_t *pSlot, AI_FollowNavInfo_t *pFollowerInfo );

	AI_FollowGroup_t *FindCreateGroup( CBaseEntity *pTarget );
	AI_FollowGroup_t *FindGroup( CBaseEntity *pTarget );
	AI_FollowGroup_t *FindFollowerGroup( CBaseEntity *pFollower );
	void RemoveGroup( AI_FollowGroup_t * );
	
	//---------------------------------
	
	CUtlVector<AI_FollowGroup_t *> m_groups;

};

//-------------------------------------

CAI_FollowManager g_AIFollowManager;

//-----------------------------------------------------------------------------
//
// CAI_FollowBehavior
//
//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( AI_FollowNavInfo_t )
	DEFINE_FIELD( AI_FollowNavInfo_t, position, FIELD_VECTOR ),
	DEFINE_FIELD( AI_FollowNavInfo_t, range, FIELD_FLOAT ),
	DEFINE_FIELD( AI_FollowNavInfo_t, tolerance, FIELD_FLOAT ),
	DEFINE_FIELD( AI_FollowNavInfo_t, followPointTolerance, FIELD_FLOAT ),
END_DATADESC();

BEGIN_DATADESC( CAI_FollowBehavior )
	DEFINE_FIELD( CAI_FollowBehavior, m_hFollowTarget, FIELD_EHANDLE ),
	DEFINE_EMBEDDED( CAI_FollowBehavior, m_FollowNavGoal ),
	DEFINE_EMBEDDED( CAI_FollowBehavior, m_TargetMonitor ),
	DEFINE_EMBEDDED( CAI_FollowBehavior, m_ShotRegulator ),
	DEFINE_EMBEDDED( CAI_FollowBehavior, m_WaitAfterFailedFollow ),
	DEFINE_FIELD( CAI_FollowBehavior, m_CurrentFollowActivity, FIELD_INTEGER ),
	DEFINE_EMBEDDED( CAI_FollowBehavior, m_TimeBlockUseWaitPoint ),
	DEFINE_EMBEDDED( CAI_FollowBehavior, m_TimeCheckForWaitPoint ),
	DEFINE_FIELD( CAI_FollowBehavior, m_pInterruptWaitPoint, FIELD_CLASSPTR ),
	DEFINE_EMBEDDED( CAI_FollowBehavior, m_TimeBeforeSpreadFacing )
	//									m_hFollowManagerInfo	(reset on load)
END_DATADESC();

//-------------------------------------

CAI_FollowBehavior::CAI_FollowBehavior()
{
	memset( &m_FollowNavGoal, 0, sizeof( m_FollowNavGoal ) );
	
	m_WaitAfterFailedFollow.Set( 1.0, 3.0 );
	m_hFollowManagerInfo = NULL;
	
	m_TimeBlockUseWaitPoint.Set( 0.5, 1.5 );
	m_TimeCheckForWaitPoint.Set( 0.5 );
	m_pInterruptWaitPoint = NULL;

	m_ShotRegulator.SetParameters( 2, 6, 0.3, 0.8 );

	m_TimeBeforeSpreadFacing.Set( 2.0, 3.0 );
}

//-------------------------------------

CBaseEntity * CAI_FollowBehavior::GetFollowTarget()
{ 
	return m_hFollowTarget;
}

//-------------------------------------

void CAI_FollowBehavior::SetFollowTarget( CBaseEntity *pLeader, bool fFinishCurSchedule ) 
{ 
	if ( pLeader == m_hFollowTarget )
		return;

	if ( m_hFollowTarget )
	{
		g_AIFollowManager.RemoveFollower( m_hFollowManagerInfo );
		m_hFollowTarget = NULL;
		m_hFollowManagerInfo = NULL;
		if ( IsRunning() )
		{
			if ( GetNavigator()->GetGoalType() == GOALTYPE_TARGETENT )
			{
				GetNavigator()->ClearGoal(); // Stop him from walking toward the player
			}
			if ( GetEnemy() != NULL )
				GetOuter()->m_IdealNPCState = NPC_STATE_COMBAT;
		}
	}

	if ( pLeader ) 
	{
		if ( ( m_hFollowManagerInfo = g_AIFollowManager.AddFollower( pLeader, GetOuter() ) ) != NULL )
		{
			m_hFollowTarget = pLeader;
		}
	}

	NotifyChangeBehaviorStatus(fFinishCurSchedule);
}

//-------------------------------------

bool CAI_FollowBehavior::UpdateFollowPosition()
{
	VPROF_BUDGET( "CAI_FollowBehavior::UpdateFollowPosition", VPROF_BUDGETGROUP_NPCS );

	if (m_hFollowTarget == NULL)
		return false;
	
	if ( !g_AIFollowManager.CalcFollowPosition( m_hFollowManagerInfo, &m_FollowNavGoal ) )
	{
		return false;
	}

#if TODO
	// @TODO (toml 07-27-03): this is too simplistic. fails when the new point is an inappropriate target
	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer *>(m_hFollowTarget.Get());
	Vector targetVelocity = pPlayer->GetSmoothedVelocity();
	m_FollowNavGoal.position += targetVelocity * 0.5;
#endif
	
	return true;
}

//-------------------------------------

bool CAI_FollowBehavior::CanSelectSchedule()
{
	if ( !GetOuter()->IsInterruptable() )
		return false;

	if ( GetOuter()->HasCondition( COND_RECEIVED_ORDERS ) )
		return false;

	return ShouldFollow();
}

//-------------------------------------

bool CAI_FollowBehavior::PlayerIsPushing()
{ 
	return (m_hFollowTarget && m_hFollowTarget->IsPlayer() && HasCondition( COND_PLAYER_PUSHING ) ); 
}

//-------------------------------------

void CAI_FollowBehavior::BeginScheduleSelection()
{
	if ( GetOuter()->m_hCine )
		GetOuter()->m_hCine->CancelScript();

	m_TimeBeforeSpreadFacing.Reset();
}

//-------------------------------------

void CAI_FollowBehavior::EndScheduleSelection()
{
	if ( GetOuter()->m_pHintNode )
	{
		GetOuter()->m_pHintNode->Unlock( 0.5 );
		GetOuter()->m_pHintNode = NULL;
	}
}

//-------------------------------------

void CAI_FollowBehavior::UpdateOnRemove()
{
	if ( m_hFollowManagerInfo )
	{
		g_AIFollowManager.RemoveFollower( m_hFollowManagerInfo );
		m_hFollowManagerInfo = NULL;
		m_hFollowTarget = NULL;
	}
	BaseClass::UpdateOnRemove();
}

//-------------------------------------

void CAI_FollowBehavior::Precache()
{
	if ( m_hFollowTarget != NULL && m_hFollowManagerInfo == NULL )
	{
		// Post load fixup
		if ( ( m_hFollowManagerInfo = g_AIFollowManager.AddFollower( m_hFollowTarget, GetOuter() ) ) == NULL )
		{
			m_hFollowTarget = NULL;
		}
	}
}

//-------------------------------------

void CAI_FollowBehavior::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if ( !m_TargetMonitor.IsMarkSet() )
		Msg( GetOuter(), "No mark set\n" );
		
	if ( m_WaitAfterFailedFollow.IsRunning() && m_WaitAfterFailedFollow.Expired())
		SetCondition( COND_WAIT_AFTER_FAILED_FOLLOW_EXPIRED );
	
	if ( m_TargetMonitor.TargetMoved( GetFollowTarget() ) )
	{
		Msg( GetOuter(), "Target moved\n" );
		m_TargetMonitor.ClearMark();
		Assert( !HasCondition( COND_TARGET_MOVED_FROM_MARK ) );
		SetCondition( COND_TARGET_MOVED_FROM_MARK );
	}

	m_pInterruptWaitPoint = NULL;

	if ( GetOuter()->m_pHintNode == NULL && ShouldUseFollowPoints() &&
		 m_TimeBlockUseWaitPoint.Expired() && m_TimeCheckForWaitPoint.Expired() )
	{
		m_TimeCheckForWaitPoint.Reset();
		m_pInterruptWaitPoint = FindFollowPoint();
		if ( m_pInterruptWaitPoint )
			SetCondition( COND_FOUND_WAIT_POINT );
	}

	m_ShotRegulator.Update();
}

//-------------------------------------

int CAI_FollowBehavior::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	m_TargetMonitor.ClearMark();
	Msg( GetOuter(), "Target cleared on fail\n" );
	
	if ( failedTask == TASK_MOVE_TO_FOLLOW_POSITION )
	{
		m_WaitAfterFailedFollow.Start();
	}
	
	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-------------------------------------

bool CAI_FollowBehavior::ShouldFollow()
{
	if ( !GetFollowTarget() )
		return false;
	
	return true;
}

//-------------------------------------

int CAI_FollowBehavior::SelectScheduleManagePosition()
{
	if ( PlayerIsPushing() )
		return SCHED_MOVE_AWAY_FOLLOW;

	if ( !UpdateFollowPosition() )
		return SCHED_FAIL;

	return SCHED_NONE;
}
	
//-------------------------------------

bool CAI_FollowBehavior::ShouldUseFollowPoints()
{
	if ( GetEnemy() != NULL )
		return false;

	return IsFollowGoalInRange( m_FollowNavGoal.followPointTolerance + 0.1 );
}

//-------------------------------------

CAI_Hint *CAI_FollowBehavior::FindFollowPoint()
{
	if ( !m_TimeBlockUseWaitPoint.Expired() )
		return NULL;

	const Vector &followPos = GetGoalPosition();
	return CAI_Hint::FindHint( GetOuter(), 
							   HINT_FOLLOW_WAIT_POINT, 
							   bits_HINT_NODE_VISIBLE | bits_HINT_NODE_NEAREST, 
							   max( m_FollowNavGoal.followPointTolerance, GetGoalRange() ), 
							   &followPos );
}

//-------------------------------------

int CAI_FollowBehavior::SelectScheduleFollowPoints()
{
	bool bShouldUseFollowPoints = ShouldUseFollowPoints();
	CAI_Hint *&m_pHintNode=GetOuter()->m_pHintNode;

	if ( m_pHintNode )
	{
		if ( !bShouldUseFollowPoints || (m_pHintNode->GetAbsOrigin() - GetAbsOrigin()).Length() > 2.0 * GetHullWidth() )
		{
			m_pHintNode->Unlock();
			m_pHintNode = NULL;
			m_TimeBlockUseWaitPoint.Reset();
		}
	}

	if ( bShouldUseFollowPoints )
	{
		if (!m_pHintNode)
		{
			m_pHintNode = ( m_pInterruptWaitPoint ) ? m_pInterruptWaitPoint : FindFollowPoint();
			
			if ( m_pHintNode )
			{
				if ( !m_pHintNode->Lock( GetOuter() ) )
				{
					m_pHintNode = NULL;
					m_TimeBlockUseWaitPoint.Reset();
				}
			}
		}
		if ( m_pHintNode )
			return SCHED_FOLLOWER_GO_TO_WAIT_POINT;
	}
	
	return SCHED_NONE;
}

//-------------------------------------

int CAI_FollowBehavior::SelectScheduleMoveToFormation()
{
	if( ( GetNpcState() != NPC_STATE_COMBAT	&& !( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ))) ||
		!IsFollowGoalInRange( GetGoalRange() ) )
	{
		return SCHED_TARGET_FACE; // Code for "SCHED_MOVE_TO_FACE_FOLLOW_TARGET". Used by Talker clients to interject comment
	}
	return SCHED_NONE;
}

//-------------------------------------

int CAI_FollowBehavior::SelectSchedule()
{
	if ( GetFollowTarget() )
	{
		if ( !GetFollowTarget()->IsAlive() )
		{
			// UNDONE: Comment about the recently dead player here?
			SetFollowTarget( NULL );
		}
		else if ( ShouldFollow() )
		{
			int result = SCHED_NONE;
			
			result = SelectScheduleManagePosition();
			if ( result != SCHED_NONE )
				return result;
			
			result = SelectScheduleFollowPoints();
			if ( result != SCHED_NONE )
				return result;
				
			result = SelectScheduleMoveToFormation();
			if ( result != SCHED_NONE )
				return result;
				
			if ( HasCondition ( COND_NO_PRIMARY_AMMO ) && HaveSequenceForActivity( GetOuter()->TranslateActivity( ACT_RUN_AIM ) ) )
				return SCHED_HIDE_AND_RELOAD;
		}

		if ( PlayerIsPushing() )	
			return SCHED_MOVE_AWAY;
	}
	else
	{
		// Should not have landed here. Follow target ent must have been destroyed
		NotifyChangeBehaviorStatus();
	}
	
	return BaseClass::SelectSchedule();
}

//-------------------------------------

int CAI_FollowBehavior::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
		case SCHED_IDLE_STAND:
		{
			return SCHED_FOLLOWER_IDLE_STAND;
		}

		case SCHED_TARGET_FACE:
		{
			if ( ( !m_WaitAfterFailedFollow.IsRunning() || m_WaitAfterFailedFollow.Expired() ) && !IsFollowGoalInRange( GetGoalRange() )  )
			{
				m_WaitAfterFailedFollow.Stop();
				return SCHED_MOVE_TO_FACE_FOLLOW_TARGET;			
			}
			return SCHED_FACE_FOLLOW_TARGET; // @TODO (toml 03-03-03): should select a facing sched
		}

		case SCHED_TARGET_CHASE:
		{
			return SCHED_FOLLOW;
		}

		case SCHED_CHASE_ENEMY:
		{
			return SCHED_FOLLOWER_IDLE_STAND;
		}

		case SCHED_RANGE_ATTACK1:
		{
			if ( !m_ShotRegulator.ShouldShoot() )				
				return SCHED_FOLLOWER_IDLE_STAND; // @TODO (toml 07-02-03): Should do something more tactically sensible
			break;
		}

		case SCHED_CHASE_ENEMY_FAILED:
		{
			if (HasMemory(bits_MEMORY_INCOVER))
			{
				// Make sure I don't get too far from the player
				if ( GetFollowTarget() )
				{
					float fDist = (GetLocalOrigin() - GetFollowTarget()->GetAbsOrigin()).Length();
					if (fDist > 500)
					{
						return SCHED_FOLLOW;
					}
				}
			}
			break;
		}

		case CAI_PlayerAlly::SCHED_TALKER_RETURN_FROM_HEALTHKIT:
		{
			// Don't run back to our savedposition, instead just immediately return to the player
			return SCHED_FOLLOW;
		}
	}
	return BaseClass::TranslateSchedule( scheduleType );
}

//-------------------------------------

void CAI_FollowBehavior::StartTask( const Task_t *pTask )
{
	VPROF_BUDGET( "CAI_FollowBehavior::StartTask", VPROF_BUDGETGROUP_NPCS );

	switch ( pTask->iTask )
	{
		case TASK_RANGE_ATTACK1:
		{
			m_ShotRegulator.OnFiredWeapon();
			BaseClass::StartTask( pTask );
			break;
		}

		case TASK_GET_PATH_TO_FOLLOW_POSITION:
		{
			if ( !UpdateFollowPosition() )
			{
				TaskFail(FAIL_NO_TARGET);
			}
			
			AI_NavGoal_t goal( GetGoalPosition(), AIN_DEF_ACTIVITY, GetGoalTolerance() );
			goal.pTarget = m_hFollowTarget;
			GetNavigator()->SetGoal( goal );
			
			break;
		}
		
		case TASK_CANT_FOLLOW:
		{
			SetFollowTarget( NULL, true );
			TaskComplete();
			break;
		}

		case TASK_FOLLOW_WALK_PATH_FOR_UNITS:
		{
			GetNavigator()->SetMovementActivity(ACT_WALK);
			break;
		}
			
		case TASK_FOLLOWER_FACE_TACTICAL:
		{
		}

		case TASK_FACE_FOLLOW_TARGET:
		{
			Vector faceTarget = vec3_origin;
			if( GetNpcState() == NPC_STATE_COMBAT )
			{
				if( gpGlobals->curtime - GetOuter()->GetEnemyLastTimeSeen() < 5.0 )
				{
					faceTarget = GetEnemyLKP();
				}
				else
				{
					trace_t tr;
					Vector vecStart, vecDir;

					ASSERT( m_hFollowTarget != NULL );

					vecStart = m_hFollowTarget->EyePosition();

					CBasePlayer *pPlayer;

					pPlayer = dynamic_cast<CBasePlayer *>(m_hFollowTarget.Get());

					if( pPlayer )
					{
						// Follow target is a player.
						pPlayer->EyeVectors( &vecDir, NULL, NULL );
					}
					else
					{
						// Not a player. 
						m_hFollowTarget->GetVectors( &vecDir, NULL, NULL );
					}

					AI_TraceLine( vecStart, vecStart + vecDir * 8192, MASK_OPAQUE, m_hFollowTarget, COLLISION_GROUP_NONE, &tr );


					faceTarget = tr.endpos;
				}
			}
			else if( m_TimeBeforeSpreadFacing.Expired() && ( GetNpcState() == NPC_STATE_IDLE || GetNpcState() == NPC_STATE_ALERT ) )
			{
				// Fan out and face to cover all directions.
				int count = g_AIFollowManager.CountFollowersInGroup( GetOuter() );

				if( count > 0 )
				{
					// Slice up the directions among followers and leader. ( +1 because we count the leader!)
					float flSlice = 360.0 / (count + 1);

					// Add one to slots so then are 1 to N instead of 0 to N - 1.
					int slot = g_AIFollowManager.GetFollowerSlot( GetOuter() ) + 1;

					QAngle angle = m_hFollowTarget->GetAbsAngles();

					// split up the remaining angles among followers in my group.
					angle.y = UTIL_AngleMod( angle.y + flSlice * slot );

					Vector vecDir;
					AngleVectors( angle, &vecDir );

					faceTarget = GetOuter()->GetAbsOrigin() + vecDir * 128;
				}
			}
			else if ( m_hFollowTarget != NULL )
			{
				faceTarget = m_hFollowTarget->GetAbsOrigin();
			}

			if ( faceTarget != vec3_origin )
			{
				if ( !GetOuter()->FInAimCone( faceTarget ) )
				{
					GetMotor()->SetIdealYawToTarget( faceTarget );
					GetOuter()->SetTurnActivity(); 
				}
				else
					TaskComplete();
			}
			else
				TaskFail(FAIL_NO_TARGET);

			break;
		}
			
		case TASK_MOVE_TO_FOLLOW_POSITION:
		{
			if ( m_hFollowTarget == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else if ( (m_hFollowTarget->GetAbsOrigin() - GetAbsOrigin()).Length() < 1 )
			{
				TaskComplete();
				m_TimeBeforeSpreadFacing.Reset();
			}
			else
			{
				m_CurrentFollowActivity = ACT_INVALID;
			}
			break;
		}
		
		case TASK_SET_FOLLOW_TARGET_MARK:
		{
			if ( m_hFollowTarget == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				Msg( GetOuter(), "TASK_SET_FOLLOW_TARGET_MARK\n" );
				m_TargetMonitor.SetMark( m_hFollowTarget, pTask->flTaskData );
				TaskComplete();
			}
			break;
		}
			
		default:
			BaseClass::StartTask( pTask );
	}
}

//-------------------------------------

void CAI_FollowBehavior::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
		case TASK_GET_PATH_TO_FOLLOW_POSITION:
		{
			AssertMsg( 0, "TASK_GET_PATH_TO_FOLLOW_POSITION expected to resolve in StartTask()" );
			break;
		}
			
		case TASK_FOLLOW_WALK_PATH_FOR_UNITS:
		{
			float distance;

			distance = (GetOuter()->m_vecLastPosition - GetLocalOrigin()).Length2D();

			// Walk path until far enough away
			if ( distance > pTask->flTaskData || 
				 GetNavigator()->GetGoalType() == GOALTYPE_NONE )
			{
				TaskComplete();
				GetNavigator()->ClearGoal();		// Stop moving
			}
			break;
		}
			
		case TASK_FOLLOWER_FACE_TACTICAL:
		case TASK_FACE_FOLLOW_TARGET:
		{
			GetMotor()->UpdateYaw();

			if ( GetOuter()->FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}

		case TASK_MOVE_TO_FOLLOW_POSITION:
		{
			if ( m_hFollowTarget == NULL )
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				const float TARGET_MOVE_THRESH = 60;
				
				// Re-evaluate when you think your finished, or the target has moved too far
				if ( !UpdateFollowPosition() )
				{
					TaskFail(FAIL_NO_TARGET);
				}

				if ( IsFollowGoalInRange( GetGoalRange() ) )
				{
					m_TimeBeforeSpreadFacing.Reset();
					TaskComplete();
					GetNavigator()->ClearGoal();		// Stop moving
					return;
				}

				float distance = ( GetNavigator()->GetGoalPos() - GetLocalOrigin() ).Length2D();
				const Vector &followPos = GetGoalPosition();
				
				// Update the nav goal if needed
				if ( ( distance < GetGoalRange() ) || (GetNavigator()->GetGoalPos() - followPos).Length() > TARGET_MOVE_THRESH )
				{
					distance = ( followPos - GetLocalOrigin() ).Length2D();
					GetNavigator()->UpdateGoalPos( followPos );
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				// BUGBUG: this is checking linear distance (ie. through walls) and not path distance or even visibility

				// Pick the right movement activity.
				Activity followActivity = GetOuter()->GetFollowActivity( distance );

				// Never stop running once started
				if ( m_CurrentFollowActivity != ACT_RUN && followActivity != m_CurrentFollowActivity )
				{
					m_CurrentFollowActivity = followActivity;
					GetNavigator()->SetMovementActivity(followActivity);
					GetOuter()->SetIdealActivity(followActivity);
				}
			}
			break;
		}
			
		default:
			BaseClass::RunTask( pTask );
	}
}

//-------------------------------------

bool CAI_FollowBehavior::IsCurTaskContinuousMove()
{
	const Task_t *pCurTask = GetCurTask();
	if ( pCurTask && pCurTask->iTask == TASK_MOVE_TO_FOLLOW_POSITION )
		return true;
	return BaseClass::IsCurTaskContinuousMove();
}

//-------------------------------------

bool CAI_FollowBehavior::FValidateHintType( CAI_Hint *pHint )
{
	return ( pHint->HintType() == HINT_FOLLOW_WAIT_POINT );
}

//-------------------------------------

bool CAI_FollowBehavior::IsValidEnemy(CBaseEntity *pEnemy)
{
	if ( !BaseClass::IsValidEnemy( pEnemy ) )
		return false;
	if ( gpGlobals->curtime - GetOuter()->GetEnemies()->LastTimeSeen( pEnemy ) > 5.0 )
		return false;
	return true;
}

//-------------------------------------

bool CAI_FollowBehavior::ShouldAlwaysThink()
{
	return true;
}


//-----------------------------------------------------------------------------
//
// CAI_FollowGoal
//
// Purpose: A level tool to control the follow behavior. Use is not required
//			in order to use behavior.
//
//-----------------------------------------------------------------------------

class CAI_FollowGoal : public CAI_GoalEntity
{
	DECLARE_CLASS( CAI_FollowGoal, CAI_GoalEntity );

	virtual void EnableGoal( CAI_BaseNPC *pAI );
	virtual void DisableGoal( CAI_BaseNPC *pAI  );
};

//-------------------------------------

LINK_ENTITY_TO_CLASS( ai_goal_follow, CAI_FollowGoal );

//-------------------------------------

void CAI_FollowGoal::EnableGoal( CAI_BaseNPC *pAI )
{
	CAI_FollowBehavior *pBehavior;
	if ( !pAI->GetBehavior( &pBehavior ) )
		return;
	
	CBaseEntity *pGoalEntity = GetGoalEntity();
	if ( !pGoalEntity )
	{
		if ( pAI->IRelationType(UTIL_PlayerByIndex( 1 )) == D_LI )
			pBehavior->SetFollowTarget( UTIL_PlayerByIndex( 1 ) );
	}
	else
	{
		pBehavior->SetFollowTarget( pGoalEntity );
	}
}

//-------------------------------------

void CAI_FollowGoal::DisableGoal( CAI_BaseNPC *pAI  )
{ 
	CAI_FollowBehavior *pBehavior;
	if ( !pAI->GetBehavior( &pBehavior ) )
		return;
	
	pBehavior->SetFollowTarget( NULL );
}

//-----------------------------------------------------------------------------
//
// CAI_FollowManager
//
//-----------------------------------------------------------------------------

//-------------------------------------
//
// Purpose: Formation definitions
//

struct AI_FollowSlot_t
{
	int 		priority;

	TableVector position;
	float		positionVariability;
	
	float		rangeMin;
	float		rangeMax;
	
	float		tolerance;
	// @Q (toml 02-28-03): facing?
};

struct AI_FollowFormation_t
{
	const char *		pszName;
	unsigned 			flags;
	int					nSlots;
	float				followPointTolerance;
	AI_FollowSlot_t *	pSlots;
};

enum AI_FollowFormationFlags_t
{
	AIFF_DEFAULT 			= 0,
	AIFF_USE_FOLLOW_POINTS 	= 0x01,
};

//-------------------------------------

static AI_FollowSlot_t g_SimpleFollowFormationSlots[] = 
{
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
	{ 1, { 0, 0, 0 }, 0, 96, 120, AIN_DEF_TOLERANCE },
};

static AI_FollowFormation_t g_SimpleFollowFormation = 
{
	"Simple",
	AIFF_DEFAULT | AIFF_USE_FOLLOW_POINTS,
	ARRAYSIZE(g_SimpleFollowFormationSlots),
	168,
	g_SimpleFollowFormationSlots
};

//-------------------------------------

AI_FollowFormation_t *g_AI_Formations[] =
{
	&g_SimpleFollowFormation
};

enum AI_Formations_t
{
	AIF_SIMPLE
};

AI_FollowFormation_t *AIGetFormation( AI_Formations_t formation )
{
	return g_AI_Formations[formation];
}

//---------------------------------------------------------

ASSERT_INVARIANT( sizeof(FollowerListIter_t) == sizeof(AI_FollowManagerInfoHandle_t) );

AI_FollowManagerInfoHandle_t CAI_FollowManager::AddFollower( CBaseEntity *pTarget, CAI_BaseNPC *pFollower )
{
	AI_FollowGroup_t *pGroup = FindCreateGroup( pTarget );
	int slot = FindBestSlot( pGroup, pFollower );

	if ( slot != -1 )
	{
		AI_FollowSlot_t *pSlot 		= &pGroup->pFormation->pSlots[slot];
		FollowerListIter_t iterNode = pGroup->followers.insert(pGroup->followers.end());

		iterNode->hFollower 		= pFollower;
		iterNode->slot 				= slot;
		iterNode->pGroup			= pGroup;

		pGroup->slotUsage.SetBit( slot );
		
		CalculateFieldsFromSlot( pSlot, &iterNode->navInfo );
		
		return *((AI_FollowManagerInfoHandle_t *)((void *)&iterNode));
	}
	return NULL;
}

//-------------------------------------

bool CAI_FollowManager::CalcFollowPosition( AI_FollowManagerInfoHandle_t hInfo, AI_FollowNavInfo_t *pNavInfo )
{
	FollowerListIter_t iterNode = *((FollowerListIter_t *)(&hInfo));
	if ( hInfo && iterNode->pGroup )
	{
		Assert( iterNode->pGroup->hFollowTarget.Get() );
		CBaseEntity *pTarget = iterNode->pGroup->hFollowTarget;
		if ( iterNode->navInfo.position != vec3_origin )
		{
			QAngle angles = pTarget->GetLocalAngles();
			angles.x = angles.z = 0;
				
			matrix3x4_t fRotateMatrix;
			AngleMatrix(angles, fRotateMatrix);
			
			VectorRotate( iterNode->navInfo.position, fRotateMatrix, pNavInfo->position);
			pNavInfo->position += pTarget->WorldSpaceCenter();
		}
		else
		{
			pNavInfo->position = iterNode->navInfo.position + pTarget->WorldSpaceCenter();
		}
			
		pNavInfo->tolerance 			= iterNode->navInfo.tolerance;
		pNavInfo->range 				= iterNode->navInfo.range;
		pNavInfo->followPointTolerance 	= iterNode->pGroup->pFormation->followPointTolerance;
		return true;
	}		
	return false;
}

//-------------------------------------

void CAI_FollowManager::RemoveFollower( AI_FollowManagerInfoHandle_t hInfo )
{
	FollowerListIter_t iterNode = *((FollowerListIter_t *)(&hInfo));
	if ( hInfo && iterNode->pGroup )
	{
		AI_FollowGroup_t *pGroup = iterNode->pGroup;
		pGroup->slotUsage.ClearBit( iterNode->slot );
		pGroup->followers.erase( iterNode );
		if ( pGroup->followers.size() == 0 )
		{
			RemoveGroup( pGroup );
		}
	}		
}

//-------------------------------------

int CAI_FollowManager::FindBestSlot( AI_FollowGroup_t *pGroup, CAI_BaseNPC *pFollower )
{
	// @TODO (toml 02-28-03): crude placeholder
	int nSlots = pGroup->pFormation->nSlots;
	
	for ( int i = 0; i < nSlots; i++ )
	{
		if ( !pGroup->slotUsage.GetBit( i ) )
		{
			return i;
		}
	}
	return -1;
}

//-------------------------------------

void CAI_FollowManager::CalculateFieldsFromSlot( AI_FollowSlot_t *pSlot, AI_FollowNavInfo_t *pFollowerInfo )
{
	// @TODO (toml 02-28-03): placeholder. Force break if someone tries to actually use
	Assert( pSlot->priority == 1 );
	Assert( pSlot->positionVariability == 0.0 );
	Assert( pSlot->tolerance == AIN_DEF_TOLERANCE );

	pFollowerInfo->position		= pSlot->position;
	pFollowerInfo->range 		= random->RandomFloat( pSlot->rangeMin, pSlot->rangeMax );
	pFollowerInfo->tolerance 	= pSlot->tolerance;
}

//-------------------------------------

AI_FollowGroup_t *CAI_FollowManager::FindCreateGroup( CBaseEntity *pTarget )
{
	AI_FollowGroup_t *pGroup = FindGroup( pTarget );
	
	if ( !pGroup )
	{
		pGroup = new AI_FollowGroup_t;
		
		pGroup->pFormation = AIGetFormation( AIF_SIMPLE );
		pGroup->slotUsage.Resize( pGroup->pFormation->nSlots );
		pGroup->hFollowTarget = pTarget;
		
		m_groups.AddToHead( pGroup );
	}
	
	return pGroup;
}

//-------------------------------------

void CAI_FollowManager::RemoveGroup( AI_FollowGroup_t *pGroup )
{
	for ( int i = 0; i < m_groups.Count(); i++ )
	{
		if ( m_groups[i] == pGroup )
		{
			delete m_groups[i];
			m_groups.FastRemove(i);
			return;
		}
	}
}

//-------------------------------------

AI_FollowGroup_t *CAI_FollowManager::FindGroup( CBaseEntity *pTarget )
{
	for ( int i = 0; i < m_groups.Count(); i++ )
	{
		if ( m_groups[i]->hFollowTarget == pTarget )
			return m_groups[i];
	}
	return NULL;
}

//-------------------------------------

AI_FollowGroup_t *CAI_FollowManager::FindFollowerGroup( CBaseEntity *pFollower )
{
	for ( int i = 0; i < m_groups.Count(); i++ )
	{
		FollowerListIter_t it = m_groups[i]->followers.begin();

		while ( it != m_groups[i]->followers.end() )
		{
			if ( it->hFollower.Get() == pFollower )
				return m_groups[i];
			it++;
		}
	}
	return NULL;
}
	
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER(CAI_FollowBehavior)

	DECLARE_TASK(TASK_FOLLOW_WALK_PATH_FOR_UNITS)
	DECLARE_TASK(TASK_CANT_FOLLOW)
	DECLARE_TASK(TASK_FACE_FOLLOW_TARGET)
	DECLARE_TASK(TASK_MOVE_TO_FOLLOW_POSITION)
	DECLARE_TASK(TASK_GET_PATH_TO_FOLLOW_POSITION)
	DECLARE_TASK(TASK_SET_FOLLOW_TARGET_MARK)
	DECLARE_TASK(TASK_FOLLOWER_FACE_TACTICAL)

	DECLARE_CONDITION(COND_TARGET_MOVED_FROM_MARK)
	DECLARE_CONDITION(COND_FOUND_WAIT_POINT)
	DECLARE_CONDITION(COND_WAIT_AFTER_FAILED_FOLLOW_EXPIRED)
	
	//=========================================================
	// > SCHED_MOVE_AWAY
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_MOVE_AWAY,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_MOVE_AWAY_FAIL "
		"		 TASK_STORE_LASTPOSITION				0"
		"		 TASK_MOVE_AWAY_PATH					100"
		"		 TASK_FOLLOW_WALK_PATH_FOR_UNITS		100"
		"		 TASK_STOP_MOVING						0"
		"		 TASK_FACE_FOLLOW_TARGET				0"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_MOVE_AWAY_FAIL
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_MOVE_AWAY_FAIL,

		"	Tasks"
		"		 TASK_STOP_MOVING						0"
		"		 TASK_FACE_FOLLOW_TARGET				0"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_MOVE_AWAY_FOLLOW
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_MOVE_AWAY_FOLLOW,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_TARGET_FACE"
		"		 TASK_STORE_LASTPOSITION				0"
		"		 TASK_MOVE_AWAY_PATH					100"
		"		 TASK_FOLLOW_WALK_PATH_FOR_UNITS		100"
		"		 TASK_STOP_MOVING						0"
		"		 TASK_SET_SCHEDULE						SCHEDULE:SCHED_TARGET_FACE"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_FOLLOW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_FOLLOW,

		"	Tasks"
		"		TASK_GET_PATH_TO_FOLLOW_POSITION 0"
		"		TASK_MOVE_TO_FOLLOW_POSITION	0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_TARGET_FACE "
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PROVOKED"
		"		COND_PLAYER_PUSHING"
	);

	//=========================================================
	// > SCHED_MOVE_TO_FACE_FOLLOW_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_MOVE_TO_FACE_FOLLOW_TARGET,

		"	Tasks"
//		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
//		"		TASK_FACE_FOLLOW_TARGET				0"
//		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_FOLLOW"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PROVOKED"
		"		COND_PLAYER_PUSHING"
	)
	
	//=========================================================
	// > SCHED_FACE_FOLLOW_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_FACE_FOLLOW_TARGET,

		"	Tasks"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_FACE_FOLLOW_TARGET				0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_FOLLOWER_IDLE_STAND "
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PROVOKED"
		"		COND_PLAYER_PUSHING"
	)

	//=========================================================
	// > SCHED_FOLLOWER_GO_TO_WAIT_POINT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_FOLLOWER_GO_TO_WAIT_POINT,

		"	Tasks"
		"		TASK_LOCK_HINTNODE			0		" // this will fail the schedule if no hint node or not already lockable
		"		TASK_GET_PATH_TO_HINTNODE	0"
		"		TASK_SET_FOLLOW_TARGET_MARK 36"
		"		TASK_WALK_PATH				0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_FACE_HINTNODE			0"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_FOLLOWER_STAND_AT_WAIT_POINT "
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PROVOKED"
		"		COND_PLAYER_PUSHING"
		"		COND_TARGET_MOVED_FROM_MARK"
	)

	//=========================================================
	// > SCHED_FOLLOWER_STAND_AT_WAIT_POINT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_FOLLOWER_STAND_AT_WAIT_POINT,

		"	Tasks"
		"		TASK_FACE_HINTNODE			0"
		"		TASK_PLAY_HINT_ACTIVITY		0"
		"		TASK_WAIT					30"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_FOLLOWER_STAND_AT_WAIT_POINT "
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PROVOKED"
		"		COND_PLAYER_PUSHING"
		"		COND_TARGET_MOVED_FROM_MARK"
		
	)

	DEFINE_SCHEDULE
	(
		SCHED_FOLLOWER_IDLE_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_SET_FOLLOW_TARGET_MARK		36"
		"		TASK_WAIT						2.5"
		"		TASK_FACE_FOLLOW_TARGET			0"
		"		TASK_WAIT						3"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SMELL"
		"		COND_PROVOKED"
		"		COND_GIVE_WAY"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_BULLET_IMPACT"
		"		COND_PLAYER_PUSHING"
		"		COND_TARGET_MOVED_FROM_MARK"
		"		COND_WAIT_AFTER_FAILED_FOLLOW_EXPIRED"
		"		COND_FOUND_WAIT_POINT"
	)

AI_END_CUSTOM_SCHEDULE_PROVIDER()

//=============================================================================
