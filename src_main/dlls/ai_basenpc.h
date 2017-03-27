//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Base NPC character with AI
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_BASENPC_H
#define AI_BASENPC_H

#ifdef _WIN32
#pragma once
#endif

#include "simtimer.h" 	 	   
#include "basecombatcharacter.h"
#include "ai_debug.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_condition.h"
#include "ai_component.h"
#include "ai_task.h"
#include "ai_movetypes.h"
#include "ai_navtype.h"
#include "ai_namespaces.h"
#include "ai_npcstate.h"
#include "ai_hull.h"
#include "ai_moveshoot.h"
#include "entityoutput.h"
#include "utlvector.h"
#include "activitylist.h"
#include "bitstring.h"
#include "ai_basenpc.h"
#include "ai_navgoaltype.h" //GoalType_t enum

class CAI_Schedule;
class CAI_Network;
class CAI_Route;
class CAI_Hint;
class CAI_Navigator;
class CAI_Pathfinder;
class CAI_Senses;
class CAI_Enemies;
class CAI_Squad;
class CAI_Expresser;
class CAI_BehaviorBase;
class CAI_Motor;
class CAI_MoveProbe;
class CAI_LocalNavigator;
class CAI_TacticalServices;
class CBitString;
class CAI_ScriptedSequence;
class CSceneEntity;
class CBaseGrenade;
class CBaseDoor;
class CBasePropDoor;
struct AI_Waypoint_t;

typedef CFixedBitString<MAX_CONDITIONS> CAI_ScheduleBits;

//=============================================================================
//
// Constants & enumerations
//
//=============================================================================
#define TURRET_CLOSE_RANGE	200
#define TURRET_MEDIUM_RANGE 500

//-------------------------------------
// Memory
//-------------------------------------

#define MEMORY_CLEAR					0
#define bits_MEMORY_PROVOKED			( 1 << 0 )// right now only used for houndeyes.
#define bits_MEMORY_INCOVER				( 1 << 1 )// npc knows it is in a covered position.
#define bits_MEMORY_SUSPICIOUS			( 1 << 2 )// Ally is suspicious of the player, and will move to provoked more easily
//#define	bits_MEMORY_				( 1 << 3 )
//#define	bits_MEMORY_				( 1 << 4 )
#define bits_MEMORY_PATH_FAILED			( 1 << 5 )// Failed to find a path
#define bits_MEMORY_FLINCHED			( 1 << 6 )// Has already flinched
//#define bits_MEMORY_ 					( 1 << 7 )
#define bits_MEMORY_TOURGUIDE			( 1 << 8 )// I have been acting as a tourguide.
//#define bits_MEMORY_					( 1 << 9 )// 
#define bits_MEMORY_LOCKED_HINT			( 1 << 10 )// 
//#define bits_MEMORY_					( 1 << 12 )

#define bits_MEMORY_TURNING				( 1 << 13 )// Turning, don't interrupt me.
#define bits_MEMORY_TURNHACK			( 1 << 14 )

#define bits_MEMORY_HAD_ENEMY			( 1 << 15 )// Had an enemy
#define bits_MEMORY_HAD_PLAYER			( 1 << 16 )// Had player
#define bits_MEMORY_HAD_LOS				( 1 << 17 )// Had LOS to enemy

#define bits_MEMORY_CUSTOM4				( 1 << 28 )	// NPC-specific memory
#define bits_MEMORY_CUSTOM3				( 1 << 29 )	// NPC-specific memory
#define bits_MEMORY_CUSTOM2				( 1 << 30 )	// NPC-specific memory
#define bits_MEMORY_CUSTOM1				( 1 << 31 )	// NPC-specific memory

//-------------------------------------
// Spawn flags
//-------------------------------------

#define SF_NPC_WAIT_TILL_SEEN			( 1 << 0  )	// spawnflag that makes npcs wait until player can see them before attacking.
#define SF_NPC_GAG						( 1 << 1  )	// no idle noises from this npc
#define SF_NPC_FALL_TO_GROUND			( 1 << 2  )	// used my NPC_Maker
#define SF_NPC_DROP_HEALTHKIT			( 1 << 3  )	// Drop a healthkit upon death
#define SF_NPC_START_EFFICIENT			( 1 << 4  ) // Set into efficiency mode from spawn
//										( 1 << 5  )
//										( 1 << 6  )
#define SF_NPC_WAIT_FOR_SCRIPT			( 1 << 7  )	// spawnflag that makes npcs wait to check for attacking until the script is done or they've been attacked
#define SF_NPC_LONG_RANGE				( 1 << 8  )	// makes npcs look far and relaxes weapon range limit 
#define SF_NPC_FADE_CORPSE				( 1 << 9  )	// Fade out corpse after death
#define SF_NPC_ALWAYSTHINK				( 1 << 10 )	// Simulate even when player isn't in PVS.
#define SF_NPC_TEMPLATE					( 1 << 11 )	// This NPC will be used as a template by an npc_maker -- do not spawn.
//										( 1 << 12 )	
//										( 1 << 13 )	
//										( 1 << 14 )	
//										( 1 << 15 )	
// !! Flags above ( 1 << 15 )	 are reserved for NPC sub-classes

//-------------------------------------
//
// Return codes from CanPlaySequence.
//
//-------------------------------------

enum CanPlaySequence_t
{
	CANNOT_PLAY = 0,		// Can't play for any number of reasons.
	CAN_PLAY_NOW,			// Can play the script immediately.
	CAN_PLAY_ENQUEUED,		// Can play the script after I finish playing my current script.
};

//-------------------------------------
//
// Efficiency modes
//
// @Note (toml 07-16-03): using enum because will probably want to support scaled efficiency
//-------------------------------------

enum AI_Efficiency_t
{
	AIE_NORMAL,
	AIE_EFFICIENT
};

//-------------------------------------
//
// Debug bits
//
//-------------------------------------

enum DebugBaseNPCBits_e
{
	bits_debugDisableAI = 0x00000001,		// disable AI
	bits_debugStepAI	= 0x00000002,		// step AI

};

//=============================================================================
//
// Types used by CAI_BaseNPC
//
//=============================================================================

struct AIScheduleState_t
{
	int					 iCurTask;
	TaskStatus_e		 fTaskStatus;
	float				 timeStarted;
	float				 timeCurTaskStarted;
	AI_TaskFailureCode_t taskFailureCode;

	DECLARE_SIMPLE_DATADESC();
};

// -----------------------------------------
//	An entity that this NPC can't reach
// -----------------------------------------

struct UnreachableEnt_t
{
	EHANDLE	hUnreachableEnt;	// Entity that's unreachable
	float	fExpireTime;		// Time to forget this information
	Vector	vLocationWhenUnreachable;
	
	DECLARE_SIMPLE_DATADESC();
};

//=============================================================================
//
// Utility functions
//
//=============================================================================

Vector VecCheckToss ( CBaseEntity *pEdict, Vector vecSpot1, Vector vecSpot2, float flHeightMaxRatio, float flGravityAdj, bool bRandomize );
Vector VecCheckThrow( CBaseEntity *pEdict, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj = 1.0f );

extern Vector g_vecAttackDir;

bool FBoxVisible ( CBaseEntity *pLooker, CBaseEntity *pTarget );
bool FBoxVisible ( CBaseEntity *pLooker, CBaseEntity *pTarget, Vector &vecTargetOrigin, float flSize = 0.0 );

// FIXME: move to utils?
float DeltaV( float v0, float v1, float d );
float ChangeDistance( float flInterval, float flGoalDistance, float flGoalVelocity, float flCurVelocity, float flIdealVelocity, float flAccelRate, float &flNewDistance, float &flNewVelocity );

//=============================================================================
//
// class CAI_Manager
//
// Central location for components of the AI to operate across all AIs without
// iterating over the global list of entities.
//
//=============================================================================

class CAI_Manager
{
public:
	CAI_Manager();
	
	CAI_BaseNPC **	AccessAIs();
	int				NumAIs();
	
	void AddAI( CAI_BaseNPC *pAI );
	void RemoveAI( CAI_BaseNPC *pAI );
	
private:
	enum
	{
		MAX_AIS = 256
	};
	
	typedef CUtlVector<CAI_BaseNPC *> CAIArray;
	
	CAIArray m_AIs;

};

//-------------------------------------

extern CAI_Manager g_AI_Manager;

//=============================================================================
//
//	class CAI_BaseNPC
//
//=============================================================================

class CAI_BaseNPC : public CBaseCombatCharacter, 
					public CAI_DefMovementSink
{
	DECLARE_CLASS( CAI_BaseNPC, CBaseCombatCharacter );

public:
	//-----------------------------------------------------
	//
	// Initialization, cleanup, serialization, identity
	//
	
	CAI_BaseNPC();
	~CAI_BaseNPC();

	//---------------------------------
	
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual int			Save( ISave &save ); 
	virtual int			Restore( IRestore &restore );
	virtual void		OnRestore();

	bool				ShouldSavePhysics()	{ return false; }
	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const { return MASK_NPCSOLID; }
	
	//---------------------------------
	
	virtual void		PostConstructor( const char *szClassname );
	virtual void		Precache( void ); // derived calls at start of Spawn()
	virtual bool 		CreateVPhysics();
	virtual void		NPCInit( void ); // derived calls after Spawn()
	void				NPCInitThink( void );
	virtual void		PostNPCInit() {};// called after NPC_InitThink
	virtual void		StartNPC( void );
	virtual bool		IsTemplate( void );

	virtual void		CleanupOnDeath( CBaseEntity *pCulprit = NULL );
	virtual void		UpdateOnRemove( void );

	virtual void		StartTargetHandling( CBaseEntity *pTargetEnt );


	//---------------------------------
	// Component creation factories
	// 
	
	// The master call, override if you introduce new component types. Call base first
	virtual bool 			CreateComponents();
	
	// Components defined by the base AI class
	virtual CAI_Senses *	CreateSenses();
	virtual CAI_MoveProbe *	CreateMoveProbe();
	virtual CAI_Motor *		CreateMotor();
	virtual CAI_LocalNavigator *CreateLocalNavigator();
	virtual CAI_Navigator *	CreateNavigator();
	virtual CAI_Pathfinder *CreatePathfinder();
	virtual CAI_TacticalServices *CreateTacticalServices();

	//---------------------------------
	
	virtual CAI_BaseNPC *MyNPCPointer( void ) { return this; }

public:
	//-----------------------------------------------------
	//
	// AI processing - thinking, schedule selection and task running
	//
	//-----------------------------------------------------
	void				CallNPCThink( void ) { this->NPCThink(); } // this may be unneeded now functions in datadesc can be virtual (and the right thing happpens) (toml 10-23-02)
	
	// Thinking, including core thinking, movement, animation
	virtual void		NPCThink( void );

	// Core thinking (schedules & tasks)
	virtual void		RunAI( void );// core ai function!	

	// Called to gather up all relevant conditons
	virtual void		GatherConditions( void );

	// Called immediately prior to schedule processing
	virtual void		PrescheduleThink( void ) { return; };

	// Notification that the current schedule, if any, is ending and a new one is being selected
	virtual void		OnScheduleChange( void );

	// This function implements a decision tree for the NPC.  It is responsible for choosing the next behavior (schedule)
	// based on the current conditions and state.
	virtual int			SelectSchedule( void );
	virtual int			SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );

	// After the schedule has been selected, it will be processed by this function so child NPC classes can 
	// remap base schedules into child-specific behaviors
	virtual int			TranslateSchedule( int scheduleType );

	virtual void		StartTask( const Task_t *pTask );
	virtual void		RunTask( const Task_t *pTask );

	virtual void		HandleAnimEvent( animevent_t *pEvent );

	bool				IsInterruptable();

	virtual bool		ShouldAlwaysThink();

	enum
	{
		NEXT_SCHEDULE 	= LAST_SHARED_SCHEDULE,
		NEXT_TASK		= LAST_SHARED_TASK,
		NEXT_CONDITION 	= LAST_SHARED_CONDITION,
	};

protected:
	// Used by derived classes to chain a task to a task that might not be the 
	// one they are currently handling:
	void				ChainStartTask( int task, float taskData = 0 )	{ Task_t tempTask = { task, taskData }; StartTask( (const Task_t *)&tempTask ); }
	void				ChainRunTask( int task, float taskData = 0 )	{ Task_t tempTask = { task, taskData }; RunTask( (const Task_t *)	&tempTask );	}

	void				StartTaskOverlay();
	void				RunTaskOverlay();
	void				EndTaskOverlay();

private:
	bool				PreThink( void );
	void				PerformSensing();
	void				MaintainSchedule( void );
	void				RunAnimation( void );
	void				PostRun( void );
	void				PerformMovement();
	
	virtual int			StartTask ( Task_t *pTask ) { Msg( "Called wrong StartTask()\n" ); StartTask( (const Task_t *)pTask ); return 0; } // to ensure correct signature in derived classes
	virtual int			RunTask ( Task_t *pTask )	{ Msg( "Called wrong RunTask()\n" ); RunTask( (const Task_t *)pTask ); return 0; } // to ensure correct signature in derived classes

public:
	//-----------------------------------------------------
	//
	// Schedules & tasks
	//
	//-----------------------------------------------------
	
	void				SetSchedule( CAI_Schedule *pNewSchedule );
	void				SetSchedule( int localScheduleID ) 			{ SetSchedule( GetScheduleOfType( localScheduleID ) ); }
	
	void				SetDefaultFailSchedule( int failSchedule )	{ m_failSchedule = failSchedule; }
	
	void				ClearSchedule( void );
	
	CAI_Schedule *		GetCurSchedule()							{ return m_pSchedule; }
	bool				IsCurSchedule( int schedId, bool fTranslate = true )	{ return ( m_pSchedule && 
																						   ( ( fTranslate && m_pSchedule == GetScheduleOfType( schedId ) ) ||
																						   	 ( !fTranslate && GetClassScheduleIdSpace()->ScheduleGlobalToLocal( m_pSchedule->GetId() ) == schedId) ) ); }

	virtual CAI_Schedule *GetSchedule(int localScheduleID);
	virtual int			GetLocalScheduleId( int globalScheduleID )	{ return GetClassScheduleIdSpace()->ScheduleGlobalToLocal( globalScheduleID ); }

	float				GetTimeScheduleStarted() const				{ return m_ScheduleState.timeStarted; }
	
	//---------------------------------
	
	const Task_t*		GetTask( void );
	int					TaskIsRunning( void );
	
	virtual void		TaskFail( AI_TaskFailureCode_t );
	void				TaskFail( const char *pszGeneralFailText )	{ TaskFail( MakeFailCode( pszGeneralFailText ) ); }
	void				TaskComplete( bool fIgnoreSetFailedCondition = false );
	
	void				TaskMovementComplete( void );
	inline int			TaskIsComplete( void ) 						{ return (GetTaskStatus() == TASKSTATUS_COMPLETE); }

	virtual const char *TaskName(int taskID);

	float				GetTimeTaskStarted() const					{ return m_ScheduleState.timeCurTaskStarted; }
	virtual int			GetLocalTaskId( int globalTaskId)			{ return GetClassScheduleIdSpace()->TaskGlobalToLocal( globalTaskId ); }

	virtual const char *GetSchedulingErrorName()					{ return "CAI_BaseNPC"; }

protected:
	static bool			LoadSchedules(void);
	virtual bool		LoadedSchedules(void);
	virtual void		BuildScheduleTestBits( void );

	//---------------------------------

private:
	// This function maps the type through TranslateSchedule() and then retrieves the pointer
	// to the actual CAI_Schedule from the database of schedules available to this class.
	CAI_Schedule *		GetScheduleOfType( int scheduleType );
	
	// This is the main call to select/translate a schedule
	CAI_Schedule *		GetNewSchedule( void );
	CAI_Schedule *		GetFailSchedule( void );


	bool				FHaveSchedule( void );
	bool				FScheduleDone ( void );
	CAI_Schedule *		ScheduleInList( const char *pName, CAI_Schedule **pList, int listCount );

	int 				GetScheduleCurTaskIndex() const			{ return m_ScheduleState.iCurTask;		}
	int 				IncScheduleCurTaskIndex()				{ return ++m_ScheduleState.iCurTask; 		}
	void 				ResetScheduleCurTaskIndex()				{ m_ScheduleState.iCurTask = 0;			}
	void				NextScheduledTask ( void );
	bool				IsScheduleValid ( void );
	bool				ShouldSelectIdealState( void );
	
	void				OnStartTask( void ) 					{ SetTaskStatus( TASKSTATUS_RUN_MOVE_AND_TASK ); }
	void 				SetTaskStatus( TaskStatus_e status )	{ m_ScheduleState.fTaskStatus = status; 	}
	TaskStatus_e 		GetTaskStatus() const					{ return m_ScheduleState.fTaskStatus; 	}

	void				DiscardScheduleState();

	//---------------------------------

	CAI_Schedule *		m_pSchedule;
	AIScheduleState_t	m_ScheduleState;
	int					m_failSchedule;				// Schedule type to choose if current schedule fails
	bool				m_bDoPostRestoreRefindPath;

public:
	//-----------------------------------------------------
	//
	// Hooks for CAI_Behaviors, *if* derived class supports them
	//
	//-----------------------------------------------------
	template <class BEHAVIOR_TYPE>
	bool GetBehavior( BEHAVIOR_TYPE **ppBehavior )
	{
		CAI_BehaviorBase **ppBehaviors = AccessBehaviors();
		
		*ppBehavior = NULL;
		for ( int i = 0; i < NumBehaviors(); i++ )
		{
			*ppBehavior = dynamic_cast<BEHAVIOR_TYPE *>(ppBehaviors[i]);
			if ( *ppBehavior )
				return true;
		}
		return false;
	}

	virtual CAI_BehaviorBase *GetRunningBehavior() { return NULL; }

	// Notification that the status behavior ability to select schedules has changed.
	// Return "true" to signal a schedule interrupt is desired
	virtual bool OnBehaviorChangeStatus(  CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule ) { return false; }

private:
	virtual CAI_BehaviorBase **	AccessBehaviors() 	{ return NULL; }
	virtual int					NumBehaviors()		{ return 0; }

public:
	//-----------------------------------------------------
	//
	// Conditions
	//
	//-----------------------------------------------------

	virtual const char*	ConditionName(int conditionID);
	
	virtual void		RemoveIgnoredConditions ( void );
	void				SetCondition( int iCondition /*, bool state = true*/ );
	bool				HasCondition( int iCondition );
	bool				HasInterruptCondition( int iCondition );
	void				ClearCondition( int iCondition );
	void				ClearConditions( int *pConditions, int nConditions );
	void				SetIgnoreConditions( int *pConditions, int nConditions );
	bool				ConditionInterruptsCurSchedule( int iCondition );

	void				SetCustomInterruptCondition( int nCondition );
	void				ClearCustomInterruptCondition( int nCondition );
	void				ClearCustomInterruptConditions( void );

	bool				ConditionsGathered() const		{ return m_bConditionsGathered; }
	const CAI_ScheduleBits &AccessConditionBits() const { return m_Conditions; }
	CAI_ScheduleBits &	AccessConditionBits()			{ return m_Conditions; }

private:
	CAI_ScheduleBits	m_Conditions;
	CAI_ScheduleBits	m_CustomInterruptConditions;	//Bit string assembled by the schedule running, then 
														//modified by leaf classes to suit their needs
	CAI_ScheduleBits	m_InverseIgnoreConditions;

	bool				m_bConditionsGathered;

public:
	//-----------------------------------------------------
	//
	// NPC State
	//
	//-----------------------------------------------------
	
	virtual NPC_STATE	SelectIdealState( void );

	void				SetState ( NPC_STATE State );
	virtual bool		ShouldGoToIdleState( void ) 							{ return ( false ); };
	virtual	void 		OnStateChange( NPC_STATE OldState, NPC_STATE NewState ) {/*Base class doesn't care*/};
	
	NPC_STATE			GetState( void )										{ return m_NPCState; }

	AI_Efficiency_t		GetEfficiency() const						{ return m_Efficiency; }
	void				SetEfficiency( AI_Efficiency_t efficiency )	{ m_Efficiency = efficiency; }


	//---------------------------------

	NPC_STATE			m_NPCState;				// npc's current state
	NPC_STATE			m_IdealNPCState;		// npc should change to this state
	float				m_flLastStateChangeTime;

private:
	AI_Efficiency_t		m_Efficiency;

public:
	//-----------------------------------------------------
	//
	//	Activities
	// 
	//-----------------------------------------------------
	
	Activity			TranslateActivity( Activity idealActivity, Activity *pIdealWeaponActivity = NULL );
	Activity			NPC_TranslateActivity( Activity eNewActivity );
	Activity			GetActivity( void ) { return m_Activity; }
	virtual void		SetActivity( Activity NewActivity );
	Activity			GetIdealActivity( void ) { return m_IdealActivity; }
	void				SetIdealActivity( Activity NewActivity );
	void				ResetIdealActivity( Activity newIdealActivity );
	void				SetSequenceByName( char *szSequence );
	Activity			GetScriptCustomMoveActivity( void );
	Activity			GetStoppedActivity( void );
	bool				HaveSequenceForActivity( Activity activity )				{ return ( SelectWeightedSequence( activity ) != ACTIVITY_NOT_AVAILABLE ); }
	inline bool			IsActivityStarted(void);
	virtual bool		IsActivityFinished( void );
	virtual bool		IsActivityMovementPhased( Activity activity );
	virtual void		OnChangeActivity( Activity eNewActivity ) {}

private:

	void				MaintainActivity(void);
	void				AdvanceToIdealActivity(void);
	void				ResolveActivityToSequence(Activity NewActivity, int &iSequence, Activity &translatedActivity, Activity &weaponActivity);
	void				SetActivityAndSequence(Activity NewActivity, int iSequence, Activity translatedActivity, Activity weaponActivity);

	Activity			m_Activity;				// Current animation state
	Activity			m_translatedActivity;	// Current actual translated animation

	Activity			m_IdealActivity;				// Desired animation state
	int					m_nIdealSequence;				// Desired animation sequence
	Activity			m_IdealTranslatedActivity;		// Desired actual translated animation state
	Activity			m_IdealWeaponActivity;			// Desired weapon animation state

public:
	//-----------------------------------------------------
	//
	// Senses
	//
	//-----------------------------------------------------

	CAI_Senses *		GetSenses()			{ return m_pSenses; }
	const CAI_Senses *	GetSenses() const	{ return m_pSenses; }
	
	void				SetDistLook( float flDistLook );

	virtual bool		QueryHearSound( CSound *pSound ) { return true; }
	virtual bool		QuerySeeEntity( CBaseEntity *pEntity) { return true; }

	virtual void		OnLooked( int iDistance );
	virtual void		OnListened();

	virtual void		OnSeeEntity( CBaseEntity *pEntity ) {}
	
	virtual int			GetSoundInterests( void );

	CSound *			GetLoudestSoundOfType( int iType );
	virtual CSound *	GetBestSound( void );
	virtual CSound *	GetBestScent( void );
	virtual float		HearingSensitivity( void ) { return 1.0; };

protected:
	virtual void		ClearSenseConditions( void );
	
private:
	CAI_Senses *		m_pSenses;

public:
	//-----------------------------------------------------
	//
	// Enemy and target
	//
	//-----------------------------------------------------

	Vector GetSmoothedVelocity( void );

	CBaseEntity*		GetEnemy() const					{ return m_hEnemy.Get(); }
	void				SetEnemy( CBaseEntity *pEnemy );

	const Vector &		GetEnemyLKP() const;
	float				GetEnemyLastTimeSeen() const;
	void				MarkEnemyAsEluded();
	void				ClearEnemyMemory();
	bool				EnemyHasEludedMe() const;
	
	virtual CBaseEntity *BestEnemy();		// returns best enemy in memory list
	virtual	bool		IsValidEnemy( CBaseEntity *pEnemy );

	bool				ChooseEnemy();
	virtual bool		ShouldChooseNewEnemy();
	virtual void		GatherEnemyConditions( CBaseEntity *pEnemy );
	float				EnemyDistance( CBaseEntity *pEnemy );
	CBaseCombatCharacter *GetEnemyCombatCharacterPointer();
	void SetEnemyOccluder(CBaseEntity *pBlocker);
	CBaseEntity *GetEnemyOccluder(void);

	//---------------------------------
	
	CBaseEntity*		GetTarget()							{ return m_hTargetEnt.Get(); }
	void				SetTarget( CBaseEntity *pTarget );
	void				CheckTarget( CBaseEntity *pTarget );

private:
	void *				CheckEnemy( CBaseEntity *pEnemy ) { return NULL; } // OBSOLETE, replaced by GatherEnemyConditions(), left here to make derived code not compile

	// Updates the goal position in case of GOALTYPE_ENEMY
	void				UpdateEnemyPos();

	// Updates the goal position in case of GOALTYPE_TARGETENT
	void				UpdateTargetPos();

	//---------------------------------
	
	EHANDLE				m_hEnemy;		// the entity that the npc is fighting.
	EHANDLE				m_hTargetEnt;	// the entity that the npc is trying to reach

	CRandStopwatch		m_GiveUpOnDeadEnemyTimer;

	float				m_flAcceptableTimeSeenEnemy;

public:
	//-----------------------------------------------------
	//
	// Commander mode stuff.
	//
	//-----------------------------------------------------
	virtual bool IsPlayerAlly() { Class_T classification = Classify(); return ( ( classification == CLASS_PLAYER_ALLY || classification == CLASS_PLAYER_ALLY_VITAL ) && IRelationType( UTIL_PlayerByIndex( 1 ) ) == D_LI ); }
	virtual bool CanBeSelected() { return false; }
	virtual bool IsSelected() { return m_bSelectedByPlayer; }
	virtual void PlayerSelect( bool select );
	virtual void OnPlayerSelect() {};
	virtual void SetCommandGoal( Vector vecGoal ) { m_vecCommandGoal = vecGoal; }
	virtual void ClearCommandGoal() { m_vecCommandGoal = vec3_invalid; }
	virtual bool TargetOrder( CBaseEntity *pTarget ) { OnTargetOrder(); return true; }
	virtual void OnTargetOrder() {};
	virtual void MoveOrder( const Vector &vecDest ) { SetCommandGoal( vecDest ); SetCondition( COND_RECEIVED_ORDERS ); OnMoveOrder(); }
	virtual void OnMoveOrder() {};
	const Vector &GetCommandGoal() { return m_vecCommandGoal; }

private:
	bool m_bSelectedByPlayer;
	Vector m_vecCommandGoal;
	EHANDLE	m_hCommandTarget;
	
public:
	//-----------------------------------------------------
	//
	//  Sounds
	// 
	//-----------------------------------------------------
	virtual CanPlaySequence_t CanPlaySequence( bool fDisregardState, int interruptLevel );

	virtual bool		CanPlaySentence( bool fDisregardState ) { return IsAlive(); }
	virtual int			PlaySentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, CBaseEntity *pListener = NULL );
	virtual int			PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener );
	void				SentenceStop( void );

	virtual	bool		IsSoundSource( void ) { return true; }

	virtual bool		FOkToMakeSound( void );
	virtual void		JustMadeSound( void );

	virtual void		DeathSound( void )					{ return; };
	virtual void		AlertSound( void )					{ return; };
	virtual void		IdleSound( void )					{ return; };
	virtual void		PainSound( void )					{ return; };
	virtual void		FearSound( void )				 	{ return; };
	virtual void		LostEnemySound( void ) 				{ return; };
	virtual void		FoundEnemySound( void ) 			{ return; };

	virtual void		SpeakSentence( int sentenceType ) 	{ return; };
	virtual bool		ShouldPlayIdleSound( void );

	virtual void		StopLoopingSounds( void ) { return; }

	//---------------------------------

	virtual CAI_Expresser *GetExpresser() { return NULL; }

protected:
	float				m_flSoundWaitTime;	// Time when I'm allowed to make another sound

public:
	//-----------------------------------------------------
	//
	// Capabilities report (from CBaseCombatCharacter)
	//
	//-----------------------------------------------------
	virtual int			CapabilitiesGet( void );

	// local capabilities access
	int					CapabilitiesAdd( int capabilities );
	int					CapabilitiesRemove( int capabilities );
	void				CapabilitiesClear( void );

private:
	int					m_afCapability;			// tells us what a npc can/can't do.

public:
	//-----------------------------------------------------
	//
	// Pathfinding, navigation & movement
	//
	//-----------------------------------------------------
	
	CAI_Navigator *		GetNavigator() 				{ return m_pNavigator; }
	const CAI_Navigator *GetNavigator() const 		{ return m_pNavigator; }

	CAI_LocalNavigator *GetLocalNavigator()			{ return m_pLocalNavigator; }
	const CAI_LocalNavigator *GetLocalNavigator() const { return m_pLocalNavigator; }

	CAI_Pathfinder *	GetPathfinder() 			{ return m_pPathfinder; }
	const CAI_Pathfinder *GetPathfinder() const 	{ return m_pPathfinder; }

	CAI_MoveProbe *		GetMoveProbe() 				{ return m_pMoveProbe; }
	const CAI_MoveProbe *GetMoveProbe() const		{ return m_pMoveProbe; }

	CAI_Motor *			GetMotor() 					{ return m_pMotor; }
	const CAI_Motor *	GetMotor() const			{ return m_pMotor; }
	
	//---------------------------------
	
	// The current navigation (movement) mode (e.g. fly, swim, locomote, etc) 
	Navigation_t		GetNavType() const;
	void				SetNavType( Navigation_t navType );
	
	CBaseEntity *		GetNavTargetEntity(void);

	int					IsMoving( void );
	
	// NPCs can override this to tweak with how costly particular movements are
	virtual	bool		MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost );

	// Turns a directional vector into a yaw value that points down that vector.
	float				VecToYaw( const Vector &vecDir );

	//  Turning
	virtual	float		CalcIdealYaw( const Vector &vecTarget );
	virtual float		MaxYawSpeed( void );		// Get max yaw speed
	bool				FacingIdeal( void );

	//   Add multiple facing goals while moving/standing still.
	virtual void		AddFacingTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void		AddFacingTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void		AddFacingTarget( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual float		GetFacingDirection( Vector &vecDir );

	// ------------
	// Methods used by motor to query properties/preferences/move-related state
	// ------------
	virtual bool		IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const; // Override for specific creature types
	bool				IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos, float maxUp, float maxDown, float maxDist ) const;
	bool 				ShouldMoveWait();
	virtual float		StepHeight() const			{ return 18.0f; }
	virtual float		GetMaxJumpSpeed() const		{ return 350.0f; }
	
	//---------------------------------
	
	virtual bool		OverrideMove( float flInterval );				// Override to take total control of movement (return true if done so)
	virtual	bool		OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );

	//---------------------------------
	
	virtual bool		IsUnusableNode(int iNodeID, CAI_Hint *pHint); // Override for special NPC behavior
	virtual bool		ValidateNavGoal();
	virtual bool		IsCurTaskContinuousMove();

	//---------------------------------
	
	void				RememberUnreachable( CBaseEntity* pEntity );	// Remember that entity is unreachable
	virtual bool		IsUnreachable( CBaseEntity* pEntity );			// Is entity is unreachable?

	//---------------------------------
	
	void TestPlayerPushing( CBaseEntity *pPlayer );

	//---------------------------------
	// Inherited from IAI_MotorMovementServices
	virtual float		CalcYawSpeed( void );

	virtual bool		OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, 
												float distClear, 
												AIMoveResult_t *pResult );

	// Translations of the above into some useful game terms
	virtual bool		OnUpcomingDoor( AILocalMoveGoal_t *pMoveGoal, 
										 CBaseDoor *pDoor,
										 float distClear, 
										 AIMoveResult_t *pResult );

	virtual bool OnUpcomingPropDoor( AILocalMoveGoal_t *pMoveGoal,
 											CBasePropDoor *pDoor,
											float distClear,
											AIMoveResult_t *pResult );

	//---------------------------------
	
	float				m_flMoveWaitFinished;
	

	//
	// Stuff for opening doors.
	//	
	void OnDoorFullyOpen(CBasePropDoor *pDoor);
	void OnDoorBlocked(CBasePropDoor *pDoor);
	CHandle<CBasePropDoor> m_hOpeningDoor;	// The CBasePropDoor that we are in the midst of opening for navigation.

protected:
	// BRJ 4/11
	// Semi-obsolete-looking Lars code I moved out of the engine and into here
	int FlyMove( const Vector& vecPosition, unsigned int mask );
	int WalkMove( const Vector& vecPosition, unsigned int mask );

private:
	CAI_Navigator *		m_pNavigator;
	CAI_LocalNavigator *m_pLocalNavigator;
	CAI_Pathfinder *	m_pPathfinder;
	CAI_MoveProbe *		m_pMoveProbe;
	CAI_Motor *			m_pMotor;

	//  Unreachable Entities
	CUtlVector<UnreachableEnt_t> m_UnreachableEnts;								// Array of unreachable entities

public:
	//-----------------------------------------------------
	//
	// Eye position, view offset, head direction, eye direction
	//
	//-----------------------------------------------------
	
	void				SetDefaultEyeOffset ( void );
	virtual Vector		EyeOffset( Activity nActivity );

	//---------------------------------
	
	virtual Vector		HeadDirection2D( void );
	virtual Vector		HeadDirection3D( void );
	virtual Vector		EyeDirection2D( void );
	virtual Vector		EyeDirection3D( void );

	virtual CBaseEntity *EyeLookTarget( void );		// Overridden by subclass to force look at an entity
	virtual void		AddLookTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp = 0.0 ) { };
	virtual void		AddLookTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 ) { };
	virtual void		SetHeadDirection( const Vector &vTargetPos, float flInterval );
	virtual void		MaintainLookTargets( float flInterval );
	virtual bool		ValidEyeTarget(const Vector &lookTargetPos);
	virtual void		AimGun();
	virtual void		SetAim( const Vector &aimDir );
	virtual	void		RelaxAim( void );

protected:
	Vector				m_vDefaultEyeOffset;
	float				m_flNextEyeLookTime;	// Next time a pick a new place to look
	
	float				m_flEyeIntegRate;		 // How fast does eye move to target

private:
	Vector				m_vEyeLookTarget;		 // Where I want to be looking
	Vector				m_vCurEyeTarget;		 // Direction I'm looking at
	EHANDLE				m_hEyeLookTarget;		 // What I want to be looking at
	float				m_flHeadYaw;			 // Current head yaw
	float				m_flHeadPitch;			 // Current head pitch

public:
	//-----------------------------------------------------
	//
	// Scripting
	//
	//-----------------------------------------------------

	// Scripted sequence Info
	enum SCRIPTSTATE
	{
		SCRIPT_PLAYING = 0,				// Playing the action animation.
		SCRIPT_WAIT,						// Waiting on everyone in the script to be ready. Plays the pre idle animation if there is one.
		SCRIPT_POST_IDLE,					// Playing the post idle animation after playing the action animation.
		SCRIPT_CLEANUP,					// Cancelling the script / cleaning up.
		SCRIPT_WALK_TO_MARK,				// Walking to the scripted sequence position.
		SCRIPT_RUN_TO_MARK,				// Running to the scripted sequence position.
		SCRIPT_CUSTOM_MOVE_TO_MARK,	// Moving to the scripted sequence position while playing a custom movement animation.
	};

	bool				ExitScriptedSequence();
	bool				CineCleanup();
	// forces movement and sets a new schedule
	bool				ScheduledMoveToGoalEntity( int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity );
	bool				ScheduledFollowPath( int scheduleType, CBaseEntity *pPathStart, Activity movementActivity );

	virtual float		PlayScene( const char *pszScene );
	bool				AutoMovement( void );

	SCRIPTSTATE			m_scriptState;		// internal cinematic state
	CHandle<CAI_ScriptedSequence>	m_hCine;
	Activity			m_ScriptArrivalActivity;
	string_t			m_strScriptArrivalSequence;

	CSceneEntity		*m_pScene;

public:
	//-----------------------------------------------------
	//
	// Memory
	//
	//-----------------------------------------------------

	inline void			Remember( int iMemory ) 		{ m_afMemory |= iMemory; }
	inline void			Forget( int iMemory ) 			{ m_afMemory &= ~iMemory; }
	inline bool			HasMemory( int iMemory ) 		{ if ( m_afMemory & iMemory ) return TRUE; return FALSE; }
	inline bool			HasAllMemories( int iMemory ) 	{ if ( (m_afMemory & iMemory) == iMemory ) return TRUE; return FALSE; }

	virtual CAI_Enemies *GetEnemies( void );
	virtual void		RemoveMemory( void );

	virtual bool		UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, bool firstHand = true );
	
	void				SetLastAttackTime( float time)	{ m_flLastAttackTime = time; }

protected:
	CAI_Enemies *		m_pEnemies;	// Holds information about enemies / danger positions / shared between sqaud members
	int					m_afMemory;
	EHANDLE				m_hEnemyOccluder;	// The entity my enemy is hiding behind.

	float				m_flSumDamage;				// How much consecutive damage I've received
	float				m_flLastDamageTime;			// Last time I received damage
	float				m_flLastAttackTime;			// Last time that I attacked my current enemy
	float				m_flNextWeaponSearchTime;	// next time to search for a better weapon

public:
	//-----------------------------------------------------
	//
	// Squads & tactics
	//
	//-----------------------------------------------------

	virtual bool		InitSquad( void );

	virtual const char*	SquadSlotName(int slotID)		{ return gm_SquadSlotNamespace.IdToSymbol(slotID);					}
	bool				OccupyStrategySlot( int squadSlotID );
	bool				OccupyStrategySlotRange( int slotIDStart, int slotIDEnd );
	bool				HasStrategySlot( int squadSlotID );
	bool				HasStrategySlotRange( int slotIDStart, int slotIDEnd );
	void				VacateStrategySlot( void );
	
	CAI_Squad *			GetSquad()						{ return m_pSquad; 		}
	void				SetSquad( CAI_Squad *pSquad)	{ m_pSquad = pSquad; 	}
	void				SetSquadName( string_t name )	{ m_SquadName = name; 	}

	string_t			GetHintGroup( void )			{ return m_strHintGroup;		}
	void				ClearHintGroup( void )			{ SetHintGroup( NULL_STRING );	}
	void				SetHintGroup( string_t name, bool bHintGroupNavLimiting = false );

	//---------------------------------

	CAI_TacticalServices *GetTacticalServices()			{ return m_pTacticalServices; }
	const CAI_TacticalServices *GetTacticalServices() const { return m_pTacticalServices; }

	//---------------------------------
	//  Cover
	
	virtual bool		FindCoverPos( CBaseEntity *pEntity, Vector *pResult);
	virtual bool		IsValidCover ( const Vector &vecCoverLocation, CAI_Hint const *pHint );
	virtual bool		IsValidShootPosition ( const Vector &vecCoverLocation, CAI_Hint const *pHint );
	virtual bool		IsCoverPosition( const Vector &vecThreat, const Vector &vecPosition );
	virtual float		CoverRadius( void ) { return 1024; } // Default cover radius

protected:
	virtual void		OnChangeHintGroup( string_t oldGroup, string_t newGroup ) {}

	CAI_Squad *			m_pSquad;		// The squad that I'm on
	string_t			m_SquadName;

	int					m_iMySquadSlot;	// this is the behaviour slot that the npc currently holds in the squad. 

private:
	string_t			m_strHintGroup;
	bool				m_bHintGroupNavLimiting;
	CAI_TacticalServices *m_pTacticalServices;

public:
	//-----------------------------------------------------
	//
	// Base schedule & task support; Miscellaneous
	//
	//-----------------------------------------------------

	void				InitRelationshipTable( void );
	void				AddRelationship( const char *pszRelationship, CBaseEntity *pActivator );

	void				NPCUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	CBaseGrenade*		IncomingGrenade(void);

	virtual bool		ShouldFadeOnDeath( void );

	void				NPCInitDead( void );	// Call after animation/pose is set up
	void				CorpseFallThink( void );

	float				ThrowLimit( const Vector &vecStart, const Vector &vecEnd, float fGravity, float fArcSize, const Vector &mins, const Vector &maxs, CBaseEntity *pTarget, Vector *jumpVel, CBaseEntity **pBlocker);

	// these functions will survey conditions and set appropriate conditions bits for attack types.
	virtual int			RangeAttack1Conditions( float flDot, float flDist );
	virtual int			RangeAttack2Conditions( float flDot, float flDist );
	virtual int			MeleeAttack1Conditions( float flDot, float flDist );
	virtual int			MeleeAttack2Conditions( float flDot, float flDist );

	virtual bool		OnBeginMoveAndShoot()	{ return true; }
	virtual void		OnEndMoveAndShoot()	{}
	//---------------------------------
	
	virtual	CBaseEntity *FindNamedEntity( const char *pszName );

	//---------------------------------
	//  States
	//---------------------------------

	virtual void		ClearAttackConditions( void );
	void				GatherAttackConditions( CBaseEntity *pTarget, float flDist );
	bool				Weapon_IsBetterAvailable ( void ) ;
	virtual bool		WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions);


	// override to change the chase location of an enemy
	// This is where your origin should go when you are chasing pEnemy when his origin is at chasePosition
	// by default, leave this alone to make your origin coincide with his.
	virtual void		TranslateEnemyChasePosition( CBaseEntity *pEnemy, Vector &chasePosition, float &tolerance );

	virtual bool		FCanCheckAttacks ( void );
	virtual void		CheckAmmo( void ) {}

	virtual bool		FValidateHintType( CAI_Hint *pHint );
	virtual Activity	GetHintActivity( short sHintType );
	virtual float		GetHintDelay( short sHintType );
	virtual Activity	GetCoverActivity( CAI_Hint* pHint );
	virtual Activity	GetReloadActivity( CAI_Hint* pHint );

	virtual Activity	GetFollowActivity( float flDistance );
	void				SetTurnActivity( void );

	// Returns the time when the door will be open
	float				OpenDoorAndWait( CBaseEntity *pDoor );

	bool				BBoxFlat( void );

	//---------------------------------

	virtual bool		PassesDamageFilter( CBaseEntity *pAttacker );

	//---------------------------------

	void				MakeDamageBloodDecal( int cCount, float flNoise, trace_t *ptr, Vector vecDir );
	void				TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void				DecalTrace( trace_t *pTrace, char const *decalName );
	void				ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );
	bool				PlayerInSpread( const Vector &sourcePos, const Vector &targetPos, float flSpread, bool ignoreHatedPlayers = true );
	CBaseEntity *		PlayerInRange( const Vector &vecLocation, float flDist );

	//---------------------------------
	// combat functions
	//---------------------------------
	virtual bool		InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );	

	Activity			GetSmallFlinchActivity( void );
	
	virtual bool		ShouldGib( const CTakeDamageInfo &info ) { return false; }	// Always ragdoll, unless specified by the leaf class
	virtual bool		Event_Gibbed( const CTakeDamageInfo &info );
	void				Event_Killed( const CTakeDamageInfo &info );

	virtual Vector		GetShootEnemyDir( const Vector &shootOrigin );
#ifdef HL2_DLL
	virtual Vector		GetActualShootTrajectory( const Vector &shootOrigin );
#endif //HL2_DLL
	virtual void		CollectShotStats( const Vector &vecShootOrigin, const Vector &vecShootDir );
	virtual Vector		BodyTarget( const Vector &posSrc, bool bNoisy = true );
	virtual void		FireBullets( int cShots, const Vector &vecSrc, const Vector &vecDirShooting, const Vector &vecSpread, float flDistance, int iBulletType, int iTracerFreq = 4, int firingEntID = -1, int attachmentID = -1,int iDamage = 0, CBaseEntity *pAttacker = NULL, bool bFirstShotAccurate = false );

	virtual	bool		ShouldMoveAndShoot();

	//---------------------------------
	//  Damage
	//---------------------------------
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual int			OnTakeDamage_Dying( const CTakeDamageInfo &info );
	virtual int			OnTakeDamage_Dead( const CTakeDamageInfo &info );
	virtual bool		IsLightDamage( float flDamage, int bitsDamageType );
	virtual bool		IsHeavyDamage( float flDamage, int bitsDamageType );

	void				DoRadiusDamage( const CTakeDamageInfo &info, int iClassIgnore );
	void				DoRadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, int iClassIgnore );

	//---------------------------------

	CBaseEntity*		DropItem( char *pszItemName, Vector vecPos, QAngle vecAng );// drop an item.

	//---------------------------------
	// Inputs
	//---------------------------------
	void InputSetRelationship( inputdata_t &inputdata );
	void InputSetHealth( inputdata_t &inputdata );

	//---------------------------------
	
	virtual void		NotifyDeadFriend( CBaseEntity *pFriend ) { return; };

	//---------------------------------
	// Utility methods
	static Vector CalcThrowVelocity(const Vector &startPos, const Vector &endPos, float fGravity, float fArcSize);

	//---------------------------------
	
	float				m_flWaitFinished;			// if we're told to wait, this is the time that the wait will be over.

	CAI_MoveAndShootOverlay m_MoveAndShootOverlay;

	Vector				m_vecLastPosition;			// npc sometimes wants to return to where it started after an operation.
	Vector				m_vSavePosition;			// position stored by code that called this schedules
	CAI_Hint*			m_pHintNode;				// this is the hint that the npc is moving towards or performing active idle on.

	int					m_cAmmoLoaded;				// how much ammo is in the weapon (used to trigger reload anim sequences)
	float				m_flDistTooFar;				// if enemy farther away than this, bits_COND_ENEMY_TOOFAR set in GatherEnemyConditions
	CBaseEntity			*m_pGoalEnt;				// path corner we are heading towards
	string_t			m_spawnEquipment;

	bool				m_fNoDamageDecal;

	EHANDLE				m_hStoredPathTarget;		// For TASK_SET_GOAL
	Vector				m_vecStoredPathGoal;		//
	GoalType_t			m_nStoredPathType;			// 
	int					m_fStoredPathFlags;			//

	bool				m_bDidDeathCleanup;


	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_lifeState );

	//---------------------------------
	//	Outputs
	//---------------------------------
	COutputEvent		m_OnDamaged;
	COutputEvent		m_OnDeath;
	COutputEvent		m_OnHalfHealth;
	COutputEHANDLE		m_OnFoundEnemy; 
	COutputEvent		m_OnLostEnemyLOS; 
	COutputEvent		m_OnLostEnemy; 
	COutputEHANDLE		m_OnFoundPlayer;
	COutputEvent		m_OnLostPlayerLOS;
	COutputEvent		m_OnLostPlayer; 
	COutputEvent		m_OnHearWorld;
	COutputEvent		m_OnHearPlayer;
	COutputEvent		m_OnHearCombat;

public:
	// use this to shrink the bbox temporarily
	void				SetHullSizeNormal( bool force = false );
	bool				SetHullSizeSmall( bool force = false );

	bool				IsUsingSmallHull() const	{ return m_fIsUsingSmallHull; }

	const Vector &		GetHullMins() const		{ return NAI_Hull::Mins(GetHullType()); }
	const Vector &		GetHullMaxs() const		{ return NAI_Hull::Maxs(GetHullType()); }
	float				GetHullWidth()	const	{ return NAI_Hull::Width(GetHullType()); }
	float				GetHullHeight() const	{ return NAI_Hull::Height(GetHullType()); }

	void				SetupVPhysicsHull();


private:
	void				TryRestoreHull( void );
	bool				m_fIsUsingSmallHull;

public:
	inline int UsableNPCObjectCaps( int baseCaps )
	{
		if ( IsAlive() )
			baseCaps |= FCAP_IMPULSE_USE;
		return baseCaps;
	}

	//-----------------------------------------------------
	//
	// Core mapped data structures 
	//
	// String Registries for default AI Shared by all CBaseNPCs
	//	These are used only during initialization and in debug
	//-----------------------------------------------------

	static void InitSchedulingTables();

	static CAI_GlobalScheduleNamespace *GetSchedulingSymbols()		{ return &gm_SchedulingSymbols; }
	static CAI_ClassScheduleIdSpace &AccessClassScheduleIdSpaceDirect() { return gm_ClassScheduleIdSpace; }
	virtual CAI_ClassScheduleIdSpace *	GetClassScheduleIdSpace()	{ return &gm_ClassScheduleIdSpace; }

	static int			GetScheduleID	(const char* schedName);
	static const char*	GetActivityName	(int actID);	

	static void			AddActivityToSR(const char *actName, int conID);

protected:
	static CAI_GlobalNamespace gm_SquadSlotNamespace;
	static CAI_LocalIdSpace    gm_SquadSlotIdSpace;

private:
	// Checks to see that the nav hull is valid for the NPC
	bool IsNavHullValid() const;

	friend class CAI_SchedulesManager;
	
	static bool			LoadDefaultSchedules(void);

	static void			InitDefaultScheduleSR(void);
	static void			InitDefaultTaskSR(void);
	static void			InitDefaultConditionSR(void);
	static void			InitDefaultActivitySR(void);
	static void			InitDefaultSquadSlotSR(void);

	static int			GetActivityID	(const char* actName);
	static int			GetConditionID	(const char* condName);
	static int			GetTaskID		(const char* taskName);
	static int			GetSquadSlotID	(const char* slotName);

	static CStringRegistry*	m_pActivitySR;
	static int			m_iNumActivities;
	
	static CAI_GlobalScheduleNamespace	gm_SchedulingSymbols;
	static CAI_ClassScheduleIdSpace		gm_ClassScheduleIdSpace;
	
public:
	//----------------------------------------------------
	// Debugging tools
	//
	
	// -----------------------------
	//  Debuging Fields and Methods
	// -----------------------------
	const char*			m_failText;					// Text of why it failed
	const char*			m_interruptText;			// Text of why schedule interrupted
	CAI_Schedule*		m_failedSchedule;			// The schedule that failed last
	CAI_Schedule*		m_interuptSchedule;			// The schedule that was interrupted last
	int					m_nDebugCurIndex;			// Index used for stepping through AI
	virtual void		ReportAIState( void );
	virtual void		ReportOverThinkLimit( float time );
	void 				DumpTaskTimings();
	void				DrawDebugGeometryOverlays(void);
	virtual int			DrawDebugTextOverlays(void);

	static void			ClearAllSchedules(void);

	static int			m_nDebugBits;

	static CAI_BaseNPC*	m_pDebugNPC;
	static int			m_nDebugPauseIndex;		// Current step
	static inline void	SetDebugNPC( CAI_BaseNPC *pNPC )  { m_pDebugNPC = pNPC; }
	static inline bool	IsDebugNPC( CAI_BaseNPC *pNPC ) { return( pNPC == m_pDebugNPC ); }

private:
	static void			ForceSelectedGo(CBaseEntity *pPlayer, const Vector &targetPos, const Vector &traceDir, bool bRun);
	static void			ForceSelectedGoRandom(void);

	void				ToggleFreeze(void);

	friend void 		CC_NPC_Go();
	friend void 		CC_NPC_GoRandom();
	friend void 		CC_NPC_Freeze();
};


//-----------------------------------------------------------------------------
// Purpose: Returns whether our ideal activity has started. If not, we are in
//			a transition sequence.
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsActivityStarted(void)
{
	return (GetSequence() == m_nIdealSequence);
}


typedef CHandle<CAI_BaseNPC> AIHANDLE;


// ============================================================================
//	Macros for introducing new schedules in sub-classes
//
// Strings registries and schedules use unique ID's for each item, but 
// sub-class enumerations are non-unique, so we translate between the 
// enumerations and unique ID's
// ============================================================================

#define AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( derivedClass ) \
	IMPLEMENT_CUSTOM_SCHEDULE_PROVIDER(derivedClass ) \
	void derivedClass::InitCustomSchedules( void ) \
	{ \
		typedef derivedClass CNpc; \
		const char *pszClassName = #derivedClass; \
		\
		CUtlVector<char *> schedulesToLoad; \
		CUtlVector<AIScheduleLoadFunc_t> reqiredOthers; \
		CAI_NamespaceInfos scheduleIds; \
		CAI_NamespaceInfos taskIds; \
		CAI_NamespaceInfos conditionIds;
		

//-----------------

#define AI_BEGIN_CUSTOM_NPC( className, derivedClass ) \
	IMPLEMENT_CUSTOM_AI(className, derivedClass ) \
	void derivedClass::InitCustomSchedules( void ) \
	{ \
		typedef derivedClass CNpc; \
		const char *pszClassName = #derivedClass; \
		\
		CUtlVector<char *> schedulesToLoad; \
		CUtlVector<AIScheduleLoadFunc_t> reqiredOthers; \
		CAI_NamespaceInfos scheduleIds; \
		CAI_NamespaceInfos taskIds; \
		CAI_NamespaceInfos conditionIds; \
		CAI_NamespaceInfos squadSlotIds;
		
//-----------------

#define EXTERN_SCHEDULE( id ) \
	scheduleIds.PushBack( #id, id ); \
	extern const char * g_psz##id; \
	schedulesToLoad.AddToTail( (char *)g_psz##id );

//-----------------

#define DEFINE_SCHEDULE( id, text ) \
	scheduleIds.PushBack( #id, id ); \
	char * g_psz##id = \
		"\n	Schedule" \
		"\n		" #id \
		text \
		"\n"; \
	schedulesToLoad.AddToTail( (char *)g_psz##id );
	
//-----------------

#define DECLARE_CONDITION( id ) \
	conditionIds.PushBack( #id, id );

//-----------------

#define DECLARE_TASK( id ) \
	taskIds.PushBack( #id, id );

//-----------------

#define DECLARE_ACTIVITY( id ) \
	ADD_CUSTOM_ACTIVITY( CNpc, id );

//-----------------

#define DECLARE_SQUADSLOT( id ) \
	squadSlotIds.PushBack( #id, id );

//-----------------

#define DECLARE_INTERACTION( interaction ) \
	ADD_CUSTOM_INTERACTION( interaction );

//-----------------

#define DECLARE_USES_SCHEDULE_PROVIDER( classname )	reqiredOthers.AddToTail( ScheduleLoadHelper(classname) );

//-----------------

// IDs are stored and then added in order due to constraints in the namespace implementation
#define AI_END_CUSTOM_SCHEDULE_PROVIDER() \
		\
		int i; \
		\
		CNpc::AccessClassScheduleIdSpaceDirect().Init( BaseClass::GetSchedulingSymbols(), &BaseClass::AccessClassScheduleIdSpaceDirect() ); \
		\
		scheduleIds.Sort(); \
		taskIds.Sort(); \
		conditionIds.Sort(); \
		\
		for ( i = 0; i < scheduleIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_SCHEDULE_NAMED( CNpc, scheduleIds[i].pszName, scheduleIds[i].localId );  \
		} \
		\
		for ( i = 0; i < taskIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_TASK_NAMED( CNpc, taskIds[i].pszName, taskIds[i].localId );  \
		} \
		\
		for ( i = 0; i < conditionIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_CONDITION_NAMED( CNpc, conditionIds[i].pszName, conditionIds[i].localId );  \
		} \
		\
		for ( i = 0; i < reqiredOthers.Count(); i++ ) \
		{ \
			(*reqiredOthers[i])();  \
		} \
		\
		for ( i = 0; i < schedulesToLoad.Count(); i++ ) \
		{ \
			if ( CNpc::gm_SchedLoadStatus.fValid ) \
			{ \
				CNpc::gm_SchedLoadStatus.fValid = g_AI_SchedulesManager.LoadSchedulesFromBuffer( pszClassName, schedulesToLoad[i], &AccessClassScheduleIdSpaceDirect() ); \
			} \
			else \
				break; \
		} \
	}

//-------------------------------------

// IDs are stored and then added in order due to constraints in the namespace implementation
#define AI_END_CUSTOM_NPC() \
		\
		int i; \
		\
		CNpc::AccessClassScheduleIdSpaceDirect().Init( BaseClass::GetSchedulingSymbols(), &BaseClass::AccessClassScheduleIdSpaceDirect() ); \
		CNpc::gm_SquadSlotIdSpace.Init( &BaseClass::gm_SquadSlotNamespace, &BaseClass::gm_SquadSlotIdSpace); \
		\
		scheduleIds.Sort(); \
		taskIds.Sort(); \
		conditionIds.Sort(); \
		squadSlotIds.Sort(); \
		\
		for ( i = 0; i < scheduleIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_SCHEDULE_NAMED( CNpc, scheduleIds[i].pszName, scheduleIds[i].localId );  \
		} \
		\
		for ( i = 0; i < taskIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_TASK_NAMED( CNpc, taskIds[i].pszName, taskIds[i].localId );  \
		} \
		\
		for ( i = 0; i < conditionIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_CONDITION_NAMED( CNpc, conditionIds[i].pszName, conditionIds[i].localId );  \
		} \
		\
		for ( i = 0; i < squadSlotIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_SQUADSLOT_NAMED( CNpc, squadSlotIds[i].pszName, squadSlotIds[i].localId );  \
		} \
		\
		for ( i = 0; i < reqiredOthers.Count(); i++ ) \
		{ \
			(*reqiredOthers[i])();  \
		} \
		\
		for ( i = 0; i < schedulesToLoad.Count(); i++ ) \
		{ \
			if ( CNpc::gm_SchedLoadStatus.fValid ) \
			{ \
				CNpc::gm_SchedLoadStatus.fValid = g_AI_SchedulesManager.LoadSchedulesFromBuffer( pszClassName, schedulesToLoad[i], &AccessClassScheduleIdSpaceDirect() ); \
			} \
			else \
				break; \
		} \
	}

//-------------------------------------

struct AI_NamespaceAddInfo_t
{
	AI_NamespaceAddInfo_t( const char *pszName, int localId )
	 :	pszName( pszName ),
		localId( localId )
	{
	}
	
	const char *pszName;
	int			localId;
};

class CAI_NamespaceInfos : public CUtlVector<AI_NamespaceAddInfo_t>
{
public:
	void PushBack(  const char *pszName, int localId )
	{
		AddToTail( AI_NamespaceAddInfo_t( pszName, localId ) );
	}

	void Sort()
	{
		if ( Count() )
			qsort( Base(), Count(), sizeof(AI_NamespaceAddInfo_t), Compare );
	}
	
private:
	static int Compare(const void *pLeft, const void *pRight )
	{
		return ((AI_NamespaceAddInfo_t *)pLeft)->localId - ((AI_NamespaceAddInfo_t *)pRight)->localId;
	}
	
};

//-------------------------------------

// Declares the static variables that hold the string registry offset for the new subclass
// as well as the initialization in schedule load functions

struct AI_SchedLoadStatus_t
{
	bool fValid;
	int  signature;
};

// Load schedules pulled out to support stepping through with debugger
inline bool AI_DoLoadSchedules( bool (*pfnBaseLoad)(), void (*pfnInitCustomSchedules)(),
								AI_SchedLoadStatus_t *pLoadStatus )
{
	(*pfnBaseLoad)();
	
	if (pLoadStatus->signature != g_AI_SchedulesManager.GetScheduleLoadSignature())
	{
		(*pfnInitCustomSchedules)();
		pLoadStatus->fValid	   = true;
		pLoadStatus->signature = g_AI_SchedulesManager.GetScheduleLoadSignature();
	}
	return pLoadStatus->fValid;
}

//-------------------------------------

typedef bool (*AIScheduleLoadFunc_t)();

// @Note (toml 02-16-03): The following class exists to allow us to establish an anonymous friendship
// in DEFINE_CUSTOM_SCHEDULE_PROVIDER. The particulars of this implementation is almost entirely
// defined by bugs in MSVC 6.0
class ScheduleLoadHelperImpl
{
public:
	template <typename T> 
	static AIScheduleLoadFunc_t AccessScheduleLoadFunc(T *)
	{
		return (&T::LoadSchedules);
	}
};

#define ScheduleLoadHelper( type ) (ScheduleLoadHelperImpl::AccessScheduleLoadFunc((type *)0))

//-------------------------------------

#define DEFINE_CUSTOM_SCHEDULE_PROVIDER\
	static AI_SchedLoadStatus_t 		gm_SchedLoadStatus; \
	static CAI_ClassScheduleIdSpace 	gm_ClassScheduleIdSpace; \
	static const char *					gm_pszErrorClassName;\
	\
	static CAI_ClassScheduleIdSpace &	AccessClassScheduleIdSpaceDirect() 	{ return gm_ClassScheduleIdSpace; } \
	virtual CAI_ClassScheduleIdSpace *	GetClassScheduleIdSpace()			{ return &gm_ClassScheduleIdSpace; } \
	virtual const char *				GetSchedulingErrorName()			{ return gm_pszErrorClassName; } \
	\
	static void							InitCustomSchedules(void);\
	\
	static bool							LoadSchedules(void);\
	virtual bool						LoadedSchedules(void); \
	\
	friend class ScheduleLoadHelperImpl;	\
	\
	class CScheduleLoader \
	{ \
	public: \
		CScheduleLoader(); \
	} m_ScheduleLoader; \
	\
	friend class CScheduleLoader;

//-------------------------------------

#define DEFINE_CUSTOM_AI\
	DEFINE_CUSTOM_SCHEDULE_PROVIDER \
	\
	static CAI_LocalIdSpace gm_SquadSlotIdSpace; \
	\
	const char*				SquadSlotName	(int squadSlotID);

//-------------------------------------

#define IMPLEMENT_CUSTOM_SCHEDULE_PROVIDER(derivedClass)\
	AI_SchedLoadStatus_t		derivedClass::gm_SchedLoadStatus = { true, -1 }; \
	CAI_ClassScheduleIdSpace 	derivedClass::gm_ClassScheduleIdSpace; \
	const char *				derivedClass::gm_pszErrorClassName = #derivedClass; \
	\
	derivedClass::CScheduleLoader::CScheduleLoader()\
	{ \
		derivedClass::LoadSchedules(); \
	} \
	\
	/* --------------------------------------------- */ \
	/* Load schedules for this type of NPC           */ \
	/* --------------------------------------------- */ \
	bool derivedClass::LoadSchedules(void)\
	{\
		return AI_DoLoadSchedules( derivedClass::BaseClass::LoadSchedules, \
								   derivedClass::InitCustomSchedules, \
								   &derivedClass::gm_SchedLoadStatus ); \
	}\
	\
	bool derivedClass::LoadedSchedules(void) \
	{ \
		return derivedClass::gm_SchedLoadStatus.fValid;\
	} 


//-------------------------------------

// Initialize offsets and implement methods for loading and getting squad info for the subclass
#define IMPLEMENT_CUSTOM_AI(className, derivedClass)\
	IMPLEMENT_CUSTOM_SCHEDULE_PROVIDER(derivedClass)\
	\
	CAI_LocalIdSpace 	derivedClass::gm_SquadSlotIdSpace; \
	\
	/* -------------------------------------------------- */ \
	/* Given squadSlot enumeration return squadSlot name */ \
	/* -------------------------------------------------- */ \
	const char* derivedClass::SquadSlotName(int slotEN)\
	{\
		return gm_SquadSlotNamespace.IdToSymbol( derivedClass::gm_SquadSlotIdSpace.LocalToGlobal(slotEN) );\
	}


//-------------------------------------

#define ADD_CUSTOM_SCHEDULE_NAMED(derivedClass,schedName,schedEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddSchedule( schedName, schedEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_SCHEDULE(derivedClass,schedEN) ADD_CUSTOM_SCHEDULE_NAMED(derivedClass,#schedEN,schedEN)

#define ADD_CUSTOM_TASK_NAMED(derivedClass,taskName,taskEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddTask( taskName, taskEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_TASK(derivedClass,taskEN) ADD_CUSTOM_TASK_NAMED(derivedClass,#taskEN,taskEN)

#define ADD_CUSTOM_CONDITION_NAMED(derivedClass,condName,condEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddCondition( condName, condEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_CONDITION(derivedClass,condEN) ADD_CUSTOM_CONDITION_NAMED(derivedClass,#condEN,condEN)

//-------------------------------------

#define INIT_CUSTOM_AI(derivedClass)\
	derivedClass::AccessClassScheduleIdSpaceDirect().Init( BaseClass::GetSchedulingSymbols(), &BaseClass::AccessClassScheduleIdSpaceDirect() ); \
	derivedClass::gm_SquadSlotIdSpace.Init( &CAI_BaseNPC::gm_SquadSlotNamespace, &BaseClass::gm_SquadSlotIdSpace);

#define	ADD_CUSTOM_INTERACTION( interaction )	{ interaction = CBaseCombatCharacter::GetInteractionID(); }

#define ADD_CUSTOM_SQUADSLOT_NAMED(derivedClass,squadSlotName,squadSlotEN)\
	if ( !derivedClass::gm_SquadSlotIdSpace.AddSymbol( squadSlotName, squadSlotEN, "squadslot", derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_SQUADSLOT(derivedClass,squadSlotEN) ADD_CUSTOM_SQUADSLOT_NAMED(derivedClass,#squadSlotEN,squadSlotEN)

#define ADD_CUSTOM_ACTIVITY_NAMED(derivedClass,activityName,activityEnum)\
	REGISTER_PRIVATE_ACTIVITY(activityEnum);\
	CAI_BaseNPC::AddActivityToSR(activityName,activityEnum);

#define ADD_CUSTOM_ACTIVITY(derivedClass,activityEnum) ADD_CUSTOM_ACTIVITY_NAMED(derivedClass,#activityEnum,activityEnum)


//=============================================================================
// class CAI_Component
//=============================================================================

inline const Vector &CAI_Component::GetLocalOrigin() const
{
	return GetOuter()->GetLocalOrigin();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::SetLocalOrigin(const Vector &origin)
{
	GetOuter()->SetLocalOrigin(origin);
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::GetAbsOrigin() const
{
	return GetOuter()->GetAbsOrigin();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::SetSolid( SolidType_t val )
{
	GetOuter()->SetSolid(val);
}

//-----------------------------------------------------------------------------

inline SolidType_t CAI_Component::GetSolid() const
{
	return GetOuter()->GetSolid();
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::WorldAlignMins() const
{
	return GetOuter()->WorldAlignMins();
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::WorldAlignMaxs() const
{
	return GetOuter()->WorldAlignMaxs();
}
	
//-----------------------------------------------------------------------------

inline Hull_t CAI_Component::GetHullType() const
{
	return GetOuter()->GetHullType();
}

//-----------------------------------------------------------------------------

inline Vector CAI_Component::WorldSpaceCenter() const
{
	return GetOuter()->WorldSpaceCenter();
}

//-----------------------------------------------------------------------------

inline float CAI_Component::GetGravity() const
{
	return GetOuter()->GetGravity();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::SetGravity( float flGravity )
{
	GetOuter()->SetGravity( flGravity );
}

//-----------------------------------------------------------------------------

inline float CAI_Component::GetHullWidth() const
{
	return NAI_Hull::Width(GetOuter()->GetHullType());
}

//-----------------------------------------------------------------------------

inline float CAI_Component::GetHullHeight() const
{
	return NAI_Hull::Height(GetOuter()->GetHullType());
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::GetHullMins() const
{
	return NAI_Hull::Mins(GetOuter()->GetHullType());
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::GetHullMaxs() const
{
	return NAI_Hull::Maxs(GetOuter()->GetHullType());
}

//-----------------------------------------------------------------------------

inline int CAI_Component::GetCollisionGroup() const
{
	return GetOuter()->GetCollisionGroup();
}

//-----------------------------------------------------------------------------

inline CBaseEntity *CAI_Component::GetEnemy()
{
	return GetOuter()->GetEnemy();
}

//-----------------------------------------------------------------------------

inline const Vector &CAI_Component::GetEnemyLKP() const
{
	return GetOuter()->GetEnemyLKP();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::TranslateEnemyChasePosition( CBaseEntity *pEnemy, Vector &chasePosition, float &tolerance )
{
	GetOuter()->TranslateEnemyChasePosition( pEnemy, chasePosition, tolerance );
}

//-----------------------------------------------------------------------------

inline CBaseEntity *CAI_Component::GetTarget()
{
	return GetOuter()->GetTarget();
}
//-----------------------------------------------------------------------------

inline void CAI_Component::SetTarget( CBaseEntity *pTarget )
{
	GetOuter()->SetTarget( pTarget );
}

//-----------------------------------------------------------------------------

inline const Task_t *CAI_Component::GetCurTask()
{
	return GetOuter()->GetTask();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::TaskFail( AI_TaskFailureCode_t code )
{
	GetOuter()->TaskFail( code );
}
//-----------------------------------------------------------------------------

inline void CAI_Component::TaskFail( const char *pszGeneralFailText )
{
	GetOuter()->TaskFail( pszGeneralFailText );
}
//-----------------------------------------------------------------------------

inline void CAI_Component::TaskComplete( bool fIgnoreSetFailedCondition )
{
	GetOuter()->TaskComplete( fIgnoreSetFailedCondition );
}
//-----------------------------------------------------------------------------

inline int CAI_Component::TaskIsRunning()
{
	return GetOuter()->TaskIsRunning();
}
//-----------------------------------------------------------------------------

inline int CAI_Component::TaskIsComplete()
{
	return GetOuter()->TaskIsComplete();
}

//-----------------------------------------------------------------------------

inline Activity CAI_Component::GetActivity()
{
	return GetOuter()->GetActivity();
}
//-----------------------------------------------------------------------------

inline void CAI_Component::SetActivity( Activity NewActivity )
{
	GetOuter()->SetActivity( NewActivity );
}
//-----------------------------------------------------------------------------

inline float CAI_Component::GetIdealSpeed() const
{
	return GetOuter()->GetIdealSpeed();
}
//-----------------------------------------------------------------------------

inline int CAI_Component::GetSequence()
{
	return GetOuter()->GetSequence();
}

//-----------------------------------------------------------------------------

inline int CAI_Component::GetEntFlags() const
{
	return GetOuter()->GetFlags();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::AddEntFlag( int flags )
{
	GetOuter()->AddFlag( flags );
}

//-----------------------------------------------------------------------------

inline void CAI_Component::RemoveEntFlag( int flagsToRemove )
{
	GetOuter()->RemoveFlag( flagsToRemove );
}

//-----------------------------------------------------------------------------

inline void CAI_Component::ToggleEntFlag( int flagToToggle )
{
	GetOuter()->ToggleFlag( flagToToggle );
}

//-----------------------------------------------------------------------------

inline CBaseEntity* CAI_Component::GetGoalEnt()
{
	return GetOuter()->m_pGoalEnt;
}
//-----------------------------------------------------------------------------

inline void CAI_Component::SetGoalEnt( CBaseEntity *pGoalEnt )
{
	GetOuter()->m_pGoalEnt = pGoalEnt;
}

//-----------------------------------------------------------------------------

inline void CAI_Component::Remember( int iMemory )
{
	GetOuter()->Remember( iMemory );
}
//-----------------------------------------------------------------------------

inline void CAI_Component::Forget( int iMemory )
{
	GetOuter()->Forget( iMemory );
}
//-----------------------------------------------------------------------------

inline bool CAI_Component::HasMemory( int iMemory )
{
	return GetOuter()->HasMemory( iMemory );
}

//-----------------------------------------------------------------------------

inline CAI_Enemies *CAI_Component::GetEnemies()
{
	return GetOuter()->GetEnemies();
}

//-----------------------------------------------------------------------------

inline const char *CAI_Component::GetEntClassname()
{
	return GetOuter()->GetClassname();
}

//-----------------------------------------------------------------------------

inline int CAI_Component::CapabilitiesGet()
{
	return GetOuter()->CapabilitiesGet();
}

//-----------------------------------------------------------------------------

inline void CAI_Component::SetLocalAngles( const QAngle& angles )
{
	GetOuter()->SetLocalAngles( angles );
}

//-----------------------------------------------------------------------------

inline const QAngle &CAI_Component::GetLocalAngles( void ) const
{
	return GetOuter()->GetLocalAngles();
}

//-----------------------------------------------------------------------------

inline edict_t *CAI_Component::GetEdict() const
{
	return GetOuter()->GetEdict();
}

//-----------------------------------------------------------------------------

inline float CAI_Component::GetLastThink( char *szContext )
{
	return GetOuter()->GetLastThink( szContext );
}

// ============================================================================

#endif // AI_BASENPC_H
