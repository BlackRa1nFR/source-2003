//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef NPC_METROPOLICE_H
#define NPC_METROPOLICE_H
#ifdef _WIN32
#pragma once
#endif

#include "AI_BaseNPC.h"
#include "AI_BaseHumanoid.h"
#include "AI_BaseActor.h"
#include "ai_behavior.h"
#include "ai_behavior_standoff.h"

#define METROPOLICE_SF_RAPPEL	0x00000040
#define METROPOLICE_SF_NOCHATTER ( 1 << 15 )

//=========================================================
//	>> CNPC_MetroPolice
//=========================================================
class CNPC_MetroPolice : public CAI_BaseActor
{
	DECLARE_CLASS( CNPC_MetroPolice, CAI_BaseHumanoid );

public:
	CNPC_MetroPolice();
	bool CreateBehaviors();
	void Spawn( void );
	void Precache( void );

	Class_T		Classify( void );
	Disposition_t IRelationType(CBaseEntity *pTarget); // Why do people insist on trying to columnize class member names?
	float		MaxYawSpeed( void );
	void		HandleAnimEvent( animevent_t *pEvent );
	int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void		AlertSound( void );
	void		PainSound( void );
	void		DeathSound( void );
	void		SurpriseSound( void );
	int			GetSoundInterests( void );

	void		BuildScheduleTestBits( void );

	bool		CanDeployManhack( void );

	bool		ShouldHitPlayer( const Vector &targetDir, float targetDist );

	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );

	void		PrescheduleThink( void );

	void		CreateDropLine( void );

	bool		IsUnreachable( CBaseEntity* pEntity );

	bool		FInAimCone( const Vector &vecSpot );

	void 		OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	int			CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );

	// Inputs
	void		InputBeginRappel( inputdata_t &inputdata );

	void NotifyDeadFriend ( CBaseEntity* pFriend );

	Activity NPC_TranslateActivity( Activity newActivity );
	virtual int	SelectSchedule( void );
	virtual int TranslateSchedule( int scheduleType );

	DECLARE_DATADESC();

private:
	int				m_iPistolClips;		// How many clips the cop has in reserve
	int				m_iManhacks;		// How many manhacks the cop has
	bool			m_fWeaponDrawn;		// Is my weapon drawn? (ready to use)
	bool			m_fCanPoint;		// Can only point once per time spent in combat state.
	int				m_LastShootSlot;
	CRandSimTimer	m_TimeYieldShootSlot;

	CHandle<CRopeKeyframe>	m_hRappelLine;

	// Outputs
	COutputEvent	m_OnRappelTouchdown;

	AIHANDLE		m_hManhack;

	CAI_StandoffBehavior m_StandoffBehavior;
public:
	DEFINE_CUSTOM_AI;
};

#endif // NPC_METROPOLICE_H
