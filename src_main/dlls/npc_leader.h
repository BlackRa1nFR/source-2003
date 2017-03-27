//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	NPCLEADER_H
#define	NPCLEADER_H
#pragma once

#if 0
#include "npc_talker.h"


//=============================================================================
// >> CNPCLeader
//=============================================================================
class CNPCLeader : public CNPCSimpleTalker
{
	DECLARE_CLASS( CNPCLeader, CNPCSimpleTalker );

public:
	void			LeaderUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void			GatherConditions( void );
	void			PrescheduleThink( void );
	int				SelectSchedule ( void );
	bool			IsLeading( void ) { return m_hFollower != NULL && m_hFollower->IsPlayer(); }
	bool			FollowerOnLeft( void );
	void			StopLeading( bool clearSchedule );
	void			StartLeading( CBaseEntity *pFollower, CBaseEntity *pDest );

	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );

	DEFINE_CUSTOM_AI;

	DECLARE_DATADESC();

	EHANDLE m_hGoalEnt;			// where we're leading to
	EHANDLE m_hFollower;		// who we are leading
	float m_flFollowerDist;		// how far away the follower is. 

	//=========================================================
	// NPCLeader schedules
	//=========================================================
	enum
	{
		SCHED_LEADER_LEAD = BaseClass::NEXT_SCHEDULE,
		SCHED_LEADER_BACKTRACK,
		SCHED_LEADER_GET_CLOSER,
		SCHED_LEADER_WAIT,

		// !ALWAYS LAST!
		LAST_NPCLeader_SCHEDULE
	};

	//=========================================================
	// NPCLeader tasks
	//=========================================================
	enum
	{
		TASK_LEADER_GET_PATH_TO_GOAL = BaseClass::NEXT_TASK,
		TASK_LEADER_GET_PATH_TO_FOLLOWER,
		TASK_LEADER_WALK_PATH_TO_FOLLOWER,
		TASK_LEADER_FACE_FOLLOWER,
		TASK_LEADER_WAIT_FOLLOWER_NEAR,
		TASK_LEADER_ANNOUNCE_BACKTRACK,
		TASK_LEADER_ANNOUNCE_END_TOUR,
		TASK_LEADER_REMIND_FOLLOWER,
		TASK_LEADER_STOP_LEADING,
		
		// !ALWAYS LAST!
		LAST_NPCLeader_TASK
	};

	//=========================================================
	// NPCLeader Conditions
	//=========================================================
	enum NPCLeaderConditions
	{
		COND_LEADER_FOLLOWERNEAR = BaseClass::NEXT_CONDITION,
		COND_LEADER_FOLLOWERVERYFAR,
		COND_LEADER_FOLLOWERVISIBLE,
		COND_LEADER_FOLLOWERLAGGING,
		COND_LEADER_FOLLOWERLOST,
		COND_LEADER_FOLLOWERFOUND,
		COND_LEADER_FOLLOWERCAUGHTUP,

		// !ALWAYS LAST!
		LAST_NPCLeader_CONDITION
	};
};

#endif

#endif // NPCLEADER_H
