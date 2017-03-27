//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Projectile shot from the AR2 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef	WEAPONAR2_H
#define	WEAPONAR2_H

#include "basegrenade_shared.h"
#include "basehlcombatweapon.h"

class CGrenadeAR2;

class CWeaponAR2 : public CHLMachineGun
{
public:
	DECLARE_CLASS( CWeaponAR2, CHLMachineGun );

	CWeaponAR2();

	DECLARE_SERVERCLASS();

	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	bool	Reload( void );
	float	GetFireRate( void );
	void	ItemPostFrame( void );
	void	Precache( void );
	void	PrimaryAttack( void );
	void	AddViewKick( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	int		GetMinBurst() { return 4; }
	int		GetMaxBurst() { return 7; }

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	int		GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		
		if( GetOwner() && GetOwner()->IsPlayer() )
		{
			cone = ( m_bZoomed ) ? VECTOR_CONE_1DEGREES : VECTOR_CONE_3DEGREES;
		}
		else
		{
			cone = VECTOR_CONE_8DEGREES;
		}
		
		return cone;
	}
	
	virtual bool	Deploy( void );
	virtual void	Drop( const Vector &velocity );

protected:

	void			Zoom( void );

	int				m_nShotsFired;
	bool			m_bZoomed;

	static const char *pShootSounds[];

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
};


#endif	//WEAPONAR2_H
