//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Crowbar - an old favorite
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib.h"
#include "in_buttons.h"
#include "soundent.h"
#include "BaseBludgeonWeapon.h"
#include "vstdlib/random.h"

ConVar    sk_plr_dmg_crowbar		( "sk_plr_dmg_crowbar","0");
ConVar    sk_npc_dmg_crowbar		( "sk_npc_dmg_crowbar","0");

#define	CROWBAR_RANGE	55.0f
#define	CROWBAR_REFIRE	0.4f

//-----------------------------------------------------------------------------
// CWeaponCrowbar
//-----------------------------------------------------------------------------

class CWeaponCrowbar : public CBaseHLBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponCrowbar, CBaseHLBludgeonWeapon );

	DECLARE_SERVERCLASS();

	CWeaponCrowbar();

	float		GetRange( void )		{	return	CROWBAR_RANGE;	}
	float		GetFireRate( void )		{	return	CROWBAR_REFIRE;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );

	void		SecondaryAttack( void )	{	return;	}
};

IMPLEMENT_SERVERCLASS_ST(CWeaponCrowbar, DT_WeaponCrowbar)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_crowbar, CWeaponCrowbar );
PRECACHE_WEAPON_REGISTER( weapon_crowbar );

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponCrowbar::CWeaponCrowbar( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponCrowbar::GetDamageForActivity( Activity hitActivity )
{
	if ( ( GetOwner() != NULL ) && ( GetOwner()->IsPlayer() ) )
		return sk_plr_dmg_crowbar.GetFloat();

	return sk_npc_dmg_crowbar.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponCrowbar::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat( 1.0f, 2.0f );
	punchAng.y = random->RandomFloat( -2.0f, -1.0f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng ); 
}