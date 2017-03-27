/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/


#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "bone_setup.h"

class CWeapon_Manhack : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeapon_Manhack, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

	void			Spawn( void );
	void			Precache( void );

	void			ItemPostFrame( void );
	void			PrimaryAttack( void );
	void			SecondaryAttack( void );

	float			m_flBladeYaw;
};

IMPLEMENT_SERVERCLASS_ST( CWeapon_Manhack, DT_Weapon_Manhack)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_manhack, CWeapon_Manhack );
PRECACHE_WEAPON_REGISTER(weapon_manhack);

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CWeapon_Manhack )

	DEFINE_FIELD( CWeapon_Manhack, m_flBladeYaw,			FIELD_FLOAT ),

END_DATADESC()

void CWeapon_Manhack::Spawn( )
{
	// Call base class first
	BaseClass::Spawn();

	Precache( );
	SetModel( GetViewModel() );

	FallInit();// get ready to fall down.

	m_flBladeYaw = NULL;
	AddSolidFlags( FSOLID_NOT_SOLID );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::ItemPostFrame( void )
{
	WeaponIdle( );
}

void CWeapon_Manhack::Precache( void )
{
	BaseClass::Precache();
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::PrimaryAttack()
{
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CWeapon_Manhack::SecondaryAttack()
{
}



