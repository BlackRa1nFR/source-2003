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

#ifndef NPC_ROLLER_H
#define NPC_ROLLER_H

#ifdef _WIN32
#pragma once
#endif


#include "soundenvelope.h"

#define ROLLER_CODE_DIGITS		4
#define ROLLER_TONE_TIME		0.5


//=========================================================
// Custom schedules
//=========================================================
enum
{
	SCHED_ROLLER_PATROL = LAST_SHARED_SCHEDULE,
	SCHED_ROLLER_WAIT_FOR_PHYSICS,
	SCHED_ROLLER_UNSTICK,

	LAST_ROLLER_SCHED,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_ROLLER_WAIT_FOR_PHYSICS = LAST_SHARED_TASK,
	TASK_ROLLER_FIND_PATROL_NODE,
	TASK_ROLLER_UNSTICK,
	TASK_ROLLER_ON,
	TASK_ROLLER_OFF,
	TASK_ROLLER_ISSUE_CODE,

	LAST_ROLLER_TASK,
};

//=========================================================
// Custom Conditions
//=========================================================
enum 
{
	COND_ROLLER_PHYSICS = LAST_SHARED_CONDITION,

	LAST_ROLLER_CONDITION,
};


//-----------------------------------------------------------------------------
// Purpose: This class only implements the IMotionEvent-specific behavior
//			It keeps track of the forces so they can be integrated
//-----------------------------------------------------------------------------
class CRollerController : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:
	IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

	AngularImpulse	m_vecAngular;
	Vector			m_vecLinear;

	void Off( void ) { m_fIsStopped = true; }
	void On( void ) { m_fIsStopped = false; }

	bool IsOn( void ) { return !m_fIsStopped; }

private:
	bool	m_fIsStopped;
};


//=========================================================
//=========================================================
class CNPC_Roller : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Roller, CAI_BaseNPC );

public:
	~CNPC_Roller( void );
	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void );

	virtual int TranslateSchedule( int scheduleType );
	virtual int SelectSchedule( void );

	DECLARE_DATADESC();

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	bool ShouldSavePhysics()	{ return true; }
	bool IsActivityFinished(void) { return true; }
	void SetActivity( Activity NewActivity ) { };
	virtual void PrescheduleThink( void );

	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	int VPhysicsTakeDamage( const CTakeDamageInfo &info );

	bool FInViewCone( CBaseEntity *pEntity );

	bool OverrideMove( float flInterval = 0.1 );

	void TaskFail( AI_TaskFailureCode_t code );
	void TaskFail( const char *pszGeneralFailText )	{ TaskFail( MakeFailCode( pszGeneralFailText ) ); }

	void Unstick( void );
	void RemainUpright( void );

	CRollerController			m_RollerController;
	IPhysicsMotionController	*m_pMotionController;

	bool FValidateHintType(CAI_Hint *pHint);

	bool IsBlockedForward( void );
	bool IsBlockedBackward( void );
	bool IsBlocked( const Vector &vecCheck );
	bool m_fHACKJustSpawned;

	void MarcoSound( void );
	void PoloSound( void );

	virtual void PowerOn( void );
	virtual void PowerOff( void );

	virtual void PowerOnSound( void );
	virtual void PowerOffSound( void );

	virtual int RollerPhysicsDamageMask( void );

	Vector m_vecUnstickDirection;

	float m_flLastZPos;
	float m_flTimeMarcoSound;
	float m_flTimePoloSound;
	int m_iFail;
	float m_flForwardSpeed;
	unsigned char m_iAccessCode[ ROLLER_CODE_DIGITS ];
	int m_iCodeProgress;
	CSoundPatch *m_pRollSound;

	DEFINE_CUSTOM_AI;
};


#endif NPC_ROLLER_H