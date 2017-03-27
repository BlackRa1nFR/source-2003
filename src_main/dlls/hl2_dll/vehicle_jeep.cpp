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
#include "grenade_ar2.h"
#include "te_effect_dispatch.h"

#define	VEHICLE_HITBOX_DRIVER		1
#define LOCK_SPEED					10
#define JEEP_GUN_YAW				"vehicle_weapon_yaw"
#define JEEP_GUN_PITCH				"vehicle_weapon_pitch"
#define JEEP_GUN_SPIN				"gun_spin"
#define	JEEP_GUN_SPIN_RATE			20

#define CANNON_MAX_UP_PITCH			20
#define CANNON_MAX_DOWN_PITCH		20
#define CANNON_MAX_LEFT_YAW			90
#define CANNON_MAX_RIGHT_YAW		90

#define OVERTURNED_EXIT_WAITTIME	2.0f

#define JEEP_AMMOCRATE_HITGROUP		5
#define JEEP_WHEEL_COUNT			4

#define JEEP_STEERING_SLOW_FRAC		0.333f
#define JEEP_STEERING_SLOW_ANGLE	50.0f
#define JEEP_STEERING_AVG_ANGLE		25.0f
#define JEEP_STEERING_FAST_FRAC		0.677f
#define JEEP_STEERING_FAST_ANGLE	10.0f

ConVar	jeep_use_mortar( "jeep_use_mortar", "0" );

//-----------------------------------------------------------------------------
// Purpose: Four wheel physics vehicle server vehicle with weaponry
//-----------------------------------------------------------------------------
class CJeepFourWheelServerVehicle : public CFourWheelServerVehicle
{
	typedef CFourWheelServerVehicle BaseClass;
// IServerVehicle
public:
	bool		NPC_HasPrimaryWeapon( void ) { return true; }
	void		NPC_AimPrimaryWeapon( Vector vecTarget );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPropJeep : public CPropVehicleDriveable
{
	DECLARE_CLASS( CPropJeep, CPropVehicleDriveable );

public:

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CPropJeep( void );

	// CPropVehicle
	virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual void	DriveVehicle( float flFrameTime, int iButtons, int iButtonsDown, int iButtonsReleased );
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );

	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// CBaseEntity
	void			Think(void);
	void			Precache( void );
	void			Spawn( void );

	virtual void	CreateServerVehicle( void );
	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true );
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	ExitVehicle( int iRole );

	void			AimGunAt( Vector *endPos, float flInterval );

	// NPC Driving
	bool			NPC_HasPrimaryWeapon( void ) { return true; }
	void			NPC_AimPrimaryWeapon( Vector vecTarget );

private:

	void		FireCannon( void );
	void		ChargeCannon( void );
	void		FireChargedCannon( void );

	void		DrawBeam( const Vector &startPos, const Vector &endPos, float width );
	void		StopChargeSound( void );
	void		GetCannonAim( Vector *resultDir );

	void		InitWaterData( void );
	void		HandleWater( void );
	bool		CheckWater( void );
	void		CreateSplash( const Vector &vecPosition );
	void		CreateRipple( const Vector &vecPosition );

	void		UpdateSteeringAngle( void );
	void		CreateDangerSounds( void );

	float		m_flDangerSoundTime;
	int			m_nBulletType;
	bool		m_bCannonCharging;
	float		m_flCannonTime;
	float		m_flCannonChargeStartTime;
	Vector		m_vecGunOrigin;
	CSoundPatch	*m_sndCannonCharge;
	int			m_nSpinPos;
	float		m_aimYaw;
	float		m_aimPitch;
	float		m_throttleDisableTime;

	// handbrake after the fact to keep vehicles from rolling
	float		m_flHandbrakeTime;
	bool		m_bInitialHandbrake;

	float		m_flOverturnedTime;

	struct JeepWaterData_t
	{
		bool		m_bWheelInWater[JEEP_WHEEL_COUNT];
		bool		m_bWheelWasInWater[JEEP_WHEEL_COUNT];
		Vector		m_vecWheelContactPoints[JEEP_WHEEL_COUNT];
		bool		m_bBodyInWater;
		bool		m_bBodyWasInWater;
	};
	
	JeepWaterData_t	m_WaterData;
};

BEGIN_DATADESC( CPropJeep )
	DEFINE_FIELD( CPropJeep, m_flDangerSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CPropJeep, m_nBulletType, FIELD_INTEGER ),
	DEFINE_FIELD( CPropJeep, m_bCannonCharging, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropJeep, m_flCannonTime, FIELD_TIME ),
	DEFINE_FIELD( CPropJeep, m_flCannonChargeStartTime, FIELD_TIME ),
	DEFINE_FIELD( CPropJeep, m_vecGunOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_SOUNDPATCH( CPropJeep, m_sndCannonCharge ),
	DEFINE_FIELD( CPropJeep, m_nSpinPos, FIELD_INTEGER ),
	DEFINE_FIELD( CPropJeep, m_aimYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CPropJeep, m_aimPitch, FIELD_FLOAT ),
	DEFINE_FIELD( CPropJeep, m_throttleDisableTime, FIELD_TIME ),
	DEFINE_FIELD( CPropJeep, m_flHandbrakeTime, FIELD_TIME ),
	DEFINE_FIELD( CPropJeep, m_bInitialHandbrake, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropJeep, m_flOverturnedTime, FIELD_TIME ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPropJeep, DT_PropJeep )
END_SEND_TABLE();

LINK_ENTITY_TO_CLASS( prop_vehicle_jeep, CPropJeep );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropJeep::CPropJeep( void )
{
	m_bCannonCharging = false;
	m_flCannonChargeStartTime = 0;
	m_flCannonTime = 0;
	m_nBulletType = -1;
	m_flOverturnedTime = 0.0f;

	InitWaterData();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::CreateServerVehicle( void )
{
	// Create our armed server vehicle
	m_pServerVehicle = new CJeepFourWheelServerVehicle();
	m_pServerVehicle->SetVehicle( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::Precache( void )
{
	enginesound->PrecacheSound("weapons/gauss/chargeloop.wav");

	BaseClass::Precache();
}

//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropJeep::Spawn( void )
{
	// Setup vehicle as a real-wheels car.
	SetVehicleType( VEHICLE_TYPE_CAR_WHEELS );

	BaseClass::Spawn();
	m_flHandbrakeTime = gpGlobals->curtime + 0.1;
	m_bInitialHandbrake = false;

	m_flMinimumSpeedToEnterExit = LOCK_SPEED;

	m_nBulletType = GetAmmoDef()->Index("GaussEnergy");

	// Initialize pose parameters
	SetPoseParameter( JEEP_GUN_YAW, 0 );
	SetPoseParameter( JEEP_GUN_PITCH, 0 );
	m_nSpinPos = 0;
	SetPoseParameter( JEEP_GUN_SPIN, m_nSpinPos );
	m_aimYaw = 0;
	m_aimPitch = 0;

	SetSolidFlags( FSOLID_NOT_STANDABLE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;
	if ( ptr->hitbox != VEHICLE_HITBOX_DRIVER )
	{
		if ( inputInfo.GetDamageType() & DMG_BULLET )
		{
			info.ScaleDamage( 0.0001 );
		}
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropJeep::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	//Do scaled up physics damage to the car
	CTakeDamageInfo info = inputInfo;
	info.ScaleDamage( 25 );
	
	// HACKHACK: Scale up grenades until we get a better explosion/pressure damage system
	if ( inputInfo.GetDamageType() & DMG_BLAST )
	{
		info.SetDamageForce( inputInfo.GetDamageForce() * 10 );
	}
	VPhysicsTakeDamage( info );

	// reset the damage
	info.SetDamage( inputInfo.GetDamage() );

	// small amounts of shock damage disrupt the car, but aren't transferred to the player
	if ( info.GetDamageType() == DMG_SHOCK )
	{
		if ( info.GetDamage() <= 10 )
		{
			// take 10% damage and make the engine stall
			info.ScaleDamage( 0.1 );
			m_throttleDisableTime = gpGlobals->curtime + 2;
		}
	}

	//Check to do damage to driver
	if ( GetDriver() )
	{
		//Take no damage from physics damages
		if ( info.GetDamageType() & DMG_CRUSH )
			return 0;

		//Take the damage
		GetDriver()->TakeDamage( info );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CPropJeep::BodyTarget( const Vector &posSrc, bool bNoisy )
{
	Vector	shotPos;
	matrix3x4_t	matrix;

	int eyeAttachmentIndex = LookupAttachment("vehicle_driver_eyes");
	GetAttachment( eyeAttachmentIndex, matrix );
	MatrixGetColumn( matrix, 3, shotPos );

	if ( bNoisy )
	{
		shotPos[0] += random->RandomFloat( -8.0f, 8.0f );
		shotPos[1] += random->RandomFloat( -8.0f, 8.0f );
		shotPos[2] += random->RandomFloat( -8.0f, 8.0f );
	}

	return shotPos;
}

//-----------------------------------------------------------------------------
// Purpose: Aim Gun at a target
//-----------------------------------------------------------------------------
void CPropJeep::AimGunAt( Vector *endPos, float flInterval )
{
	matrix3x4_t gunMatrix;
	GetAttachment( LookupAttachment("gun_ref"), gunMatrix );

	// transform the enemy into gun space
	Vector localEnemyPosition;
	VectorITransform( *endPos, gunMatrix, localEnemyPosition );

	// do a look at in gun space (essentially a delta-lookat)
	QAngle localEnemyAngles;
	VectorAngles( localEnemyPosition, localEnemyAngles );
	
	// convert to +/- 180 degrees
	localEnemyAngles.x = UTIL_AngleDiff( localEnemyAngles.x, 0 );	
	localEnemyAngles.y = UTIL_AngleDiff( localEnemyAngles.y, 0 );

	float targetYaw = m_aimYaw + localEnemyAngles.y;
	float targetPitch = m_aimPitch + localEnemyAngles.x;
	
	// Constrain our angles
	targetYaw	= clamp( targetYaw, -CANNON_MAX_LEFT_YAW, CANNON_MAX_RIGHT_YAW );
	targetPitch = clamp( targetPitch, -CANNON_MAX_DOWN_PITCH, CANNON_MAX_UP_PITCH );

	// Exponentially approach the target
	float yawSpeed = 8;
	float pitchSpeed = 8;

	m_aimYaw = UTIL_Approach( targetYaw, m_aimYaw, yawSpeed );
	m_aimPitch = UTIL_Approach( targetPitch, m_aimPitch, pitchSpeed );

	SetPoseParameter( JEEP_GUN_YAW, -m_aimYaw);
	SetPoseParameter( JEEP_GUN_PITCH, -m_aimPitch );

	InvalidateBoneCache();

	// read back to avoid drift when hitting limits
	// as long as the velocity is less than the delta between the limit and 180, this is fine.
	m_aimPitch = -GetPoseParameter( JEEP_GUN_PITCH );
	m_aimYaw = -GetPoseParameter( JEEP_GUN_YAW );

	// Now draw crosshair for actual aiming point
	Vector	vecMuzzle, vecMuzzleDir;
	QAngle	vecMuzzleAng;

	GetAttachment( "Muzzle", vecMuzzle, vecMuzzleAng );
	AngleVectors( vecMuzzleAng, &vecMuzzleDir );

	trace_t	tr;
	UTIL_TraceLine( vecMuzzle, vecMuzzle + (vecMuzzleDir * MAX_TRACE_LENGTH), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		// see if we hit something, if so, adjust endPos to hit location
	if ( tr.fraction < 1.0 )
	{
		m_vecGunCrosshair = vecMuzzle + ( vecMuzzleDir * MAX_TRACE_LENGTH * tr.fraction );
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::InitWaterData( void )
{
	m_WaterData.m_bBodyInWater = false;
	m_WaterData.m_bBodyWasInWater = false;

	for ( int iWheel = 0; iWheel < JEEP_WHEEL_COUNT; ++iWheel )
	{
		m_WaterData.m_bWheelInWater[iWheel] = false;
		m_WaterData.m_bWheelWasInWater[iWheel] = false;
		m_WaterData.m_vecWheelContactPoints[iWheel].Init();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::HandleWater( void )
{
	// Check to see if we are in water.
	if ( CheckWater() )
	{
		for ( int iWheel = 0; iWheel < JEEP_WHEEL_COUNT; ++iWheel )
		{
			// Create an entry/exit splash!
			if ( m_WaterData.m_bWheelInWater[iWheel] != m_WaterData.m_bWheelWasInWater[iWheel] )
			{
				CreateSplash( m_WaterData.m_vecWheelContactPoints[iWheel] );
				CreateRipple( m_WaterData.m_vecWheelContactPoints[iWheel] );
			}
			
			// Create ripples.
			if ( m_WaterData.m_bWheelInWater[iWheel] && m_WaterData.m_bWheelWasInWater[iWheel] )
			{
				CreateRipple( m_WaterData.m_vecWheelContactPoints[iWheel] );
			}
		}
	}

	// Save of data from last think.
	for ( int iWheel = 0; iWheel < JEEP_WHEEL_COUNT; ++iWheel )
	{
		m_WaterData.m_bWheelWasInWater[iWheel] = m_WaterData.m_bWheelInWater[iWheel];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPropJeep::CheckWater( void )
{
	bool bInWater = false;

	// Check all four wheels.
	for ( int iWheel = 0; iWheel < JEEP_WHEEL_COUNT; ++iWheel )
	{
		// Get the current wheel and get its contact point.
		IPhysicsObject *pWheel = m_VehiclePhysics.GetWheel( iWheel );
		if ( !pWheel )
			continue;

		// Check to see if we hit water.
		pWheel->GetContactPoint( &m_WaterData.m_vecWheelContactPoints[iWheel], NULL );
		m_WaterData.m_bWheelInWater[iWheel] = ( UTIL_PointContents( m_WaterData.m_vecWheelContactPoints[iWheel] ) & MASK_WATER ) ? true : false;
		if ( m_WaterData.m_bWheelInWater[iWheel] )
		{
			bInWater = true;
		}
	}

	// Check the body and the BONNET.
	int iEngine = LookupAttachment( "vehicle_engine" );
	Vector vecEnginePoint;
	QAngle vecEngineAngles;
	GetAttachment( iEngine, vecEnginePoint, vecEngineAngles );

	m_WaterData.m_bBodyInWater = ( UTIL_PointContents( vecEnginePoint ) & MASK_WATER ) ? true : false;
	if ( m_WaterData.m_bBodyInWater )
	{
		if ( !m_VehiclePhysics.IsEngineDisabled() )
		{
			m_VehiclePhysics.SetDisableEngine( true );
		}
	}
	else
	{
		if ( m_VehiclePhysics.IsEngineDisabled() )
		{
			m_VehiclePhysics.SetDisableEngine( false );
		}
	}

	return bInWater;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::CreateSplash( const Vector &vecPosition )
{
	// Splash data.
	CEffectData	data;
	data.m_vOrigin = vecPosition;
	data.m_vNormal.Init( 0.0f, 0.0f, 1.0f );
	VectorAngles( data.m_vNormal, data.m_vAngles );
	data.m_flScale = 10.0f + random->RandomFloat( 0, 2 );

	// Create the splash..
	DispatchEffect( "watersplash", data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::CreateRipple( const Vector &vecPosition )
{
	// Ripple data.
	CEffectData	data;
	data.m_vOrigin = vecPosition;
	data.m_vNormal.Init( 0.0f, 0.0f, 1.0f );
	VectorAngles( data.m_vNormal, data.m_vAngles );
	data.m_flScale = 10.0f + random->RandomFloat( 0, 2 );

	// Create the ripple.
	DispatchEffect( "waterripple", data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::Think(void)
{
	BaseClass::Think();

	// Water!?
	HandleWater();

	SetSimulationTime( gpGlobals->curtime );
	
	SetNextThink( gpGlobals->curtime );
	SetAnimatedEveryTick( true );

	if ( m_VehiclePhysics.UpdateBooster() )
	{
		SetNextThink( gpGlobals->curtime );		// booster still running, keep updating
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

	// Check overturned status.
	if ( !IsOverturned() )
	{
		m_flOverturnedTime = 0.0f;
	}
	else
	{
		m_flOverturnedTime += gpGlobals->frametime;
	}

	// spin gun if charging cannon
	//FIXME: Don't bother for E3
	if ( m_bCannonCharging )
	{
		m_nSpinPos += JEEP_GUN_SPIN_RATE;
		SetPoseParameter( JEEP_GUN_SPIN, m_nSpinPos );
	}

	// aim gun based on where player is looking
	if ( m_hPlayer != NULL )
	{
		Vector	eyeDir;
		m_hPlayer->EyeVectors( &eyeDir, NULL, NULL );

		// trace a line from our eyes to some place in the distance
		Vector	endPos = m_hPlayer->EyePosition() + ( eyeDir * MAX_TRACE_LENGTH );
		trace_t	tr;
		UTIL_TraceLine( m_hPlayer->EyePosition(), endPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		// see if we hit something, if so, adjust endPos to hit location
		if ( tr.fraction < 1.0 )
		{
   			endPos = m_hPlayer->EyePosition() + ( eyeDir * MAX_TRACE_LENGTH * tr.fraction );
		}

		m_vecLookCrosshair = endPos;
		AimGunAt( &endPos, 0.1f );
	}

	StudioFrameAdvance();

	// If the exit anim has finished, move the player to the right spot and stop animating
	if ( IsSequenceFinished() )
	{
		GetServerVehicle()->HandleEntryExitFinish( m_bExitAnimOn );
		m_bExitAnimOn = false;
		m_bEnterAnimOn = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &startPos - 
//			&endPos - 
//			width - 
//			useMuzzle - 
//-----------------------------------------------------------------------------
void CPropJeep::DrawBeam( const Vector &startPos, const Vector &endPos, float width )
{
	//Tracer down the middle
	UTIL_Tracer( startPos, endPos, 0, TRACER_DONT_USE_ATTACHMENT, 6500, false, "GaussTracer" );

	//Draw the main beam shaft
	CBeam *pBeam = CBeam::BeamCreate( GAUSS_BEAM_SPRITE, 0.5 );
	
	pBeam->SetStartPos( startPos );
	pBeam->PointEntInit( endPos, this );
	pBeam->SetEndAttachment( LookupAttachment("Muzzle") );
	pBeam->SetWidth( width );
	pBeam->SetEndWidth( 0.05f );
	pBeam->SetBrightness( 255 );
	pBeam->SetColor( 255, 185+random->RandomInt( -16, 16 ), 40 );
	pBeam->RelinkBeam();
	pBeam->LiveForTime( 0.1f );

	//Draw electric bolts along shaft
	pBeam = CBeam::BeamCreate( GAUSS_BEAM_SPRITE, 3.0f );
	
	pBeam->SetStartPos( startPos );
	pBeam->PointEntInit( endPos, this );
	pBeam->SetEndAttachment( LookupAttachment("Muzzle") );

	pBeam->SetBrightness( random->RandomInt( 64, 255 ) );
	pBeam->SetColor( 255, 255, 150+random->RandomInt( 0, 64 ) );
	pBeam->RelinkBeam();
	pBeam->LiveForTime( 0.1f );
	pBeam->SetNoise( 1.6f );
	pBeam->SetEndWidth( 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::FireCannon( void )
{
	//Don't fire again if it's been too soon
	if ( m_flCannonTime > gpGlobals->curtime )
		return;

	if ( jeep_use_mortar.GetBool() )
	{
		m_flCannonTime	= gpGlobals->curtime + 1.0f;

		Vector aimDir;
		GetCannonAim( &aimDir );

		//Create the grenade
		CGrenadeAR2 *pGrenade = (CGrenadeAR2*) Create( "grenade_ar2", m_vecGunOrigin, vec3_angle, this );
		pGrenade->SetAbsVelocity( aimDir * 1200.0f );

		pGrenade->SetLocalAngularVelocity( RandomAngle( -400, 400 ) );
		pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY ); 
		
		CBaseEntity *pDriver = GetDriver();

		if ( pDriver != NULL )
		{
			pGrenade->SetOwner( pDriver->MyCombatCharacterPointer() );
		}

		pGrenade->SetDamage( 50.0f );

		EmitSound( "PropAPC.FireRocket" );
	}
	else
	{
		m_flCannonTime = gpGlobals->curtime + 0.2f;
		m_bCannonCharging = false;

		//Find the direction the gun is pointing in
		Vector aimDir;
		GetCannonAim( &aimDir );

		Vector vecUp, vecRight;
		VectorVectors( aimDir, vecRight, vecUp );

		float x, y, z;

		//Gassian spread
		do {
			x = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
			y = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
			z = x*x+y*y;
		} while (z > 1);

		aimDir	= aimDir + x * VECTOR_CONE_1DEGREES.x * vecRight + y * VECTOR_CONE_1DEGREES.y * vecUp;

		Vector endPos = m_vecGunOrigin + ( aimDir * 10000 );

		//Shoot a shot straight out
		trace_t	tr;
		UTIL_TraceLine( m_vecGunOrigin, endPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		
		ClearMultiDamage();

		CBaseEntity *pHit = tr.m_pEnt;
		
		if ( pHit != NULL )
		{
			CAmmoDef *pAmmoDef = GetAmmoDef();
			int ammoType = pAmmoDef->Index("GaussEnergy");
			CTakeDamageInfo dmgInfo( this, GetDriver(), 35/*pAmmoDef->PlrDamage(ammoType)*/, DMG_SHOCK );
			CalculateBulletDamageForce( &dmgInfo, ammoType, aimDir, tr.endpos );

			pHit->DispatchTraceAttack( dmgInfo, aimDir, &tr );
		}
		
		//NOTENOTE: We do not reflect off of surfaces in this version of the gun

		//Draw our beam
		DrawBeam( tr.startpos, tr.endpos, 2.4 );
		
		ApplyMultiDamage();

		UTIL_ImpactTrace( &tr, m_nBulletType, "ImpactJeep" );

		if ( (tr.surface.flags & SURF_SKY) == false )
		{
			CPVSFilter filter( tr.endpos );
			te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );
		}

		// Register a muzzleflash for the AI
		if ( m_hPlayer )
		{
			m_hPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );
		}

		CPASAttenuationFilter sndFilter( this, "PropJeep.FireCannon" );
		EmitSound( sndFilter, entindex(), "PropJeep.FireCannon" );

		// make cylinders of gun spin a bit
		m_nSpinPos += JEEP_GUN_SPIN_RATE;
		//SetPoseParameter( JEEP_GUN_SPIN, m_nSpinPos );	//FIXME: Don't bother with this for E3, won't look right
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::FireChargedCannon( void )
{
	bool penetrated = false;

	m_bCannonCharging	= false;
	m_flCannonTime		= gpGlobals->curtime + 0.5f;

	StopChargeSound();

	CPASAttenuationFilter sndFilter( this, "PropJeep.FireChargedCannon" );
	EmitSound( sndFilter, entindex(), "PropJeep.FireChargedCannon" );

	//Find the direction the gun is pointing in
	Vector aimDir;
	GetCannonAim( &aimDir );

	Vector endPos = m_vecGunOrigin + ( aimDir * MAX_TRACE_LENGTH );
	
	//Shoot a shot straight out
	trace_t	tr;
	UTIL_TraceLine( m_vecGunOrigin, endPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	
	if ( tr.surface.flags & SURF_SKY )
		return;

	ClearMultiDamage();

	//Find how much damage to do
	float flChargeAmount = ( gpGlobals->curtime - m_flCannonChargeStartTime ) / MAX_GAUSS_CHARGE_TIME;

	//Clamp this
	if ( flChargeAmount > 1.0f )
	{
		flChargeAmount = 1.0f;
	}

	//Determine the damage amount
	//FIXME: Use ConVars!
	float flDamage = 15 + ( ( 250 - 15 ) * flChargeAmount );

	CBaseEntity *pHit = tr.m_pEnt;
	
	//Look for wall penetration
	if ( tr.DidHitWorld() )
	{
		//Try wall penetration
		UTIL_ImpactTrace( &tr, m_nBulletType, "ImpactJeep" );
		UTIL_DecalTrace( &tr, "RedGlowFade" );

		CPVSFilter filter( tr.endpos );
		te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );
		
		Vector	testPos = tr.endpos + ( aimDir * 48.0f );

		UTIL_TraceLine( testPos, tr.endpos, MASK_SHOT, GetDriver(), COLLISION_GROUP_NONE, &tr );
			
		if ( tr.allsolid == false )
		{
			UTIL_DecalTrace( &tr, "RedGlowFade" );

			penetrated = true;
		}
	}
	else if ( pHit != NULL )
	{
		CTakeDamageInfo dmgInfo( this, GetDriver(), flDamage, DMG_SHOCK );
		CalculateBulletDamageForce( &dmgInfo, GetAmmoDef()->Index("GaussEnergy"), aimDir, tr.endpos );

		//Do direct damage to anything in our path
		pHit->DispatchTraceAttack( dmgInfo, aimDir, &tr );
	}

	ApplyMultiDamage();

	//Kick up an effect
	UTIL_ImpactTrace( &tr, m_nBulletType, "ImpactJeep" );

	//Show the effect
	DrawBeam( m_vecGunOrigin, tr.endpos, 9.6 );

	//Do a gauss explosion
	CPVSFilter filter( tr.endpos );
	te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );

	// Register a muzzleflash for the AI
	if ( m_hPlayer )
	{
		m_hPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5f );
	}

	//Rock the car
	IPhysicsObject *pObj = VPhysicsGetObject();

	if ( pObj != NULL )
	{
		Vector	shoveDir = aimDir * -( flDamage * 500.0f );

		pObj->ApplyForceOffset( shoveDir, m_vecGunOrigin );
	}

	//Do radius damage if we didn't penetrate the wall
	if ( penetrated == true )
	{
		RadiusDamage( CTakeDamageInfo( this, this, flDamage, DMG_SHOCK ), tr.endpos, 200.0f, CLASS_NONE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::ChargeCannon( void )
{
	//Don't fire again if it's been too soon
	if ( m_flCannonTime > gpGlobals->curtime )
		return;

	//See if we're starting a charge
	if ( m_bCannonCharging == false )
	{
		m_flCannonChargeStartTime = gpGlobals->curtime;
		m_bCannonCharging = true;

		//Start charging sound
		CPASAttenuationFilter filter( this );
		m_sndCannonCharge = (CSoundEnvelopeController::GetController()).SoundCreate( filter, entindex(), CHAN_STATIC, "weapons/gauss/chargeloop.wav", ATTN_NORM );

		assert(m_sndCannonCharge!=NULL);
		if ( m_sndCannonCharge != NULL )
		{
			(CSoundEnvelopeController::GetController()).Play( m_sndCannonCharge, 1.0f, 50 );
			(CSoundEnvelopeController::GetController()).SoundChangePitch( m_sndCannonCharge, 250, 3.0f );
		}

		return;
	}

	//TODO: Add muzzle effect?

	//TODO: Check for overcharge and have the weapon simply fire or instead "decharge"?
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::StopChargeSound( void )
{
	if ( m_sndCannonCharge != NULL )
	{
		(CSoundEnvelopeController::GetController()).SoundFadeOut( m_sndCannonCharge, 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Finds the true aiming position of the gun (looks at what player 
//			is looking at and adjusts)
// Input  : &resultDir - direction to be calculated
//-----------------------------------------------------------------------------
void CPropJeep::GetCannonAim( Vector *resultDir )
{
	Vector	muzzleOrigin;
	QAngle	muzzleAngles;

	GetAttachment( LookupAttachment("gun_ref"), muzzleOrigin, muzzleAngles );

	AngleVectors( muzzleAngles, resultDir );
}

//-----------------------------------------------------------------------------
// Purpose: If the player uses the jeep while at the back, he gets ammo from the crate instead
//-----------------------------------------------------------------------------
void CPropJeep::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( !pPlayer )
		return;

	// Find out if the player's looking at our ammocrate hitbox 
	Vector vecForward;
	pPlayer->EyeVectors( &vecForward, NULL, NULL );

	trace_t tr;
	Vector vecStart = pPlayer->EyePosition();
	UTIL_TraceLine( vecStart, vecStart + vecForward * 1024, MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_HITBOX, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.m_pEnt == this && tr.hitgroup == JEEP_AMMOCRATE_HITGROUP )
	{
		// Player's using the crate.
		// Fill up his SMG ammo.
		pPlayer->GiveAmmo( 200, "MediumRound");
		return;
	}

	// Fall back and get in the vehicle instead
	BaseClass::Use( pActivator, pCaller, useType, value );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// If we are overturned and hit any key - leave the vehicle (IN_USE is already handled!).
	if ( m_flOverturnedTime > OVERTURNED_EXIT_WAITTIME )
	{
		if ( (ucmd->buttons & (IN_FORWARD|IN_BACK|IN_MOVELEFT|IN_MOVERIGHT|IN_SPEED|IN_JUMP|IN_ATTACK|IN_ATTACK2) ) && !m_bExitAnimOn )
		{
			// Can't exit yet? We're probably still moving. Swallow the keys.
			if ( !CanExitVehicle(player) )
				return;

			if ( PlayExitAnimation() )
				return;
			
			player->LeaveVehicle();
			return;
		}
	}

	// If the throttle is disabled or we're upside-down, don't allow throttling (including turbo)
	CUserCmd tmp;
	if ( ( m_throttleDisableTime > gpGlobals->curtime ) || ( IsOverturned() ) )
	{
		tmp = (*ucmd);
		tmp.buttons &= ~(IN_FORWARD|IN_BACK|IN_SPEED);
		ucmd = &tmp;
	}
	
	// Can't turbo while reversing
	if ( ucmd->buttons & (IN_BACK|IN_JUMP) )
	{
		tmp = (*ucmd);
		tmp.buttons &= ~IN_SPEED;
		ucmd = &tmp;
	}

	BaseClass::SetupMove( player, ucmd, pHelper, move );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::DriveVehicle( float flFrameTime, int iButtons, int iButtonsDown, int iButtonsReleased )
{
	// If we're holding down an attack button, update our state
	if ( IsOverturned() == false )
	{
		if ( iButtons & IN_ATTACK )
		{
			FireCannon();
		}
		else if ( iButtons & IN_ATTACK2 )
		{
			if ( jeep_use_mortar.GetBool() == false )
			{
				ChargeCannon();
			}
		}
	}

	// If we've released our secondary button, fire off our cannon
	if ( ( iButtonsReleased & IN_ATTACK2 ) && ( m_bCannonCharging ) )
	{
		if ( jeep_use_mortar.GetBool() == false )
		{
			FireChargedCannon();
		}
	}

	BaseClass::DriveVehicle( flFrameTime, iButtons, iButtonsDown, iButtonsReleased );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pMoveData - 
//-----------------------------------------------------------------------------
void CPropJeep::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData )
{
	BaseClass::ProcessMovement( pPlayer, pMoveData );

	// Update the steering angles based on speed.
	UpdateSteeringAngle();

	// Create dangers sounds in front of the vehicle.
	CreateDangerSounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::UpdateSteeringAngle( void )
{
	float flMaxSpeed = m_VehiclePhysics.GetMaxSpeed();
	float flSpeed = m_VehiclePhysics.GetSpeed();

	float flSlowSpeed = flMaxSpeed * JEEP_STEERING_SLOW_FRAC;
	float flFastSpeed = flMaxSpeed * JEEP_STEERING_FAST_FRAC;

	if ( flSpeed <= flSlowSpeed )
	{
		m_VehiclePhysics.SetSteeringDegrees( JEEP_STEERING_SLOW_ANGLE );
	}
	else if ( flSpeed >= flFastSpeed )
	{
		m_VehiclePhysics.SetSteeringDegrees( JEEP_STEERING_FAST_ANGLE );
	}
	else
	{
		m_VehiclePhysics.SetSteeringDegrees( JEEP_STEERING_AVG_ANGLE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create danger sounds in front of the vehicle.
//-----------------------------------------------------------------------------
void CPropJeep::CreateDangerSounds( void )
{
	QAngle dummy;
	GetAttachment( "Muzzle", m_vecGunOrigin, dummy );

//	if ( m_flDangerSoundTime > gpGlobals->curtime )
//		return;

	QAngle vehicleAngles = GetLocalAngles();
	Vector vecStart = GetAbsOrigin();
	Vector vecDir, vecRight;

	GetVectors( &vecDir, &vecRight, NULL );

	const float soundDuration = 0.25;
	float speed = m_VehiclePhysics.GetHLSpeed();
	// Make danger sounds ahead of the jeep
	if ( fabs(speed) > 120 )
	{
		Vector	vecSpot;

		float steering = m_VehiclePhysics.GetSteering();
		if ( steering != 0 )
		{
			if ( speed > 0 )
			{
				vecDir += vecRight * steering * 0.5;
			}
			else
			{
				vecDir -= vecRight * steering * 0.5;
			}
			VectorNormalize(vecDir);
		}
		const float radius = speed * 0.4;
		// 0.3 seconds ahead of the jeep
		vecSpot = vecStart + vecDir * (speed * 0.3f);
		CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, radius, soundDuration, this, 0 );
		CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, vecSpot, radius, soundDuration, this, 1 );
		//NDebugOverlay::Box(vecSpot, Vector(-radius,-radius,-radius),Vector(radius,radius,radius), 255, 0, 255, 0, soundDuration);

#if 0
		trace_t	tr;
		// put sounds a bit to left and right but slightly closer to Jeep to make a "cone" of sound 
		// in front of it
		vecSpot = vecStart + vecDir * (speed * 0.5f) - vecRight * speed * 0.5;
		UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, soundDuration, this, 1 );

		vecSpot = vecStart + vecDir * (speed * 0.5f) + vecRight * speed * 0.5;
		UTIL_TraceLine( vecStart, vecSpot, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		CSoundEnt::InsertSound( SOUND_DANGER, vecSpot, 400, soundDuration, this, 2);
#endif
	}

	m_flDangerSoundTime = gpGlobals->curtime + 0.1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeep::ExitVehicle( int iRole )
{
	BaseClass::ExitVehicle(iRole);

	//If the player has exited, stop charging
	StopChargeSound();
	m_bCannonCharging = false;
	m_bEnterAnimOn = false;
}

//========================================================================================================================================
// JEEP FOUR WHEEL PHYSICS VEHICLE SERVER VEHICLE
//========================================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CJeepFourWheelServerVehicle::NPC_AimPrimaryWeapon( Vector vecTarget )
{
	((CPropJeep*)m_pVehicle)->AimGunAt( &vecTarget, 0.1f );
}
