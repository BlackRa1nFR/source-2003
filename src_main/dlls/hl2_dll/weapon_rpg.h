#include "basehlcombatweapon.h"
#include "sprite.h"
#include "NPCEvent.h"
#include "smoke_trail.h"

#pragma once

#ifndef WEAPON_RPG_H
#define WEAPON_RPG_H

class CWeaponRPG;

//###########################################################################
//	>> CMissile		(missile launcher class is below this one!)
//###########################################################################
class CMissile : public CBaseCombatCharacter
{
	DECLARE_CLASS( CMissile, CBaseCombatCharacter );

public:
	CMissile();

	Class_T Classify( void ) { return CLASS_MISSILE; }
	
	void Spawn( void );
	void Precache( void );
	void StingerTouch( CBaseEntity *pOther );
	void StingerExplode( void );
	void ShotDown( void );
	void AccelerateThink( void );
	void AugerThink( void );
	void IgniteThink( void );
	void SeekThink( void );

	int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	static CMissile *CMissile::Create( const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner );

	CHandle<CWeaponRPG>		m_hOwner;
	RocketTrail				*m_pRocketTrail;
	float					m_flAugerTime;		// Amount of time to auger before blowing up anyway
	
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

	void	SetTargetEntity( CBaseEntity *pTarget ) { m_hTargetEnt = pTarget; }
	CBaseEntity *GetTargetEntity( void ) { return m_hTargetEnt; }

	void	SetLaserPosition( const Vector &origin, const Vector &normal );
	Vector	GetChasePosition();
	void	TurnOn( void );
	void	TurnOff( void );
	void	Toggle( void );

	void	LaserThink( void );

protected:

	CHandle<CSprite>	m_hLaserDot;
	Vector				m_vecSurfaceNormal;
	EHANDLE				m_hTargetEnt;

	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------
// RPG
//-----------------------------------------------------------------------------

class CWeaponRPG : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponRPG, CBaseHLCombatWeapon );
public:

	CWeaponRPG();

	DECLARE_SERVERCLASS();

	void	Spawn( void );
	void	Precache( void );

	void	PrimaryAttack( void );
	virtual float GetFireRate( void ) { return 1; };
	void	ItemPostFrame( void );

	void	DecrementAmmo( CBaseCombatCharacter *pOwner );

	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	bool	Reload( void );

	virtual void Drop( const Vector &vecVelocity );

	void	StartGuiding( void );
	void	StopGuiding( void );
	void	ToggleGuiding( void );
	bool	IsGuiding( void );

	void	NotifyRocketDied( void );

	bool	HasAnyAmmo( void );

	void	CreateLaserPointer( void );
	void	UpdateLaserPosition( void );
	Vector	GetLaserPosition( void );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}

	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
	{
		switch( pEvent->event )
		{

			case EVENT_WEAPON_MISSILE_FIRE:
			{

				CMissile *pMissile;
				Vector vecOrigin;
				Vector vecForward;

				CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
				Assert( pOwner );

				pOwner->EyeVectors( &vecForward );

				vecOrigin = GetOwner()->EyePosition();
				vecOrigin = vecOrigin + vecForward * 32;

				WeaponSound(SINGLE_NPC);
				pMissile = CMissile::Create( vecOrigin, pOwner->EyeAngles(), GetOwner()->edict() );

				pMissile->m_hOwner = NULL;

				break;
			}
			default:
				BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
				break;
		}
	}
	
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
	
protected:

	bool				m_bInitialStateUpdate;
	bool				m_bGuiding;
	CHandle<CLaserDot>	m_hLaserDot;
	CHandle<CMissile>	m_hMissile;
};

#endif // WEAPON_RPG_H