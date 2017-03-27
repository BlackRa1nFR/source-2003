//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Projectile shot from the MP5 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	WEAPONMP5_H
#define	WEAPONMP5_H

#include "hl1_basegrenade.h"
#include "hl1_basecombatweapon_shared.h"

class CGrenadeMP5;

class CWeaponMP5 : public CBaseHL1CombatWeapon
{
	DECLARE_CLASS( CWeaponMP5, CBaseHL1CombatWeapon );
public:

	CWeaponMP5();

	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	DryFire( void );
	void	WeaponIdle( void );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};


#endif	//WEAPONMP5_H
