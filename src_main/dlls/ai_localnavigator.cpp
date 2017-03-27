//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "ai_localnavigator.h"

#include "ai_basenpc.h"
#include "ai_planesolver.h"
#include "ai_moveprobe.h"
#include "ai_motor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC(CAI_LocalNavigator)
	//						m_pPlaneSolver	(not saved)
	//						m_pMoveProbe	(not saved)
END_DATADESC();

//-------------------------------------

CAI_LocalNavigator::CAI_LocalNavigator(CAI_BaseNPC *pOuter) : CAI_Component( pOuter ) 
{
	m_pMoveProbe = NULL;
	m_pPlaneSolver = new CAI_PlaneSolver( pOuter );
}

//-------------------------------------

CAI_LocalNavigator::~CAI_LocalNavigator() 
{
	delete m_pPlaneSolver;
}

//-------------------------------------

void CAI_LocalNavigator::Init( IAI_MovementSink *pMovementServices )	
{ 
	CAI_ProxyMovementSink::Init( pMovementServices );
	m_pMoveProbe = GetOuter()->GetMoveProbe(); // @TODO (toml 03-30-03): this is a "bad" way to grab this pointer. Components should have an explcit "init" phase.
}

//-------------------------------------

void CAI_LocalNavigator::ResetMoveCalculations()
{
	m_pPlaneSolver->Reset();
}

//-------------------------------------

bool CAI_LocalNavigator::MoveCalcDirect( AILocalMoveGoal_t *pMoveGoal, float *pDistClear, AIMoveResult_t *pResult )
{
	AI_PROFILE_SCOPE(CAI_Motor_MoveCalcDirect);
	
	float  minCheckDist = GetOuter()->GetMotor()->MinCheckDist();
	float  probeDist	= m_pPlaneSolver->CalcProbeDist( pMoveGoal->speed ); // having this match steering allows one fewer traces
	float  checkDist	= max( minCheckDist, probeDist );
	Vector testPos		= GetLocalOrigin() + pMoveGoal->dir * checkDist;
	
	if ( pMoveGoal->speed )
	{
		bool fTraceClear = GetMoveProbe()->MoveLimit( pMoveGoal->navType, GetLocalOrigin(), testPos, MASK_NPCSOLID, pMoveGoal->pMoveTarget, &pMoveGoal->directTrace );
		pMoveGoal->bHasTraced = true;
	
		float distClear = checkDist - pMoveGoal->directTrace.flDistObstructed;
		if (distClear < 0.001)
			distClear = 0;
		
		if ( fTraceClear )
		{
			*pResult = AIMR_OK;
			return true;
		}
		
		if ( ( pMoveGoal->flags & ( AILMG_TARGET_IS_TRANSITION | AILMG_TARGET_IS_GOAL ) ) && 
			 pMoveGoal->maxDist < distClear )
		{
			*pResult = AIMR_OK;
			return true;
		}

		*pDistClear = distClear;
	}
	else
	{
		// Should never end up in this function with speed of zero. Probably an activity problem.
		*pResult = AIMR_ILLEGAL;
		return true;
	}

	return false;
}

//-------------------------------------

bool CAI_LocalNavigator::MoveCalcSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( (pMoveGoal->flags & AILMG_NO_STEER) )
		return false;

	if ( GetOuter()->GetEfficiency() > AIE_NORMAL )
		return false;

	AI_PROFILE_SCOPE(CAI_Motor_MoveCalcSteer);
	Vector moveSolution;
	if ( m_pPlaneSolver->Solve( *pMoveGoal, distClear, &moveSolution ) )
	{
		if ( moveSolution != pMoveGoal->dir )
		{
			float dot = moveSolution.AsVector2D().Dot( pMoveGoal->dir.AsVector2D() );

			const float COS_HALF_30 = 0.966;
			if ( dot > COS_HALF_30 )
			{
				float probeDist = m_pPlaneSolver->CalcProbeDist( pMoveGoal->speed );
				if ( pMoveGoal->maxDist < probeDist * 0.33333 && distClear > probeDist * 0.6666)
				{
					// A waypoint is coming up, but there's probably time to steer
					// away after hitting it
					*pResult = AIMR_OK;
					return true;
				}
			}

			pMoveGoal->facing = pMoveGoal->dir = moveSolution;
		}
		*pResult = AIMR_OK;
		return true;
	}
		
	return false;
}

//-------------------------------------

bool CAI_LocalNavigator::MoveCalcStop( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if (distClear < pMoveGoal->maxDist)
	{
		if ( distClear < 0.1 )
		{
			DebugNoteMovementFailure();
			*pResult = AIMR_ILLEGAL;
		}
		else
		{
			pMoveGoal->maxDist = distClear;
			*pResult = AIMR_OK;
		}

		return true;
	}
	*pResult = AIMR_OK;
	return true;
}

//-------------------------------------

AIMoveResult_t CAI_LocalNavigator::MoveCalcRaw( AILocalMoveGoal_t *pMoveGoal )
{
	AI_PROFILE_SCOPE(CAI_Motor_MoveCalc);
	
	AIMoveResult_t result = AIMR_OK; // Assume success
	AIMoveTrace_t  directTrace;
	float	   	   distClear;
	
	// --------------------------------------------------

	if ( MoveCalcDirect( pMoveGoal, &distClear, &result) )
		return DbgResult( result );
	
	if ( OnObstructionPreSteer( pMoveGoal, distClear, &result ) )
		return DbgResult( result );
	
	if ( MoveCalcSteer( pMoveGoal, distClear, &result ) )
		return DbgResult( result );
		
	// --------------------------------------------------
	
	if ( OnFailedSteer( pMoveGoal, distClear, &result ) )
		return DbgResult( result );

	if ( OnFailedLocalNavigation( pMoveGoal, distClear, &result ) )
		return DbgResult( result );

	if ( distClear < GetOuter()->GetMotor()->MinStoppingDist() )
	{
		if ( OnInsufficientStopDist( pMoveGoal, distClear, &result ) )
			return DbgResult( result );

		if ( MoveCalcStop( pMoveGoal, distClear, &result) )
			return DbgResult( result );
	}

	// A hopeful result... may get in trouble at next waypoint and obstruction is still there
	if ( distClear > pMoveGoal->curExpectedDist )
		return DbgResult( AIMR_OK );

	// --------------------------------------------------

	DebugNoteMovementFailure();
	return DbgResult( AIMR_ILLEGAL );
}

//-------------------------------------

AIMoveResult_t CAI_LocalNavigator::MoveCalc( AILocalMoveGoal_t *pMoveGoal )
{
	AIMoveResult_t result = MoveCalcRaw( pMoveGoal );
	
	// If success, try to dampen really fast turning movement
	if ( result == AIMR_OK)
	{
		float interval = GetOuter()->GetMotor()->GetMoveInterval();
		float currentYaw = UTIL_AngleMod( GetLocalAngles().y );
		float goalYaw;
		float deltaYaw;
		float speed;
		float clampedYaw;

		// Clamp yaw
		goalYaw = UTIL_VecToYaw( pMoveGoal->facing );
		deltaYaw = fabs( UTIL_AngleDiff( goalYaw, currentYaw ) );
		if ( deltaYaw > 15 )
		{
			speed = deltaYaw * 4.0; // i.e., any maneuver takes a quarter a second
			clampedYaw = AI_ClampYaw( speed, currentYaw, goalYaw, interval );

			if ( clampedYaw != goalYaw )
			{
				pMoveGoal->facing = UTIL_YawToVector( clampedYaw );
			}
		}
	}
	
	return result;
}
//-----------------------------------------------------------------------------
