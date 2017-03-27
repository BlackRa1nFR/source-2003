//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: RPG
//
//=============================================================================


#ifndef WEAPON_RPG_H
#define WEAPON_RPG_H
#ifdef _WIN32
#pragma once
#endif


#include "hl1_basecombatweapon_shared.h"
#include "sprite.h"
#include "NPCEvent.h"
#include "smoke_trail.h"
#include "hl1_basegrenade.h"


class CWeaponRPG;

//###########################################################################
//	CRpgRocket
//###########################################################################
class CRpgRocket : public CHL1BaseGrenade
{
	DECLARE_CLASS( CRpgRocket, CHL1BaseGrenade );
	DECLARE_SERVERCLASS();

public:
	CRpgRocket();

	Class_T Classify( void ) { return CLASS_MISSILE; }
	
	void Spawn( void );
	void Precache( void );
	void RocketTouch( CBaseEntity *pOther );
	void IgniteThink( void );
	void SeekThink( void );

	static CRpgRocket *Create( const Vector &vecOrigin, const QAngle &angAngles, CBasePlayer *pentOwner = NULL );

	CHandle<CWeaponRPG>		m_hOwner;
	float					m_flIgniteTime;
	int						m_iTrail;
	
	DECLARE_DATADESC();
};


//-----------------------------------------------------------------------------
// Laser Dot
//-----------------------------------------------------------------------------

class CLaserDot : public CBaseEntity 
{
	DECLARE_CLASS( CLaserDot, CBaseEntity );
public:

	CLaserDot( void );

	static CLaserDot *Create( const Vector &origin, CBaseEntity *pOwner = NULL );

	void	Precache( void );
	void	ItemPostFrame( void );
	void	SetLaserPosition( const Vector &origin, const Vector &normal );
	Vector	GetChasePosition();
	void	TurnOn( void );
	void	TurnOff( void );
	void	Toggle( void );

	bool	IsActive( void );

	int		ObjectCaps() { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }

private:

	CHandle<CSprite>	m_hLaserSprite;
	Vector				m_vecSurfaceNormal;
};


//-----------------------------------------------------------------------------
// CWeaponRPG
//-----------------------------------------------------------------------------
class CWeaponRPG : public CBaseHL1CombatWeapon
{
	DECLARE_CLASS( CWeaponRPG, CBaseHL1CombatWeapon );
public:

	CWeaponRPG( void );

	void	ItemPostFrame( void );
	void	Precache( void );
	bool	Deploy( void );
	void	PrimaryAttack( void );
	void	WeaponIdle( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	void	NotifyRocketDied( void );
	bool	Reload( void );

	virtual int	GetDefaultClip1( void ) const;

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	void	CreateLaserPointer( void );
	void	UpdateSpot( void );
	bool	IsGuiding( void );
	void	StartGuiding( void );
	void	StopGuiding( void );

private:
	bool				m_bGuiding;
	CHandle<CLaserDot>	m_hLaserDot;
	CHandle<CRpgRocket>	m_hMissile;
	bool				m_bIntialStateUpdate;
	bool				m_bLaserDotSuspended;
	float				m_flLaserDotReviveTime;
};


#endif	// WEAPON_RPG_H
