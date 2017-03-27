//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Crossbow
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
#include "IEffects.h"


#define BOLT_MODEL			"models/crossbow_bolt.mdl"

#define BOLT_AIR_VELOCITY	2000
#define BOLT_WATER_VELOCITY	1000


extern ConVar sk_plr_dmg_xbow_bolt_plr;
extern ConVar sk_plr_dmg_xbow_bolt_npc;


//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CCrossbowBolt : public CBaseCombatCharacter
{
	DECLARE_CLASS( CCrossbowBolt, CBaseCombatCharacter );

public:
	CCrossbowBolt() { };

	Class_T Classify( void ) { return CLASS_NONE; }

public:
	void Spawn( void );
	void Precache( void );
	void BubbleThink( void );
	void BoltTouch( CBaseEntity *pOther );

	static CCrossbowBolt *BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, CBasePlayer *pentOwner = NULL );

	DECLARE_DATADESC();
};
LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt );

BEGIN_DATADESC( CCrossbowBolt )
	// Function Pointers
	DEFINE_FUNCTION( CCrossbowBolt, BubbleThink ),
	DEFINE_FUNCTION( CCrossbowBolt, BoltTouch ),
END_DATADESC()

CCrossbowBolt *CCrossbowBolt::BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, CBasePlayer *pentOwner )
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = (CCrossbowBolt *)CreateEntityByName( "crossbow_bolt" );
	UTIL_SetOrigin( pBolt, vecOrigin );
	pBolt->SetAbsAngles( angAngles );
	pBolt->Spawn();
	pBolt->SetOwnerEntity( pentOwner );

	return pBolt;
}

void CCrossbowBolt::Spawn( )
{
	Precache( );

	SetModel( BOLT_MODEL );
	UTIL_SetSize( this, Vector(0, 0, 0), Vector(0, 0, 0) );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetGravity( 0.05 );

	SetTouch( &CCrossbowBolt::BoltTouch );

	SetThink( &CCrossbowBolt::BubbleThink );
	SetNextThink( gpGlobals->curtime + 0.2 );
}


void CCrossbowBolt::Precache( )
{
	engine->PrecacheModel( BOLT_MODEL );
}

void TE_StickyBolt( IRecipientFilter& filter, float delay,	int iIndex, Vector vecDirection, const Vector * origin );


void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	SetTouch( NULL );
	SetThink( NULL );

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		trace_t	tr = BaseClass::GetTouchTrace( );
		Vector	vecNormalizedVel = GetAbsVelocity();

		ClearMultiDamage( );
		VectorNormalize( vecNormalizedVel );

		if ( pOther->IsPlayer() )
		{
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), sk_plr_dmg_xbow_bolt_plr.GetFloat(), DMG_NEVERGIB );
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}
		else
		{
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), sk_plr_dmg_xbow_bolt_npc.GetFloat(), DMG_BULLET | DMG_NEVERGIB );
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}

		ApplyMultiDamage();

		SetAbsVelocity( Vector( 0, 0, 0 ) );

		// play body "thwack" sound
		EmitSound( "Weapon_Crossbow.BoltHitBody" );

		Vector vForward;

		AngleVectors( GetAbsAngles(), &vForward );

		UTIL_TraceLine( GetAbsOrigin(),	GetAbsOrigin() + vForward * MAX_COORD_RANGE,
		MASK_NPCSOLID_BRUSHONLY, pOther, COLLISION_GROUP_NONE, &tr );

		vForward = GetAbsOrigin() - tr.endpos;
		VectorNormalize ( vForward );

		CPASFilter filter( GetAbsOrigin() );

		TE_StickyBolt( filter, 0.05, entindex(), vForward, &tr.endpos );

		if ( !g_pGameRules->IsMultiplayer() )
		{
			UTIL_Remove( this );
		}
	}
	else
	{
		EmitSound( "Weapon_Crossbow.BoltHitWorld" );

		SetThink( &CCrossbowBolt::SUB_Remove );
		SetNextThink( gpGlobals->curtime );// this will get changed below if the bolt is allowed to stick in what it hit.

		if ( FClassnameIs( pOther, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
			VectorNormalize( vecDir );
			SetAbsOrigin( GetAbsOrigin() - vecDir * 12 );

			QAngle angAngles;
			VectorAngles( vecDir, angAngles );
			SetAbsAngles( angAngles );

			AddSolidFlags( FSOLID_NOT_SOLID );
			Relink();

			SetMoveType( MOVETYPE_FLY );
			SetAbsVelocity( Vector( 0, 0, 0 ) );
			SetLocalAngularVelocity( QAngle( 0, 0, 0 ) );

			QAngle angNew = GetAbsAngles();
			angNew.z = random->RandomFloat( 0, 360 );
			SetAbsAngles( angNew );

			Vector vForward;
			trace_t tr;

			AngleVectors( GetAbsAngles(), &vForward );

			UTIL_TraceLine( GetAbsOrigin(),	GetAbsOrigin() + vForward * MAX_COORD_RANGE,
			MASK_NPCSOLID_BRUSHONLY, pOther, COLLISION_GROUP_NONE, &tr );

			vForward = GetAbsOrigin() - tr.endpos;
			VectorNormalize ( vForward );

			CPASFilter filter( GetAbsOrigin() );

			TE_StickyBolt( filter, 0.05, entindex(), vForward, &tr.endpos );
		}

		if (  UTIL_PointContents( GetAbsOrigin() ) != CONTENTS_WATER)
		{
			g_pEffects->Sparks( GetAbsOrigin() );
		}
	}

	if ( g_pGameRules->IsMultiplayer() )
	{
//		SetThink( &CCrossbowBolt::ExplodeThink );
//		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

void CCrossbowBolt::BubbleThink( void )
{
	QAngle angNewAngles;

	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );

	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( GetWaterLevel()  == 0 )
		return;

	UTIL_BubbleTrail( GetAbsOrigin() - GetAbsVelocity() * 0.1, GetAbsOrigin(), 1 );
}


//-----------------------------------------------------------------------------
// CWeaponCrossbow
//-----------------------------------------------------------------------------


class CWeaponCrossbow : public CBaseHL1CombatWeapon
{
	DECLARE_CLASS( CWeaponCrossbow, CBaseHL1CombatWeapon );
public:

	CWeaponCrossbow( void );

	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	bool	Reload( void );
	void	WeaponIdle( void );
	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	void	FireBolt( void );
	void	ToggleZoom( void );

private:
	bool	m_fInZoom;
};

LINK_ENTITY_TO_CLASS( weapon_crossbow, CWeaponCrossbow );

PRECACHE_WEAPON_REGISTER( weapon_crossbow );

IMPLEMENT_SERVERCLASS_ST( CWeaponCrossbow, DT_WeaponCrossbow )
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponCrossbow )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponCrossbow::CWeaponCrossbow( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= true;
	m_fInZoom			= false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Precache( void )
{
	UTIL_PrecacheOther( "crossbow_bolt" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponCrossbow::PrimaryAttack( void )
{
	if ( m_fInZoom && g_pGameRules->IsMultiplayer() )
	{
//		FireSniperBolt();
		FireBolt();
	}
	else
	{
		FireBolt();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SecondaryAttack( void )
{
	ToggleZoom();
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0;
}


void CWeaponCrossbow::FireBolt( void )
{
	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	Vector vecAiming	= pOwner->GetAutoaimVector( AUTOAIM_2DEGREES );	
	Vector vecSrc		= pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles( vecAiming, angAiming );

	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate( vecSrc, angAiming, pOwner );

	if ( pOwner->GetWaterLevel() == 3 )
	{
		pBolt->SetAbsVelocity( vecAiming * BOLT_WATER_VELOCITY );
	}
	else
	{
		pBolt->SetAbsVelocity( vecAiming * BOLT_AIR_VELOCITY );
	}

	pBolt->SetLocalAngularVelocity( QAngle( 0, 0, 10 ) );

	m_iClip1--;

	pOwner->ViewPunch( QAngle( -2, 0, 0 ) );

	WeaponSound( SINGLE );
	WeaponSound( RELOAD );
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 200, 0.2 );

	if ( m_iClip1 > 0 )
	{
		SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	}
	else if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0 )
	{
		SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	}

	if ( !m_iClip1 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pOwner->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	m_flNextPrimaryAttack	= gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack	= gpGlobals->curtime + 0.75;

	if ( m_iClip1 > 0 )
	{
		SetWeaponIdleTime( gpGlobals->curtime + 5.0 );
	}
	else
	{
		SetWeaponIdleTime( gpGlobals->curtime + 0.75 );
	}
}


bool CWeaponCrossbow::Reload( void )
{
	bool fRet;

	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		if ( m_fInZoom )
		{
			ToggleZoom();
		}

		WeaponSound( RELOAD );
	}

	return fRet;
}


void CWeaponCrossbow::WeaponIdle( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );
	}

	if ( !HasWeaponIdleTimeElapsed() )
		return;

	int iAnim;
	float flRand = random->RandomFloat( 0, 1 );
	if ( flRand <= 0.75 )
	{
		if ( m_iClip1 <= 0 )
			iAnim = ACT_CROSSBOW_IDLE_UNLOADED;
		else
			iAnim = ACT_VM_IDLE;
	}
	else
	{
		if ( m_iClip1 <= 0 )
			iAnim = ACT_CROSSBOW_FIDGET_UNLOADED;
		else
			iAnim = ACT_VM_FIDGET;
	}

	SendWeaponAnim( iAnim );
}


bool CWeaponCrossbow::Deploy( void )
{
	if ( m_iClip1 <= 0 )
	{
		return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_CROSSBOW_DRAW_UNLOADED, (char*)GetAnimPrefix() );
	}

	return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_VM_DRAW, (char*)GetAnimPrefix() );
}


bool CWeaponCrossbow::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_fInZoom )
	{
		ToggleZoom();
	}

	return BaseClass::Holster( pSwitchingTo );
}


void CWeaponCrossbow::ToggleZoom( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	if ( m_fInZoom )
	{
		pPlayer->m_Local.m_iFOV = 0;
		m_fInZoom = false;
	}
	else
	{
		pPlayer->m_Local.m_iFOV = 20;
		m_fInZoom = true;
	}
}
