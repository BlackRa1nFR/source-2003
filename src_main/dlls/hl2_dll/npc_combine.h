//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef NPC_COMBINE_H
#define NPC_COMBINE_H
#ifdef _WIN32
#pragma once
#endif

#include "AI_BaseNPC.h"
#include "AI_BaseHumanoid.h"
#include "AI_Behavior.h"
#include "ai_behavior_assault.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_follow.h"

//=========================================================
//	>> CNPC_Combine
//=========================================================
class CNPC_Combine : public CAI_BaseHumanoid
{
	DECLARE_CLASS( CNPC_Combine, CAI_BaseHumanoid );

public:
	static const char *pCombineSentences[];

	int				m_nKickDamage;
	Vector			m_vecTossVelocity;
	bool			m_bStanding;
	bool			m_bCanCrouch;
	bool			m_bFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int				m_iSentence;
	int				m_voicePitch;
	int				m_lastGrenadeCondition;

	void SetVoicePitch( int voicePitch )	{ m_voicePitch = voicePitch; }
	int GetVoicePitch() const				{ return m_voicePitch; }

	// ---------------
	// Time Variables
	// ---------------
	float			m_flNextPainSoundTime;
	float			m_flNextAlertSoundTime;
	float			m_flNextGrenadeCheck;	
	float			m_flNextLostSoundTime;
	float			m_flAlertPatrolTime;		// When to stop doing alert patrol

	CNPC_Combine();

	int				GetGrenadeConditions( float flDot, float flDist );
	int				RangeAttack2Conditions( float flDot, float flDist ); // For innate grenade attack
	int				MeleeAttack1Conditions( float flDot, float flDist ); // For kick/punch
	void			CheckAmmo( void );
	bool			FVisible( CBaseEntity *pEntity, int traceMask = MASK_OPAQUE, CBaseEntity **ppBlocker = NULL );

	NPC_STATE		SelectIdealState ( void );

	int				m_nShots;
	float			m_flShotDelay;

	// Input handlers.
	void InputLookOn( inputdata_t &inputdata );
	void InputLookOff( inputdata_t &inputdata );

	bool			UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, bool firstHand = true );

	void			Spawn( void );
	void			Precache( void );

	Class_T			Classify( void );
	float			MaxYawSpeed( void );
  	bool			OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );;
	void			HandleAnimEvent( animevent_t *pEvent );
	Vector			Weapon_ShootPosition( );
	CBaseEntity		*Kick( void );

	Vector			EyeOffset( Activity nActivity );
	Vector			EyePosition( void );
	Vector			BodyTarget( const Vector &posSrc, bool bNoisy = true );

	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );

	Activity		NPC_TranslateActivity( Activity eNewActivity );
	void			BuildScheduleTestBits( void );
	virtual int		SelectSchedule( void );
	int				SelectScheduleAttack();

	bool			CreateBehaviors();

	bool			OnBeginMoveAndShoot();
	void			OnEndMoveAndShoot();

	// Combat
	int				CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );


	// -------------
	// Sounds
	// -------------
	void			DeathSound( void );
	void			PainSound( void );
	void			IdleSound( void );
	void			AlertSound( void );
	void			LostEnemySound( void );
	void			FoundEnemySound( void );
	void			AnnounceAssault( void );

	void			NotifyDeadFriend( CBaseEntity* pFriend );

	virtual float	HearingSensitivity( void ) { return 1.0; };
	int				GetSoundInterests( void );

	// Speaking
	void			SpeakSentence( int sentType );

	virtual int		TranslateSchedule( int scheduleType );

	//=========================================================
	// Combine S schedules
	//=========================================================
	enum
	{
		SCHED_COMBINE_SUPPRESS = BaseClass::NEXT_SCHEDULE,
		SCHED_COMBINE_COMBAT_FAIL,
		SCHED_COMBINE_VICTORY_DANCE,
		SCHED_COMBINE_COMBAT_FACE,
		SCHED_COMBINE_HIDE_AND_RELOAD,
		SCHED_COMBINE_SIGNAL_SUPPRESS,
		SCHED_COMBINE_ASSAULT,
		SCHED_COMBINE_ESTABLISH_LINE_OF_FIRE,
		SCHED_COMBINE_PRESS_ATTACK,
		SCHED_COMBINE_REPEL,
		SCHED_COMBINE_REPEL_ATTACK,
		SCHED_COMBINE_REPEL_LAND,
		SCHED_COMBINE_WAIT_IN_COVER,
		SCHED_COMBINE_RANGE_ATTACK1,
		SCHED_COMBINE_RANGE_ATTACK2,
		SCHED_COMBINE_TAKE_COVER1,
		SCHED_COMBINE_TAKE_COVER_FROM_BEST_SOUND,
		SCHED_COMBINE_RUN_AWAY_FROM_BEST_SOUND,
		SCHED_COMBINE_GRENADE_COVER1,
		SCHED_COMBINE_TOSS_GRENADE_COVER1,
		SCHED_COMBINE_TAKECOVER_FAILED,
		SCHED_COMBINE_SHOOT_ENEMY_COVER,
		SCHED_COMBINE_GRENADE_AND_RELOAD,
		SCHED_COMBINE_PATROL,
		NEXT_SCHEDULE,
	};

	//=========================================================
	// Combine Tasks
	//=========================================================
	enum 
	{
		TASK_COMBINE_FACE_TOSS_DIR = BaseClass::NEXT_TASK,
		TASK_COMBINE_IGNORE_ATTACKS,
		//TASK_COMBINE_MOVE_AND_SHOOT,
		//TASK_COMBINE_MOVE_AND_AIM,
		NEXT_TASK
	};

	//=========================================================
	// Combine Conditions
	//=========================================================
	enum Combine_Conds
	{
		COND_COMBINE_NO_FIRE = BaseClass::NEXT_CONDITION,
		COND_COMBINE_DEAD_FRIEND,
		COND_COMBINE_SHOULD_PATROL,
		NEXT_CONDITION
	};

public:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

private:

	class CCombineStandoffBehavior : public CAI_ComponentWithOuter<CNPC_Combine, CAI_StandoffBehavior>
	{
		typedef CAI_ComponentWithOuter<CNPC_Combine, CAI_StandoffBehavior> BaseClass;

		virtual int SelectScheduleAttack()
		{
			int result = GetOuter()->SelectScheduleAttack();
			if ( result == SCHED_NONE )
				result = BaseClass::SelectScheduleAttack();
			return result;
		}
	};

	int			m_iNumGrenades;
	CAI_AssaultBehavior			m_AssaultBehavior;
	CCombineStandoffBehavior	m_StandoffBehavior;
	CAI_FollowBehavior			m_FollowBehavior;
};


#endif // NPC_COMBINE_H
