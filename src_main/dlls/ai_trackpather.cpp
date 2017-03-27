//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "trains.h"
#include "ai_trackpather.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	TRACKPATHER_DEBUG_LEADING	1
#define	TRACKPATHER_DEBUG_PATH		2
ConVar g_debug_trackpather( "g_debug_trackpather", "0", FCVAR_CHEAT );

//------------------------------------------------------------------------------

BEGIN_DATADESC( CAI_TrackPather )
	DEFINE_FIELD( CAI_TrackPather, m_vecDesiredPosition,FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CAI_TrackPather, m_vecGoalOrientation,FIELD_VECTOR ),

	DEFINE_FIELD( CAI_TrackPather, m_pCurrentPathTarget, FIELD_CLASSPTR ),
	DEFINE_FIELD( CAI_TrackPather, m_pDestPathTarget, FIELD_CLASSPTR ),
	DEFINE_FIELD( CAI_TrackPather, m_pLastPathTarget, FIELD_CLASSPTR ),
	
	DEFINE_FIELD( CAI_TrackPather, m_strCurrentPathName,	FIELD_STRING ),
	DEFINE_FIELD( CAI_TrackPather, m_strDestPathName,		FIELD_STRING ),
	DEFINE_FIELD( CAI_TrackPather, m_strLastPathName,		FIELD_STRING ),
	
	DEFINE_FIELD( CAI_TrackPather, m_vecLastGoalCheckPosition,	FIELD_VECTOR ),
	DEFINE_FIELD( CAI_TrackPather, m_flPathUpdateTime,		FIELD_FLOAT ),
	DEFINE_FIELD( CAI_TrackPather, m_flEnemyPathUpdateTime,	FIELD_FLOAT ),
	DEFINE_FIELD( CAI_TrackPather, m_flPathLeadBias,		FIELD_FLOAT ),
	DEFINE_FIELD( CAI_TrackPather, m_bForcedMove,			FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_TrackPather, m_bPatrolling,			FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_TrackPather, m_bPatrolBreakable,		FIELD_BOOLEAN ),

	// Derived class pathing data
	DEFINE_FIELD( CAI_TrackPather, m_flPathBaseLead,		FIELD_FLOAT ),
	DEFINE_FIELD( CAI_TrackPather, m_flTargetDistanceThreshold,	FIELD_FLOAT ),
	DEFINE_FIELD( CAI_TrackPather, m_flAvoidDistance,		FIELD_FLOAT ),

	// Inputs
	DEFINE_INPUTFUNC( CAI_TrackPather, FIELD_STRING, "SetTrack", InputSetTrack ),
	DEFINE_INPUTFUNC( CAI_TrackPather, FIELD_STRING, "FlyToPathTrack", InputFlyToPathTrack ),
	DEFINE_INPUTFUNC( CAI_TrackPather, FIELD_VOID,	 "StartPatrol", InputStartPatrol ),
	DEFINE_INPUTFUNC( CAI_TrackPather, FIELD_VOID,	 "StartPatrolBreakable", InputStartPatrolBreakable ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Initialize pathing data
//-----------------------------------------------------------------------------
void CAI_TrackPather::InitPathingData( float flBaseLead, float flTargetDistance, float flAvoidDistance )
{
	m_flPathBaseLead = flBaseLead;
	m_flTargetDistanceThreshold = flTargetDistance;
	m_flAvoidDistance = flAvoidDistance;

	m_pCurrentPathTarget = NULL;
	m_pDestPathTarget = NULL;
	m_pLastPathTarget = NULL;
	m_flPathUpdateTime = gpGlobals->curtime;
	m_flEnemyPathUpdateTime	= gpGlobals->curtime;
	m_flPathLeadBias = 0.0f;
	m_bForcedMove = false;
	m_bPatrolling = false;
	m_bPatrolBreakable = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_TrackPather::OnRestore( void )
{
	BaseClass::OnRestore();

	// Restore current path
	if ( m_strCurrentPathName != NULL_STRING )
	{
		m_pCurrentPathTarget = (CPathTrack *) gEntList.FindEntityByName( NULL, m_strCurrentPathName, NULL );
	}
	else
	{
		m_pCurrentPathTarget = NULL;
	}

	// Restore destination path
	if ( m_strDestPathName != NULL_STRING )
	{
		m_pDestPathTarget = (CPathTrack *) gEntList.FindEntityByName( NULL, m_strDestPathName, NULL );
	}
	else
	{
		m_pDestPathTarget = NULL;
	}

	// Restore last path
	if ( m_strLastPathName != NULL_STRING )
	{
		m_pLastPathTarget = (CPathTrack *) gEntList.FindEntityByName( NULL, m_strLastPathName, NULL );
	}
	else
	{
		m_pLastPathTarget = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_TrackPather::OnSave( void )
{
	BaseClass::OnSave();

	// Stash all the paths into strings for restoration later
	m_strCurrentPathName = ( m_pCurrentPathTarget != NULL ) ? m_pCurrentPathTarget->GetEntityName() : NULL_STRING;
	m_strDestPathName = ( m_pDestPathTarget != NULL ) ? m_pDestPathTarget->GetEntityName() : NULL_STRING;
	m_strLastPathName = ( m_pLastPathTarget != NULL ) ? m_pLastPathTarget->GetEntityName() : NULL_STRING;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_TrackPather::UpdateTrackNavigation( void )
{
	//FIXME: Need old solution for simple navigation along scripted paths

	// Set us to a target if we don't have a goal entity
	if ( ( CPathTrack::ValidPath( m_pDestPathTarget ) == NULL ) && ( m_target != NULL_STRING ) )
	{
		SetTrack( m_target );
	}

	if ( m_pCurrentPathTarget )
	{
		// Find out where we want to head (chasing enemy)
		UpdateTargetPosition();

		// Move along our path towards our current destination
		UpdatePathCorner();

		// Figure out our desired position along the path
		UpdateDesiredPosition();

		// Debug
		if ( g_debug_trackpather.GetInt() == TRACKPATHER_DEBUG_LEADING )
		{
			NDebugOverlay::Cross3D( GetDesiredPosition(), -Vector(32,32,32), Vector(32,32,32), 0, 255, 255, true, 0.1f );
			NDebugOverlay::Cross3D( m_pCurrentPathTarget->GetAbsOrigin(), -Vector(16,16,16), Vector(16,16,16), 0, 255, 0, true, 0.5f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Figure out our desired position along our current path
//-----------------------------------------------------------------------------
void CAI_TrackPather::UpdateDesiredPosition( void )
{
	// Get our new point along our path to fly to (leads out ahead of us)
	SetDesiredPosition( GetPathGoalPoint( m_flPathBaseLead + m_flPathLeadBias ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_TrackPather::UpdateTargetPosition( void )
{
	// Don't update our target if we're being told to go somewhere
	if ( m_bForcedMove )
		return;
	// Don't update our target if we're patrolling
	if ( m_bPatrolling )
	{
		// If we have an enemy, and our patrol is breakable, stop patrolling
		if ( !m_bPatrolBreakable || !GetEnemy() )
			return;

		m_bPatrolling = false;
	}

	Vector targetPos;

	if ( !GetTrackPatherTarget( &targetPos ) )
		return;

	// Must have a valid destination
	if ( CPathTrack::ValidPath( m_pDestPathTarget ) == NULL )
		return;

	// Not time to update again
	if ( m_flEnemyPathUpdateTime > gpGlobals->curtime )
		return;

	// See if the target has moved enough to make us recheck
	if ( ( targetPos - m_vecLastGoalCheckPosition ).Length() < m_flTargetDistanceThreshold )
		return;

	// Find the best position to be on our path
	CPathTrack *pDest = BestPointOnPath( targetPos, m_flAvoidDistance, true );

	if ( CPathTrack::ValidPath( pDest ) == NULL )
	{
		// This means that a valid path could not be found to our target!
//		Assert(0);
		return;
	}

	// This is our new destination
	m_pDestPathTarget = pDest;

	// Keep this goal point for comparisons later
	m_vecLastGoalCheckPosition = targetPos;
	
	// Only do this on set intervals
	m_flEnemyPathUpdateTime	= gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_TrackPather::UpdatePathCorner( void )
{
	// Find the next path along a route to get us to our goal
	if ( m_pCurrentPathTarget != m_pDestPathTarget )
	{
		// Only update on short intervals
		if ( m_flPathUpdateTime < gpGlobals->curtime )
		{
			CPathTrack	*pNearestTrack = BestPointOnPath( WorldSpaceCenter(), 0.0f, false );
			
			// If our nearest track was not our last known track, trip the pass trigger
			if ( pNearestTrack != m_pLastPathTarget )
			{
				// Trip our "path_track reached" output
				variant_t emptyVariant;

				pNearestTrack->AcceptInput( "InPass", this, this, emptyVariant, 0 );
				
				// Hold this for changes
				m_pLastPathTarget = m_pCurrentPathTarget;
			}

			// Get our current closest path point
			m_pCurrentPathTarget= pNearestTrack;
			m_flPathUpdateTime	= gpGlobals->curtime + 0.5f;

			// Find the next point on our path that we need
			CPathTrack *pNextPath = FindNextPathPoint( m_pCurrentPathTarget, m_pDestPathTarget );
			
			// See if we have a path to follow
			if ( CPathTrack::ValidPath( pNextPath ) != NULL )
			{
				// Debug
				if ( g_debug_trackpather.GetInt() == TRACKPATHER_DEBUG_PATH )
				{
					NDebugOverlay::Cross3D( m_vecDesiredPosition, -Vector(16,16,16), Vector(16,16,16), 255, 0, 0, true, 0.1f );
					NDebugOverlay::Cross3D( m_pCurrentPathTarget->GetAbsOrigin(), -Vector(16,16,16), Vector(16,16,16), 0, 255, 0, true, 0.1f );
				}

				// This is our current navigation target
				m_pCurrentPathTarget = pNextPath;
			}
		}
		
		// Debug
		if ( g_debug_trackpather.GetInt() == TRACKPATHER_DEBUG_LEADING )
		{
			NDebugOverlay::Cross3D( m_pCurrentPathTarget->GetAbsOrigin(), -Vector(16,16,16), Vector(16,16,16), 255, 0, 255, true, 0.5f );
		}
	}
	else
	{
		//FIXME: This is a bit kludgy...

		// If our destination track was not our last known track, trip the pass trigger
		if ( m_pDestPathTarget != m_pLastPathTarget )
		{
			CPathTrack	*pNearestTrack = BestPointOnPath( WorldSpaceCenter(), 0.0f, false );

			// This is a nasty little bit of work...
			if ( ( pNearestTrack == m_pDestPathTarget ) && ( ArrivedAtPathPoint( pNearestTrack, 256 ) ) )
			{
				// Trip our "path_track reached" output
				variant_t emptyVariant;

				m_pDestPathTarget->AcceptInput( "InPass", this, this, emptyVariant, 0 );
				
				// Reached our goal
				m_bForcedMove = false;

				UpdatePatrol();

				// Hold this for changes
				m_pLastPathTarget = m_pDestPathTarget;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_TrackPather::UpdatePatrol( void )
{
	if ( !m_bPatrolling )
		return;

	// If we're set to patrol, and we can't see the enemy, move to the other end of the track.
	// Figure out which way we've come along the path
	bool bNext = true;
	if ( m_pLastPathTarget && m_pLastPathTarget->GetNext() == m_pDestPathTarget )
	{
		bNext = false;
	}

	// Now find the desired end of the track
	CPathTrack *pPath = m_pDestPathTarget;
	CPathTrack *pLastPath = pPath;
	int	loopRuns = 0;
	while ( CPathTrack::ValidPath( pPath ) )
	{
		if ( loopRuns++ > 128 )
			break;

		pLastPath = pPath;
		if ( bNext )
		{
			pPath = pPath->GetNext();
		}
		else
		{
			pPath = pPath->GetPrevious();
		}
	}
	
	// Now move to the last path point
	SetTrack( pLastPath );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &targetPos - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CPathTrack *CAI_TrackPather::BestPointOnPath( const Vector &targetPos, float avoidRadius, bool visible )
{
	CPathTrack *pPath = CPathTrack::ValidPath( m_pCurrentPathTarget );

	// Find our nearest node if we don't already have one
	if ( pPath == NULL )
	{
		if ( CPathTrack::ValidPath( m_pDestPathTarget ) != NULL )
		{
			pPath = m_pDestPathTarget;
		}
		else
		{
			//FIXME: Implement
			assert(0);
			return NULL;
		}
	}

	CPathTrack *pNearestPath	= NULL;
	float		flNearestDist	= 999999999;
	float		flPathDist;
	int			loopRuns = 0;

	enum 
	{
		FORWARD,
		BACKWARD
	};

	// Our target may be a player in a vehicle
	CBasePlayer *pPlayer = ToBasePlayer( GetTrackPatherTargetEnt() );
	CBaseEntity *pVehicle = NULL;
	if ( pPlayer && pPlayer->IsInAVehicle() )
	{
		pVehicle = pPlayer->GetVehicleEntity();
	}

	for ( int dir = FORWARD; dir <= BACKWARD; ++dir )
	{
		CPathTrack *pInitPath = pPath;

		// Find the nearest node to the target (going forward)
		while ( CPathTrack::ValidPath( pPath ) )
		{
			// Sanity check our path walk
			if ( ( loopRuns++ ) > 1024 )
			{
				// This means that the path is either really large, or we've slipped through a continuous loop somehow
				DevWarning( "%s: Cyclical path found in path_track connections, check links!\n", GetClassname() );
				//assert(0);
				break;
			} 

			// Debug
			if ( g_debug_trackpather.GetInt() == TRACKPATHER_DEBUG_PATH )
			{
				NDebugOverlay::Cross3D( pPath->GetAbsOrigin(), -Vector(4,4,4), Vector(4,4,4), 0, 0, 255, true, 0.1f );
			}

			// Find the distance between this test point and our goal point
			flPathDist = ( pPath->GetAbsOrigin() - targetPos ).LengthSqr();

			// See if it's closer and it's also not within our avoidance radius
			if ( ( flPathDist < flNearestDist ) && ( ( pPath->GetAbsOrigin() - targetPos ).Length2D() > avoidRadius ) )
			{
				// If it has to be visible, run those checks
				if ( visible )
				{
					trace_t	tr;
					AI_TraceHull( pPath->GetAbsOrigin(), targetPos, -Vector(4,4,4), Vector(4,4,4), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

					// Check to see if we've hit the target, or the player's vehicle if it's a player in a vehicle
					bool bHitTarget = false;
					if ( GetTrackPatherTargetEnt() == tr.m_pEnt || pVehicle == tr.m_pEnt )
					{
						bHitTarget = true;
					}
					if ( tr.fraction == 1.0f || ( !m_bForcedMove && GetTrackPatherTargetEnt() && bHitTarget ) )
					{
						pNearestPath	= pPath;
						flNearestDist	= flPathDist;
					}
				}
				else
				{
					pNearestPath	= pPath;
					flNearestDist	= flPathDist;
				}
			}

			// Move to the next path point
			pPath = ( dir == FORWARD ) ? pPath->GetNext() : pPath->GetPrevious();

			// Debug
			if ( ( g_debug_trackpather.GetInt() == TRACKPATHER_DEBUG_PATH ) && ( CPathTrack::ValidPath( pPath ) != NULL ) )
			{
				NDebugOverlay::Line( ( dir == FORWARD ) ? pPath->GetPrevious()->GetAbsOrigin() : pPath->GetNext()->GetAbsOrigin(), pPath->GetAbsOrigin(), 0, 0, 255, true, 0.1f );
			}

			// Check for a cyclical path
			if ( pPath == m_pCurrentPathTarget )
				return pNearestPath;
		}

		pPath = pInitPath;

	}

	return pNearestPath;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_TrackPather::ArrivedAtPathPoint( CPathTrack *pPathPoint, float goalThreshold )
{
	if ( CPathTrack::ValidPath( pPathPoint ) == NULL )
		return false;

	//See if we're within the threshold
	if ( ( pPathPoint->GetAbsOrigin() - WorldSpaceCenter() ).Length() < goalThreshold )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CPathTrack
//-----------------------------------------------------------------------------
CPathTrack *CAI_TrackPather::FindNextPathPoint( CPathTrack *pStart, CPathTrack *pDest )
{
	// Make sure we're not trying to go nowhere
	if ( pStart == pDest )
		return pDest;

	CPathTrack *pNext = pStart->GetNext();

	bool	forwardFound = false;
	bool	backwardFound = false;
	float	forwardCost = 0.0f;
	float	backwardCost = 0.0f;
	int		loopRuns = 0;

	// Try going forward through the list
	while ( CPathTrack::ValidPath( pNext ) )
	{
		// Sanity check our path walk
		if ( ( loopRuns++ ) > 1024 )
		{
			// This means that the path is either really large, or we've slipped through a continuous loop somehow
			DevWarning( "%s: Cyclical path found in path_track connections, check links!\n", GetClassname() );
			assert(0);
			break;
		}

		// Found the target, early out
		if ( pNext == pDest )
		{
			forwardFound = true;
			break;
		}

		// Check for erroneous loop-through
		if ( pNext == pStart )
		{
			//NOTENOTE: We wrapped?  Should never happen
			DevWarning( "%s: Cyclical path found in path_track connections, check links!\n", GetClassname() );
			assert(0);
		}

		if ( pNext->GetNext() != NULL )
		{
			//FIXME: Cache in the path track
			forwardCost += ( pNext->GetAbsOrigin() - pNext->GetNext()->GetAbsOrigin() ).Length();
		}

		pNext = pNext->GetNext();
	}

	// Start over
	pNext	 = pStart->GetPrevious();
	loopRuns = 0;

	// Try going backward through the lsit
	while ( CPathTrack::ValidPath( pNext ) )
	{
		// Sanity check our path walk
		if ( ( loopRuns++ ) > 1024 )
		{
			// This means that the path is either really large, or we've slipped through a continuous loop somehow
			DevWarning( "%s: Cyclical path found in path_track connections, check links!\n", GetClassname() );
			assert(0);
			break;
		}

		// If we're already more expensive than going forward, early-out
		if ( forwardFound && ( backwardCost >= forwardCost ) )
			break;

		// Found the target, early out
		if ( pNext == pDest )
		{
			backwardFound = true;
			break;
		}

		// Check for erroneous loop-through
		if ( pNext == pStart )
		{
			DevWarning( "%s: Cyclical path found in path_track connections, check links!\n", GetClassname() );
			assert(0);
		}

		if ( pNext->GetPrevious() != NULL )
		{
			//FIXME: Cache in the path track
			backwardCost += ( pNext->GetAbsOrigin() - pNext->GetPrevious()->GetAbsOrigin() ).Length();
		}

		pNext = pNext->GetPrevious();
	}

	// If we found a path both ways, take the shortest route
	if ( forwardFound && backwardFound )
	{
		// Mask off the forward path if it's longer than going backwards
		if ( forwardCost >= backwardCost )
		{
			forwardFound = false;
		}
	}

	if ( forwardFound )
		return pStart->GetNext();
	
	if ( backwardFound )
		return pStart->GetPrevious();

	// NOTENOTE: No path was found?
	// DevWarning( "Unable to find path to: %s\n", m_pDestPathTarget->GetEntityName() );
	// assert(0);
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CAI_TrackPather::GetPathGoalPoint( float leadDist )
{
	// If our points are invalid, just stay put
	if ( ( m_pCurrentPathTarget == NULL ) && ( m_pDestPathTarget == NULL ) )
		return WorldSpaceCenter();

	/*
	// If we're at the target, just return the target's position
	if ( m_pCurrentPathTarget == m_pDestPathTarget )
		return m_pDestPathTarget->GetAbsOrigin();
	*/

	// Find our base offset from our current path point
	float	totalPathLength = ( m_pCurrentPathTarget->GetAbsOrigin() - WorldSpaceCenter() ).Length();

	// If we're already past the maximum, then just close distance
	if ( totalPathLength >= leadDist )
		return m_pCurrentPathTarget->GetAbsOrigin();

	CPathTrack *pNext = FindNextPathPoint( m_pCurrentPathTarget, m_pDestPathTarget );
	CPathTrack *pPrev = m_pCurrentPathTarget;

	Vector	segmentDir;
	float	segmentLength;
	int		loopRuns = 0;

	// Iterate through the path, building our line's length
	while ( CPathTrack::ValidPath( pNext ) )
	{
		// Sanity check our path walk
		if ( ( loopRuns++ ) > 1024 )
		{
			// This means that the path is either really large, or we've slipped through a continuous loop somehow
			DevWarning( "%s: Cyclical path found in path_track connections, check links!\n", GetClassname() );
			assert(0);
			break;
		}

		// Find this segment's length
		segmentDir = ( pNext->GetAbsOrigin() - pPrev->GetAbsOrigin() );
		segmentLength = VectorNormalize( segmentDir );

		// See if we're over the max now
		if ( ( totalPathLength + segmentLength ) >= leadDist )
		{
			// Difference left over
			float	leadDiff = leadDist - totalPathLength;

			// Offset the position by that amount
			Vector	offsetPos = pPrev->GetAbsOrigin() + ( segmentDir *  leadDiff );
			
			// Debug
			if ( g_debug_trackpather.GetInt() == TRACKPATHER_DEBUG_LEADING )
			{	
				NDebugOverlay::Cross3D( pPrev->GetAbsOrigin(), -Vector(16,16,16), Vector(16,16,16), 0, 255, 0, true, 0.1f );
				NDebugOverlay::Cross3D( offsetPos, -Vector(16,16,16), Vector(16,16,16), 0, 255, 0, true, 0.1f );
				NDebugOverlay::Line( pPrev->GetAbsOrigin(), offsetPos, 0, 255, 0, true, 0.1f );
			}

			return offsetPos;
		}

		// Debug
		if ( g_debug_trackpather.GetInt() == TRACKPATHER_DEBUG_LEADING )
		{
			NDebugOverlay::Cross3D( pPrev->GetAbsOrigin(), -Vector(16,16,16), Vector(16,16,16), 0, 255, 0, true, 0.1f );
			NDebugOverlay::Line( pPrev->GetAbsOrigin(), pNext->GetAbsOrigin(), 0, 255, 0, true, 0.1f );
		}

		// Add this to our path length
		totalPathLength += segmentLength;

		// Move to our next point
		pPrev = pNext;
		pNext = FindNextPathPoint( pNext, m_pDestPathTarget );
		
		// See if we're at our destination
		if ( pNext == m_pDestPathTarget )
			return m_pDestPathTarget->GetAbsOrigin();
	}

	// Stay put
	if ( pPrev == NULL )
		return WorldSpaceCenter();

	// Head towards our last known position
	return pPrev->GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : string - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_TrackPather::SetTrack( string_t strTrackName )
{
	// Find our specified target
	CBaseEntity *pGoalEnt = gEntList.FindEntityByName( NULL, strTrackName, NULL );

	if ( pGoalEnt == NULL )
	{
		DevWarning( "%s: Could not find path_track '%s'!\n", GetClassname(), strTrackName );
		return false;
	}
	return SetTrack( pGoalEnt );
}

//-----------------------------------------------------------------------------
bool CAI_TrackPather::SetTrack( CBaseEntity *pGoalEnt )
{
	VPROF_BUDGET( "CAI_TrackPather::SetTrack", VPROF_BUDGETGROUP_NPCS );

	if ( !pGoalEnt )
		return false;

	CPathTrack *pPathTrack = dynamic_cast<CPathTrack *>(pGoalEnt);

	if ( !pPathTrack )
		return false;

	// Set our fallback goal position to this (will be stomped later by lead code)
	m_vecDesiredPosition = pGoalEnt->GetAbsOrigin();

	//FIXME: orientation removed from path_corners!
	//FIXME: This isn't really used
	AngleVectors( pGoalEnt->GetLocalAngles(), &m_vecGoalOrientation );

	// Set our destination to this path for now (in case there's no enemy to chase)
	m_pDestPathTarget = CPathTrack::ValidPath( dynamic_cast<CPathTrack *>(pGoalEnt) );

	// Setup our new nearest point on the new track
	m_pCurrentPathTarget = m_pDestPathTarget; // Reset track
	m_pCurrentPathTarget = BestPointOnPath( WorldSpaceCenter(), 0.0f, false );
	
	//FIXME: Leave?
	//m_pLastPathTarget = NULL;

	// Update the path immediately
	m_flPathUpdateTime	= gpGlobals->curtime;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputSetTrack( inputdata_t &inputdata )
{
	SetTrack( MAKE_STRING( inputdata.value.String() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputFlyToPathTrack( inputdata_t &inputdata )
{
	SetTrack( MAKE_STRING( inputdata.value.String() ) );
	m_bForcedMove = true;

	//FIXME: This is already done in SetTrack
	//Find our closest point to start flying to
	//m_pCurrentPathTarget = BestPointOnPath( WorldSpaceCenter(), 0.0f, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputStartPatrol( inputdata_t &inputdata )
{
	m_bPatrolBreakable = false;
	m_bPatrolling = true;	
	UpdatePatrol();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_TrackPather::InputStartPatrolBreakable( inputdata_t &inputdata )
{
	m_bPatrolBreakable = true;
	m_bPatrolling = true;	
	UpdatePatrol();
}

//-----------------------------------------------------------------------------

CBaseEntity *CAI_TrackPather::GetCurPathTarget()
{ 
	return m_pCurrentPathTarget; 
}

