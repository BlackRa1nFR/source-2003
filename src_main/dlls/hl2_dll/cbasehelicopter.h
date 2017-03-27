//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Base class for helicopters & helicopter-type vehicles
//
// $NoKeywords: $
//=============================================================================

#include "AI_BaseNPC.h"
#include "ai_trackpather.h"

#pragma once
#ifndef CBASEHELICOPTER_H
#define CBASEHELICOPTER_H

//---------------------------------------------------------
//  Helicopter flags
//---------------------------------------------------------
enum HelicopterFlags_t
{
	BITS_HELICOPTER_GUN_ON			= 0x00000001,	// Gun is on and aiming
	BITS_HELICOPTER_MISSILE_ON		= 0x00000002,	// Missile turrets are on and aiming
};


//---------------------------------------------------------
//---------------------------------------------------------
#define SF_NOWRECKAGE		0x08
#define SF_NOROTORWASH		0x20
#define SF_AWAITINPUT		0x40

//---------------------------------------------------------
//---------------------------------------------------------
// Pathing data
#define BASECHOPPER_LEAD_DISTANCE			800.0f
#define	BASECHOPPER_MIN_CHASE_DIST_DIFF		128.0f	// Distance threshold used to determine when a target has moved enough to update our navigation to it
#define BASECHOPPER_AVOID_DIST				256.0f

#define BASECHOPPER_MAX_SPEED				400.0f
#define BASECHOPPER_MAX_FIRING_SPEED		250.0f
#define BASECHOPPER_MIN_ROCKET_DIST			1000.0f
#define BASECHOPPER_MAX_GUN_DIST			2000.0f

//---------------------------------------------------------
// Physics rotor pushing
#define BASECHOPPER_WASH_RADIUS			256
#define BASECHOPPER_WASH_PUSH_MIN		30		// Initial force * their mass applied to objects in the wash
#define BASECHOPPER_WASH_PUSH_MAX		40		// Maximum force * their mass applied to objects in the wash
#define BASECHOPPER_WASH_RAMP_TIME		1.0		// Time it takes to ramp from the initial to the max force on an object in the wash (at the center of the wash)
#define BASECHOPPER_WASH_MAX_MASS		300		// Don't attempt to push anything over this mass
#define BASECHOPPER_WASH_MAX_OBJECTS	4		// Maximum number of objects the wash will push at once

// Wash physics pushing
struct washentity_t
{
	DECLARE_DATADESC();

	EHANDLE		hEntity;
	float		flWashStartTime;
};

#define BASECHOPPER_WASH_ALTITUDE			1024.0f

//=========================================================
//=========================================================

class CBaseHelicopter : public CAI_TrackPather
{
public:
	DECLARE_CLASS( CBaseHelicopter, CAI_TrackPather );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CBaseHelicopter( void );

	void Spawn( void );
	void Precache( void );
	
	virtual void PrescheduleThink( void );

	void Event_Killed( const CTakeDamageInfo &info );
	void StopLoopingSounds();

	int  BloodColor( void ) { return DONT_BLEED; }
	void GibMonster( void );

	virtual void SetObjectCollisionBox( void );
	virtual bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );

	Class_T Classify ( void ) { return CLASS_COMBINE; }

	void CallDyingThink( void ) { DyingThink(); }

	bool HasEnemy( void ) { return GetEnemy() != NULL; }
	void GatherEnemyConditions( CBaseEntity *pEnemy );
	virtual bool ChooseEnemy( void );
	virtual void HelicopterThink( void );
	virtual void HelicopterPostThink( void ) { };
	virtual void FlyTouch( CBaseEntity *pOther );
	virtual void CrashTouch( CBaseEntity *pOther );
	virtual void DyingThink( void );
	virtual void Startup( void );
	virtual void NullThink( void );

	virtual void Flight( void );

	virtual void ShowDamage( void ) {};

	void UpdatePlayerDopplerShift( void );

	virtual void Hunt( void );

	virtual bool IsCrashing( void ) { return m_lifeState != LIFE_ALIVE; }
	virtual float GetAcceleration( void ) { return 5; }

	virtual void ApplySidewaysDrag( const Vector &vecRight );
	virtual void ApplyGeneralDrag( void );


	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	virtual bool FireGun( void );

	virtual float GetRotorVolume( void ) { return 1.0; }
	virtual void InitializeRotorSound( void );
	virtual void UpdateRotorSoundPitch( int iPitch );

	virtual void AimRocketGun(void) {};
	virtual void FireRocket(  Vector vLaunchPos, Vector vLaunchDir  ) {};

	virtual bool	GetTrackPatherTarget( Vector *pPos ) { if ( GetEnemy() ) { *pPos = GetEnemy()->WorldSpaceCenter(); return true; } return false; }
	virtual CBaseEntity *GetTrackPatherTargetEnt()	{ return GetEnemy(); }

	void	DrawDebugGeometryOverlays(void);

	// Rotor washes
	virtual void	DoRotorWash( void );
	void			DrawRotorWash( float flAltitude, Vector vecRotorOrigin );
	void			DoRotorPhysicsPush( Vector &vecRotorOrigin, float flAltitude );
	bool			DoWashPush( washentity_t *pWash, Vector vecWashOrigin );

	CSoundPatch		*m_pRotorSound;				// Rotor loop played when the player can see the helicopter
	CSoundPatch		*m_pRotorBlast;				// Sound played when the helicopter's pushing around physics objects

	float			m_flForce;
	int				m_fHelicopterFlags;

	Vector			m_vecDesiredFaceDir;

	float			m_flLastSeen;
	float			m_flPrevSeen;

	int				m_iSoundState;		// don't save this

	Vector			m_vecTarget;
	Vector			m_vecTargetPosition;

	float			m_flMaxSpeed;		// Maximum speed of the helicopter.
	float			m_flMaxSpeedFiring;	// Maximum speed of the helicopter whilst firing guns.

	float			m_flGoalSpeed;		// Goal speed
	float			m_flInitialSpeed;

	// Inputs
	void InputActivate( inputdata_t &inputdata );

	// Outputs
	COutputEvent	m_AtTarget;			// Fired when pathcorner has been reached
	COutputEvent	m_LeaveTarget;		// Fired when pathcorner is left

	// Inputs
	void			InputGunOn( inputdata_t &inputdata );
	void			InputGunOff( inputdata_t &inputdata );
	void			InputMissileOn( inputdata_t &inputdata );
	void			InputMissileOff( inputdata_t &inputdata );

protected:	
	// Custom conservative collision volumes
	Vector			m_cullBoxMins;
	Vector			m_cullBoxMaxs;

	// Wash physics pushing
	CUtlVector< washentity_t >	m_hEntitiesPushedByWash;
};


#endif // CBASEHELICOPTER_H
