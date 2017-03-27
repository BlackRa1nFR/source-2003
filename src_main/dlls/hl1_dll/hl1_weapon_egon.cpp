//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Egon
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "NPCEvent.h"
#include "hl1_basecombatweapon_shared.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "hl1_player.h"
#include "sprite.h"
#include "shake.h"
#include "beam_shared.h"
#include "beam_flags.h"
#include "SoundEmitterSystemBase.h"
#include "soundenvelope.h"



//-----------------------------------------------------------------------------
// CWeaponEgon
//-----------------------------------------------------------------------------


#define EGON_PULSE_INTERVAL			0.1
#define EGON_DISCHARGE_INTERVAL		0.1

#define EGON_BEAM_SPRITE		"sprites/xbeam1.vmt"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.vmt"

extern ConVar sk_plr_dmg_egon_wide;


enum EGON_FIRESTATE { FIRE_OFF, FIRE_STARTUP, FIRE_CHARGE };


class CWeaponEgon : public CBaseHL1CombatWeapon
{
	DECLARE_CLASS( CWeaponEgon, CBaseHL1CombatWeapon );
public:

	CWeaponEgon( void );

	void	Precache( void );
	bool	Deploy( void );
	void	PrimaryAttack( void );
	void	WeaponIdle( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	bool	HasAmmo( void );
	void	UseAmmo( int count );
	void	Attack( void );
	void	EndAttack( void );
	void	Fire( const Vector &vecOrigSrc, const Vector &vecDir );
	void	UpdateEffect( const Vector &startPoint, const Vector &endPoint );
	void	CreateEffect( void );
	void	DestroyEffect( void );

	EGON_FIRESTATE		m_fireState;
	float				m_flAmmoUseTime;	// since we use < 1 point of ammo per update, we subtract ammo on a timer.
	float				m_flShakeTime;
	float				m_flStartFireTime;
	float				m_flDmgTime;
	CHandle<CSprite>	m_hSprite;
	CHandle<CBeam>		m_hBeam;
	CHandle<CBeam>		m_hNoise;
};

LINK_ENTITY_TO_CLASS( weapon_egon, CWeaponEgon );

PRECACHE_WEAPON_REGISTER( weapon_egon );

IMPLEMENT_SERVERCLASS_ST( CWeaponEgon, DT_WeaponEgon )
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponEgon )
	DEFINE_FIELD( CWeaponEgon, m_fireState, FIELD_INTEGER ),
	DEFINE_FIELD( CWeaponEgon, m_flAmmoUseTime, FIELD_TIME ),
	DEFINE_FIELD( CWeaponEgon, m_flShakeTime, FIELD_TIME ),
	DEFINE_FIELD( CWeaponEgon, m_flStartFireTime, FIELD_TIME ),
	DEFINE_FIELD( CWeaponEgon, m_flDmgTime, FIELD_TIME ),
	DEFINE_FIELD( CWeaponEgon, m_hSprite, FIELD_EHANDLE ),
	DEFINE_FIELD( CWeaponEgon, m_hBeam, FIELD_EHANDLE ),
	DEFINE_FIELD( CWeaponEgon, m_hNoise, FIELD_EHANDLE ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponEgon::CWeaponEgon( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponEgon::Precache( void )
{

	BaseClass::Precache();
}

bool CWeaponEgon::Deploy( void )
{
	m_fireState = FIRE_OFF;

	return BaseClass::Deploy();
}

bool CWeaponEgon::HasAmmo( void )
{
	CHL1_Player *pPlayer = ToHL1Player( GetOwner() );
	if ( !pPlayer )
	{
		return false;
	}

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return false;

	return true;
}

void CWeaponEgon::UseAmmo( int count )
{
	CHL1_Player *pPlayer = ToHL1Player( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) >= count )
		pPlayer->RemoveAmmo( count, m_iPrimaryAmmoType );
	else
		pPlayer->RemoveAmmo( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ), m_iPrimaryAmmoType );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponEgon::PrimaryAttack( void )
{
	CHL1_Player *pPlayer = ToHL1Player( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	// don't fire underwater
	if ( pPlayer->GetWaterLevel() == 3 )
	{
		if ( m_fireState != FIRE_OFF || m_hBeam )
		{
			EndAttack();
		}
		else
		{
			WeaponSound( EMPTY );
		}

		m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
		return;
	}

	Vector vecAiming	= pPlayer->GetAutoaimVector( 0 );
	Vector vecSrc		= pPlayer->Weapon_ShootPosition( );

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if ( !HasAmmo() )
			{
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.25;
				WeaponSound( EMPTY );
				return;
			}

			m_flAmmoUseTime = gpGlobals->curtime;// start using ammo ASAP.

			EmitSound( "Weapon_Gluon.Start" );

			SendWeaponAnim( ACT_VM_PRIMARYATTACK );
						
			m_flShakeTime = 0;
			m_flStartFireTime = gpGlobals->curtime;

			SetWeaponIdleTime( gpGlobals->curtime + 0.1 );

			m_flDmgTime = gpGlobals->curtime + EGON_PULSE_INTERVAL;
			m_fireState = FIRE_STARTUP;
		}
		break;

		case FIRE_STARTUP:
		{
			Fire( vecSrc, vecAiming );
		
			if ( gpGlobals->curtime >= ( m_flStartFireTime + 2.0 ) )
			{
				EmitSound( "Weapon_Gluon.Run" );

				m_fireState = FIRE_CHARGE;
			}

			if ( !HasAmmo() )
			{
				EndAttack();
				m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
			}
		}
		case FIRE_CHARGE:
		{
			Fire( vecSrc, vecAiming );
		
			if ( !HasAmmo() )
			{
				EndAttack();
				m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
			}
		}
		break;
	}
}

void CWeaponEgon::Fire( const Vector &vecOrigSrc, const Vector &vecDir )
{
	CHL1_Player *pPlayer = ToHL1Player( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 450, 0.1 );

	Vector vecDest	= vecOrigSrc + (vecDir * MAX_TRACE_LENGTH);

	trace_t	tr;
	UTIL_TraceLine( vecOrigSrc, vecDest, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.allsolid )
		return;

	CBaseEntity *pEntity = tr.m_pEnt;
	if ( pEntity == NULL )
		return;

	if ( g_pGameRules->IsMultiplayer() )
	{
		if ( m_hSprite )
		{
			if ( pEntity->m_takedamage != DAMAGE_NO )
			{
				m_hSprite->TurnOn();
			}
			else
			{
				m_hSprite->TurnOff();
			}
		}
	}

	if ( m_flDmgTime < gpGlobals->curtime )
	{
		// wide mode does damage to the ent, and radius damage
		if ( pEntity->m_takedamage != DAMAGE_NO )
		{
			ClearMultiDamage();
			CTakeDamageInfo info( this, pPlayer, sk_plr_dmg_egon_wide.GetFloat(), DMG_ENERGYBEAM | DMG_ALWAYSGIB );
			CalculateMeleeDamageForce( &info, vecDir, tr.endpos );
			pEntity->DispatchTraceAttack( info, vecDir, &tr );
			ApplyMultiDamage();
		}

		if ( g_pGameRules->IsMultiplayer() )
		{
			// radius damage a little more potent in multiplayer.
			RadiusDamage( CTakeDamageInfo( this, pPlayer, sk_plr_dmg_egon_wide.GetFloat() / 4, DMG_ENERGYBEAM | DMG_BLAST | DMG_ALWAYSGIB ), tr.endpos, 128, CLASS_NONE );
		}

		if ( !pPlayer->IsAlive() )
			return;

		if ( g_pGameRules->IsMultiplayer() )
		{
			//multiplayer uses 5 ammo/second
			if ( gpGlobals->curtime >= m_flAmmoUseTime )
			{
				UseAmmo( 1 );
				m_flAmmoUseTime = gpGlobals->curtime + 0.2;
			}
		}
		else
		{
			// Wide mode uses 10 charges per second in single player
			if ( gpGlobals->curtime >= m_flAmmoUseTime )
			{
				UseAmmo( 1 );
				m_flAmmoUseTime = gpGlobals->curtime + 0.1;
			}
		}

		m_flDmgTime = gpGlobals->curtime + EGON_DISCHARGE_INTERVAL;
		if ( m_flShakeTime < gpGlobals->curtime )
		{
			UTIL_ScreenShake( tr.endpos, 5.0, 150.0, 0.75, 250.0, SHAKE_START );
			m_flShakeTime = gpGlobals->curtime + 1.5;
		}
	}

	Vector vecUp, vecRight;
	QAngle angDir;

	VectorAngles( vecDir, angDir );
	AngleVectors( angDir, NULL, &vecRight, &vecUp );

	Vector tmpSrc = vecOrigSrc + (vecUp * -8) + (vecRight * 3);
	UpdateEffect( tmpSrc, tr.endpos );
}

void CWeaponEgon::UpdateEffect( const Vector &startPoint, const Vector &endPoint )
{
	if ( !m_hBeam )
	{
		CreateEffect();
	}

	if ( m_hBeam )
	{
		m_hBeam->SetStartPos( endPoint );
		UTIL_Relink( m_hBeam );
	}

	if ( m_hSprite )
	{
		m_hSprite->SetAbsOrigin( endPoint );
		UTIL_Relink( m_hSprite );

		m_hSprite->m_flFrame += 8 * gpGlobals->frametime;
		if ( m_hSprite->m_flFrame > m_hSprite->Frames() )
			m_hSprite->m_flFrame = 0;
	}

	if ( m_hNoise )
	{
		m_hNoise->SetStartPos( endPoint );
		UTIL_Relink( m_hNoise );
	}
}

void CWeaponEgon::CreateEffect( void )
{
	CHL1_Player *pPlayer = ToHL1Player( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	DestroyEffect();

	m_hBeam = CBeam::BeamCreate( EGON_BEAM_SPRITE, 3.5 );
	m_hBeam->PointEntInit( GetAbsOrigin(), this );
	m_hBeam->SetBeamFlags( FBEAM_SINENOISE );
	m_hBeam->SetEndAttachment( 1 );
	m_hBeam->AddSpawnFlags( SF_BEAM_TEMPORARY );	// Flag these to be destroyed on save/restore or level transition
	m_hBeam->SetOwnerEntity( pPlayer );
	m_hBeam->SetScrollRate( 10 );
	m_hBeam->SetBrightness( 200 );
	m_hBeam->SetColor( 50, 50, 255 );
	m_hBeam->SetNoise( 0.2 );

	m_hNoise = CBeam::BeamCreate( EGON_BEAM_SPRITE, 5.0 );
	m_hNoise->PointEntInit( GetAbsOrigin(), this );
	m_hNoise->SetEndAttachment( 1 );
	m_hNoise->AddSpawnFlags( SF_BEAM_TEMPORARY );
	m_hNoise->SetOwnerEntity( pPlayer );
	m_hNoise->SetScrollRate( 25 );
	m_hNoise->SetBrightness( 200 );
	m_hNoise->SetColor( 50, 50, 255 );
	m_hNoise->SetNoise( 0.8 );

	m_hSprite = CSprite::SpriteCreate( EGON_FLARE_SPRITE, GetAbsOrigin(), false );
	m_hSprite->SetScale( 1.0 );
	m_hSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
	m_hSprite->AddSpawnFlags( SF_SPRITE_TEMPORARY );
	m_hSprite->SetOwnerEntity( pPlayer );
}


void CWeaponEgon::DestroyEffect( void )
{
	if ( m_hBeam )
	{
		UTIL_Remove( m_hBeam );
		m_hBeam = NULL;
	}
	if ( m_hNoise )
	{
		UTIL_Remove( m_hNoise );
		m_hNoise = NULL;
	}
	if ( m_hSprite )
	{
		m_hSprite->Expand( 10, 500 );
		m_hSprite = NULL;
	}
}

void CWeaponEgon::EndAttack( void )
{
	StopSound( "Weapon_Gluon.Run" );
	
	if ( m_fireState != FIRE_OFF )
	{
		 EmitSound( "Weapon_Gluon.Off" );
	}

	SetWeaponIdleTime( gpGlobals->curtime + 2.0 );
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;

	m_fireState = FIRE_OFF;

	DestroyEffect();
}

bool CWeaponEgon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
    EndAttack();

	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponEgon::WeaponIdle( void )
{
	if ( !HasWeaponIdleTimeElapsed() )
		return;

	if ( m_fireState != FIRE_OFF )
		 EndAttack();
	
	int iAnim;

	float flRand = random->RandomFloat( 0,1 );
	if ( flRand <= 0.5 )
	{
		iAnim = ACT_VM_IDLE;
	}
	else 
	{
		iAnim = ACT_VM_FIDGET;
	}

	SendWeaponAnim( iAnim );
}
