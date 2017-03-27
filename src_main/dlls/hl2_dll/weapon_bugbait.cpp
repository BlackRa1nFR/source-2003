//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "engine/IEngineSound.h"
#include "npcevent.h"
#include "in_buttons.h"

#include "grenade_bugbait.h"

//
// Bug Bait Weapon
//

class CWeaponBugBait : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponBugBait, CBaseHLCombatWeapon );
public:

	DECLARE_SERVERCLASS();

			CWeaponBugBait( void );
	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	bool	Deploy( void );
	bool	Holster( void );

	void	ItemPostFrame( void );
	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void ) {}
	void	ThrowGrenade( CBasePlayer *pPlayer );
	
	bool	HasAnyAmmo( void ) { return true; }
	
	bool	Reload( void );
	void	SetPickupTouch( void );
	void	Drop( const Vector &vecVelocity );

	void	SetSporeEmitterState( bool state = true );

	void	BaitTouch( CBaseEntity *pOther );

	DECLARE_DATADESC();

protected:

	bool		m_bRedraw;
	bool		m_bEmitSpores;
	SporeTrail	*m_pSporeTrail;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponBugBait, DT_WeaponBugBait)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_bugbait, CWeaponBugBait );
PRECACHE_WEAPON_REGISTER( weapon_bugbait );

BEGIN_DATADESC( CWeaponBugBait )

	DEFINE_FIELD( CWeaponBugBait, m_pSporeTrail,	FIELD_CLASSPTR ),
	DEFINE_FIELD( CWeaponBugBait, m_bRedraw,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CWeaponBugBait, m_bEmitSpores,	FIELD_BOOLEAN ),

DEFINE_FUNCTION( CWeaponBugBait, BaitTouch ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponBugBait::CWeaponBugBait( void )
{
	m_bRedraw		= false;
	m_pSporeTrail	= NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "npc_grenade_bugbait" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::PrimaryAttack( void )
{
	if ( m_bRedraw )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	// BUGBUG: Animation events don't work on the player right now!!!
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	SendWeaponAnim( ACT_VM_THROW );
	
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponBugBait::ThrowGrenade( CBasePlayer *pPlayer )
{
	Vector	vForward, vRight, vUp, vThrowPos, vThrowVel;
	
	pPlayer->EyeVectors( &vForward, &vRight, &vUp );

	vThrowPos = pPlayer->EyePosition();

	vThrowPos += vForward * 18.0f;
	vThrowPos += vRight * 12.0f;

	vThrowVel = vForward * 1000 + pPlayer->GetAbsVelocity();

	BugBaitGrenade_Create( vThrowPos, vec3_angle, vThrowVel, QAngle(600,random->RandomInt(-1200,1200),0), pPlayer );

	m_bRedraw = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER );

	Vector	offsetPos;

	offsetPos.Random( -16.0f, 16.0f );
	offsetPos.z = 0;

	UTIL_SetOrigin( this, GetAbsOrigin() + offsetPos );

	Vector vecNewVelocity;
	vecNewVelocity.Random( -1.0f, 1.0f );

	vecNewVelocity[2] = random->RandomFloat( 0.75f, 1.0f );
	vecNewVelocity	*= random->RandomInt( 50, 250 );

	SetAbsVelocity( vecNewVelocity );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CWeaponBugBait::BaitTouch( CBaseEntity *pOther )
{
	if ( pOther->IsPlayer() == false )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *) pOther;

	//Always equip
	pPlayer->Weapon_Equip( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponBugBait::SetPickupTouch( void )
{
	SetTouch( BaitTouch );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponBugBait::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	switch( pEvent->event )
	{
		case EVENT_WEAPON_THROW:
			ThrowGrenade( pOwner );
			break;

		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponBugBait::Reload( void )
{
	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	//See if we're attacking
	if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) )
	{
		PrimaryAttack();
	}

	if ( m_bRedraw )
	{
		Reload();
	}

	WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponBugBait::Deploy( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return false;

	/*
	if ( m_pSporeTrail == NULL )
	{
		m_pSporeTrail = SporeTrail::CreateSporeTrail();
		
		m_pSporeTrail->m_bEmit				= true;
		m_pSporeTrail->m_flSpawnRate		= 100.0f;
		m_pSporeTrail->m_flParticleLifetime	= 2.0f;
		m_pSporeTrail->m_flStartSize		= 1.0f;
		m_pSporeTrail->m_flEndSize			= 4.0f;
		m_pSporeTrail->m_flSpawnRadius		= 8.0f;

		m_pSporeTrail->m_vecEndColor		= Vector( 0, 0, 0 );

		CBaseViewModel *vm = pOwner->GetViewModel();
		
		if ( vm != NULL )
		{
			m_pSporeTrail->FollowEntity( vm->entindex() );
		}
	}
	*/

	m_bRedraw = false;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponBugBait::Holster( void )
{
	m_bRedraw = false;

	return BaseClass::Holster();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : true - 
//-----------------------------------------------------------------------------
void CWeaponBugBait::SetSporeEmitterState( bool state )
{
	m_bEmitSpores = state;
}
