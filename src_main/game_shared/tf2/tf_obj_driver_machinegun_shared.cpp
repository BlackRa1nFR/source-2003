//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Vehicle mounted machinegun that the driver controls
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "basetfplayer_shared.h"
#include "baseobject_shared.h"
#include "tf_obj_driver_machinegun_shared.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "tf_gamerules.h"

#if defined( CLIENT_DLL )
#include "hud_ammo.h"
#else
#endif

// ------------------------------------------------------------------------ //
#define DRIVER_MACHINEGUN_MINS				Vector(-10, -10, 0)
#define DRIVER_MACHINEGUN_MAXS				Vector( 10,  10, 10)
#define DRIVER_MACHINEGUN_MODEL				"models/objects/obj_manned_plasmagun.mdl"
#define DRIVER_MACHINEGUN_SOUND1			"weapons/carbine/carb_fire1.wav"
#define DRIVER_MACHINEGUN_SOUND2			"weapons/carbine/carb_fire2.wav"

IMPLEMENT_NETWORKCLASS_ALIASED( ObjectDriverMachinegun, DT_ObjectDriverMachinegun )

BEGIN_NETWORK_TABLE( CObjectDriverMachinegun, DT_ObjectDriverMachinegun )
#if !defined( CLIENT_DLL )
	SendPropInt( SENDINFO(m_nAmmoCount), 7, SPROP_UNSIGNED ),
#else
	RecvPropInt( RECVINFO(m_nAmmoCount) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS(obj_driver_machinegun, CObjectDriverMachinegun);
PRECACHE_REGISTER(obj_driver_machinegun);

BEGIN_PREDICTION_DATA( CObjectDriverMachinegun )
	DEFINE_PRED_FIELD( CObjectDriverMachinegun, m_nAmmoCount, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

ConVar	obj_driver_machinegun_health( "obj_driver_machinegun_health","100", FCVAR_REPLICATED, "Driver's mounted machinegun health" );
ConVar	obj_driver_machinegun_range( "obj_driver_machinegun_range","1048", FCVAR_REPLICATED, "Driver's mounted machinegun range" );
ConVar	obj_driver_machinegun_damage( "obj_driver_machinegun_damage","10", FCVAR_REPLICATED, "Driver's mounted machinegun damage" );
ConVar	obj_driver_machinegun_max_ammo( "obj_driver_machinegun_max_ammo","30", FCVAR_REPLICATED, "Driver's mounted machinegun ammo" );
ConVar	obj_driver_machinegun_ammo_recharge_rate( "obj_driver_machinegun_ammo_recharge_rate","0.5", FCVAR_REPLICATED, "Driver's mounted machinegun ammo recharge rate" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObjectDriverMachinegun::CObjectDriverMachinegun()
{
	SetPredictionEligible( true );

#if !defined( CLIENT_DLL )
	m_iHealth = obj_driver_machinegun_health.GetInt();
#endif

	m_flNextAttack = 0;
	m_nMaxAmmoCount = m_nAmmoCount = obj_driver_machinegun_max_ammo.GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDriverMachinegun::Spawn()
{
	Precache();
	SetModel( DRIVER_MACHINEGUN_MODEL );

#if !defined( CLIENT_DLL )
	SetBodygroup( 1, true );

	UTIL_SetSize(this, DRIVER_MACHINEGUN_MINS, DRIVER_MACHINEGUN_MAXS);
	m_takedamage = DAMAGE_YES;

	SetType( OBJ_DRIVER_MACHINEGUN );
	m_fObjectFlags |= OF_SUPPRESS_NOTIFY_UNDER_ATTACK | OF_DONT_AUTO_REPAIR;

	SetThink( RechargeThink );
#endif

 	m_nBarrelAttachment = LookupAttachment( "barrel" );
	m_nAmmoType = GetAmmoDef()->Index( "Bullets" );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDriverMachinegun::Precache()
{
	PrecacheModel( DRIVER_MACHINEGUN_MODEL );
	PrecacheSound( DRIVER_MACHINEGUN_SOUND1 );
	PrecacheSound( DRIVER_MACHINEGUN_SOUND2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CObjectDriverMachinegun::CanFireNow( void )
{
	if ( !m_nAmmoCount )
		return false;

	return ( m_flNextAttack < gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDriverMachinegun::Fire( CBaseTFPlayer *pDriver )
{
	// We have to flush the bone cache because it's possible that only the bone controllers
	// have changed since the bonecache was generated, and bone controllers aren't checked.
	InvalidateBoneCache();

	QAngle vecAng;
	Vector vecSrc, vecAim;
	GetAttachment( m_nBarrelAttachment, vecSrc, vecAng );
	AngleVectors( vecAng, &vecAim, 0, 0 );

	static Vector spread = VECTOR_CONE_5DEGREES;
	TFGameRules()->FireBullets( CTakeDamageInfo( this, pDriver, obj_driver_machinegun_damage.GetFloat(), DMG_BULLET ), 
			1, vecSrc, vecAim, spread, obj_driver_machinegun_range.GetFloat(), m_nAmmoType, 4, entindex(), m_nBarrelAttachment );

#if !defined (CLIENT_DLL)
	SetActivity( ACT_VM_PRIMARYATTACK );
#else
	int sequence = SelectWeightedSequence( ACT_VM_PRIMARYATTACK ); 
	if ( sequence != ACTIVITY_NOT_AVAILABLE )
	{
		ResetSequence( sequence );
		m_flCycle = 0;
		m_bClientSideFrameReset = !m_bClientSideFrameReset;
	}
#endif
	m_fEffects |= EF_MUZZLEFLASH;

	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();
	if ( RandomFloat(0,1) > 0.5 )
	{
		EmitSound( filter, entindex(), CHAN_WEAPON, DRIVER_MACHINEGUN_SOUND1, 0.75, ATTN_NORM);
	}
	else
	{
		EmitSound( filter, entindex(), CHAN_WEAPON, DRIVER_MACHINEGUN_SOUND2, 0.75, ATTN_NORM);
	}

	m_nAmmoCount -= 1;

#if !defined (CLIENT_DLL)
	SetNextThink( gpGlobals->curtime + obj_driver_machinegun_ammo_recharge_rate.GetFloat() );
#endif

	// If I'm EMPed, slow the firing rate down
	m_flNextAttack = gpGlobals->curtime + ( HasPowerup(POWERUP_EMP) ? 0.3f : 0.15f );
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectDriverMachinegun::SetTargetAngles( const QAngle &vecAngles )
{
	BaseClass::SetTargetAngles( vecAngles );

#if !defined( CLIENT_DLL )
	SetBoneController( 0, vecAngles[YAW] );
	SetBoneController( 1, vecAngles[PITCH] );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CObjectDriverMachinegun::GetFireOrigin( void )
{
	Vector vecOrigin;
	QAngle dummy;
	GetAttachment( m_nBarrelAttachment, vecOrigin, dummy );
	return vecOrigin;
}

//-----------------------------------------------------------------------------
// Purpose: Rcharge my ammo count
//-----------------------------------------------------------------------------
void CObjectDriverMachinegun::RechargeThink( void )
{
#if !defined( CLIENT_DLL )
	SetNextThink( gpGlobals->curtime + obj_driver_machinegun_ammo_recharge_rate.GetFloat() );

	// I can't do anything if I'm not active
	if ( !ShouldBeActive() )
		return;

	if (m_nAmmoCount < m_nMaxAmmoCount)
	{
		m_nAmmoCount += 1;
	}
#endif
}

// Client code only
#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CObjectDriverMachinegun::GetBoneControllers( float controllers[MAXSTUDIOBONECTRLS] )
{
	BaseClass::GetBoneControllers( controllers );

	controllers[0] = anglemod( m_vecGunAngles[YAW] ) / 360.0;
	controllers[1] = anglemod( m_vecGunAngles[PITCH] ) / 360.0;
}

//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------
void CObjectDriverMachinegun::DrawHudElements( void )
{
	GetHudAmmo()->SetPrimaryAmmo( m_nAmmoType, m_nAmmoCount );
	GetHudAmmo()->SetSecondaryAmmo( -1, -1 );
}
#endif
