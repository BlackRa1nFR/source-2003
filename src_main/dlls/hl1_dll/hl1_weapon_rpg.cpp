//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		RPG
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hl1_basecombatweapon_shared.h"
#include "basecombatcharacter.h"
#include "movie_explosion.h"
#include "soundent.h"
#include "player.h"
#include "rope.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "explode.h"
#include "util.h"
#include "in_buttons.h"
#include "shake.h"
#include "te_effect_dispatch.h"
#include "hl1_weapon_rpg.h"


extern ConVar sk_plr_dmg_rpg;


void TE_BeamFollow( IRecipientFilter& filter, float delay,
	int iEntIndex, int modelIndex, int haloIndex, float life, float width, float endWidth, 
	float fadeLength,float r, float g, float b, float a );


//=============================================================================
// RPG Rocket
//=============================================================================


BEGIN_DATADESC( CRpgRocket )
	DEFINE_FIELD( CRpgRocket, m_hOwner,			FIELD_EHANDLE ),
	DEFINE_FIELD( CRpgRocket, m_vecAbsVelocity,	FIELD_VECTOR ),
	DEFINE_FIELD( CRpgRocket, m_flIgniteTime,	FIELD_TIME ),
	
	// Function Pointers
	DEFINE_FUNCTION( CRpgRocket, RocketTouch ),
	DEFINE_FUNCTION( CRpgRocket, IgniteThink ),
	DEFINE_FUNCTION( CRpgRocket, SeekThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( rpg_rocket, CRpgRocket );

IMPLEMENT_SERVERCLASS_ST(CRpgRocket, DT_RpgRocket)
END_SEND_TABLE()

CRpgRocket::CRpgRocket()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CRpgRocket::Precache( void )
{
	engine->PrecacheModel( "models/rpgrocket.mdl" );
	engine->PrecacheModel( "sprites/animglow01.vmt" );
	m_iTrail = engine->PrecacheModel("sprites/smoke.vmt");
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CRpgRocket::Spawn( void )
{
	Precache();

	SetSolid( SOLID_BBOX );
	SetModel("models/rpgrocket.mdl");
	UTIL_SetSize( this, -Vector(0,0,0), Vector(0,0,0) );

	SetTouch( RocketTouch );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetThink( IgniteThink );
	
	SetNextThink( gpGlobals->curtime + 0.4f );

	QAngle angAngs;
	Vector vecFwd;

	angAngs = GetAbsAngles();
	angAngs.x -= 30;
	AngleVectors( angAngs, &vecFwd );
	SetAbsVelocity( vecFwd * 250 );

	SetGravity( 0.5 );

	m_flDamage	= sk_plr_dmg_rpg.GetFloat();
	m_DmgRadius	= m_flDamage * 2.5;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CRpgRocket::RocketTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	if ( m_hOwner != NULL )
	{
		m_hOwner->NotifyRocketDied();
	}

	StopSound( "Weapon_RPG.RocketIgnite" );
	ExplodeTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRpgRocket::IgniteThink( void )
{
	SetMoveType( MOVETYPE_FLY );

	m_fEffects |= EF_DIMLIGHT;

	EmitSound( "Weapon_RPG.RocketIgnite" );

	SetThink( SeekThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	CBroadcastRecipientFilter filter;
	TE_BeamFollow( filter, 0.0,
		entindex(),
		m_iTrail,
		0,
		4,
		5,
		5,
		0,
		224,
		224,
		255,
		255 );

	m_flIgniteTime = gpGlobals->curtime;
}

#define	RPG_HOMING_SPEED	0.125f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRpgRocket::SeekThink( void )
{
	CBaseEntity *pOther = NULL;
	Vector vecTarget;
	Vector vecFwd;
	Vector vecDir;
	float flDist, flMax, flDot;
	trace_t tr;

	AngleVectors( GetAbsAngles(), &vecFwd );

	vecTarget = vecFwd;
	flMax = 4096;
	
	// Examine all entities within a reasonable radius
	while ( (pOther = gEntList.FindEntityByClassname( pOther, "laser_spot" ) ) != NULL)
	{
		CLaserDot *pDot = static_cast<CLaserDot*>(pOther);

		if ( pDot->IsActive() )
		{
			UTIL_TraceLine( GetAbsOrigin(), pDot->GetAbsOrigin(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction >= 0.90 )
			{
				vecDir = pDot->GetAbsOrigin() - GetAbsOrigin();
				flDist = VectorLength( vecDir );
				VectorNormalize( vecDir );
				flDot = DotProduct( vecFwd, vecDir );
				if ( (flDot > 0) && (flDist * (1 - flDot) < flMax) )
				{
					flMax = flDist * (1 - flDot);
					vecTarget = vecDir;
				}
			}
		}
	}

	QAngle vecAng;
	VectorAngles( vecTarget, vecAng );
	SetAbsAngles( vecAng );

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = GetAbsVelocity().Length();
	if ( gpGlobals->curtime - m_flIgniteTime < 1.0 )
	{
		SetAbsVelocity( GetAbsVelocity() * 0.2 + vecTarget * (flSpeed * 0.8 + 400) );
		if ( GetWaterLevel() == 3 )
		{
			// go slow underwater
			if ( GetAbsVelocity().Length() > 300 )
			{
				Vector vecVel = GetAbsVelocity();
				VectorNormalize( vecVel );
				SetAbsVelocity( vecVel * 300 );
			}

			UTIL_BubbleTrail( GetAbsOrigin() - GetAbsVelocity() * 0.1, GetAbsOrigin(), 4 );
		} 
		else 
		{
			if ( GetAbsVelocity().Length() > 2000 )
			{
				Vector vecVel = GetAbsVelocity();
				VectorNormalize( vecVel );
				SetAbsVelocity( vecVel * 2000 );
			}
		}
	}
	else
	{
		if ( m_fEffects & EF_DIMLIGHT )
		{
			m_fEffects = 0;
			StopSound( "Weapon_RPG.RocketIgnite" );
		}

		SetAbsVelocity( GetAbsVelocity() * 0.2 + vecTarget * flSpeed * 0.798 );

		if ( GetWaterLevel() == 0 && GetAbsVelocity().Length() < 1500 )
		{
			Detonate();
		}
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : &vecOrigin - 
//			&vecAngles - 
//			NULL - 
//
// Output : CRpgRocket
//-----------------------------------------------------------------------------
CRpgRocket *CRpgRocket::Create( const Vector &vecOrigin, const QAngle &angAngles, CBasePlayer *pentOwner )
{
	CRpgRocket *pRocket = (CRpgRocket *)CreateEntityByName( "rpg_rocket" );
	UTIL_SetOrigin( pRocket, vecOrigin );
	pRocket->SetAbsAngles( angAngles );
	pRocket->Spawn();
	pRocket->SetOwnerEntity( pentOwner );

	return pRocket;
}


//=============================================================================
// Laser Dot
//=============================================================================

LINK_ENTITY_TO_CLASS( laser_spot, CLaserDot );

CLaserDot::CLaserDot( void )
{
	m_hLaserSprite	= NULL;
}

void CLaserDot::Precache( void )
{
	engine->PrecacheModel( "sprites/laserdot.vmt" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
// Output : CLaserDot
//-----------------------------------------------------------------------------
CLaserDot *CLaserDot::Create( const Vector &origin, CBaseEntity *pOwner )
{
	CLaserDot *pNewLaserDot = (CLaserDot *) CBaseEntity::Create( "laser_spot", origin, QAngle(0,0,0) );

	if ( pNewLaserDot == NULL )
		return NULL;

	pNewLaserDot->SetMoveType( MOVETYPE_NONE );
	pNewLaserDot->AddSolidFlags( FSOLID_NOT_SOLID );

	//Create the graphic
	pNewLaserDot->m_hLaserSprite = CSprite::SpriteCreate( "sprites/laserdot.vmt", origin, false );

	if ( pNewLaserDot->m_hLaserSprite == NULL )
	{
		assert(0);
		return pNewLaserDot;
	}

	pNewLaserDot->m_hLaserSprite->SetAttachment( pNewLaserDot, 0 );
	pNewLaserDot->m_hLaserSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
	pNewLaserDot->m_hLaserSprite->SetScale( 1.0 );
	pNewLaserDot->m_hLaserSprite->AddSpawnFlags( SF_SPRITE_TEMPORARY );	// Don't save it
	pNewLaserDot->SetOwnerEntity( pOwner );

	return pNewLaserDot;
}

void CLaserDot::SetLaserPosition( const Vector &origin, const Vector &normal )
{
	SetAbsOrigin( origin );
//	UTIL_Relink( this );

	if ( m_hLaserSprite )
	{
		m_hLaserSprite->SetAbsOrigin( origin );
		UTIL_Relink( m_hLaserSprite );
	}

	m_vecSurfaceNormal = normal;
}

Vector CLaserDot::GetChasePosition()
{
	return GetAbsOrigin() - m_vecSurfaceNormal * 10;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLaserDot::TurnOn( void )
{
	if ( m_hLaserSprite )
	{
		m_hLaserSprite->TurnOn();
	}

	m_fEffects &= ~EF_NODRAW;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLaserDot::TurnOff( void )
{
	if ( m_hLaserSprite )
	{
		m_hLaserSprite->TurnOff();
	}

	m_fEffects |= EF_NODRAW;
}


bool CLaserDot::IsActive( void )
{
	if ( m_hLaserSprite && ( FBitSet( m_hLaserSprite->m_fEffects, EF_NODRAW ) == false ) )
	{
		return true;
	}

	return false;
}


//=============================================================================
// RPG Weapon
//=============================================================================

LINK_ENTITY_TO_CLASS( weapon_rpg, CWeaponRPG );

PRECACHE_WEAPON_REGISTER( weapon_rpg );

IMPLEMENT_SERVERCLASS_ST( CWeaponRPG, DT_WeaponRPG )
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponRPG )
	DEFINE_FIELD( CWeaponRPG,	m_bIntialStateUpdate,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CWeaponRPG,	m_bGuiding,					FIELD_BOOLEAN ),
	DEFINE_FIELD( CWeaponRPG,	m_hLaserDot,				FIELD_EHANDLE ),
	DEFINE_FIELD( CWeaponRPG,	m_hMissile,					FIELD_EHANDLE ),
	DEFINE_FIELD( CWeaponRPG,	m_bLaserDotSuspended,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CWeaponRPG,	m_flLaserDotReviveTime,		FIELD_TIME ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponRPG::CWeaponRPG( void )
{
	m_bReloadsSingly		= false;
	m_bFiresUnderwater		= true;
	m_bGuiding				= true;
	m_bIntialStateUpdate	= false;
	m_bLaserDotSuspended	= false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponRPG::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	//If we're pulling the weapon out for the first time, wait to draw the laser
	if ( m_bIntialStateUpdate ) 
	{
		if ( GetActivity() != ACT_VM_DRAW )
		{
			if ( IsGuiding() && !m_bLaserDotSuspended )
			{
				if ( m_hLaserDot != NULL )
				{
					m_hLaserDot->TurnOn();
				}
			}

			m_bIntialStateUpdate = false;
		}
		else
		{
			return;
		}
	}

	//Player has toggled guidance state
	if ( pPlayer->m_afButtonPressed & IN_ATTACK2 )
	{
		if ( IsGuiding() )
		{
			StopGuiding();
		}
		else
		{
			StartGuiding();
		}
	}

	//Move the laser
	UpdateSpot();
}


int CWeaponRPG::GetDefaultClip1( void ) const
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		// more default ammo in multiplay. 
		return BaseClass::GetDefaultClip1() * 2;
	}
	else
	{
		return BaseClass::GetDefaultClip1();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponRPG::Precache( void )
{
	UTIL_PrecacheOther( "laser_spot" );
	UTIL_PrecacheOther( "rpg_rocket" );

	BaseClass::Precache();
}


bool CWeaponRPG::Deploy( void )
{
	m_bIntialStateUpdate = true;
	m_bLaserDotSuspended = false;
	CreateLaserPointer();
	if ( m_hLaserDot != NULL )
	{
		m_hLaserDot->TurnOff();
	}

	if ( m_iClip1 <= 0 )
	{
		return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_RPG_DRAW_UNLOADED, (char*)GetAnimPrefix() );
	}

	return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_VM_DRAW, (char*)GetAnimPrefix() );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponRPG::PrimaryAttack( void )
{
	// Can't have an active missile out
	if ( m_hMissile != NULL )
		return;

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
		}
	}

	Vector vecOrigin;
	Vector vecForward;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	WeaponSound( SINGLE );
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 400, 0.2 );
	pOwner->m_fEffects |= EF_MUZZLEFLASH;

	// Register a muzzleflash for the AI
	pOwner->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	Vector	vForward, vRight, vUp;

	pOwner->EyeVectors( &vForward, &vRight, &vUp );

	Vector	muzzlePoint = pOwner->Weapon_ShootPosition() + vForward * 16.0f + vRight * 8.0f + vUp * -8.0f;

	QAngle vecAngles;
	VectorAngles( vForward, vecAngles );
	m_hMissile = CRpgRocket::Create( muzzlePoint, vecAngles, pOwner );
	m_hMissile->m_hOwner = this;
	m_hMissile->SetAbsVelocity( m_hMissile->GetAbsVelocity() + vForward * DotProduct( pOwner->GetAbsVelocity(), vForward ) );

	pOwner->ViewPunch( QAngle( -5, 0, 0 ) );

	m_iClip1--; 
				
	m_flNextPrimaryAttack = gpGlobals->curtime + 1.5;
	SetWeaponIdleTime( 1.5 );

	UpdateSpot();
}


void CWeaponRPG::WeaponIdle( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	
	if ( pOwner == NULL )
		return;

	if ( !HasWeaponIdleTimeElapsed() )
		return;

	int iAnim;
	float flRand = random->RandomFloat( 0, 1 );
	if ( flRand <= 0.75 || IsGuiding() )
	{
		if ( m_iClip1 <= 0 )
			iAnim = ACT_RPG_IDLE_UNLOADED;
		else
			iAnim = ACT_VM_IDLE;
	}
	else
	{
		if ( m_iClip1 <= 0 )
			iAnim = ACT_RPG_FIDGET_UNLOADED;
		else
			iAnim = ACT_VM_FIDGET;
	}

	SendWeaponAnim( iAnim );
}


void CWeaponRPG::NotifyRocketDied( void )
{
	m_hMissile = NULL;

	Reload();
}


bool CWeaponRPG::Reload( void )
{
	bool fResult = false;

	CBaseCombatCharacter *pOwner = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if ( m_iClip1 > 0 )
		return false;

	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return false;

	// because the RPG waits to autoreload when no missiles are active while  the LTD is on, the
	// weapons code is constantly calling into this function, but is often denied because 
	// a) missiles are in flight, but the LTD is on
	// or
	// b) player is totally out of ammo and has nothing to switch to, and should be allowed to
	//    shine the designator around
	//
	// Set the next attack time into the future so that WeaponIdle will get called more often
	// than reload, allowing the RPG LTD to be updated
	
	if ( ( m_hMissile != NULL ) && IsGuiding() )
	{
		// no reloading when there are active missiles tracking the designator.
		// ward off future autoreload attempts by setting next attack time into the future for a bit. 
		return false;
	}

	if ( m_hLaserDot != NULL )
	{
		m_hLaserDot->TurnOff();
	}
	m_bLaserDotSuspended = true;
	m_flLaserDotReviveTime = gpGlobals->curtime + 2.1;
	m_flNextSecondaryAttack = gpGlobals->curtime + 2.1;

	fResult = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	
	if ( fResult )
	{
		SetWeaponIdleTime( gpGlobals->curtime + random->RandomFloat( 10, 15 ) );
	}

	return fResult;
}


bool CWeaponRPG::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	// can't put away while guiding a missile.
	if ( IsGuiding() && ( m_hMissile != NULL ) )
		return false;

	if ( m_hLaserDot != NULL )
	{
		m_hLaserDot->TurnOff();
	}

	m_bLaserDotSuspended = false;

	return BaseClass::Holster( pSwitchingTo );
}


void CWeaponRPG::UpdateSpot( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	CreateLaserPointer();

	if ( m_hLaserDot == NULL )
		return;

	if ( IsGuiding() && m_bLaserDotSuspended && ( m_flLaserDotReviveTime <= gpGlobals->curtime ) )
	{
		m_hLaserDot->TurnOn();
		m_bLaserDotSuspended = false;
	}

	//Move the laser dot, if active
	trace_t	tr;
	Vector	muzzlePos = pPlayer->Weapon_ShootPosition();
	
	Vector	forward;
	AngleVectors( pPlayer->EyeAngles() + pPlayer->m_Local.m_vecPunchAngle, &forward );

	Vector	endPos = muzzlePos + ( forward * MAX_TRACE_LENGTH );

	// Trace out for the endpoint
	UTIL_TraceLine( muzzlePos, endPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	// Move the laser sprite
	Vector	laserPos = tr.endpos + ( tr.plane.normal * 2.0f );
	m_hLaserDot->SetLaserPosition( laserPos, tr.plane.normal );
}


void CWeaponRPG::CreateLaserPointer( void )
{
	if ( m_hLaserDot != NULL )
		return;

	m_hLaserDot = CLaserDot::Create( GetAbsOrigin(), this );
	if ( !IsGuiding() )
	{
		if ( m_hLaserDot )
		{
			m_hLaserDot->TurnOff();
		}
	}
}


bool CWeaponRPG::IsGuiding( void )
{
	return m_bGuiding;
}


void CWeaponRPG::StartGuiding( void )
{
	m_bGuiding = true;

	if ( m_hLaserDot != NULL )
	{
		m_hLaserDot->TurnOn();
	}

	UpdateSpot();
}

void CWeaponRPG::StopGuiding( void )
{
	m_bGuiding = false;

	if ( m_hLaserDot != NULL )
	{
		m_hLaserDot->TurnOff();
	}
}
