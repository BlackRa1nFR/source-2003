//========= Copyright (c) 1996-2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================


#include "cbase.h"

#include "ai_basehumanoid.h"
#include "ai_basehumanoid_movement.h"
#include "ai_route.h"


//-----------------------------------------------------------------------------
//
// class CAI_HumanoidNavigator
//


void CAI_HumanoidNavigator::SetActivity( Activity NewActivity )
{
	/*
	if (m_bResetActivity)
	{
		CAI_Navigator::SetActivity( NewActivity );
		m_bResetActivity = false;
	}
	*/
	CAI_Navigator::SetActivity( NewActivity );
}


AIMoveResult_t CAI_HumanoidNavigator::MoveNormal()
{
	if (!PreMove())
		return AIMR_ILLEGAL;
	
	// --------------------------------
	
	AIMoveResult_t result = AIMR_ILLEGAL;
	
	if ( MoveUpdateWaypoint( &result ) )
		return result;
	
	// --------------------------------

	// Set activity to be the Navigation activity
	float		preMoveSpeed		= GetIdealSpeed();
	Activity	preMoveActivity		= GetActivity();
	int			nPreMoveSequence	= GetOuter()->GetSequence(); // this is an unfortunate necessity to ensure setting back the activity picks the right one if it had been sticky
	Vector		vStart				= GetAbsOrigin();

	// --------------------------------

	m_bResetActivity = true;
	SetActivity( GetMovementActivity() );
	m_bResetActivity = true;

	/*
	if ( GetIdealSpeed() <= 0.0f )
	{
		if ( GetActivity() == ACT_TRANSITION )
			return AIMR_OK;
		Msg( "%s moving with speed <= 0 (%s)\n", GetEntClassname(), GetOuter()->GetSequenceName( GetSequence() ) );
	}
	*/
			
	// --------------------------------
	
	AILocalMoveGoal_t move;
	
	MoveCalcBaseGoal( &move );

	result = MoveEnact( move );
	
	// --------------------------------
	// If we didn't actually move, but it was a success (i.e., blocked but within tolerance), make sure no visual artifact
	if ( result == AIMR_OK && preMoveSpeed < 0.01 )
	{
		if ( ( GetAbsOrigin() - vStart ).Length() < 0.01 )
		{
			GetOuter()->SetSequence( nPreMoveSequence );
			SetActivity( preMoveActivity );
		}
	}

	// --------------------------------
	
	if ( result == AIMR_OK && !m_fPeerMoveWait )
	{
		OnClearPath();
	}

	// --------------------------------

	return result;
}




//-----------------------------------------------------------------------------
//
// class CAI_HumanoidMotor
//

BEGIN_SIMPLE_DATADESC( CAI_HumanoidMotor )
  	// DEFINE_FIELD( CAI_HumanoidMotor,	m_iStandLayer,		FIELD_INTEGER ),
END_DATADESC()


//-------------------------------------

void CAI_HumanoidMotor::MoveStop()
{ 
	CAI_Motor::MoveStop();

	if (m_iStandLayer != -1)
	{
		GetOuter()->RemoveLayer( m_iStandLayer, 0.2, 0.2 );
		m_iStandLayer = -1;
	}
}


AIMotorMoveResult_t CAI_HumanoidMotor::MoveGroundExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult )
{
	// --------------------------------------------
	// turn in the direction of movement
	// --------------------------------------------

	GetOuter()->BuildMoveScript( 0 );

	int i;

	// FIXME: move this, facing, and the animations to their own member functions, they're complicated enough that it's confusing for all of them to be here

	// interpolate desired speed, calc actual distance traveled
	float flNewSpeed = GetCurSpeed();
	float flTotalDist = 0;
	float t = GetMoveInterval();
	for (i = 0; i < GetOuter()->m_scriptMove.Count()-1; i++)
	{
		if (t < GetOuter()->m_scriptMove[i].flTime)
		{
			// get new velocity
			float a = t / GetOuter()->m_scriptMove[i].flTime;
			flNewSpeed = GetOuter()->m_scriptMove[i].flMaxVelocity * (1 - a) + GetOuter()->m_scriptMove[i+1].flMaxVelocity * a;
			
			// get distance traveled over this entry
			flTotalDist += (GetOuter()->m_scriptMove[i].flMaxVelocity + flNewSpeed) * 0.5 * t; 
			break;
		}
		else
		{
			// used all of entries time, get entries total movement
			flTotalDist += GetOuter()->m_scriptMove[i].flDist;
			t -= GetOuter()->m_scriptMove[i].flTime;
		}
	}

	// interpolate desired angle
	float flNewYaw = GetOuter()->GetAbsAngles().y;
	t = GetMoveInterval();
	for (i = 0; i < GetOuter()->m_scriptTurn.Count()-1; i++)
	{
		if (t < GetOuter()->m_scriptTurn[i].flTime)
		{
			// get new direction
			float a = t / GetOuter()->m_scriptTurn[i].flTime;
			float deltaYaw = UTIL_AngleDiff( GetOuter()->m_scriptTurn[i+1].flYaw, GetOuter()->m_scriptTurn[i].flYaw );
			flNewYaw = UTIL_AngleMod( GetOuter()->m_scriptTurn[i].flYaw + a * deltaYaw );
			break;
		}
		else
		{
			t -= GetOuter()->m_scriptTurn[i].flTime;
		}
	}

	// get facing based on movement yaw
	AILocalMoveGoal_t move2 = move;
	move2.facing = UTIL_YawToVector( flNewYaw );

	// turn in the direction needed
	MoveFacing( move2 );

	// reset actual "sequence" ground speed based current movement sequence, orientation
	GetOuter()->m_flGroundSpeed = GetOuter()->GetSequenceGroundSpeed( GetOuter()->GetSequence());

	/*
	if (1 || flNewSpeed > GetOuter()->GetIdealSpeed())
	{
		// Msg( "%6.2f : Speed %.1f : %.1f (%.1f) :  %d\n", gpGlobals->curtime, flNewSpeed, move.maxDist, move.transitionDist, GetOuter()->m_pHintNode != NULL );
		// Msg( "%6.2f : Speed %.1f : %.1f\n", gpGlobals->curtime, flNewSpeed, GetOuter()->GetIdealSpeed() );
	}
	*/

	// insert ideal layers
	// FIXME: needs full transitions, as well as starting vs stopping sequences, leaning, etc.
	if (m_iStandLayer == -1)
	{
		// int nSequence = GetOuter()->SelectWeightedSequence( GetOuter()->GetStoppedActivity() );
		// int nSequence = GetOuter()->SelectWeightedSequence( ACT_COVER_LOW );
		int nSequence = GetOuter()->GetNavigator()->GetArrivalSequence();
		if (nSequence == ACT_INVALID)
		{
			nSequence = GetOuter()->SelectWeightedSequence( ACT_IDLE );
		}
 
		m_iStandLayer = GetOuter()->AddLayeredSequence( nSequence, 0 );
		GetOuter()->SetLayerWeight( m_iStandLayer, 0.0 );
		GetOuter()->SetLayerPlaybackRate( m_iStandLayer, 0.0 );
		GetOuter()->SetLayerNoRestore( m_iStandLayer, true );
	}
	GetOuter()->SetLayerWeight( m_iStandLayer, 1.0 - (flNewSpeed / GetIdealSpeed()) );
	
	/*
	if ((GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
	{
		Msg( "%6.2f : Speed %.1f : %.1f : %.2f\n", gpGlobals->curtime, flNewSpeed, GetOuter()->GetIdealSpeed(), flNewSpeed / GetIdealSpeed() );
	}
	*/

	// can I move farther in this interval than I'm supposed to?
	if ( flTotalDist > move.maxDist )
	{
		if ( !(move.flags & AILMG_CONSUME_INTERVAL) )
		{
			// only use a portion of the time interval
			SetMoveInterval( GetMoveInterval() * (1 - move.maxDist / flTotalDist) );
		}
		else
			SetMoveInterval( 0 );
		flTotalDist = move.maxDist;
	}
	else
	{
		// use all the time
		SetMoveInterval( 0  );
	}

	SetMoveVel( move.dir * flNewSpeed );

	// --------------------------------------------
	// walk the distance
	// --------------------------------------------
	AIMotorMoveResult_t result = AIM_SUCCESS;
	if ( flTotalDist > 0.0 )
	{
		Vector vecFrom = GetLocalOrigin();
		Vector vecTo = vecFrom + move.dir * flTotalDist;
		
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


