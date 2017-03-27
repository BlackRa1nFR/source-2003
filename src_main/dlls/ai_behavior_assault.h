//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef AI_BEHAVIOR_ASSAULT_H
#define AI_BEHAVIOR_ASSAULT_H
#ifdef _WIN32
#pragma once
#endif

#include "simtimer.h"
#include "ai_behavior.h"
#include "ai_goalentity.h"
#include "ai_moveshoot.h"
#include "ai_utils.h"


enum AssaultCue_t
{
	CUE_NO_ASSAULT = 0,	// used to indicate that no assault is being conducted presently

	CUE_ENTITY_INPUT = 1,
	CUE_PLAYER_GUNFIRE,
	CUE_DONT_WAIT,
	CUE_COMMANDER,
	CUE_NONE,
};


//=============================================================================
//=============================================================================
class CRallyPoint : public CPointEntity 
{
	DECLARE_CLASS( CRallyPoint, CPointEntity );

public:
	CRallyPoint() { Lock( false ); }

	void Lock( bool lock ) { m_IsLocked = lock; }
	bool IsLocked( void ) { return m_IsLocked; }

	string_t	m_AssaultPointName;
	string_t	m_RallySequenceName;
	float		m_flAssaultDelay;
	int			m_iPriority;

	COutputEvent	m_OnArrival;

	DECLARE_DATADESC();

private:
	bool	m_IsLocked;
};

//=============================================================================
//=============================================================================
class CAssaultPoint : public CPointEntity 
{
	DECLARE_CLASS( CAssaultPoint, CPointEntity );

public:
	string_t		m_AssaultHintGroup;
	string_t		m_NextAssaultPointName;
	COutputEvent	m_OnAssaultClear;
	float			m_flAssaultTimeout;

	COutputEvent	m_OnArrival;

	DECLARE_DATADESC();
};

//=============================================================================
//=============================================================================
class CAI_AssaultBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_AssaultBehavior, CAI_SimpleBehavior );

public:
	CAI_AssaultBehavior();
	
	virtual const char *GetName() {	return "Assault"; }

	virtual bool 	CanSelectSchedule();
	virtual void	BeginScheduleSelection();
	virtual void	EndScheduleSelection();
	virtual void	GatherConditions();

	bool HasHitRallyPoint() { return m_bHitRallyPoint; }
	bool HasHitAssaultPoint() { return m_bHitAssaultPoint; }

	void ClearAssaultPoint( void );
	bool PollAssaultCue( void );
	void ReceiveAssaultCue( AssaultCue_t cue );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	int TranslateSchedule( int scheduleType );

	void InitializeBehavior();
	void SetParameters( string_t rallypointname, AssaultCue_t assaultcue );
	void SetParameters( CBaseEntity *pRallyEnt, AssaultCue_t assaultcue );

	CRallyPoint *FindBestRallyPointInRadius( const Vector &vecCenter, float flRadius );;

	void Disable( void ) { m_AssaultCue = CUE_NO_ASSAULT; m_bHitRallyPoint = false; m_bHitAssaultPoint = false; }

	enum
	{
		SCHED_MOVE_TO_RALLY_POINT = BaseClass::NEXT_SCHEDULE,		// Try to get out of the player's way
		SCHED_MOVE_TO_ASSAULT_POINT,
		SCHED_HOLD_RALLY_POINT,
		SCHED_HOLD_ASSAULT_POINT,
		SCHED_WAIT_AND_CLEAR,
		SCHED_ASSAULT_MOVE_AWAY,
		NEXT_SCHEDULE,

		TASK_GET_PATH_TO_RALLY_POINT = BaseClass::NEXT_TASK,
		TASK_FACE_RALLY_POINT,
		TASK_GET_PATH_TO_ASSAULT_POINT,
		TASK_FACE_ASSAULT_POINT,
		TASK_AWAIT_CUE,
		TASK_AWAIT_ASSAULT_TIMEOUT,
		TASK_ANNOUNCE_CLEAR,
		TASK_WAIT_ASSAULT_DELAY,
		TASK_ASSAULT_MOVE_AWAY_PATH,
		NEXT_TASK,

/*
		COND_PUT_CONDITIONS_HERE = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,
*/
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

public:
	CHandle<CAssaultPoint> m_hAssaultPoint;
	CHandle<CRallyPoint> m_hRallyPoint;

private:
	virtual int		SelectSchedule();

	CAI_ShotRegulator m_ShotRegulator;

	AssaultCue_t	m_AssaultCue;			// the cue we're waiting for to begin the assault
	AssaultCue_t	m_ReceivedAssaultCue;	// the last assault cue we received from someone/thing external.

	bool			m_bHitRallyPoint;
	bool			m_bHitAssaultPoint;

	//---------------------------------
	
	DECLARE_DATADESC();
};

#endif // AI_BEHAVIOR_ASSAULT_H
