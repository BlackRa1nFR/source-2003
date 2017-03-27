//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "iservervehicle.h"
#include "movevars_shared.h"

#include "ai_moveprobe.h"

#include "ai_basenpc.h"
#include "ai_nav_property.h"
#include "ai_routedist.h"

#define MOVE_HEIGHT_EPSILON	(0.0625f)

#undef LOCAL_STEP_SIZE
// FIXME: this should be based in their hull width
#define	LOCAL_STEP_SIZE	16.0 // 8 // 16

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC(CAI_MoveProbe)
END_DATADESC();


//-----------------------------------------------------------------------------
// Categorizes the blocker and sets the appropriate bits
//-----------------------------------------------------------------------------
AIMoveResult_t AIComputeBlockerMoveResult( CBaseEntity *pBlocker )
{
	if (pBlocker->MyNPCPointer())
		return AIMR_BLOCKED_NPC;
	else if (pBlocker->entindex() == 0)
		return AIMR_BLOCKED_WORLD;
	return AIMR_BLOCKED_ENTITY;
}


//-----------------------------------------------------------------------------

void CAI_MoveProbe::TraceLine( const Vector &vecStart, const Vector &vecEnd, unsigned int mask, 
							   bool bUseCollisionGroup, trace_t *pResult ) const
{
	int 		collisionGroup 	= (bUseCollisionGroup) ? 
									GetCollisionGroup() : 
									COLLISION_GROUP_NONE;

	AI_TraceLine( vecStart, vecEnd, mask, GetOuter(), collisionGroup, pResult );

#ifdef _DEBUG
	// Just to make sure; I'm not sure that this is always the case but it should be
	if (pResult->allsolid)
	{
		Assert( pResult->startsolid );
	}
#endif
}

//-----------------------------------------------------------------------------

class CTraceFilterNav : public CTraceFilterSimple
{
public:
	CTraceFilterNav( const IServerEntity *passedict, int collisionGroup );
	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );
};

CTraceFilterNav::CTraceFilterNav( const IServerEntity *passedict, int collisionGroup ) : 
	CTraceFilterSimple( passedict, collisionGroup )
{
}

bool CTraceFilterNav::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	IServerEntity *pServerEntity = (IServerEntity*)pHandleEntity;
	CBaseEntity *pEntity = (CBaseEntity *)pServerEntity;

	if ( NavPropertyIsOnEntity( pEntity, NAV_IGNORE ) )
		return false;

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}


//-----------------------------------------------------------------------------

// @Note (toml 11-14-02): VC6 was found to be generating incorrect code in the
// call to engine->TraceHull() when optimized. Disabling optimizations for now
#pragma optimize("", off)
void CAI_MoveProbe::TraceHull( 
	const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, 
	const Vector &hullMax, unsigned int mask, trace_t *pResult ) const
{
	AI_PROFILE_SCOPE( CAI_MoveProbe_TraceHull );

	CTraceFilterNav traceFilter( GetOuter(), GetCollisionGroup() );
	UTIL_TraceHull( vecStart, vecEnd, hullMin, hullMax, mask, &traceFilter, pResult );

	// Just to make sure; I'm not sure that this is always the case but it should be
	Assert( !pResult->allsolid || pResult->startsolid );
}
#pragma optimize("", on)

//-----------------------------------------------------------------------------

void CAI_MoveProbe::TraceHull( const Vector &vecStart, const Vector &vecEnd, 
	unsigned int mask, trace_t *pResult ) const
{
	TraceHull( vecStart, vecEnd, WorldAlignMins(), WorldAlignMaxs(), mask, pResult);
}


//-----------------------------------------------------------------------------
// CheckStep() is a fundamentally 2D operation!	vecEnd.z is ignored.
// We can step up one StepHeight or down one StepHeight from vecStart
//-----------------------------------------------------------------------------
CBaseEntity *CAI_MoveProbe::CheckStep( const Vector &vecStart, const Vector &vecEnd, 
	unsigned int collisionMask, StepGroundTest_t groundTest, Vector *pVecResult, Vector *pHitNormal ) const
{
	AI_PROFILE_SCOPE( CAI_Motor_CheckStep );
	
	CBaseEntity *pBlocker = NULL;

	Assert( pVecResult != &vecEnd );

	*pVecResult = vecStart;
	*pHitNormal = vec3_origin;

	// This is fundamentally a 2D operation; we just want the end
	// position in Z to be no more than a step height from the start position
	Vector stepStart( vecStart.x, vecStart.y, vecStart.z + MOVE_HEIGHT_EPSILON );
	Vector stepEnd( vecEnd.x, vecEnd.y, vecStart.z + MOVE_HEIGHT_EPSILON );

	trace_t trace;
	TraceHull( stepStart, stepEnd, collisionMask, &trace );

	if (trace.startsolid || (trace.fraction < 1))
	{
		// Either the entity is starting embedded in the world, or it hit something.
		// Raise the box by the step height and try again

		if ( !trace.startsolid )
		{
			// Advance to first obstruction point
			stepStart = trace.endpos;
			// Trace up to locate the maximum step up in the space
			Vector stepUp( stepStart );
			stepUp.z += StepHeight();
			TraceHull( stepStart, stepUp, collisionMask, &trace );

			stepStart = trace.endpos;
		}
		else
			stepStart.z += StepHeight();

		// Now move forward
 		stepEnd.z = stepStart.z;

		TraceHull( stepStart, stepEnd, collisionMask, &trace );

		// Ok, raising it didn't work; we're obstructed
		if (trace.startsolid || (trace.fraction < 1))
		{
			// 
			pBlocker = trace.m_pEnt;
			*pHitNormal = trace.plane.normal;
		}

		if (trace.startsolid)
		{
			Assert( *pVecResult == vecStart );
			return pBlocker;
		}

		// If trace.fraction == 0, we fall through and check the position
		// we moved up to for suitability. This allows for sub-step
		// traces if the position ends up being suitable
			
		stepEnd = trace.endpos;
	}

	// seems okay, now find the ground
	// The ground is only valid if it's within a step height of the original position
	Assert( VectorsAreEqual( trace.endpos, stepEnd, 1e-3 ) );
	stepStart = stepEnd; 
	stepEnd.z = vecStart.z - StepHeight() - MOVE_HEIGHT_EPSILON;

	TraceHull( stepStart, stepEnd, collisionMask, &trace );

	// in empty space, lie and say we hit the world
	if (trace.fraction == 1.0f)
	{
		Assert( *pVecResult == vecStart );
		return GetContainingEntity( INDEXENT(0) );
	}

	// Checks to see if the thing we're on is a *type* of thing we
	// are capable of standing on
	CBaseEntity *pFloor = trace.m_pEnt;
	if (!CanStandOn( pFloor ))
	{
		Assert( *pVecResult == vecStart );
		return pFloor;
	}

	if (groundTest != STEP_DONT_CHECK_GROUND)
	{
		// Next, check to see if we can *geometrically* stand on the floor
		bool bIsFloorFlat = CheckStandPosition( trace.endpos, collisionMask );
		if (groundTest != STEP_ON_INVALID_GROUND && !bIsFloorFlat)
		{
			return pFloor;
		}
		// If we started on shaky ground (namely, it's not geometrically ok),
		// then we can continue going even if we remain on shaky ground.
		// This allows NPCs who have been blown into an invalid area to get out
		// of that invalid area and into a valid area. As soon as we're in
		// a valid area, though, we're not allowed to leave it.
	}

	// Return a point that is *on the ground*
	// We'll raise it by an epsilon in check step again
	*pVecResult = trace.endpos;

	// FIXME: NOTE: We may be able to remove this, but due to certain collision
	// problems on displacements, and due to the fact that CheckStep is currently
	// being called from code outside motor code, we need to give it a little 
	// room to avoid boundary condition problems. Also note that this will
	// cause us to start 2*EPSILON above the ground the next time that this
	// function is called, but for now, that appears to not be a problem.
	pVecResult->z += MOVE_HEIGHT_EPSILON;

	return pBlocker; // totally clear if pBlocker is NULL, partial blockage otherwise
}

//-----------------------------------------------------------------------------
// Checks a ground-based movement
// NOTE: The movement will be based on an *actual* start position and
// a *desired* end position; it works this way because the ground-based movement
// is 2 1/2D, and we may end up on a ledge above or below the actual desired endpoint.
//-----------------------------------------------------------------------------
bool CAI_MoveProbe::TestGroundMove( const Vector &vecActualStart, const Vector &vecDesiredEnd, 
	unsigned int collisionMask, unsigned flags, AIMoveTrace_t *pMoveTrace ) const
{
	AIMoveTrace_t ignored;
	if ( !pMoveTrace )
		pMoveTrace = &ignored;

	// Set a reasonable default set of values
	pMoveTrace->flDistObstructed = 0.0f;
	pMoveTrace->pObstruction 	 = NULL;
	pMoveTrace->vHitNormal		 = vec3_origin;
	pMoveTrace->fStatus 		 = AIMR_OK;
	pMoveTrace->vEndPosition 	 = vecActualStart;
	pMoveTrace->flStepUpDistance = 0;

	Vector vecMoveDir;
	pMoveTrace->flTotalDist = ComputePathDirection( NAV_GROUND, vecActualStart, vecDesiredEnd, &vecMoveDir );
	if (pMoveTrace->flTotalDist == 0.0f)
	{
		return true;
	}

	// If it starts hanging over an edge, tough it out until it's not
	// This allows us to blow an NPC in an invalid region + allow him to walk out
	StepGroundTest_t groundTest;
	if (flags & AITGM_IGNORE_FLOOR)
	{
		groundTest = STEP_DONT_CHECK_GROUND;
	}
	else
	{
		if ((flags & AITGM_IGNORE_INITIAL_STAND_POS) || CheckStandPosition(vecActualStart, collisionMask))
			groundTest = STEP_ON_VALID_GROUND;
		else
			groundTest = STEP_ON_INVALID_GROUND;
	}

	//  Take single steps towards the goal
	float distClear = 0;
	Vector vecStepCurrent = vecActualStart;
	Vector vecStepTarget;
	Vector vecStepResult = vecActualStart;
	Vector vecHitNormal = vec3_origin;
	CBaseEntity *pBlocker = NULL;
	
	for (;;)
	{
		float flStepSize = min( LOCAL_STEP_SIZE, pMoveTrace->flTotalDist - distClear );
		if ( flStepSize < 0.001 )
			break;

		VectorMA( vecStepCurrent, flStepSize, vecMoveDir, vecStepTarget );
		
		pBlocker = CheckStep( vecStepCurrent, vecStepTarget, collisionMask, groundTest, &vecStepResult, &vecHitNormal );
		
		if ( pBlocker )
		{
			distClear += ( vecStepResult - vecStepCurrent ).Length2D();
			break;
		}
		
		float dz = vecStepResult.z - vecStepCurrent.z;
		if ( dz < 0 )
		{
			dz = 0;
		}
		pMoveTrace->flStepUpDistance += dz;
		distClear += flStepSize;
		vecStepCurrent = vecStepResult;
	}
	
	pMoveTrace->vEndPosition = vecStepResult;
	
	if (pBlocker)
	{
		pMoveTrace->pObstruction	 = pBlocker;
		pMoveTrace->vHitNormal		 = vecHitNormal;
		pMoveTrace->fStatus			 = AIComputeBlockerMoveResult( pBlocker );
		pMoveTrace->flDistObstructed = pMoveTrace->flTotalDist - distClear;
		return false;
	}

	// FIXME: If you started on a ledge and ended on a ledge, 
	// should it return an error condition (that you hit the world)?
	// Certainly not for Step(), but maybe for GroundMoveLimit()?
	
	// Make sure we actually made it to the target position 
	// and not a ledge above or below the target.
	if (fabs(pMoveTrace->vEndPosition.z - vecDesiredEnd.z) > 0.5f * GetHullHeight())
	{
		// Ok, we ended up on a ledge above or below the desired destination
		pMoveTrace->pObstruction = GetContainingEntity( INDEXENT(0) );
		pMoveTrace->vHitNormal	 = vec3_origin;
		pMoveTrace->fStatus = AIMR_BLOCKED_WORLD;
		pMoveTrace->flDistObstructed = ComputePathDistance( NAV_GROUND, pMoveTrace->vEndPosition, vecDesiredEnd );
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Tries to generate a route from the specified start to end positions
// Will return the results of the attempt in the AIMoveTrace_t structure
//-----------------------------------------------------------------------------
void CAI_MoveProbe::GroundMoveLimit( const Vector &vecStart, const Vector &vecEnd, 
	unsigned int collisionMask, const CBaseEntity *pTarget, bool fCheckStep, AIMoveTrace_t* pTrace ) const
{
	// NOTE: Never call this directly!!! Always use MoveLimit!!
	// This assertion should ensure this happens
	Assert( !IsMoveBlocked( *pTrace ) );

	AI_PROFILE_SCOPE( CAI_Motor_GroundMoveLimit );

	Vector vecActualStart, vecDesiredEnd;

	pTrace->flTotalDist = ComputePathDistance( NAV_GROUND, vecStart, vecEnd );

	if ( !IterativeFloorPoint( vecStart, collisionMask, &vecActualStart ) )
	{
		pTrace->flDistObstructed = pTrace->flTotalDist;
		pTrace->pObstruction	= GetContainingEntity( INDEXENT(0) );
		pTrace->vHitNormal		= vec3_origin;
		pTrace->fStatus			= AIMR_BLOCKED_WORLD;
		pTrace->vEndPosition	= vecStart;

		//Msg( "Warning: attempting to path from/to a point that is in solid space or is too high\n" );
		return;
	}
	
	// find out where they (in theory) should have ended up  
	IterativeFloorPoint( vecEnd, collisionMask, &vecDesiredEnd ); 

	// When checking the route, look for ground geometric validity
	// Let's try to avoid invalid routes
	TestGroundMove( vecActualStart, vecDesiredEnd, collisionMask, (fCheckStep) ? AITGM_DEFAULT : AITGM_IGNORE_FLOOR, pTrace );

	// check to see if the player is in a vehicle and the vehicle is blocking our way
	bool bVehicleMatchesObstruction = false;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( 1 ) );
	if ( pPlayer && pPlayer->IsInAVehicle() )
	{
		CBaseEntity *pVehicleEnt = pPlayer->GetVehicle()->GetVehicleEnt();
		if ( pVehicleEnt == pTrace->pObstruction )
			bVehicleMatchesObstruction = true;
	}

	if ( (pTarget && (pTarget == pTrace->pObstruction)) || bVehicleMatchesObstruction )
	{
		// Collided with target entity, return there was no collision!!
		// but leave the end trace position
		pTrace->flDistObstructed = 0.0f;
		pTrace->pObstruction = NULL;
		pTrace->vHitNormal = vec3_origin;
		pTrace->fStatus = AIMR_OK;
	}
}


//-----------------------------------------------------------------------------
// Purpose: returns zero if the caller can walk a straight line from
//			vecStart to vecEnd ignoring collisions with pTarget
//
//			if the move fails, returns the distance remaining to vecEnd
//-----------------------------------------------------------------------------
void CAI_MoveProbe::FlyMoveLimit( const Vector &vecStart, const Vector &vecEnd, 
	unsigned int collisionMask, const CBaseEntity *pTarget, AIMoveTrace_t *pMoveTrace ) const
{
	// NOTE: Never call this directly!!! Always use MoveLimit!!
	// This assertion should ensure this happens
	Assert( !IsMoveBlocked( *pMoveTrace) );

	trace_t tr;
	TraceHull( vecStart, vecEnd, collisionMask, &tr );

	if ( tr.fraction < 1 )
	{
		CBaseEntity *pBlocker = tr.m_pEnt;
		if ( pBlocker )
		{
			if ( pTarget == pBlocker )
			{
				// Colided with target entity, movement is ok
				pMoveTrace->vEndPosition = tr.endpos;
				return;
			}

			// If blocked by an npc remember
			pMoveTrace->pObstruction = pBlocker;
			pMoveTrace->vHitNormal	 = vec3_origin;
			pMoveTrace->fStatus = AIComputeBlockerMoveResult( pBlocker );
		}
		pMoveTrace->flDistObstructed = ComputePathDistance( NAV_FLY, tr.endpos, vecEnd );
		pMoveTrace->vEndPosition = tr.endpos;
		return;
	}

	// no collisions, movement is ok
	pMoveTrace->vEndPosition = vecEnd;
}


//-----------------------------------------------------------------------------
// Purpose: returns zero if the caller can jump from
//			vecStart to vecEnd ignoring collisions with pTarget
//
//			if the jump fails, returns the distance
//			that can be travelled before an obstacle is hit
//-----------------------------------------------------------------------------
void CAI_MoveProbe::JumpMoveLimit( const Vector &vecStart, const Vector &vecEnd, 
	unsigned int collisionMask, const CBaseEntity *pTarget, AIMoveTrace_t *pMoveTrace ) const
{
	pMoveTrace->vJumpVelocity.Init( 0, 0, 0 );

	float flDist = ComputePathDistance( NAV_JUMP, vecStart, vecEnd );

	if (!IsJumpLegal(vecStart, vecEnd, vecEnd))
	{
		pMoveTrace->fStatus = AIMR_ILLEGAL;
		pMoveTrace->flDistObstructed = flDist;
		return;
	}

	// --------------------------------------------------------------------------
	// Drop start and end vectors to the floor and check to see if they're legal
	// --------------------------------------------------------------------------
	Vector vecFrom;
	IterativeFloorPoint( vecStart, collisionMask, &vecFrom );

	Vector vecTo;
	IterativeFloorPoint( vecEnd, collisionMask, &vecTo );
	if (!CheckStandPosition( vecTo, collisionMask))
	{
		pMoveTrace->fStatus = AIMR_ILLEGAL;
		pMoveTrace->flDistObstructed = flDist;
		return;
	}

	if (vecFrom == vecTo)
	{
		pMoveTrace->fStatus = AIMR_ILLEGAL;
		pMoveTrace->flDistObstructed = flDist;
		return;
	}

	if ((vecFrom - vecTo).Length2D() == 0.0)
	{
		pMoveTrace->fStatus = AIMR_ILLEGAL;
		pMoveTrace->flDistObstructed = flDist;
		return;
	}

	// FIXME: add max jump velocity callback?  Look at the velocity in the jump animation?  use ideal running speed?
	float maxHorzVel = GetOuter()->GetMaxJumpSpeed();

	Vector gravity = Vector(0, 0, sv_gravity.GetFloat() * GetGravity());

	// intialize error state to it being an illegal jump
	CBaseEntity *pObstruction = NULL;
	AIMoveResult_t fStatus = AIMR_ILLEGAL;
	float flDistObstructed = flDist;

	// initialize jump state
	float minSuccessfulJumpHeight = 1024.0;
	float minJumpHeight = 0.0;
	float minJumpStep = 1024.0;

	// initial jump, sets baseline for minJumpHeight
	Vector vecApex;
	Vector rawJumpVel = CalcJumpLaunchVelocity(vecFrom, vecTo, gravity.z, &minJumpHeight, maxHorzVel, &vecApex );
	float baselineJumpHeight = minJumpHeight;

	// FIXME: this is a binary search, which really isn't the right thing to do.  If there's a gap 
	// the npc can jump through, this won't reliably find it.  The only way I can think to do this is a 
	// linear search trying ever higher jumps until the gap is either found or the jump is illegal.
	do
	{
		rawJumpVel = CalcJumpLaunchVelocity(vecFrom, vecTo, gravity.z, &minJumpHeight, maxHorzVel, &vecApex );
		// Msg( "%.0f ", minJumpHeight );

		if (!IsJumpLegal(vecFrom, vecApex, vecTo))
		{
			// too high, try lower
			minJumpStep = minJumpStep / 2.0;
			minJumpHeight = minJumpHeight - minJumpStep;
		}
		else
		{
			// Calculate the total time of the jump minus a tiny fraction
			float jumpTime		= (vecFrom - vecTo).Length2D()/rawJumpVel.Length2D();
			float timeStep		= jumpTime / 10.0;	

			Vector vecTest = vecFrom;
			bool bMadeIt = true;

			// this sweeps out a rough approximation of the jump
			// FIXME: this won't reliably hit the apex
			for (float flTime = 0 ; flTime < jumpTime - 0.01; flTime += timeStep )
			{
				trace_t trace;

				// Calculate my position after the time step (average velocity over this time step)
				Vector nextPos = vecTest + (rawJumpVel - 0.5 * gravity * timeStep) * timeStep;

				TraceHull( vecTest, nextPos, collisionMask, &trace );

				if (trace.startsolid || trace.fraction < 0.99) // FIXME: getting inconsistant trace fractions, revisit after Jay resolves collision eplisons
				{
					// NDebugOverlay::Box( trace.endpos, WorldAlignMins(), WorldAlignMaxs(), 255, 255, 0, 0, 10.0 );

					/*
					// if we think we hit an obstruction, see if it's the vehicle the player is riding in
					CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( 1 ) );
					if ( pPlayer && pPlayer->IsInAVehicle() )
					{
						CBaseEntity *pVehicleEnt = pPlayer->GetVehicle()->GetVehicleEnt();
						if ( pVehicleEnt == trace.m_pEnt )
						{
							bMadeIt = true;
							break;
						}
					}
					*/
						
					// save error state
					pObstruction = trace.m_pEnt;
					fStatus = AIComputeBlockerMoveResult( pObstruction );
					flDistObstructed = ComputePathDistance( NAV_JUMP, vecTest, vecTo );

					if (trace.plane.normal.z < 0.0)
					{
						// hit a ceiling looking thing, too high, try lower
						minJumpStep = minJumpStep / 2.0;
						minJumpHeight = minJumpHeight - minJumpStep;
					}
					else
					{
						// hit wall looking thing, try higher
						minJumpStep = minJumpStep / 2.0;
						minJumpHeight += minJumpStep;
					}
					bMadeIt = false;
					break;
				}

				rawJumpVel	= rawJumpVel - gravity * timeStep;
				vecTest		= nextPos;
			}

			if (bMadeIt)
			{
				// made it, try lower
				minSuccessfulJumpHeight = minJumpHeight;
				minJumpStep = minJumpStep / 2.0;
				minJumpHeight -= minJumpStep;
			}
		}
	}
	while (minJumpHeight > baselineJumpHeight && minJumpHeight <= 1024.0 && minJumpStep >= 16.0);

	// Msg( "(%.0f)\n", minSuccessfulJumpHeight );

	if (minSuccessfulJumpHeight != 1024.0)
	{
		// Get my jump velocity
		pMoveTrace->vJumpVelocity = CalcJumpLaunchVelocity(vecFrom, vecTo, gravity.z, &minSuccessfulJumpHeight, maxHorzVel, &vecApex );
	}
	else
	{
		// ----------------------------------------------------------
		// If blocked by an npc remember
		// ----------------------------------------------------------
		pMoveTrace->pObstruction = pObstruction;
		pMoveTrace->vHitNormal	= vec3_origin;
		pMoveTrace->fStatus = fStatus;
		pMoveTrace->flDistObstructed = flDistObstructed;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns zero if the caller can climb from
//			vecStart to vecEnd ignoring collisions with pTarget
//
//			if the climb fails, returns the distance remaining 
//			before the obstacle is hit
//-----------------------------------------------------------------------------
void CAI_MoveProbe::ClimbMoveLimit( const Vector &vecStart, const Vector &vecEnd, 
	const CBaseEntity *pTarget, AIMoveTrace_t *pMoveTrace ) const
{
	trace_t tr;
	TraceHull( vecStart, vecEnd, MASK_NPCSOLID, &tr );

	if (tr.fraction < 1.0)
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if (pEntity == pTarget)
		{
			return;
		}
		else
		{
			// ----------------------------------------------------------
			// If blocked by an npc remember
			// ----------------------------------------------------------
			pMoveTrace->pObstruction = pEntity;
			pMoveTrace->vHitNormal = vec3_origin;
			pMoveTrace->fStatus = AIComputeBlockerMoveResult( pEntity );

			float flDist = (1.0 - tr.fraction) * ComputePathDistance( NAV_CLIMB, vecStart, vecEnd );
			if (flDist <= 0.001) 
			{
				flDist = 0.001;
			}
			pMoveTrace->flDistObstructed = flDist;
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_MoveProbe::MoveLimit( Navigation_t navType, const Vector &vecStart, 
	const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, 
	bool fCheckStep, AIMoveTrace_t* pTrace) const
{
	// Set a reasonable default set of values
	pTrace->flTotalDist = ComputePathDistance( navType, vecStart, vecEnd );
	pTrace->flDistObstructed = 0.0f;
	pTrace->pObstruction = NULL;
	pTrace->vHitNormal = vec3_origin;
	pTrace->fStatus = AIMR_OK;
	pTrace->vEndPosition = vecStart;

	switch (navType)
	{
	case NAV_GROUND:	
		GroundMoveLimit(vecStart, vecEnd, collisionMask, pTarget, fCheckStep, pTrace);
		break;

	case NAV_FLY:
		FlyMoveLimit(vecStart, vecEnd, collisionMask, pTarget, pTrace);
		break;

	case NAV_JUMP:
		JumpMoveLimit(vecStart, vecEnd, collisionMask, pTarget, pTrace);
		break;

	case NAV_CLIMB:		
		ClimbMoveLimit(vecStart, vecEnd, pTarget, pTrace);
		break;

	default:
		pTrace->fStatus = AIMR_ILLEGAL;
		pTrace->flDistObstructed = ComputePathDistance( navType, vecStart, vecEnd );
		break;
	}

	return !IsMoveBlocked(pTrace->fStatus);
}

//-----------------------------------------------------------------------------
// Purpose: Returns a jump lauch velocity for the current target entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CAI_MoveProbe::CalcJumpLaunchVelocity(const Vector &startPos, const Vector &endPos, float flGravity, float *pminHeight, float maxHorzVelocity, Vector *pvecApex ) const
{
	// Get the height I have to jump to get to the target
	float	stepHeight	  = endPos.z - startPos.z;


	// get horizontal distance to target
	Vector targetDir2D	= endPos - startPos;
	targetDir2D.z = 0;
	float distance = VectorNormalize(targetDir2D);

	// get minimum times and heights to meet ideal horz velocity
	float minHorzTime = distance / maxHorzVelocity;
	float minHorzHeight = 0.5 * flGravity * (minHorzTime * 0.5) * (minHorzTime * 0.5);

	// jump height must be enough to hang in the air
	*pminHeight = max( *pminHeight, minHorzHeight );
	// jump height must be enough to cover the step up
	*pminHeight = max( *pminHeight, stepHeight );

	// time from start to apex
	float t0 = sqrt( ( 2.0 * *pminHeight) / flGravity );
	// time from apex to end
	float t1 = sqrt( ( 2.0 * fabs( *pminHeight - stepHeight) ) / flGravity );

	float velHorz = distance / (t0 + t1);

	Vector jumpVel = targetDir2D * velHorz;

	jumpVel.z = (float)sqrt(2.0f * flGravity * (*pminHeight));

	if (pvecApex)
	{
		*pvecApex = startPos + targetDir2D * velHorz * t0 + Vector( 0, 0, *pminHeight );
	}

	// -----------------------------------------------------------
	// Make the horizontal jump vector and add vertical component
	// -----------------------------------------------------------

	return jumpVel;
}

//-----------------------------------------------------------------------------

bool CAI_MoveProbe::CheckStandPosition( const Vector &vecStart, unsigned int collisionMask ) const
{
	AI_PROFILE_SCOPE( CAI_Motor_CheckStandPosition );

	Vector contactMin, contactMax;

	// this should assume the model is already standing
	Vector vecUp	= Vector( vecStart.x, vecStart.y, vecStart.z + 0.1 );
	Vector vecDown	= Vector( vecStart.x, vecStart.y, vecStart.z - StepHeight() );

	// check a half sized box centered around the foot
	const Vector &vHullMins = WorldAlignMins();
	const Vector &vHullMaxs = WorldAlignMaxs();

	contactMin.x = vHullMins.x * 0.75 + vHullMaxs.x * 0.25;
	contactMax.x = vHullMins.x * 0.25 + vHullMaxs.x * 0.75;
	contactMin.y = vHullMins.y * 0.75 + vHullMaxs.y * 0.25;
	contactMax.y = vHullMins.y * 0.25 + vHullMaxs.y * 0.75;
	contactMin.z = vHullMins.z;
	contactMax.z = vHullMins.z;

	trace_t trace;
	TraceHull( vecUp, vecDown, contactMin, contactMax, collisionMask, &trace );

	if (trace.fraction == 1.0 || !CanStandOn( trace.m_pEnt ))
		return false;

	if ( GetOuter()->GetEfficiency() == AIE_NORMAL )
	{
		// check a box for each quadrant, allow one failure
		int already_failed = false;
		for	(int x = 0; x <= 1 ;x++)
		{
			for	(int y = 0; y <= 1; y++)
			{
				// create bounding boxes for each quadrant
				contactMin[0] = x ? 0 :vHullMins.x;
				contactMax[0] = x ? vHullMaxs.x : 0;
				contactMin[1] = y ? 0 : vHullMins.y;
				contactMax[1] = y ? vHullMaxs.y : 0;
				
				TraceHull( vecUp, vecDown, contactMin, contactMax, collisionMask, &trace );

				// this should hit something, if it doesn't allow one failure
				if (trace.fraction == 1.0 || !CanStandOn( trace.m_pEnt ))
				{
					if (already_failed)
						return false;
					else
					{
						already_failed = true;
					}
				}
				else
				{
				}
			}
		}
	}

	return true;
}



//-----------------------------------------------------------------------------
// Computes a point on the floor below the start point, somewhere
// between vecStart.z + flStartZ and vecStart.z + flEndZ
//-----------------------------------------------------------------------------
bool CAI_MoveProbe::FloorPoint( const Vector &vecStart, unsigned int collisionMask, 
						   float flStartZ, float flEndZ, Vector *pVecResult ) const
{
	// make a pizzabox shaped bounding hull
	Vector mins = WorldAlignMins();
	Vector maxs( WorldAlignMaxs().x, WorldAlignMaxs().y, WorldAlignMins().z );

	// trace down step height and a bit more
	Vector vecUp( vecStart.x, vecStart.y, vecStart.z + flStartZ );
	Vector vecDown( vecStart.x, vecStart.y, vecStart.z + flEndZ );
	
	trace_t trace;
	TraceHull( vecUp, vecDown, mins, maxs, collisionMask, &trace );

	if (trace.startsolid)
	{
		vecUp.z = vecStart.z;
		TraceHull( vecUp, vecDown, mins, maxs, collisionMask, &trace );
	}

	// this should have hit a solid surface by now
	if (trace.fraction == 1 || trace.allsolid)
	{
		// set result to start position if it doesn't work
		*pVecResult = vecStart;
		return false;
	}

	*pVecResult = trace.endpos;
	return true;
}


//-----------------------------------------------------------------------------
// A floorPoint that is useful only in the contect of iterative movement
//-----------------------------------------------------------------------------
bool CAI_MoveProbe::IterativeFloorPoint( const Vector &vecStart, unsigned int collisionMask, Vector *pVecResult ) const
{
	// Used by the movement code, it guarantees we don't move outside a step
	// height from our current position
	return FloorPoint( vecStart, collisionMask, StepHeight(), -(12*60), pVecResult );
}

//-----------------------------------------------------------------------------

float CAI_MoveProbe::StepHeight() const
{
	return GetOuter()->StepHeight();
}

//-----------------------------------------------------------------------------

bool CAI_MoveProbe::CanStandOn( CBaseEntity *pSurface ) const
{
	return GetOuter()->CanStandOn( pSurface );
}

//-----------------------------------------------------------------------------

bool CAI_MoveProbe::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const
{
	return GetOuter()->IsJumpLegal( startPos, apex, endPos );
}

//-----------------------------------------------------------------------------

