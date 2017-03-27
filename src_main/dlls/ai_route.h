//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_ROUTE_H
#define AI_ROUTE_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "vector.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_waypoint.h"

struct AI_Waypoint_t;
struct OverlayLine_t;

class CAI_BaseNPC;

//=============================================================================
//	>> CAI_Path
//=============================================================================

#define DEF_WAYPOINT_TOLERANCE (0.1)

class CAI_Path
{
//-----------------------------------------------------------------

public:

	void			SetWaypoints(AI_Waypoint_t* route, bool fSetGoalFromLast = false) ;

	bool 			PrependWaypoints( AI_Waypoint_t *pWaypoints ) { return m_Waypoints.PrependWaypoints( pWaypoints ); }
	void 			PrependWaypoint( const Vector &newPoint, Navigation_t navType, unsigned waypointFlags ) { m_Waypoints.PrependWaypoint( newPoint, navType, waypointFlags ); }
	bool 			IsEmpty() const				{ return m_Waypoints.IsEmpty(); }

	AI_Waypoint_t *	GetCurWaypoint() 				{ return m_Waypoints.GetFirst(); }
	const AI_Waypoint_t *GetCurWaypoint() const 	{ return m_Waypoints.GetFirst(); }

	AI_Waypoint_t *	GetGoalWaypoint() 				{ return m_Waypoints.GetLast(); }
	const AI_Waypoint_t *GetGoalWaypoint() const 	{ return m_Waypoints.GetLast(); }

	const Vector &	CurWaypointPos() const;
	const Vector &	NextWaypointPos() const;
	float			CurWaypointYaw() const;
	int				CurWaypointFlags() const;
	Navigation_t	CurWaypointNavType() const;

	AI_Waypoint_t * GetTransitionWaypoint();

	//---------------------------------
	// How close do we need to get to the goal
	//---------------------------------
	void			SetGoalTolerance(float tolerance)	{ m_goalTolerance = tolerance;		}
	float			GetGoalTolerance() const			{ return m_goalTolerance;			}

	void			SetWaypointTolerance(float tolerance)	{ m_waypointTolerance = tolerance;  }
	float			GetWaypointTolerance() const			{ return m_waypointTolerance;  }

	//---------------------------------
	// The activity to use during motion
	//---------------------------------
	Activity		GetMovementActivity() const					{ return m_activity;				}
	Activity		SetMovementActivity(Activity activity)		{ return (m_activity = activity);	}

	Activity		GetArrivalActivity( ) const;
	void			SetArrivalActivity(Activity activity);
	int				GetArrivalSequence( ) const;
	void			SetArrivalSequence(int sequence);

	void			SetGoalDirection( const Vector &goalDirection );
	void			SetGoalDirection( CBaseEntity *pTarget );
	Vector			GetGoalDirection( );


	//---------------------------------
	// Target of this path
	//---------------------------------
	void			SetTarget(CBaseEntity * pTarget )	{ m_target = pTarget;				}
	void			ClearTarget()						{ m_target = NULL; m_vecTargetOffset = vec3_origin;	}
	void			SetTargetOffset( const Vector &vecOffset)	{ m_vecTargetOffset = vecOffset;	}
	CBaseEntity *	GetTarget()							{ return m_target;					}

	void			SetGoalType(GoalType_t goalType);				// Set the goal type
	void			SetGoalPosition(const Vector &goalPos);			// Set the goal position
	void			SetLastNodeAsGoal(void);						// Sets last node as goal and goal position
	void			ResetGoalPosition(const Vector &goalPos);		// Reset the goal position

	// Returns the *base* goal position (without the offset applied) 
	const Vector&	BaseGoalPosition() const;

	// Returns the *actual* goal position (with the offset applied)
	const Vector &	ActualGoalPosition(void) const;			// Get the goal position

	GoalType_t		GoalType(void) const;						// Get the goal type
	
	void			SetGoalFlags( unsigned flags )		{ m_goalFlags = flags;				}
	unsigned		GoalFlags( void ) const;			// Get the goal flags

	void			Advance( void );					// Advance to next waypoint if possible

	bool			CurWaypointIsGoal(void) const;

	void			Clear(void);


	void			UpdateTransitions();

	CAI_Path();
	~CAI_Path();

	//---------------------------------

	int GetLastNodeReached() { return m_iLastNodeReached; }
	void ClearWaypoints()
	{ 
		m_Waypoints.RemoveAll();
		m_iLastNodeReached = NO_NODE;
	}

private:

	// Computes the goal distance for each waypoint along the route
	void ComputeRouteGoalDistances(AI_Waypoint_t *pGoalWaypoint);

	//---------------------------------
	CAI_WaypointList m_Waypoints;

	//---------------------------------
	float		m_goalTolerance;			// How close do we need to get to the goal
	Activity	m_activity;					// The activity to use during motion
	EHANDLE		m_target;					// Target of this path
	Vector		m_vecTargetOffset;			// offset from the target in world space
	float		m_waypointTolerance;

	//---------------------------------
	int			m_iLastNodeReached;			// What was the last node that I reached

	bool		m_bGoalPosSet;				// Was goal position set (used to check for errors)
	Vector		m_goalPos;					// Our ultimate goal position

	bool		m_bGoalTypeSet;				// Was goal position set (used to check for errors)
	GoalType_t	m_goalType;					// Type of goal

	unsigned	m_goalFlags;				// Goal flags


	//---------------------------------
	Vector		m_goalDirection;
	EHANDLE		m_goalDirectionTarget;

	bool		m_bGoalVelocitySet;
	Vector		m_goalVelocity;

	//---------------------------------
	static AI_Waypoint_t gm_InvalidWaypoint;

	DECLARE_SIMPLE_DATADESC();

};

#endif // AI_ROUTE_H
