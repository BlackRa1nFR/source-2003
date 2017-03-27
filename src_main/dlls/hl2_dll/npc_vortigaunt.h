//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: This is the base version of the vortigaunt
//
// $NoKeywords: $
//=============================================================================

#ifndef NPC_VORTIGAUNT_H
#define NPC_VORTIGAUNT_H
#ifdef _WIN32
#pragma once
#endif

#include "AI_BaseNPC.h"
#include "ai_behavior.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"

#define		VORTIGAUNT_MAX_BEAMS				8

class CBeam;
class CSprite;


//=========================================================
//	>> CNPC_Vortigaunt
//=========================================================
class CNPC_Vortigaunt : public CNPCSimpleTalker
{
	DECLARE_CLASS( CNPC_Vortigaunt, CNPCSimpleTalker );

public:
	void			Spawn( void );
	void			Precache( void );
	float			MaxYawSpeed( void );
	int				GetSoundInterests( void );

	int				RangeAttack1Conditions( float flDot, float flDist );
	int				MeleeAttack1Conditions( float flDot, float flDist );
	CBaseEntity*	Kick( void );
	void			Claw( int nHand );
	Vector			GetShootEnemyDir( const Vector &shootOrigin );

	void			AlertSound( void );
	Class_T			Classify ( void );
	void			HandleAnimEvent( animevent_t *pEvent );
	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
	Activity		NPC_TranslateActivity( Activity eNewActivity );

	void			Event_Killed( const CTakeDamageInfo &info );

	bool			ShouldGoToIdleState( void );
	void			RunTask( const Task_t *pTask );
	void			StartTask( const Task_t *pTask );
	int				ObjectCaps( void ) { return UsableNPCObjectCaps( BaseClass::ObjectCaps() ); }
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	
	void			DeclineFollowing( void );

	// Override these to set behavior
	virtual int		TranslateSchedule( int scheduleType );
	virtual int		SelectSchedule( void );
	bool			IsValidEnemy( CBaseEntity *pEnemy );

	void			DeathSound( void );
	void			PainSound( void );
	
	void			TalkInit( void );

	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void			SpeakSentence( int sentType );

	void			VortigauntThink(void);
	bool			CreateBehaviors();


private:
	float			m_flNextNPCThink;

	// ------------
	// Beams
	// ------------
	void			ClearBeams( );
	void			ArmBeam( int side , int beamType);
	void			WackBeam( int side, CBaseEntity *pEntity );
	void			ZapBeam( int side );
	void			BeamGlow( void );
	void			ClawBeam( CBaseEntity* pHurt, int nNoise, int nHand );
	void			DefendBeams( void );
	CBeam*			m_pBeam[VORTIGAUNT_MAX_BEAMS];
	int				m_iBeams;
	int				m_nLightningSprite;

	// ---------------
	//  Glow
	// ----------------
	void			ClearHandGlow( );
	float			m_fGlowAge;
	float			m_fGlowScale;
	float			m_fGlowChangeTime;
	bool			m_bGlowTurningOn;
	int				m_nCurGlowIndex;
	CSprite*		m_pLeftHandGlow;
	CSprite*		m_pRightHandGlow;
	void			StartHandGlow( int beamType );
	void			EndHandGlow( void );

	// ----------------
	//  Healing
	// ----------------
	float			m_flNextHealTime;		// Next time allowed to heal player
	float			m_flNextHealStepTime;	// Healing increment
	int				m_nCurrentHealAmt;		// How much healed this session
	int				m_nLastArmorAmt;		// Player armor at end of last heal
	int				m_iSuitSound;			// 0 = off, 1 = startup, 2 = going
	float			m_flSuitSoundTime;
	void			HealBeam( int side );
	bool			IsHealPositionValid(void);

	// -----------------
	//  Stomping
	// -----------------
	bool			IsStompable(CBaseEntity *pEntity);

	float			m_painTime;
	float			m_nextLineFireTime;
	bool			m_bInBarnacleMouth;

	EHANDLE			m_hVictim;		// Who I'm actively attacking (as opposed to enemy that I'm not necessarily attacking)

	CAI_StandoffBehavior m_StandoffBehavior;
	CAI_AssaultBehavior	 m_AssaultBehavior;

public:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};

#endif // NPC_VORTIGAUNT_H
