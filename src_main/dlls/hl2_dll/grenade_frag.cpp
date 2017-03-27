//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basegrenade_shared.h"
#include "grenade_frag.h"

ConVar sk_plr_dmg_fraggrenade	( "sk_plr_dmg_fraggrenade","0");
ConVar sk_npc_dmg_fraggrenade	( "sk_npc_dmg_fraggrenade","0");
ConVar sk_fraggrenade_radius	( "sk_fraggrenade_radius", "0");

#define GRENADE_MODEL "models/Weapons/w_grenade.mdl"

class CGrenadeFrag : public CBaseGrenade
{
	DECLARE_CLASS( CGrenadeFrag, CBaseGrenade );
public:
	void			Spawn( void );
	void			Precache( void );
	bool			CreateVPhysics( void );
	void			SetTimer( float timer );
	void			SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	void			TakeDamage( const CTakeDamageInfo &inputInfo );
};

LINK_ENTITY_TO_CLASS( npc_grenade_frag, CGrenadeFrag );


void CGrenadeFrag::Spawn( void )
{
	Precache( );

	SetModel( GRENADE_MODEL );

	m_flDamage		= sk_plr_dmg_fraggrenade.GetFloat();
	m_DmgRadius		= sk_fraggrenade_radius.GetFloat();
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	CreateVPhysics();
}

bool CGrenadeFrag::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, 0, false );
	return true;
}


void CGrenadeFrag::Precache( void )
{
	engine->PrecacheModel( GRENADE_MODEL );
	BaseClass::Precache();
}

void CGrenadeFrag::SetTimer( float timer )
{
	SetThink( PreDetonate );
	SetNextThink( gpGlobals->curtime + timer );
}


void CGrenadeFrag::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}

void CGrenadeFrag::TakeDamage( const CTakeDamageInfo &inputInfo )
{
	// Grenades only suffer blast damage and burn damage.
	if( !(inputInfo.GetDamageType() & (DMG_BLAST|DMG_BURN) ) )
		return;

	BaseClass::TakeDamage( inputInfo );
}

CBaseGrenade *Fraggrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer )
{
	CGrenadeFrag *pGrenade = (CGrenadeFrag *)CBaseEntity::Create( "npc_grenade_frag", position, angles, pOwner );
	
	// Set the timer for 1 second less than requested. We're going to issue a SOUND_DANGER
	// one second before detonation.
	pGrenade->SetTimer( timer - 1.5 );
	pGrenade->SetVelocity( velocity, angVelocity );
	pGrenade->SetOwner( ToBaseCombatCharacter( pOwner ) );
	pGrenade->m_takedamage = DAMAGE_YES;

	return pGrenade;
}

