//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "vehicle_base.h"
#include "ammodef.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "grenade_ar2.h"
#include "in_buttons.h"
#include "globalstate.h"
#include "basegrenade_shared.h"
#include "soundent.h"
#include "ai_basenpc.h"
#include "ndebugoverlay.h"

#define	COVER_TIME		( 15.0 )
#define APC_MAX_HEALTH	5000.0

// Spawn Flags
#define SF_APC_DRONE	1

#define CANNON_MAX_UP_PITCH		30
#define CANNON_MAX_DOWN_PITCH	10
#define CANNON_MAX_LEFT_YAW		30
#define CANNON_MAX_RIGHT_YAW	30

#define CANNON_BURST_SIZE		8
#define GRENADE_SALVO_SIZE		3

//-----------------------------------------------------------------------------
// Purpose: Four wheel physics vehicle server vehicle with weaponry
//-----------------------------------------------------------------------------
class CAPCFourWheelServerVehicle : public CFourWheelServerVehicle
{
	typedef CFourWheelServerVehicle BaseClass;
// IServerVehicle
public:
	bool		NPC_HasPrimaryWeapon( void ) { return true; }
	void		NPC_AimPrimaryWeapon( Vector vecTarget );
	bool		NPC_HasSecondaryWeapon( void ) { return true; }
	void		NPC_AimSecondaryWeapon( Vector vecTarget );

	// Weaponry
	void		Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange );
	void		Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange );
	float		Weapon_PrimaryCanFireAt( void );		// Return the time at which this vehicle's primary weapon can fire again
	float		Weapon_SecondaryCanFireAt( void );		// Return the time at which this vehicle's secondary weapon can fire again
};

//-----------------------------------------------------------------------------
// A driveable vehicle with a gun that shoots wherever the driver looks.
//-----------------------------------------------------------------------------
class CPropAPC : public CPropVehicleDriveable
{
	DECLARE_CLASS( CPropAPC, CPropVehicleDriveable );
public:
	// CBaseEntity
	virtual void Precache( void );
	void	Think( void );
	void	Spawn(void);
	void	ItemPostFrame( CBasePlayer *player );

	// CPropVehicle
	virtual void	CreateServerVehicle( void );
	virtual void	DriveVehicle( float flFrameTime, int iButtons, int iButtonsDown, int iButtonsReleased );
	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual Class_T	ClassifyPassenger( CBasePlayer *pPassenger, Class_T defaultClassification );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual float	DamageModifier ( CTakeDamageInfo &info );
	virtual bool	CanControlVehicle( void );
	virtual void	EnterVehicle( CBasePlayer *pPlayer );

	// Weaponry
	const Vector	&GetPrimaryGunOrigin( void ) { return m_vecGunOrigin; }
	void			AimPrimaryWeapon( Vector vecForward ) { m_vecGunAimForward = vecForward; }
	void			AimSecondaryWeaponAt( Vector vecTarget ) { m_vecGrenadeTarget = vecTarget; }
	float			PrimaryWeaponFireTime( void ) { return m_flCannonTime; }
	float			SecondaryWeaponFireTime( void ) { return m_flGrenadeTime; }

private:

	void	FireCannon( void );
	void	FireGrenade( void );
	void	UpdateHealth( void );

	float	m_flCannonTime;
	float	m_flGrenadeTime;
	float	m_flCoverTime;		// time your identity stays uncovered after firing from the apc
	float	m_flRegenTime;		// "health" regeneration timer
	float	m_flDangerSoundTime;

	// handbrake after the fact to keep vehicles from rolling
	float	m_flHandbrakeTime;
	bool	m_bInitialHandbrake;

	Vector	m_vecGunOrigin;
	Vector	m_vecGunAimForward;
	Vector	m_vecGrenadeTarget;
	bool	m_bLeftBarrel;
	bool	m_bCoverBlown;		// if you fire on someone, it will blow your cover
	int		m_APCHealth;
	int		m_iCannonBurstLeft;
	int		m_iGrenadeSalvoLeft;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( prop_vehicle_apc, CPropAPC );


BEGIN_DATADESC( CPropAPC )

	DEFINE_FIELD( CPropAPC, m_flCannonTime, FIELD_TIME ),
	DEFINE_FIELD( CPropAPC, m_flGrenadeTime, FIELD_TIME ),
	DEFINE_FIELD( CPropAPC, m_flCoverTime, FIELD_TIME ),
	DEFINE_FIELD( CPropAPC, m_flRegenTime, FIELD_TIME ),
	DEFINE_FIELD( CPropAPC, m_flDangerSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CPropAPC, m_flHandbrakeTime, FIELD_TIME ),
	DEFINE_FIELD( CPropAPC, m_bInitialHandbrake, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropAPC, m_vecGunOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CPropAPC, m_vecGunAimForward, FIELD_VECTOR ),
	DEFINE_FIELD( CPropAPC, m_vecGrenadeTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CPropAPC, m_bLeftBarrel, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropAPC, m_bCoverBlown, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropAPC, m_APCHealth, FIELD_INTEGER ),
	DEFINE_FIELD( CPropAPC, m_iCannonBurstLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CPropAPC, m_iGrenadeSalvoLeft, FIELD_INTEGER ),

END_DATADESC()

//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropAPC::Spawn( void )
{
	BaseClass::Spawn();
	m_APCHealth = APC_MAX_HEALTH;
	m_flCycle = 0;
	m_iCannonBurstLeft = CANNON_BURST_SIZE;
	m_iGrenadeSalvoLeft = GRENADE_SALVO_SIZE;

	m_flHandbrakeTime = gpGlobals->curtime + 0.1;
	m_bInitialHandbrake = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::CreateServerVehicle( void )
{
	// Create our armed server vehicle
	m_pServerVehicle = new CAPCFourWheelServerVehicle();
	m_pServerVehicle->SetVehicle( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMoveData - 
//-----------------------------------------------------------------------------
Class_T	CPropAPC::ClassifyPassenger( CBasePlayer *pPassenger, Class_T defaultClassification )
{ 
	// indicate we're a combine if the global says to be stealth and we have not been aggressive (shot)
	if ( ( GlobalEntity_GetState("player_stealth") == GLOBAL_ON) &&
		!m_bCoverBlown )
		return CLASS_COMBINE;	
	else
		return defaultClassification; 
}

//-----------------------------------------------------------------------------
// Purpose: check to see if player can use this vehicle or if it's been disabled
//-----------------------------------------------------------------------------
bool CPropAPC::CanControlVehicle( void )
{
	// Is this a drone or dead vehicle? don't allow control then
	return (m_spawnflags & SF_APC_DRONE || m_APCHealth != 0 );
//	if ( m_spawnflags & SF_APC_DRONE || m_APCHealth == 0 )
//		return false;
//	else
//		return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CPropAPC::DamageModifier ( CTakeDamageInfo &info ) 
{ 
	CTakeDamageInfo DmgInfo = info;
	// bullets, slashing and headbutts don't hurt us in the apc, neither do rockets
	if( (DmgInfo.GetDamageType() & DMG_BULLET) || (DmgInfo.GetDamageType() & DMG_SLASH) ||
		(DmgInfo.GetDamageType() & DMG_CLUB) || (DmgInfo.GetDamageType() & DMG_BLAST) )
	{
		return (0);
	}
	else
	{
		return 1.0; 
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropAPC::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo DmgInfo = info;

	if (DmgInfo.GetDamageType() & DMG_BLAST)
	{
		m_APCHealth -= DmgInfo.GetDamage();
		if ( m_APCHealth < 0 )
		{
			m_APCHealth = 0;
		}
	}
	if ( m_hPlayer != NULL )
	{
		m_hPlayer->TakeDamage( DmgInfo );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMoveData - 
//-----------------------------------------------------------------------------
void CPropAPC::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData )
{
	BaseClass::ProcessMovement( pPlayer, pMoveData );

	if ( m_flDangerSoundTime > gpGlobals->curtime )
		return;

	QAngle vehicleAngles = GetLocalAngles();
	Vector vecStart = GetAbsOrigin();
	Vector vecDir;

	GetVectors( &vecDir, NULL, NULL );

	// Make danger sounds ahead of the APC
	trace_t	tr;
	Vector	vecSpot, vecLeftDir, vecRightDir;

	// lay down sound path
	vecSpot = vecStart + vecDir * 600;
	CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.1, this );

	// put sounds a bit to left and right but slightly closer to APC to make a "cone" of sound 
	// in front of it
	QAngle leftAngles = vehicleAngles;
	leftAngles[YAW] += 20;
	VehicleAngleVectors( leftAngles, &vecLeftDir, NULL, NULL );
	vecSpot = vecStart + vecLeftDir * 400;
	UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.1, this );

	QAngle rightAngles = vehicleAngles;
	rightAngles[YAW] -= 20;
	VehicleAngleVectors( rightAngles, &vecRightDir, NULL, NULL );
	vecSpot = vecStart + vecRightDir * 400;
	UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, 0.1, this);

	m_flDangerSoundTime = gpGlobals->curtime + 0.3;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::Think( void )
{
	BaseClass::Think();

	// if health needs to be updated, then keep thinking, stop otherwise
	if ( m_APCHealth < APC_MAX_HEALTH )
	{
		UpdateHealth();
		SetNextThink( gpGlobals->curtime );
	}

	// set handbrake after physics sim settles down
	if ( gpGlobals->curtime < m_flHandbrakeTime )
	{
		SetNextThink( gpGlobals->curtime );
	}
	else if ( !m_bInitialHandbrake )	// after initial timer expires, set the handbrake
	{
		m_bInitialHandbrake = true;
		m_VehiclePhysics.SetHandbrake( true );
		m_VehiclePhysics.Think();
	}

	StudioFrameAdvance();

	if ( IsSequenceFinished() )
	{
		if ( m_bExitAnimOn )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
			if ( pPlayer )
			{
				pPlayer->LeaveVehicle();		// now that sequence is finished, leave car
				Vector vecEyes;
				QAngle vecEyeAng;
				GetAttachment( "vehicle_driver_eyes", vecEyes, vecEyeAng );
				vecEyeAng.x = 0;
				vecEyeAng.z = 0;
				pPlayer->SnapEyeAngles( vecEyeAng );			
			}
			m_bExitAnimOn = false;
		}
		int iSequence = SelectWeightedSequence( ACT_IDLE );
		if ( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			m_flCycle = 0;
			m_flAnimTime = gpGlobals->curtime;
			ResetSequence( iSequence );
			ResetClientsideFrame();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::DriveVehicle( float flFrameTime, int iButtons, int iButtonsDown, int iButtonsReleased )
{
	VectorTransform( Vector( 0, 42, 84 ), EntityToWorldTransform(), m_vecGunOrigin );

	if ( iButtons & IN_ATTACK )
	{
		FireCannon();
		m_bCoverBlown = true;
		m_flCoverTime = gpGlobals->curtime + COVER_TIME;
	}
	else if ( iButtons & IN_ATTACK2 )
	{
		FireGrenade();
		m_bCoverBlown = true;
		m_flCoverTime = gpGlobals->curtime + COVER_TIME;
	}

	// need to reset cover blown flag??
	if ( gpGlobals->curtime > m_flCoverTime )
	{
		m_bCoverBlown = false;
	}

	UpdateHealth();

	BaseClass::DriveVehicle( flFrameTime, iButtons, iButtonsDown, iButtonsReleased );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::ItemPostFrame( CBasePlayer *player )
{
	// Aim the cannon
	QAngle	transAngles = m_hPlayer->EyeAngles();
	Vector	viewDir;
	// constrain the firing pitch (don't fire at ground, etc.)
	if ( (transAngles[PITCH] < 90) && ( transAngles[PITCH] > CANNON_MAX_DOWN_PITCH) )
		transAngles[PITCH] = CANNON_MAX_DOWN_PITCH;
	if ( (transAngles[PITCH] > 180) && (transAngles[PITCH] < 360 - CANNON_MAX_UP_PITCH ) )	// pointing too high?
		transAngles[PITCH] = 360 - CANNON_MAX_UP_PITCH;
	//transAngles[PITCH] -= 3;		// adjust shot angle
	AngleVectors( transAngles, &viewDir );
	AimPrimaryWeapon( viewDir );

	// Aim the grenade launcher
	Vector vecForward;
	trace_t	tr;
	m_hPlayer->EyeVectors( &vecForward, NULL, NULL );
	Vector	endPos = m_hPlayer->EyePosition() + ( vecForward * MAX_TRACE_LENGTH );
	UTIL_TraceLine( m_hPlayer->EyePosition(), endPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	AimSecondaryWeaponAt( tr.endpos );
}


//-----------------------------------------------------------------------------
// Purpose: Updates health of apc and controls damage it takes
//-----------------------------------------------------------------------------
void CPropAPC::UpdateHealth( void )
{
	// update "health" regen timer and regen amount
	if ( gpGlobals->curtime > m_flRegenTime ) 
	{
		float maxThrottle;
		if ( m_APCHealth > 0 )
		{
			m_APCHealth += APC_MAX_HEALTH / 60;		// 60 seconds to regenerate fully
			if ( m_APCHealth >= APC_MAX_HEALTH )
			{
				m_APCHealth = APC_MAX_HEALTH;		// cap it
				maxThrottle = 1.0;
			}
			else
			{
				// make speed inversely proportional to the damage it's sustained
				maxThrottle = m_APCHealth / APC_MAX_HEALTH * 0.3;
			}
		}
		else
		{
			maxThrottle = 0;			// no throttle when disabled
			m_VehiclePhysics.SetHandbrake( true );
		}
		m_VehiclePhysics.SetMaxThrottle( maxThrottle );

		m_flRegenTime = gpGlobals->curtime + RandomFloat(1.0, 2.0);		// random time for firing
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::FireCannon( void )
{
	if ( m_flCannonTime > gpGlobals->curtime )
		return;

	// If we're still firing the salvo, fire quickly
	m_iCannonBurstLeft--;
	if ( m_iCannonBurstLeft > 0 )
	{
		m_flCannonTime = gpGlobals->curtime + RandomFloat( 0.1, 0.2 );
	}
	else
	{
		// Reload the salvo
		m_iCannonBurstLeft = CANNON_BURST_SIZE;
		m_flCannonTime = gpGlobals->curtime + 4.0f;
	}

	QAngle vecAngles;
	VectorAngles( m_vecGunAimForward, vecAngles );
	Vector vecRight;
	AngleVectors( vecAngles, NULL, &vecRight, NULL );

	// Alternate barrels
	Vector shotOfs;
	if ( m_bLeftBarrel )
	{
		shotOfs = m_vecGunOrigin + ( vecRight * -8.0f );
	}
	else
	{
		shotOfs = m_vecGunOrigin + ( vecRight * 8.0f );
	}
	m_bLeftBarrel = !m_bLeftBarrel;

	// Fire the round
	int	bulletType = GetAmmoDef()->Index("MediumRound");
	FireBullets( 1, shotOfs, m_vecGunAimForward, VECTOR_CONE_4DEGREES, MAX_TRACE_LENGTH, bulletType, 1 );
	g_pEffects->MuzzleFlash( shotOfs, vecAngles, 2.0f, MUZZLEFLASH_TYPE_DEFAULT );

	EmitSound( "PropAPC.FireCannon" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAPC::FireGrenade( void )
{
	if ( m_flGrenadeTime > gpGlobals->curtime )
		return;

	// Launch a grenade to hit the current grenade target
	Vector vecGrenOrg = GetLocalOrigin() + Vector(0,0,32);
	Vector vecVelocity = VecCheckToss( this, vecGrenOrg, m_vecGrenadeTarget, -1, 0.5, true );
	if ( vecVelocity == vec3_origin )
		return;

	// If we're still firing the salvo, fire quickly
	m_iGrenadeSalvoLeft--;
	if ( m_iGrenadeSalvoLeft > 0 )
	{
		m_flGrenadeTime = gpGlobals->curtime + RandomFloat( 0.1,0.3 );
	}
	else
	{
		// Reload the salvo
		m_iGrenadeSalvoLeft = GRENADE_SALVO_SIZE;
		m_flGrenadeTime = gpGlobals->curtime + 4.0f;
	}

	// Launch the grenade
	CBaseGrenade *pGrenade = (CBaseGrenade*)CreateNoSpawn( "npc_contactgrenade", vecGrenOrg, vec3_angle, this );
	pGrenade->Spawn();
	pGrenade->SetOwner( (CBaseCombatCharacter*)GetDriver() );
	pGrenade->SetOwnerEntity( this );
	QAngle vecSpin;
	vecSpin.x = random->RandomFloat( -1000.0, 1000.0 );
	vecSpin.y = random->RandomFloat( -1000.0, 1000.0 );
	vecSpin.z = random->RandomFloat( -1000.0, 1000.0 );
	pGrenade->SetAbsVelocity( vecVelocity );
	pGrenade->SetLocalAngularVelocity( vecSpin );

	EmitSound( "PropAPC.FireRocket" );
	UTIL_Smoke(m_vecGunOrigin,random->RandomInt(10, 15), 10);
}

//-------------------------------------------------------------
// EnterVehicle -- play animation of getting into the vehicle
//-------------------------------------------------------------
void CPropAPC::EnterVehicle( CBasePlayer *pPlayer )
{
	BaseClass::EnterVehicle( pPlayer );

	// setup to play "enter" sequence
	int iSequence = LookupSequence( "enter" );
	// skip if no animation present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		m_flCycle = 0;
		m_flAnimTime = gpGlobals->curtime;
		ResetSequence( iSequence );
		ResetClientsideFrame();
		m_bEnterAnimOn = true;
	}
}

//========================================================================================================================================
// APC FOUR WHEEL PHYSICS VEHICLE SERVER VEHICLE
//========================================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCFourWheelServerVehicle::NPC_AimPrimaryWeapon( Vector vecTarget )
{
	CPropAPC *pAPC = ((CPropAPC*)m_pVehicle);

	Vector vecToTarget = ( vecTarget - pAPC->GetPrimaryGunOrigin() );
	VectorNormalize( vecToTarget );
	pAPC->AimPrimaryWeapon( vecToTarget );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCFourWheelServerVehicle::NPC_AimSecondaryWeapon( Vector vecTarget )
{
	// Add some random noise
	Vector vecOffset = vecTarget + RandomVector( -128, 128 );
	((CPropAPC*)m_pVehicle)->AimSecondaryWeaponAt( vecOffset );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCFourWheelServerVehicle::Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange )
{
	*flMinRange = 64;
	*flMaxRange = 1024;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAPCFourWheelServerVehicle::Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange )
{
	*flMinRange = 128;
	*flMaxRange = 2048;
}

//-----------------------------------------------------------------------------
// Purpose: Return the time at which this vehicle's primary weapon can fire again
//-----------------------------------------------------------------------------
float CAPCFourWheelServerVehicle::Weapon_PrimaryCanFireAt( void )
{
	return ((CPropAPC*)m_pVehicle)->PrimaryWeaponFireTime();
}

//-----------------------------------------------------------------------------
// Purpose: Return the time at which this vehicle's secondary weapon can fire again
//-----------------------------------------------------------------------------
float CAPCFourWheelServerVehicle::Weapon_SecondaryCanFireAt( void )
{
	return ((CPropAPC*)m_pVehicle)->SecondaryWeaponFireTime();;
}

