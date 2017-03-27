//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: The downtrodden citizens of City 17. Timid when unarmed, they will
//			rise up against their Combine oppressors when given a weapon.
//
//=============================================================================

#ifndef	NPC_CITIZEN_H
#define	NPC_CITIZEN_H

#include "ai_behavior.h"
#include "ai_baseactor.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_talker.h"

//=========================================================
//=========================================================

// Spawnflags.
#define	SF_CITIZEN_MEDIC			1024	//1024
#define SF_CITIZEN_RANDOM_HEAD		4096
#define SF_CITIZEN_FOLLOW			65536	//follow the player as soon as I spawn.

#define CITIZEN_AE_HEAL 1

class CNPC_Citizen : public CAI_PlayerAlly
{
	DECLARE_CLASS( CNPC_Citizen, CAI_PlayerAlly );

public:
	bool	CreateBehaviors();
	void	Precache( void );
	void	Spawn( void );
	void	PostNPCInit( void );
	Class_T Classify( void );
	int		GetSoundInterests ( void );

	void SelectCitizenModel();

	string_t GetModelName( void ) const;

	void GatherConditions( void );
	void PrescheduleThink( void );

	void HandleAnimEvent( animevent_t *pEvent );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	int OnTakeDamage_Alive( const CTakeDamageInfo &info );

	virtual int	SelectSchedule( void );
	int SelectHealSchedule();
	virtual int TranslateSchedule( int scheduleType );

	bool NearCommandGoal();
	bool VeryFarFromCommandGoal();

	Activity	NPC_TranslateActivity( Activity eNewActivity );
	Activity	GetHintActivity( short sHintType );

	bool		FValidateHintType ( CAI_Hint *pHint );

	void			BuildScheduleTestBits( void );
	int				DrawDebugTextOverlays(void); 

	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	void Touch( CBaseEntity *pOther );

	bool FInAimCone( const Vector &vecSpot );

	void LocateEnemySound();

	void SetLookTarget(CBaseEntity *pLookTarget);
	virtual CBaseEntity *EyeLookTarget();

	virtual bool FCanCheckAttacks( void );

	bool ShouldStandoff( void );
	bool ShouldHealTarget( CBaseEntity *pTarget, bool bActiveUse = false );
	int ObjectCaps( void ) { return UsableNPCObjectCaps( BaseClass::ObjectCaps() ); }

	virtual bool CanBeSelected() { return true; }
	virtual bool TargetOrder( CBaseEntity *pTarget );
	virtual void MoveOrder( const Vector &vecDest );

	void	Heal( void );
	bool	CanHeal( void );
	bool	PassesDamageFilter( CBaseEntity *pAttacker );

	bool FindCoverPos( CBaseEntity *pEntity, Vector *pResult);
	bool IsCoverPosition( const Vector &vecThreat, const Vector &vecPosition );
	bool ValidateNavGoal();

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// -------------------
	//	Sounds
	// -------------------
	void				FearSound(void);
	void				PainSound(void);
	void				DeathSound(void);

	int		m_nInspectActivity;
	float	m_flNextFearSoundTime;
	float	m_flStopManhackFlinch;
	float	m_fNextInspectTime;		// Next time I'm allowed to get inspected by a scanner
	float m_flHealTime;
	float m_flPlayerHealTime;
	float m_flAllyHealTime;

	static int m_nextHead;

	static bool gm_bFindingTurretCover;

	EHANDLE m_hCurLookTarget;		// The entity we have decided we should look at right now.

	//=========================================================
	// Private conditions
	//=========================================================
	enum Citizen_Conds
	{
		COND_CIT_HURTBYMANHACK = BaseClass::NEXT_CONDITION,
		COND_CIT_PLAYERHEALREQUEST
	};

	//=========================================================
	// Citizen schedules
	//=========================================================
	enum
	{
		SCHED_CITIZEN_COWER = BaseClass::NEXT_SCHEDULE,
		SCHED_CITIZEN_PLAY_INSPECT_ACTIVITY,
		SCHED_CITIZEN_FLIP_OFF,
		SCHED_CITIZEN_NAP,
		SCHED_CITIZEN_STAND_LOOK,
		SCHED_CITIZEN_MANHACKFLINCH,
		SCHED_CITIZEN_TAKE_COVER_FROM_BEST_SOUND,
		SCHED_CITIZEN_FLEE_FROM_BEST_SOUND,
		SCHED_CITIZEN_HEAL,
		SCHED_CITIZEN_FAIL_TAKE_COVER_TURRET,

	};

	//=========================================================
	// Citizen tasks
	//=========================================================
	enum 
	{
		TASK_CIT_TARGET_POLICE = BaseClass::NEXT_TASK,
		TASK_CIT_GET_LOOK_TARGET,
		TASK_CIT_FACE_GROUP,
		TASK_CIT_PLAY_INSPECT_ACT,
		TASK_CIT_DEFER_MANHACKFLINCH,
		TASK_CIT_HEAL,

	};

	
	CAI_AssaultBehavior		m_AssaultBehavior;
	CAI_FollowBehavior		m_FollowBehavior;
	CAI_StandoffBehavior	m_StandoffBehavior;

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};

//---------------------------------------------------------
//---------------------------------------------------------
inline bool CNPC_Citizen::NearCommandGoal()
{
	const float flDistSqr = (12*5) * (12*5);
	return ( ( GetAbsOrigin() - GetCommandGoal() ).LengthSqr() <= flDistSqr );
}

//---------------------------------------------------------
//---------------------------------------------------------
inline bool CNPC_Citizen::VeryFarFromCommandGoal()
{
	const float flDistSqr = (12*50) * (12*50);
	return ( ( GetAbsOrigin() - GetCommandGoal() ).LengthSqr() > flDistSqr );
}

#endif	//NPC_CITIZEN_H
