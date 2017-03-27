//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "vcollide_parse.h"
#include "vehicle_base.h"
#include "npc_vehicledriver.h"
#include "in_buttons.h"
#include "engine/ienginesound.h"
#include "soundenvelope.h"
#include "soundemittersystembase.h"
#include "saverestore_utlvector.h"
#include "keyvalues.h"
#include "studio.h"
#include "bone_setup.h"
#include "collisionutils.h"
#include "animation.h"

ConVar g_debug_vehiclesound( "g_debug_vehiclesound", "0", FCVAR_CHEAT );

#define HITBOX_SET	2

//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
BEGIN_DATADESC_NO_BASE( vehicle_gear_t )

	DEFINE_FIELD( vehicle_gear_t, flMinSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_gear_t, flMaxSpeed,			FIELD_FLOAT ),
	DEFINE_AUTO_ARRAY( vehicle_gear_t, flMinPitch,		FIELD_FLOAT ),
	DEFINE_AUTO_ARRAY( vehicle_gear_t, flMaxPitch,		FIELD_FLOAT ),
	DEFINE_FIELD( vehicle_gear_t, flSpeedApproachFactor,FIELD_FLOAT ),
	DEFINE_AUTO_ARRAY( vehicle_gear_t, iszSound,		FIELD_STRING ),

END_DATADESC()

BEGIN_DATADESC_NO_BASE( vehiclesounds_t )

	DEFINE_AUTO_ARRAY( vehiclesounds_t, iszSound,		FIELD_STRING ),
	DEFINE_UTLVECTOR( vehiclesounds_t, pGears,			FIELD_EMBEDDED ),

END_DATADESC()

BEGIN_DATADESC_NO_BASE( CBaseServerVehicle )

// These are reset every time by the constructor of the owning class 
//	DEFINE_FIELD( CBaseServerVehicle, m_pVehicle, FIELD_CLASSPTR ),
//	DEFINE_FIELD( CBaseServerVehicle, m_pDrivableVehicle; ??? ),

	// Controls
	DEFINE_FIELD( CBaseServerVehicle, m_nNPCButtons,		FIELD_INTEGER ),
	DEFINE_FIELD( CBaseServerVehicle, m_nPrevNPCButtons,	FIELD_INTEGER ),
	DEFINE_FIELD( CBaseServerVehicle, m_flTurnDegrees,		FIELD_FLOAT ),
	DEFINE_FIELD( CBaseServerVehicle, m_flVehicleVolume,	FIELD_FLOAT ),

	// We're going to reparse this data from file in Precache
	DEFINE_EMBEDDED( CBaseServerVehicle, m_vehicleSounds ),

	DEFINE_FIELD( CBaseServerVehicle, m_iSoundGear,			FIELD_INTEGER ),
	DEFINE_FIELD( CBaseServerVehicle, m_iGearEngineLoop,	FIELD_INTEGER ),
	DEFINE_FIELD( CBaseServerVehicle, m_flSpeedPercentage,	FIELD_FLOAT ),
	DEFINE_SOUNDPATCH( CBaseServerVehicle, m_EngineSoundPatch ),
	DEFINE_SOUNDPATCH( CBaseServerVehicle, m_SpeedSoundPatch ),
	
	DEFINE_FIELD( CBaseServerVehicle, m_bThrottleDown,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseServerVehicle, m_bRestartGear,		FIELD_BOOLEAN ),

// NOT SAVED
//	DEFINE_UTLVECTOR( CBaseServerVehicle, m_EntryAnimations ),
//	DEFINE_UTLVECTOR( CBaseServerVehicle, m_ExitAnimations ),
//	DEFINE_FIELD( CBaseServerVehicle, m_bParsedAnimations,	FIELD_INTEGER ),
	DEFINE_FIELD( CBaseServerVehicle, m_iCurrentExitAnim,	FIELD_INTEGER ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Base class for drivable vehicle handling. Contain it in your 
//			drivable vehicle.
//-----------------------------------------------------------------------------
CBaseServerVehicle::CBaseServerVehicle( void )
{
	m_pVehicle = NULL;
	m_pDrivableVehicle = NULL;
	m_nNPCButtons = 0;
	m_nPrevNPCButtons = 0;
	m_flTurnDegrees = 0;
	m_flVehicleVolume = 0.5;
	m_iSoundGear = 0;
	m_iGearEngineLoop = 0;
	m_EngineSoundPatch = NULL;
	m_SpeedSoundPatch = NULL;
	m_bRestartGear = false;
	m_bParsedAnimations = false;
	m_iCurrentExitAnim = 0;

	for ( int i = 0; i < VS_NUM_SOUNDS; ++i )
	{
		m_vehicleSounds.iszSound[i] = NULL_STRING;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseServerVehicle::~CBaseServerVehicle( void )
{
	// Destroy our sound patches
	StopSpeedSound();
	StopGearEngineLoop();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::Precache( void )
{
	// Precache all our gear sounds
	int iNumGears = m_vehicleSounds.pGears.Count();
	for ( int i = 0; i < iNumGears; i++ )
	{
		for ( int j = 0; j < VS_NUM_GEAR_SOUNDS; j++ )
		{
			if ( m_vehicleSounds.pGears[i].iszSound[j] != NULL_STRING )
			{
				enginesound->PrecacheSound( STRING(m_vehicleSounds.pGears[i].iszSound[j]) );
			}
		}
	}

	// Precache our other sounds
	for ( i = 0; i < VS_NUM_SOUNDS; i++ )
	{
		if ( m_vehicleSounds.iszSound[i] != NULL_STRING )
		{
			enginesound->PrecacheSound( STRING(m_vehicleSounds.iszSound[i]) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parses the vehicle's script for the vehicle sounds
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::Initialize( const char *pScriptName )
{
	byte *pFile = engine->LoadFileForMe( pScriptName, NULL );
	if ( !pFile )
		return false;

	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( (char *)pFile );
	CVehicleSoundsParser soundParser;
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcmpi( pBlock, "vehicle_sounds" ) )
		{
			pParse->ParseCustom( &m_vehicleSounds, &soundParser );
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );
	engine->FreeFile( pFile );

	// Extract the pitch ranges for our engine loops
	int iNumGears = m_vehicleSounds.pGears.Count();
	for ( int i = 0; i < iNumGears; i++ )
	{
		for ( int j = 0; j < VS_NUM_GEAR_SOUNDS; j++ )
		{
			if ( m_vehicleSounds.pGears[i].iszSound[j] != NULL_STRING )
			{
				enginesound->PrecacheSound( STRING(m_vehicleSounds.pGears[i].iszSound[j]) );

				CSoundParameters params;
				CBaseEntity::GetParametersForSound( STRING(m_vehicleSounds.pGears[i].iszSound[j]), params );
				m_vehicleSounds.pGears[i].flMinPitch[j] = params.pitchlow;
				m_vehicleSounds.pGears[i].flMaxPitch[j] = params.pitchhigh;
			}
		}
	}

	Precache();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SetVehicle( CBaseEntity *pVehicle )
{ 
	m_pVehicle = pVehicle; 
	m_pDrivableVehicle = dynamic_cast<IDrivableVehicle*>(m_pVehicle);
	Assert( m_pDrivableVehicle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IDrivableVehicle *CBaseServerVehicle::GetDrivableVehicle( void )
{
	Assert( m_pDrivableVehicle );
	return m_pDrivableVehicle;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the driver. Unlike GetPassenger(VEHICLE_DRIVER), it will return
//			the NPC driver if it has one.
//-----------------------------------------------------------------------------
CBaseEntity	*CBaseServerVehicle::GetDriver( void )
{
	return GetPassenger(VEHICLE_DRIVER);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBasePlayer	*CBaseServerVehicle::GetPassenger( int nRole ) 
{ 
	Assert( nRole == VEHICLE_DRIVER ); 
	return ToBasePlayer( GetDrivableVehicle()->GetDriver() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseServerVehicle::GetPassengerRole( CBasePlayer *pPassenger )
{
	if (pPassenger == GetDrivableVehicle()->GetDriver())
		return VEHICLE_DRIVER;
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Get and set the current driver. Use PassengerRole_t enum in shareddefs.h for adding passengers
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SetPassenger( int nRole, CBasePlayer *pPassenger )
{
	// Baseclass only handles vehicles with a single passenger
	Assert( nRole == VEHICLE_DRIVER ); 
	
	// IVehicle is clearing the driver
	if ( pPassenger )
	{
		GetDrivableVehicle()->EnterVehicle( pPassenger );
	}
	else
	{
		GetDrivableVehicle()->ExitVehicle( nRole );
	}
}
	
//-----------------------------------------------------------------------------
// Purpose: Get a position in *world space* inside the vehicle for the player to start at
//-----------------------------------------------------------------------------
void CBaseServerVehicle::GetPassengerStartPoint( int nRole, Vector *pPoint, QAngle *pAngles )
{
	Assert( nRole == VEHICLE_DRIVER ); 

	// NOTE: We don't set the angles, which causes them to remain the same
	// as they were before entering the vehicle

	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	if ( pAnimating )
	{
		char pAttachmentName[32];
		Q_snprintf( pAttachmentName, 32, "vehicle_feet_passenger%d", nRole );
		int nFeetAttachmentIndex = pAnimating->LookupAttachment(pAttachmentName);
		if ( nFeetAttachmentIndex > 0 )
		{
			QAngle vecAngles;
			pAnimating->GetAttachment( nFeetAttachmentIndex, *pPoint, vecAngles );
			return;
		}
	}

	// Couldn't find the attachment point, so just use the origin
	*pPoint = m_pVehicle->GetAbsOrigin();
}

//---------------------------------------------------------------------------------
// Check Exit Point for leaving vehicle.
//
// Input: yaw/roll from vehicle angle to check for exit
//		  distance from origin to drop player (allows for different shaped vehicles
// Output: returns true if valid location, pEndPoint
//         updated with actual exit point
//---------------------------------------------------------------------------------
bool CBaseServerVehicle::CheckExitPoint( float yaw, int distance, Vector *pEndPoint )
{
	QAngle vehicleAngles = m_pVehicle->GetLocalAngles();
  	Vector vecStart = m_pVehicle->GetAbsOrigin();
  	Vector vecDir;
   
  	vecStart.z += 48;		// always 48" from ground
  	vehicleAngles[YAW] += yaw;	
  	AngleVectors( vehicleAngles, NULL, &vecDir, NULL );
	// Vehicles are oriented along the Y axis
	vecDir *= -1;
  	*pEndPoint = vecStart + vecDir * distance;
  
  	trace_t tr;
  	UTIL_TraceHull( vecStart, *pEndPoint, Vector(-10,-10,-36), Vector(10,10,36), MASK_SOLID, m_pVehicle, COLLISION_GROUP_NONE, &tr );
  
  	if( tr.fraction < 1.0 )
  		return false;
  
  	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Where does this passenger exit the vehicle?
//-----------------------------------------------------------------------------
void CBaseServerVehicle::GetPassengerExitPoint( int nRole, Vector *pExitPoint, QAngle *pAngles )
{ 
	Assert( nRole == VEHICLE_DRIVER ); 

	// First, see if we've got an attachment point
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	if ( pAnimating )
	{
		Vector vehicleExitOrigin;
		QAngle vehicleExitAngles;
		if ( pAnimating->GetAttachment( "vehicle_driver_exit", vehicleExitOrigin, vehicleExitAngles ) )
		{
			// Make sure it's clear
			trace_t tr;
			UTIL_TraceHull( vehicleExitOrigin + Vector(0,0,48), vehicleExitOrigin, Vector(-10,-10,-36), Vector(10,10,36), MASK_SOLID, m_pVehicle, COLLISION_GROUP_NONE, &tr );
			if ( !tr.startsolid )
			{
				*pAngles = vehicleExitAngles;
				*pExitPoint = tr.endpos;
				return;
			}
		}
	}

	// left side 
	if( CheckExitPoint( 90, 90, pExitPoint ) )	// angle from car, distance from origin, actual exit point
		return;

	// right side
	if( CheckExitPoint( -90, 90, pExitPoint ) )
		return;

	// front
	if( CheckExitPoint( 0, 100, pExitPoint ) )
		return;

	// back
	if( CheckExitPoint( 180, 170, pExitPoint ) )
		return;

	// All else failed, just pop out top
	Vector vecWorldMins, vecWorldMaxs;
	m_pVehicle->WorldSpaceAABB( &vecWorldMins, &vecWorldMaxs );
	pExitPoint->x = (vecWorldMins.x + vecWorldMaxs.x) * 0.5f;
	pExitPoint->y = (vecWorldMins.y + vecWorldMaxs.y) * 0.5f;
	pExitPoint->z = vecWorldMaxs.z + 50.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::ParseEntryExitAnims( void )
{
	// Try and find the right animation to play in the model's keyvalues
	KeyValues *modelKeyValues = new KeyValues("");
	if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( m_pVehicle->GetModel() ), modelinfo->GetModelKeyValueText( m_pVehicle->GetModel() ) ) )
	{
		// Do we have an entry section?
		KeyValues *pkvEntryList = modelKeyValues->FindKey("vehicle_entry");
		if ( pkvEntryList )
		{
			// Look through the entry animations list
			KeyValues *pkvEntryAnim = pkvEntryList->GetFirstSubKey();
			while ( pkvEntryAnim )
			{
				// Add 'em to our list
				int iIndex = m_EntryAnimations.AddToTail();
				Q_strncpy( m_EntryAnimations[iIndex].szAnimName, pkvEntryAnim->GetName(), sizeof(m_EntryAnimations[iIndex].szAnimName) );
				m_EntryAnimations[iIndex].iHitboxGroup = pkvEntryAnim->GetInt();

				pkvEntryAnim = pkvEntryAnim->GetNextKey();
			}
		}

		// Do we have an exit section?
		KeyValues *pkvExitList = modelKeyValues->FindKey("vehicle_exit");
		if ( pkvExitList )
		{
			// Look through the entry animations list
			KeyValues *pkvExitAnim = pkvExitList->GetFirstSubKey();
			while ( pkvExitAnim )
			{
				// Add 'em to our list
				int iIndex = m_ExitAnimations.AddToTail();
				Q_strncpy( m_ExitAnimations[iIndex].szAnimName, pkvExitAnim->GetName(), sizeof(m_ExitAnimations[iIndex].szAnimName) );
				if ( !Q_strncmp( pkvExitAnim->GetString(), "upsidedown", 10 ) )
				{
					m_ExitAnimations[iIndex].bUpright = false;
				}
				else 
				{
					m_ExitAnimations[iIndex].bUpright = true;
				}
				CBaseAnimating *pAnimating = (CBaseAnimating *)m_pVehicle;
				m_ExitAnimations[iIndex].iAttachment = pAnimating->LookupAttachment( m_ExitAnimations[iIndex].szAnimName );

				pkvExitAnim = pkvExitAnim->GetNextKey();
			}
		}
	}

	modelKeyValues->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseServerVehicle::GetEntryAnimForPoint( const Vector &vecEyePoint )
{
	// Parse the vehicle animations the first time they get in the vehicle
	if ( !m_bParsedAnimations )
	{
		// Load the entry/exit animations from the vehicle
		ParseEntryExitAnims();
		m_bParsedAnimations = true;
	}

	// No entry anims? Vehicles with no entry anims are always enterable.
	if ( !m_EntryAnimations.Count() )
		return 0;

	// Figure out which entrypoint hitbox the player is in
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	if ( !pAnimating )
		return 0;

	studiohdr_t *pStudioHdr = pAnimating->GetModelPtr();
	if (!pStudioHdr)
		return 0;
	int iHitboxSet = FindHitboxSetByName( pStudioHdr, "entryboxes" );
	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( iHitboxSet );
	if ( !set || !set->numhitboxes )
		return 0;

	// Loop through the hitboxes and find out which one we're in
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox( i );

		Vector vecPosition;
		QAngle vecAngles;
		pAnimating->GetBonePosition( pbox->bone, vecPosition, vecAngles );

		// Build a rotation matrix from orientation
		matrix3x4_t fRotateMatrix;
		AngleMatrix( vecAngles, vecPosition, fRotateMatrix);

		Vector localEyePoint;
		VectorITransform( vecEyePoint, fRotateMatrix, localEyePoint );
		if ( IsPointInBox( localEyePoint, pbox->bbmin, pbox->bbmax ) )
		{
			// Find the entry animation for this hitbox
			int iCount = m_EntryAnimations.Count();
			for ( int entry = 0; entry < iCount; entry++ )
			{
				if ( m_EntryAnimations[entry].iHitboxGroup == pbox->group )
				{
					// Get the sequence for the animation
					return pAnimating->LookupSequence( m_EntryAnimations[entry].szAnimName );
				}
			}
		}
	}

	// Fail
	return ACTIVITY_NOT_AVAILABLE;
}

//-----------------------------------------------------------------------------
// Purpose: Find an exit animation that'll get the player to a valid position
//-----------------------------------------------------------------------------
int CBaseServerVehicle::GetExitAnimToUse( void )
{
	// Parse the vehicle animations the first time they get in the vehicle
	if ( !m_bParsedAnimations )
	{
		// Load the entry/exit animations from the vehicle
		ParseEntryExitAnims();
		m_bParsedAnimations = true;
	}

	// No exit anims? 
	if ( !m_ExitAnimations.Count() )
		return 0;

	// Figure out which entrypoint hitbox the player is in
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	if ( !pAnimating )
		return ACTIVITY_NOT_AVAILABLE;

	studiohdr_t *pStudioHdr = pAnimating->GetModelPtr();
	if (!pStudioHdr)
		return 0;

	bool bUpright = IsVehicleUpright();

	// Loop through the exit animations and find one that ends in a clear position
	int iCount = m_ExitAnimations.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		if ( m_ExitAnimations[i].bUpright == bUpright )
		{
			trace_t tr;
			Vector vehicleExitOrigin;
			QAngle vehicleExitAngles;

			// Ensure the endpoint is clear
			pAnimating->GetAttachment( m_ExitAnimations[i].iAttachment, vehicleExitOrigin, vehicleExitAngles );
  			UTIL_TraceHull( m_pVehicle->GetAbsOrigin(), vehicleExitOrigin, VEC_HULL_MIN, VEC_HULL_MAX, MASK_SOLID, m_pVehicle, COLLISION_GROUP_NONE, &tr );
		  	if ( tr.fraction == 1.0 )
			{
				m_iCurrentExitAnim = i;
				return pAnimating->LookupSequence( m_ExitAnimations[i].szAnimName );
			}
		}
	}

	// Fail
	return ACTIVITY_NOT_AVAILABLE;	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::HandleEntryExitFinish( bool bExitAnimOn )
{
	// Figure out which entrypoint hitbox the player is in
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	if ( !pAnimating )
		return;
	
	// Did the exit anim just finish?
	if ( bExitAnimOn )
	{
		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer *>(GetDriver());
		if ( pPlayer )
		{
			Vector vecEyes;
			QAngle vecEyeAng;
			if ( m_iCurrentExitAnim >= 0 )
			{
				pAnimating->GetAttachment( m_ExitAnimations[m_iCurrentExitAnim].szAnimName, vecEyes, vecEyeAng );
			}
			else
			{
				pAnimating->GetAttachment( "vehicle_driver_eyes", vecEyes, vecEyeAng );
			}
			vecEyeAng.x = 0;
			vecEyeAng.z = 0;
			pPlayer->LeaveVehicle( vecEyes, vecEyeAng );
		}
	}

	// Start the vehicle idling again
	int iSequence = pAnimating->SelectWeightedSequence( ACT_IDLE );
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		pAnimating->m_flCycle = 0;
		pAnimating->m_flAnimTime = gpGlobals->curtime;
		pAnimating->ResetSequence( iSequence );
		pAnimating->ResetClientsideFrame();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Where does the passenger see from?
//-----------------------------------------------------------------------------
void CBaseServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	Assert( nRole == VEHICLE_DRIVER );
	CBasePlayer *pPlayer = GetPassenger( VEHICLE_DRIVER );
	Assert( pPlayer );

	*pAbsAngles = pPlayer->EyeAngles();
	*pAbsOrigin = m_pVehicle->GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	GetDrivableVehicle()->SetupMove( player, ucmd, pHelper, move );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData )
{
	GetDrivableVehicle()->ProcessMovement( pPlayer, pMoveData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	GetDrivableVehicle()->FinishMove( player, ucmd, move );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::ItemPostFrame( CBasePlayer *player )
{
	Assert( player == GetDriver() );

	GetDrivableVehicle()->ItemPostFrame( player );

	if ( player->m_afButtonPressed & IN_USE )
	{
		if ( GetDrivableVehicle()->CanExitVehicle(player) )
		{
			// Let the vehicle try to play the exit animation
			if ( GetDrivableVehicle()->PlayExitAnimation() )
				return;

			// Couldn't find an animation, so exit immediately
			player->LeaveVehicle();
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_ThrottleForward( void )
{
	m_nNPCButtons |= IN_FORWARD;
	m_nNPCButtons &= ~IN_BACK;
	m_nNPCButtons &= ~IN_JUMP;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_ThrottleReverse( void )
{
	m_nNPCButtons |= IN_BACK;
	m_nNPCButtons &= ~IN_FORWARD;
	m_nNPCButtons &= ~IN_JUMP;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_ThrottleCenter( void )
{
	m_nNPCButtons &= ~IN_FORWARD;
	m_nNPCButtons &= ~IN_BACK;
	m_nNPCButtons &= ~IN_JUMP;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_Brake( void )
{
	m_nNPCButtons &= ~IN_FORWARD;
	m_nNPCButtons &= ~IN_BACK;
	m_nNPCButtons |= IN_JUMP;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_TurnLeft( float flDegrees )
{
	m_nNPCButtons |= IN_MOVELEFT;
	m_nNPCButtons &= ~IN_MOVERIGHT;
	m_flTurnDegrees = -flDegrees;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_TurnRight( float flDegrees )
{
	m_nNPCButtons |= IN_MOVERIGHT;
	m_nNPCButtons &= ~IN_MOVELEFT;
	m_flTurnDegrees = flDegrees;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_TurnCenter( void )
{
	m_nNPCButtons &= ~IN_MOVERIGHT;
	m_nNPCButtons &= ~IN_MOVELEFT;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_PrimaryFire( void )
{
	m_nNPCButtons |= IN_ATTACK;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_SecondaryFire( void )
{
	m_nNPCButtons |= IN_ATTACK2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange )
{
	*flMinRange = 64;
	*flMaxRange = 1024;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange )
{
	*flMinRange = 64;
	*flMaxRange = 1024;
}

//-----------------------------------------------------------------------------
// Purpose: Return the time at which this vehicle's primary weapon can fire again
//-----------------------------------------------------------------------------
float CBaseServerVehicle::Weapon_PrimaryCanFireAt( void )
{
	return gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Return the time at which this vehicle's secondary weapon can fire again
//-----------------------------------------------------------------------------
float CBaseServerVehicle::Weapon_SecondaryCanFireAt( void )
{
	return gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Vehicle Sound Start
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SoundStart( void )
{
	// speed sound
	if ( m_vehicleSounds.iszSound[VS_SPEED_SOUND] != NULL_STRING )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		CPASAttenuationFilter filter( m_pVehicle );
		m_SpeedSoundPatch = controller.SoundCreate( filter, m_pVehicle->entindex(), CHAN_STATIC, STRING(m_vehicleSounds.iszSound[VS_SPEED_SOUND]), ATTN_NORM );
		controller.Play( m_SpeedSoundPatch, 0.0, 100 );		// start with 0 volume (no speed)
	}

	// play startup sound for vehicle
	PlaySound( VS_ENGINE_START );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SoundShutdown( float flFadeTime )
{
	StopSpeedSound( flFadeTime );
	StopGearEngineLoop( flFadeTime );

	// Stop any looping sounds that may be running, as the following stop sound may not exist
	// and thus leave a looping sound playing after the user gets out.
	for ( int i = 0; i < NUM_SOUNDS_TO_STOP_ON_EXIT; i++ )
	{
		StopSound( g_iSoundsToStopOnExit[i] );
	}

	// Stop all gear sounds
	for ( i = 0; i < VS_NUM_GEAR_SOUNDS; i++ )
	{
		StopSoundForGear( (vehiclesound_gear)i, m_iSoundGear );
	}

	// Play shutdown sound for vehicle, but only if the car was moving
	if ( m_bThrottleDown )
	{
		PlaySound( VS_ENGINE_STOP );
	} 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SoundUpdate( float flFrameTime, float flCurrentSpeed, bool bThrottleDown, bool bReverse )
{
	bool bLastThrottle = m_bThrottleDown;
	m_bThrottleDown = bThrottleDown;

	if ( g_debug_vehiclesound.GetInt() )
	{
		Msg("CurrentSpeed: %.3f  ", flCurrentSpeed );
	}

	// Figure out our speed for the purposes of sound playing.
	// We slow the transition down a little to make the gear changes slower.
	if ( m_vehicleSounds.pGears.Count() > 0 )
	{
		if ( flCurrentSpeed > m_flSpeedPercentage )
		{
			flCurrentSpeed = Approach( flCurrentSpeed, m_flSpeedPercentage, flFrameTime * m_vehicleSounds.pGears[m_iSoundGear].flSpeedApproachFactor );
		}
	}
	m_flSpeedPercentage = flCurrentSpeed;

	if ( g_debug_vehiclesound.GetInt() )
	{
		Msg("Sound Speed: %.3f\n", m_flSpeedPercentage );
	}

	// Only do gear changes when the throttle's down
	RecalculateSoundGear( m_bThrottleDown, bReverse );

	// Update our speed sound
	if ( m_SpeedSoundPatch )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		float flPitch = RemapVal( m_flSpeedPercentage, 0, 1, 85, 120 );
		controller.SoundChangePitch( m_SpeedSoundPatch, flPitch, 0.1 );

		// Drop the volume a lot if we're going slowly
		if ( m_flSpeedPercentage >= 0.3 )
		{
			controller.SoundChangeVolume( m_SpeedSoundPatch, RemapVal( m_flSpeedPercentage, 0.3, 1.0, 0.0, 1.0 ), 0.1 );
		}
		else
		{
			controller.SoundChangeVolume( m_SpeedSoundPatch, 0, 0.1 );
		}
	}

	// Update our engine loop
	if ( m_EngineSoundPatch )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		// Reduce the volume if the throttle is up
		if ( !m_bThrottleDown )
		{
			// Drop the pitch to 75% of current, as well as fading the volume
			float flPitch = controller.SoundGetPitch( m_EngineSoundPatch );
			float flFadeTime = 1.0;;
			controller.SoundChangePitch( m_EngineSoundPatch, (flPitch * 0.75), flFadeTime );	 // Drop to 75% of its current pitch
			controller.SoundChangeVolume( m_EngineSoundPatch, 0, flFadeTime );
		}
		else 
		{
			// Adjust the pitch of the sound within the current gear's min/max speeds
			float flMinPitch = m_vehicleSounds.pGears[m_iSoundGear].flMinPitch[m_iGearEngineLoop];
			float flMaxPitch = m_vehicleSounds.pGears[m_iSoundGear].flMaxPitch[m_iGearEngineLoop];
			float flMinSpeed = m_vehicleSounds.pGears[m_iSoundGear].flMinSpeed;
			float flMaxSpeed = m_vehicleSounds.pGears[m_iSoundGear].flMaxSpeed;
			float flPitch = RemapVal( m_flSpeedPercentage, flMinSpeed, flMaxSpeed, flMinPitch, flMaxPitch );
			flPitch = clamp( flPitch, flMinPitch, flMaxPitch );
			controller.SoundChangePitch( m_EngineSoundPatch, flPitch, 0.1 );

			// Just put the foot on the gas?
			if ( !bLastThrottle )
			{
				// Bring the volume back up to full
				controller.SoundChangeVolume( m_EngineSoundPatch, 1.0, 0.1 );

				// Stop engine startup & idle sounds
				StopSound( VS_ENGINE_IDLE );
				StopSound( VS_ENGINE_START );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play a non-gear based vehicle sound
//-----------------------------------------------------------------------------
void CBaseServerVehicle::PlaySound( vehiclesound iSound )
{
	if ( m_vehicleSounds.iszSound[iSound] != NULL_STRING )
	{
		CPASAttenuationFilter filter( m_pVehicle );
		CBaseEntity::EmitSound( filter, m_pVehicle->entindex(), CHAN_VOICE, STRING(m_vehicleSounds.iszSound[iSound]), m_flVehicleVolume, ATTN_NORM, 0, 100 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop a non-gear based vehicle sound
//-----------------------------------------------------------------------------
void CBaseServerVehicle::StopSound( vehiclesound iSound )
{
	if ( m_vehicleSounds.iszSound[iSound] != NULL_STRING )
	{
		CBaseEntity::StopSound( m_pVehicle->entindex(), CHAN_VOICE, STRING(m_vehicleSounds.iszSound[iSound]) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the gear we should be in based upon the vehicle's current speed
//-----------------------------------------------------------------------------
void CBaseServerVehicle::RecalculateSoundGear( bool bThrottleDown, bool bReverse )
{
	int iLastGear = m_iSoundGear;

	int iNumGears = m_vehicleSounds.pGears.Count();
	for ( int i = (iNumGears-1); i >= 0; i-- )
	{
		if ( m_flSpeedPercentage > m_vehicleSounds.pGears[i].flMinSpeed )
		{
			m_iSoundGear = i;
			break;
		}
	}

	// If we're going in reverse, we want to stay in first gear
	if ( bReverse )
	{
		m_iSoundGear = 0;
	}

	// Detect gear changes and start the new gear's sound, but only if the throttle is down
	if ( bThrottleDown && (m_bRestartGear || (iLastGear != m_iSoundGear)) )
	{
		m_bRestartGear = false;

		// Fade the old gear's engine loop
		if ( m_EngineSoundPatch )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundFadeOut( m_EngineSoundPatch, 0.5 );
			m_EngineSoundPatch = NULL;
		}

		// Start the new gear's engine loop
		if ( iLastGear > m_iSoundGear )
		{
			if ( g_debug_vehiclesound.GetInt() )
			{
				Msg("DOWNSHIFTING\n");
			}
			StartGearEngineLoop( VS_DOWNSHIFTED );
		}
		else
		{
			if ( g_debug_vehiclesound.GetInt() )
			{
				Msg("SHIFTING UP\n");
			}
			StartGearEngineLoop( VS_ENGINE_LOOP );
		}
	}
	else if ( m_flSpeedPercentage < 0.01 && !bThrottleDown )
	{
		// If we're in the first gear, and not moving, stop the gear sound.
		// This is to ensure we get the first gear sound correctly started when you
		// start accelerating after stopping.
		StopGearEngineLoop();
		m_bRestartGear = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play a gear-based vehicle sound.
//-----------------------------------------------------------------------------
void CBaseServerVehicle::PlaySoundForGear( vehiclesound_gear iSound, int iGear )
{
	// No gears?
	if ( m_vehicleSounds.pGears.Count() == 0 )
		return;

	if ( iGear == -1 )
	{
		iGear = m_iSoundGear;
	}

	if ( m_vehicleSounds.pGears[iGear].iszSound[iSound] != NULL_STRING )
	{
		CPASAttenuationFilter filter( m_pVehicle );
		CBaseEntity::EmitSound( filter, m_pVehicle->entindex(), CHAN_VOICE, STRING(m_vehicleSounds.pGears[iGear].iszSound[iSound]), m_flVehicleVolume, ATTN_NORM, 0, 100 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop a gear-based vehicle sound.
//-----------------------------------------------------------------------------
void CBaseServerVehicle::StopSoundForGear( vehiclesound_gear iSound, int iGear )
{
	// No gears?
	if ( m_vehicleSounds.pGears.Count() == 0 )
		return;

	if ( iGear == -1 )
	{
		iGear = m_iSoundGear;
	}

	if ( m_vehicleSounds.pGears[iGear].iszSound[iSound] != NULL_STRING )
	{
		CBaseEntity::StopSound( m_pVehicle->entindex(), CHAN_VOICE, STRING(m_vehicleSounds.pGears[iGear].iszSound[iSound]) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::StartGearEngineLoop( vehiclesound_gear iSound )
{
	// No gears?
	if ( m_vehicleSounds.pGears.Count() == 0 )
		return;

	// Already started?
	if ( m_EngineSoundPatch )
		return;

	m_iGearEngineLoop = iSound;
	if ( m_vehicleSounds.pGears[m_iSoundGear].iszSound[iSound] != NULL_STRING )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		CPASAttenuationFilter filter( m_pVehicle );
		m_EngineSoundPatch = controller.SoundCreate( filter, m_pVehicle->entindex(), CHAN_STATIC, STRING(m_vehicleSounds.pGears[m_iSoundGear].iszSound[iSound]), ATTN_NORM );
		controller.Play( m_EngineSoundPatch, 1.0, 100 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::StopGearEngineLoop( float flFadeTime )
{
	if ( m_EngineSoundPatch )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		if ( flFadeTime )
		{
			controller.SoundFadeOut( m_EngineSoundPatch, flFadeTime );
		}
		else
		{
			controller.SoundDestroy( m_EngineSoundPatch );
		}
		m_EngineSoundPatch = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::StopSpeedSound( float flFadeTime )
{
	if ( m_SpeedSoundPatch )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		if ( flFadeTime )
		{
			controller.SoundFadeOut( m_SpeedSoundPatch, flFadeTime );
		}
		else
		{
			controller.SoundDestroy( m_SpeedSoundPatch );
		}
		m_SpeedSoundPatch = NULL;
	}
}

//===========================================================================================================
// Vehicle Sounds
//===========================================================================================================
// These are sounds that are to be automatically stopped whenever the vehicle's driver leaves it
vehiclesound g_iSoundsToStopOnExit[] =
{
	VS_ENGINE_START,
	VS_ENGINE_IDLE,
	VS_ENGINE2_START,
	VS_ENGINE2_STOP,
};

char *vehiclesound_parsenames[VS_NUM_SOUNDS] =
{
	"engine_start",
	"engine_stop",
	"engine_idle",
	"turbo",
	"turbo_off",
	"skid_lowfriction",
	"skid_normalfriction",
	"skid_highfriction",
	"speed_sound",
	"engine2_start",
	"engine2_stop",
	"misc1",
	"misc2",
	"misc3",
	"misc4",
};

char *vehiclesound_gear_parsenames[VS_NUM_GEAR_SOUNDS] =
{
	"engine_loop",
	"foot_off",
	"foot_off_slow",
	"downshifted",
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVehicleSoundsParser::ParseKeyValue( void *pData, const char *pKey, const char *pValue )
{
	vehiclesounds_t *pSounds = (vehiclesounds_t *)pData;
	// New gear?
	if ( !strcmpi( pKey, "gear" ) )
	{
		// Create, initialize, and add a new gear to our list
		int iNewGear = pSounds->pGears.AddToTail();
		pSounds->pGears[iNewGear].flMaxSpeed = 0;
		pSounds->pGears[iNewGear].flSpeedApproachFactor = 1.0;
		for ( int i = 0; i < VS_NUM_GEAR_SOUNDS; i++ )
		{
			pSounds->pGears[iNewGear].iszSound[i] = NULL_STRING;
		}

		// Set our min speed to the previous gear's max
		if ( iNewGear == 0 )
		{
			// First gear, so our minspeed is 0
			pSounds->pGears[iNewGear].flMinSpeed = 0;
		}
		else
		{
			pSounds->pGears[iNewGear].flMinSpeed = pSounds->pGears[iNewGear-1].flMaxSpeed;
		}

		// Remember which gear we're reading data from
		m_iCurrentGear = iNewGear;
	}
	else
	{
		// Are we currently in a gear block?
		if ( m_iCurrentGear >= 0 )
		{
			Assert( m_iCurrentGear < pSounds->pGears.Count() );

			// Check gear sounds
			for ( int i = 0; i < VS_NUM_GEAR_SOUNDS; i++ )
			{
				if ( !strcmpi( pKey, vehiclesound_gear_parsenames[i] ) )
				{
					pSounds->pGears[m_iCurrentGear].iszSound[i] = AllocPooledString(pValue);
					return;
				}
			}

			// Check gear keys
			if ( !strcmpi( pKey, "max_speed" ) )
			{
				pSounds->pGears[m_iCurrentGear].flMaxSpeed = atof(pValue);
				return;
			}
			if ( !strcmpi( pKey, "speed_approach_factor" ) )
			{
				pSounds->pGears[m_iCurrentGear].flSpeedApproachFactor = atof(pValue);
				return;
			}
		}

		// We're done reading a gear, so stop checking them.
		m_iCurrentGear = -1;

		for ( int i = 0; i < VS_NUM_SOUNDS; i++ )
		{
			if ( !strcmpi( pKey, vehiclesound_parsenames[i] ) )
			{
				pSounds->iszSound[i] = AllocPooledString(pValue);
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVehicleSoundsParser::SetDefaults( void *pData ) 
{
	vehiclesounds_t *pSounds = (vehiclesounds_t *)pData;
	for ( int i = 0; i < VS_NUM_SOUNDS; i++ )
	{
		pSounds->iszSound[i] = NULL_STRING;
	}

	pSounds->pGears.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Parse just the vehicle_sound section of a vehicle file.
//-----------------------------------------------------------------------------
void CVehicleSoundsParser::ParseVehicleSounds( const char *pScriptName, vehiclesounds_t *pSounds )
{
	byte *pFile = engine->LoadFileForMe( pScriptName, NULL );
	if ( !pFile )
		return;

	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( (char *)pFile );
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcmpi( pBlock, "vehicle_sounds" ) )
		{
			pParse->ParseCustom( &pSounds, this );
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );

	engine->FreeFile( pFile );
}
