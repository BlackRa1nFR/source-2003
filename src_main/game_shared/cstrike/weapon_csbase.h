//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef WEAPON_CSBASE_H
#define WEAPON_CSBASE_H
#ifdef _WIN32
#pragma once
#endif


#include "cs_playeranimstate.h"
#include "cs_weapon_parse.h"


#if defined( CLIENT_DLL )
	#define CWeaponCSBase C_WeaponCSBase
#endif


class CCSPlayer;


// bullet types
/*typedef	enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,

	//GOOSEMAN's new bullets
	BULLET_PLAYER_45ACP,
	BULLET_PLAYER_338MAG,
	BULLET_PLAYER_762MM,
	BULLET_PLAYER_556MM,
	BULLET_PLAYER_50AE,
	BULLET_PLAYER_57MM,
	BULLET_PLAYER_357SIG,
} Bullet;
*/
// These are the names of the ammo types that go in the CAmmoDefs and that the 
// weapon script files reference.
#define BULLET_PLAYER_50AE		"BULLET_PLAYER_50AE"
#define BULLET_PLAYER_762MM		"BULLET_PLAYER_762MM"
#define BULLET_PLAYER_556MM		"BULLET_PLAYER_556MM"
#define BULLET_PLAYER_338MAG	"BULLET_PLAYER_338MAG"
#define BULLET_PLAYER_9MM		"BULLET_PLAYER_9MM"
#define BULLET_PLAYER_BUCKSHOT	"BULLET_PLAYER_BUCKSHOT"
#define BULLET_PLAYER_45ACP		"BULLET_PLAYER_45ACP"
#define BULLET_PLAYER_357SIG	"BULLET_PLAYER_357SIG"
#define BULLET_PLAYER_57MM		"BULLET_PLAYER_57MM"
#define AMMO_TYPE_HEGRENADE		"AMMO_TYPE_HEGRENADE"
#define AMMO_TYPE_FLASHBANG		"AMMO_TYPE_FLASHBANG"


// Given an ammo type (like from a weapon's GetPrimaryAmmoType()), this compares it
// against the ammo name you specify.
// MIKETODO: this should use indexing instead of searching and strcmp()'ing all the time.
bool IsAmmoType( int iAmmoType, const char *pAmmoName );


class CWeaponCSBase : public CBaseCombatWeapon
{
public:
	DECLARE_CLASS( CWeaponCSBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponCSBase();

	#ifdef GAME_DLL
		DECLARE_DATADESC();

		virtual void CheckRespawn();
		virtual CBaseEntity* Respawn();
		
		virtual const Vector& GetBulletSpread();
		virtual void	ItemBusyFrame();
		virtual float 	GetFireRate();
		virtual float	GetDefaultAnimSpeed();
		virtual void	BulletWasFired( const Vector &vecStart, const Vector &vecEnd );
		virtual bool	ShouldRemoveOnRoundRestart();

		void RecalculateAccuracy();

		void Materialize();
		void AttemptToMaterialize();
	#endif

	// All predicted weapons need to implement and return true
	virtual bool	IsPredicted() const;

	// Pistols reset m_iShotsFired to 0 when the attack button is released.
	bool			IsPistol() const;

	CCSPlayer* GetPlayerOwner() const;

	virtual float GetMaxSpeed() const;	// What's the player's max speed while holding this weapon.

	// Get CS-specific weapon data.
	CCSWeaponInfo const	&GetCSWpnData() const;

	void KickBack( float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change );


public:
	#if defined( CLIENT_DLL )
		
		virtual bool	ShouldPredict();
		virtual void	DrawCrosshair();
	
	#else

		virtual bool	Reload();
		virtual bool	Deploy();
		bool IsUseable();

	#endif

	bool PlayEmptySound();
	virtual void	ItemPostFrame();


	bool	m_bDelayFire;			// This variable is used to delay the time between subsequent button pressing.
	int		m_iShotsFired;
	float	m_flGlock18Shoot;		// time to shoot the remaining bullets of the glock18 burst fire
	float	m_flAccuracy;
	int		m_iDirection;			// The current lateral kicking direction; 1 = right,  0 = left


private:

	#ifdef CLIENT_DLL
		
	#else
		
	#endif


	void EjectBrassLate();


	float	m_flInaccuracy;
	float	m_flAccuracyTime;
	float	m_flDecreaseShotsFired;

	CWeaponCSBase( const CWeaponCSBase & );
};


#endif // WEAPON_CSBASE_H
