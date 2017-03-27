//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef WEAPON_PHYSCANNON_H
#define WEAPON_PHYSCANNON_H
#ifdef _WIN32
#pragma once
#endif

#include "basehlcombatweapon.h"

class CBasePlayer;
class IPhysicsObject;
class CBaseEntity;
class CSoundPatch;
class CBeam;
class CSprite;

extern CBasePlayer *WasLaunchedByPlayer( IPhysicsObject *pPhysicsObject, float currentTime, float dt );
extern void PlayerPickupObject( CBasePlayer *pPlayer, CBaseEntity *pObject );

class CGrabController : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:
	CGrabController( void );
	~CGrabController( void );
	void AttachEntity( CBaseEntity *pEntity, IPhysicsObject *pPhys, const Vector &position, const QAngle &rotation );
	void DetachEntity( void );
	void OnRestore();
	void SetTargetPosition( const Vector &target, const QAngle &targetOrientation );
	float ComputeError();
	float GetLoadWeight( void ) const { return m_flLoadWeight; }

	CBaseEntity *GetAttached() { return (CBaseEntity *)m_attachedEntity; }

	IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );
	void SetMaxImpulse( const Vector &linear, const AngularImpulse &angular );
	
private:
	hlshadowcontrol_params_t	m_shadow;
	float			m_timeToArrive;
	float			m_errorTime;
	float			m_error;

	float			m_saveRotDamping;
	float			m_flLoadWeight;
	EHANDLE			m_attachedEntity;

	IPhysicsMotionController *m_controller;

	friend class CWeaponPhysCannon;
};

#define	NUM_BEAMS	4
#define	NUM_SPRITES	6
#define PHYSCANNON_BEAM_SPRITE "sprites/orangelight1.vmt"

class CWeaponPhysCannon : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponPhysCannon, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CWeaponPhysCannon( void );

	void	Drop( const Vector &vecVelocity );
	void	Precache();
	virtual void	OnRestore();
	void	PrimaryAttack();
	void	SecondaryAttack();
	void	WeaponIdle();
	void	ItemPreFrame();
	void	ItemPostFrame();
	void	ForceDrop( void );

	bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	bool	Deploy( void );

	bool	HasAnyAmmo( void ) { return true; }

protected:

	void	DoEffect( int effectType, Vector *pos = NULL );

	void	OpenElements( void );
	void	CloseElements( void );

	bool	CanPickupObject( CBaseEntity *pTarget );
	void	CheckForTarget( void );
	void	FindObject();
	void	AttachObject( CBaseEntity *pObject, const Vector &attachPosition );
	void	UpdateObject();
	void	DetachObject( bool playSound = true, bool wasLaunched = false );
	void	LaunchObject( const Vector &vecDir, float flForce );
	void	StartEffects( void );
	void	StopEffects( void );

	float			GetLoadPercentage();
	CSoundPatch		*GetMotorSound( void );

	void	DryFire( void );
	void	PrimaryFireEffect( void );

	bool	m_bOpen;
	bool	m_bActive;

	int		m_nChangeState;			//For delayed state change of elements
	float	m_flCheckSuppressTime;	//Amount of time to suppress the checking for targets

	float	m_flElementDebounce;
	float	m_flElementPosition;
	float	m_flElementDestination;

	float	m_flObjectRadius;	// Object's approximate size

	CBeam				*m_pBeams[NUM_BEAMS];
	CSprite				*m_pGlowSprites[NUM_SPRITES];
	CSprite				*m_pEndSprites[2];
	CSprite				*m_pCenterSprite;
	CSprite				*m_pBlastSprite;

	CSoundPatch			*m_sndMotor;		// Whirring sound for the gun
	
	CGrabController		m_grabController;
	QAngle				m_attachedAnglesPlayerSpace;
	Vector				m_attachedPositionObjectSpace;
};

#endif // WEAPON_PHYSCANNON_H
