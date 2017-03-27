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
#include "soundenvelope.h"
#include "soundent.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"
#include "vcollide_parse.h"
#include "ndebugoverlay.h"
#include "npc_vehicledriver.h"
#include "vehicle_crane.h"

#define	VEHICLE_HITBOX_DRIVER		1

extern ConVar g_debug_vehicledriver;

// Crane spring constants
#define CRANE_SPRING_CONSTANT_HANGING			1e6f
#define CRANE_SPRING_CONSTANT_INITIAL_RAISING	(CRANE_SPRING_CONSTANT_HANGING * 0.5)
#define CRANE_SPRING_CONSTANT_LOWERING			150.0f
#define CRANE_SPRING_DAMPING					1e6f
#define CRANE_SPRING_RELATIVE_DAMPING			10

// Crane bones that have physics followers
static const char *pCraneFollowerBoneNames[] =
{
	"base",
	"arm",
	"platform",
};

// Crane tip
LINK_ENTITY_TO_CLASS( crane_tip, CCraneTip );

BEGIN_DATADESC( CCraneTip )

	DEFINE_PHYSPTR( CCraneTip, m_pSpring ),

END_DATADESC()

// Crane
LINK_ENTITY_TO_CLASS( prop_vehicle_crane, CPropCrane );

BEGIN_DATADESC( CPropCrane )

	// Inputs
	DEFINE_INPUTFUNC( CPropCrane, FIELD_VOID, "Lock",	InputLock ),
	DEFINE_INPUTFUNC( CPropCrane, FIELD_VOID, "Unlock",	InputUnlock ),

	// Keys
	DEFINE_EMBEDDED( CPropCrane, m_ServerVehicle ),
	// This is reconstructed in CreateVPhysics
	// DEFINE_EMBEDDED( CPropCrane, m_BoneFollowerManager ),

	DEFINE_FIELD( CPropCrane, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CPropCrane, m_hNPCDriver, FIELD_EHANDLE ),
	DEFINE_FIELD( CPropCrane, m_nNPCButtons, FIELD_INTEGER ),
	DEFINE_FIELD( CPropCrane, m_bLocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropCrane, m_bEnterAnimOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropCrane, m_bExitAnimOn, FIELD_BOOLEAN ),
	DEFINE_OUTPUT( CPropCrane, m_playerOn, "PlayerOn" ),
	DEFINE_OUTPUT( CPropCrane, m_playerOff, "PlayerOff" ),
	DEFINE_FIELD( CPropCrane, m_savedViewOffset, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CPropCrane, m_iTurning, FIELD_INTEGER ),
	DEFINE_FIELD( CPropCrane, m_bStartSoundAtCrossover, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropCrane, m_flTurn, FIELD_FLOAT ),
	DEFINE_FIELD( CPropCrane, m_flTurnStartTime, FIELD_TIME ),
	DEFINE_FIELD( CPropCrane, m_flTurnActiveTime, FIELD_TIME ),
	DEFINE_FIELD( CPropCrane, m_bExtending, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropCrane, m_flExtension, FIELD_FLOAT ),
	DEFINE_FIELD( CPropCrane, m_flExtensionRate, FIELD_FLOAT ),
	DEFINE_FIELD( CPropCrane, m_bDropping, FIELD_BOOLEAN ),
	//DEFINE_FIELD( CPropCrane, m_flNextDangerSoundTime, FIELD_TIME ),
	//DEFINE_FIELD( CPropCrane, m_flNextCreakSound, FIELD_TIME ),
	DEFINE_FIELD( CPropCrane, m_flNextDropAllowedTime, FIELD_TIME ),
	DEFINE_FIELD( CPropCrane, m_flSlowRaiseTime, FIELD_TIME ),
	DEFINE_FIELD( CPropCrane, m_flMaxExtensionSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CPropCrane, m_flMaxTurnSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CPropCrane, m_flExtensionAccel, FIELD_FLOAT ),
	DEFINE_FIELD( CPropCrane, m_flExtensionDecel, FIELD_FLOAT ),
	DEFINE_FIELD( CPropCrane, m_flTurnAccel, FIELD_FLOAT ),
	DEFINE_FIELD( CPropCrane, m_flTurnDecel, FIELD_FLOAT ),
	DEFINE_KEYFIELD( CPropCrane, m_iszMagnetName, FIELD_STRING, "magnetname" ),
	DEFINE_FIELD( CPropCrane, m_hCraneMagnet, FIELD_EHANDLE ),
	DEFINE_FIELD( CPropCrane, m_hCraneTip, FIELD_EHANDLE ),
	DEFINE_FIELD( CPropCrane, m_hRope, FIELD_EHANDLE ),
	DEFINE_PHYSPTR( CPropCrane, m_pConstraintGroup ),
	DEFINE_KEYFIELD( CPropCrane, m_vehicleScript, FIELD_STRING, "vehiclescript" ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CPropCrane, DT_PropCrane)
	SendPropEHandle(SENDINFO(m_hPlayer)),
END_SEND_TABLE();


//------------------------------------------------
// Precache
//------------------------------------------------
void CPropCrane::Precache( void )
{
	BaseClass::Precache();
	m_ServerVehicle.Initialize( STRING(m_vehicleScript) );
}


//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropCrane::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );
	SetCollisionGroup( COLLISION_GROUP_VEHICLE );

	BaseClass::Spawn();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NOCLIP );

	m_takedamage = DAMAGE_EVENTS_ONLY;
	m_flTurn = 0;
	m_flExtension = 0;
	m_flNextDangerSoundTime = 0;
	m_flNextCreakSound = 0;
	m_flNextDropAllowedTime = 0;
	m_flSlowRaiseTime = 0;
	m_bDropping = false;

	InitCraneSpeeds();

	SetPoseParameter( "armextensionpose", m_flExtension );

	CreateVPhysics();
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::OnRestore( void )
{
	BaseClass::OnRestore();

	CreateVPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::Activate( void )
{
	BaseClass::Activate();

	// If we load a game, we don't need to set this all up again.
	if ( m_hCraneMagnet )
		return;

	// Find our magnet
	if ( m_iszMagnetName == NULL_STRING )
	{
		Warning( "prop_vehicle_crane %s has no magnet entity specified!\n", STRING(GetEntityName()) );
		UTIL_Remove( this );
		return;
	}

	m_hCraneMagnet = dynamic_cast<CPhysMagnet *>(gEntList.FindEntityByName( NULL, STRING(m_iszMagnetName), NULL ));
	if ( !m_hCraneMagnet )
	{
		Warning( "prop_vehicle_crane %s failed to find magnet %s.\n", STRING(GetEntityName()), STRING(m_iszMagnetName) );
		UTIL_Remove( this );
		return;
	}

	// We want the magnet to cast a long shadow
	m_hCraneMagnet->SetShadowCastDistance( 2048 );

	// Create our constraint group
	m_pConstraintGroup = physenv->CreateConstraintGroup();
	m_hCraneMagnet->SetConstraintGroup( m_pConstraintGroup );

	// Create our crane tip
	Vector vecOrigin;
	QAngle vecAngles;
	GetCraneTipPosition( &vecOrigin, &vecAngles );
	m_hCraneTip = CCraneTip::Create( m_hCraneMagnet, m_pConstraintGroup, vecOrigin, vecAngles );
	if ( !m_hCraneTip )
	{
		UTIL_Remove( this );
		return;
	}
	m_pConstraintGroup->Activate();

	// Make a rope to connect 'em
	int iIndex = m_hCraneMagnet->LookupAttachment("magnetcable_a");
	m_hRope = CRopeKeyframe::Create( this, m_hCraneMagnet, 1, iIndex );
	if ( m_hRope )
	{
		m_hRope->m_Width = 3;
		m_hRope->m_nSegments = ROPE_MAX_SEGMENTS / 2;
		m_hRope->EnableWind( false );
		m_hRope->SetupHangDistance( 0 );
		m_hRope->m_RopeLength = (m_hCraneMagnet->GetAbsOrigin() - m_hCraneTip->GetAbsOrigin()).Length() * 1.1;
	}

	// Start with the magnet off
	TurnMagnetOff();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropCrane::CreateVPhysics( void )
{
	BaseClass::CreateVPhysics();
	m_BoneFollowerManager.InitBoneFollowers( this, ARRAYSIZE(pCraneFollowerBoneNames), pCraneFollowerBoneNames );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::UpdateOnRemove( void )
{
	m_BoneFollowerManager.DestroyBoneFollowers();
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::InitCraneSpeeds( void )
{
	m_flMaxExtensionSpeed = CRANE_EXTENSION_RATE_MAX;
	m_flMaxTurnSpeed = CRANE_TURN_RATE_MAX;
	m_flExtensionAccel = CRANE_EXTENSION_ACCEL;
	m_flExtensionDecel = CRANE_EXTENSION_DECEL;
	m_flTurnAccel = CRANE_TURN_ACCEL;
	m_flTurnDecel = CRANE_DECEL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
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
int CPropCrane::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	//Do scaled up physics damage to the car
	CTakeDamageInfo info = inputInfo;
	info.ScaleDamage( 25 );

	// reset the damage
	info.SetDamage( inputInfo.GetDamage() );

	//Check to do damage to driver
	if ( m_hPlayer != NULL )
	{
		//Take no damage from physics damages
		if ( info.GetDamageType() & DMG_CRUSH )
			return 0;

		//Take the damage
		m_hPlayer->TakeDamage( info );
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CPropCrane::BodyTarget( const Vector &posSrc, bool bNoisy )
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
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::Think(void)
{
	SetNextThink( gpGlobals->curtime + 0.1 );

	if ( GetDriver() )
	{
		BaseClass::Think();
		
		if ( m_hNPCDriver )
		{
			GetServerVehicle()->NPC_DriveVehicle();
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
	else
	{
		// Run the crane's movement
		RunCraneMovement( 0.1 );
	}

	// Update follower bones
	m_BoneFollowerManager.UpdateBoneFollowers();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//-----------------------------------------------------------------------------
void CPropCrane::ItemPostFrame( CBasePlayer *player )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( !pPlayer )
		return;

	ResetUseKey( pPlayer );

	// Check to see if this vehicle is locked
	if ( CanEnterVehicle(pPlayer) )
	{
		pPlayer->GetInVehicle( GetServerVehicle(), VEHICLE_DRIVER);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to enter / exit the vehicle
//-----------------------------------------------------------------------------
bool CPropCrane::CanEnterVehicle( CBaseEntity *pEntity )
{
	// Prevent entering if the vehicle's being driven by an NPC
	if ( GetDriver() && GetDriver() != pEntity )
		return false;

	// Prevent entering if the vehicle's locked
	return ( !m_bLocked );
}

//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to enter / exit the vehicle
//-----------------------------------------------------------------------------
bool CPropCrane::CanExitVehicle( CBaseEntity *pEntity )
{
	// Prevent exiting if the vehicle's locked, or rotating
	return ( !m_bLocked && (GetLocalAngularVelocity() == vec3_angle) );
}

//-----------------------------------------------------------------------------
// Purpose: Override base class to add display 
//-----------------------------------------------------------------------------
void CPropCrane::DrawDebugGeometryOverlays(void) 
{
	// Draw if BBOX is on
	if ( m_debugOverlays & OVERLAY_BBOX_BIT )
	{
		Vector vecPoint = m_hCraneMagnet->GetAbsOrigin();
		int iIndex = m_hCraneMagnet->LookupAttachment("magnetcable_a");
		if ( iIndex >= 0 )
		{
			QAngle vecAngles;
			m_hCraneMagnet->GetAttachment( iIndex, vecPoint, vecAngles );
		}

		NDebugOverlay::Line( m_hCraneTip->GetAbsOrigin(), vecPoint, 255,255,255, true, 0.1 );
	}

	BaseClass::DrawDebugGeometryOverlays();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::EnterVehicle( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	// Remove any player who may be in the vehicle at the moment
	ExitVehicle(VEHICLE_DRIVER);

	m_hPlayer = pPlayer;
	m_flTurn = 0;
	m_flExtensionRate = 0;
	m_savedViewOffset = pPlayer->GetViewOffset();
	pPlayer->SetViewOffset( vec3_origin );

	m_playerOn.FireOutput( pPlayer, this, 0 );

	m_ServerVehicle.SoundStart();

	// Play the "enter" animation
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::ExitVehicle( int iRole )
{
	CBasePlayer *pPlayer = m_hPlayer;
	if ( !pPlayer )
		return;

	m_hPlayer = NULL;
	ResetUseKey( pPlayer );
	pPlayer->SetViewOffset( m_savedViewOffset );

	m_playerOff.FireOutput( pPlayer, this, 0 );
	m_bEnterAnimOn = false;

	m_ServerVehicle.SoundShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::ResetUseKey( CBasePlayer *pPlayer )
{
	pPlayer->m_afButtonPressed &= ~IN_USE;
}

//-----------------------------------------------------------------------------
// Purpose: Pass player movement into the crane's driving system
//-----------------------------------------------------------------------------
void CPropCrane::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	DriveCrane( ucmd->buttons, player->m_afButtonPressed );

	// Run the crane's movement
	RunCraneMovement( gpGlobals->frametime );
}

//-----------------------------------------------------------------------------
// Purpose: Crane rotates around with +left and +right, and extends/retracts 
//			the cable with +forward and +back.
//-----------------------------------------------------------------------------
void CPropCrane::DriveCrane( int iDriverButtons, int iButtonsPressed, float flNPCSteering )
{
	float flPrevTurn = m_flTurn;
	int iPrevTurning = m_iTurning;
	bool bPlayStartup = false;
	bool bWasExtending = m_bExtending;

	// Handle rotation of the crane
	if ( iDriverButtons & IN_MOVELEFT )
	{
		// NPCs may cheat and set the steering
		if ( flNPCSteering )
		{
			m_flTurn = flNPCSteering;
		}
		else
		{
			// Try adding some randomness to make it feel shaky? 
			float flTurnAdd = m_flTurnAccel;
			// If we're turning back on ourselves, use decel speed
			if ( m_flTurn < 0 )
			{
				flTurnAdd = max( flTurnAdd, m_flTurnDecel );
			}

			m_flTurn = UTIL_Approach( m_flMaxTurnSpeed, m_flTurn, flTurnAdd * gpGlobals->frametime );
		}
		m_iTurning = TURNING_LEFT;
	}
	else if ( iDriverButtons & IN_MOVERIGHT )
	{
		// NPCs may cheat and set the steering
		if ( flNPCSteering )
		{
			m_flTurn = flNPCSteering;
		}
		else
		{
			// Try adding some randomness to make it feel shaky?
			float flTurnAdd = m_flTurnAccel;
			// If we're turning back on ourselves, increase the rate
			if ( m_flTurn > 0 )
			{
				flTurnAdd = max( flTurnAdd, m_flTurnDecel );
			}
			m_flTurn = UTIL_Approach( -m_flMaxTurnSpeed, m_flTurn, flTurnAdd * gpGlobals->frametime );
		}
		m_iTurning = TURNING_RIGHT;
	}
	else
	{
		m_flTurn = UTIL_Approach( 0, m_flTurn, m_flTurnDecel * gpGlobals->frametime );
		m_iTurning = TURNING_NOT;
	}

	SetLocalAngularVelocity( QAngle(0,m_flTurn * 10,0) );

	// Handle extension / retraction of the arm
	if ( iDriverButtons & IN_FORWARD )
	{
		m_flExtensionRate = UTIL_Approach( m_flMaxExtensionSpeed, m_flExtensionRate, m_flExtensionAccel * gpGlobals->frametime );
		m_bExtending = true;
	}
	else if ( iDriverButtons & IN_BACK )
	{
		m_flExtensionRate = UTIL_Approach( -m_flMaxExtensionSpeed, m_flExtensionRate, m_flExtensionAccel * gpGlobals->frametime );
		m_bExtending = true;
	}
	else
	{
		m_flExtensionRate = UTIL_Approach( 0, m_flExtensionRate, m_flExtensionDecel * gpGlobals->frametime );
		m_bExtending = false;
	}

	//Msg("Turn: %f\nExtensionRate: %f\n", m_flTurn, m_flExtensionRate );

	//If we're holding down an attack button, update our state
	if ( iButtonsPressed & (IN_ATTACK | IN_ATTACK2) )
	{
		// If we have something on the magnet, turn the magnet off
		if ( m_hCraneMagnet->GetTotalMassAttachedObjects() )
		{
			TurnMagnetOff();
		}
		else if ( !m_bDropping && m_flNextDropAllowedTime < gpGlobals->curtime )
		{
			TurnMagnetOn();

			// Drop the magnet till it hits something
			m_bDropping = true;
			m_hCraneMagnet->ResetHasHitSomething();
			m_hCraneTip->m_pSpring->SetSpringConstant( CRANE_SPRING_CONSTANT_LOWERING );

			m_ServerVehicle.PlaySound( VS_MISC1 );
		}
	}

	// All the following jiggery-pokery is designed to play the engine revving sounds correctly.
	// Start the rev-up sound whenever the player depresses a turn key, but not if the player's
	// pushed the turn key to one side while the crane's still rotating the other way. In that
	// case, play the rev-down sound, until the crane crosses over and starts turning the desired
	// way, at which point the rev-up sound starts.

	// Spot the points at which we cross the turning point
	if ( m_iTurning != iPrevTurning )
	{
		// Swapping sides?
		if ( (flPrevTurn >= 0 && m_flTurn < 0) || (flPrevTurn <= 0 && m_flTurn > 0 ) )
		{
			bPlayStartup = true;
		}
		else 
		{
			// Repressed same direction
			if ( m_iTurning == TURNING_LEFT && m_flTurn < 0 )
			{
				bPlayStartup = true;
			}
			else if ( m_iTurning == TURNING_RIGHT && m_flTurn > 0 )
			{
				bPlayStartup = true;
			}
		}
	}

	// Did we start turning this frame?
	if ( bPlayStartup )
	{
		m_flTurnStartTime = gpGlobals->curtime;

		m_ServerVehicle.StopSound( VS_ENGINE_IDLE );
		m_ServerVehicle.StopSoundForGear( VS_FOOT_OFF_GAS );
		m_ServerVehicle.StartGearEngineLoop( VS_ENGINE_LOOP );
	}
	else if ( !m_iTurning && iPrevTurning )
	{
		m_flTurnActiveTime = gpGlobals->curtime - m_flTurnStartTime;

		// Play the throttle off sound if we were accelerating for a bit
		if ( m_flTurnActiveTime > 1.0 )
		{
			m_ServerVehicle.PlaySoundForGear( VS_FOOT_OFF_GAS );
		}
		else 
		{
			// throttle not down long enough, stop throttle sound
			m_ServerVehicle.PlaySound( VS_ENGINE_IDLE );
		}
	}
	else if ( m_iTurning && !iPrevTurning )
	{
		m_ServerVehicle.StopSound( VS_ENGINE_IDLE );
		m_ServerVehicle.StopSoundForGear( VS_FOOT_OFF_GAS );
	}

	float flSpeedPercentage = clamp( fabs(m_flTurn) / m_flMaxTurnSpeed, 0, 1 );
	m_ServerVehicle.SoundUpdate( gpGlobals->frametime, flSpeedPercentage, (m_iTurning != TURNING_NOT), false );

	// Play sounds for arm extension / retraction
	if ( m_bExtending && !bWasExtending )
	{
		m_ServerVehicle.StopSound( VS_ENGINE2_STOP );
		m_ServerVehicle.PlaySound( VS_ENGINE2_START );
	}
	else if ( !m_bExtending && bWasExtending )
	{
		m_ServerVehicle.StopSound( VS_ENGINE2_START );
		m_ServerVehicle.PlaySound( VS_ENGINE2_STOP );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::RecalculateCraneTip( void )
{
	Vector vecOrigin;
	QAngle vecAngles;
	GetCraneTipPosition( &vecOrigin, &vecAngles );
	m_hCraneTip->SetAbsOrigin( vecOrigin );

	// NOTE: We need to do this because we're not using Physics...
	if ( m_hCraneTip->VPhysicsGetObject() )
	{
		m_hCraneTip->VPhysicsGetObject()->UpdateShadow( vecOrigin, vec3_angle, true, gpGlobals->frametime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pMoveData - 
//-----------------------------------------------------------------------------
void CPropCrane::RunCraneMovement( float flTime )
{
	if ( m_flExtensionRate )
	{
		// Extend / Retract the crane
		m_flExtension = clamp( m_flExtension + (m_flExtensionRate * 10 * flTime), 0, 1 );
		SetPoseParameter( "armextensionpose", m_flExtension );
		StudioFrameAdvance();
	}

	// Drop the magnet until it hits the ground
	if ( m_bDropping )
	{
		// Drop until the magnet hits something 
		if ( m_hCraneMagnet->HasHitSomething() )
		{
			// We hit the ground, stop dropping
			m_hCraneTip->m_pSpring->SetSpringConstant( CRANE_SPRING_CONSTANT_INITIAL_RAISING );
			m_bDropping = false;
			m_flNextDropAllowedTime = gpGlobals->curtime + 3.0;
			m_flSlowRaiseTime = gpGlobals->curtime;

			m_ServerVehicle.PlaySound( VS_MISC2 );
		}
	}
	else if ( (m_flSlowRaiseTime + CRANE_SLOWRAISE_TIME) > gpGlobals->curtime )
	{
		float flDelta = (gpGlobals->curtime - m_flSlowRaiseTime);

		flDelta = clamp( flDelta, 0, CRANE_SLOWRAISE_TIME );
		float flCurrentSpringConstant = RemapVal( flDelta, 0, CRANE_SLOWRAISE_TIME, CRANE_SPRING_CONSTANT_INITIAL_RAISING, CRANE_SPRING_CONSTANT_HANGING );
		m_hCraneTip->m_pSpring->SetSpringConstant( flCurrentSpringConstant );
	}

	// If we've moved in any way, update the tip
	if ( m_bDropping || m_flExtensionRate || GetLocalAngularVelocity() != vec3_angle )
	{
		RecalculateCraneTip();
	}

	// Make danger sounds underneath the magnet if we have something attached to it
	if ( (m_flNextDangerSoundTime < gpGlobals->curtime) && (m_hCraneMagnet->GetTotalMassAttachedObjects() > 0) )
	{
		// Trace down from the magnet and make a danger sound on the ground
		trace_t tr;
		Vector vecSource = m_hCraneMagnet->GetAbsOrigin();
		UTIL_TraceLine( vecSource, vecSource - Vector(0,0,2048), MASK_SOLID_BRUSHONLY, m_hCraneMagnet, 0, &tr );

		if ( tr.fraction < 1.0 )
		{
			// Make the volume proportional to the amount of mass on the magnet
			float flVolume = clamp( (m_hCraneMagnet->GetTotalMassAttachedObjects() * 0.5), 100.f, 600.f );
			CSoundEnt::InsertSound( SOUND_DANGER, tr.endpos, flVolume, 0.2, this );
			//CSoundEnt::InsertSound( SOUND_PHYSICS_DANGER, tr.endpos, flVolume, 0.2, this, 1 );

			/*
			Msg("Total: %.2f Volume: %.2f\n", m_hCraneMagnet->GetTotalMassAttachedObjects(), flVolume );
			Vector vecVolume = Vector(flVolume,flVolume,flVolume) * 0.5;
			NDebugOverlay::Box( tr.endpos, -vecVolume, vecVolume, 255,0,0, false, 0.3 );
			NDebugOverlay::Cross3D( tr.endpos, -Vector(10,10,10), Vector(10,10,10), 255,0,0, false, 0.3 );
			*/
		}

		m_flNextDangerSoundTime = gpGlobals->curtime + 0.3;
	}

	// Play creak sounds on the magnet if there's heavy weight on it
	if ( (m_flNextCreakSound < gpGlobals->curtime) && (m_hCraneMagnet->GetTotalMassAttachedObjects() > 100) )
	{
		// Randomly play creaks from the magnet, and increase the chance based on the turning speed
		float flSpeedPercentage = clamp( fabs(m_flTurn) / m_flMaxTurnSpeed, 0, 1 );
		if ( RandomFloat(0,1) > (0.95 - (0.1 * flSpeedPercentage)) )
		{
			if ( m_ServerVehicle.m_vehicleSounds.iszSound[VS_MISC4] != NULL_STRING )
			{
				CPASAttenuationFilter filter( m_hCraneMagnet );
				CBaseEntity::EmitSound( filter, m_hCraneMagnet->entindex(), CHAN_VOICE, STRING(m_ServerVehicle.m_vehicleSounds.iszSound[VS_MISC4]), 1.0, ATTN_NORM, 0, 100 );
			}
			m_flNextCreakSound = gpGlobals->curtime + 5.0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::TurnMagnetOn( void )
{
	if ( !m_hCraneMagnet->IsOn() )
	{
		variant_t emptyVariant;
		m_hCraneMagnet->AcceptInput( "Toggle", this, this, emptyVariant, USE_TOGGLE );
		m_ServerVehicle.PlaySound( VS_MISC3 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::TurnMagnetOff( void )
{
	if ( m_hCraneMagnet->IsOn() )
	{
		variant_t emptyVariant;
		m_hCraneMagnet->AcceptInput( "Toggle", this, this, emptyVariant, USE_TOGGLE );
		m_ServerVehicle.PlaySound( VS_MISC3 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const Vector &CPropCrane::GetCraneTipPosition( void )
{
	return m_hCraneTip->GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: Fills out the values with the desired position of the crane's tip
//-----------------------------------------------------------------------------
void CPropCrane::GetCraneTipPosition( Vector *vecOrigin, QAngle *vecAngles )
{
	GetAttachment( "cable_tip", *vecOrigin, *vecAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Vehicles are permanently oriented off angle for vphysics.
//-----------------------------------------------------------------------------
void CPropCrane::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
{
	// This call is necessary to cause m_rgflCoordinateFrame to be recomputed
	const matrix3x4_t &entityToWorld = EntityToWorldTransform();

	if (pForward != NULL)
	{
		MatrixGetColumn( entityToWorld, 1, *pForward ); 
	}

	if (pRight != NULL)
	{
		MatrixGetColumn( entityToWorld, 0, *pRight ); 
	}

	if (pUp != NULL)
	{
		MatrixGetColumn( entityToWorld, 2, *pUp ); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CPropCrane::GetDriver( void ) 
{ 
	if ( m_hNPCDriver ) 
		return m_hNPCDriver; 

	return m_hPlayer; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPropCrane::PlayExitAnimation( void )
{
	// Look for the "exit" sequence
	int iSequence = LookupSequence( "exit" );

	// Skip if no animation present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		m_flCycle = 0;
		m_flAnimTime = gpGlobals->curtime;
		ResetSequence( iSequence );
		ResetClientsideFrame();
		m_bExitAnimOn = true;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Prevent the player from entering / exiting the vehicle
//-----------------------------------------------------------------------------
void CPropCrane::InputLock( inputdata_t &inputdata )
{
	m_bLocked = true;
}

//-----------------------------------------------------------------------------
// Purpose: Allow the player to enter / exit the vehicle
//-----------------------------------------------------------------------------
void CPropCrane::InputUnlock( inputdata_t &inputdata )
{
	m_bLocked = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropCrane::SetNPCDriver( CNPC_VehicleDriver *pDriver )
{
	m_hNPCDriver = pDriver;
	m_nNPCButtons = 0;

	if ( pDriver )
	{
		m_flMaxExtensionSpeed = CRANE_EXTENSION_RATE_MAX * 1.5;
		m_flMaxTurnSpeed = CRANE_TURN_RATE_MAX * 1.5;
		m_flExtensionAccel = CRANE_EXTENSION_ACCEL * 2;
		m_flExtensionDecel = CRANE_EXTENSION_DECEL * 20; // Npcs stop quickly to make them more accurate
		m_flTurnAccel = CRANE_TURN_ACCEL * 2;
		m_flTurnDecel = CRANE_DECEL * 10;	// Npcs stop quickly to make them more accurate

		// Set our owner entity to be the NPC, so it can path check without hitting us
		SetOwnerEntity( pDriver );
	}
	else
	{
		// Restore player crane speeds
		InitCraneSpeeds();
		SetOwnerEntity( NULL );

		// Shutdown the crane's sounds
		m_ServerVehicle.SoundShutdown( 1.0 );
	}
}

//========================================================================================================================================
// CRANE VEHICLE SERVER VEHICLE
//========================================================================================================================================
CPropCrane *CCraneServerVehicle::GetCrane( void )
{
	return (CPropCrane*)GetDrivableVehicle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraneServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	Assert( nRole == VEHICLE_DRIVER );
	CBasePlayer *pPlayer = ToBasePlayer( GetDrivableVehicle()->GetDriver() );
	Assert( pPlayer );

	*pAbsAngles = pPlayer->EyeAngles(); // yuck. this is an in/out parameter.

	float flPitchFactor = 1.0;
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetCrane()->GetAttachment( "vehicle_driver_eyes", vehicleEyeOrigin, vehicleEyeAngles );
	AngleMatrix( vehicleEyeAngles, vehicleEyePosToWorld );

	// Compute the relative rotation between the unperterbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix( *pAbsAngles, cameraToWorld );

	matrix3x4_t worldToEyePos;
	MatrixInvert( vehicleEyePosToWorld, worldToEyePos );

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms( worldToEyePos, cameraToWorld, vehicleCameraToEyePos );

	// Now perterb the attachment point
	vehicleEyeAngles.x = RemapAngleRange( PITCH_CURVE_ZERO * flPitchFactor, PITCH_CURVE_LINEAR, vehicleEyeAngles.x );
	vehicleEyeAngles.z = RemapAngleRange( ROLL_CURVE_ZERO * flPitchFactor, ROLL_CURVE_LINEAR, vehicleEyeAngles.z );
	AngleMatrix( vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld );

	// Now treat the relative eye angles as being relative to this new, perterbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms( vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld );

	// output new view abs angles
	MatrixAngles( newCameraToWorld, *pAbsAngles );

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn( newCameraToWorld, 3, *pAbsOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraneServerVehicle::NPC_SetDriver( CNPC_VehicleDriver *pDriver )
{
	GetCrane()->SetNPCDriver( pDriver );

	if ( pDriver )
	{
		SetVehicleVolume( 1.0 );	// Vehicles driven by NPCs are louder
		GetCrane()->SetSimulatedEveryTick( false );
	}
	else
	{
		SetVehicleVolume( 0.5 );
		GetCrane()->SetSimulatedEveryTick( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraneServerVehicle::NPC_DriveVehicle( void )
{
	if ( g_debug_vehicledriver.GetInt() )
	{
		if ( m_nNPCButtons )
		{
			Vector vecForward, vecRight;
			GetCrane()->GetVectors( &vecForward, &vecRight, NULL );
			if ( m_nNPCButtons & IN_FORWARD )
			{
				NDebugOverlay::Line( GetCrane()->GetAbsOrigin(), GetCrane()->GetAbsOrigin() + vecForward * 200, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_BACK )
			{
				NDebugOverlay::Line( GetCrane()->GetAbsOrigin(), GetCrane()->GetAbsOrigin() - vecForward * 200, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_MOVELEFT )
			{
				NDebugOverlay::Line( GetCrane()->GetAbsOrigin(), GetCrane()->GetAbsOrigin() - vecRight * 200, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_MOVERIGHT )
			{
				NDebugOverlay::Line( GetCrane()->GetAbsOrigin(), GetCrane()->GetAbsOrigin() + vecRight * 200, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_JUMP )
			{
				NDebugOverlay::Box( GetCrane()->GetAbsOrigin(), -Vector(20,20,20), Vector(20,20,20), 0,255,0, true, 0.1 );
			}
		}
	}

	GetCrane()->DriveCrane( m_nNPCButtons, m_nNPCButtons, m_flTurnDegrees );

	// Clear out attack buttons each frame
	m_nNPCButtons &= ~IN_ATTACK;
	m_nNPCButtons &= ~IN_ATTACK2;

	// Run the crane's movement
	GetCrane()->RunCraneMovement( 0.1 );
}

//===============================================================================================================================
// CRANE CABLE TIP
//===============================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: To by usable by the constraint system, this needs to have a phys model.
//-----------------------------------------------------------------------------
void CCraneTip::Spawn( void )
{
	Precache();
	SetModel( "models/props_junk/cardboard_box001a.mdl" );
	m_fEffects |= EF_NODRAW;

	// We don't want this to be solid, because we don't want it to collide with the hydra.
	SetSolid( SOLID_VPHYSICS );
	AddSolidFlags( FSOLID_NOT_SOLID );
	VPhysicsInitShadow( false, false );

	// Disable movement on this sucker, we're going to move him manually
	SetMoveType( MOVETYPE_NONE );
	
	BaseClass::Spawn();

	m_pSpring = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCraneTip::Precache( void )
{
	PrecacheModel( "models/props_junk/cardboard_box001a.mdl" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
bool CCraneTip::CreateConstraint( CBaseAnimating *pCraneMagnet, IPhysicsConstraintGroup *pGroup )
{
	IPhysicsObject *pPhysObject = VPhysicsGetObject();
	IPhysicsObject *pCraneMagnetPhysObject = pCraneMagnet->VPhysicsGetObject();
	if ( !pCraneMagnetPhysObject )
	{
		Msg(" Error: Tried to create a crane_tip with a crane magnet that has no physics model.\n" );
		return false;
	}
	Assert( pPhysObject );

	// Check to see if it's got an attachment point to connect to
	Vector vecPoint = pCraneMagnet->GetAbsOrigin();
	int iIndex = pCraneMagnet->LookupAttachment("magnetcable_a");
	if ( iIndex >= 0 )
	{
		QAngle vecAngles;
		pCraneMagnet->GetAttachment( iIndex, vecPoint, vecAngles );
	}

	// Create our spring
	/*
	constraint_lengthparams_t length;
	length.Defaults();
	length.InitWorldspace( pPhysObject, pCraneMagnetPhysObject, GetAbsOrigin(), vecPoint );
	length.constraint.Defaults();
	m_pConstraint = physenv->CreateLengthConstraint( pPhysObject, pCraneMagnetPhysObject, pGroup, length );
	*/

	springparams_t spring;
	spring.constant = CRANE_SPRING_CONSTANT_HANGING;
	spring.damping = CRANE_SPRING_DAMPING;
	spring.naturalLength = (GetAbsOrigin() - vecPoint).Length();
	spring.relativeDamping = CRANE_SPRING_RELATIVE_DAMPING;
	spring.startPosition = GetAbsOrigin();
	spring.endPosition = vecPoint;
	spring.useLocalPositions = false;
	spring.onlyStretch = false;
	m_pSpring = physenv->CreateSpring( pPhysObject, pCraneMagnetPhysObject, &spring );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Create a Hydra Impale between the hydra and the entity passed in
//-----------------------------------------------------------------------------
CCraneTip *CCraneTip::Create( CBaseAnimating *pCraneMagnet, IPhysicsConstraintGroup *pGroup, const Vector &vecOrigin, const QAngle &vecAngles )
{
	CCraneTip *pCraneTip = (CCraneTip *)CBaseEntity::Create( "crane_tip", vecOrigin, vecAngles );
	if ( !pCraneTip )
		return NULL;

	if ( !pCraneTip->CreateConstraint( pCraneMagnet, pGroup ) )
		return NULL;

	return pCraneTip;
}

