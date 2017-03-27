//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef WEAPON_BASECSGRENADE_H
#define WEAPON_BASECSGRENADE_H
#ifdef _WIN32
#pragma once
#endif


#include "weapon_csbase.h"


#ifdef CLIENT_DLL
	
	#define CBaseCSGrenade C_BaseCSGrenade

#endif


class CBaseCSGrenade : public CWeaponCSBase
{
public:
	DECLARE_CLASS( CBaseCSGrenade, CWeaponCSBase );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CBaseCSGrenade() {}

#ifdef CLIENT_DLL

#else
	DECLARE_DATADESC();

	void	Precache();
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void	PrimaryAttack();
	void	SecondaryAttack();
	void	DecrementAmmo( CBaseCombatCharacter *pOwner );
	void	ItemPostFrame();

	bool	Deploy();
	bool	Holster();

	int		CapabilitiesGet();
	
	bool	Reload();

	// Each derived grenade class implements this.
	virtual void ThrowGrenade();


protected:

	bool	m_bRedraw;	// Draw the weapon again after throwing a grenade

#endif

	CBaseCSGrenade( const CBaseCSGrenade & ) {}
};


#endif // WEAPON_BASECSGRENADE_H
