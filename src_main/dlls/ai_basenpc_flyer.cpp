//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Base class for many flying NPCs
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "ai_basenpc_flyer.h"
#include "ai_route.h"
#include "ai_navigator.h"
#include "ai_motor.h"

BEGIN_DATADESC( CAI_BaseFlyingBot )

	DEFINE_FIELD( CAI_BaseFlyingBot,	m_vCurrentVelocity,			FIELD_VECTOR),
	DEFINE_FIELD( CAI_BaseFlyingBot,	m_vCurrentAngularVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CAI_BaseFlyingBot,	m_vCurrentBanking,			FIELD_VECTOR),
	DEFINE_FIELD( CAI_BaseFlyingBot,	m_vNoiseMod,				FIELD_VECTOR),
	DEFINE_FIELD( CAI_BaseFlyingBot,	m_fHeadYaw,					FIELD_FLOAT),
	DEFINE_FIELD( CAI_BaseFlyingBot,	m_vLastPatrolDir,			FIELD_VECTOR),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose : Override to return correct velocity
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_BaseFlyingBot::GetVelocity(Vector *vVelocity, QAngle *vAngVelocity)
{
	if (vVelocity != NULL)
	{
		VectorCopy(m_vCurrentVelocity,*vVelocity);
	}
	if (vAngVelocity != NULL)
	{
		*vAngVelocity = GetLocalAngularVelocity();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turn head yaw into facing direction
// Input  :
// Output :
//-----------------------------------------------------------------------------
QAngle CAI_BaseFlyingBot::BodyAngles()
{
	return QAngle(0,m_fHeadYaw,0);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseFlyingBot::TurnHeadToTarget(float flInterval, const Vector &MoveTarget )
{
	float desYaw = UTIL_AngleDiff(VecToYaw(MoveTarget - GetLocalOrigin()), GetLocalAngles().y);

	// If I've flipped completely around, reverse angles
	float fYawDiff = m_fHeadYaw - desYaw;
	if (fYawDiff > 180)
	{
		m_fHeadYaw -= 360;
	}
	else if (fYawDiff < -180)
	{
		m_fHeadYaw += 360;
	}

	// RIGHT NOW, this affects every flying bot. This rate should be member data that individuals
	// can manipulate. THIS change for MANHACKS E3 2003 (sjb)
	float iRate = 0.8;

	// Make frame rate independent
	float timeToUse = flInterval;
	while (timeToUse > 0)
	{
		m_fHeadYaw	   = (iRate * m_fHeadYaw) + (1-iRate)*desYaw;
		timeToUse -= 0.1;
	}

	while( m_fHeadYaw > 360 )  
	{
		m_fHeadYaw -= 360.0f;
	}

	while( m_fHeadYaw < 0 )
	{
		m_fHeadYaw += 360.f;
	}

	SetBoneController( 0, m_fHeadYaw );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CAI_BaseFlyingBot::MinGroundDist(void)
{
	return 0;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CAI_BaseFlyingBot::VelocityToAvoidObstacles(float flInterval)
{
	// --------------------------------
	//  Avoid banging into stuff
	// --------------------------------
	trace_t tr;
	Vector vTravelDir = m_vCurrentVelocity*flInterval;
	Vector endPos = GetAbsOrigin() + vTravelDir;
	AI_TraceEntity( this, GetAbsOrigin(), endPos, MASK_NPCSOLID|CONTENTS_WATER, &tr );
	if (tr.fraction != 1.0)
	{	
		// Bounce off in normal 
		Vector vBounce = tr.plane.normal * 0.5 * m_vCurrentVelocity.Length();
		return (vBounce);
	}
	
	// --------------------------------
	// Try to remain above the ground.
	// --------------------------------
	float flMinGroundDist = MinGroundDist();
	AI_TraceLine(GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, -flMinGroundDist), 
		MASK_NPCSOLID_BRUSHONLY|CONTENTS_WATER, this, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction < 1)
	{
		// Clamp veloctiy
		if (tr.fraction < 0.1)
		{
			tr.fraction = 0.1;
		}

		return Vector(0, 0, 50/tr.fraction);
	}
	return vec3_origin;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_BaseFlyingBot::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{	
		// Skip as done via bone controller
		case TASK_FACE_ENEMY:
		{
			TaskComplete();
			break;
		}
		// Activity is just idle (have no run)
		case TASK_RUN_PATH:
		{
			GetNavigator()->SetMovementActivity(ACT_IDLE);
			TaskComplete();
			break;
		}
		// Don't check for run/walk activity
		case TASK_RUN_TO_TARGET:
		case TASK_WALK_TO_TARGET:
		{
			if (GetTarget() == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else 
			{
				if (!GetNavigator()->SetGoal( GOALTYPE_TARGETENT ) )
				{
					TaskFail(FAIL_NO_ROUTE);
					GetNavigator()->ClearGoal();
				}
			}
			TaskComplete();
			break;
		}
		// Override to get more to get a directional path
		case TASK_GET_PATH_TO_RANDOM_NODE:  
		{
			if ( GetNavigator()->SetRandomGoal( pTask->flTaskData, m_vLastPatrolDir ) )
				TaskComplete();
			else
				TaskFail(FAIL_NO_REACHABLE_NODE);
			break;
		}
		default:
		{
			BaseClass::StartTask(pTask);
		}
	}
}

//------------------------------------------------------------------------------

void CAI_BaseFlyingBot::MoveToTarget(float flInterval, const Vector &MoveTarget)
{
}

//------------------------------------------------------------------------------

AI_NavPathProgress_t CAI_BaseFlyingBot::ProgressFlyPath( 
	float flInterval,
	const CBaseEntity *pNewTarget, 
	unsigned collisionMask, 
	bool bNewTrySimplify, 
	float strictPointTolerance)
{
  	AI_ProgressFlyPathParams_t params( collisionMask );

	params.SetCurrent( pNewTarget, bNewTrySimplify );

	AI_NavPathProgress_t progress = GetNavigator()->ProgressFlyPath( params );
	
	switch ( progress )
	{
		case AINPP_NO_CHANGE:
		case AINPP_ADVANCED:
		{
			MoveToTarget(flInterval, GetNavigator()->GetCurWaypointPos());
			break;
		}
		
		case AINPP_COMPLETE:
		{
			TaskMovementComplete();
			break;
		}
		
		case AINPP_BLOCKED: // function is not supposed to test blocking, just simple path progression
		default:
		{
			AssertMsg( 0, ( "Unexpected result" ) );
			break;
		}
	}

	return progress;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CAI_BaseFlyingBot::CAI_BaseFlyingBot()
{
#ifdef _DEBUG
	m_vCurrentVelocity.Init();
	m_vCurrentBanking.Init();
	m_vLastPatrolDir.Init();
#endif
}
