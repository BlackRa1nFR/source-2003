//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basegrenade_shared.h"

class CBaseGrenadeTimed : public CBaseGrenade
{
public:
	DECLARE_CLASS( CBaseGrenadeTimed, CBaseGrenade );

	void	Spawn( void );
	void	Precache( void );
};
LINK_ENTITY_TO_CLASS( npc_handgrenade, CBaseGrenadeTimed );


void CBaseGrenadeTimed::Spawn( void )
{
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX );

	SetModel( "models/Weapons/w_grenade.mdl" ); 

	UTIL_SetSize(this, Vector( -4, -4, -4), Vector(4, 4, 4));

	m_fRegisteredSound = FALSE;

	QAngle angles;
	Vector vel = GetAbsVelocity();

	VectorAngles( vel, angles );
	SetLocalAngles( angles );
	
	SetTouch( &CBaseGrenadeTimed::BounceTouch );	// Bounce if touched
	
	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	SetThink( &CBaseGrenadeTimed::TumbleThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	// if the delay is < 0.1 seconds, don't fly anywhere
	if ((m_flDetonateTime - gpGlobals->curtime) < 0.1)
	{
		SetNextThink( gpGlobals->curtime );
		SetAbsVelocity( vec3_origin );
	}

	// m_nSequence = random->RandomInt( 3, 6 ); // FIXME: missing tumble animations
	m_flPlaybackRate = 1.0;

	// Tumble through the air
	// pGrenade->m_vecAngVelocity.x = -400;

	SetGravity(1.0);  // Don't change or throw calculations will be off!
	SetFriction(0.8);

	m_flDamage = 100;	// ????

	// Allow player to blow this puppy up in the air
	m_takedamage = DAMAGE_YES;
}



void CBaseGrenadeTimed::Precache( void )
{
	BaseClass::Precache( );

	engine->PrecacheModel("models/weapons/w_grenade.mdl");
}
