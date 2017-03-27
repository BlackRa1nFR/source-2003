//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Defines the headcrab, a tiny, jumpy alien parasite.
//
// $NoKeywords: $
//=============================================================================

#ifndef NPC_HEADCRAB_H
#define NPC_HEADCRAB_H
#pragma once


#include "AI_BaseNPC.h"
#include "SoundEnt.h"

class CBaseHeadcrab : public CAI_BaseNPC
{
	DECLARE_CLASS( CBaseHeadcrab, CAI_BaseNPC );

public:
	void Spawn( void );
	void Precache( void );
	void RunTask( const Task_t *pTask );
	void StartTask( const Task_t *pTask );

	void OnChangeActivity( Activity NewActivity );

	virtual void JumpAttack( bool bRandomJump, const Vector &vecPos = vec3_origin, bool bThrown = false );

	void LeapTouch ( CBaseEntity *pOther );
	virtual void TouchDamage( CBaseEntity *pOther );
	virtual const Vector &WorldSpaceCenter( void ) const;
	bool	CorpseGib( const CTakeDamageInfo &info );
	void	Touch( CBaseEntity *pOther );
	Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true );
	
	float	MaxYawSpeed( void );
	void	GatherConditions( void );
	void	PrescheduleThink( void );
	Class_T Classify( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		RangeAttack1Conditions ( float flDot, float flDist );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	BuildScheduleTestBits( void );

	virtual void BiteSound( void ) = 0;
	virtual void AttackSound( void ) {};
	virtual void ImpactSound( void ) {};

	virtual int		SelectSchedule( void );
	virtual int		TranslateSchedule( int scheduleType );

	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	int				m_nGibCount;

	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

protected:
	bool	m_bHidden;
	float	m_flTimeDrown;
	float	m_flNextFlinchTime;
	Vector	m_vecJumpVel;
};


//=========================================================
//=========================================================
// The ever popular chubby classic headcrab
//=========================================================
//=========================================================
class CHeadcrab : public CBaseHeadcrab
{
	DECLARE_CLASS( CHeadcrab, CBaseHeadcrab );

public:
	void Precache( void );
	void Spawn( void );

	float	MaxYawSpeed( void );

	void	BiteSound( void );
	void	PainSound( void );
	void	DeathSound( void );
	void	IdleSound( void );
	void	AlertSound( void );
	void	AttackSound( void );
};

//=========================================================
//=========================================================
// The spindly, fast headcrab
//=========================================================
//=========================================================
class CFastHeadcrab : public CBaseHeadcrab
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CFastHeadcrab, CBaseHeadcrab );

	void Precache( void );
	void Spawn( void );
	bool QuerySeeEntity(CBaseEntity *pSightEnt);

	float	MaxYawSpeed( void );

	void PrescheduleThink( void );
	void RunTask( const Task_t *pTask );
	void StartTask( const Task_t *pTask );

	int TranslateSchedule( int scheduleType );

	int m_iRunMode;
	float m_flRealGroundSpeed;
	float m_flSlowRunTime;
	float m_flPauseTime;

	void	BiteSound( void );
	void	PainSound( void );
	void	DeathSound( void );
	void	IdleSound( void );
	void	AlertSound( void );
	void	AttackSound( void );
};


//=========================================================
//=========================================================
// Treacherous black headcrab
//=========================================================
//=========================================================
class CBlackHeadcrab : public CBaseHeadcrab
{
	DECLARE_CLASS( CBlackHeadcrab, CBaseHeadcrab );

public:

	void ThrowAt( const Vector &vecPos );
	void ThrowThink( void );

	void Eject( const QAngle &vecAngles, float flVelocityScale, CBaseEntity *pEnemy );
	void EjectTouch( CBaseEntity *pOther );

	//
	// CBaseHeadcrab implementation.
	//
	virtual void JumpAttack( bool bRandomJump, const Vector &vecPos = vec3_origin, bool bThrown = false );

	void TouchDamage( CBaseEntity *pOther );
	void BiteSound( void );
	void AttackSound( void );

	//
	// CAI_BaseNPC implementation.
	//
	virtual void PrescheduleThink( void );
	virtual void BuildScheduleTestBits( void );
	virtual int SelectSchedule( void );
	virtual int TranslateSchedule( int scheduleType );

	virtual Activity NPC_TranslateActivity( Activity eNewActivity );
	virtual void HandleAnimEvent( animevent_t *pEvent );
	virtual float MaxYawSpeed( void );

	virtual int	GetSoundInterests( void ) { return (BaseClass::GetSoundInterests() | SOUND_DANGER | SOUND_BULLET_IMPACT); }

	virtual void PainSound( void );
	virtual void DeathSound( void );
	virtual void IdleSound( void );
	virtual void AlertSound( void );
	virtual void ImpactSound( void );

	//
	// CBaseEntity implementation.
	//
	virtual void Precache( void );
	virtual void Spawn( void );

	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

private:

	void JumpFlinch( const Vector *pvecAwayFromPos );
	void Panic( float flDuration );

	bool m_bCommittedToJump;		// Whether we have 'locked in' to jump at our enemy.
	Vector m_vecCommittedJumpPos;	// The position of our enemy when we locked in our jump attack.

	bool m_bPanicState;
	float m_flPanicStopTime;

	float m_flNextNPCThink;
};

#endif // NPC_HEADCRAB_H
