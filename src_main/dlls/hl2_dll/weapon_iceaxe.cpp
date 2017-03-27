//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Ice Axe - all purpose pointy beating stick
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

#define	ICEAXE_RANGE	60
#define	ICEAXE_REFIRE	0.25f

ConVar    sk_plr_dmg_iceaxe			( "sk_plr_dmg_iceaxe","0");
ConVar    sk_npc_dmg_iceaxe			( "sk_npc_dmg_iceaxe","0");

//-----------------------------------------------------------------------------
// CWeaponIceAxe
//-----------------------------------------------------------------------------

class CWeaponIceAxe : public CBaseHLBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponIceAxe, CBaseHLBludgeonWeapon );

	DECLARE_SERVERCLASS();

	CWeaponIceAxe();

	float		GetRange( void )		{	return	ICEAXE_RANGE;	}
	float		GetFireRate( void )		{	return	ICEAXE_REFIRE;	}

	void		ImpactSound( bool isWorld )	{	return;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );
};

IMPLEMENT_SERVERCLASS_ST(CWeaponIceAxe, DT_WeaponIceAxe)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_iceaxe, CWeaponIceAxe );
PRECACHE_WEAPON_REGISTER( weapon_iceaxe );

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponIceAxe::CWeaponIceAxe( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponIceAxe::GetDamageForActivity( Activity hitActivity )
{
	if ( ( GetOwner() != NULL ) && ( GetOwner()->IsPlayer() ) )
		return sk_plr_dmg_iceaxe.GetFloat();
	
	return sk_npc_dmg_iceaxe.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponIceAxe::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );;
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat( 2.0f, 4.0f );
	punchAng.y = random->RandomFloat( -2.0f, -1.0f );
	punchAng.z = 0.0f;

	pPlayer->ViewPunch( punchAng ); 
}