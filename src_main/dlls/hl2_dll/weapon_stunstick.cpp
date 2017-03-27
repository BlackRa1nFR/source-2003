//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Stun Stick- beating stick with a zappy end
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
#include "decals.h"
#include "AI_BaseNPC.h"
#include "npcevent.h"

#define	STUNSTICK_RANGE		48
#define	STUNSTICK_REFIRE	0.6f

ConVar    sk_plr_dmg_stunstick	( "sk_plr_dmg_stunstick","0");
ConVar    sk_npc_dmg_stunstick	( "sk_npc_dmg_stunstick","0");

//-----------------------------------------------------------------------------
// CWeaponStunStick
//-----------------------------------------------------------------------------

class CWeaponStunStick : public CBaseHLBludgeonWeapon
{
	DECLARE_CLASS( CWeaponStunStick, CBaseHLBludgeonWeapon );
public:

	CWeaponStunStick();

	DECLARE_SERVERCLASS();

	float		GetRange( void )		{ return STUNSTICK_RANGE; }
	float		GetFireRate( void )		{ return STUNSTICK_REFIRE; }

	void		ImpactEffect( trace_t &traceHit );

	float		GetDamageForActivity( Activity hitActivity );

	void		SecondaryAttack( void )	{}
	DECLARE_ACTTABLE();

	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
};

IMPLEMENT_SERVERCLASS_ST(CWeaponStunStick, DT_WeaponStunStick)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_stunstick, CWeaponStunStick );
PRECACHE_WEAPON_REGISTER( weapon_stunstick );

acttable_t CWeaponStunStick::m_acttable[] = 
{
	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
};

IMPLEMENT_ACTTABLE(CWeaponStunStick);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponStunStick::CWeaponStunStick( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponStunStick::GetDamageForActivity( Activity hitActivity )
{
	if ( ( GetOwner() != NULL ) && ( GetOwner()->IsPlayer() ) )
		return sk_plr_dmg_stunstick.GetFloat();
	
	return sk_npc_dmg_stunstick.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStunStick::ImpactEffect( trace_t &traceHit )
{
	//Glowing spark effect for hit
	//UTIL_DecalTrace( &m_trLineHit, "PlasmaGlowFade" );
	
	//FIXME: need new decals
	UTIL_ImpactTrace( &traceHit, DMG_CLUB );
}

void CWeaponStunStick::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_MELEE_HIT:
		{
			Vector vecShootOrigin = pOperator->Weapon_ShootPosition();
			CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( 70, Vector(-16,-16,-16), Vector(16,16,16), sk_npc_dmg_stunstick.GetFloat(), DMG_SHOCK | DMG_CLUB );
			
			// did I hit someone?
			if ( pHurt )
			{
				// play sound
				WeaponSound( MELEE_HIT );
				// punch angles
				// do effect?
			}
			else
			{
				WeaponSound( MELEE_MISS );
			}
		}
		break;
		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}
