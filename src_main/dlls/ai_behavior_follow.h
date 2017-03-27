//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#ifndef AI_BEHAVIOR_FOLLOW_H
#define AI_BEHAVIOR_FOLLOW_H

#include "simtimer.h"
#include "ai_behavior.h"
#include "ai_goalentity.h"
#include "ai_utils.h"
#include "ai_moveshoot.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------

struct AI_FollowNavInfo_t
{
	Vector  position;
	float 	range;
	float   tolerance;
	float   followPointTolerance;

	DECLARE_SIMPLE_DATADESC();
};

DECLARE_POINTER_HANDLE(AI_FollowManagerInfoHandle_t);

//-------------------------------------

class CAI_FollowBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_FollowBehavior, CAI_SimpleBehavior );
public:
	CAI_FollowBehavior();

	virtual const char *GetName() {	return "Follow"; }

	virtual bool 	CanSelectSchedule();

	CBaseEntity *	GetFollowTarget();
	void			SetFollowTarget( CBaseEntity *pLeader, bool fFinishCurSchedule = false );

	bool			FarFromFollowTarget()	{ return ( m_hFollowTarget && (GetAbsOrigin() - m_hFollowTarget->GetAbsOrigin()).LengthSqr() > (75*12)*(75*12) ); }

private:

	virtual void	BeginScheduleSelection();
	virtual void	EndScheduleSelection();
	
	virtual void	UpdateOnRemove();

	virtual void	Precache();
	virtual void 	GatherConditions();
	virtual int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual int		SelectSchedule();
	virtual int		TranslateSchedule( int scheduleType );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );

	virtual bool	IsCurTaskContinuousMove();
	virtual bool	FValidateHintType( CAI_Hint *pHint );
	
	bool			IsValidEnemy(CBaseEntity *pEnemy);
	bool			ShouldAlwaysThink();

	int 			SelectScheduleManagePosition();
	int 			SelectScheduleFollowPoints();
	int 			SelectScheduleMoveToFormation();
	
	//----------------------------

	bool			ShouldUseFollowPoints();
	CAI_Hint *		FindFollowPoint();

	//----------------------------
	
	bool			ShouldFollow();
	bool 			UpdateFollowPosition();
	float			GetGoalRange();
	const Vector &	GetGoalPosition();
	float 			GetGoalTolerance();
	bool			PlayerIsPushing();

	bool			IsFollowGoalInRange( float tolerance );
	
	//----------------------------

	enum
	{
		SCHED_MOVE_AWAY = BaseClass::NEXT_SCHEDULE,		// Try to get out of the player's way
		SCHED_MOVE_AWAY_FOLLOW,							// same, but follow afterward
		SCHED_MOVE_AWAY_FAIL,							// Turn back toward player
		SCHED_FOLLOW,
		SCHED_FOLLOWER_IDLE_STAND,
		SCHED_MOVE_TO_FACE_FOLLOW_TARGET,
		SCHED_FACE_FOLLOW_TARGET,
		SCHED_FOLLOWER_GO_TO_WAIT_POINT,
		SCHED_FOLLOWER_STAND_AT_WAIT_POINT,
		NEXT_SCHEDULE,

		TASK_FOLLOW_WALK_PATH_FOR_UNITS = BaseClass::NEXT_TASK,
		TASK_CANT_FOLLOW,
		TASK_FACE_FOLLOW_TARGET,
		TASK_MOVE_TO_FOLLOW_POSITION,
		TASK_GET_PATH_TO_FOLLOW_POSITION,
		TASK_SET_FOLLOW_TARGET_MARK,
		TASK_FOLLOWER_FACE_TACTICAL,
		NEXT_TASK,

		COND_TARGET_MOVED_FROM_MARK = BaseClass::NEXT_CONDITION,
		COND_FOUND_WAIT_POINT,
		COND_WAIT_AFTER_FAILED_FOLLOW_EXPIRED,
		NEXT_CONDITION,
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
	
	//----------------------------
	
	EHANDLE 		   				m_hFollowTarget;
	AI_FollowNavInfo_t 				m_FollowNavGoal;
	
	CAI_MoveMonitor	   				m_TargetMonitor;

	CAI_ShotRegulator				m_ShotRegulator;
	
	CRandStopwatch	   				m_WaitAfterFailedFollow;
	
	//---------------------------------

	Activity						m_CurrentFollowActivity;

	//---------------------------------
	
	CRandSimTimer					m_TimeBlockUseWaitPoint;
	CSimTimer						m_TimeCheckForWaitPoint;
	CAI_Hint *						m_pInterruptWaitPoint;
	
	//---------------------------------

	CRandSimTimer					m_TimeBeforeSpreadFacing;

	//---------------------------------
	
	AI_FollowManagerInfoHandle_t 	m_hFollowManagerInfo;
	
	//---------------------------------
	
	DECLARE_DATADESC();
};

//-------------------------------------

inline const Vector &CAI_FollowBehavior::GetGoalPosition()
{
	return m_FollowNavGoal.position;
}

//-------------------------------------

inline float CAI_FollowBehavior::GetGoalTolerance()
{
	return m_FollowNavGoal.tolerance;
}

//-------------------------------------

inline float CAI_FollowBehavior::GetGoalRange()
{
	return m_FollowNavGoal.range;
}

//-------------------------------------

inline bool CAI_FollowBehavior::IsFollowGoalInRange( float tolerance )
{
	const Vector origin = WorldSpaceCenter();
	const Vector &goal = GetGoalPosition();
	if ( fabs( origin.z - goal.z ) > GetHullHeight() )
		return false;
	float distanceSq = ( goal.AsVector2D() - origin.AsVector2D() ).LengthSqr();

	return ( distanceSq < tolerance*tolerance );
}
//-----------------------------------------------------------------------------

#endif // AI_BEHAVIOR_FOLLOW_H
