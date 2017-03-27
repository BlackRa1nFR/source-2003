//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
// @TODO (toml 06-26-02): The entry points in this file need to be organized
//=============================================================================

#include "cbase.h"

#include <float.h> // for FLT_MAX

#include "animation.h"		// for NOMOTION
#include "collisionutils.h"
#include "ndebugoverlay.h"

#include "ai_navigator.h"
#include "ai_node.h"
#include "ai_route.h"
#include "ai_routedist.h"
#include "ai_waypoint.h"
#include "ai_pathfinder.h"
#include "ai_link.h"
#include "ai_memory.h"
#include "ai_motor.h"
#include "ai_localnavigator.h"
#include "ai_moveprobe.h"
#include "BasePropDoor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

#ifdef DEBUG
bool g_fTestSteering = 0;
#endif

//-----------------------------------------------------------------------------

const Vector AIN_NO_DEST( FLT_MAX, FLT_MAX, FLT_MAX );
#define FLIER_CUT_CORNER_DIST		16 // 8 means the npc's bounding box is contained without the box of the node in WC

//-----------------------------------------------------------------------------
// class CAI_Navigator
//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_Navigator )

	DEFINE_FIELD( CAI_Navigator,	m_navType,					FIELD_INTEGER ),
	//								m_pMotor
	//								m_pMoveProbe
	//								m_pLocalNavigator
	//								m_pAINetwork
	DEFINE_EMBEDDEDBYREF( CAI_Navigator,	m_pPath ),
	//								m_pClippedWaypoints	(not saved)
	//								m_flTimeClipped		(not saved)
	DEFINE_FIELD( CAI_Navigator,	m_fNavComplete,				FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_Navigator,	m_bNotOnNetwork,			FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_Navigator,	m_bLastNavFailed,			FIELD_BOOLEAN ),
  	DEFINE_FIELD( CAI_Navigator,	m_flNextSimplifyTime,		FIELD_TIME ),
	DEFINE_FIELD( CAI_Navigator,	m_fForcedSimplify,			FIELD_FLOAT ),
  	DEFINE_FIELD( CAI_Navigator,	m_timePathRebuildMax,		FIELD_FLOAT ),
  	DEFINE_FIELD( CAI_Navigator,	m_timePathRebuildDelay,		FIELD_FLOAT ),
  	DEFINE_FIELD( CAI_Navigator,	m_timePathRebuildFail,		FIELD_TIME ),
  	DEFINE_FIELD( CAI_Navigator,	m_timePathRebuildNext,		FIELD_TIME ),
	DEFINE_FIELD( CAI_Navigator,	m_fRememberStaleNodes,		FIELD_BOOLEAN ),
	// 								m_fPeerMoveWait		(think transient)
	//								m_hPeerWaitingOn	(peer move fields do not need to be saved, tied to current schedule and path, which are not saved)
	DEFINE_EMBEDDED( CAI_Navigator,	m_PeerWaitMoveTimer ),
	DEFINE_EMBEDDED( CAI_Navigator,	m_PeerWaitClearTimer ),

END_DATADESC()


//-----------------------------------------------------------------------------

CAI_Navigator::CAI_Navigator(CAI_BaseNPC *pOuter)
 :	CAI_Component(pOuter)
{
	m_pPath					= new CAI_Path;
	m_pAINetwork			= NULL;
	m_bNotOnNetwork			= false;
	m_flNextSimplifyTime	= 0;

	m_pClippedWaypoints		= new CAI_WaypointList;
	m_flTimeClipped			= -1;

	// ----------------------------
	
	m_navType = NAV_GROUND;
	m_fNavComplete = false;
	m_bLastNavFailed = false;
	
	// ----------------------------

	m_fRememberStaleNodes = true;
	
	m_PeerWaitMoveTimer.Set(0.25); // 2 thinks
	m_PeerWaitClearTimer.Set(3.0);

	m_pMotor = NULL;
	m_pMoveProbe = NULL;
	m_pLocalNavigator = NULL;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::Init( CAI_Network *pNetwork )
{
	m_pMotor = GetOuter()->GetMotor();
	m_pMoveProbe = GetOuter()->GetMoveProbe();
	m_pLocalNavigator = GetOuter()->GetLocalNavigator();
	m_pAINetwork = pNetwork;
}

//-----------------------------------------------------------------------------

CAI_Navigator::~CAI_Navigator()
{
	delete m_pPath;
	delete m_pClippedWaypoints;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::ActivityIsLocomotive( Activity activity ) 
{
	return ( activity > ACT_IDLE ); 
}

//-----------------------------------------------------------------------------

CAI_Pathfinder *CAI_Navigator::GetPathfinder()
{ 
	return GetOuter()->GetPathfinder(); 
}

//-----------------------------------------------------------------------------

const CAI_Pathfinder *CAI_Navigator::GetPathfinder() const
{ 
	return GetOuter()->GetPathfinder(); 
}

//-----------------------------------------------------------------------------

CBaseEntity *CAI_Navigator::GetNavTargetEntity()
{
	if ( GetGoalType() == GOALTYPE_ENEMY || GetGoalType() == GOALTYPE_TARGETENT )
		return GetOuter()->GetNavTargetEntity();
	return GetPath()->GetTarget();
}

//-----------------------------------------------------------------------------

void CAI_Navigator::TaskMovementComplete()
{
	GetOuter()->TaskMovementComplete();
}

//-----------------------------------------------------------------------------

float CAI_Navigator::MaxYawSpeed()
{
	return GetOuter()->MaxYawSpeed();
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetSpeed( float fl )
{
	GetOuter()->m_flSpeed = fl;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetGoal( const AI_NavGoal_t &goal, unsigned flags )
{
	CAI_Path *pPath = GetPath();

	OnNewGoal();
		
	//Clear out previous state
	if ( flags & AIN_CLEAR_PREVIOUS_STATE )
		ClearPath();
	else if ( flags & AIN_CLEAR_TARGET )
		pPath->ClearTarget();

	//Set the activity
	if ( goal.activity != AIN_DEF_ACTIVITY )
		pPath->SetMovementActivity( goal.activity );
		
	//Set the tolerance
	if ( goal.tolerance == AIN_HULL_TOLERANCE )
		pPath->SetGoalTolerance( GetHullWidth() );
	else if ( goal.tolerance != AIN_DEF_TOLERANCE )
		pPath->SetGoalTolerance( goal.tolerance );
	else if (pPath->GetGoalTolerance() == 0 )
		pPath->SetGoalTolerance( GetHullWidth() * 2 );

	pPath->SetWaypointTolerance( GetHullWidth() * 0.5 );

	//See if we're only attempted to get to the nearest node
	if ( goal.flags & AIN_NEAREST_NODE )
	{
		// @TODO (toml 07-01-02): this is logic originally homed in the antlion flee logic.
		// The intent is to set a goal that is less likely to fail given a randomly selected
		// point by not requiring that the point be navigable, so long as there exists
		// a nearby node that is. The logic is here now, but as the lower level are recast
		// in terms of the goal structure, this should eventually end up in CAI_Route::Build()
		Assert( goal.type == GOALTYPE_LOCATION );
			
		int myNodeID;
		int destNodeID;
		
		myNodeID = GetNetwork()->NearestNodeToNPCAtPoint( GetOuter(), GetAbsOrigin() );
		if (myNodeID == NO_NODE)
		{
			// @TODO (toml 07-01-02): need to propogate better failure codes from SetGoal()
			//TaskFail("No node near me");
			return false;
		}
		
		destNodeID = GetNetwork()->NearestNodeToPoint( goal.dest );
		if (destNodeID == NO_NODE)
		{
			//TaskFail("No node to flee to");
			return false;
		}

		AI_Waypoint_t *pRoute = GetPathfinder()->FindBestPath( myNodeID, destNodeID );
		
		if ( pRoute == NULL )
			return false;

		pPath->SetGoalType( GOALTYPE_LOCATION );
		pPath->SetWaypoints( pRoute );
		pPath->SetLastNodeAsGoal();

		return true;
	}

	// NOTE: Setting this after clearing the previous state
	// avoids a spurious warning about setting the goal type twice
	pPath->SetGoalType( goal.type );
	pPath->SetGoalFlags( goal.flags );
		
	CBaseEntity *pPathTarget = goal.pTarget;
	if ((goal.type == GOALTYPE_TARGETENT) || (goal.type == GOALTYPE_ENEMY))
	{
		// Guarantee that the path target 
		if (goal.type == GOALTYPE_TARGETENT)
			pPathTarget = GetTarget(); 
		else
			pPathTarget = GetEnemy();

		Assert( goal.pTarget == AIN_DEF_TARGET || goal.pTarget == pPathTarget );

		// Set the goal offset
		if ( goal.dest != AIN_NO_DEST )
			pPath->SetTargetOffset( goal.dest );

		// We're not setting the goal position here because
		// it's going to be computed + set in DoFindPath.
	}
	else
	{
		// When our goaltype is position based, we have to set
		// the goal position here because it won't get set during DoFindPath().
		if ( goal.dest != AIN_NO_DEST )
			pPath->ResetGoalPosition( goal.dest );
		else if ( goal.destNode != AIN_NO_NODE )
			pPath->ResetGoalPosition( GetNodePos( goal.destNode ) );
	}

	if ( pPathTarget > AIN_DEF_TARGET )
	{
		pPath->SetTarget( pPathTarget );
	}

	bool result = FindPath();

	if ( result == false )
	{
		if ( flags & AIN_DISCARD_IF_FAIL )
			ClearPath();
	}
	else
	{
		// Set our ideal yaw. This has to be done *after* finding the path, 
		// because the goal position may not be set until then
		if ( goal.flags & AIN_YAW_TO_DEST )
			GetMotor()->SetIdealYawToTarget( pPath->ActualGoalPosition() );
		SimplifyPath( true );
		
		if ( goal.arrivalActivity != AIN_DEF_ACTIVITY )
		{
			pPath->SetArrivalActivity( goal.arrivalActivity );
		}
		else if ( goal.arrivalSequence != -1 )
		{
			pPath->SetArrivalSequence( goal.arrivalSequence );
		}
	}
	
	return result;
}


//-----------------------------------------------------------------------------
// Change the target of the path
//-----------------------------------------------------------------------------
bool CAI_Navigator::SetGoalTarget( CBaseEntity *pEntity, const Vector &offset )
{
	OnNewGoal();
	CAI_Path *pPath = GetPath();
	pPath->SetTargetOffset( offset );
	pPath->SetTarget( pEntity );
	return FindPath();
}


//-----------------------------------------------------------------------------

bool CAI_Navigator::SetRadialGoal( const Vector &center, float radius, float arc, float stepDist, bool bClockwise, bool bAirRoute)
{
	OnNewGoal();
	GetPath()->SetGoalType(GOALTYPE_LOCATION);

	GetPath()->SetWaypoints(GetPathfinder()->BuildRadialRoute( GetLocalOrigin(), center, radius, arc, stepDist, bClockwise, GetPath()->GetGoalTolerance(), bAirRoute ), true);			
	return IsGoalActive();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetRandomGoal( const Vector &from, float minPathLength, const Vector &dir )
{
	OnNewGoal();
	if ( GetNetwork()->NumNodes() <= 0 )
		return false;
		
	int fromNodeID = GetNetwork()->NearestNodeToNPCAtPoint( GetOuter(), from );
	
	if (fromNodeID == NO_NODE)
		return false;
		
	AI_Waypoint_t* pRoute = GetPathfinder()->FindShortRandomPath( fromNodeID, minPathLength, dir );

	if (!pRoute)
		return false;

	GetPath()->SetGoalType(GOALTYPE_LOCATION);
	GetPath()->SetWaypoints(pRoute);
	GetPath()->SetLastNodeAsGoal();
	
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetWanderGoal( float minRadius, float maxRadius )
{
	// @Note (toml 11-07-02): this is a bogus placeholder implementation!!!
	for ( int i = 0; i < 5; i++ )
	{
		float dist = random->RandomFloat( minRadius, maxRadius );
		Vector dir = UTIL_YawToVector( random->RandomFloat( 0, 359.99 ) );
		
		if ( SetVectorGoal( dir, dist, minRadius ) )
			return true;
	}
	
	return SetRandomGoal( 1 );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::CalculateDeflection( const Vector &start, const Vector &dir, const Vector &normal, Vector *pResult )
{
	Vector temp;
	CrossProduct( dir, normal, temp );
	CrossProduct( normal, temp, *pResult );
	VectorNormalize( *pResult );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetVectorGoal( const Vector &dir, float targetDist, float minDist, bool fShouldDeflect )
{
	AIMoveTrace_t moveTrace;
	float distAchieved = 0;
	
	Vector testLoc = GetLocalOrigin() + ( dir * targetDist );
	GetMoveProbe()->MoveLimit( GetNavType(), GetLocalOrigin(), testLoc, MASK_NPCSOLID, NULL, &moveTrace );
	
	if ( moveTrace.fStatus != AIMR_OK )
	{
		distAchieved = targetDist - moveTrace.flDistObstructed;
		if ( fShouldDeflect && moveTrace.vHitNormal != vec3_origin )
		{
			Vector vecDeflect;
			CalculateDeflection( moveTrace.vEndPosition, dir, moveTrace.vHitNormal, &vecDeflect );

			testLoc = moveTrace.vEndPosition + ( vecDeflect * ( targetDist - distAchieved ) );
	
			Vector temp = moveTrace.vEndPosition;
			GetMoveProbe()->MoveLimit( GetNavType(), temp, testLoc, MASK_NPCSOLID, NULL, &moveTrace );

			distAchieved += ( targetDist - distAchieved ) - moveTrace.flDistObstructed;
		}
		
		if ( distAchieved < minDist + 0.01 )
			return false;
	}
	
	return SetGoal( moveTrace.vEndPosition );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetVectorGoalFromTarget( const Vector &goalPos, float minDist, bool fShouldDeflect )
{
	Vector vDir = goalPos;
	float dist = ComputePathDirection( GetNavType(), GetLocalOrigin(), goalPos, &vDir );
	return SetVectorGoal( vDir, dist, minDist, fShouldDeflect );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetRandomGoal( float minPathLength, const Vector &dir )
{
	return SetRandomGoal( GetLocalOrigin(), minPathLength, dir );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::PrependLocalAvoidance( float distObstacle )
{
	if ( GetOuter()->GetEfficiency() > AIE_NORMAL )
		return false;

	AI_PROFILE_SCOPE(CAI_Navigator_PrependLocalAvoidance);

	AI_Waypoint_t *pAvoidanceRoute = NULL;
	
	pAvoidanceRoute = GetPathfinder()->BuildTriangulationRoute(
		GetLocalOrigin(), 
		GetCurWaypointPos(), 
		GetNavTargetEntity(), 
		bits_WP_TO_DETOUR, 
		NO_NODE,
		0.0, 
		distObstacle, 
		GetNavType() );
	
	if ( !pAvoidanceRoute )
		return false;
		
	// FIXME: should the route get simplified? 
	return GetPath()->PrependWaypoints( pAvoidanceRoute );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::PrependWaypoint( const Vector &newPoint, Navigation_t navType, unsigned waypointFlags )
{
	GetPath()->PrependWaypoint( newPoint, navType, waypointFlags );
}

//-----------------------------------------------------------------------------

const Vector &CAI_Navigator::GetGoalPos() const
{ 
	return GetPath()->BaseGoalPosition(); 
}

//-----------------------------------------------------------------------------

CBaseEntity *CAI_Navigator::GetGoalTarget()
{
	return GetPath()->GetTarget();
}

//-----------------------------------------------------------------------------

float CAI_Navigator::GetGoalTolerance() const
{
	return GetPath()->GetGoalTolerance(); 
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetGoalTolerance(float tolerance)
{
	GetPath()->SetGoalTolerance(tolerance);
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::RefindPathToGoal( bool fSignalTaskStatus )
{
	return FindPath( fSignalTaskStatus );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::UpdateGoalPos( const Vector &goalPos )
{
	// FindPath should be finding the goal position if the goal type is
	// one of these two... We could just ignore the suggested position
	// in these two cases I suppose!
	Assert( (GetPath()->GoalType() != GOALTYPE_ENEMY) && (GetPath()->GoalType() != GOALTYPE_TARGETENT) );

	GetPath()->ResetGoalPosition( goalPos );
	return FindPath();
}

//-----------------------------------------------------------------------------

Activity CAI_Navigator::SetMovementActivity(Activity activity)
{
	return GetPath()->SetMovementActivity( activity );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::ClearGoal() 
{ 
	ClearPath();
	OnNewGoal();
	return true; 
}

//-----------------------------------------------------------------------------

Activity CAI_Navigator::GetMovementActivity() const
{
	return GetPath()->GetMovementActivity();
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalActivity(Activity activity)
{
	GetPath()->SetArrivalActivity(activity);
}


//-----------------------------------------------------------------------------

int CAI_Navigator::GetArrivalSequence( )
{
	int sequence = GetPath()->GetArrivalSequence( );
	if (sequence == ACT_INVALID)
	{
		Activity activity = GetOuter()->GetStoppedActivity();
		Assert( activity != ACT_INVALID );

		sequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( activity ) );

		if ( sequence == ACT_INVALID )
		{
			Msg( GetOuter(), "No appropriate sequence for arrival activity %s (%d)\n", GetOuter()->GetActivityName( GetPath()->GetArrivalActivity() ), GetPath()->GetArrivalActivity() );
			sequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( ACT_IDLE ) );
		}
		Assert( sequence != ACT_INVALID );
		GetPath()->SetArrivalSequence( sequence );
	}
	return sequence;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalSequence( int sequence )
{
	GetPath()->SetArrivalSequence( sequence );
}

//-----------------------------------------------------------------------------

Activity CAI_Navigator::GetArrivalActivity( ) const
{
	return GetPath()->GetArrivalActivity( );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalDirection( const Vector &goalDirection )
{ 
	GetPath()->SetGoalDirection( goalDirection );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalDirection( const QAngle &goalAngle ) 
{ 
	Vector goalDirection;

	AngleVectors( goalAngle, &goalDirection );

	GetPath()->SetGoalDirection( goalDirection );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalDirection( CBaseEntity * pTarget )
{
	GetPath()->SetGoalDirection( pTarget );
}

//-----------------------------------------------------------------------------

Vector CAI_Navigator::GetArrivalDirection( )
{
	return GetPath()->GetGoalDirection( );
}

//-----------------------------------------------------------------------------

const Vector &CAI_Navigator::GetCurWaypointPos() const
{
	return GetPath()->CurWaypointPos();
}

//-----------------------------------------------------------------------------

int CAI_Navigator::GetCurWaypointFlags() const
{
	return GetPath()->CurWaypointFlags();
}

//-----------------------------------------------------------------------------

GoalType_t CAI_Navigator::GetGoalType() const
{
	return GetPath()->GoalType();
}

//-----------------------------------------------------------------------------

int	CAI_Navigator::GetGoalFlags() const
{
	return GetPath()->GoalFlags();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::CurWaypointIsGoal() const
{
	return GetPath()->CurWaypointIsGoal();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::IsGoalSet() const
{
	return ( GetPath()->GoalType() != GOALTYPE_NONE );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::IsGoalActive() const
{
	return ( GetPath() && !( const_cast<CAI_Path *>(GetPath())->IsEmpty() ) );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::GetPointAlongPath( Vector *pResult, float distance )
{
	if ( !GetPath()->GetCurWaypoint() )
		return false;

	AI_Waypoint_t *pCurWaypoint	 = GetPath()->GetCurWaypoint();
	AI_Waypoint_t *pEndPoint 	 = pCurWaypoint;
	float 		   distRemaining = distance;
	float 		   distToNext;
	Vector		   vPosPrev		 = GetLocalOrigin();

	while ( pEndPoint->GetNext() )
	{
		distToNext = ComputePathDistance( GetNavType(), vPosPrev, pEndPoint->GetPos() );
		
		if ( distToNext > distRemaining)
			break;
		
		distRemaining -= distToNext;
		vPosPrev = pEndPoint->GetPos();
		pEndPoint = pEndPoint->GetNext();
	}
	
	Vector &result = *pResult;
	float distToEnd = ComputePathDistance( GetNavType(), vPosPrev, pEndPoint->GetPos() ); 
	if ( distToEnd - distRemaining < 0.1 )
	{
		result = pEndPoint->GetPos();
	}
	else
	{
		result = pEndPoint->GetPos() - vPosPrev;
		VectorNormalize( result );
		result *= distRemaining;
		result += vPosPrev;
	}

	return true;
}

//-----------------------------------------------------------------------------

AI_PathNode_t CAI_Navigator::GetNearestNode()
{
	COMPILE_TIME_ASSERT( (int)AIN_NO_NODE == NO_NODE );
	return (AI_PathNode_t)( GetNetwork()->NearestNodeToNPCAtPoint( GetOuter(), GetLocalOrigin() ) );
}

//-----------------------------------------------------------------------------

Vector CAI_Navigator::GetNodePos( AI_PathNode_t node )
{
	return GetNetwork()->GetNode((int)node)->GetPosition(GetHullType());
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnScheduleChange()						
{ 
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnClearPath(void) 
{
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnNewGoal()
{
	ResetCalculations();
	m_fNavComplete = true;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnNavComplete()
{
	ResetCalculations();
	TaskMovementComplete();
	m_fNavComplete = true;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnNavFailed()
{
#ifdef DEBUG
	if ( CurWaypointIsGoal() )
	{
		float flWaypointDist = ComputePathDistance( GetNavType(), GetLocalOrigin(), GetPath()->ActualGoalPosition() );
		if ( flWaypointDist < GetGoalTolerance() + 0.1 )
		{
			Msg( "Nav failed but NPC was within goal tolerance?\n" );
		}
	}
#endif

	ResetCalculations();
	m_fNavComplete = true;
	m_bLastNavFailed = true;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnNavFailed( AI_TaskFailureCode_t code )
{
	OnNavFailed();
	SetActivity( GetOuter()->GetStoppedActivity() );
	TaskFail(code);
}
//-----------------------------------------------------------------------------

void CAI_Navigator::OnNavFailed( const char *pszGeneralFailText )	
{ 
	OnNavFailed( MakeFailCode( pszGeneralFailText )	);
}

//-----------------------------------------------------------------------------

void CAI_Navigator::ResetCalculations()
{
	m_hPeerWaitingOn = NULL;
	m_PeerWaitMoveTimer.Force();
	m_PeerWaitClearTimer.Force();

	GetLocalNavigator()->ResetMoveCalculations();
}

//-----------------------------------------------------------------------------
// Purpose: Sets navigation type, maintaining any necessary invariants
//-----------------------------------------------------------------------------
void CAI_Navigator::SetNavType( Navigation_t navType )
{ 
	m_navType = navType;
}


//-----------------------------------------------------------------------------

AIMoveResult_t CAI_Navigator::MoveClimb() 
{
	// --------------------------------------------------
	//  CLIMB START
	// --------------------------------------------------
	
	if ( GetNavType() != NAV_CLIMB )
	{
		SetActivity( ACT_CLIMB_UP );
		GetMotor()->MoveClimbStart();
	}

	SetNavType( NAV_CLIMB );

	AIMoveResult_t result = GetMotor()->MoveClimbExecute( GetPath()->CurWaypointPos(), GetPath()->CurWaypointYaw() );

	if ( result == AIMR_CHANGE_TYPE )
	{
		if ( GetPath()->GetCurWaypoint()->GetNext() )
			AdvancePath();

		if ( !GetPath()->GetCurWaypoint()->GetNext() || !(GetPath()->CurWaypointNavType() == NAV_CLIMB))
		{
			SetActivity( GetMovementActivity() );
			GetMotor()->MoveClimbStop();
			SetNavType(NAV_GROUND);
		}
	}
	return result;
}

//-----------------------------------------------------------------------------

AIMoveResult_t CAI_Navigator::MoveJump()
{
	// --------------------------------------------------
	//  JUMPING
	// --------------------------------------------------
	if ( (GetNavType() != NAV_JUMP) && (GetEntFlags() & FL_ONGROUND) )
	{
		// --------------------------------------------------
		//  Now check if I can actually jump this distance?
		// --------------------------------------------------
		AIMoveTrace_t moveTrace;
		GetMoveProbe()->MoveLimit( NAV_JUMP, GetLocalOrigin(), GetPath()->CurWaypointPos(), 
			MASK_NPCSOLID, GetNavTargetEntity(), &moveTrace );
		if ( IsMoveBlocked( moveTrace ) )
		{
			return moveTrace.fStatus;
		}

		SetNavType(NAV_JUMP);

		GetMotor()->MoveJumpStart( moveTrace.vJumpVelocity );
	}	
	// --------------------------------------------------
	//  LANDING (from jump)
	// --------------------------------------------------
	else if (GetNavType() == NAV_JUMP && (GetEntFlags() & FL_ONGROUND)) 
	{
		// Msg( "jump to %f %f %f landed %f %f %f", GetPath()->CurWaypointPos().x, GetPath()->CurWaypointPos().y, GetPath()->CurWaypointPos().z, GetLocalOrigin().x, GetLocalOrigin().y, GetLocalOrigin().z );

		GetMotor()->MoveJumpStop( );

		SetNavType(NAV_GROUND);

		// --------------------------------------------------
		//  If I've jumped to my goal I'm done
		// --------------------------------------------------
		if (CurWaypointIsGoal())
		{
			OnNavComplete();
			return AIMR_OK;
		}
		// --------------------------------------------------
		//  Otherwise advance my route and walk
		// --------------------------------------------------
		else 
		{
			AdvancePath();
			return AIMR_CHANGE_TYPE;
		}
	}
	// --------------------------------------------------
	//  IN-AIR (from jump)
	// --------------------------------------------------
	else
	{
		GetMotor()->MoveJumpExecute( );
	}

	return AIMR_OK;
}


//-----------------------------------------------------------------------------

void CAI_Navigator::MoveCalcBaseGoal( AILocalMoveGoal_t *pMoveGoal )
{
	pMoveGoal->navType			= GetNavType();
	pMoveGoal->target			= GetPath()->CurWaypointPos();
	pMoveGoal->maxDist			= ComputePathDirection( GetNavType(), GetLocalOrigin(), pMoveGoal->target, &pMoveGoal->dir );
	pMoveGoal->facing			= pMoveGoal->dir;
	pMoveGoal->speed			= GetMotor()->GetIdealSpeed();
	pMoveGoal->curExpectedDist	= pMoveGoal->speed * GetMotor()->GetMoveInterval();
	pMoveGoal->pMoveTarget		= GetNavTargetEntity();

	if ( pMoveGoal->curExpectedDist > pMoveGoal->maxDist )
		pMoveGoal->curExpectedDist = pMoveGoal->maxDist;

	if ( GetPath()->CurWaypointIsGoal())
		pMoveGoal->flags |= AILMG_TARGET_IS_GOAL;
	else
	{
		AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
		if ( pCurWaypoint->GetNext() && pCurWaypoint->GetNext()->NavType() != pCurWaypoint->NavType() )
			pMoveGoal->flags |= AILMG_TARGET_IS_TRANSITION;
	}

	pMoveGoal->pPath = GetPath();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	bool fShouldAttemptHit = false;
	bool fShouldAdvancePath = false;
	float tolerance = 0;

	if ( pMoveGoal->flags & AILMG_TARGET_IS_GOAL )
	{
		fShouldAttemptHit = true;
		tolerance = GetPath()->GetGoalTolerance();
	}
	else if ( !( pMoveGoal->flags & AILMG_TARGET_IS_TRANSITION ) )
	{
		fShouldAttemptHit = true;
		fShouldAdvancePath = true;
		tolerance = GetPath()->GetWaypointTolerance();
	}

	if ( fShouldAttemptHit )
	{
		if ( distClear > pMoveGoal->maxDist )
		{
#ifdef PHYSICS_NPC_SHADOW_DISCREPENCY
			if ( distClear < AI_EPS_CASTS ) // needed because vphysics can pull us back up to this far
			{
				DebugNoteMovementFailure();
				*pResult = pMoveGoal->directTrace.fStatus;
				pMoveGoal->maxDist = 0;
				return true;
			}
#endif
			*pResult = AIMR_OK;
			return true;
		}

#ifdef PHYSICS_NPC_SHADOW_DISCREPENCY
		if ( pMoveGoal->maxDist + AI_EPS_CASTS < tolerance )
#else
		if ( pMoveGoal->maxDist < tolerance )
#endif
		{
			if ( !( pMoveGoal->flags & AILMG_TARGET_IS_GOAL ) ||
				 ( pMoveGoal->directTrace.fStatus != AIMR_BLOCKED_NPC ) ||
				 ( !((CAI_BaseNPC *)pMoveGoal->directTrace.pObstruction)->IsMoving() ) )
			{
				pMoveGoal->maxDist = distClear;
				*pResult = AIMR_OK;

				if ( fShouldAdvancePath )
				{
					AdvancePath();
				}
				else if ( distClear < 0.01 )
				{
					*pResult = pMoveGoal->directTrace.fStatus;
				}
				return true;
			}
		}
	}

	// Allow the NPC to override this behavior. Above logic takes priority
	if ( GetOuter()->OnObstructionPreSteer( pMoveGoal, distClear, pResult ) )
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	// Allow the NPC to override this behavior
	if ( GetOuter()->OnInsufficientStopDist( pMoveGoal, distClear, pResult ))
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

#ifdef PHYSICS_NPC_SHADOW_DISCREPENCY
	if ( distClear < AI_EPS_CASTS ) // needed because vphysics can pull us back up to this far
	{
		DebugNoteMovementFailure();
		*pResult = pMoveGoal->directTrace.fStatus;
		pMoveGoal->maxDist = 0;
		return true;
	}
#endif

	if ( !IsMovingOutOfWay( *pMoveGoal, distClear ) )
	{
		if ( distClear < 1.0 )
		{
			// too close, nothing happening, I'm screwed
			DebugNoteMovementFailure();
			*pResult = pMoveGoal->directTrace.fStatus;
			pMoveGoal->maxDist = 0;
			return true;
		}
		return false;
	}
	
	*pResult = AIMR_OK;
	pMoveGoal->maxDist = distClear;
	pMoveGoal->flags |= AILMG_CONSUME_INTERVAL;
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnFailedSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	// Allow the NPC to override this behavior
	if ( GetOuter()->OnFailedSteer( pMoveGoal, distClear, pResult ))
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

	if ( pMoveGoal->flags & AILMG_TARGET_IS_GOAL )
	{
		if ( distClear >= GetPathDistToGoal() )
		{
			*pResult = AIMR_OK;
			return true;
		}

		if ( distClear > pMoveGoal->maxDist - GetPath()->GetGoalTolerance() )
		{
			Assert( CurWaypointIsGoal() && fabs(pMoveGoal->maxDist - GetPathDistToCurWaypoint()) < 0.01 );

			if ( pMoveGoal->maxDist > distClear )
				pMoveGoal->maxDist = distClear;

			if ( distClear < 0.125 )
				OnNavComplete();

			pMoveGoal->flags |= AILMG_CONSUME_INTERVAL;
			*pResult = AIMR_OK;

			return true;
		}
	}
		
	if ( !(	pMoveGoal->flags & AILMG_TARGET_IS_TRANSITION ) )
	{
		float distToWaypoint = GetPathDistToCurWaypoint();
		float halfHull 		 = GetHullWidth() * 0.5;
		
		if ( distToWaypoint < halfHull )
		{
			if ( distClear > distToWaypoint + halfHull )
			{
				*pResult = AIMR_OK;
				return true;
			}
		}
	}

#if 0
	if ( pMoveGoal->directTrace.pObstruction->MyNPCPointer() &&
		 !GetOuter()->IsUsingSmallHull() && 
		 GetOuter()->SetHullSizeSmall() )
	{
		*pResult = AIMR_CHANGE_TYPE;
		return true;
	}
#endif

	if ( pMoveGoal->directTrace.fStatus == AIMR_BLOCKED_NPC && pMoveGoal->directTrace.vHitNormal != vec3_origin )
	{
		AIMoveTrace_t moveTrace;
		Vector vDeflection;
		CalculateDeflection( GetLocalOrigin(), pMoveGoal->dir, pMoveGoal->directTrace.vHitNormal, &vDeflection );

		if ( pMoveGoal->dir.AsVector2D().Dot( vDeflection.AsVector2D() ) > 0.7 )
		{
			Vector testLoc = GetLocalOrigin() + ( vDeflection * pMoveGoal->curExpectedDist );
			GetMoveProbe()->MoveLimit( GetNavType(), GetLocalOrigin(), testLoc, MASK_NPCSOLID, NULL, &moveTrace );
			if ( moveTrace.fStatus == AIMR_OK )
			{
				pMoveGoal->dir = vDeflection;
				pMoveGoal->maxDist = pMoveGoal->curExpectedDist;
				*pResult = AIMR_OK;
				return true;
			}
		}
	}

	if ( distClear < pMoveGoal->maxDist && !TestingSteering() && PrependLocalAvoidance( distClear ) )
	{
		*pResult = AIMR_CHANGE_TYPE;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnFailedLocalNavigation( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	// Allow the NPC to override this behavior
	if ( GetOuter()->OnFailedLocalNavigation( pMoveGoal, distClear, pResult ))
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

	if ( DelayNavigationFailure( pMoveGoal->directTrace ) )
	{
		*pResult = AIMR_OK;
		pMoveGoal->maxDist = distClear;
		pMoveGoal->flags |= AILMG_CONSUME_INTERVAL;
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::DelayNavigationFailure( const AIMoveTrace_t &trace )
{
	// This code only handles the case of a group of AIs in close proximity, preparing
	// to move mostly as a group, but on slightly different think schedules. Without
	// this patience factor, in the middle or at the rear might fail just because it
	// happened to have its think function called a half cycle before the one
	// in front of it.

	Assert( m_fPeerMoveWait == false ); // expected to be cleared each frame, and never call this function twice
	
	CAI_BaseNPC *pBlocker = trace.pObstruction ? trace.pObstruction->MyNPCPointer() : NULL;
	if ( pBlocker )
	{
		if ( m_hPeerWaitingOn != pBlocker || m_PeerWaitClearTimer.Expired() )
		{
			m_fPeerMoveWait = true;
			m_hPeerWaitingOn = pBlocker;
			m_PeerWaitMoveTimer.Reset();
			m_PeerWaitClearTimer.Reset();
		}
		else if ( m_hPeerWaitingOn == pBlocker && !m_PeerWaitMoveTimer.Expired() )
		{
			m_fPeerMoveWait = true;
		}
	}
	
	return m_fPeerMoveWait;
}

//-----------------------------------------------------------------------------

// @TODO (toml 11-12-02): right now, physics can pull the box back pretty far even though a hull
// trace said we could make the move. Jay is looking into it. For now, if the NPC physics shadow
// is active, allow for a bugger tolerance
extern ConVar npc_vphysics;

bool test_it = false;

bool CAI_Navigator::MoveUpdateWaypoint( AIMoveResult_t *pResult )
{
	// Note that goal & waypoint tolerances are handled in progress blockage cases (e.g., OnObstructionPreSteer)

	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	float 		   waypointDist = ComputePathDistance( GetNavType(), GetLocalOrigin(), pCurWaypoint->GetPos() );
	bool		   bIsGoal		= CurWaypointIsGoal();
	float		   tolerance	= ( npc_vphysics.GetBool() ) ? 0.25 : 0.0625;

	bool fHit = false;

	if ( waypointDist <= tolerance )
	{
		if ( test_it )
		{
			if ( pCurWaypoint->GetNext() && pCurWaypoint->GetNext()->NavType() != pCurWaypoint->NavType() )
			{
				if ( waypointDist < 0.001 )
					fHit = true;
			}
			else
				fHit = true;
		}
		else
			fHit = true;
	}
	
	if ( fHit )
	{
		if ( bIsGoal )
		{
			OnNavComplete();
			*pResult = AIMR_OK;
			
		}
		else
		{
			AdvancePath();
			*pResult = AIMR_CHANGE_TYPE;
		}
		return true;
	}
	
	return false;
}
	 
//-----------------------------------------------------------------------------

bool CAI_Navigator::OnMoveStalled( const AILocalMoveGoal_t &move )
{
	SetActivity( GetOuter()->GetStoppedActivity() );
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnMoveExecuteFailed( const AILocalMoveGoal_t &move, const AIMoveTrace_t &trace, AIMotorMoveResult_t fMotorResult, AIMoveResult_t *pResult )
{
	// Allow the NPC to override this behavior
	if ( GetOuter()->OnMoveExecuteFailed( move, trace, fMotorResult, pResult ))
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

	if ( fMotorResult == AIM_PARTIAL_HIT_TARGET )
	{
		OnNavComplete();
		*pResult = AIMR_OK;
	}
	else if ( fMotorResult == AIM_PARTIAL_HIT_NPC && DelayNavigationFailure( trace ) )
	{
		*pResult = AIMR_OK;
	}
	
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnMoveBlocked( AIMoveResult_t *pResult )
{
	// Allow the NPC to override this behavior
	if ( GetOuter()->OnMoveBlocked( pResult ))
		return true;

	SetActivity( GetOuter()->GetStoppedActivity() );

	const float EPS = 0.1;
	
	float flWaypointDist = ComputePathDistance( GetNavType(), GetLocalOrigin(), GetPath()->ActualGoalPosition() );

	if ( flWaypointDist < GetGoalTolerance() + EPS )
	{
		OnNavComplete();
		*pResult = AIMR_OK;
		return true;
	}

	return false;
}

//-------------------------------------

AIMoveResult_t CAI_Navigator::MoveEnact( const AILocalMoveGoal_t &baseMove )
{
	AIMoveResult_t result = AIMR_ILLEGAL;
	AILocalMoveGoal_t move = baseMove;

	result = GetLocalNavigator()->MoveCalc( &move );
	
	if ( result == AIMR_OK )
	{
		result = GetMotor()->MoveNormalExecute( move );
	}
	else if ( result != AIMR_CHANGE_TYPE )
	{
		GetMotor()->MoveStop();
	}
	
	if ( IsMoveBlocked( result ) )
	{
		OnMoveBlocked( &result );
	}
	
	return result;
}

//-----------------------------------------------------------------------------

bool newcode;

AIMoveResult_t CAI_Navigator::MoveNormal()
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
	
	SetActivity( GetMovementActivity() );

	if ( GetIdealSpeed() <= 0.0f )
	{
		if ( GetActivity() == ACT_TRANSITION )
			return AIMR_OK;
		Msg( "%s moving with speed <= 0 (%s)\n", GetEntClassname(), GetOuter()->GetSequenceName( GetSequence() ) );
	}
			
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

bool CAI_Navigator::PreMove()
{
	Navigation_t goalType = GetPath()->CurWaypointNavType();
	Navigation_t curType  = GetNavType();
	
	if ( goalType == NAV_GROUND && curType != NAV_GROUND )
	{
		Msg( "Warning: %s(%s) appears to have wrong nav type in CAI_Navigator::MoveGround()\n", GetOuter()->GetClassname(), STRING( GetOuter()->GetEntityName() ) );
		switch ( curType )
		{
			case NAV_CLIMB:
			{
				GetMotor()->MoveClimbStop();
				break;
			}
			
			case NAV_JUMP:
			{
				GetMotor()->MoveJumpStop();
				break;
			}
		}

		SetNavType( NAV_GROUND );
	}
	else if ( goalType == NAV_FLY && curType != NAV_FLY )
	{
		AssertMsg( 0, ( "GetNavType() == NAV_FLY" ) );
		return false;
	}

	// --------------------------------
	
	Assert( GetMotor()->GetMoveInterval() > 0 );

	// --------------------------------
	
	SimplifyPath();
	
	m_fPeerMoveWait = false;
	
	return true;
}

//--------------------------------------------------------------------------------------------

bool CAI_Navigator::IsMovingOutOfWay( const AILocalMoveGoal_t &moveGoal, float distClear )
{
	// FIXME: We can make this work for regular entities; although the
	// original code was simply looking @ NPCs. I'm reproducing that code now
	// although I want to make it work for both.
	CAI_BaseNPC *pBlocker = moveGoal.directTrace.pObstruction ? moveGoal.directTrace.pObstruction->MyNPCPointer() : NULL;

	// if it's the world, it ain't moving
	if (!pBlocker)
		return false;

	// if they're doing something, assume it'll work out
	if (pBlocker->IsMoving())
	{
		if ( distClear > moveGoal.curExpectedDist * 0.75 )
			return true;

		Vector2D velBlocker = pBlocker->GetMotor()->GetCurVel().AsVector2D();
		Vector2D velBlockerNorm = velBlocker;

		Vector2DNormalize( velBlockerNorm );

		float dot = moveGoal.dir.AsVector2D().Dot( velBlockerNorm );

		if (dot > -0.25 )
		{
			return true;
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Move towards the next waypoint on my route
// Input  :
// Output :
//-----------------------------------------------------------------------------

enum AINavResult_t
{
	AINR_OK,
	AINR_NO_GOAL,
	AINR_NO_ROUTE,
	AINR_BLOCKED,
	AINR_ILLEGAL
};

void CAI_Navigator::Move( float flInterval ) 
{
	if (flInterval > 1.0)
	{
		// Bound interval so we don't get ludicrous motion when debugging
		// or when framerate drops catostrophically
		flInterval = 1.0;
	}

	if ( !GetOuter()->OverrideMove( flInterval ) )
	{
		// UNDONE: Figure out how much of the timestep was consumed by movement
		// this frame and restart the movement/schedule engine if necessary

		bool fShouldMove = ( GetGoalType() != GOALTYPE_NONE && 
							 !HasMemory( bits_MEMORY_TURNING ) &&
							 ActivityIsLocomotive( GetMovementActivity() ) );
		if ( fShouldMove )
		{
			AINavResult_t result = AINR_OK;
			
			GetMotor()->SetMoveInterval( flInterval );
			
			// ---------------------------------------------------------------------
			// Move should never happen if I don't have a movement goal or route
			// ---------------------------------------------------------------------
			if ( GetPath()->GoalType() == GOALTYPE_NONE )
			{
				Warning( "AIError: Move requested with no route!\n" );
				result = AINR_NO_GOAL;
			}
			else if (!GetPath()->GetCurWaypoint())
				result = AINR_NO_ROUTE;

			if ( result == AINR_OK )
			{
				// ---------------------------------------------------------------------
				// If I've been asked to wait, let's wait
				// ---------------------------------------------------------------------
				if ( GetOuter()->ShouldMoveWait() )
					return;
				
				int nLoopCount = 0;

				AIMoveResult_t moveResult = AIMR_CHANGE_TYPE;
				m_fNavComplete = false;

				while ( moveResult >= AIMR_OK && !m_fNavComplete )
				{
					if ( GetMotor()->GetMoveInterval() <= 0 )
					{
						moveResult = AIMR_OK;
						break;
					}
					
					switch (GetPath()->CurWaypointNavType())
					{
					case NAV_CLIMB:
						moveResult = MoveClimb();
						break;

					case NAV_JUMP:
						moveResult = MoveJump();
						break;

					case NAV_GROUND:
					case NAV_FLY:
						moveResult = MoveNormal();
						break;

					default:
						Msg( "Bogus route move type!");
						moveResult = AIMR_ILLEGAL;
						break;
					}

					++nLoopCount;
					if ( nLoopCount > 16 )
					{
						Msg( "ERROR: AI navigation not terminating. Possibly bad cyclical solving?\n" );
						moveResult = AIMR_ILLEGAL;
						break;
					}

				}

				// --------------------------------------------
				//  Update move status
				// --------------------------------------------
				if ( IsMoveBlocked( moveResult ) )
				{
					if (moveResult != AIMR_BLOCKED_NPC)
						MarkCurWaypointFailedLink();
					OnNavFailed( FAIL_NO_ROUTE );
				}
				return;
			}
			
			static AI_TaskFailureCode_t failures[] =
			{
				NO_TASK_FAILURE,				// AINR_OK (never should happen)
				FAIL_NO_ROUTE_GOAL,				// AINR_NO_GOAL
				FAIL_NO_ROUTE,					// AINR_NO_ROUTE
				FAIL_NO_ROUTE_BLOCKED,			// AINR_BLOCKED
				FAIL_NO_ROUTE_ILLEGAL			// AINR_ILLEGAL
			};
			
			OnNavFailed( failures[result] );
		}
		else 
		{
			// @TODO (toml 10-30-02): the climb part of this is unacceptable, but needed until navigation can handle commencing
			// 						  a navigation while in the middle of a climb

			if ( GetNavType() == NAV_CLIMB )
			{
				GetMotor()->MoveClimbStop();
				SetNavType( NAV_GROUND );
			}
			GetMotor()->MoveStop();
			AssertMsg( TaskIsRunning() || TaskIsComplete(), ("Schedule stalled!!\n") );
		}
	}

}


//-----------------------------------------------------------------------------
// Purpose: Returns yaw speed based on what they're doing. 
//-----------------------------------------------------------------------------
float CAI_Navigator::CalcYawSpeed( void )
{
	// Allow the NPC to override this behavior
	float flNPCYaw = GetOuter()->CalcYawSpeed();
	if (flNPCYaw >= 0.0f)
		return flNPCYaw;

	float maxYaw = MaxYawSpeed();

	//return maxYaw;

	if( IsGoalSet() && GetIdealSpeed() != 0.0)
	{
		// ---------------------------------------------------
		// If not moving to a waypoint use a base turing speed
		// ---------------------------------------------------
		if (!GetPath()->GetCurWaypoint()) 
		{
			return maxYaw;
		}
		// --------------------------------------------------------------
		// If moving towards a waypoint, set the turn speed based on the 
		// distance of the waypoint and my forward velocity
		// --------------------------------------------------------------
		if (GetIdealSpeed() > 0) 
		{
			// -----------------------------------------------------------------
			// Get the projection of npc's heading direction on the waypoint dir
			// -----------------------------------------------------------------
			float waypointDist = (GetPath()->CurWaypointPos() - GetLocalOrigin()).Length();

			// If waypoint is close, aim for the waypoint
			if (waypointDist < 100)
			{
				float scale = 1 + (0.01*(100 - waypointDist));
				return (maxYaw * scale);
			}
		}
	}
	return maxYaw;
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to advance the route to the next waypoint, triangulating
//			around entities that are in the way
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Navigator::AdvancePath()
{
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();

	if ( pCurWaypoint->Flags() & bits_WP_TO_PATHCORNER )
	{
		CBaseEntity *pEntity = pCurWaypoint->hPathCorner;
		if ( pEntity )
		{
			variant_t emptyVariant;
			pEntity->AcceptInput( "InPass", GetOuter(), pEntity, emptyVariant, 0 );
		}
	}

	if ( GetPath()->CurWaypointIsGoal() )
		return;

	if ( pCurWaypoint->Flags() & bits_WP_TO_DOOR )
	{
		// dvs: FIXME: all the door opening code should be in one place, either CAI_BaseNPC, CAI_Navigator, or CBasePropDoor
		CBasePropDoor *pDoor = (CBasePropDoor *)(CBaseEntity *)pCurWaypoint->GetEHandleData();
		if (pDoor != NULL)
		{
			pDoor->NPCOpenDoor(GetOuter());

			// Wait for the door to finish opening before trying to move through the doorway.
			GetOuter()->m_flMoveWaitFinished = gpGlobals->curtime + pDoor->GetOpenInterval();
		}
		else
		{
			DevMsg("%s trying to open a door that has been deleted!\n", GetOuter()->GetDebugName());
		}
	}

	// If we've just passed a path_corner, advance m_pGoalEnt
	if ( pCurWaypoint->GetNext() && ( pCurWaypoint->Flags() & bits_WP_TO_PATHCORNER ) )
	{
		// BUGBUG: This doesn't actually work!!!
		SetGoalEnt( GetGoalEnt()->GetNextTarget() );
	}
	GetPath()->Advance();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

#ifdef DEBUG
bool g_fAIDisableSimplifyPath = false;
#define IsSimplifyPathDisabled() g_fAIDisableSimplifyPath
#else
#define IsSimplifyPathDisabled() false
#endif

const float MIN_ANGLE_COS_SIMPLIFY = 0.766; // 40 deg left or right

bool CAI_Navigator::ShouldAttemptSimplifyTo( const Vector &pos )
{
	if ( m_fForcedSimplify )
		return true;
		
	Vector vecToPos = ( pos - GetLocalOrigin() );
	vecToPos.z = 0;
	VectorNormalize( vecToPos );
	
	float dot = GetOuter()->BodyDirection2D().AsVector2D().Dot( vecToPos.AsVector2D() );
	
	return ( m_fForcedSimplify || dot > MIN_ANGLE_COS_SIMPLIFY );
}

//-------------------------------------

bool CAI_Navigator::ShouldSimplifyTo( const Vector &pos )
{
	trace_t tr;
	AI_TraceLine(GetOuter()->EyePosition(), pos, MASK_NPCSOLID, GetOuter(), COLLISION_GROUP_NONE, &tr);
	
	if ( tr.fraction == 1.0 )
	{
		AIMoveTrace_t moveTrace;
		GetMoveProbe()->MoveLimit( GetNavType(), 
			GetLocalOrigin(), pos, MASK_NPCSOLID, 
			GetPath()->GetTarget(), &moveTrace );

		return !IsMoveBlocked(moveTrace);
	}
	return false;
}

//-------------------------------------

void CAI_Navigator::SimplifyPathInsertSimplification( AI_Waypoint_t *pSegmentStart, const Vector &point )
{
	if ( point != pSegmentStart->GetPos() )
	{
		AI_Waypoint_t *pNextWaypoint = pSegmentStart->GetNext();
		Assert( pNextWaypoint );
		Assert( pSegmentStart->NavType() == pNextWaypoint->NavType() );

		AI_Waypoint_t *pNewWaypoint = new AI_Waypoint_t( point, 0, pSegmentStart->NavType(), 0, NO_NODE );

		while ( GetPath()->GetCurWaypoint() != pNextWaypoint )
		{
			GetPath()->Advance();
		}
		pNewWaypoint->SetNext( pNextWaypoint );
		GetPath()->SetWaypoints( pNewWaypoint );
	}
	else
	{
		while ( GetPath()->GetCurWaypoint() != pSegmentStart )
		{
			GetPath()->Advance();
		}
	}

}

//-------------------------------------

// @TODO (toml 04-25-03): Optimize after E3
const float ROUTE_SIMPLIFY_TIME_DELAY 		= 0.5;

bool CAI_Navigator::SimplifyPathForwardScan( const CAI_Navigator::SimplifyForwardScanParams &params, 
											 AI_Waypoint_t *pCurWaypoint, const Vector &curPoint, float distRemaining, bool skipTest, int *pTestCount )
{
	if ( *pTestCount >= params.maxSamples )
		return false;

	AI_Waypoint_t *pNextWaypoint = pCurWaypoint->GetNext();

	if ( distRemaining > 0)
	{
		if ( pCurWaypoint->IsReducible() )
		{
			// Walk out to test point, or next waypoint
			AI_Waypoint_t *pRecursionWaypoint;
			Vector nextPoint;
			
			float distToNext = ComputePathDistance( GetNavType(), curPoint, pNextWaypoint->GetPos() );
			if (distToNext < params.increment * 1.10 )
			{
				nextPoint = pNextWaypoint->GetPos();
				distRemaining -= distToNext;
				pRecursionWaypoint = pNextWaypoint;
			}
			else
			{
				Vector offset = pNextWaypoint->GetPos() - pCurWaypoint->GetPos();
				VectorNormalize( offset );
				offset *= params.increment;
				nextPoint = curPoint + offset;
				distRemaining -= params.increment;
				pRecursionWaypoint = pCurWaypoint;
			}
			
			bool skipTestNext = ( ComputePathDistance( GetNavType(), GetLocalOrigin(), nextPoint ) > params.radius + 0.1 );

			if ( SimplifyPathForwardScan( params, pRecursionWaypoint, nextPoint, distRemaining, skipTestNext, pTestCount ) )
				return true;
		}
	}
	
	if ( !skipTest && ShouldAttemptSimplifyTo( curPoint ) )
	{
		(*pTestCount)++;
		if ( ShouldSimplifyTo( curPoint ) )
		{
			SimplifyPathInsertSimplification( pCurWaypoint, curPoint );
			return true;
		}
	}
	
	return false;
}

//-------------------------------------

bool CAI_Navigator::SimplifyPathForwardScan( const CAI_Navigator::SimplifyForwardScanParams &params )
{
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	float distRemaining = params.scanDist - GetPathDistToCurWaypoint();
	int testCount = 0;

	if ( distRemaining < 0.1 )
		return false;
	
	if ( SimplifyPathForwardScan( params, pCurWaypoint, pCurWaypoint->GetPos(), distRemaining, true, &testCount ) )
		return true;
		
	return false;
}

//-------------------------------------

bool CAI_Navigator::SimplifyPathForward()
{
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	AI_Waypoint_t *pNextWaypoint = pCurWaypoint->GetNext();

	if ( !pNextWaypoint )
		return false;
	
	static SimplifyForwardScanParams fullScanParams = 
	{
		32 * 12, 	// Distance to move out path
		12 * 12, 	// Radius within which a point must be to be valid
		3 * 12, 	// Increment to move out on
		4, 			// maximum number of point samples
	};
	
	if ( SimplifyPathForwardScan( fullScanParams ) )
		return true;

	if ( ShouldAttemptSimplifyTo( pNextWaypoint->GetPos() ) && 
		 ComputePathDistance( GetNavType(), GetLocalOrigin(), pNextWaypoint->GetPos() ) < fullScanParams.scanDist && 
		 ShouldSimplifyTo( pNextWaypoint->GetPos() ) ) // @TODO (toml 04-25-03): need to handle this better. this is here because forward scan may look out so far that a close obvious solution is skipped (due to test limiting)
	{
		delete pCurWaypoint;
		GetPath()->SetWaypoints(pNextWaypoint);
		return true;
	}
	
	return false;
}

//-------------------------------------

bool CAI_Navigator::SimplifyPathBacktrack()
{
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	AI_Waypoint_t *pNextWaypoint = pCurWaypoint->GetNext();
	
	// ------------------------------------------------------------------------
	// If both waypoints are ground waypoints and my path sends me back tracking 
	// more than 24 inches, try to aim for (roughly) the nearest point on the line 
	// connecting the first two waypoints
	// ------------------------------------------------------------------------
	if (pCurWaypoint->GetNext() &&
		(pNextWaypoint->Flags() & bits_WP_TO_NODE) &&
		(pNextWaypoint->NavType() == NAV_GROUND) &&
		(pCurWaypoint->NavType() == NAV_GROUND)	&&
		(pCurWaypoint->Flags() & bits_WP_TO_NODE) )	
	{

		Vector firstToMe	= (GetLocalOrigin()			   - pCurWaypoint->GetPos());
		Vector firstToNext	= pNextWaypoint->GetPos() - pCurWaypoint->GetPos();
		VectorNormalize(firstToNext);
		firstToMe.z			= 0;
		firstToNext.z		= 0;
		float firstDist	= firstToMe.Length();
		float firstProj	= DotProduct(firstToMe,firstToNext);
		float goalTolerance = GetPath()->GetGoalTolerance();
		if (firstProj>0.5*firstDist) 
		{
			Vector targetPos = pCurWaypoint->GetPos() + (firstProj * firstToNext);

			// Only use a local or jump move
			int buildFlags = 0;
			if (CapabilitiesGet() & bits_CAP_MOVE_GROUND)
				buildFlags |= bits_BUILD_GROUND; 
			if (CapabilitiesGet() & bits_CAP_MOVE_JUMP)
				buildFlags |= bits_BUILD_JUMP;

			// Make sure I can get to the new point
			AI_Waypoint_t *route1 = GetPathfinder()->BuildLocalRoute(GetLocalOrigin(), targetPos, GetPath()->GetTarget(), bits_WP_TO_DETOUR, NO_NODE, buildFlags, goalTolerance);
			if (!route1) 
				return false;

			// Make sure the new point gets me to the target location
			AI_Waypoint_t *route2 = GetPathfinder()->BuildLocalRoute(targetPos, pNextWaypoint->GetPos(), GetPath()->GetTarget(), bits_WP_TO_DETOUR, NO_NODE, buildFlags, goalTolerance);
			if (!route2) 
			{
				DeleteAll(route1);
				return false;
			}

			// Add the two route together
			AddWaypointLists(route1,route2);

			// Now add the rest of the old route to the new one
			AddWaypointLists(route1,pNextWaypoint->GetNext());

			// Now advance the route linked list, putting the finished waypoints back in the waypoint pool
			AI_Waypoint_t *freeMe = pCurWaypoint->GetNext();
			delete pCurWaypoint;
			delete freeMe;

			GetPath()->SetWaypoints(route1);
			return true;
		}
	}
	return false;
}			

//-------------------------------------

bool CAI_Navigator::SimplifyPathQuick()
{

	static SimplifyForwardScanParams quickScanParams = 
	{
		(12.0 * 12.0) - 0.1,	// Distance to move out path
		12 * 12, 				// Radius within which a point must be to be valid
		0.5 * 12, 				// Increment to move out on
		1, 						// maximum number of point samples
	};

	if ( SimplifyPathForwardScan( quickScanParams ) )
		return true;

	return false;
}

//-------------------------------------

bool CAI_Navigator::SimplifyPath( bool fForce )
{
	if ( TestingSteering() || IsSimplifyPathDisabled() )
		return false;

	m_fForcedSimplify = fForce;

	Navigation_t navType = GetOuter()->GetNavType();
	if (navType == NAV_GROUND || navType == NAV_FLY)
	{
		AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
		if ( !pCurWaypoint || !pCurWaypoint->IsReducible() )
			return false;

		if ( m_fForcedSimplify || m_flNextSimplifyTime <= gpGlobals->curtime)
		{
			m_flNextSimplifyTime = gpGlobals->curtime + ROUTE_SIMPLIFY_TIME_DELAY;

			if ( SimplifyPathForward() )
				return true;
			
			if ( SimplifyPathBacktrack() )
				return true;
		}
	
		if ( SimplifyPathQuick() )
			return true;
	}

	return false;
}
//-----------------------------------------------------------------------------

AI_NavPathProgress_t CAI_Navigator::ProgressFlyPath( const AI_ProgressFlyPathParams_t &params )
{
	AI_NavPathProgress_t result = AINPP_NO_CHANGE;
	
	if ( IsGoalActive() )
	{
		float waypointDist = ( GetCurWaypointPos() - GetLocalOrigin() ).Length();

		if ( CurWaypointIsGoal() )
		{
			float tolerance = max( params.goalTolerance, GetPath()->GetGoalTolerance() );
			if ( waypointDist <= tolerance )
				result = AINPP_COMPLETE;
		}
		else
		{
			if ( waypointDist <= params.waypointTolerance )
			{
				AdvancePath();	
				result = AINPP_ADVANCED;
			}

			if ( SimplifyFlyPath( params ) )
				result = AINPP_ADVANCED;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SimplifyFlyPath( unsigned collisionMask, const CBaseEntity *pTarget, 
									 float strictPointTolerance, float blockTolerance,
									 AI_NpcBlockHandling_t blockHandling)
{
	AI_ProgressFlyPathParams_t params( collisionMask, strictPointTolerance, blockTolerance,
									   0, 0, blockHandling );
	params.SetCurrent( pTarget );
	SimplifyFlyPath( params );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SimplifyFlyPath(  const AI_ProgressFlyPathParams_t &params )
{
	if ( !GetPath()->GetCurWaypoint() )
		return false;

	// don't shorten path_corners
	bool result = false;
	bool bIsStrictWaypoint = ( !params.bTrySimplify || ( (GetPath()->CurWaypointFlags() & (bits_WP_TO_PATHCORNER|bits_WP_DONT_SIMPLIFY) ) != 0 ) );

	Vector dir = GetCurWaypointPos() - GetLocalOrigin();
	float length = VectorNormalize( dir );
	
	if ( !bIsStrictWaypoint || length < params.strictPointTolerance )
	{
		// FIXME: This seems strange... Why should this condition ever be met?
		// Don't advance your waypoint if you don't have one!
		if (GetPath()->CurWaypointIsGoal())
			return false;

		AIMoveTrace_t moveTrace;
		GetMoveProbe()->MoveLimit( NAV_FLY, GetLocalOrigin(), GetPath()->NextWaypointPos(),
			params.collisionMask, params.pTarget, &moveTrace);
		
		if ( moveTrace.flDistObstructed - params.blockTolerance < 0.01 || 
		     ( ( params.blockHandling == AISF_IGNORE) && ( moveTrace.fStatus == AIMR_BLOCKED_NPC ) ) )
		{
			AdvancePath();
			result = true;
		}
		else if ( moveTrace.pObstruction && params.blockHandling == AISF_AVOID )
			PrependLocalAvoidance( moveTrace.flDistObstructed );
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CAI_Navigator::MovementCost( int moveType, Vector &vecStart, Vector &vecEnd )
{
	float cost;
	
	cost = (vecStart - vecEnd).Length();

	if ( moveType == bits_CAP_MOVE_JUMP || moveType == bits_CAP_MOVE_CLIMB )
	{
		cost *= 2.0;
	}

	// Allow the NPC to override the movement cost
	GetOuter()->MovementCost( moveType, vecStart, vecEnd, &cost );
	
	return cost;
}

//-----------------------------------------------------------------------------
// Purpose: Will the entities hull fit at the given node
// Input  :
// Output : Returns true if hull fits at node
//-----------------------------------------------------------------------------
bool CAI_Navigator::CanFitAtNode(int nodeNum, unsigned int collisionMask ) 
{
	// Make sure I have a network
	if (!GetNetwork())
	{
		Msg("CanFitAtNode() called with no network!\n");
		return false;
	}

	CAI_Node* pTestNode = GetNetwork()->GetNode(nodeNum);
	Vector startPos		= pTestNode->GetPosition(GetHullType());

	// -------------------------------------------------------------------
	// Check ground nodes for standable bottom
	// -------------------------------------------------------------------
	if (pTestNode->GetType() == NODE_GROUND)
	{
		if (!GetMoveProbe()->CheckStandPosition( startPos, collisionMask ))
		{
			return false;
		}
	}


	// -------------------------------------------------------------------
	// Check climb exit nodes for standable bottom
	// -------------------------------------------------------------------
	if ((pTestNode->GetType() == NODE_CLIMB) &&
		(pTestNode->m_eNodeInfo & (bits_NODE_CLIMB_BOTTOM | bits_NODE_CLIB_EXIT )))
	{
		if (!GetMoveProbe()->CheckStandPosition( startPos, collisionMask ))
		{
			return false;
		}
	}


	// -------------------------------------------------------------------
	// Check that hull fits at position of node
	// -------------------------------------------------------------------
	if (!CanFitAtPosition( startPos, collisionMask ))
	{
		startPos.z += GetOuter()->StepHeight();
		if (!CanFitAtPosition( startPos, collisionMask ))
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if NPC's hull fits in the given spot with the
//			given collision mask
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Navigator::CanFitAtPosition( const Vector &vStartPos, unsigned int collisionMask )
{
	Vector vEndPos	= vStartPos;
	vEndPos.z += 0.01;
	trace_t tr;
	AI_TraceHull( vStartPos, vEndPos, GetHullMins(), GetHullMaxs(), collisionMask, GetOuter(), COLLISION_GROUP_NONE, &tr );
	if (tr.startsolid)
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------

float CAI_Navigator::GetPathDistToCurWaypoint() const
{
	return ComputePathDistance( GetNavType(), GetLocalOrigin(), GetPath()->CurWaypointPos() );
}

//-----------------------------------------------------------------------------

float CAI_Navigator::GetPathDistToGoal() const
{
	return ( GetPathDistToCurWaypoint() + GetPath()->GetCurWaypoint()->flPathDistGoal );
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a route
// Input  :
// Output : True if successful / false if fail
//-----------------------------------------------------------------------------
bool CAI_Navigator::FindPath( bool fSignalTaskStatus )
{
	GetPath()->ClearWaypoints();

	// -----------------------------------------------------------------
	//  If I've failed making the path before 
	// -----------------------------------------------------------------
	if (HasMemory(bits_MEMORY_PATH_FAILED))
	{
		// If I've passed by fail time, fail this task
		if (m_timePathRebuildFail < gpGlobals->curtime)
		{
			if ( fSignalTaskStatus )
				OnNavFailed( FAIL_NO_ROUTE );
			else
				OnNavFailed();
			return false;
		}
		// If its time to try again do so
		else if (m_timePathRebuildNext < gpGlobals->curtime)
		{
			// If I suceeded I'm done
			if (DoFindPath())
			{	
				Forget(bits_MEMORY_PATH_FAILED);

				if (fSignalTaskStatus && GetCurTask()->iTask != TASK_WAIT_FOR_MOVEMENT) 
				{
					TaskComplete();
				}
				return true;
			}
			else
			{	// Otherwise schedule a time to try again
				m_timePathRebuildNext = gpGlobals->curtime + m_timePathRebuildDelay;
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	// -----------------------------------------------------------------
	//  If this is my first attempt making the path 
	// -----------------------------------------------------------------
	else
	{
		if ( DoFindPath() )
		{	// If I suceeded I'm done
			Forget(bits_MEMORY_PATH_FAILED);

			if ( fSignalTaskStatus && !GetOuter()->IsCurTaskContinuousMove() )
			{
				TaskComplete();
			}
		}
		// If I'm asked to fail right away
		else if (m_timePathRebuildMax == 0)
		{
			if ( fSignalTaskStatus )
				OnNavFailed( FAIL_NO_ROUTE );
			else
				OnNavFailed();
			return false;
		}
		// Otherwise remember that I failed and set fail timer
		else
		{	// Remember the failure and schedule a time to try again
			Remember(bits_MEMORY_PATH_FAILED);
			m_timePathRebuildNext = gpGlobals->curtime + m_timePathRebuildDelay;
			m_timePathRebuildFail = gpGlobals->curtime + m_timePathRebuildMax;
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when route fails.  Marks last link on which that failure
//			occured as stale so when then next node route is build that link
//			will be explictly checked for blockages
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CAI_Navigator::MarkCurWaypointFailedLink( void )
{
	if ( TestingSteering() )
		return;

	if ( !m_fRememberStaleNodes )
		return;

	// Prevent a crash in release
	if( !GetPath() || !GetPath()->GetCurWaypoint() )
		return;

	int startID =	GetPath()->GetLastNodeReached();
	int endID	=	GetPath()->GetCurWaypoint()->iNodeID;

	if ((startID == NO_NODE) || (endID == NO_NODE))
	{
		return;
	}

	// Find link and mark it as stale
	CAI_Node *pNode = GetNetwork()->GetNode(startID);
	CAI_Link *pLink = pNode->GetLink( endID );
	if ( pLink )
	{
		pLink->m_LinkInfo |= bits_LINK_STALE_SUGGESTED;
		pLink->m_timeStaleExpires = gpGlobals->curtime + 4.0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Builds a route to the given vecGoal using either local movement
//			or nodes
// Input  :
// Output : True is route was found, false otherwaise
//-----------------------------------------------------------------------------
bool CAI_Navigator::DoFindPathToPos(void)
{
	CAI_Path *		pPath 			= GetPath();
	CAI_Pathfinder *pPathfinder 	= GetPathfinder();
	const Vector &	actualGoalPos 	= pPath->ActualGoalPosition();
	CBaseEntity *	pTarget 		= pPath->GetTarget();
	float 			tolerance 		= pPath->GetGoalTolerance();
	Vector			origin;
	
	if ( gpGlobals->curtime - m_flTimeClipped > 0.11  || m_bLastNavFailed )
		m_pClippedWaypoints->RemoveAll();

	if ( m_pClippedWaypoints->IsEmpty() )
		origin = GetLocalOrigin();
	else
	{
		AI_Waypoint_t *pLastClipped = m_pClippedWaypoints->GetLast();
		origin = pLastClipped->GetPos();
	}

	m_bLastNavFailed = false;

	pPath->ClearWaypoints();

	int buildFlags = 0;
	bool bTryLocal = ShouldUseLocalPaths();

	// Set up build flags
	if (GetNavType() == NAV_CLIMB)
	{
		 // if I'm climbing, then only allow routes that are also climb routes
		buildFlags = bits_BUILD_CLIMB;
		bTryLocal = false;
	}
	else if ( (CapabilitiesGet() & bits_CAP_MOVE_FLY) || (CapabilitiesGet() & bits_CAP_MOVE_SWIM) )
	{
		buildFlags = (bits_BUILD_FLY | bits_BUILD_GIVEWAY | bits_BUILD_TRIANG);
	}
	else if (CapabilitiesGet() & bits_CAP_MOVE_GROUND)
	{
		buildFlags = (bits_BUILD_GROUND | bits_BUILD_GIVEWAY | bits_BUILD_TRIANG);
		if ( CapabilitiesGet() & bits_CAP_MOVE_JUMP )
		{
			buildFlags |= bits_BUILD_JUMP;
		}
	}

	AI_Waypoint_t *pFirstWaypoint = NULL;

	//  First try a local route 
	if ( bTryLocal )
	{
		pFirstWaypoint = pPathfinder->BuildLocalRoute(origin, actualGoalPos, pTarget, 
													 bits_WP_TO_GOAL, NO_NODE, 
													 buildFlags, tolerance);
	}

	//  If the fails, try a node route
	if ( !pFirstWaypoint )
	{
		pFirstWaypoint = pPathfinder->BuildNodeRoute( origin, actualGoalPos, tolerance );
	}
	
	if (!pFirstWaypoint)
	{
		//  Sorry no path
		return false;
	}
	
	pPath->SetWaypoints( pFirstWaypoint );
	if ( !m_pClippedWaypoints->IsEmpty() )
	{
		AI_Waypoint_t *pFirstClipped = m_pClippedWaypoints->GetFirst();
		m_pClippedWaypoints->Set( NULL );
		pFirstClipped->ModifyFlags( bits_WP_DONT_SIMPLIFY, true );
		pPath->PrependWaypoints( pFirstClipped );
	}

	if ( pFirstWaypoint->GetNext() && pFirstWaypoint->GetNext()->NavType() == GetNavType() )
	{
		// If we're seemingly beyond the waypoint, and our hull is over the line, move on
		const float EPS = 0.1;
		Vector vClosest;
		CalcClosestPointOnLineSegment( origin, 
									   pFirstWaypoint->GetPos(), pFirstWaypoint->GetNext()->GetPos(), 
									   vClosest );
		if ( ( pFirstWaypoint->GetPos() - vClosest ).Length() > EPS &&
			 ( origin - vClosest ).Length() < GetHullWidth() * 0.5 )
		{
			pPath->Advance();
		}
	}
	
	return true;
}



// UNDONE: This is broken
// UNDONE: Remove this and change the pathing code to be able to refresh itself and support
// circular paths, etc.
#define MAX_PATHCORNER_NODES	128
//-----------------------------------------------------------------------------
// Purpose: Attemps to find a route
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Navigator::DoFindPath( void )
{
	AI_PROFILE_SCOPE(CAI_Navigator_DoFindPath);

	GetPath()->ClearWaypoints();

	bool		returnCode;

	returnCode = false;

	switch( GetPath()->GoalType() )
	{
	case GOALTYPE_PATHCORNER:
		{
			// NPC is on a path_corner loop
			CBaseEntity	*pPathCorner = GetGoalEnt();
			if ( pPathCorner != NULL )
			{
				// Find path to first pathcorner
				Vector initPos = pPathCorner->GetLocalOrigin();

				float tolerance = GetPath()->GetGoalTolerance();
				TranslateEnemyChasePosition( pPathCorner, initPos, tolerance );

				GetPath()->ResetGoalPosition( initPos );
				GetPath()->SetGoalTolerance( tolerance );

				if ( ( returnCode = DoFindPathToPos() ) != false )
				{
					// HACKHACK: If the starting path_corner has a speed, copy that to the entity
					if ( pPathCorner->m_flSpeed != 0 )
					{
						SetSpeed( pPathCorner->m_flSpeed );
					}

					AI_Waypoint_t *lastWaypoint	= GetPath()->GetGoalWaypoint();
					Assert(lastWaypoint);

					lastWaypoint->ModifyFlags( bits_WP_TO_PATHCORNER, true );
					lastWaypoint->hPathCorner = pPathCorner;

					int count;
					pPathCorner = pPathCorner->GetNextTarget(); // first already accounted for in pathfind
					if ( pPathCorner )
					{
						lastWaypoint->ModifyFlags( bits_WP_TO_GOAL, false );
					}

					for ( count = 0; count < MAX_PATHCORNER_NODES - 1 && pPathCorner; count++, pPathCorner = pPathCorner->GetNextTarget() )
					{
						// BRJ 10/4/02
						// FIXME: I'm not certain about the navtype here
						AI_Waypoint_t *curWaypoint = new AI_Waypoint_t( pPathCorner->GetLocalOrigin(), 0, GetNavType(), bits_WP_TO_PATHCORNER, NO_NODE );
						Vector waypointPos = curWaypoint->GetPos();
						TranslateEnemyChasePosition( pPathCorner, waypointPos, tolerance );
						curWaypoint->SetPos( waypointPos );
						GetPath()->SetGoalTolerance( tolerance );
						curWaypoint->hPathCorner = pPathCorner;

						if ( !lastWaypoint )
						{
							// first node, hook it up to the route
							GetPath()->SetWaypoints( curWaypoint );
						}
						else
						{
							lastWaypoint->SetNext(curWaypoint);
						}

						lastWaypoint = curWaypoint;
					}
					// UNDONE: Probably can reorg this loop to fix this
					if ( lastWaypoint )
					{
						lastWaypoint->SetNext(NULL);
						if ( count < MAX_PATHCORNER_NODES )
						{
							lastWaypoint->ModifyFlags( bits_WP_TO_GOAL, true );
							GetPath()->SetGoalPosition( lastWaypoint->GetPos() );
						}
					}
				}
			}
			else
			{
				// The NPC has reached the end of the path.
				returnCode = false;
			}
		}
		break;

	case GOALTYPE_ENEMY:
		{
			// NOTE: This is going to set the goal position, which was *not*
			// set in SetGoal for this movement type
			CBaseEntity *pEnemy = GetPath()->GetTarget();
			if (pEnemy)
			{
				Assert( pEnemy == GetEnemy() );

				Vector newPos = GetEnemyLKP();

				float tolerance = GetPath()->GetGoalTolerance();
				TranslateEnemyChasePosition( pEnemy, newPos, tolerance );

				// NOTE: Calling reset here because this can get called
				// any time we have to update our goal position
				GetPath()->ResetGoalPosition( newPos );
				GetPath()->SetGoalTolerance( tolerance );

				returnCode = DoFindPathToPos();
			}
		}
		break;

	case GOALTYPE_LOCATION:
	case GOALTYPE_FLANK:
	case GOALTYPE_COVER:
		returnCode = DoFindPathToPos();
		break;

	case GOALTYPE_TARGETENT:
		{
			// NOTE: This is going to set the goal position, which was *not*
			// set in SetGoal for this movement type
			CBaseEntity *pTarget = GetPath()->GetTarget();
			if (pTarget)
			{
				Assert( pTarget == GetTarget() );

				// NOTE: Calling reset here because this can get called
				// any time we have to update our goal position
				GetPath()->ResetGoalPosition( pTarget->GetAbsOrigin() );
				returnCode = DoFindPathToPos();
			}
		}
		break;
	}

	return returnCode;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::ClearPath( void )
{
	m_timePathRebuildMax	= 0;					// How long to try rebuilding path before failing task
	m_timePathRebuildFail	= 0;					// Current global time when should fail building path
	m_timePathRebuildNext	= 0;					// Global time to try rebuilding again
	m_timePathRebuildDelay	= 0;					// How long to wait before trying to rebuild again

	Forget( bits_MEMORY_PATH_FAILED );

	SaveStoppingPath();

	GetPath()->Clear();
}

//-----------------------------------------------------------------------------

ConVar ai_use_clipped_paths( "ai_use_clipped_paths", "0" );

void CAI_Navigator::SaveStoppingPath( void )
{
	m_pClippedWaypoints->RemoveAll();
	m_flTimeClipped = -1;

	float distRemaining = GetMotor()->MinStoppingDist();
	if ( ai_use_clipped_paths.GetBool() && distRemaining > 0.1 && GetPath()->GetCurWaypoint() )
	{
		AI_Waypoint_t *pSavedWaypoints = NULL;
		AI_Waypoint_t *pLastSavedWaypoint = NULL;
		AI_Waypoint_t *pCurWaypoint	 = GetPath()->GetCurWaypoint();
		AI_Waypoint_t *pEndPoint 	 = pCurWaypoint;
		float 		   distToNext;
		Vector		   vPosPrev		 = GetLocalOrigin();
		AI_Waypoint_t *pNewWaypoint;

		while ( pEndPoint->GetNext() )
		{
			distToNext = ComputePathDistance( pEndPoint->NavType(), vPosPrev, pEndPoint->GetPos() );
			
			if ( distToNext > distRemaining)
				break;
				
			pNewWaypoint = new AI_Waypoint_t(*pEndPoint);
			
			if ( !pSavedWaypoints )
				pSavedWaypoints = pNewWaypoint;
				
			if ( pLastSavedWaypoint )
				pLastSavedWaypoint->SetNext( pNewWaypoint );
			pLastSavedWaypoint = pNewWaypoint;
			
			distRemaining -= distToNext;
			vPosPrev = pEndPoint->GetPos();
			pEndPoint = pEndPoint->GetNext();
		}
		
		float distToEnd = ComputePathDistance( pEndPoint->NavType(), vPosPrev, pEndPoint->GetPos() ); 
		if ( distToEnd - distRemaining < 0.1 )
		{
			pNewWaypoint = new AI_Waypoint_t(*pEndPoint);
			if ( !pSavedWaypoints )
				pSavedWaypoints = pNewWaypoint;
			if ( pLastSavedWaypoint )
				pLastSavedWaypoint->SetNext( pNewWaypoint );
		}
		else
		{
			Vector remainder;
			// Append here
			remainder = pEndPoint->GetPos() - vPosPrev;
			VectorNormalize( remainder );
			float yaw = UTIL_VecToYaw( remainder );
			remainder *= distRemaining;
			remainder += vPosPrev;

			pNewWaypoint = new AI_Waypoint_t( remainder, yaw, pEndPoint->NavType(), bits_WP_TO_GOAL, 0);
			if ( !pSavedWaypoints )
				pSavedWaypoints = pNewWaypoint;
			if ( pLastSavedWaypoint )
				pLastSavedWaypoint->SetNext( pNewWaypoint );
		}
		if ( pSavedWaypoints )
		{
			m_flTimeClipped = gpGlobals->curtime;
			m_pClippedWaypoints->Set( pSavedWaypoints );
		}
	}

}
//-----------------------------------------------------------------------------

static Vector GetRouteColor(Navigation_t navType, int waypointFlags)
{
	if (navType == NAV_JUMP)
	{
		return Vector(255,0,0);
	}

	if (waypointFlags & bits_WP_TO_GOAL)
	{
		return Vector(200,0,255);
	}
	
	if (waypointFlags & bits_WP_TO_DETOUR)
	{
		return Vector(0,200,255);
	}
	
	if (waypointFlags & bits_WP_TO_NODE)
	{
		return Vector(0,0,255);
	}

	else //if (waypointBits & bits_WP_TO_PATHCORNER)
	{
		return Vector(0,255,150);
	}
}


//-----------------------------------------------------------------------------
// Returns a color for debugging purposes
//-----------------------------------------------------------------------------
static Vector GetWaypointColor(Navigation_t navType)
{
	switch( navType )
	{
	case NAV_JUMP:
		return Vector(255,90,90);

	case NAV_GROUND:
		return Vector(0,255,255);

	case NAV_CLIMB:
		return Vector(90,255,255);

	case NAV_FLY:
		return Vector(90,90,255);

	default:
		return Vector(255,0,0);
	}
}

//-----------------------------------------------------------------------------

void CAI_Navigator::DrawDebugRouteOverlay(void)
{
	AI_Waypoint_t *waypoint = GetPath()->GetCurWaypoint();

	if (waypoint)
	{
		Vector RGB = GetRouteColor(waypoint->NavType(), waypoint->Flags());
		NDebugOverlay::Line(GetLocalOrigin(), waypoint->GetPos(), RGB[0],RGB[1],RGB[2], true,0);
	}

	while (waypoint) 
	{
		Vector RGB = GetWaypointColor(waypoint->NavType());
		NDebugOverlay::Box(waypoint->GetPos(), Vector(-3,-3,-3),Vector(3,3,3), RGB[0],RGB[1],RGB[2], true,0);

		if (waypoint->GetNext()) 
		{
			Vector RGB = GetRouteColor(waypoint->GetNext()->NavType(), waypoint->GetNext()->Flags());
			NDebugOverlay::Line(waypoint->GetPos(), waypoint->GetNext()->GetPos(),RGB[0],RGB[1],RGB[2], true,0);
		}
		waypoint = waypoint->GetNext();
	}

}

//-----------------------------------------------------------------------------
