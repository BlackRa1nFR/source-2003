//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: UNDONE: Rename this to prop_vehicle.cpp !!!
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "vcollide_parse.h"
#include "vehicle_base.h"
#include "ndebugoverlay.h"
#include "igamemovement.h"
#include "soundenvelope.h"
#include "in_buttons.h"
#include "npc_vehicledriver.h"
#include "physics_saverestore.h"
#include "func_break.h"
#include "physics_impact_damage.h"

ConVar g_debug_vehiclebase( "g_debug_vehiclebase", "0", FCVAR_CHEAT );
extern ConVar g_debug_vehicledriver;

BEGIN_DATADESC( CPropVehicle )

	DEFINE_EMBEDDED( CPropVehicle, m_VehiclePhysics ),

	// These are necessary to save here because the 'owner' of these fields must be the prop_vehicle
	DEFINE_PHYSPTR( CPropVehicle, m_VehiclePhysics.m_pVehicle ),
	DEFINE_PHYSPTR_ARRAY( CPropVehicle, m_VehiclePhysics.m_pWheels ),

	DEFINE_FIELD( CPropVehicle, m_nVehicleType, FIELD_INTEGER ),

	// Keys
	DEFINE_KEYFIELD( CPropVehicle, m_vehicleScript, FIELD_STRING, "VehicleScript" ),
	DEFINE_FIELD( CPropVehicle, m_vecSmoothedVelocity, FIELD_VECTOR ),

	// Inputs
	DEFINE_INPUTFUNC( CPropVehicle, FIELD_FLOAT, "Throttle", InputThrottle ),
	DEFINE_INPUTFUNC( CPropVehicle, FIELD_FLOAT, "Steer", InputSteering ),
	DEFINE_INPUTFUNC( CPropVehicle, FIELD_FLOAT, "Action", InputAction ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( prop_vehicle, CPropVehicle );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#pragma warning (disable:4355)
CPropVehicle::CPropVehicle() : m_VehiclePhysics( this )
{
	SetVehicleType( VEHICLE_TYPE_CAR_WHEELS );
}
#pragma warning (default:4355)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropVehicle::~CPropVehicle ()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::Spawn( )
{
	CFourWheelServerVehicle *pServerVehicle = dynamic_cast<CFourWheelServerVehicle*>(GetServerVehicle());
	m_VehiclePhysics.SetOuter( this, pServerVehicle );

	// NOTE: The model has to be set before we can spawn vehicle physics
	BaseClass::Spawn();
	SetCollisionGroup( COLLISION_GROUP_VEHICLE );

	m_VehiclePhysics.Spawn();
	if (!m_VehiclePhysics.Initialize( STRING(m_vehicleScript), m_nVehicleType ))
		return;
	SetNextThink( gpGlobals->curtime );

	m_vecSmoothedVelocity.Init();
}


//-----------------------------------------------------------------------------
// Purpose: Restore
//-----------------------------------------------------------------------------
int CPropVehicle::Restore( IRestore &restore )
{
	CFourWheelServerVehicle *pServerVehicle = dynamic_cast<CFourWheelServerVehicle*>(GetServerVehicle());
	m_VehiclePhysics.SetOuter( this, pServerVehicle );
	return BaseClass::Restore( restore );
}


//-----------------------------------------------------------------------------
// Purpose: Tell the vehicle physics system whenever we teleport, so it can fixup the wheels.
//-----------------------------------------------------------------------------
void CPropVehicle::Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	matrix3x4_t startMatrixInv;

	MatrixInvert( EntityToWorldTransform(), startMatrixInv );
	BaseClass::Teleport( newPosition, newAngles, newVelocity );

	// Calculate the relative transform of the teleport
	matrix3x4_t xform;
	ConcatTransforms( EntityToWorldTransform(), startMatrixInv, xform );
	m_VehiclePhysics.Teleport( xform );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::DrawDebugGeometryOverlays()
{
	if (m_debugOverlays & OVERLAY_BBOX_BIT) 
	{	
		m_VehiclePhysics.DrawDebugGeometryOverlays();
	}
	BaseClass::DrawDebugGeometryOverlays();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropVehicle::DrawDebugTextOverlays()
{
	int nOffset = BaseClass::DrawDebugTextOverlays();
	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		nOffset = m_VehiclePhysics.DrawDebugTextOverlays( nOffset );
	}
	return nOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::InputThrottle( inputdata_t &inputdata )
{
	m_VehiclePhysics.SetThrottle( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::InputSteering( inputdata_t &inputdata )
{
	m_VehiclePhysics.SetSteering( inputdata.value.Float(), 2*gpGlobals->frametime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::InputAction( inputdata_t &inputdata )
{
	m_VehiclePhysics.SetAction( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::Think()
{
	m_VehiclePhysics.Think();
}

#define SMOOTHING_FACTOR 0.9

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	Vector	velocity;
	VPhysicsGetObject()->GetVelocity( &velocity, NULL );

	//Update our smoothed velocity
	m_vecSmoothedVelocity = m_vecSmoothedVelocity * SMOOTHING_FACTOR + velocity * ( 1 - SMOOTHING_FACTOR );

	// must be a wheel
	if (!m_VehiclePhysics.VPhysicsUpdate( pPhysics ))
		return;
	
	BaseClass::VPhysicsUpdate( pPhysics );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const Vector
//-----------------------------------------------------------------------------
Vector CPropVehicle::GetSmoothedVelocity( void )
{
	return m_vecSmoothedVelocity;
}

//-----------------------------------------------------------------------------
// Purpose: Player driveable vehicle class
//-----------------------------------------------------------------------------

IMPLEMENT_SERVERCLASS_ST(CPropVehicleDriveable, DT_PropVehicleDriveable)

	SendPropEHandle(SENDINFO(m_hPlayer)),
//	SendPropFloat(SENDINFO_DT_NAME(m_controls.throttle, m_throttle), 8,	SPROP_ROUNDUP,	0.0f,	1.0f),
	SendPropInt(SENDINFO(m_nSpeed),	8),
	SendPropInt(SENDINFO(m_nRPM), 13),
	SendPropFloat(SENDINFO(m_flThrottle), 0, SPROP_NOSCALE ),
	SendPropInt(SENDINFO(m_nBoostTimeLeft), 8),
	SendPropInt(SENDINFO(m_nHasBoost), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nScannerDisabledWeapons), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nScannerDisabledVehicle), 1, SPROP_UNSIGNED),
	SendPropVector(SENDINFO(m_vecLookCrosshair), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vecGunCrosshair), -1, SPROP_COORD),
	SendPropInt(SENDINFO(m_bEnterAnimOn), 1, SPROP_UNSIGNED ),
	SendPropInt(SENDINFO(m_bExitAnimOn), 1, SPROP_UNSIGNED ),

END_SEND_TABLE();


BEGIN_DATADESC( CPropVehicleDriveable )
	// Inputs
	DEFINE_INPUTFUNC( CPropVehicleDriveable, FIELD_VOID, "Lock",	InputLock ),
	DEFINE_INPUTFUNC( CPropVehicleDriveable, FIELD_VOID, "Unlock",	InputUnlock ),
	DEFINE_INPUTFUNC( CPropVehicleDriveable, FIELD_VOID, "TurnOn",	InputTurnOn ),
	DEFINE_INPUTFUNC( CPropVehicleDriveable, FIELD_VOID, "TurnOff", InputTurnOff ),

	// Outputs
	DEFINE_OUTPUT( CPropVehicleDriveable, m_playerOn, "PlayerOn" ),
	DEFINE_OUTPUT( CPropVehicleDriveable, m_playerOff, "PlayerOff" ),
	DEFINE_OUTPUT( CPropVehicleDriveable, m_pressedAttack, "PressedAttack" ),
	DEFINE_OUTPUT( CPropVehicleDriveable, m_pressedAttack2, "PressedAttack2" ),
	DEFINE_OUTPUT( CPropVehicleDriveable, m_attackaxis, "AttackAxis" ),
	DEFINE_OUTPUT( CPropVehicleDriveable, m_attack2axis, "Attack2Axis" ),
	DEFINE_FIELD( CPropVehicleDriveable, m_hPlayer, FIELD_EHANDLE ),

	DEFINE_EMBEDDEDBYREF( CPropVehicleDriveable, m_pServerVehicle ),
	DEFINE_FIELD( CPropVehicleDriveable, m_nSpeed, FIELD_INTEGER ),
	DEFINE_FIELD( CPropVehicleDriveable, m_nRPM, FIELD_INTEGER ),
	DEFINE_FIELD( CPropVehicleDriveable, m_flThrottle, FIELD_FLOAT ),
	DEFINE_FIELD( CPropVehicleDriveable, m_nBoostTimeLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CPropVehicleDriveable, m_nHasBoost, FIELD_INTEGER ),
	DEFINE_FIELD( CPropVehicleDriveable, m_vecLookCrosshair, FIELD_VECTOR ),
	DEFINE_FIELD( CPropVehicleDriveable, m_vecGunCrosshair, FIELD_VECTOR ),
	DEFINE_FIELD( CPropVehicleDriveable, m_nScannerDisabledWeapons, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropVehicleDriveable, m_nScannerDisabledVehicle, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropVehicleDriveable, m_savedViewOffset, FIELD_VECTOR ),

	DEFINE_FIELD( CPropVehicleDriveable, m_bLocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropVehicleDriveable, m_flMinimumSpeedToEnterExit, FIELD_FLOAT ),
	DEFINE_FIELD( CPropVehicleDriveable, m_bEnterAnimOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropVehicleDriveable, m_bExitAnimOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPropVehicleDriveable, m_flTurnOffKeepUpright, FIELD_FLOAT ),

	DEFINE_FIELD( CPropVehicleDriveable, m_hNPCDriver, FIELD_EHANDLE ),
	DEFINE_FIELD( CPropVehicleDriveable, m_hKeepUpright, FIELD_EHANDLE ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( prop_vehicle_driveable, CPropVehicleDriveable );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropVehicleDriveable::CPropVehicleDriveable( void )
{
	m_pServerVehicle = NULL;
	m_hKeepUpright = NULL;
	m_flTurnOffKeepUpright = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropVehicleDriveable::~CPropVehicleDriveable( void )
{
	DestroyServerVehicle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::CreateServerVehicle( void )
{
	// Create our server vehicle
	m_pServerVehicle = new CFourWheelServerVehicle();
	m_pServerVehicle->SetVehicle( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::DestroyServerVehicle()
{
	if ( m_pServerVehicle )
	{
		delete m_pServerVehicle;
		m_pServerVehicle = NULL;
	}
}

//------------------------------------------------
// Precache
//------------------------------------------------
void CPropVehicleDriveable::Precache( void )
{
	BaseClass::Precache();
	m_pServerVehicle->Precache( );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::Spawn( void )
{
	// Has to be created before Spawn is called (since that causes Precache to be called)
	DestroyServerVehicle();
	CreateServerVehicle();
	m_pServerVehicle->Initialize( STRING(m_vehicleScript) );

	BaseClass::Spawn();

	m_flMinimumSpeedToEnterExit = 0;
	m_takedamage = DAMAGE_EVENTS_ONLY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropVehicleDriveable::Restore( IRestore &restore )
{
	// Has to be created before	we can restore
	// and we can't create it in the constructor because it could be
	// overridden by a derived class.
	DestroyServerVehicle();
	CreateServerVehicle();
	return BaseClass::Restore( restore );
}


//-----------------------------------------------------------------------------
// Purpose: Vehicles are permanently oriented off angle for vphysics.
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
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
// Purpose: AngleVectors equivalent that accounts for the hacked 90 degree rotation of vehicles
//			BUGBUG: VPhysics is hardcoded so that vehicles must face down Y instead of X like everything else
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::VehicleAngleVectors( const QAngle &angles, Vector *pForward, Vector *pRight, Vector *pUp )
{
	AngleVectors( angles, pRight, pForward, pUp );
	if ( pForward )
	{
	  	*pForward *= -1;
	}
}
  

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( !pPlayer )
		return;

	ResetUseKey( pPlayer );

	// Find out which hitbox the player's eyepoint is within
	int iEntryAnim = m_pServerVehicle->GetEntryAnimForPoint( pPlayer->EyePosition() );

	// Are we in an entrypoint zone? 
	if ( iEntryAnim != ACTIVITY_NOT_AVAILABLE )
	{
		// Check to see if this vehicle can be controlled or if it's locked
		if ( CanControlVehicle() && CanEnterVehicle(pPlayer) )
		{
			pPlayer->GetInVehicle( GetServerVehicle(), VEHICLE_DRIVER);

			// Setup the "enter" vehicle sequence and skip the animation if it isn't present.
			m_flCycle = 0;
			m_flAnimTime = gpGlobals->curtime;
			ResetSequence( iEntryAnim );
			ResetClientsideFrame();
			m_bEnterAnimOn = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CPropVehicleDriveable::GetDriver( void ) 
{ 
	if ( m_hNPCDriver ) 
		return m_hNPCDriver; 

	return m_hPlayer; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::EnterVehicle( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	// Remove any player who may be in the vehicle at the moment
	ExitVehicle(VEHICLE_DRIVER);

	m_VehiclePhysics.ReleaseHandbrake();
	m_hPlayer = pPlayer;
	m_savedViewOffset = pPlayer->GetViewOffset();
	pPlayer->SetViewOffset( vec3_origin );

	m_playerOn.FireOutput( pPlayer, this, 0 );
	StartEngine();

	// Start Thinking
	SetNextThink( gpGlobals->curtime );

	m_VehiclePhysics.GetVehicle()->OnVehicleEnter();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::ExitVehicle( int iRole )
{
	CBasePlayer *pPlayer = m_hPlayer;
	if ( !pPlayer )
		return;

	m_hPlayer = NULL;
	ResetUseKey( pPlayer );
	pPlayer->SetViewOffset( m_savedViewOffset );

	m_playerOff.FireOutput( pPlayer, this, 0 );

	// clear out the fire buttons
	m_attackaxis.Set( 0, pPlayer, this );
	m_attack2axis.Set( 0, pPlayer, this );

	m_nSpeed = 0;
	m_flThrottle = 0.0f;

	StopEngine();

	m_VehiclePhysics.GetVehicle()->OnVehicleExit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::PlayExitAnimation( void )
{
	int iSequence = m_pServerVehicle->GetExitAnimToUse();

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
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::ResetUseKey( CBasePlayer *pPlayer )
{
	pPlayer->m_afButtonPressed &= ~IN_USE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::DriveVehicle( CBasePlayer *pPlayer, CUserCmd *ucmd )
{
	//Lose control when the player dies
	if ( pPlayer->IsAlive() == false )
		return;

	DriveVehicle( ucmd->frametime, ucmd->buttons, pPlayer->m_afButtonPressed, pPlayer->m_afButtonReleased );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::DriveVehicle( float flFrameTime, int iButtons, int iButtonsDown, int iButtonsReleased )
{
	m_VehiclePhysics.UpdateDriverControls( iButtons, flFrameTime );

	m_nSpeed = m_VehiclePhysics.GetSpeed();	//send speed to client
	m_nRPM = clamp( m_VehiclePhysics.GetRPM(), 0, 4095 );
	m_nBoostTimeLeft = m_VehiclePhysics.BoostTimeLeft();
	m_nHasBoost = m_VehiclePhysics.HasBoost();
	m_flThrottle = m_VehiclePhysics.GetThrottle();

	m_nScannerDisabledWeapons = 0;		// off for now, change once we have scanners
	m_nScannerDisabledVehicle = 0;		// off for now, change once we have scanners

	//
	// Fire the appropriate outputs based on button pressed events.
	//
	// BUGBUG: m_afButtonPressed is broken - check the player.cpp code!!!
	float attack = 0, attack2 = 0;

	if ( iButtonsDown & IN_ATTACK )
	{
		m_pressedAttack.FireOutput( this, this, 0 );
	}
	if ( iButtonsDown & IN_ATTACK2 )
	{
		m_pressedAttack2.FireOutput( this, this, 0 );
	}

	if ( iButtons & IN_ATTACK )
	{
		attack = 1;
	}
	if ( iButtons & IN_ATTACK2 )
	{
		attack2 = 1;
	}

	m_attackaxis.Set( attack, this, this );
	m_attack2axis.Set( attack2, this, this );
}

//-----------------------------------------------------------------------------
// Purpose: Tells whether or not the car has been overturned
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::IsOverturned( void )
{
	Vector	vUp;
	VehicleAngleVectors( GetAbsAngles(), NULL, NULL, &vUp );

	float	upDot = DotProduct( Vector(0,0,1), vUp );

	// Tweak this number to adjust what's considered "overturned"
	if ( upDot < 0.0f )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::Think()
{
	BaseClass::Think();

	// Always think if we have any driver in us
	if ( GetDriver() )
	{
		SetNextThink( gpGlobals->curtime );
	}

	// If we have an NPC Driver, tell him to drive
	if ( m_hNPCDriver )
	{
		GetServerVehicle()->NPC_DriveVehicle();
	}

	// Keep thinking while we're waiting to turn off the keep upright
	if ( m_flTurnOffKeepUpright )
	{
		SetNextThink( gpGlobals->curtime );

		// Time up?
		if ( m_hKeepUpright && m_flTurnOffKeepUpright < gpGlobals->curtime && m_hKeepUpright )
		{
			variant_t emptyVariant;
			m_hKeepUpright->AcceptInput( "TurnOff", this, this, emptyVariant, USE_TOGGLE );
			m_flTurnOffKeepUpright = 0;

			UTIL_Remove( m_hKeepUpright );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// If the engine's not active, prevent driving
	if ( !IsEngineOn() )
		return;

	// If the player's entering/exiting the vehicle, prevent movement
	if ( m_bEnterAnimOn || m_bExitAnimOn )
		return;

	DriveVehicle( player, ucmd );
}

//-----------------------------------------------------------------------------
// Purpose: Prevent the player from entering / exiting the vehicle
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::InputLock( inputdata_t &inputdata )
{
	m_bLocked = true;
}

//-----------------------------------------------------------------------------
// Purpose: Allow the player to enter / exit the vehicle
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::InputUnlock( inputdata_t &inputdata )
{
	m_bLocked = false;
}

//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to enter the vehicle
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::CanEnterVehicle( CBaseEntity *pEntity )
{
	// Prevent entering if the vehicle's being driven by an NPC
	if ( GetDriver() && GetDriver() != pEntity )
		return false;

	if ( IsOverturned() )
		return false;

	// Prevent entering if the vehicle's locked, or if it's moving too fast.
	return ( !m_bLocked && (m_nSpeed <= m_flMinimumSpeedToEnterExit) );
}

//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to exit the vehicle
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::CanExitVehicle( CBaseEntity *pEntity )
{
	// Prevent exiting if the vehicle's locked, or if it's moving too fast.
	return ( !m_bLocked && (m_nSpeed <= m_flMinimumSpeedToEnterExit) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::InputTurnOn( inputdata_t &inputdata )
{
	// If the player's in the vehicle, start the engine.
	if ( GetServerVehicle()->GetPassenger( VEHICLE_DRIVER ) )
	{
		StartEngine();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::InputTurnOff( inputdata_t &inputdata )
{
	// If the player's in the vehicle, stop the engine.
	if ( GetServerVehicle()->GetPassenger( VEHICLE_DRIVER ) )
	{
		StopEngine();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if the engine is on.
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::IsEngineOn( void )
{
	return m_VehiclePhysics.IsOn();
}

//-----------------------------------------------------------------------------
// Purpose: Turn on the engine, but only if we're allowed to
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::StartEngine( void )
{
	m_VehiclePhysics.TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::StopEngine( void )
{
	m_VehiclePhysics.TurnOff();
}

//-----------------------------------------------------------------------------
// Purpose: // The player takes damage if he hits something going fast enough
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// Don't care if we don't have a driver
	if ( !GetDriver() )
		return;

	// Make sure we don't keep hitting the same entity
	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
	if ( pEvent->deltaCollisionTime < 0.5 && (pHitEntity == this) )
		return;

	BaseClass::VPhysicsCollision( index, pEvent );

	// If we hit hard enough, damage the player
	// Don't take damage from ramming bad guys
	if ( pHitEntity->MyNPCPointer() )
		return;

	// Don't take damage from ramming ragdolls
	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_PART_OF_RAGDOLL )
		return;

	// Ignore func_breakables
	CBreakable *pBreakable = dynamic_cast<CBreakable *>(pHitEntity);
	if ( pBreakable )
	{
		// ROBIN: Do we want to only do this on func_breakables that are about to die?
		//if ( pBreakable->HasSpawnFlags( SF_PHYSICS_BREAK_IMMEDIATELY ) )
		return;
	}

	// Over our skill's minimum crash level?
	int damageType = 0;
	float flDamage = CalculatePhysicsImpactDamage( index, pEvent, gDefaultPlayerVehicleImpactDamageTable, 1.0, true, damageType );
	if ( flDamage > 0 )
	{
		Vector damagePos;
		pEvent->pInternalData->GetContactPoint( damagePos );
		Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
		CTakeDamageInfo info( this, GetDriver(), damageForce, damagePos, flDamage, damageType );
		GetDriver()->TakeDamage( info );
	}
}

int CPropVehicleDriveable::VPhysicsGetObjectList( IPhysicsObject **pList, int listMax )
{
	return GetPhysics()->VPhysicsGetObjectList( pList, listMax );
}

//-----------------------------------------------------------------------------
// Purpose: Handle trace attacks from the physcannon
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	// If we've just been zapped by the physcannon, try and right ourselves
	if ( info.GetDamageType() & DMG_PHYSGUN )
	{
		// If we don't have one, create an upright controller for us
		if ( !m_hKeepUpright )
		{
			m_hKeepUpright = CKeepUpright::Create( GetAbsOrigin(), QAngle(0,0,1), this, 8, false );
		}
		Assert( m_hKeepUpright );
		variant_t emptyVariant;
		m_hKeepUpright->AcceptInput( "TurnOn", this, this, emptyVariant, USE_TOGGLE );

		// Turn off the keepupright after a short time
		m_flTurnOffKeepUpright = gpGlobals->curtime + 5;
		SetNextThink( gpGlobals->curtime );
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//========================================================================================================================================
// FOUR WHEEL PHYSICS VEHICLE SERVER VEHICLE
//========================================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFourWheelServerVehicle::SetVehicle( CBaseEntity *pVehicle )
{
	ASSERT( dynamic_cast<CPropVehicleDriveable*>(pVehicle) );
	BaseClass::SetVehicle( pVehicle );
}

//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void CFourWheelServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	Assert( nRole == VEHICLE_DRIVER );
	CBasePlayer *pPlayer = GetPassenger( VEHICLE_DRIVER );
	Assert( pPlayer );

	*pAbsAngles = pPlayer->EyeAngles(); // yuck. this is an in/out parameter.
	GetFourWheelVehiclePhysics()->GetVehicleViewPosition( "vehicle_driver_eyes", 1.0f, pAbsOrigin, pAbsAngles );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const vehicleparams_t *CFourWheelServerVehicle::GetVehicleParams( void )
{ 
	return &GetFourWheelVehiclePhysics()->GetVehicleParams(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropVehicleDriveable *CFourWheelServerVehicle::GetFourWheelVehicle( void )
{
	return (CPropVehicleDriveable *)m_pVehicle;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFourWheelVehiclePhysics *CFourWheelServerVehicle::GetFourWheelVehiclePhysics( void )
{
	CPropVehicleDriveable *pVehicle = GetFourWheelVehicle();
	return pVehicle->GetPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFourWheelServerVehicle::IsVehicleUpright( void )
{ 
	return (GetFourWheelVehicle()->IsOverturned() == false); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFourWheelServerVehicle::NPC_SetDriver( CNPC_VehicleDriver *pDriver )
{
	if ( pDriver )
	{
		m_nNPCButtons = 0;
		GetFourWheelVehicle()->m_hNPCDriver = pDriver;
		GetFourWheelVehicle()->StartEngine();
		SetVehicleVolume( 1.0 );	// Vehicles driven by NPCs are louder

		// Set our owner entity to be the NPC, so it can path check without hitting us
		GetFourWheelVehicle()->SetOwnerEntity( pDriver );

		// Start Thinking
		GetFourWheelVehicle()->SetNextThink( gpGlobals->curtime );
	}
	else
	{
		GetFourWheelVehicle()->m_hNPCDriver = NULL;
		GetFourWheelVehicle()->StopEngine();
		GetFourWheelVehicle()->SetOwnerEntity( NULL );
		SetVehicleVolume( 0.5 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFourWheelServerVehicle::NPC_DriveVehicle( void )
{
	if ( g_debug_vehicledriver.GetInt() )
	{
		if ( m_nNPCButtons )
		{
			Vector vecForward, vecRight;
			GetFourWheelVehicle()->GetVectors( &vecForward, &vecRight, NULL );
			if ( m_nNPCButtons & IN_FORWARD )
			{
				NDebugOverlay::Line( GetFourWheelVehicle()->GetAbsOrigin(), GetFourWheelVehicle()->GetAbsOrigin() + vecForward * 200, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_BACK )
			{
				NDebugOverlay::Line( GetFourWheelVehicle()->GetAbsOrigin(), GetFourWheelVehicle()->GetAbsOrigin() - vecForward * 200, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_MOVELEFT )
			{
				NDebugOverlay::Line( GetFourWheelVehicle()->GetAbsOrigin(), GetFourWheelVehicle()->GetAbsOrigin() - vecRight * 200 * -m_flTurnDegrees, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_MOVERIGHT )
			{
				NDebugOverlay::Line( GetFourWheelVehicle()->GetAbsOrigin(), GetFourWheelVehicle()->GetAbsOrigin() + vecRight * 200 * m_flTurnDegrees, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_JUMP )
			{
				NDebugOverlay::Box( GetFourWheelVehicle()->GetAbsOrigin(), -Vector(20,20,20), Vector(20,20,20), 0,255,0, true, 0.1 );
			}
		}
	}

	int buttonsChanged = m_nPrevNPCButtons ^ m_nNPCButtons;
	int afButtonPressed = buttonsChanged & m_nNPCButtons;		// The changed ones still down are "pressed"
	int afButtonReleased = buttonsChanged & (~m_nNPCButtons);	// The ones not down are "released"
	GetFourWheelVehicle()->DriveVehicle( gpGlobals->frametime, m_nNPCButtons, afButtonPressed, afButtonReleased );
	m_nPrevNPCButtons = m_nNPCButtons;

	// NPC's cheat by using analog steering.
	GetFourWheelVehiclePhysics()->SetSteering( m_flTurnDegrees, 0 );

	// Clear out attack buttons each frame
	m_nNPCButtons &= ~IN_ATTACK;
	m_nNPCButtons &= ~IN_ATTACK2;
}
