//========= Copyright (c) 1996-2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "ai_basehumanoid.h"
#include "ai_basehumanoid_movement.h"
#include "ai_route.h"

#ifdef HL2_DLL
#include "ai_interactions.h"
#endif


void CBaseAnimating::ResetSequence(int nSequence)
{
	/*
	if ((m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
	{
		Msg("ResetSequence : %s: %s -> %s\n", GetClassname(), GetSequenceName(GetSequence()), GetSequenceName(nSequence));
	}
	*/
	
	m_nSequence = nSequence;
	ResetSequenceInfo();
}



//-----------------------------------------------------------------------------
// CLASS: CAI_BaseHumanoid
//
// Purpose: The home of fancy human animation transition code
//
//-----------------------------------------------------------------------------

CAI_BaseHumanoid::CAI_BaseHumanoid()
{

}

//-------------------------------------

CAI_Motor *CAI_BaseHumanoid::CreateMotor()
{
	return new CAI_HumanoidMotor( this );
	// return new CAI_Motor( this );
}

//-------------------------------------

CAI_Navigator *CAI_BaseHumanoid::CreateNavigator()
{
	return new CAI_HumanoidNavigator( this );
	// return new CAI_Navigator( this );
}
	
//-----------------------------------------------------------------------------
// Purpose: allows each sequence to have a different
//			turn rate associated with it.
//-----------------------------------------------------------------------------
float CAI_BaseHumanoid::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 60;
		break;
	case ACT_RUN:
	case ACT_RUN_HURT:
	case ACT_WALK_HURT:
		return 15;
		break;
	case ACT_WALK:
	case ACT_WALK_CROUCH:
	case ACT_RUN_CROUCH:
		return 15;
		break;
	default:
		if (IsMoving())
		{
			return 15;
		}
		return 45; // too fast?
		break;
	}
}

//-------------------------------------

void CAI_BaseHumanoid::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_STOP_MOVING:
		break;
	}

	BaseClass::StartTask( pTask );
}


//-------------------------------------

void CAI_BaseHumanoid::RunTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_STOP_MOVING:
		break;
	}

	BaseClass::RunTask( pTask );
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CAI_BaseHumanoid::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
#ifdef HL2_DLL
	// Annoying to ifdef this out. Copy it into all the HL2 specific humanoid NPC's instead?
	if ( interactionType == g_interactionBarnacleVictimDangle )
	{
		// Force choosing of a new schedule
		ClearSchedule();
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		m_IdealNPCState = NPC_STATE_IDLE;
		SetAbsVelocity( vec3_origin );
		SetMoveType( MOVETYPE_STEP );
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimGrab )
	{
		if ( GetFlags() & FL_ONGROUND )
		{
			RemoveFlag( FL_ONGROUND );
		}
		m_IdealNPCState = NPC_STATE_PRONE;
		PainSound();
		return true;
	}
#endif
	return BaseClass::HandleInteraction( interactionType, data, sourceEnt);
}


//-------------------------------------

/*
void CAI_BaseHumanoid::SetMoveSequence( float flGoalDistance )
{
	if (m_script.Count() == 0)
	{
		BuildMoveScript( flGoalDistance );
	}

	if (m_iScript == -1)
	{
		m_iScript++;
		m_flCycle = 0;
		SetActivity( m_script[m_iScript].eActivity );
		ResetSequence( m_script[m_iScript].iSequence );
		// m_bMoveSeqFinished = false;
	}
	else if ((!m_script[m_iScript].bLooping) && IsSequenceFinished())
	{
		m_iScript++;
		m_flCycle = GetExitPhase( GetSequence() );
		Msg( "exitphase %.1f\n", m_flCycle );
		SetActivity( m_script[m_iScript].eActivity );
		ResetSequence( m_script[m_iScript].iSequence );
		// m_bMoveSeqFinished = false;
	} 
	else if (m_script[m_iScript].bLooping && m_flExitDist > flGoalDistance)
	{
		m_iScript++;
		SetActivity( m_script[m_iScript].eActivity );
		ResetSequence( m_script[m_iScript].iSequence );
		m_flCycle = GetMovementFrame( m_flExitDist - flGoalDistance);
		// Msg( "exit %.1f flGoalDistance %.1f\n", m_flExitDist, flGoalDistance );
		ResetSequenceInfo( );
		// m_bMoveSeqFinished = false;
	}

	if (GetIdealActivity() == ACT_INVALID)
	{
		m_script.RemoveAll();
		// _asm int 3;
	}
	// Msg( "entry %.1f exit %.1f\n", m_flEntryDist, m_flExitDist );
}
*/

//-------------------------------------

void CAI_BaseHumanoid::BuildMoveScript( float flGoalDistance )
{
	m_scriptMove.RemoveAll();
	m_scriptTurn.RemoveAll();

	BuildMoveScript( );
	BuildTurnScript( );

	/*
	if (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
	{
		int i;
#if 0

		for (i = 1; i < m_scriptMove.Count(); i++)
		{
			NDebugOverlay::Line( m_scriptMove[i-1].vecLocation, m_scriptMove[i].vecLocation, 255,255,255, true, 0.1 );

			NDebugOverlay::Box( m_scriptMove[i].vecLocation, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 0,255,255, 0, 0.1 );

			NDebugOverlay::Line( m_scriptMove[i].vecLocation, m_scriptMove[i].vecLocation + Vector( 0,0,m_scriptMove[i].flMaxVelocity), 0,255,255, true, 0.1 );
		}
#endif
#if 0
		for (i = 1; i < m_scriptTurn.Count(); i++)
		{
			NDebugOverlay::Line( m_scriptTurn[i-1].vecLocation, m_scriptTurn[i].vecLocation, 255,255,255, true, 0.1 );

			NDebugOverlay::Box( m_scriptTurn[i].vecLocation, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 255,0,0, 0, 0.1 );

			NDebugOverlay::Line( m_scriptTurn[i].vecLocation + Vector( 0,0,1), m_scriptTurn[i].vecLocation + Vector( 0,0,1) + UTIL_YawToVector( m_scriptTurn[i].flYaw ) * 32, 255,0,0, true, 0.1 );
		}
#endif
	}
	*/
}	


#define YAWSPEED	150


void CAI_BaseHumanoid::BuildTurnScript( )
{
	int i;

	AI_Movementscript_t script;
	script.Init();

	// current location
	script.vecLocation = GetAbsOrigin();
	script.flYaw = GetAbsAngles().y;
	m_scriptTurn.AddToTail( script );

	//-------------------------

	// insert default turn parameters, try to turn 80% to goal at all corners before getting there
	int prev = 0;
	for (i = 0; i < m_scriptMove.Count(); i++)
	{
		AI_Waypoint_t *pCurWaypoint = m_scriptMove[i].pWaypoint;
		if (pCurWaypoint)
		{
			script.Init();
			script.vecLocation = pCurWaypoint->vecLocation;
			script.pWaypoint = pCurWaypoint;
			script.flElapsedTime = m_scriptMove[i].flElapsedTime;

			m_scriptTurn[prev].flTime = script.flElapsedTime - m_scriptTurn[prev].flElapsedTime;

			if (pCurWaypoint->GetNext())
			{
				Vector d1 = pCurWaypoint->GetNext()->vecLocation - script.vecLocation;
				Vector d2 = script.vecLocation - m_scriptTurn[prev].vecLocation;
				
				d1.z = 0;
				VectorNormalize( d1 );
				d2.z = 0;
				VectorNormalize( d2 );

				float y1 = UTIL_VecToYaw( d1 );
				float y2 = UTIL_VecToYaw( d2 );

				float deltaYaw = fabs( UTIL_AngleDiff( y1, y2 ) );

				if (deltaYaw > 0.1)
				{
					// turn to 80% of goal
					script.flYaw = UTIL_ApproachAngle( y1, y2, deltaYaw * 0.8 );
					m_scriptTurn.AddToTail( script );
					// Msg("turn waypoint %.1f %.1f %.1f\n", script.vecLocation.x, script.vecLocation.y, script.vecLocation.z );
					prev++;
				}
			}
			else
			{
				Vector vecDir = GetNavigator()->GetArrivalDirection();
				script.flYaw = UTIL_VecToYaw( vecDir );
				m_scriptTurn.AddToTail( script );
				// Msg("turn waypoint %.1f %.1f %.1f\n", script.vecLocation.x, script.vecLocation.y, script.vecLocation.z );
				prev++;
			}
		}
	}

	// propagate ending facing back over any nearby nodes
	// FIXME: this needs to minimize total turning, not just local/end turning.
	// depending on waypoint spacing, complexity, it may turn the wrong way!
	for (i = m_scriptTurn.Count()-1; i > 1; i--)
	{
		float deltaYaw = UTIL_AngleDiff( m_scriptTurn[i-1].flYaw, m_scriptTurn[i].flYaw );
	
		float maxYaw = YAWSPEED * m_scriptTurn[i-1].flTime;

		if (fabs(deltaYaw) > maxYaw)
		{
			m_scriptTurn[i-1].flYaw = UTIL_ApproachAngle( m_scriptTurn[i-1].flYaw, m_scriptTurn[i].flYaw, maxYaw );
		}
	}

	for (i = 0; i < m_scriptTurn.Count() - 1; )
	{
		i = i + BuildTurnScript( i, i + 1 ) + 1;
	}
	//-------------------------
}



int CAI_BaseHumanoid::BuildTurnScript( int i, int j )
{
	int k;

	Vector vecDir = m_scriptTurn[j].vecLocation - m_scriptTurn[i].vecLocation;
	float interiorYaw = UTIL_VecToYaw( vecDir );

	float deltaYaw;

	deltaYaw = fabs( UTIL_AngleDiff( interiorYaw, m_scriptTurn[i].flYaw ) );
	float t1 = deltaYaw / YAWSPEED;

	deltaYaw = fabs( UTIL_AngleDiff( m_scriptTurn[j].flYaw, interiorYaw ) );
	float t2 = deltaYaw / YAWSPEED;

	float totalTime = m_scriptTurn[j].flElapsedTime - m_scriptTurn[i].flElapsedTime;

	Assert( totalTime >  0 );

	if (t1 < 0.01)
	{
		if (t2 > totalTime * 0.8)
		{
			// too close, nothing to do
			return 0;
		}

		// go ahead and force yaw
		m_scriptTurn[i].flYaw = interiorYaw;

		// we're already aiming close enough to the interior yaw, set the point where we need to blend out
		k = BuildInsertNode( i, totalTime - t2 );
		m_scriptTurn[k].flYaw = interiorYaw;

		return 1;
	}
	else if (t2 < 0.01)
	{
		if (t1 > totalTime * 0.8)
		{
			// too close, nothing to do
			return 0;
		}

 		// we'll finish up aiming close enough to the interior yaw, set the point where we need to blend in
		k = BuildInsertNode( i, t1 );
		m_scriptTurn[k].flYaw = interiorYaw;
		
		return 1;
	}
	else if (t1 + t2 > totalTime)
	{
		// don't bother with interior node
		return 0;
		
		// waypoints need to much turning, ignore interior yaw
		float a = (t1 / (t1 + t2));
		t1 = a * totalTime;

		k = BuildInsertNode( i, t1 );

		deltaYaw = UTIL_AngleDiff( m_scriptTurn[j].flYaw, m_scriptTurn[i].flYaw );
		m_scriptTurn[k].flYaw = UTIL_ApproachAngle( m_scriptTurn[j].flYaw, m_scriptTurn[i].flYaw, deltaYaw * (1 - a) );

		return 1;
	}
	else if (t1 + t2 < totalTime * 0.8)
	{
		// turn to face interior, run a ways, then turn away
		k = BuildInsertNode( i, t1 );
		m_scriptTurn[k].flYaw = interiorYaw;

		k = BuildInsertNode( i, t2 );
		m_scriptTurn[k].flYaw = interiorYaw;

		return 2;
	}
	return 0;
}


int CAI_BaseHumanoid::BuildInsertNode( int i, float flTime )
{
	AI_Movementscript_t script;
	script.Init();

	Assert( flTime > 0.0 );

	for (i; i < m_scriptTurn.Count(); i++)
	{
		if (m_scriptTurn[i].flTime < flTime)
		{
			flTime -= m_scriptTurn[i].flTime;
		}
		else
		{
			float a = flTime / m_scriptTurn[i].flTime;

			script.flTime = (m_scriptTurn[i].flTime - flTime);

			m_scriptTurn[i].flTime = flTime;

			script.flElapsedTime = m_scriptTurn[i].flElapsedTime * (1 - a) + m_scriptTurn[i+1].flElapsedTime * a;

			script.vecLocation = m_scriptTurn[i].vecLocation * (1 - a) + m_scriptTurn[i+1].vecLocation * a;

			m_scriptTurn.InsertAfter( i, script );

			return i + 1;
		}
	}
	Assert( 0 );
	return 0;
}



void CAI_BaseHumanoid::BuildMoveScript( )
{
	int i;
	float a;

	float idealVelocity = 50;
	if (GetIdealSpeed() != 0)
	{
		idealVelocity = GetIdealSpeed();
	}
	float idealAccel = idealVelocity / 1.0 + 50;

	AI_Movementscript_t script;

	// set current location as start of script
	script.vecLocation = GetAbsOrigin();
	script.flYaw = GetAbsAngles().y;
	script.flMaxVelocity = GetMotor()->GetCurSpeed();
	m_scriptMove.AddToTail( script );

	//-------------------------

	// add all waypoint locations and velocities
	AI_Waypoint_t *pCurWaypoint = GetNavigator()->GetPath()->GetCurWaypoint();
	float flTotalDist = 0;
	while (pCurWaypoint /*&& flTotalDist / idealVelocity < 3.0*/) // limit lookahead to 3 seconds
	{
		script.Init();

		script.vecLocation = pCurWaypoint->vecLocation;
		script.pWaypoint = pCurWaypoint;

		// Msg("waypoint %.1f %.1f %.1f\n", script.vecLocation.x, script.vecLocation.y, script.vecLocation.z );

		AI_Waypoint_t *pNext = pCurWaypoint->GetNext();

		if (pNext)
		{
			Vector d1 = pNext->vecLocation - script.vecLocation;
			Vector d2 = script.vecLocation - m_scriptMove[m_scriptMove.Count()-1].vecLocation;
			
			// remove very short links
			if (d1.Length() < 1.0)
			{
				if (m_scriptMove.Count() > 1)
				{
					int i = m_scriptMove.Count() - 1;
					m_scriptMove[i].vecLocation = pCurWaypoint->vecLocation;
					m_scriptMove[i].pWaypoint = pCurWaypoint;
				}
				pCurWaypoint = pNext;
				continue;
			}

			d1.z = 0;
			flTotalDist += VectorNormalize( d1 );
			d2.z = 0;
			VectorNormalize( d2 );

			// figure velocity
			float dot = (DotProduct( d1, d2 ) + 0.2);
			if (dot > 0)
			{
				dot = clamp( dot, 0.0f, 1.0f );
				script.flMaxVelocity = idealVelocity * dot;
			}
			else
			{
				script.flMaxVelocity = 0;
			}
		}
		else
		{
			script.flMaxVelocity = 0;
		}

		m_scriptMove.AddToTail( script );
		pCurWaypoint = pNext;
	}

	//-------------------------

	// add accel, decel
	for (i = 0; i < m_scriptMove.Count() - 1; )
	{
		Vector vecDir = m_scriptMove[i+1].vecLocation - m_scriptMove[i].vecLocation;
		m_scriptMove[i].flDist = VectorNormalize( vecDir );

		if (m_scriptMove[i].flDist < 0.01)
		{
			if (i != 0)
			{
				m_scriptMove.Remove( i );
				continue;
			}
		}
		i++;
	}

	// clamp forward velocities
	for (i = 0; i < m_scriptMove.Count() - 1; i++ )
	{
		// time, distance to accel to next max vel
		float dv = m_scriptMove[i+1].flMaxVelocity - m_scriptMove[i].flMaxVelocity;

		if (dv > 0.0)
		{
			float t1 = dv / idealAccel;
			float d1 = m_scriptMove[i].flMaxVelocity * t1 + 0.5 * (idealAccel) * t1 * t1;

			if (d1 > m_scriptMove[i].flDist)
			{
				float r1, r2;
				
				if (SolveQuadratic( 0.5 * idealAccel, m_scriptMove[i].flMaxVelocity, -m_scriptMove[i].flDist, r1, r2 ))
				{
					m_scriptMove[i+1].flMaxVelocity = m_scriptMove[i].flMaxVelocity + idealAccel * r1;
				}
			}
		}
	}

	// clamp decel velocities
	for (i = m_scriptMove.Count() - 1; i > 0; i-- )
	{
		// time, distance to accel to next max vel
		float dv = m_scriptMove[i].flMaxVelocity - m_scriptMove[i-1].flMaxVelocity;

		if (dv > 0.0)
		{
			float t1 = dv / idealAccel;
			float d1 = m_scriptMove[i].flMaxVelocity * t1 + 0.5 * (idealAccel) * t1 * t1;

			if (d1 > m_scriptMove[i].flDist)
			{
				float r1, r2;
				
				if (SolveQuadratic( 0.5 * idealAccel, m_scriptMove[i].flMaxVelocity, -m_scriptMove[i].flDist, r1, r2 ))
				{
					m_scriptMove[i-1].flMaxVelocity = m_scriptMove[i].flMaxVelocity - idealAccel * r1;
				}
			}
		}
	}

	/*
	for (i = 0; i < m_scriptMove.Count(); i++)
	{
		Msg("%.2f ", m_scriptMove[i].flMaxVelocity );
	}
	Msg("\n");
	*/

	// insert ideal velocities
	for (i = 0; i < m_scriptMove.Count() - 1;)
	{
		// accel to ideal
		float t1 = (idealVelocity - m_scriptMove[i].flMaxVelocity) / idealAccel;
		float d1 = m_scriptMove[i].flMaxVelocity * t1 + 0.5 * (idealAccel) * t1 * t1;

		// decel from ideal
		float t2 = (idealVelocity - m_scriptMove[i+1].flMaxVelocity) / idealAccel;
		float d2 = m_scriptMove[i+1].flMaxVelocity * t2 + 0.5 * (idealAccel) * t2 * t2;

		m_scriptMove[i].flDist = (m_scriptMove[i+1].vecLocation - m_scriptMove[i].vecLocation).Length();

		if (d1 + d2 < m_scriptMove[i].flDist)
		{
			// it's possible to accel and decal to idealVelocity between next two nodes
			Vector start =  m_scriptMove[i].vecLocation;
			Vector end = m_scriptMove[i+1].vecLocation;
			float dist = m_scriptMove[i].flDist;

			// insert the two points needed to end accel and start decel
			if (d1 > 1.0 && t1 > 0.1)
			{
				a = d1 / dist;

				script.Init();
				script.vecLocation = end * a + start * (1 - a);
				script.flMaxVelocity = idealVelocity;
				m_scriptMove.InsertAfter( i, script );
				i++;
			}

			if (dist - d2 > 1.0 && t2 > 0.1)
			{
				// Msg("%.2f : ", a );

				a = (dist - d2) / dist;

				script.Init();
				script.vecLocation = end * a + start * (1 - a);
				script.flMaxVelocity = idealVelocity;
				m_scriptMove.InsertAfter( i, script );
				i++;
			}

			i++;
		}
		else
		{
			// check to see if the amount of change needed to reach target is less than the ideal acceleration
			float flNeededAccel = fabs( DeltaV( m_scriptMove[i].flMaxVelocity, m_scriptMove[i+1].flMaxVelocity, m_scriptMove[i].flDist ) );
			if (flNeededAccel < idealAccel)
			{
				// if so, they it's possible to get a bit towards the ideal velocity
				float v1 = m_scriptMove[i].flMaxVelocity;
				float v2 = m_scriptMove[i+1].flMaxVelocity;

				// based on solving:
				//		v1+A*t1-v2+A*t2=0
				//		v1*t1+0.5*A*t1*t1-v2*t2+0.5*A*t2*t2=0
				// replacing t2 with:
				//		t2=(-v1-A*t1+v2)/A
				// gives
				//		v1*t1+0.5*A*t1*t1+v2*(-v1/A-t1+v2/A)+0.5*A*(-v1/A-t1+v2/A)*(-v1/A-t1+v2/A)-d = 0

				float a = idealAccel;
				float b = 2*v1 - 2*v2;
				float c = (v2+0.5*v2+0.5*v1)*(v1/idealAccel) + (v2+0.5*v2+0.5*v1)*(v2/idealAccel) + (-m_scriptMove[i].flDist);

				float t1,t1b;
					
				if (SolveQuadratic( a, b, c, t1, t1b ))
				{
					float d1 = m_scriptMove[i].flMaxVelocity * t1 + 0.5 * idealAccel * t1 * t1;

					float a = d1 / m_scriptMove[i].flDist;
					script.Init();
					script.vecLocation = m_scriptMove[i+1].vecLocation * a + m_scriptMove[i].vecLocation * (1 - a);
					script.flMaxVelocity = m_scriptMove[i].flMaxVelocity + idealAccel * t1;

					if (script.flMaxVelocity < idealVelocity)
					{
						// Msg("insert %.2f %.2f %.2f\n", m_scriptMove[i].flMaxVelocity, script.flMaxVelocity, m_scriptMove[i+1].flMaxVelocity ); 
						m_scriptMove.InsertAfter( i, script );
						i += 1;
					}
				}
			}
			i += 1;
		}
	}

	// rebuild fields
	m_scriptMove[0].flElapsedTime = 0;
	for (i = 0; i < m_scriptMove.Count() - 1; )
	{
		m_scriptMove[i].flDist = (m_scriptMove[i+1].vecLocation - m_scriptMove[i].vecLocation).Length();

		if (m_scriptMove[i].flMaxVelocity > 0 || m_scriptMove[i+1].flMaxVelocity > 0)
		{
			float t = m_scriptMove[i].flDist / (0.5 * (m_scriptMove[i].flMaxVelocity + m_scriptMove[i+1].flMaxVelocity));
			m_scriptMove[i].flTime = t;
		}
		else
		{
			m_scriptMove[i].flTime = 1.0;
		}

		if (m_scriptMove[i].flDist < 0.01 || m_scriptMove[i].flTime < 0.01)
		{
			// Assert( m_scriptMove[i+1].pWaypoint == NULL );

			m_scriptMove.Remove( i + 1 );
			continue;
		}

		m_scriptMove[i+1].flElapsedTime = m_scriptMove[i].flElapsedTime + m_scriptMove[i].flTime;

		i++;
	}

	/*
	for (i = 0; i < m_scriptMove.Count(); i++)
	{
		// Msg("(%.2f : %.2f : %.2f)", m_scriptMove[i].flMaxVelocity, m_scriptMove[i].flDist, m_scriptMove[i].flTime );
		Msg("(%.2f:%.2f)", m_scriptMove[i].flTime, m_scriptMove[i].flElapsedTime );
	}
	Msg("\n");
	*/
}





#if 0
void CAI_BaseHumanoid::BuildMoveScript( float flGoalDistance )
{
	movementscript_t script;

	m_script.RemoveAll();

	// FIXME: this is random
	Activity interiorActivity= GetNavigator()->GetMovementActivity();
	int interiorSequence = SelectWeightedSequence( NPC_TranslateActivity( interiorActivity ) );
	// int interiorNode = GetEntryNode( interiorSequence );
	
	// FIXME: this is random
	Activity goalActivity = GetTransitionActivity();
	int goalSequence = SelectWeightedSequence( NPC_TranslateActivity( goalActivity ) );
	// int goalNode = GetEntryNode( goalSequence );

	/*
	m_flEntryDist = 0;
	int nextSequence = FindTransitionSequence( GetSequence(), interiorSequence, NULL );
	while ( nextSequence != interiorSequence)
	{
		m_flEntryDist += GetSequenceMoveDist( nextSequence );
		script.iSequence = nextSequence;
		script.bLooping = false;
		script.eActivity = interiorActivity;
		m_script.AddToTail( script );
		nextSequence = FindTransitionSequence( nextSequence, interiorSequence, NULL );
	}

	script.iSequence = nextSequence;
	script.bLooping = true;
	script.eActivity = interiorActivity;
	m_script.AddToTail( script );

	m_flExitDist = 0;
	if (nextSequence != goalSequence)
	{
		nextSequence = FindTransitionSequence( nextSequence, goalSequence, NULL );

		while ( nextSequence != goalSequence)
		{
			m_flExitDist += GetSequenceMoveDist( nextSequence );
			script.iSequence = nextSequence;
			script.bLooping = false;
			script.eActivity = goalActivity;
			m_script.AddToTail( script );
			nextSequence = FindTransitionSequence( nextSequence, goalSequence, NULL );
		}

		script.iSequence = nextSequence;
		script.bLooping = false;
		script.eActivity = goalActivity;
		m_script.AddToTail( script );
	}

	// explicit close
	script.eActivity = ACT_INVALID;
	m_script.AddToTail( script );
	*/

	m_iScript = -1;

	if (m_flEntryDist + m_flExitDist > flGoalDistance)
	{
		float d = flGoalDistance / (m_flEntryDist + m_flExitDist);
		m_flEntryDist *= d;
		m_flExitDist *= d;
	}
}
#endif

//-------------------------------------

#if 0
Activity CAI_BaseHumanoid::GetTransitionActivity( )
{
	AI_Waypoint_t *waypoint = GetNavigator()->GetPath()->GetTransitionWaypoint();

	if ( waypoint->Flags() & bits_WP_TO_GOAL )
	{
		if ( waypoint->activity != ACT_INVALID)
		{
			return waypoint->activity;
		}

		return GetStoppedActivity( );
	}

	if (waypoint)
		waypoint = waypoint->GetNext();

	switch(waypoint->NavType() )
	{
	case NAV_JUMP:
		return ACT_JUMP; // are jumps going to get a movement track added to them?

	case NAV_GROUND:
		return GetNavigator()->GetMovementActivity(); // yuck

	case NAV_CLIMB:
		return ACT_CLIMB_UP; // depends on specifics of climb node

	default:
		return ACT_IDLE;
	}
}
#endif

//-------------------------------------
// Purpose:	return a velocity that should be hit at the end of the interval to match goal
// Input  : flInterval - time interval to consider
//        : flGoalDistance - distance to goal
//        : flGoalVelocity - desired velocity at goal
//        : flCurVelocity - current velocity
//        : flIdealVelocity - velocity to go at if goal is too far away
//        : flAccelRate - maximum acceleration/deceleration rate
// Output : target velocity at time t+flInterval
//-------------------------------------

float ChangeDistance( float flInterval, float flGoalDistance, float flGoalVelocity, float flCurVelocity, float flIdealVelocity, float flAccelRate, float &flNewDistance, float &flNewVelocity )
{
	float scale = 1.0;
	if (flGoalDistance < 0)
	{
		flGoalDistance = - flGoalDistance;
		flCurVelocity = -flCurVelocity;
		scale = -1.0;
	}

	flNewVelocity = flCurVelocity;
	flNewDistance = 0.0;

	// if I'm too close, just go ahead and set the velocity
	if (flGoalDistance < 0.01)
	{
		return flGoalVelocity * scale;
	}

	float flGoalAccel = DeltaV( flCurVelocity, flGoalVelocity, flGoalDistance );

	flNewVelocity = flCurVelocity;

	// --------------------------------------------
	// if goal is close enough try to match the goal velocity, else try to go ideal velocity
	// --------------------------------------------
	if (flGoalAccel < 0 && flGoalAccel < -flAccelRate)
	{
		// I need to slow down;
		flNewVelocity = flCurVelocity + flGoalAccel * flInterval;
		if (flNewVelocity < 0)
			flNewVelocity = 0;
	}
	else if (flGoalAccel > 0 && flGoalAccel >= flAccelRate)
	{
		// I need to speed up
		flNewVelocity = flCurVelocity + flGoalAccel * flInterval;
		if (flNewVelocity > flGoalVelocity)
			flGoalVelocity = flGoalVelocity;
	}
	else if (flNewVelocity < flIdealVelocity)
	{
		// speed up to ideal velocity;
		flNewVelocity = flCurVelocity + flAccelRate * flInterval;
		if (flNewVelocity > flIdealVelocity)
			flNewVelocity = flIdealVelocity;
		// don't overshoot
		if (0.5*(flNewVelocity + flCurVelocity) * flInterval > flGoalDistance)
		{
			flNewVelocity = 0.5 * (2 * flGoalDistance / flInterval - flCurVelocity);
		}
	}
	else if (flNewVelocity > flIdealVelocity)
	{
		// slow down to ideal velocity;
		flNewVelocity = flCurVelocity - flAccelRate * flInterval;
		if (flNewVelocity < flIdealVelocity)
			flNewVelocity = flIdealVelocity;
	}

	float flDist = 0.5*(flNewVelocity + flCurVelocity) * flInterval;

	if (flDist > flGoalDistance)
	{
		flDist = flGoalDistance;
		flNewVelocity = flGoalVelocity;
	}

	flNewVelocity = flNewVelocity * scale;

	flNewDistance = (flGoalDistance - flDist) * scale;
	
	return 0.0;
}

//-----------------------------------------------------------------------------
