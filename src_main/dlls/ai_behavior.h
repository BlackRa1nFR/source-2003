//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_BEHAVIOR_H
#define AI_BEHAVIOR_H

#include "ai_component.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "networkvar.h"

#ifdef DEBUG
#pragma warning(push)
#include <typeinfo>
#pragma warning(pop)
#pragma warning(disable:4290)
#endif

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// CAI_Behavior...
//
// Purpose:	The core component that defines a behavior in an NPC by selecting
//			schedules and running tasks
//
//			Intended to be used as an organizational tool as well as a way
//			for various NPCs to share behaviors without sharing an inheritance
//			relationship, and without cramming those behaviors into the base
//			NPC class.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Base class defines interface to behaviors and provides bridging
//			methods
//-----------------------------------------------------------------------------

class IBehaviorBackBridge;

//-------------------------------------

class CAI_BehaviorBase : public CAI_Component
{
public:
	CAI_BehaviorBase(CAI_BaseNPC *pOuter = NULL)
	 : 	CAI_Component(pOuter),
	 	m_pBackBridge(NULL)
	{
	}

	virtual const char *GetName() = 0;

	bool IsRunning()								{ return ( GetOuter()->GetRunningBehavior() == this ); }
	virtual bool CanSelectSchedule()				{ return true; }
	virtual void BeginScheduleSelection() 			{}
	virtual void EndScheduleSelection() 			{}
	
	void SetBackBridge( IBehaviorBackBridge *pBackBridge )
	{
		Assert( m_pBackBridge == NULL || pBackBridge == NULL );
		m_pBackBridge = pBackBridge;
	}

	void BridgePrecache()							{ Precache();		}
	void BridgeSpawn()								{ Spawn();			}
	void BridgeUpdateOnRemove()						{ UpdateOnRemove();	}

	void BridgeOnChangeHintGroup( string_t oldGroup, string_t newGroup ) { 	OnChangeHintGroup( oldGroup, newGroup ); }

	void BridgeGatherConditions()					{ GatherConditions(); }
	void BridgePrescheduleThink()					{ PrescheduleThink(); }
	void BridgeOnScheduleChange()					{ OnScheduleChange(); }

	bool BridgeSelectSchedule( int *pResult);
	bool BridgeSelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode, int *pResult );
	bool BridgeStartTask( const Task_t *pTask );
	bool BridgeRunTask( const Task_t *pTask);
	int BridgeTranslateSchedule( int scheduleType );
	bool BridgeGetSchedule( int localScheduleID, CAI_Schedule **ppResult );
	bool BridgeTaskName(int taskID, const char **);
	Activity BridgeNPC_TranslateActivity( Activity activity );
	void BridgeBuildScheduleTestBits( void )		{ BuildScheduleTestBits(); }
	bool BridgeIsCurTaskContinuousMove( bool *pResult );
	bool BridgeFValidateHintType( CAI_Hint *pHint, bool *pResult );
	bool BridgeIsValidEnemy( CBaseEntity *pEnemy );
	bool BridgeIsValidCover( const Vector &vLocation, CAI_Hint const *pHint );
	bool BridgeIsValidShootPosition( const Vector &vLocation, CAI_Hint const *pHint );
	bool BridgeShouldAlwaysThink( bool *pResult );
	void BridgeOnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );
	void BridgeOnRestore();

	virtual CAI_ClassScheduleIdSpace *GetClassScheduleIdSpace();

	virtual int	Save( ISave &save );
	virtual int	Restore( IRestore &restore );

	static void SaveBehaviors(ISave &save, CAI_BehaviorBase *pCurrentBehavior, CAI_BehaviorBase **ppBehavior, int nBehaviors );
	static int RestoreBehaviors(IRestore &restore, CAI_BehaviorBase **ppBehavior, int nBehaviors ); // returns index of "current" behavior, or -1

protected:

	int GetNpcState() { return GetOuter()->m_NPCState; }

	virtual void Precache()							{}
	virtual void Spawn()							{}
	virtual void UpdateOnRemove()					{}
	
	virtual void GatherConditions();
	virtual void PrescheduleThink();
	virtual void OnScheduleChange();

	virtual int SelectSchedule();
	virtual int	SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );
	virtual int TranslateSchedule( int scheduleType );
	virtual CAI_Schedule *GetSchedule(int schedule);
	virtual const char *GetSchedulingErrorName();
	virtual void BuildScheduleTestBits() {}

protected:
	// Used by derived classes to chain a task to a task that might not be the 
	// one they are currently handling:
	void	ChainStartTask( int task, float taskData = 0 );
	void	ChainRunTask( int task, float taskData = 0 );

protected:

	virtual Activity NPC_TranslateActivity( Activity activity );

	virtual bool IsCurTaskContinuousMove();
	virtual bool FValidateHintType( CAI_Hint *pHint );

	virtual	bool IsValidEnemy( CBaseEntity *pEnemy );
	virtual	bool IsValidCover( const Vector &vLocation, CAI_Hint const *pHint );
	virtual	bool IsValidShootPosition( const Vector &vLocation, CAI_Hint const *pHint );

	virtual bool ShouldAlwaysThink();

	virtual void OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon ) {};
	
	virtual void OnRestore() {};
	
	bool NotifyChangeBehaviorStatus( bool fCanFinishSchedule = false );

	bool HaveSequenceForActivity( Activity activity )		{ return GetOuter()->HaveSequenceForActivity( activity ); }
	
	//---------------------------------

	string_t			GetHintGroup( void )			{ return GetOuter()->GetHintGroup();	}
	void				ClearHintGroup( void )			{ GetOuter()->ClearHintGroup();			}
	void				SetHintGroup( string_t name )	{ GetOuter()->SetHintGroup( name );		}

	virtual void		OnChangeHintGroup( string_t oldGroup, string_t newGroup ) {}

	//
	// These allow derived classes to implement custom schedules
	//
	static CAI_GlobalScheduleNamespace *GetSchedulingSymbols()		{ return CAI_BaseNPC::GetSchedulingSymbols(); }
	static bool				LoadSchedules()							{ return true; }
	virtual bool			IsBehaviorSchedule( int scheduleType )	{ return false; }

	CAI_Navigator *			GetNavigator() 							{ return GetOuter()->GetNavigator(); 		}
	CAI_Motor *				GetMotor() 								{ return GetOuter()->GetMotor(); 			}
	CAI_TacticalServices *	GetTacticalServices()					{ return GetOuter()->GetTacticalServices();	}

	bool 				 m_fOverrode;
	IBehaviorBackBridge *m_pBackBridge;

	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------
// Purpose: Template provides provides back bridge to owning class and 
//			establishes namespace settings
//-----------------------------------------------------------------------------

template <class NPC_CLASS = CAI_BaseNPC, const int ID_SPACE_OFFSET = 100000>
class CAI_Behavior : public CAI_ComponentWithOuter<NPC_CLASS, CAI_BehaviorBase>
{
public:
	DECLARE_CLASS_NOFRIEND( CAI_Behavior, NPC_CLASS );
	
	enum
	{
		NEXT_TASK 			= ID_SPACE_OFFSET,
		NEXT_SCHEDULE 		= ID_SPACE_OFFSET,
		NEXT_CONDITION 		= ID_SPACE_OFFSET
	};

	void SetCondition( int condition )
	{
		if ( condition >= ID_SPACE_OFFSET && condition < ID_SPACE_OFFSET + 10000 ) // it's local to us
			condition = GetClassScheduleIdSpace()->ConditionLocalToGlobal( condition );
		GetOuter()->SetCondition( condition );
	}

	bool HasCondition( int condition )
	{
		if ( condition >= ID_SPACE_OFFSET && condition < ID_SPACE_OFFSET + 10000 ) // it's local to us
			condition = GetClassScheduleIdSpace()->ConditionLocalToGlobal( condition );
		return GetOuter()->HasCondition( condition );
	}

	bool HasInterruptCondition( int condition )
	{
		if ( condition >= ID_SPACE_OFFSET && condition < ID_SPACE_OFFSET + 10000 ) // it's local to us
			condition = GetClassScheduleIdSpace()->ConditionLocalToGlobal( condition );
		return GetOuter()->HasInterruptCondition( condition );
	}

	void ClearCondition( int condition )
	{
		if ( condition >= ID_SPACE_OFFSET && condition < ID_SPACE_OFFSET + 10000 ) // it's local to us
			condition = GetClassScheduleIdSpace()->ConditionLocalToGlobal( condition );
		GetOuter()->ClearCondition( condition );
	}

protected:
	CAI_Behavior(NPC_CLASS *pOuter = NULL)
	 : CAI_ComponentWithOuter<NPC_CLASS, CAI_BehaviorBase>(pOuter)
	{
	}

	static CAI_GlobalScheduleNamespace *GetSchedulingSymbols()
	{
		return NPC_CLASS::GetSchedulingSymbols();
	}
	virtual CAI_ClassScheduleIdSpace *GetClassScheduleIdSpace()
	{
		return GetOuter()->GetClassScheduleIdSpace();
	}

	static CAI_ClassScheduleIdSpace &AccessClassScheduleIdSpaceDirect()
	{
		return NPC_CLASS::AccessClassScheduleIdSpaceDirect();
	}

private:
	virtual bool IsBehaviorSchedule( int scheduleType ) { return ( scheduleType >= ID_SPACE_OFFSET && scheduleType < ID_SPACE_OFFSET + 10000 ); }
};


//-----------------------------------------------------------------------------
// Purpose: Some bridges a little more complicated to allow behavior to see 
//			what base class would do or control order in which it's donw
//-----------------------------------------------------------------------------

class IBehaviorBackBridge
{
public:
	virtual void 		BackBridge_GatherConditions() = 0;
	virtual int 		BackBridge_TranslateSchedule( int scheduleType ) = 0;
	virtual Activity 	BackBridge_NPC_TranslateActivity( Activity activity ) = 0;
	virtual bool		BackBridge_IsValidEnemy(CBaseEntity *pEnemy) = 0;
	virtual bool		BackBridge_IsValidCover( const Vector &vLocation, CAI_Hint const *pHint ) = 0;
	virtual bool		BackBridge_IsValidShootPosition( const Vector &vLocation, CAI_Hint const *pHint ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: The common instantiation of the above template
//-----------------------------------------------------------------------------

typedef CAI_Behavior<> CAI_SimpleBehavior;

//-----------------------------------------------------------------------------
// Purpose: Base class for AIs that want to act as a host for CAI_Behaviors
//			NPCs aren't required to use this, but probably want to.
//-----------------------------------------------------------------------------

template <class BASE_NPC>
class CAI_BehaviorHost : public BASE_NPC,
						 private IBehaviorBackBridge
{
public:
	DECLARE_CLASS_NOFRIEND( CAI_BehaviorHost, BASE_NPC );

	CAI_BehaviorHost()
	  : m_pCurBehavior(NULL)
	{
#ifdef DEBUG
  		m_fDebugInCreateBehaviors = false;
#endif
	}

	virtual void CleanupOnDeath( CBaseEntity *pCulprit = NULL )
	{
		DeferSchedulingToBehavior( NULL );
		BaseClass::CleanupOnDeath( pCulprit );
	}

	virtual int		Save( ISave &save );
	virtual int		Restore( IRestore &restore );
	virtual bool 	CreateComponents();

	// Automatically called during entity construction, derived class calls AddBehavior()
	virtual bool 	CreateBehaviors()	{ return true; }
	
	// Bridges
	void			Precache();
	void			NPCInit();
	void			UpdateOnRemove();
	void 			GatherConditions();
	void 			PrescheduleThink();
	int 			SelectSchedule();
	int				SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	void 			OnScheduleChange();
	int 			TranslateSchedule( int scheduleType );
	void 			StartTask( const Task_t *pTask );
	void 			RunTask( const Task_t *pTask );
	CAI_Schedule *	GetSchedule(int localScheduleID);
	const char *	TaskName(int taskID);
	void			BuildScheduleTestBits( void );

	void			OnChangeHintGroup( string_t oldGroup, string_t newGroup );
	
	Activity 		NPC_TranslateActivity( Activity activity );

	bool			IsCurTaskContinuousMove();
	bool			FValidateHintType( CAI_Hint *pHint );

	bool			IsValidEnemy(CBaseEntity *pEnemy);
	bool			IsValidCover( const Vector &vLocation, CAI_Hint const *pHint );
	bool			IsValidShootPosition( const Vector &vLocation, CAI_Hint const *pHint );
	
	bool			ShouldAlwaysThink();

	void			OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );

	void			OnRestore();

	virtual bool	OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule );
	virtual void	OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior );

protected:
	void			AddBehavior( CAI_BehaviorBase *pBehavior );
	
	bool			BehaviorSelectSchedule();

	bool 			IsRunningBehavior() const;
	CAI_BehaviorBase *GetRunningBehavior();
	CAI_BehaviorBase *DeferSchedulingToBehavior( CAI_BehaviorBase *pNewBehavior );

private:
	void 			BackBridge_GatherConditions();
	int				BackBridge_TranslateSchedule( int scheduleType );
	Activity		BackBridge_NPC_TranslateActivity( Activity activity );
	bool			BackBridge_IsValidEnemy(CBaseEntity *pEnemy);
	bool			BackBridge_IsValidCover( const Vector &vLocation, CAI_Hint const *pHint );
	bool			BackBridge_IsValidShootPosition( const Vector &vLocation, CAI_Hint const *pHint );

	CAI_BehaviorBase **AccessBehaviors();
	int				NumBehaviors();

	CAI_BehaviorBase *			   m_pCurBehavior;
	CUtlVector<CAI_BehaviorBase *> m_Behaviors;
	
#ifdef DEBUG
	bool 			m_fDebugInCreateBehaviors;
#endif
	
};

//-----------------------------------------------------------------------------

inline bool CAI_BehaviorBase::BridgeSelectSchedule( int *pResult )
{
	m_fOverrode = true;
	int result = SelectSchedule();
	if ( m_fOverrode )
	{
		if ( result != SCHED_NONE )
		{
			if ( IsBehaviorSchedule( result ) )
				*pResult = GetClassScheduleIdSpace()->ScheduleLocalToGlobal( result );
			else
				*pResult = result;
			return true;
		}
		Warning( "An AI behavior is in control but has no recommended schedule\n" );
	}
	return false;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeSelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode, int *pResult )
{
	m_fOverrode = true;
	int result = SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
	if ( m_fOverrode )
	{
		if ( result != SCHED_NONE )
		{
			if ( IsBehaviorSchedule( result ) )
				*pResult = GetClassScheduleIdSpace()->ScheduleLocalToGlobal( result );
			else
				*pResult = result;
			return true;
		}
		Warning( "An AI behavior is in control but has no recommended schedule\n" );
	}
	return false;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeStartTask( const Task_t *pTask )
{
	m_fOverrode = true;
	StartTask( pTask );
	return m_fOverrode;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeRunTask( const Task_t *pTask)
{
	m_fOverrode = true;
	RunTask( pTask );
	return m_fOverrode;
}

//-------------------------------------

inline void CAI_BehaviorBase::ChainStartTask( int task, float taskData )
{
	Task_t tempTask = { task, taskData }; 

	bool fPrevOverride = m_fOverrode;
	GetOuter()->StartTask( (const Task_t *)&tempTask );
	m_fOverrode = fPrevOverride;;
}

//-------------------------------------

inline void CAI_BehaviorBase::ChainRunTask( int task, float taskData )
{ 
	Task_t tempTask = { task, taskData }; 
	bool fPrevOverride = m_fOverrode;
	GetOuter()->RunTask( (const Task_t *)	&tempTask );
	m_fOverrode = fPrevOverride;;
}



//-------------------------------------

inline int CAI_BehaviorBase::BridgeTranslateSchedule( int scheduleType )
{
	int localId = AI_IdIsGlobal( scheduleType ) ? GetClassScheduleIdSpace()->ScheduleGlobalToLocal( scheduleType ) : scheduleType;
	int result = TranslateSchedule( localId );
	
	return result;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeGetSchedule( int localScheduleID, CAI_Schedule **ppResult )
{
	*ppResult = GetSchedule( localScheduleID );
	return (*ppResult != NULL );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeTaskName( int taskID, const char **ppResult )
{
	if ( AI_IdIsLocal( taskID ) )
	{
		*ppResult = GetSchedulingSymbols()->TaskIdToSymbol( GetClassScheduleIdSpace()->TaskLocalToGlobal( taskID ) );
		return (*ppResult != NULL );
	}
	return false;
}

//-------------------------------------

inline Activity CAI_BehaviorBase::BridgeNPC_TranslateActivity( Activity activity )
{
	return NPC_TranslateActivity( activity );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsCurTaskContinuousMove( bool *pResult )
{
	bool fPrevOverride = m_fOverrode;
	m_fOverrode = true;
	*pResult = IsCurTaskContinuousMove();
	bool result = m_fOverrode;
	m_fOverrode = fPrevOverride;
	return result;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeFValidateHintType( CAI_Hint *pHint, bool *pResult )
{
	bool fPrevOverride = m_fOverrode;
	m_fOverrode = true;
	*pResult = FValidateHintType( pHint );
	bool result = m_fOverrode;
	m_fOverrode = fPrevOverride;
	return result;
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsValidEnemy( CBaseEntity *pEnemy )
{
	return IsValidEnemy( pEnemy );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsValidCover( const Vector &vLocation, CAI_Hint const *pHint )
{
	return IsValidCover( vLocation, pHint );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeIsValidShootPosition( const Vector &vLocation, CAI_Hint const *pHint )
{
	return IsValidShootPosition( vLocation, pHint );
}

//-------------------------------------

inline bool CAI_BehaviorBase::BridgeShouldAlwaysThink( bool *pResult )
{
	bool fPrevOverride = m_fOverrode;
	m_fOverrode = true;
	*pResult = ShouldAlwaysThink();
	bool result = m_fOverrode;
	m_fOverrode = fPrevOverride;
	return result;
}

//-------------------------------------

inline void CAI_BehaviorBase::BridgeOnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
}

//-------------------------------------

inline void CAI_BehaviorBase::BridgeOnRestore()
{
	OnRestore();
}


//-----------------------------------------------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::GatherConditions()					
{ 
	if ( m_pCurBehavior )
		m_pCurBehavior->BridgeGatherConditions(); 
	else
		BaseClass::GatherConditions();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::BackBridge_GatherConditions()
{
	BaseClass::GatherConditions();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnScheduleChange()
{ 
	if ( m_pCurBehavior )
		m_pCurBehavior->BridgeOnScheduleChange(); 
	BaseClass::OnScheduleChange();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BehaviorSelectSchedule()
{
	for ( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		if ( m_Behaviors[i]->CanSelectSchedule() )
		{
			DeferSchedulingToBehavior( m_Behaviors[i] );
			return true;
		}
	}
	
	DeferSchedulingToBehavior( NULL );
	return false;
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsRunningBehavior() const
{
	return ( m_pCurBehavior != NULL );
}

//-------------------------------------

template <class BASE_NPC>
inline CAI_BehaviorBase *CAI_BehaviorHost<BASE_NPC>::GetRunningBehavior()
{
	return m_pCurBehavior;
}

//-------------------------------------

template <class BASE_NPC>
inline CAI_BehaviorBase *CAI_BehaviorHost<BASE_NPC>::DeferSchedulingToBehavior( CAI_BehaviorBase *pNewBehavior )
{
	bool change = ( m_pCurBehavior != pNewBehavior);
	CAI_BehaviorBase *pOldBehavior = m_pCurBehavior;
	m_pCurBehavior = pNewBehavior;
	
	if ( change ) 
	{
		if ( m_pCurBehavior )
			m_pCurBehavior->BeginScheduleSelection();
		if ( pOldBehavior )
		{
			pOldBehavior->EndScheduleSelection();
			VacateStrategySlot();
		}

		OnChangeRunningBehavior( pOldBehavior, pNewBehavior );
	}
	
	return pOldBehavior;
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::BackBridge_TranslateSchedule( int scheduleType ) 
{
	return BaseClass::TranslateSchedule( scheduleType );
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::TranslateSchedule( int scheduleType ) 
{
	if ( m_pCurBehavior )
	{
		return m_pCurBehavior->BridgeTranslateSchedule( scheduleType );
	}
	return BaseClass::TranslateSchedule( scheduleType );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	if ( m_pCurBehavior )
		m_pCurBehavior->BridgePrescheduleThink();
}	

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::SelectSchedule( void )
{
	int result = 0;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeSelectSchedule( &result ) )
		return result;
	return BaseClass::SelectSchedule();
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	int result = 0;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeSelectFailSchedule( failedSchedule, failedTask, taskFailCode, &result ) )
		return result;
	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::StartTask( const Task_t *pTask )
{
	if ( m_pCurBehavior && m_pCurBehavior->BridgeStartTask( pTask ) )
		return;
	BaseClass::StartTask( pTask );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::RunTask( const Task_t *pTask )
{
	if ( m_pCurBehavior && m_pCurBehavior->BridgeRunTask( pTask ) )
		return;
	BaseClass::RunTask( pTask );
}

//-------------------------------------

template <class BASE_NPC>
inline CAI_Schedule *CAI_BehaviorHost<BASE_NPC>::GetSchedule(int localScheduleID)
{
	CAI_Schedule *pResult;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeGetSchedule( localScheduleID, &pResult ) )
		return pResult;
	return BaseClass::GetSchedule( localScheduleID );
}

//-------------------------------------

template <class BASE_NPC>
inline const char *CAI_BehaviorHost<BASE_NPC>::TaskName(int taskID)
{
	const char *pszResult = NULL;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeTaskName( taskID, &pszResult ) )
		return pszResult;
	return BaseClass::TaskName( taskID );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::BuildScheduleTestBits( void )
{
	if ( m_pCurBehavior )
		m_pCurBehavior->BridgeBuildScheduleTestBits(); 
	BaseClass::BuildScheduleTestBits();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnChangeHintGroup( string_t oldGroup, string_t newGroup )
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeOnChangeHintGroup( oldGroup, newGroup );
	}
	BaseClass::OnChangeHintGroup( oldGroup, newGroup );
}

//-------------------------------------

template <class BASE_NPC>
inline Activity CAI_BehaviorHost<BASE_NPC>::BackBridge_NPC_TranslateActivity( Activity activity )
{
	return BaseClass::NPC_TranslateActivity( activity );
}

//-------------------------------------

template <class BASE_NPC>
inline Activity CAI_BehaviorHost<BASE_NPC>::NPC_TranslateActivity( Activity activity )
{
	if ( m_pCurBehavior )
	{
		return m_pCurBehavior->BridgeNPC_TranslateActivity( activity );
	}
	return BaseClass::NPC_TranslateActivity( activity );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsCurTaskContinuousMove()
{
	bool result = false;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeIsCurTaskContinuousMove( &result ) )
		return result;
	return BaseClass::IsCurTaskContinuousMove();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::FValidateHintType( CAI_Hint *pHint )
{
	bool result = false;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeFValidateHintType( pHint, &result ) )
		return result;
	return BaseClass::FValidateHintType( pHint );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_IsValidEnemy(CBaseEntity *pEnemy)
{
	return BaseClass::IsValidEnemy( pEnemy );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_IsValidCover( const Vector &vLocation, CAI_Hint const *pHint )
{
	return BaseClass::IsValidCover( vLocation, pHint );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::BackBridge_IsValidShootPosition( const Vector &vLocation, CAI_Hint const *pHint )
{
	return BaseClass::IsValidShootPosition( vLocation, pHint );
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsValidEnemy( CBaseEntity *pEnemy )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeIsValidEnemy( pEnemy );
	
	return BaseClass::IsValidEnemy( pEnemy );
}
	
//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::ShouldAlwaysThink()
{
	bool result = false;
	if ( m_pCurBehavior && m_pCurBehavior->BridgeShouldAlwaysThink( &result ) )
		return result;
	return BaseClass::ShouldAlwaysThink();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeOnChangeActiveWeapon( pOldWeapon, pNewWeapon );
	}
	BaseClass::OnChangeActiveWeapon( pOldWeapon, pNewWeapon );
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnRestore()
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeOnRestore();
	}
	BaseClass::OnRestore();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsValidCover( const Vector &vLocation, CAI_Hint const *pHint )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeIsValidCover( vLocation, pHint );
	
	return BaseClass::IsValidCover( vLocation, pHint );
}
	
//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::IsValidShootPosition( const Vector &vLocation, CAI_Hint const *pHint )
{
	if ( m_pCurBehavior )
		return m_pCurBehavior->BridgeIsValidShootPosition( vLocation, pHint );
	
	return BaseClass::IsValidShootPosition( vLocation, pHint );
}
	
//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::Precache()
{
	BaseClass::Precache();
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgePrecache();
	}
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::NPCInit()
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeSpawn();
	}
	BaseClass::NPCInit();
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::UpdateOnRemove()
{
	for( int i = 0; i < m_Behaviors.Count(); i++ )
	{
		m_Behaviors[i]->BridgeUpdateOnRemove();
	}
	BaseClass::UpdateOnRemove();
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::OnBehaviorChangeStatus(  CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule )
{
	if ( pBehavior == GetRunningBehavior() && !pBehavior->CanSelectSchedule() && !fCanFinishSchedule )
	{
		DeferSchedulingToBehavior( NULL );
		return true;
	}
	return false;
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior )
{
}

//-------------------------------------

template <class BASE_NPC>
inline void CAI_BehaviorHost<BASE_NPC>::AddBehavior( CAI_BehaviorBase *pBehavior )
{
#ifdef DEBUG
	Assert( m_Behaviors.Find( pBehavior ) == m_Behaviors.InvalidIndex() );
	Assert( m_fDebugInCreateBehaviors );
	for ( int i = 0; i < m_Behaviors.Count(); i++)
	{
		Assert( typeid(*m_Behaviors[i]) != typeid(*pBehavior) );
	}
#endif
	m_Behaviors.AddToTail( pBehavior );
	pBehavior->SetOuter( this );
	pBehavior->SetBackBridge( this );
}

//-------------------------------------

template <class BASE_NPC>
inline CAI_BehaviorBase **CAI_BehaviorHost<BASE_NPC>::AccessBehaviors()
{
	if (m_Behaviors.Count())
		return m_Behaviors.Base();
	return NULL;
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::NumBehaviors()
{
	return m_Behaviors.Count();
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::Save( ISave &save )
{
	int result = BaseClass::Save( save );
	if ( result )
		CAI_BehaviorBase::SaveBehaviors( save, m_pCurBehavior, AccessBehaviors(), NumBehaviors() );
	return result;
}

//-------------------------------------

template <class BASE_NPC>
inline int CAI_BehaviorHost<BASE_NPC>::Restore( IRestore &restore )
{
	int result = BaseClass::Restore( restore );
	if ( result )
	{
		int iCurrent = CAI_BehaviorBase::RestoreBehaviors( restore, AccessBehaviors(), NumBehaviors() );
		if ( iCurrent != -1 )
			m_pCurBehavior = AccessBehaviors()[iCurrent];
		else
			m_pCurBehavior = NULL;
	}
	return result;
}

//-------------------------------------

template <class BASE_NPC>
inline bool CAI_BehaviorHost<BASE_NPC>::CreateComponents()
{
	if ( BaseClass::CreateComponents() )
	{
#ifdef DEBUG
		m_fDebugInCreateBehaviors = true;
#endif
		bool result = CreateBehaviors();
#ifdef DEBUG
		m_fDebugInCreateBehaviors = false;
#endif
		return result;
	}
	return false;
}


//-----------------------------------------------------------------------------

#endif // AI_BEHAVIOR_H
