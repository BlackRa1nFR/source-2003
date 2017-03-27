//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		The class from which all bludgeon melee
//				weapons are derived. 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "basehlcombatweapon.h"

//=========================================================
// CBaseHLBludgeonWeapon 
//=========================================================
class CBaseHLBludgeonWeapon : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CBaseHLBludgeonWeapon, CBaseHLCombatWeapon );
public:
	CBaseHLBludgeonWeapon();

	DECLARE_SERVERCLASS();

	virtual	void	Spawn( void );
	virtual	void	Precache( void );
	
	//Attack functions
	virtual	void	PrimaryAttack( void );
	virtual	void	SecondaryAttack( void );
	
	virtual void	ItemPostFrame( void );

	//Functions to select animation sequences 
	virtual int		GetPrimaryAttackActivity( void )	{	return	ACT_VM_HITCENTER;	}
	virtual int		GetSecondaryAttackActivity( void )	{	return	ACT_VM_HITCENTER2;	}

	virtual	float	GetFireRate( void )								{	return	0.2f;	}
	virtual float	GetRange( void )								{	return	32.0f;	}
	virtual	float	GetDamageForActivity( Activity hitActivity )	{	return	1.0f;	}

	virtual int		CapabilitiesGet( void );
	virtual	int		WeaponMeleeAttack1Condition( float flDot, float flDist );

private:
	virtual	void	ImpactEffect( trace_t &trace );
	virtual	void	ImpactSound( bool isWorld );
	void			Swing( int bIsSecondary );
	void			Hit( trace_t &traceHit, Activity nHitActivity );
	Activity		ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBasePlayer *pOwner );
};

