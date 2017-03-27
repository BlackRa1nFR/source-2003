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
// $NoKeywords: $
//=============================================================================

#ifndef HL1TALKNPC_H
#define HL1TALKNPC_H

#pragma warning(push)
#include <set>
#pragma warning(pop)

#ifdef _WIN32
#pragma once
#endif

#include "soundflags.h"

#include "AI_Task.h"
#include "AI_Schedule.h"
#include "AI_Default.h"
#include "AI_Speech.h"
#include "AI_BaseNPC.h"
#include "AI_Behavior.h"
#include "AI_Behavior_Follow.h"
#include "npc_talker.h"


#define SF_NPC_PREDISASTER			( 1 << 16 )	// This is a predisaster scientist or barney. Influences how they speak.



//=========================================================
// Talking NPC base class
// Used for scientists and barneys
//=========================================================

//=============================================================================
// >> CHL1NPCTalker
//=============================================================================

class CHL1NPCTalker : public CNPCSimpleTalker
{
	DECLARE_CLASS( CHL1NPCTalker, CNPCSimpleTalker );
	
public:
	CHL1NPCTalker( void )
	{
	}

	void	StartTask( const Task_t *pTask );
	void	RunTask( const Task_t *pTask );
	int		SelectSchedule ( void );
	bool	HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
	bool	ShouldGib( const CTakeDamageInfo &info );

	int		TranslateSchedule( int scheduleType );
	void	IdleHeadTurn( const Vector &vTargetPos, float flDuration = 0.0, float flImportance = 1.0f );
	void    SetHeadDirection( const Vector &vTargetPos, float flInterval);
	bool	CorpseGib( const CTakeDamageInfo &info );

	Disposition_t IRelationType( CBaseEntity *pTarget );

	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );


	void			StartFollowing( CBaseEntity *pLeader );
	void			StopFollowing( void );
	
	void			Touch( CBaseEntity *pOther );

	float			PickRandomLookTarget( bool bExcludePlayers = false, float minTime = 1.5, float maxTime = 2.5 );

protected:
	virtual void 	FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
	virtual void	DeclineFollowing( void ) {}
	
public:

	bool	m_bInBarnacleMouth;


	enum
	{
		SCHED_HL1TALKER_FOLLOW_MOVE_AWAY = BaseClass::NEXT_SCHEDULE,
		SCHED_HL1TALKER_IDLE_SPEAK_WAIT,
		SCHED_HL1TALKER_BARNACLE_HIT,
		SCHED_HL1TALKER_BARNACLE_PULL,
		SCHED_HL1TALKER_BARNACLE_CHOMP,
		SCHED_HL1TALKER_BARNACLE_CHEW,

		NEXT_SCHEDULE,
	};

	enum
	{
		TASK_HL1TALKER_FOLLOW_WALK_PATH_FOR_UNITS = BaseClass::NEXT_TASK,

		NEXT_TASK,
	};

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};



#endif		//HL1TALKNPC_H
