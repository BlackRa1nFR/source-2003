//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "vehicle_base.h"
#include "engine/IEngineSound.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "IEffects.h"
#include "beam_shared.h"
#include "weapon_gauss.h"
#include "soundenvelope.h"
#include "decals.h"
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "physics_saverestore.h"

#define	VEHICLE_HITBOX_DRIVER	1

#define AIRBOAT_SPLASH_RIPPLE	0
#define AIRBOAT_SPLASH_SPRAY	1
#define AIRBOAT_SPLASH_RIPPLE_SIZE	20.0f

class CPropAirboat : public CPropVehicleDriveable
{
	DECLARE_CLASS( CPropAirboat, CPropVehicleDriveable );
	DECLARE_DATADESC();

public:

	// BUGBUG: NOTE: Charlie: Disabled save/load until it's implemented on the airboat controller in vphysics!
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }

	// CPropVehicle
	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual void	DriveVehicle( CBasePlayer *pPlayer, CUserCmd *ucmd );
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );

	// CBaseEntity
	void			Think(void);
	void			Precache( void );
	void			Spawn( void );

	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

private:

	void			FireGun( void );

	void			UpdateSplashEffects( void );
	void			CreateSplash( int nSplashType );

private:

	float						m_flHandbrakeTime;				// handbrake after the fact to keep vehicles from rolling
	bool						m_bInitialHandbrake;
	IPhysicsVehicleController  *m_pVehicle;
};

LINK_ENTITY_TO_CLASS( prop_vehicle_airboat, CPropAirboat );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CPropAirboat )
	DEFINE_FIELD( CPropAirboat, m_flHandbrakeTime,			FIELD_TIME ),
	DEFINE_FIELD( CPropAirboat, m_bInitialHandbrake,		FIELD_BOOLEAN ),
	DEFINE_PHYSPTR( CPropAirboat, m_pVehicle ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::Spawn( void )
{
	// Setup vehicle as a ray-cast airboat.
	SetVehicleType( VEHICLE_TYPE_AIRBOAT_RAYCAST );
	SetCollisionGroup( COLLISION_GROUP_VEHICLE );

	BaseClass::Spawn();

	// Handbrake data.
	m_flHandbrakeTime = gpGlobals->curtime + 0.1;
	m_bInitialHandbrake = false;
	m_VehiclePhysics.SetHasBrakePedal( false );

	// Slow reverse.
	m_VehiclePhysics.SetMaxReverseThrottle( -0.3f );

	// Setup vehicle variables.
	m_pVehicle = m_VehiclePhysics.GetVehicle();

	m_takedamage = DAMAGE_EVENTS_ONLY;

	// Get the physics object so we can adjust the buoyancy.
	IPhysicsObject *pPhysAirboat = VPhysicsGetObject();
	if ( pPhysAirboat )
	{
		pPhysAirboat->SetBuoyancyRatio( 0.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( ptr->hitbox == VEHICLE_HITBOX_DRIVER )
	{
		if ( m_hPlayer != NULL )
		{
			m_hPlayer->TakeDamage( info );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropAirboat::OnTakeDamage( const CTakeDamageInfo &info )
{
	//Do scaled up physic damage to the car
	CTakeDamageInfo physDmg = info;
	physDmg.ScaleDamage( 5 );
	VPhysicsTakeDamage( physDmg );

	//Check to do damage to driver
	if ( m_hPlayer != NULL )
	{
		//Take no damage from physics damages
		if ( info.GetDamageType() & DMG_CRUSH )
			return 0;

		//Take the damage
		physDmg.ScaleDamage( 1 );
		m_hPlayer->TakeDamage( info );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::Think(void)
{
	BaseClass::Think();
	
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

	// play enter animation
	if ( (m_bEnterAnimOn || m_bExitAnimOn) && !IsSequenceFinished() )
	{
		StudioFrameAdvance();
	}
	else if ( IsSequenceFinished() )
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
		m_bEnterAnimOn = false;
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
void CPropAirboat::FireGun( void )
{
	// Trace from eyes and see what we hit.
	Vector vecEyeDirection;
	m_hPlayer->EyeVectors( &vecEyeDirection, NULL, NULL );
	Vector vecEndPos = m_hPlayer->EyePosition() + ( vecEyeDirection * MAX_TRACE_LENGTH );
	trace_t	trace;
	UTIL_TraceLine( m_hPlayer->EyePosition(), vecEndPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &trace );

	if ( trace.m_pEnt )
	{
		// Get the gun position.
		Vector	vecGunPosition;
		QAngle	vecGunAngles;
		GetAttachment( LookupAttachment( "gun_barrel" ), vecGunPosition, vecGunAngles );
		
		// Get a ray from the gun to the target.
		Vector vecRay = trace.endpos - vecGunPosition;
		VectorNormalize( vecRay );
		
		CAmmoDef *pAmmoDef = GetAmmoDef();
		int ammoType = pAmmoDef->Index( "MediumRound" );
		FireBullets( 1, vecGunPosition, vecRay, vec3_origin, 4096, ammoType );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropAirboat::UpdateSplashEffects( void )
{
	// Splash effects.
	CreateSplash( AIRBOAT_SPLASH_RIPPLE );
//	CreateSplash( AIRBOAT_SPLASH_SPRAY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::DriveVehicle( CBasePlayer *pPlayer, CUserCmd *ucmd )
{
	//Lose control when the player dies
	if ( pPlayer->IsAlive() == false )
		return;

	// Fire gun.
	if ( ucmd->buttons & IN_ATTACK )
	{
		FireGun();
	}

	m_VehiclePhysics.UpdateDriverControls( ucmd->buttons, ucmd->frametime );

	// Create splashes.
	UpdateSplashEffects();

	// Save this data.
	m_nSpeed = m_VehiclePhysics.GetSpeed();
	m_nRPM = m_VehiclePhysics.GetRPM();	
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pMoveData - 
//-----------------------------------------------------------------------------
void CPropAirboat::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData )
{
	BaseClass::ProcessMovement( pPlayer, pMoveData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	DriveVehicle( player, ucmd );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropAirboat::CreateSplash( int nSplashType )
{
	Vector vecSplashPoint;
	QAngle vecSplashAngles;
	GetAttachment( "splash_pt", vecSplashPoint, vecSplashAngles );

	Vector vecForward, vecUp;
	AngleVectors( vecSplashAngles, &vecForward, &vecUp, NULL );

	CEffectData	data;
	data.m_vOrigin = vecSplashPoint;
	
	switch ( nSplashType )
	{
	case AIRBOAT_SPLASH_SPRAY:
		{
			Vector vecSplashDir;
			vecSplashDir = ( vecForward + vecUp ) * 0.5f;
			VectorNormalize( vecSplashDir );
			data.m_vNormal = vecSplashDir;
			data.m_flScale = 10.0f + random->RandomFloat( 0, 10.0f * 0.25 );
			DispatchEffect( "waterripple", data );
			DispatchEffect( "watersplash", data );
		}
	case AIRBOAT_SPLASH_RIPPLE:
		{
			Vector vecSplashDir;
			vecSplashDir = vecUp;
			data.m_vNormal = vecSplashDir;
			data.m_flScale = AIRBOAT_SPLASH_RIPPLE_SIZE + random->RandomFloat( 0, AIRBOAT_SPLASH_RIPPLE_SIZE * 0.25 );
			DispatchEffect( "waterripple", data );
		}
	default:
		{
			return;
		}
	}
}
