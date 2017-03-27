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

#ifndef	NPC_WSCANNER_H
#define	NPC_WSCANNER_H

#include "AI_BaseNPC_Flyer.h"

//-----------------------------------------------------------------------------
// Attachment points.
//-----------------------------------------------------------------------------
#define	WSCANNER_NUM_BEAMS				4
#define	WSCANNER_NUM_GLOWS				3

enum WScannerState_t
{	
	WSCANNER_STATE_ATTACHED,
	WSCANNER_STATE_SINGLE,
	WSCANNER_STATE_GETTING_HELP,
};

class CSprite;
class CBeam;

//-----------------------------------------------------------------------------
// WScanner 
//-----------------------------------------------------------------------------
class CNPC_WScanner : public CAI_BaseFlyingBot
{
	DECLARE_CLASS( CNPC_WScanner, CAI_BaseFlyingBot );
	static int		m_nNumAttackers;		// Total number of attacking wasteland scanners

public:
	CNPC_WScanner();

	Class_T			Classify(void);

	void			Event_Killed( const CTakeDamageInfo &info );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	void			HandleAnimEvent( animevent_t *pEvent );

 	virtual int		SelectSchedule(void);
	virtual int		TranslateSchedule( int scheduleType );

	bool			FValidateHintType(CAI_Hint *pHint);

	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true ) { return WorldSpaceCenter(); }

	void			SetWScannerState(WScannerState_t pNewState);
	Vector			SplitPos(int nPos);
	void			Split(void);
	void			Join(void);
	void			StartJoinBeams(void);
	void			UpdateSquadHelper(CBaseEntity* pHelper);

	bool			OverrideMove(float flInterval);
	bool 			OverridePathMove(float flInterval);
	
	void			MoveToTarget(float flInterval, const Vector &MoveTarget);
	void			MoveToAttack(float flInterval);
	Vector			MoveToEvade(CBaseCombatCharacter *pEnemy);
	void			MoveExecute_Alive(float flInterval);
	float			MinGroundDist(void);
	void			SetBanking(float flInterval);

	void			PlayAlarmSound(void);
	void			PlayFlySound(void);
	void			AdjustAntenna(float flInterval);
	virtual void	StopLoopingSounds(void);

	void			Precache(void);
	void			RunTask( const Task_t *pTask );
	void			Spawn(void);
	void			StartTask( const Task_t *pTask );

	int				DrawDebugTextOverlays(void);
	void			OnScheduleChange();

	DEFINE_CUSTOM_AI;

	DECLARE_DATADESC();

	~CNPC_WScanner(void);

protected:
	int				m_nPodSize;
	EHANDLE			m_hHelper;
	WScannerState_t	m_nWScannerState;

	// ==================
	// Attack 
	// ==================
	CBeam*			m_pBeam[WSCANNER_NUM_BEAMS];
	CSprite*		m_pLaserGlow[WSCANNER_NUM_GLOWS];
	bool			m_bBankFace;
	float			m_fNextGrenadeTime;
	float			m_fNextLaserTime;
	void			LaserWarmup(void);
	void			LaserShoot(void);
	void			LaserKill(void);
	void			TossGrenade(void);
	float			m_fSwitchToHelpTime;

	// ==================
	// Movement variables.
	// ==================
	float			m_fNextMoveEvadeTime;
	float			m_fNextAlarmSoundTime;
	float			m_fNextFlySoundTime;
	float			m_fStartJoinTime;
	
	float			m_fAntennaPos;
	float			m_fAntennaTarget;

	CSprite*		m_pLightGlow;
};

#endif	//NPC_WSCANNER_H
