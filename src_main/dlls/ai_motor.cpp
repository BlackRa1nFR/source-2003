//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "animation.h"		// for NOMOTION

#include "ai_motor.h"
#include "ai_basenpc.h"
#include "ai_localnavigator.h"
#include "ai_moveprobe.h"

extern float	GetFloorZ(const Vector &origin);

//-----------------------------------------------------------------------------

// Use these functions to set breakpoints to find out where movement is failing
#ifdef DEBUG
void DebugNoteMovementFailure()
{
}

// a place to put breakpoints
#pragma warning(push)
#pragma warning(disable:4189)
AIMoveResult_t DbgResult( AIMoveResult_t result )
{
	if ( result < AIMR_OK )
	{
		int breakHere = 1;
	}

	switch ( result )
	{
		case AIMR_BLOCKED_ENTITY:
			return AIMR_BLOCKED_ENTITY;
		case AIMR_BLOCKED_WORLD:
			return AIMR_BLOCKED_WORLD;
		case AIMR_BLOCKED_NPC:
			return AIMR_BLOCKED_NPC;
		case AIMR_ILLEGAL:
			return AIMR_ILLEGAL;
		case AIMR_OK:
			return AIMR_OK;
		case AIMR_CHANGE_TYPE:
			return AIMR_CHANGE_TYPE;
	};
	return AIMR_ILLEGAL;
};
#endif

//-----------------------------------------------------------------------------
//
// class CAI_Motor
//

BEGIN_SIMPLE_DATADESC( CAI_Motor )
	//							m_flMoveInterval	(think transient)
  	DEFINE_FIELD( CAI_Motor,	m_IdealYaw,		FIELD_FLOAT ),
  	DEFINE_FIELD( CAI_Motor,	m_YawSpeed,		FIELD_FLOAT ),
  	DEFINE_FIELD( CAI_Motor,	m_vecVelocity,	FIELD_VECTOR ),
  	DEFINE_FIELD( CAI_Motor,	m_vecAngularVelocity, FIELD_VECTOR ),
	// TODO						m_facingQueue
	//							m_pMoveProbe
END_DATADESC()

//-----------------------------------------------------------------------------

CAI_Motor::CAI_Motor(CAI_BaseNPC *pOuter)
 :	CAI_Component( pOuter )
{
	m_flMoveInterval = 0;

	m_IdealYaw = 0;
	m_YawSpeed = 0;
	m_vecVelocity = Vector( 0, 0, 0 );
	m_pMoveProbe = NULL;
}

//-----------------------------------------------------------------------------

CAI_Motor::~CAI_Motor() 
{
}

//-----------------------------------------------------------------------------

void CAI_Motor::Init( IAI_MovementSink *pMovementServices )	
{ 
	CAI_ProxyMovementSink::Init( pMovementServices );
	m_pMoveProbe = GetOuter()->GetMoveProbe(); // @TODO (toml 03-30-03): this is a "bad" way to grab this pointer. Components should have an explcit "init" phase.
}

//-----------------------------------------------------------------------------
// Step iteratively toward a destination position
//-----------------------------------------------------------------------------
AIMotorMoveResult_t CAI_Motor::MoveGroundStep( const Vector &newPos, CBaseEntity *pMoveTarget, float yaw, bool bAsFarAsCan, AIMoveTrace_t *pTraceResult )
{
	// By definition, this will produce different results than GroundMoveLimit() 
	// because there's no guarantee that it will step exactly one step 

	// See how far toward the new position we can step...
	// But don't actually test for ground geometric validity;
	// if it isn't valid, there's not much we can do about it
	AIMoveTrace_t moveTrace;
	GetMoveProbe()->TestGroundMove( GetLocalOrigin(), newPos, MASK_NPCSOLID, AITGM_IGNORE_FLOOR, &moveTrace );
	if ( pTraceResult )
	{
		*pTraceResult = moveTrace;
	}

	bool bHitTarget = (moveTrace.pObstruction && (pMoveTarget == moveTrace.pObstruction ));

	// Move forward either if there was no obstruction or if we're told to
	// move as far as we can, regardless
	bool bIsBlocked = IsMoveBlocked(moveTrace.fStatus);
	if ( !bIsBlocked || bAsFarAsCan || bHitTarget )
	{
		// The true argument here causes it to touch all triggers
		// in the volume swept from the previous position to the current position
		UTIL_SetOrigin(GetOuter(), moveTrace.vEndPosition, true);
		
		// skip tiny steps, but notify the shadow object of any large steps
		if ( moveTrace.flStepUpDistance > 0.1f )
		{
			float height = clamp( moveTrace.flStepUpDistance, 0, StepHeight() );
			IPhysicsObject *pPhysicsObject = GetOuter()->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				IPhysicsShadowController *pShadow = pPhysicsObject->GetShadowController();
				if ( pShadow )
				{
					pShadow->StepUp( height );
				}
			}
		}
		if ( yaw != -1 )
		{
			QAngle angles = GetLocalAngles();
			angles.y = yaw;
			SetLocalAngles( angles );
		}
		if ( bHitTarget )
			return AIM_PARTIAL_HIT_TARGET;
			
		if ( !bIsBlocked )
			return AIM_SUCCESS;
			
		if ( moveTrace.fStatus == AIMR_BLOCKED_NPC )
			return AIM_PARTIAL_HIT_NPC;

		return AIM_PARTIAL_HIT_WORLD;
	}
	return AIM_FAILED;
}


//-----------------------------------------------------------------------------
// Purpose:	Motion for climbing
// Input  :
// Output :	Returns bits (MoveStatus_b) regarding the move status
//-----------------------------------------------------------------------------

void CAI_Motor::MoveClimbStart()
{
	// @Note (toml 06-11-02): the following code is somewhat suspect. It
	// originated in CAI_BaseNPC::MoveClimb() from early June 2002
	// At the very least, state should be restored to original, not
	// slammed.
	//
	//	 -----Original Message-----
	//	From: 	Jay Stelly  
	//	Sent:	Monday, June 10, 2002 3:57 PM
	//	To:	Tom Leonard
	//	Subject:	RE: 
	//
	//	yes.
	//
	//	Also, there is some subtlety to using movetype.  I think in 
	//	general we want to keep things in MOVETYPE_STEP because it 
	//	implies a bunch of things in the external game physics 
	//	simulator.  There is a flag FL_FLY we use to 
	//	disable gravity on MOVETYPE_STEP characters.
	//
	//	>  -----Original Message-----
	//	> From: 	Tom Leonard  
	//	> Sent:	Monday, June 10, 2002 3:55 PM
	//	> To:	Jay Stelly
	//	> Subject:	
	//	> 
	//	> Should I worry at all that the following highlighted bits of 
	//	> code are not reciprocal for all state, and furthermore, stomp 
	//	> other state?
	SetMoveType( MOVETYPE_FLY );		// No gravity
	SetSolid( SOLID_BBOX );
	SetGravity( 0.0 );
	RemoveEntFlag( FL_ONGROUND );
	UTIL_Relink( GetOuter() );
}

AIMoveResult_t CAI_Motor::MoveClimbExecute( const Vector &climbDest, float yaw)
{
	const float climbSpeed = 100.0;

	Vector moveDir = climbDest - GetLocalOrigin();
	float flDist = VectorNormalize( moveDir );
	SetSmoothedVelocity( moveDir * climbSpeed );

	if ( flDist < climbSpeed * GetMoveInterval() )
	{
		if (flDist <= 1e-2)
			flDist = 0;

		const float climbTime = flDist / climbSpeed;
		
		SetMoveInterval( GetMoveInterval() - climbTime );
		SetLocalOrigin( climbDest );

		return AIMR_CHANGE_TYPE;
	}
	else
	{
		SetMoveInterval( 0 );
	}

	// --------------------------------------------
	// Turn to face the climb
	// --------------------------------------------
	SetIdealYawAndUpdate( yaw );

	return AIMR_OK;
}

void CAI_Motor::MoveClimbStop()
{
	SetMoveType( MOVETYPE_STEP );
	SetSmoothedVelocity( vec3_origin );
	SetGravity( 1.0 );
}

//-----------------------------------------------------------------------------
// Purpose:	Motion for jumping
// Input  :
// Output : Returns bits (MoveStatus_b) regarding the move status
//-----------------------------------------------------------------------------

void CAI_Motor::MoveJumpStart( const Vector &velocity )
{
	SetSmoothedVelocity( velocity );
	SetGravity( 1.0 );
	RemoveEntFlag( FL_ONGROUND );

	SetActivity( ACT_JUMP );

	SetIdealYawAndUpdate( velocity );
}

int CAI_Motor::MoveJumpExecute( )
{
	// needs to detect being hit
	SetIdealYawAndUpdate( GetSmoothedVelocity() );

	SetActivity( ACT_GLIDE );

	// use all the time
	SetMoveInterval( 0 );

	return AIMR_OK;
}

void CAI_Motor::MoveJumpStop()
{
	SetSmoothedVelocity( Vector(0,0,0) );
	SetActivity( ACT_LAND );
}

//-----------------------------------------------------------------------------

float CAI_Motor::GetIdealSpeed() const
{
	return GetOuter()->GetIdealSpeed();
}

//-----------------------------------------------------------------------------

// how long does it take to get to full accell, decell
#define ACCELTIME	0.5

// how far will I go?
float CAI_Motor::MinStoppingDist( void )
{
	// FIXME: should this be a constant rate or a constant time like it is now?
	float flDecelRate = IdealVelocity( ) / ACCELTIME;

	if (flDecelRate > 0.0)
	{
		// assuming linear deceleration, how long till my V hits 0?
		float t = GetCurSpeed() / flDecelRate;
		// and how far will I travel? (V * t - 1/2 A t^2)
		float flDist = 0.5 * GetCurSpeed() * t;
	
		// this should always be some reasonable non-zero distance
		if (flDist > 10.0)
			return flDist;
		return 10.0;
	}
	return 10.0;
}


//-----------------------------------------------------------------------------
// Purpose: how fast should I be going ideally
//-----------------------------------------------------------------------------
float CAI_Motor::IdealVelocity( void )
{
	// FIXME: this should be a per-entity setting so run speeds are not based on animation speeds
	return GetIdealSpeed() * GetPlaybackRate();
}

//-----------------------------------------------------------------------------

void CAI_Motor::MoveStop()
{ 
	memset( &m_vecVelocity, 0, sizeof(m_vecVelocity) ); 
	GetOuter()->GetLocalNavigator()->ResetMoveCalculations();
}

//-----------------------------------------------------------------------------
// Purpose: what linear accel/decel rate do I need to hit V1 in d distance?
//-----------------------------------------------------------------------------
float DeltaV( float v0, float v1, float d )
{
	return 0.5 * (v1 * v1 - v0 * v0 ) / d;
}

//-----------------------------------------------------------------------------
// Purpose: Move the npc to the next location on its route.
// Input  : vecDir - Normalized vector indicating the direction of movement.
//			flDistance - distance to move
//			flInterval - Time interval for this movement.
//			flGoalDistance - target distance
//			flGoalVelocity - target velocity
//-----------------------------------------------------------------------------

AIMotorMoveResult_t CAI_Motor::MoveGroundExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult )
{
	// --------------------------------------------
	// turn in the direction of movement
	// --------------------------------------------
	MoveFacing( move );

	// --------------------------------------------
	float flNewSpeed = GetIdealSpeed();

	// --------------------------------------------
	// calculate actual travel distance
	// --------------------------------------------

	// assuming linear acceleration, how far will I travel?
	float flTotal = 0.5 * (GetCurSpeed() + flNewSpeed) * GetMoveInterval();

	// can I move farther in this interval than I'm supposed to?
	if ( flTotal > move.maxDist )
	{
		if ( !(move.flags & AILMG_CONSUME_INTERVAL) )
		{
			// only use a portion of the time interval
			SetMoveInterval( GetMoveInterval() * (1 - move.maxDist / flTotal) );
		}
		else
			SetMoveInterval( 0 );
		flTotal = move.maxDist;
	}
	else
	{
		// use all the time
		SetMoveInterval( 0 );
	}

	SetMoveVel( move.dir * flNewSpeed );

	// --------------------------------------------
	// walk the distance
	// --------------------------------------------
	AIMotorMoveResult_t result = AIM_SUCCESS;
	if ( flTotal > 0.0 )
	{
		Vector vecFrom = GetLocalOrigin();
		Vector vecTo = vecFrom + move.dir * flTotal;
		
		result = MoveGroundStep( vecTo, move.pMoveTarget, -1, true, pTraceResult );

		if ( result == AIM_FAILED )
			MoveStop();
	}
	else if ( !OnMoveStalled( move ) )
	{
		result = AIM_FAILED;
	}


	return result;
}


//-----------------------------------------------------------------------------
// Purpose: Move the npc to the next location on its route.
// Input  : pTargetEnt - 
//			vecDir - Normalized vector indicating the direction of movement.
//			flInterval - Time interval for this movement.
//-----------------------------------------------------------------------------

AIMotorMoveResult_t CAI_Motor::MoveFlyExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult )
{
	// turn in the direction of movement
	MoveFacing( move );

	// calc accel/decel rates
	float flNewSpeed = GetIdealSpeed();
	SetMoveVel( move.dir * flNewSpeed );

	float flTotal = 0.5 * (GetCurSpeed() + flNewSpeed) * GetMoveInterval();

	float distance = move.maxDist;

	// can I move farther in this interval than I'm supposed to?
	if (flTotal > distance)
	{
		// only use a portion of the time interval
		SetMoveInterval( GetMoveInterval() * (1 - distance / flTotal) );
		flTotal = distance;
	}
	else
	{
		// use all the time
		SetMoveInterval( 0 );
	}

	Vector vecStart, vecEnd;
	vecStart = GetLocalOrigin();
	VectorMA( vecStart, flTotal, move.dir, vecEnd );

	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( NAV_FLY, vecStart, vecEnd, MASK_NPCSOLID, NULL, &moveTrace );
	if ( pTraceResult )
		*pTraceResult = moveTrace;
	
	// Check for total blockage
	if (fabs(moveTrace.flDistObstructed - flTotal) <= 1e-1)
	{
		// But if we bumped into our target, then we succeeded!
		if ( move.pMoveTarget && (moveTrace.pObstruction == move.pMoveTarget) )
			return AIM_PARTIAL_HIT_TARGET;

		return AIM_FAILED;
	}

	// The true argument here causes it to touch all triggers
	// in the volume swept from the previous position to the current position
	UTIL_SetOrigin(GetOuter(), moveTrace.vEndPosition, true);

	return (IsMoveBlocked(moveTrace.fStatus)) ? AIM_PARTIAL_HIT_WORLD : AIM_SUCCESS;
}


//-----------------------------------------------------------------------------
// Purpose: turn in the direction of movement
// Output :
//-----------------------------------------------------------------------------

void CAI_Motor::MoveFacing( const AILocalMoveGoal_t &move )
{
	if ( GetOuter()->OverrideMoveFacing( move, GetMoveInterval() ) )
		return;

	// required movement direction
	float flMoveYaw = UTIL_VecToYaw( move.dir );

	int nSequence = GetSequence();
	float fSequenceMoveYaw = GetSequenceMoveYaw( nSequence );
	if ( fSequenceMoveYaw == NOMOTION )
	{
		fSequenceMoveYaw = 0;
	}

	if (!HasPoseParameter( nSequence, "move_yaw" ))
	{
		SetIdealYawAndUpdate( UTIL_AngleMod( flMoveYaw - fSequenceMoveYaw ) );
	}
	else
	{
		// FIXME: move this up to navigator so that path goals can ignore these overrides.
		Vector dir;
		float flInfluence = GetFacingDirection( dir );
		dir = move.facing * (1 - flInfluence) + dir * flInfluence;
		VectorNormalize( dir );

		// ideal facing direction
		float idealYaw = UTIL_AngleMod( UTIL_VecToYaw( dir ) );
		
		// FIXME: facing has important max velocity issues
		SetIdealYawAndUpdate( idealYaw );	

		// find movement direction to compensate for not being turned far enough
		float flDiff = UTIL_AngleDiff( flMoveYaw, GetLocalAngles().y );
		SetPoseParameter( "move_yaw", flDiff );
		/*
		if ((GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			Msg( "move %.1f : diff %.1f  : ideal %.1f\n", flMoveYaw, flDiff, m_IdealYaw );
		}
		*/
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the ideal yaw and run the current or specified timestep 
//			worth of rotation.
//-----------------------------------------------------------------------------

void CAI_Motor::SetIdealYawAndUpdate( float idealYaw, float yawSpeed)
{
	SetIdealYaw( idealYaw );
	if (yawSpeed == AI_CALC_YAW_SPEED)
		RecalculateYawSpeed();
	else if (yawSpeed != AI_KEEP_YAW_SPEED)
		SetYawSpeed( yawSpeed );
	UpdateYaw(-1);
}


//-----------------------------------------------------------------------------

void CAI_Motor::RecalculateYawSpeed() 
{ 
	SetYawSpeed( CalcYawSpeed() ); 
}

//-----------------------------------------------------------------------------

float AI_ClampYaw( float yawSpeedPerSec, float current, float target, float time )
{
	if (current != target)
	{
		float speed = yawSpeedPerSec * time;
		float move = target - current;

		if (target > current)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}

		if (move > 0)
		{// turning to the npc's left
			if (move > speed)
				move = speed;
		}
		else
		{// turning to the npc's right
			if (move < -speed)
				move = -speed;
		}
		
		return UTIL_AngleMod(current + move);
	}
	
	return target;
}

//-----------------------------------------------------------------------------
// Purpose: Turns a npc towards its ideal yaw.
// Input  : yawSpeed - Yaw speed in degrees per 1/10th of a second.
//			flInterval - Time interval to turn, -1 uses time since last think.
// Output : Returns the number of degrees turned.
//-----------------------------------------------------------------------------

void CAI_Motor::UpdateYaw( int yawSpeed )
{
	float ideal, current, newYaw;
	
	if ( yawSpeed = -1 )
		yawSpeed = GetYawSpeed();
		
	// NOTE: GetIdealYaw() will never exactly be reached because UTIL_AngleMod
	// also truncates the angle to 16 bits of resolution. So lets truncate it here.
	current = UTIL_AngleMod( GetLocalAngles().y );
	ideal = UTIL_AngleMod( GetIdealYaw() );
	
	newYaw = AI_ClampYaw( (float)yawSpeed * 10.0, current, ideal, gpGlobals->curtime - GetLastThink() );
		
	if (newYaw != current)
	{
		QAngle angles = GetLocalAngles();
		angles.y = newYaw;
		SetLocalAngles( angles );

		// ENTITY MUST BE RELINKED to recompute absangles
		engine->RelinkEntity( GetEdict(), false );
	}
}


//=========================================================
// DeltaIdealYaw - returns the difference ( in degrees ) between
// npc's current yaw and ideal_yaw
//
// Positive result is left turn, negative is right turn
//=========================================================
float CAI_Motor::DeltaIdealYaw ( void )
{
	float	flCurrentYaw;

	flCurrentYaw = UTIL_AngleMod( GetLocalAngles().y );

	if ( flCurrentYaw == GetIdealYaw() )
	{
		return 0;
	}


	return UTIL_AngleDiff( GetIdealYaw(), flCurrentYaw );
}


//-----------------------------------------------------------------------------

void CAI_Motor::SetIdealYawToTarget( const Vector &target )
{ 
	SetIdealYaw( CalcIdealYaw( target ) ); 
}

//-----------------------------------------------------------------------------

void CAI_Motor::SetIdealYawToTargetAndUpdate( const Vector &target, float yawSpeed )
{ 
	SetIdealYawAndUpdate( CalcIdealYaw( target ), yawSpeed ); 
}


//-----------------------------------------------------------------------------
// Purpose: Keep track of multiple objects that the npc is interested in facing
//-----------------------------------------------------------------------------
void CAI_Motor::AddFacingTarget( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp )
{
	m_facingQueue.Add( pTarget, flImportance, flDuration, flRamp );
}


void CAI_Motor::AddFacingTarget( const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	m_facingQueue.Add( vecPosition, flImportance, flDuration, flRamp );
}

void CAI_Motor::AddFacingTarget( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp )
{
	m_facingQueue.Add( pTarget, vecPosition, flImportance, flDuration, flRamp );
}


float CAI_Motor::GetFacingDirection( Vector &vecDir )
{
	float flTotalInterest = 0.0;
	vecDir = Vector( 0, 0, 0 );

	int i;

	// clean up facing targets
	for (i = 0; i < m_facingQueue.Count();)
	{
		if (!m_facingQueue[i].IsActive())
		{
			m_facingQueue.Remove( i );
		}
		else
		{
			i++;
		}
	}

	for (i = 0; i < m_facingQueue.Count(); i++)
	{
		float flInterest = m_facingQueue[i].Interest( );
		Vector tmp = m_facingQueue[i].GetPosition() - GetAbsOrigin();

		// NDebugOverlay::Line( m_facingQueue[i].GetPosition(), GetAbsOrigin(), 255, 0, 0, false, 0.1 );

		VectorNormalize( tmp );

		vecDir = vecDir * (1 - flInterest) + tmp * flInterest;

		flTotalInterest = (1 - (1 - flTotalInterest) * (1 - flInterest));

		VectorNormalize( vecDir );
	}

	return flTotalInterest;
}


//-----------------------------------------------------------------------------

AIMoveResult_t CAI_Motor::MoveNormalExecute( const AILocalMoveGoal_t &move )
{
	AI_PROFILE_SCOPE(CAI_Motor_MoveNormalExecute);
	
	// --------------------------------

	AIMotorMoveResult_t fMotorResult;
	AIMoveTrace_t 		moveTrace;
	
	if ( move.navType == NAV_GROUND )
	{
		fMotorResult = MoveGroundExecute( move, &moveTrace );
	}
	else
	{
		Assert( move.navType == NAV_FLY );
		fMotorResult = MoveFlyExecute( move, &moveTrace );
	}

	static AIMoveResult_t moveResults[] = 
	{
		AIMR_ILLEGAL,	                         // AIM_FAILED
		AIMR_OK,                                 // AIM_SUCCESS
		AIMR_BLOCKED_NPC,						 // AIM_PARTIAL_HIT_NPC
		AIMR_BLOCKED_WORLD,                      // AIM_PARTIAL_HIT_WORLD
		AIMR_BLOCKED_WORLD,                      // AIM_PARTIAL_HIT_TARGET
	};
	Assert( ARRAYSIZE( moveResults ) == AIM_NUM_RESULTS && fMotorResult >= 0 && fMotorResult <= ARRAYSIZE( moveResults ) );
	
	AIMoveResult_t result = moveResults[fMotorResult];
	
	if ( result != AIMR_OK )
	{
		OnMoveExecuteFailed( move, moveTrace, fMotorResult, &result );
		SetMoveInterval( 0 ); // always consume interval on failure, even if overridden by OnMoveExecuteFailed()
	}
	
	return DbgResult( result );
}

//-----------------------------------------------------------------------------
// Purpose: Look ahead my stopping distance, or at least my hull width
//-----------------------------------------------------------------------------
float CAI_Motor::MinCheckDist( void )
{
	// Take the groundspeed into account
	float flMoveDist = GetMoveInterval() * GetIdealSpeed();
	float flMinDist = max( MinStoppingDist(), flMoveDist);
	if ( flMinDist < GetHullWidth() )
		flMinDist = GetHullWidth();
	return flMinDist;
}


//-----------------------------------------------------------------------------

void CAI_Motor::SetSmoothedVelocity(const Vector &vecVelocity)
{
	GetOuter()->SetAbsVelocity(vecVelocity);
}

Vector CAI_Motor::GetSmoothedVelocity()
{
	return GetOuter()->GetSmoothedVelocity();
}

float CAI_Motor::StepHeight() const
{
	return GetOuter()->StepHeight();
}

bool CAI_Motor::CanStandOn( CBaseEntity *pSurface ) const
{
	return GetOuter()->CanStandOn( pSurface );
}

float CAI_Motor::CalcIdealYaw( const Vector &vecTarget )
{
	return GetOuter()->CalcIdealYaw( vecTarget );
}

float CAI_Motor::SetBoneController( int iController, float flValue )
{
	return GetOuter()->SetBoneController( iController, flValue );
}

float CAI_Motor::GetSequenceMoveYaw( int iSequence )
{
	return GetOuter()->GetSequenceMoveYaw( iSequence );
}

float CAI_Motor::GetPlaybackRate()
{
	return GetOuter()->m_flPlaybackRate;
}

float CAI_Motor::SetPoseParameter( const char *szName, float flValue )
{
	return GetOuter()->SetPoseParameter( szName, flValue );
}

float CAI_Motor::GetPoseParameter( const char *szName )
{
	return GetOuter()->GetPoseParameter( szName );
}

bool CAI_Motor::HasPoseParameter( int iSequence, const char *szName )
{
	return GetOuter()->HasPoseParameter( iSequence, szName );
}

void CAI_Motor::SetMoveType( MoveType_t val, MoveCollide_t moveCollide )
{
	GetOuter()->SetMoveType( val, moveCollide );
}

//=============================================================================

