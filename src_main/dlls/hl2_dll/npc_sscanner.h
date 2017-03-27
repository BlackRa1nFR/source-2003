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

#ifndef	NPC_SSCANNER_H
#define	NPC_SSCANNER_H

#include "AI_BaseNPC_Flyer.h"

//-----------------------------------------------------------------------------
// Attachment points.
//-----------------------------------------------------------------------------

class CSprite;
class CBeam;
class CShieldBeam;
class CScannerShield;

enum SScannerState_t
{
	SSCANNER_OPEN,	
	SSCANNER_CLOSED,
};

//-----------------------------------------------------------------------------
// Sheild Scanner 
//-----------------------------------------------------------------------------
class CNPC_SScanner : public CAI_BaseFlyingBot
{
	DECLARE_CLASS( CNPC_SScanner, CAI_BaseFlyingBot );

public:
	CNPC_SScanner();
	Class_T			Classify(void);

	void			Event_Killed( const CTakeDamageInfo &info );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void			Gib(void);
	void			NPCThink( void );

 	virtual int		SelectSchedule(void);
	virtual int		TranslateSchedule( int scheduleType );
	Activity		NPC_TranslateActivity( Activity eNewActivity );
	void			OnScheduleChange();

	bool			OverrideMove(float flInterval);
	bool			OverridePathMove( float flInterval );
	void			MoveToTarget(float flInterval, const Vector &MoveTarget);
	void			MoveToEntity(float flInterval, CBaseEntity* pMoveTarget, float fMinRange, float fMaxRange);
	void			MoveExecute_Alive(float flInterval);
	float			MinGroundDist(void);

	void			PlayFlySound(void);
	virtual void	StopLoopingSounds(void);

	void			Precache(void);
	void			RunTask( const Task_t *pTask );
	void			Spawn(void);
	void			StartTask( const Task_t *pTask );

	DEFINE_CUSTOM_AI;

	DECLARE_DATADESC();

protected:
	float			m_lastHurtTime;

	// ==================
	// Shield
	// ==================
	SScannerState_t m_nState;
	bool			m_bShieldsDisabled;
	CScannerShield*	m_pShield;
	CShieldBeam*	m_pShieldBeamL;		
	CShieldBeam*	m_pShieldBeamR;		
	float			m_fNextShieldCheckTime;
	CBaseEntity*	PickShieldEntity(void);
	void			PowerShield(void);
	void			StopShield(void);
	void			UpdateShields(void);
	bool			IsValidShieldTarget(CBaseEntity *pEntity);
	bool			IsValidShieldCover( Vector &vCoverPos );
	bool			SetShieldCoverPosition( void );

	// ==================
	// Movement variables.
	// ==================
	float			m_fNextFlySoundTime;
	Vector			m_vCoverPosition;		// Position to go for cover
};

#endif	//NPC_SSCANNER_H
