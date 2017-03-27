//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Satchel charge
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "NPCEvent.h"
#include "basecombatweapon.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "hl1_weapon_satchel.h"


//-----------------------------------------------------------------------------
// CWeaponSatchel
//-----------------------------------------------------------------------------


#define SATCHEL_VIEW_MODEL			"models/v_satchel.mdl"
#define SATCHEL_WORLD_MODEL			"models/w_satchel.mdl"
#define SATCHELRADIO_VIEW_MODEL		"models/v_satchel_radio.mdl"
#define SATCHELRADIO_WORLD_MODEL	"models/w_satchel.mdl"	// this needs fixing if we do multiplayer

LINK_ENTITY_TO_CLASS( weapon_satchel, CWeaponSatchel );

PRECACHE_WEAPON_REGISTER( weapon_satchel );

IMPLEMENT_SERVERCLASS_ST( CWeaponSatchel, DT_WeaponSatchel )
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponSatchel )
	DEFINE_FIELD( CWeaponSatchel, m_iRadioViewIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CWeaponSatchel, m_iRadioWorldIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CWeaponSatchel, m_iSatchelViewIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CWeaponSatchel, m_iSatchelWorldIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CWeaponSatchel, m_iChargeReady, FIELD_INTEGER ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSatchel::CWeaponSatchel( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= true;
}

void CWeaponSatchel::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

	m_iChargeReady = 0;	// this satchel charge weapon now forgets that any satchels are deployed by it.
}

bool CWeaponSatchel::HasAnyAmmo( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
	{
		return false;
	}

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		// player is carrying some satchels
		return true;
	}

	if ( HasChargeDeployed() )
	{
		// player isn't carrying any satchels, but has some out
		return true;
	}

	return BaseClass::HasAnyAmmo();
}

bool CWeaponSatchel::CanDeploy( void )
{
	if ( HasAnyAmmo() )
	{
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSatchel::Precache( void )
{
	m_iSatchelViewIndex		= PrecacheModel( SATCHEL_VIEW_MODEL );
	m_iSatchelWorldIndex	= PrecacheModel( SATCHEL_WORLD_MODEL );
	m_iRadioViewIndex		= PrecacheModel( SATCHELRADIO_VIEW_MODEL );
	m_iRadioWorldIndex		= PrecacheModel( SATCHELRADIO_WORLD_MODEL );

	UTIL_PrecacheOther( "monster_satchel" );

	BaseClass::Precache();
}

void CWeaponSatchel::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		return;
	}

	if ( (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		// If the firing button was just pressed, reset the firing time
		if ( pOwner->m_afButtonPressed & IN_ATTACK )
		{
			 m_flNextPrimaryAttack = gpGlobals->curtime;
		}

		PrimaryAttack();
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponSatchel::PrimaryAttack( void )
{
	switch ( m_iChargeReady )
	{
	case 0:
		{
			Throw();
		}
		break;
	case 1:
		{
			CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
			if ( !pPlayer )
			{
				return;
			}

			SendWeaponAnim( ACT_VM_PRIMARYATTACK );

			CBaseEntity *pSatchel = NULL;

			while ( (pSatchel = gEntList.FindEntityByClassname( pSatchel, "monster_satchel" ) ) != NULL)
			{
				if ( pSatchel->GetOwnerEntity() == pPlayer )
				{
					pSatchel->Use( pPlayer, pPlayer, USE_ON, 0 );
				}
			}

			m_iChargeReady = 2;
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
			m_flTimeWeaponIdle = gpGlobals->curtime + 0.5;
			break;
		}

	case 2:
		// we're reloading, don't allow fire
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponSatchel::SecondaryAttack( void )
{
	if ( m_iChargeReady != 2 )
	{
		Throw();
	}
}

void CWeaponSatchel::Throw( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		Vector vecForward;
		pPlayer->EyeVectors( &vecForward );

		Vector vecSrc	= pPlayer->WorldSpaceCenter();
		Vector vecThrow	= vecForward * 274 + pPlayer->GetAbsVelocity();

		CBaseEntity *pSatchel = Create( "monster_satchel", vecSrc, vec3_angle, pPlayer );
		if ( pSatchel )
		{
			pSatchel->SetAbsVelocity( vecThrow );
			QAngle angVel = pSatchel->GetLocalAngularVelocity();
			angVel.y = 400;
			pSatchel->SetLocalAngularVelocity( angVel );

			ActivateRadioModel();

			SendWeaponAnim( ACT_VM_DRAW );

			// player "shoot" animation
			pPlayer->SetAnimation( PLAYER_ATTACK1 );

			m_iChargeReady = 1;
			
			pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );

		}

		m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
	}
}

void CWeaponSatchel::WeaponIdle( void )
{
	if ( !HasWeaponIdleTimeElapsed() )
		return;

	switch( m_iChargeReady )
	{
		case 0:
		case 1:
			SendWeaponAnim( ACT_VM_FIDGET );
			break;
		case 2:
		{
			CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

			if ( pPlayer && (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0) )
			{
				m_iChargeReady = 0;
				if ( !pPlayer->SwitchToNextBestWeapon( pPlayer->GetActiveWeapon() ) )
				{
					Holster();
				}

				return;
			}

			ActivateSatchelModel();

			SendWeaponAnim( ACT_VM_DRAW );

			m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
			m_iChargeReady = 0;
			break;
		}
	}

	SetWeaponIdleTime( gpGlobals->curtime + random->RandomFloat( 10, 15 ) );// how long till we do this again.
}

bool CWeaponSatchel::Deploy( void )
{
	SetWeaponIdleTime( gpGlobals->curtime + random->RandomFloat( 10, 15 ) );

	if ( HasChargeDeployed() )
	{
		ActivateRadioModel();
	}
	else
	{
		ActivateSatchelModel();
	}
	
	bool bRet = BaseClass::Deploy();
	if ( bRet )
	{
		// 
		CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
		if ( pPlayer )
		{
			pPlayer->SetNextAttack( gpGlobals->curtime + 1.0 );
		}
	}

	return bRet;

}

bool CWeaponSatchel::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( !BaseClass::Holster( pSwitchingTo ) )
	{
		return false;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		pPlayer->SetNextAttack( gpGlobals->curtime + 0.5 );

		if ( (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0) && !HasChargeDeployed() )
		{
			SetThink( &CWeaponSatchel::DestroyItem );
			SetNextThink( gpGlobals->curtime + 0.1 );
		}
	}

	return true;
}

void CWeaponSatchel::ActivateSatchelModel( void )
{
	m_iViewModelIndex	= m_iSatchelViewIndex;
	m_iWorldModelIndex	= m_iSatchelWorldIndex;
	SetModel( GetViewModel() );
}

void CWeaponSatchel::ActivateRadioModel( void )
{
	m_iViewModelIndex	= m_iRadioViewIndex;
	m_iWorldModelIndex	= m_iRadioWorldIndex;
	SetModel( GetViewModel() );
}

const char *CWeaponSatchel::GetViewModel( int ) const
{
	if ( m_iViewModelIndex == m_iSatchelViewIndex )
	{
		return SATCHEL_VIEW_MODEL;
	}
	else
	{
		return SATCHELRADIO_VIEW_MODEL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CWeaponSatchel::GetWorldModel( void ) const
{
	if ( m_iViewModelIndex == m_iSatchelViewIndex )
	{
		return SATCHEL_WORLD_MODEL;
	}
	else
	{
		return SATCHELRADIO_WORLD_MODEL;
	}
}


//-----------------------------------------------------------------------------
// CSatchelCharge
//-----------------------------------------------------------------------------

#define SATCHEL_CHARGE_MODEL "models/w_satchel.mdl"


extern ConVar sk_plr_dmg_satchel;


BEGIN_DATADESC( CSatchelCharge )
	DEFINE_FIELD( CSatchelCharge, m_flNextBounceSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CSatchelCharge, m_bInAir, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSatchelCharge, m_vLastPosition, FIELD_POSITION_VECTOR ),

	// Function Pointers
	DEFINE_FUNCTION( CSatchelCharge, SatchelTouch ),
	DEFINE_FUNCTION( CSatchelCharge, SatchelThink ),
	DEFINE_FUNCTION( CSatchelCharge, SatchelUse ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( monster_satchel, CSatchelCharge );

//=========================================================
// Deactivate - do whatever it is we do to an orphaned 
// satchel when we don't want it in the world anymore.
//=========================================================
void CSatchelCharge::Deactivate( void )
{
	AddSolidFlags( FSOLID_NOT_SOLID );
	UTIL_Remove( this );
}


void CSatchelCharge::Spawn( void )
{
	Precache( );
	// motor
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_SLIDE );
	SetSolid( SOLID_BBOX ); 

	SetModel( SATCHEL_CHARGE_MODEL );

	UTIL_SetSize( this, Vector( -4, -4, 0), Vector(4, 4, 8) );
	Relink();

	SetTouch( SatchelTouch );
	SetUse( SatchelUse );
	SetThink( SatchelThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= sk_plr_dmg_satchel.GetFloat();
	m_DmgRadius		= m_flDamage * 2.5;
	m_takedamage	= DAMAGE_NO;
	m_iHealth		= 1;

	SetGravity( 0.7 );
	SetFriction( 1.0 );
	SetSequence( LookupSequence( "onback" ) );

	m_bInAir				= false;
	m_flNextBounceSoundTime	= 0;

	m_vLastPosition	= vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CSatchelCharge::SatchelUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( Detonate );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CSatchelCharge::SatchelTouch( CBaseEntity *pOther )
{
	Assert( pOther );
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) )
		return;

	StudioFrameAdvance( );

	SetGravity( 1 );// normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector(0,0,10), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0 )
	{
		// add a bit of static friction
		SetAbsVelocity( GetAbsVelocity() * 0.85 );
		SetLocalAngularVelocity( GetLocalAngularVelocity() * 0.8 );
	}

	UpdateSlideSound();

	if ( m_bInAir )
	{
		BounceSound();
		m_bInAir = false;
	}

}

void CSatchelCharge::UpdateSlideSound( void )
{	
	// HACKHACK - On ground isn't always set, so look for ground underneath
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector(0,0,10), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( !(tr.fraction < 1.0) )
	{
		m_bInAir = true;
	}
}

void CSatchelCharge::SatchelThink( void )
{
	UpdateSlideSound();

	// Bounce movement code gets this think stuck occasionally so check if I've 
	// succeeded in moving, otherwise kill my motions.
	if ((GetAbsOrigin() - m_vLastPosition).LengthSqr()<1)
	{
		SetAbsVelocity( vec3_origin );

		QAngle angVel = GetLocalAngularVelocity();
		angVel.y  = 0;
		SetLocalAngularVelocity( angVel );

		// Clear think function
		SetThink(NULL);
		return;
	}
	m_vLastPosition= GetAbsOrigin();

	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	Vector vecNewVel = GetAbsVelocity();
	if ( GetWaterLevel() == 3 )
	{
		SetMoveType( MOVETYPE_FLY );
		vecNewVel *= 0.8;
		vecNewVel.z += 8;
		SetLocalAngularVelocity( GetLocalAngularVelocity() * 0.9 );
	}
	else if ( GetWaterLevel() == 0 )
	{
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	}
	else
	{
		vecNewVel.z -= 8;
	}

	SetAbsVelocity( vecNewVel );
}

void CSatchelCharge::Precache( void )
{
	engine->PrecacheModel( SATCHEL_CHARGE_MODEL );
}

void CSatchelCharge::BounceSound( void )
{
	if ( gpGlobals->curtime > m_flNextBounceSoundTime )
	{
		EmitSound( "SatchelCharge.Bounce" );

		m_flNextBounceSoundTime = gpGlobals->curtime + 0.1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CSatchelCharge::CSatchelCharge(void)
{
	m_vLastPosition.Init();
}
