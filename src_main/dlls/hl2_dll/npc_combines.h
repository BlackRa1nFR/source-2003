//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef NPC_COMBINES_H
#define NPC_COMBINES_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_combine.h"

//=========================================================
//	>> CNPC_CombineS
//=========================================================
class CNPC_CombineS : public CNPC_Combine
{
	DECLARE_CLASS( CNPC_CombineS, CNPC_Combine );

public:
	void		Spawn( void );
	void		Precache( void );
	void		DeathSound( void );
	void		MaintainLookTargets( float flInterval );
	void		PrescheduleThink( void );
	void		BuildScheduleTestBits( void );
	int			SelectSchedule ( void );
	int			TakeDamage( const CTakeDamageInfo &info );
	int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	bool		FInAimCone( const Vector &vecSpot );
	void		HandleAnimEvent( animevent_t *pEvent );
	void		OnChangeActivity( Activity eNewActivity );
	void		Event_Killed( const CTakeDamageInfo &info );
	void		OnListened();

	void		ClearAttackConditions( void );

	int			GetWeaponProficiency( CBaseCombatWeapon *pWeapon ) { return WEAPON_PROFICIENCY_HIGH; }

	bool		m_fIsBlocking;

private:
	bool		ShouldHitPlayer( const Vector &targetDir, float targetDist );
};

#endif // NPC_COMBINES_H
