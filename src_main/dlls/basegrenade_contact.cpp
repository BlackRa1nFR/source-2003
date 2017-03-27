//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basegrenade_shared.h"

extern ConVar    sk_plr_dmg_grenade;

// ==========================================================================================

class CBaseGrenadeContact : public CBaseGrenade
{
	DECLARE_CLASS( CBaseGrenadeContact, CBaseGrenade );
public:

	void Spawn( void );
	void Precache( void );
};
LINK_ENTITY_TO_CLASS( npc_contactgrenade, CBaseGrenadeContact );


void CBaseGrenadeContact::Spawn( void )
{
	// point sized, solid, bouncing
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX );

	SetModel( "models/weapons/w_grenade.mdl" );	// BUG: wrong model

	UTIL_SetSize(this, vec3_origin, vec3_origin);

	m_fRegisteredSound = FALSE;

	// contact grenades arc lower
	SetGravity(0.5);// lower gravity since grenade is aerodynamic and engine doesn't know it.

	QAngle angles;
	VectorAngles(GetAbsVelocity(), angles);
	SetLocalAngles( angles );
	
	// make NPCs afaid of it while in the air
	SetThink( &CBaseGrenadeContact::DangerSoundThink );
	SetNextThink( gpGlobals->curtime );
	
	// Tumble in air
	QAngle vecAngVelocity( random->RandomFloat ( -100, -500 ), 0, 0 );
	SetLocalAngularVelocity( vecAngVelocity );

	// Explode on contact
	SetTouch( &CBaseGrenadeContact::ExplodeTouch );

	m_flDamage = sk_plr_dmg_grenade.GetFloat();

	// Allow player to blow this puppy up in the air
	m_takedamage	= DAMAGE_YES;

	m_iszBounceSound = NULL_STRING;
}


void CBaseGrenadeContact::Precache( void )
{
	BaseClass::Precache( );

	engine->PrecacheModel("models/weapons/w_grenade.mdl");
}
