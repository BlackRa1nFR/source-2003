//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "ai_link.h"
#include "ai_navtype.h"
#include "ai_waypoint.h"
#include "ai_pathfinder.h"
#include "ai_navgoaltype.h"
#include "ai_routedist.h"
#include "ai_route.h"

#ifdef MEASURE_AI
	#include "measure_section.h"
#endif

BEGIN_SIMPLE_DATADESC(CAI_Path)
	//					m_Waypoints	(reconsititute on load)
	DEFINE_FIELD( CAI_Path, m_goalTolerance,	FIELD_FLOAT ),
	DEFINE_FIELD( CAI_Path,	m_activity,			FIELD_INTEGER ),
	DEFINE_FIELD( CAI_Path,	m_target,			FIELD_EHANDLE ),
	DEFINE_FIELD( CAI_Path,	m_vecTargetOffset,	FIELD_VECTOR ),
	DEFINE_FIELD( CAI_Path,	m_waypointTolerance, FIELD_FLOAT ),
	//					m_iLastNodeReached
	DEFINE_FIELD( CAI_Path,	m_bGoalPosSet,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_Path,	m_goalPos,			FIELD_POSITION_VECTOR),
	DEFINE_FIELD( CAI_Path,	m_bGoalTypeSet,		FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_Path,	m_goalType,			FIELD_INTEGER ),
	DEFINE_FIELD( CAI_Path,	m_goalFlags,		FIELD_INTEGER ),
	DEFINE_FIELD( CAI_Path,	m_goalDirection,	FIELD_VECTOR ),
	DEFINE_FIELD( CAI_Path,	m_goalDirectionTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CAI_Path,	m_bGoalVelocitySet,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_Path,	m_goalVelocity,	FIELD_VECTOR )
END_DATADESC()

//-----------------------------------------------------------------------------
AI_Waypoint_t CAI_Path::gm_InvalidWaypoint( Vector(0,0,0), 0, NAV_NONE, 0, 0 );

//-----------------------------------------------------------------------------

void CAI_Path::SetWaypoints(AI_Waypoint_t* route, bool fSetGoalFromLast) 
{ 
	m_Waypoints.Set(route); 

	if ( fSetGoalFromLast )
	{
		AI_Waypoint_t *pLast = m_Waypoints.GetLast();
		if ( pLast )
			SetGoalPosition(pLast->GetPos());
	}
}

//-----------------------------------------------------------------------------

Activity CAI_Path::GetArrivalActivity( ) const
{
	const AI_Waypoint_t *pLast = m_Waypoints.GetLast();
	if ( pLast )
	{
		return pLast->activity;
	}
	return ACT_INVALID;
}

//-----------------------------------------------------------------------------

void CAI_Path::SetArrivalActivity(Activity activity)
{
	AI_Waypoint_t *pLast = m_Waypoints.GetLast();
	if ( pLast )
	{
		pLast->activity = activity;
		pLast->sequence = ACT_INVALID;
	}
}

//-----------------------------------------------------------------------------

int CAI_Path::GetArrivalSequence( ) const
{
	const AI_Waypoint_t *pLast = m_Waypoints.GetLast();
	if ( pLast )
	{
		return pLast->sequence;
	}
	return ACT_INVALID;
}

//-----------------------------------------------------------------------------

void CAI_Path::SetArrivalSequence( int sequence )
{
	AI_Waypoint_t *pLast = m_Waypoints.GetLast();
	if ( pLast )
	{
		pLast->activity = ACT_INVALID;
		pLast->sequence = sequence;
	}
}


//-----------------------------------------------------------------------------

void CAI_Path::SetGoalDirection( const Vector &goalDirection )
{
	m_goalDirectionTarget = NULL;
	m_goalDirection = goalDirection;
	/*
	AI_Waypoint_t *pLast = m_Waypoints.GetLast();
	if ( pLast )
	{
		NDebugOverlay::Box( pLast->vecLocation, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 0,0,255, 0, 2.0 );
		NDebugOverlay::Line( pLast->vecLocation, pLast->vecLocation + m_goalDirection * 32, 0,0,255, true, 2.0 );
	}
	*/
}


//-----------------------------------------------------------------------------

void CAI_Path::SetGoalDirection( CBaseEntity *pTarget )
{
	m_goalDirectionTarget = pTarget;

	if (pTarget)
	{
		AI_Waypoint_t *pLast = m_Waypoints.GetLast();
		if ( pLast )
		{
			m_goalDirection = pTarget->GetAbsOrigin() - pLast->vecLocation;
			VectorNormalize( m_goalDirection );
			/*
			NDebugOverlay::Box( pLast->vecLocation, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 0,0,255, 0, 2.0 );
			NDebugOverlay::Line( pLast->vecLocation, pLast->vecLocation + m_goalDirection * 32, 0,0,255, true, 2.0 );
			*/
		}
	}
}

//-----------------------------------------------------------------------------

Vector CAI_Path::GetGoalDirection( )
{
	if (m_goalDirectionTarget)
	{
		AI_Waypoint_t *pLast = m_Waypoints.GetLast();
		if ( pLast )
		{
			m_goalDirection = m_goalDirectionTarget->GetAbsOrigin() - pLast->vecLocation;
			VectorNormalize( m_goalDirection );
		}
	}
	return m_goalDirection;
}

//-----------------------------------------------------------------------------
const Vector &CAI_Path::CurWaypointPos() const	
{ 
	if ( GetCurWaypoint() )
		return GetCurWaypoint()->GetPos(); 
	AssertMsg(0, "Invalid call to CurWaypointPos()");
	return gm_InvalidWaypoint.GetPos();
}

//-----------------------------------------------------------------------------
const Vector &CAI_Path::NextWaypointPos() const
{ 
	if ( GetCurWaypoint() && GetCurWaypoint()->GetNext())
		return GetCurWaypoint()->GetNext()->GetPos(); 
	static Vector invalid( 0, 0, 0 );
	AssertMsg(0, "Invalid call to NextWaypointPos()");
	return gm_InvalidWaypoint.GetPos();
}

//-----------------------------------------------------------------------------
float CAI_Path::CurWaypointYaw() const
{ 
	return GetCurWaypoint()->flYaw; 
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Path::SetGoalPosition(const Vector &goalPos) 
{

#ifdef _DEBUG
	// Make sure goal position isn't set more than once
	if (m_bGoalPosSet == true)
	{
		Msg( "GetCurWaypoint Goal Position Set Twice!\n");
	}
#endif

	m_bGoalPosSet = true;
	VectorAdd( goalPos, m_vecTargetOffset, m_goalPos );
}

//-----------------------------------------------------------------------------
// Purpose: Sets last node as goal and goal position
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Path::SetLastNodeAsGoal(void)
{
	#ifdef _DEBUG
		// Make sure goal position isn't set more than once
		if (m_bGoalPosSet == true)
		{
			Msg( "GetCurWaypoint Goal Position Set Twice!\n");
		}
	#endif	
	
	// Find the last node
	if (GetCurWaypoint()) 
	{
		AI_Waypoint_t* waypoint = GetCurWaypoint();

		while (waypoint)
		{
			if (!waypoint->GetNext())
			{
				m_goalPos = waypoint->GetPos();
				m_bGoalPosSet = true;
				waypoint->ModifyFlags( bits_WP_TO_GOAL, true );
				return;
			}
			waypoint = waypoint->GetNext();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Explicitly change the goal position w/o check
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Path::ResetGoalPosition(const Vector &goalPos) 
{
	m_bGoalPosSet	= true;
	VectorAdd( goalPos, m_vecTargetOffset, m_goalPos );
}


//-----------------------------------------------------------------------------
// Returns the *base* goal position (without the offset applied) 
//-----------------------------------------------------------------------------
const Vector& CAI_Path::BaseGoalPosition() const
{
#ifdef _DEBUG
	// Make sure goal position was set
	if (m_bGoalPosSet == false)
	{
		Msg( "GetCurWaypoint Goal Position Never Set!\n");
	}
#endif

	// FIXME: A little risky; store the base if this becomes a problem
	static Vector vecResult;
	VectorSubtract(	m_goalPos, m_vecTargetOffset, vecResult );
	return vecResult;
}


//-----------------------------------------------------------------------------
	// Returns the *actual* goal position (with the offset applied)
//-----------------------------------------------------------------------------
const Vector & CAI_Path::ActualGoalPosition(void) const
{
#ifdef _DEBUG
	// Make sure goal position was set
	if (m_bGoalPosSet == false)
	{
		Msg( "GetCurWaypoint Goal Position Never Set!\n");
	}
#endif

	return m_goalPos;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Path::SetGoalType(GoalType_t goalType) 
{

#ifdef _DEBUG
	// Make sure goal position isn't set more than once
	if (m_goalType != GOALTYPE_NONE)
	{
		Msg( "GetCurWaypoint Goal Type Set Twice!\n");
	}
	else if (goalType == GOALTYPE_NONE)
	{
		Msg( "GetCurWaypoint Goal Type Set Illegally!\n");
	}
#endif

	m_bGoalTypeSet	= true;
	m_goalType		= goalType;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
GoalType_t	CAI_Path::GoalType(void) const
{
	return m_goalType;
}


//-----------------------------------------------------------------------------

void CAI_Path::Advance( void )
{
	if ( CurWaypointIsGoal() )
		return;

	// -------------------------------------------------------
	// If I have another waypoint advance my path
	// -------------------------------------------------------
	if (GetCurWaypoint()->GetNext()) 
	{
		AI_Waypoint_t *pNext = GetCurWaypoint()->GetNext();

		// If waypoint was a node take note of it
		if (GetCurWaypoint()->Flags() & bits_WP_TO_NODE)
		{
			m_iLastNodeReached = GetCurWaypoint()->iNodeID;
		}

		delete GetCurWaypoint();
		SetWaypoints(pNext);

		return;
	}
	// -------------------------------------------------
	//  This is an error catch that should *not* happen
	//  It means a route was created with no goal
	// -------------------------------------------------
	else 
	{
		Msg( "!!ERROR!! Force end of route with no goal!\n");
		GetCurWaypoint()->ModifyFlags( bits_WP_TO_GOAL, true );
	}
}



//-----------------------------------------------------------------------------
// Purpose: Clears the route and resets all its fields to default values
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Path::Clear( void )
{
	m_Waypoints.RemoveAll();

	m_goalType			= GOALTYPE_NONE;		// Type of goal
	m_goalPos			= vec3_origin;			// Our ultimate goal position
	m_bGoalPosSet		= false;				// Was goal position set
	m_bGoalTypeSet		= false;				// Was goal position set
	m_goalFlags			= false;
	m_vecTargetOffset	= vec3_origin;

	m_goalTolerance		= 0.0;					// How close do we need to get to the goal
	// FIXME: split m_goalTolerance into m_buildTolerance and m_moveTolerance, let them be seperatly controllable.

	m_activity			= ACT_IDLE;
	m_target			= NULL;


}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Navigation_t CAI_Path::CurWaypointNavType()	const
{
	if (!GetCurWaypoint())
	{
		return NAV_NONE;
	}
	else
	{
		return GetCurWaypoint()->NavType();
	}
}

int CAI_Path::CurWaypointFlags() const
{
	if (!GetCurWaypoint())
	{
		return 0;
	}
	else
	{
		return GetCurWaypoint()->Flags();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Get the goal's flags
// Output : unsigned
//-----------------------------------------------------------------------------
unsigned CAI_Path::GoalFlags( void ) const
{
	return m_goalFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if current waypoint is my goal
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Path::CurWaypointIsGoal( void ) const
{
//	Assert( GetCurWaypoint() );

	if( !GetCurWaypoint() )
		return false;


	if ( GetCurWaypoint()->Flags() & bits_WP_TO_GOAL )
	{
		#ifdef _DEBUG
			if (GetCurWaypoint()->GetNext())
			{
				Msg( "!!ERROR!! Goal is not last waypoint!\n");
			}
			if ((GetCurWaypoint()->GetPos() - m_goalPos).Length() > 0.1)
			{
				Msg( "!!ERROR!! Last waypoint isn't in goal position!\n");
			}
		#endif
		return true;
	}
	if ( GetCurWaypoint()->Flags() & bits_WP_TO_PATHCORNER )
	{
		// UNDONE: Refresh here or somewhere else?
	}
#ifdef _DEBUG
	if (!GetCurWaypoint()->GetNext())
	{
		Msg( "!!ERROR!! GetCurWaypoint has no goal!\n");
	}
#endif

	return false;
}


//-----------------------------------------------------------------------------
// Computes the goal distance for each waypoint along the route
//-----------------------------------------------------------------------------
void CAI_Path::ComputeRouteGoalDistances(AI_Waypoint_t *pGoalWaypoint)
{
	// The goal distance is the distance from any waypoint to the goal waypoint

	// Backup through the list and calculate distance to goal
	AI_Waypoint_t *pPrev;
	AI_Waypoint_t *pCurWaypoint = pGoalWaypoint;
	pCurWaypoint->flPathDistGoal = 0;
	while (pCurWaypoint->GetPrev())
	{
		pPrev = pCurWaypoint->GetPrev();

		float flWaypointDist = ComputePathDistance(pCurWaypoint->NavType(), pPrev->GetPos(), pCurWaypoint->GetPos());
		pPrev->flPathDistGoal = pCurWaypoint->flPathDistGoal + flWaypointDist;
		
		pCurWaypoint = pPrev;
	}
}



//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Path::CAI_Path()
{
	m_goalType			= GOALTYPE_NONE;		// Type of goal
	m_goalPos			= vec3_origin;			// Our ultimate goal position
	m_goalTolerance		= 0.0;					// How close do we need to get to the goal
	m_activity			= ACT_IDLE;				// The activity to use during motion
	m_target			= NULL;
	m_goalFlags			= 0;

	m_iLastNodeReached  = NO_NODE;

	m_waypointTolerance = DEF_WAYPOINT_TOLERANCE;

}

CAI_Path::~CAI_Path()
{
	DeleteAll( GetCurWaypoint() );
}


