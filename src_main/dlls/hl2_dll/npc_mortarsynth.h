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

#ifndef	NPC_MSYNTH_H
#define	NPC_MSYNTH_H

#include "AI_BaseNPC_Flyer.h"

//-----------------------------------------------------------------------------
// Attachment points.
//-----------------------------------------------------------------------------
#define	MSYNTH_MAX_SPEED				400
#define	MSYNTH_NUM_GLOWS				2

class CSprite;
class CBeam;
class CBeamChaser;

//-----------------------------------------------------------------------------
// MSynth 
//-----------------------------------------------------------------------------
class CNPC_MSynth : public CAI_BaseFlyingBot
{
	DECLARE_CLASS( CNPC_MSynth, CAI_BaseFlyingBot );

	public:
		CNPC_MSynth();

		Class_T			Classify(void);

		void			Event_Killed( const CTakeDamageInfo &info );
		int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
		void			Gib(void);
		void			NPCThink( void );

 		virtual int		SelectSchedule(void);
		virtual int		TranslateSchedule( int scheduleType );

		bool			OverrideMove(float flInterval);
		void			MoveToTarget(float flInterval, const Vector &MoveTarget);
		void			MoveToEntity(float flInterval, CBaseEntity* pMoveTarget, float fMinRange, float fMaxRange);
		void			MoveExecute_Alive(float flInterval);
		float			MinGroundDist(void);
		void			SetBanking(float flInterval);

		bool			IsHeadFacing( const Vector &vFaceTarget );

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
		// Attack 
		// ==================
		CBeam*			m_pEnergyBeam;
		CSprite*		m_pEnergyGlow[MSYNTH_NUM_GLOWS];
		float			m_fNextEnergyAttackTime;
		void			EnergyWarmup(void);
		void			EnergyShoot(void);
		void			EnergyKill(void);

		// ==================
		// Movement variables.
		// ==================
		float			m_fNextFlySoundTime;
		Vector			m_vCoverPosition;		// Position to go for cover
};

#endif	//NPC_MSYNTH_H
