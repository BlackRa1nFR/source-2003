//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Base class for helicopters & helicopter-type vehicles
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "AI_Network.h"
#include "AI_Default.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Node.h"
#include "AI_Task.h"
#include "AI_Senses.h"
#include "AI_Memory.h"
#include "entitylist.h"
#include "soundenvelope.h"
#include "gamerules.h"
#include "grenade_homer.h"
#include "ndebugoverlay.h"
#include "CBaseHelicopter.h"
#include "soundflags.h"
#include "rope.h"
#include "saverestore_utlvector.h"


void UTIL_RotorWash( const Vector &origin, const Vector &direction, float maxDistance );
void ExpandBBox(Vector &vecMins, Vector &vecMaxs);

#if 0
virtual void NullThink( void );
#endif //0

#define HELICOPTER_THINK_INTERVAL 0.1

#define	BASECHOPPER_DEBUG_WASH		1

ConVar g_debug_basehelicopter( "g_debug_basehelicopter", "0", FCVAR_CHEAT );

//---------------------------------------------------------
//---------------------------------------------------------
// TODOs
//
// -Member function: CHANGE MOVE GOAL
//
// -Member function: GET GRAVITY (or GetMaxThrust)
//
//---------------------------------------------------------
//---------------------------------------------------------

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------

BEGIN_DATADESC_NO_BASE( washentity_t )
	DEFINE_FIELD( washentity_t, hEntity,			FIELD_EHANDLE ),
	DEFINE_FIELD( washentity_t, flWashStartTime,	FIELD_TIME ),
END_DATADESC()


BEGIN_DATADESC( CBaseHelicopter )

	DEFINE_THINKFUNC( CBaseHelicopter, HelicopterThink ),
	DEFINE_THINKFUNC( CBaseHelicopter, CallDyingThink ),
	DEFINE_ENTITYFUNC( CBaseHelicopter, CrashTouch ),
	DEFINE_ENTITYFUNC( CBaseHelicopter, FlyTouch ),

	DEFINE_SOUNDPATCH( CBaseHelicopter, m_pRotorSound ),
	DEFINE_SOUNDPATCH( CBaseHelicopter, m_pRotorBlast ),
	DEFINE_FIELD( CBaseHelicopter, m_flForce,			FIELD_FLOAT ),
	DEFINE_FIELD( CBaseHelicopter, m_fHelicopterFlags,	FIELD_INTEGER),
	DEFINE_FIELD( CBaseHelicopter, m_vecDesiredFaceDir,	FIELD_VECTOR ),
	DEFINE_FIELD( CBaseHelicopter, m_flLastSeen,		FIELD_TIME ),
	DEFINE_FIELD( CBaseHelicopter, m_flPrevSeen,		FIELD_TIME ),
//	DEFINE_FIELD( CBaseHelicopter, m_iSoundState,		FIELD_INTEGER ),		// Don't save, precached
	DEFINE_FIELD( CBaseHelicopter, m_vecTarget,			FIELD_VECTOR ),
	DEFINE_FIELD( CBaseHelicopter, m_vecTargetPosition,	FIELD_POSITION_VECTOR ),

	DEFINE_FIELD( CBaseHelicopter, m_flMaxSpeed,		FIELD_FLOAT ),
	DEFINE_FIELD( CBaseHelicopter, m_flMaxSpeedFiring,	FIELD_FLOAT ),
	DEFINE_FIELD( CBaseHelicopter, m_flGoalSpeed,		FIELD_FLOAT ),
	DEFINE_KEYFIELD( CBaseHelicopter, m_flInitialSpeed, FIELD_FLOAT, "InitialSpeed" ),

	DEFINE_FIELD( CBaseHelicopter, m_cullBoxMins,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseHelicopter, m_cullBoxMaxs,	FIELD_POSITION_VECTOR ),

	DEFINE_UTLVECTOR( CBaseHelicopter, m_hEntitiesPushedByWash, FIELD_EMBEDDED ),

	// Inputs
	DEFINE_INPUTFUNC( CBaseHelicopter, FIELD_VOID, "Activate", InputActivate),
	DEFINE_INPUTFUNC( CBaseHelicopter, FIELD_VOID, "GunOn", InputGunOn ),
	DEFINE_INPUTFUNC( CBaseHelicopter, FIELD_VOID, "GunOff", InputGunOff ),
	DEFINE_INPUTFUNC( CBaseHelicopter, FIELD_VOID, "MissileOn", InputMissileOn ),
	DEFINE_INPUTFUNC( CBaseHelicopter, FIELD_VOID, "MissileOff", InputMissileOff ),

	// Outputs
	DEFINE_OUTPUT(CBaseHelicopter, m_AtTarget,			"AtPathCorner" ),
	DEFINE_OUTPUT(CBaseHelicopter, m_LeaveTarget,		"LeavePathCorner" ),//<<TEMP>> Undone

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CBaseHelicopter, DT_BaseHelicopter )
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseHelicopter::CBaseHelicopter( void )
{
	m_cullBoxMins = vec3_origin;
	m_cullBoxMaxs = vec3_origin;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
// Notes   : Have your derived Helicopter's Spawn() function call this one FIRST
//------------------------------------------------------------------------------
void CBaseHelicopter::Precache( void )
{
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
// Notes   : Have your derived Helicopter's Spawn() function call this one FIRST
//------------------------------------------------------------------------------
void CBaseHelicopter::Spawn( void )
{
	Precache( );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_STEP );
	AddFlag( FL_FLY );

	m_lifeState			= LIFE_ALIVE;

	// motor
	//******
	// All of this stuff is specific to the individual type of aircraft. Handle it yourself.
	//******
	//	m_iAmmoType = g_pGameRules->GetAmmoDef()->Index("MediumRound"); 
	//	SetModel( "models/attack_helicopter.mdl" );
	//	UTIL_SetSize( this, Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	//	UTIL_SetOrigin( this, GetLocalOrigin() );
	//	m_iHealth = 100;
	//	m_flFieldOfView = -0.707; // 270 degrees
	//	InitBoneControllers();
	//	m_iRockets			= 10;
	//	Get the rotor sound started up.

	// This base class assumes the helicopter has no guns or missiles. 
	// Set the appropriate flags in your derived class' Spawn() function.
	m_fHelicopterFlags &= ~BITS_HELICOPTER_MISSILE_ON;
	m_fHelicopterFlags &= ~BITS_HELICOPTER_GUN_ON;

	m_pRotorSound = NULL;
	m_pRotorBlast = NULL;

	m_flCycle = 0;
	ResetSequenceInfo();

	AddFlag( FL_NPC );

	m_flMaxSpeed = BASECHOPPER_MAX_SPEED;
	m_flMaxSpeedFiring = BASECHOPPER_MAX_FIRING_SPEED;
	m_takedamage = DAMAGE_AIM;

	// Don't start up if the level designer has asked the 
	// helicopter to start disabled.
	if ( !(m_spawnflags & SF_AWAITINPUT) )
	{
		Startup();
		SetNextThink( gpGlobals->curtime + 1.0f );
	}

	InitPathingData( BASECHOPPER_LEAD_DISTANCE, BASECHOPPER_MIN_CHASE_DIST_DIFF, BASECHOPPER_AVOID_DIST );

	// Setup collision hull
	ExpandBBox( m_cullBoxMins, m_cullBoxMaxs );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CBaseHelicopter::FireGun( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: custom code to fix collision errors
//-----------------------------------------------------------------------------
bool CBaseHelicopter::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	// Let normal hitbox code handle rays
	if ( mask & CONTENTS_HITBOX )
		return BaseClass::TestCollision( ray, mask, trace );

	return TestCollisionBox( ray, mask, trace, WorldAlignMins() + GetAbsOrigin(), WorldAlignMaxs() + GetAbsOrigin() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHelicopter::SetObjectCollisionBox( void )
{
	// BMCD: Use the conservative boxes for rough culling
	// Set up the conservative culling box
	SetAbsMins( GetAbsOrigin() + m_cullBoxMins );
	SetAbsMaxs( GetAbsOrigin() + m_cullBoxMaxs );
}

//------------------------------------------------------------------------------
// Purpose : The main think function for the helicopters
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::HelicopterThink( void )
{
	SetNextThink( gpGlobals->curtime + HELICOPTER_THINK_INTERVAL );

	// Don't keep this around for more than one frame.
	ClearCondition( COND_ENEMY_DEAD );

	// Animate and dispatch animation events.
	StudioFrameAdvance( );
	DispatchAnimEvents( this );

	PrescheduleThink();

	ShowDamage( );

	// -----------------------------------------------
	// If AI is disabled, kill any motion and return
	// -----------------------------------------------
	if (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI)
	{
		SetAbsVelocity( vec3_origin );
		SetLocalAngularVelocity( vec3_angle );
		SetNextThink( gpGlobals->curtime + HELICOPTER_THINK_INTERVAL );
		return;
	}

	Hunt();

	HelicopterPostThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHelicopter::DoRotorWash( void )
{
	DrawRotorWash( BASECHOPPER_WASH_ALTITUDE, GetAbsOrigin() );
}

//-----------------------------------------------------------------------------
// Purpose:
//
//-----------------------------------------------------------------------------
void CBaseHelicopter::DrawRotorWash( float flAltitude, Vector vecRotorOrigin )
{
	// Shake any ropes nearby
	if ( random->RandomInt( 0, 2 ) == 0 )
	{
		CRopeKeyframe::ShakeRopes( GetAbsOrigin(), flAltitude, 128 );
	}

	if ( m_spawnflags & SF_NOROTORWASH )
		return;

	DoRotorPhysicsPush( vecRotorOrigin, flAltitude );

	// Send down the rotor wash
	Vector vecDown( 0, 0, -1 );
	UTIL_RotorWash( vecRotorOrigin, vecDown, flAltitude );
}

//-----------------------------------------------------------------------------
// Purpose: Push a physics object in our wash. Return false if it's now out of our wash
//-----------------------------------------------------------------------------
bool CBaseHelicopter::DoWashPush( washentity_t *pWash, Vector vecWashOrigin )
{
	if ( !pWash || !pWash->hEntity.Get() )
		return false;

	// Make sure the entity is still within our wash's radius
	CBaseEntity *pEntity = pWash->hEntity;
	Vector vecSpot = pEntity->BodyTarget( vecWashOrigin );
	Vector vecToSpot = ( vecSpot - vecWashOrigin );
	vecToSpot.z = 0;
	float flDist = VectorNormalize( vecToSpot );
	if ( flDist > BASECHOPPER_WASH_RADIUS )
		return false;

	IPhysicsObject *pPhysObject = pEntity->VPhysicsGetObject();

	if ( pPhysObject == NULL )
		return false;

	// Push it away from the center of the wash
	float flMass = pPhysObject->GetMass();
	float flPushTime = (gpGlobals->curtime - pWash->flWashStartTime);
	float flMinPush = BASECHOPPER_WASH_PUSH_MIN * flMass;
	float flMaxPush = BASECHOPPER_WASH_PUSH_MAX * flMass;
	float flWashAmount = min( flMaxPush, RemapVal( flPushTime, 0, BASECHOPPER_WASH_RAMP_TIME, flMinPush, flMaxPush ) );
	Vector vecForce = flWashAmount * vecToSpot * phys_pushscale.GetFloat();
	pEntity->VPhysicsTakeDamage( CTakeDamageInfo( this, this, vecForce, vecWashOrigin, flWashAmount, DMG_BLAST ) );

	// Debug
	if ( g_debug_basehelicopter.GetInt() == BASECHOPPER_DEBUG_WASH )
	{
		NDebugOverlay::Cross3D( pEntity->GetAbsOrigin(), -Vector(4,4,4), Vector(4,4,4), 255, 0, 0, true, 0.1f );
		NDebugOverlay::Line( pEntity->GetAbsOrigin(), pEntity->GetAbsOrigin() + vecForce, 255, 255, 0, true, 0.1f );

		IPhysicsObject *pPhysObject = pEntity->VPhysicsGetObject();
		Msg("Pushed %s (mass %f) with force %f (min %.2f max %.2f) at time %.2f\n", pEntity->GetClassname(), pPhysObject->GetMass(), flWashAmount, flMinPush, flMaxPush, gpGlobals->curtime );
	}

	// If we've pushed this thing for some time, remove it to give us a chance to find lighter things nearby
	if ( flPushTime > 2.0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHelicopter::DoRotorPhysicsPush( Vector &vecRotorOrigin, float flAltitude )
{
	CBaseEntity *pEntity = NULL;
	trace_t tr;

	// First, trace down and find out where the was is hitting the ground
	UTIL_TraceLine( vecRotorOrigin, vecRotorOrigin+Vector(0,0,-flAltitude), (MASK_SOLID_BRUSHONLY|CONTENTS_WATER), NULL, COLLISION_GROUP_NONE, &tr );
	// Always raise the physics origin a bit
	Vector vecPhysicsOrigin = tr.endpos + Vector(0,0,64);

	// Debug
	if ( g_debug_basehelicopter.GetInt() == BASECHOPPER_DEBUG_WASH )
	{
		NDebugOverlay::Cross3D( vecPhysicsOrigin, -Vector(16,16,16), Vector(16,16,16), 0, 255, 255, true, 0.1f );
	}

	// Push entities that we've pushed before, and are still within range
	// Walk backwards because they may be removed if they're now out of range
	int iCount = m_hEntitiesPushedByWash.Count();
	bool bWasPushingObjects = (iCount > 0);
	for ( int i = (iCount-1); i >= 0; i-- )
	{
		if ( !DoWashPush( &(m_hEntitiesPushedByWash[i]), vecPhysicsOrigin ) )
		{
			// Out of range now, so remove
			m_hEntitiesPushedByWash.Remove(i);
		}
	}

	// Any spare slots?
	iCount = m_hEntitiesPushedByWash.Count();
	if ( iCount >= BASECHOPPER_WASH_MAX_OBJECTS )
		return;

	// Find the lightest physics entity below us and add it to our list to push around
	CBaseEntity *pLightestEntity = NULL;
	float flLightestMass = 9999;
	while ((pEntity = gEntList.FindEntityInSphere(pEntity, vecPhysicsOrigin, BASECHOPPER_WASH_RADIUS )) != NULL)
	{
		if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS || (pEntity->VPhysicsGetObject() && !pEntity->IsPlayer()) ) 
		{
			// Make sure it's not already in our wash
			bool bAlreadyPushing = false;
			for ( int i = 0; i < iCount; i++ )
			{
				if ( m_hEntitiesPushedByWash[i].hEntity == pEntity )
				{
					bAlreadyPushing = true;
					break;
				}
			}
			if ( bAlreadyPushing )
				continue;

			// Don't try to push anything too big
			IPhysicsObject *pPhysObject = pEntity->VPhysicsGetObject();
			float flMass = pPhysObject->GetMass();
			if ( flMass > BASECHOPPER_WASH_MAX_MASS )
				continue;
			// Ignore anything bigger than the one we've already found
			if ( flMass > flLightestMass )
				continue;

			Vector vecSpot = pEntity->BodyTarget( vecPhysicsOrigin );
			Vector vecToSpot = ( vecSpot - vecPhysicsOrigin );
			vecToSpot.z = 0;
			float flDist = VectorNormalize( vecToSpot );
			if ( flDist > BASECHOPPER_WASH_RADIUS )
				continue;

			flLightestMass = flMass;
			pLightestEntity = pEntity;
		}
	}

	// New item to push, so add it to our list
	if ( pLightestEntity )
	{
		washentity_t Wash;
		Wash.hEntity = pLightestEntity;
		Wash.flWashStartTime = gpGlobals->curtime;
		m_hEntitiesPushedByWash.AddToTail( Wash );
	}

	// Handle sound.
	// If we just started pushing objects, ramp the blast sound up.
	if ( !bWasPushingObjects && m_hEntitiesPushedByWash.Count() )
	{
		if ( m_pRotorBlast )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundChangeVolume( m_pRotorBlast, 1.0, 1.0 );
		}
	}
	else if ( bWasPushingObjects && m_hEntitiesPushedByWash.Count() == 0 )
	{
		if ( m_pRotorBlast )
		{
			// We just stopped pushing objects, so fade the blast sound out.
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundChangeVolume( m_pRotorBlast, 0, 1.0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHelicopter::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	if ( m_lifeState == LIFE_ALIVE || m_lifeState == LIFE_DYING )
	{
		DoRotorWash();
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::Hunt( void )
{
	UpdateTrackNavigation( );

	if( HasCondition( COND_ENEMY_DEAD ) )
	{
		SetEnemy( NULL );
	}

	// Look for my best enemy. If I change enemies, 
	// be sure and change my prevseen/lastseen timers.
	if( m_lifeState == LIFE_ALIVE )
	{
		GetSenses()->Look( 4092 );

		GetEnemies()->RefreshMemories();
		ChooseEnemy();

		if( HasEnemy() )
		{
			GatherEnemyConditions( GetEnemy() );

			if (FVisible( GetEnemy() ))
			{
				if (m_flLastSeen < gpGlobals->curtime - 2)
				{
					m_flPrevSeen = gpGlobals->curtime;
				}

				m_flLastSeen = gpGlobals->curtime;
				m_vecTargetPosition = GetEnemy()->WorldSpaceCenter();
			}
		}
		else
		{
			// look at where we're going instead
			m_vecTargetPosition = GetDesiredPosition();
		}
	}
	else
	{
		// If we're dead or dying, forget our enemy and don't look for new ones(sjb)
		SetEnemy( NULL );
	}

	if ( 1 )
	{
		Vector targetDir = m_vecTargetPosition - GetAbsOrigin();
		Vector desiredDir = GetDesiredPosition() - GetAbsOrigin();

		VectorNormalize( targetDir ); 
		VectorNormalize( desiredDir ); 

		if ( !IsCrashing() && m_flLastSeen + 5 > gpGlobals->curtime ) //&& DotProduct( targetDir, desiredDir) > 0.25)
		{
			// If we've seen the target recently, face the target.
			//Msg( "Facing Target \n" );
			m_vecDesiredFaceDir = targetDir;
		}
		else
		{
			// Face our desired position.
			// Msg( "Facing Position\n" );
			m_vecDesiredFaceDir = desiredDir;
		}
	}
	else
	{
		// Face the way the path corner tells us to.
		//Msg( "Facing my path corner\n" );
		m_vecDesiredFaceDir = GetGoalOrientation();
	}

	Flight();

	UpdatePlayerDopplerShift( );

	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->curtime, m_flLastSeen, m_flPrevSeen );
	if (m_fHelicopterFlags & BITS_HELICOPTER_GUN_ON)
	{
		//if ( (m_flLastSeen + 1 > gpGlobals->curtime) && (m_flPrevSeen + 2 < gpGlobals->curtime) )
		{
			if (FireGun( ))
			{
				// slow down if we're firing
				if (m_flGoalSpeed > m_flMaxSpeedFiring )
				{
					m_flGoalSpeed = m_flMaxSpeedFiring;
				}
			}
		}
	}

	if (m_fHelicopterFlags & BITS_HELICOPTER_MISSILE_ON)
	{
		AimRocketGun();
	}

	// Finally, forget dead enemies.
	if( GetEnemy() != NULL && (!GetEnemy()->IsAlive() || GetEnemy()->GetFlags() & FL_NOTARGET) )
	{
		SetEnemy( NULL );
	}
}



//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::UpdatePlayerDopplerShift( )
{
	// -----------------------------
	// make rotor, engine sounds
	// -----------------------------
	if (m_iSoundState == 0)
	{
		// Sound startup.
		InitializeRotorSound();
	}
	else
	{
		CBaseEntity *pPlayer = NULL;

		// UNDONE: this needs to send different sounds to every player for multiplayer.	
		// FIXME: this isn't the correct way to find a player!!!
		pPlayer = gEntList.FindEntityByName( NULL, "!player", this );
		if (pPlayer)
		{
			Vector dir = pPlayer->GetLocalOrigin() - GetLocalOrigin();
			VectorNormalize(dir);

#if 1
			float velReceiver = -DotProduct( pPlayer->GetAbsVelocity(), dir );
			float velTransmitter = -DotProduct( GetAbsVelocity(), dir );
			// speed of sound == 13049in/s
			int iPitch = 100 * ((1 - velReceiver / 13049) / (1 + velTransmitter / 13049));
#else
			// This is a bogus doppler shift, but I like it better
			float relV = DotProduct( GetAbsVelocity() - pPlayer->GetAbsVelocity(), dir );
			int iPitch = (int)(100 + relV / 50.0);
#endif

			// clamp pitch shifts
			if (iPitch > 250) 
				iPitch = 250;
			if (iPitch < 50)
				iPitch = 50;

			UpdateRotorSoundPitch( iPitch );
			// Msg( "Pitch:%d\n", iPitch );
		}
		else
		{
			Msg( "Chopper didn't find a player!\n" );
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CBaseHelicopter::Flight( void )
{
	if( GetFlags() & FL_ONGROUND )
	{
		//This would be really bad.
		RemoveFlag( FL_ONGROUND );
	}

	// Generic speed up
	if (m_flGoalSpeed < m_flMaxSpeed)
	{
		m_flGoalSpeed += GetAcceleration();
	}
	
	//NDebugOverlay::Line(GetAbsOrigin(), m_vecDesiredPosition, 0,0,255, true, 0.1);

	// tilt model 5 degrees (why?! sjb)
	QAngle vecAdj = QAngle( 5.0, 0, 0 );

	// estimate where I'll be facing in one seconds
	Vector forward, right, up;
	AngleVectors( GetLocalAngles() + GetLocalAngularVelocity() * 2 + vecAdj, &forward, &right, &up );

	// Vector vecEst1 = GetLocalOrigin() + GetAbsVelocity() + up * m_flForce - Vector( 0, 0, 384 );
	// float flSide = DotProduct( m_vecDesiredPosition - vecEst1, right );
	QAngle angVel = GetLocalAngularVelocity();
	float flSide = DotProduct( m_vecDesiredFaceDir, right );
	if (flSide < 0)
	{
		if (angVel.y < 60)
		{
			angVel.y += 8;
		}
	}
	else
	{
		if (angVel.y > -60)
		{
			angVel.y -= 8;
		}
	}

	angVel.y *= ( 0.98 ); // why?! (sjb)

	// estimate where I'll be in two seconds
	AngleVectors( GetLocalAngles() + angVel * 1 + vecAdj, NULL, NULL, &up );
	Vector vecEst = GetAbsOrigin() + GetAbsVelocity() * 2.0 + up * m_flForce * 20 - Vector( 0, 0, 384 * 2 );

	// add immediate force
	AngleVectors( GetLocalAngles() + vecAdj, &forward, &right, &up );
	
	Vector vecImpulse( 0, 0, 0 );
	vecImpulse.x += up.x * m_flForce;
	vecImpulse.y += up.y * m_flForce;
	vecImpulse.z += up.z * m_flForce;

	// add gravity
	vecImpulse.z -= 38.4; // 32ft/sec
	ApplyAbsVelocityImpulse( vecImpulse );

	float flSpeed = GetAbsVelocity().Length();
	float flDir = DotProduct( Vector( forward.x, forward.y, 0 ), Vector( GetAbsVelocity().x, GetAbsVelocity().y, 0 ) );
	if (flDir < 0)
	{
		flSpeed = -flSpeed;
	}

	float flDist = DotProduct( GetDesiredPosition() - vecEst, forward );

	// float flSlip = DotProduct( GetAbsVelocity(), right );
	float flSlip = -DotProduct( GetDesiredPosition() - vecEst, right );

	// fly sideways
	if (flSlip > 0)
	{
		if (GetLocalAngles().z > -30 && angVel.z > -15)
			angVel.z -= 4;
		else
			angVel.z += 2;
	}
	else
	{
		if (GetLocalAngles().z < 30 && angVel.z < 15)
			angVel.z += 4;
		else
			angVel.z -= 2;
	}

	// These functions contain code Ken wrote that used to be right here as part of the flight model,
	// but we want different helicopter vehicles to have different drag characteristics, so I made
	// them virtual functions (sjb)
	ApplySidewaysDrag( right );
	ApplyGeneralDrag();
	
	// apply power to stay correct height
	// FIXME: these need to be per class variables
#define MAX_FORCE		80
#define FORCE_POSDELTA	12	
#define FORCE_NEGDELTA	8

	if (m_flForce < MAX_FORCE && vecEst.z < GetDesiredPosition().z) 
	{
		m_flForce += FORCE_POSDELTA;
	}
	else if (m_flForce > 30)
	{
		if (vecEst.z > GetDesiredPosition().z) 
			m_flForce -= FORCE_NEGDELTA;
	}
	
	// pitch forward or back to get to target
	//-----------------------------------------
	// Pitch is reversed since Half-Life! (sjb)
	//-----------------------------------------
	if (flDist > 0 && flSpeed < m_flGoalSpeed /* && flSpeed < flDist */ && GetLocalAngles().x + angVel.x < 40)
	{
		// ALERT( at_console, "F " );
		// lean forward
		angVel.x += 12.0;
	}
	else if (flDist < 0 && flSpeed > -50 && GetLocalAngles().x + angVel.x  > -20)
	{
		// ALERT( at_console, "B " );
		// lean backward
		angVel.x -= 12.0;
	}
	else if (GetLocalAngles().x + angVel.x < 0)
	{
		// ALERT( at_console, "f " );
		angVel.x += 4.0;
	}
	else if (GetLocalAngles().x + angVel.x > 0)
	{
		// ALERT( at_console, "b " );
		angVel.x -= 4.0;
	}

	SetLocalAngularVelocity( angVel );
	// ALERT( at_console, "%.0f %.0f : %.0f %.0f : %.0f %.0f : %.0f\n", GetAbsOrigin().x, GetAbsVelocity().x, flDist, flSpeed, GetLocalAngles().x, m_vecAngVelocity.x, m_flForce ); 
	// ALERT( at_console, "%.0f %.0f : %.0f %0.f : %.0f\n", GetAbsOrigin().z, GetAbsVelocity().z, vecEst.z, m_vecDesiredPosition.z, m_flForce ); 
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::InitializeRotorSound( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	if ( m_pRotorSound )
	{
		// Get the rotor sound started up.
		controller.Play( m_pRotorSound, 0.0, 100 );
		controller.SoundChangeVolume(m_pRotorSound, GetRotorVolume(), 2.0);
	}

	if ( m_pRotorBlast )
	{
		// Start the blast sound and then immediately drop it to 0 (starting it at 0 wouldn't start it)
		controller.Play( m_pRotorBlast, 1.0, 100 );
		controller.SoundChangeVolume(m_pRotorBlast, 0, 0.0);
	}

	m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::UpdateRotorSoundPitch( int iPitch )
{
	if (m_pRotorSound)
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangePitch( m_pRotorSound, iPitch, 0.1 );
		controller.SoundChangeVolume( m_pRotorSound, GetRotorVolume(), 0.1 );
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::FlyTouch( CBaseEntity *pOther )
{
	// bounce if we hit something solid
	if ( pOther->GetSolid() == SOLID_BSP) 
	{
		trace_t tr = CBaseEntity::GetTouchTrace( );

		// UNDONE, do a real bounce
		ApplyAbsVelocityImpulse( tr.plane.normal * (GetAbsVelocity().Length() + 200) );
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::CrashTouch( CBaseEntity *pOther )
{
	// only crash if we hit something solid
	if ( pOther->GetSolid() == SOLID_BSP) 
	{
		SetTouch( NULL );
		SetNextThink( gpGlobals->curtime );
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::DyingThink( void )
{
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	SetLocalAngularVelocity( GetLocalAngularVelocity() * 1.02 );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
int CBaseHelicopter::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
#if 0
	// This code didn't port easily. WTF does it do? (sjb)
	if (pevInflictor->m_owner == pev)
		return 0;
#endif

	/*
	if ( (bitsDamageType & DMG_BULLET) && flDamage > 50)
	{
		// clip bullet damage at 50
		flDamage = 50;
	}
	*/

	return BaseClass::OnTakeDamage_Alive( info );
}


//-----------------------------------------------------------------------------
// Purpose: Override base class to add display of fly direction
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CBaseHelicopter::DrawDebugGeometryOverlays(void) 
{
	if (m_pfnThink!= NULL)
	{
		// ------------------------------
		// Draw route if requested
		// ------------------------------
		if (m_debugOverlays & OVERLAY_NPC_ROUTE_BIT)
		{
			NDebugOverlay::Line(GetAbsOrigin(), GetDesiredPosition(), 0,0,255, true, 0);
		}
	}
	BaseClass::DrawDebugGeometryOverlays();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CBaseHelicopter::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	// Take no damage from trace attacks
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseHelicopter::NullThink( void )
{
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.5f );
}


void CBaseHelicopter::Startup( void )
{
	m_flGoalSpeed = m_flInitialSpeed;
	SetThink( HelicopterThink );
	SetTouch( FlyTouch );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CBaseHelicopter::StopLoopingSounds()
{
	// Kill the rotor sounds
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_pRotorSound );
	controller.SoundDestroy( m_pRotorBlast );
	m_pRotorSound = NULL;
	m_pRotorBlast = NULL;

	BaseClass::StopLoopingSounds();
}

void CBaseHelicopter::Event_Killed( const CTakeDamageInfo &info )
{
	m_lifeState			= LIFE_DYING;

	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetGravity( 0.3 );

	StopLoopingSounds();

	UTIL_SetSize( this, Vector( -32, -32, -64), Vector( 32, 32, 0) );
	SetThink( CallDyingThink );
	SetTouch( CrashTouch );

	SetNextThink( gpGlobals->curtime + 0.1f );
	m_iHealth = 0;
	m_takedamage = DAMAGE_NO;

/*
	if (m_spawnflags & SF_NOWRECKAGE)
	{
		m_flNextRocket = gpGlobals->curtime + 4.0;
	}
	else
	{
		m_flNextRocket = gpGlobals->curtime + 15.0;
	}
*/	
	m_OnDeath.FireOutput( info.GetAttacker(), this );
}


void CBaseHelicopter::GibMonster( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Call Startup for a helicopter that's been flagged to start disabled
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputActivate( inputdata_t &inputdata )
{
	if( m_spawnflags & SF_AWAITINPUT )
	{
		Startup();

		// Now clear the spawnflag to protect from
		// subsequent calls.
		m_spawnflags &= ~SF_AWAITINPUT;
	}
}

//------------------------------------------------------------------------------
// Purpose : Turn the gun on
//------------------------------------------------------------------------------
void CBaseHelicopter::InputGunOn( inputdata_t &inputdata )
{
	m_fHelicopterFlags |= BITS_HELICOPTER_GUN_ON;
}

//-----------------------------------------------------------------------------
// Purpose: Turn the gun off
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputGunOff( inputdata_t &inputdata )
{
	m_fHelicopterFlags &= ~BITS_HELICOPTER_GUN_ON;
}

//------------------------------------------------------------------------------
// Purpose : Turn the missile on
//------------------------------------------------------------------------------
void CBaseHelicopter::InputMissileOn( inputdata_t &inputdata )
{
	m_fHelicopterFlags |= BITS_HELICOPTER_MISSILE_ON;
}

//-----------------------------------------------------------------------------
// Purpose: Turn the missile off
//-----------------------------------------------------------------------------
void CBaseHelicopter::InputMissileOff( inputdata_t &inputdata )
{
	m_fHelicopterFlags &= ~BITS_HELICOPTER_MISSILE_ON;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CBaseHelicopter::ApplySidewaysDrag( const Vector &vecRight )
{
	Vector vecNewVelocity = GetAbsVelocity();
	vecNewVelocity.x *= 1.0 - fabs( vecRight.x ) * 0.05;
	vecNewVelocity.y *= 1.0 - fabs( vecRight.y ) * 0.05;
	vecNewVelocity.z *= 1.0 - fabs( vecRight.z ) * 0.05;
	SetAbsVelocity( vecNewVelocity );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CBaseHelicopter::ApplyGeneralDrag( void )
{
	Vector vecNewVelocity = GetAbsVelocity();
	vecNewVelocity *= 0.995;
	SetAbsVelocity( vecNewVelocity );
}
	

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
bool CBaseHelicopter::ChooseEnemy( void )
{
	// See if there's a new enemy.
	CBaseEntity *pNewEnemy;

	pNewEnemy = BestEnemy();

	if( ( pNewEnemy != GetEnemy() ) && pNewEnemy != NULL )
	{
		//New enemy! Clear the timers and set conditions.
 		SetCondition(COND_NEW_ENEMY);
		SetEnemy( pNewEnemy );
		m_flLastSeen = m_flPrevSeen = gpGlobals->curtime;
		return true;
	}
	else
	{
		ClearCondition( COND_NEW_ENEMY );
		return false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CBaseHelicopter::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	// -------------------
	// If enemy is dead
	// -------------------
	if ( !pEnemy->IsAlive() )
	{
		SetCondition( COND_ENEMY_DEAD );
		ClearCondition( COND_SEE_ENEMY );
		ClearCondition( COND_ENEMY_OCCLUDED );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ExpandBBox(Vector &vecMins, Vector &vecMaxs)
{
	// expand for *any* rotation
	float maxval = 0;
	for (int i = 0; i < 3; i++)
	{
		float v = fabs( vecMins[i]);
		if (v > maxval)
			maxval = v;

		v = fabs( vecMaxs[i]);
		if (v > maxval)
			maxval = v;
	}

	vecMins.Init(-maxval, -maxval, -maxval);
	vecMaxs.Init(maxval, maxval, maxval);
}
