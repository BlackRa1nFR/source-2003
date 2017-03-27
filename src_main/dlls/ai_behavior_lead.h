//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#ifndef AI_BEHAVIOR_LEAD_H
#define AI_BEHAVIOR_LEAD_H

#include "simtimer.h"
#include "ai_behavior.h"

#if defined( _WIN32 )
#pragma once
#endif

typedef const char *AIConcept_t;

// @TODO (toml 02-17-03): make these real
#define CONCEPT_LEAD_ARRIVAL			"OD_ARRIVAL"
#define CONCEPT_LEAD_SUCCESS			"OD_CHEESE"
#define CONCEPT_LEAD_FAILURE			"lead_fail"
#define CONCEPT_LEAD_COMING_BACK		"OD_STAYPUT"
#define CONCEPT_LEAD_CATCHUP			"OD_CATCHUP"

//-----------------------------------------------------------------------------
// class CAI_LeadBehavior
//
// Purpose:
//
//-----------------------------------------------------------------------------

enum LeadBehaviorEvents_t
{
	LBE_ARRIVAL,
	LBE_ARRIVAL_DONE,
	LBE_SUCCESS,
	LBE_FAILURE,
	LBE_DONE,
};

//-------------------------------------
//
// Handler class interface to listen to and modify actions of the lead behavior.
// Could be an NPC, or another entity (like a goal entity)
//

class CAI_LeadBehaviorHandler
{
public:
	virtual void OnEvent( int event ) {}
	virtual const char *GetConceptModifiers( const char *pszConcept )	{ return NULL; }
};

//-------------------------------------

enum AI_LeadFlags_t
{
	AILF_NO_DEF_SUCCESS	 = 0x01,
	AILF_NO_DEF_FAILURE	 = 0x02,
	AILF_USE_GOAL_FACING = 0x04,
};

struct AI_LeadArgs_t
{
	const char *pszGoal;
	unsigned 	flags;
};


class CAI_LeadBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_LeadBehavior, CAI_SimpleBehavior );
public:
	CAI_LeadBehavior()
	 :	m_pSink(NULL),
		m_LostTimer( 2.0, 4.0 )
	{
		memset( &m_args, 0, sizeof(m_args) );
		ClearGoal();
	}
	
	virtual const char *GetName() {	return "Lead"; }

	void LeadPlayer( const AI_LeadArgs_t &leadArgs, CAI_LeadBehaviorHandler *pSink = NULL );
	void StopLeading( void );

	virtual bool CanSelectSchedule();
	void BeginScheduleSelection();
	
	bool SetGoal( const AI_LeadArgs_t &args );
	void ClearGoal()										{ m_goal = vec3_origin; m_pSink = NULL;	}
	bool HasGoal() const 									{ return (m_goal != vec3_origin); }

	bool Connect( CAI_LeadBehaviorHandler *);
	bool Disconnect( CAI_LeadBehaviorHandler *);

	enum
	{
		// Schedules
		SCHED_LEAD_PLAYER = BaseClass::NEXT_SCHEDULE,
		SCHED_LEAD_PAUSE,
		SCHED_LEAD_RETRIEVE,
		SCHED_LEAD_RETRIEVE_WAIT,
		SCHED_LEAD_SUCCEED,
		SCHED_LEAD_AWAIT_SUCCESS,
		NEXT_SCHEDULE,
		
		// Tasks
		TASK_GET_PATH_TO_LEAD_GOAL = BaseClass::NEXT_TASK,
		TASK_STOP_LEADING,
		TASK_LEAD_FACE_GOAL,
		TASK_LEAD_ARRIVE,
		TASK_LEAD_SUCCEED,
		NEXT_TASK,
		
		// Conditions
		COND_LEAD_FOLLOWER_LOST = BaseClass::NEXT_CONDITION,
		COND_LEAD_FOLLOWER_LAGGING,
		COND_LEAD_FOLLOWER_NOT_LAGGING,
		COND_LEAD_FOLLOWER_VERY_CLOSE,
		COND_LEAD_SUCCESS,
		NEXT_CONDITION

	};
	
private:

	void GatherConditions();
	virtual int SelectSchedule();
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );

	bool Speak( AIConcept_t concept );
	bool IsSpeaking();

	// --------------------------------
	//
	// Sink notifiers. Isolated to limit exposure to actual sink storage,
	// provide debugging pinch pount, and allow for class-local logic
	// in addition to sink logic
	//
	void NotifyEvent( int event )								{ if ( m_pSink ) m_pSink->OnEvent( event ) ; }
	const char * GetConceptModifiers( const char *pszConcept )	{ return ( m_pSink ) ? m_pSink->GetConceptModifiers( pszConcept ) : NULL; }
	

	// --------------------------------

	AI_LeadArgs_t			m_args;
	CAI_LeadBehaviorHandler *m_pSink;
	
	// --------------------------------
	
	Vector		m_goal;
	float		m_goalyaw;
	
	CFixedRandStopwatch m_LostTimer;
	
	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
};

//-----------------------------------------------------------------------------

#endif // AI_BEHAVIOR_LEAD_H
