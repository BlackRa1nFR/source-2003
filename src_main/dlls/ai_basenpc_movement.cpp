//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================


#include "cbase.h"

#include "game.h"			
#include "ndebugoverlay.h"

#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_node.h"
#include "ai_motor.h"
#include "ai_navigator.h"
#include "ai_hint.h"

//=============================================================================
// PATHING & HIGHER LEVEL MOVEMENT
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Static debug function to force all selected npcs to go to the 
//			given node
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ForceSelectedGo(CBaseEntity *pPlayer, const Vector &targetPos, const Vector &traceDir, bool bRun) 
{
	CAI_BaseNPC *npc = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (npc)
	{
		if (npc->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) 
		{
			Vector chasePosition = targetPos;
			float tolerance = 0;
			npc->TranslateEnemyChasePosition( pPlayer, chasePosition, tolerance );
			// It it legal to drop me here
			Vector	vUpBit = chasePosition;
			vUpBit.z += 1;

			trace_t tr;
			AI_TraceHull( chasePosition, vUpBit, npc->GetHullMins(), 
				npc->GetHullMaxs(), MASK_NPCSOLID, npc, COLLISION_GROUP_NONE, &tr );
			if (tr.startsolid || tr.fraction != 1.0 )
			{
				NDebugOverlay::BoxAngles(chasePosition, npc->GetHullMins(), 
					npc->GetHullMaxs(), npc->GetAbsAngles(), 255,0,0,20,0.5);
			}

			npc->m_vecLastPosition = chasePosition;
			if ( bRun )
				npc->SetSchedule( SCHED_FORCED_GO_RUN );
			else
				npc->SetSchedule( SCHED_FORCED_GO );
			npc->m_flMoveWaitFinished = gpGlobals->curtime;
		}
		npc = gEntList.NextEntByClass(npc);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Static debug function to make all selected npcs run around
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_BaseNPC::ForceSelectedGoRandom(void) 
{
	CAI_BaseNPC *npc = gEntList.NextEntByClass( (CAI_BaseNPC *)NULL );

	while (npc)
	{
		if (npc->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) 
		{
			npc->SetSchedule( SCHED_RUN_RANDOM );
			npc->GetNavigator()->SetMovementActivity(ACT_RUN);
		}
		npc = gEntList.NextEntByClass(npc);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ScheduledMoveToGoalEntity( int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity )
{
	SetSchedule( scheduleType );

	m_pGoalEnt = pGoalEntity;

	// HACKHACK: Call through TranslateEnemyChasePosition to fixup this goal position
	// UNDONE: Remove this and have NPCs that need this functionality fix up paths in the 
	// movement system instead of when they are specified.
	AI_NavGoal_t goal(pGoalEntity->GetAbsOrigin(), movementActivity, 128, AIN_YAW_TO_DEST);

	TranslateEnemyChasePosition( pGoalEntity, goal.dest, goal.tolerance );
	
	return GetNavigator()->SetGoal( goal );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ScheduledFollowPath( int scheduleType, CBaseEntity *pPathStart, Activity movementActivity )
{
	SetSchedule( scheduleType );

	m_pGoalEnt = pPathStart;

	// HACKHACK: Call through TranslateEnemyChasePosition to fixup this goal position
	AI_NavGoal_t goal(GOALTYPE_PATHCORNER, pPathStart->GetLocalOrigin(), movementActivity, AIN_DEF_TOLERANCE, AIN_YAW_TO_DEST);

	TranslateEnemyChasePosition( pPathStart, goal.dest, goal.tolerance );

	return GetNavigator()->SetGoal( goal );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CAI_BaseNPC::IsMoving( void ) 
{ 
	return GetNavigator()->IsGoalSet(); 
}


//-----------------------------------------------------------------------------

bool CAI_BaseNPC::IsCurTaskContinuousMove()
{
	const Task_t* pTask = GetTask();
	if (pTask &&
		(pTask->iTask != TASK_WAIT_FOR_MOVEMENT)    &&
		(pTask->iTask != TASK_MOVE_TO_TARGET_RANGE) &&
		(pTask->iTask != TASK_WEAPON_RUN_PATH))
	{
		return false;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Used to specify that the NPC has a reason not to use the a navigation node
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsUnusableNode(int iNodeID, CAI_Hint *pHint)
{
	if ( m_bHintGroupNavLimiting && m_strHintGroup != NULL_STRING && STRING(m_strHintGroup)[0] != 0 )
	{
		if (!pHint || pHint->GetGroup() != GetHintGroup())
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks the validity of the given route's goaltype
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::ValidateNavGoal()
{
	if (GetNavigator()->GetGoalType() == GOALTYPE_COVER)
	{
		// Check if this location will block my enemy's line of sight to me
		if (GetEnemy())
		{
			Activity nCoverActivity = GetCoverActivity( m_pHintNode );
			Vector	 vCoverLocation = GetNavigator()->GetGoalPos();


			// For now we have to drop the node to the floor so we can
			// get an accurate postion of the NPC.  Should change once Ken checks in
			float floorZ = GetFloorZ(vCoverLocation);
			vCoverLocation.z = floorZ;


			Vector vEyePos = vCoverLocation + EyeOffset(nCoverActivity);

			if (!IsCoverPosition( GetEnemy()->EyePosition(), vEyePos ) )
			{
				TaskFail(FAIL_BAD_PATH_GOAL);
				return false;
			}
		}
	}
	return true;
}



//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CAI_BaseNPC::OpenDoorAndWait( CBaseEntity *pDoor )
{
	float flTravelTime = 0;

	//DevMsg( 2, "A door. ");
	if (pDoor && !pDoor->IsLockedByMaster())
	{
		pDoor->Use(this, this, USE_ON, 0.0);
		flTravelTime = pDoor->GetMoveDoneTime();
		if ( pDoor->GetEntityName() != NULL_STRING )
		{
			CBaseEntity *pTarget = NULL;
			for (;;)
			{
				pTarget = gEntList.FindEntityByName( pTarget, pDoor->GetEntityName(), NULL );

				if ( pTarget != pDoor )
				{
					if ( !pTarget )
						break;

					if ( FClassnameIs( pTarget, pDoor->GetClassname() ) )
					{
						pTarget->Use(this, this, USE_ON, 0.0);
					}
				}
			}
		}
	}

	return gpGlobals->curtime + flTravelTime;
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos, 
							   float maxUp, float maxDown, float maxDist ) const
{
	if ((endPos.z - startPos.z) > maxUp + 0.1) 
		return false;
	if ((startPos.z - endPos.z) > maxDown + 0.1) 
		return false;

	if ((apex.z - startPos.z) > maxUp * 1.25 ) 
		return false;

	float dist = (startPos - endPos).Length();
	if ( dist > maxDist + 0.1) 
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if a reasonable jumping distance
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const
{
	const float MAX_JUMP_RISE		= 80.0f;
	const float MAX_JUMP_DISTANCE	= 250.0f;
	const float MAX_JUMP_DROP		= 160.0f;

	return IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DISTANCE, MAX_JUMP_DROP );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a throw velocity from start to end position
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CAI_BaseNPC::CalcThrowVelocity(const Vector &startPos, const Vector &endPos, float fGravity, float fArcSize) 
{
	// Get the height I have to throw to get to the target
	float stepHeight = endPos.z - startPos.z;
	float throwHeight = 0;

	// -----------------------------------------------------------------
	// Now calcluate the distance to a point halfway between our current
	// and target position.  (the apex of our throwing arc)
	// -----------------------------------------------------------------
	Vector targetDir2D = endPos - startPos;
	targetDir2D.z = 0;

	float distance = VectorNormalize(targetDir2D);


	// If jumping up we want to throw a bit higher than the height diff	
	if (stepHeight > 0) 
	{
		throwHeight = stepHeight + fArcSize;	
	}	
	else
	{
		throwHeight = fArcSize;
	}
	// Make sure that I at least catch some air
	if (throwHeight < fArcSize)
	{
		throwHeight = fArcSize;
	}


	// -------------------------------------------------------------
	// calculate the vertical and horizontal launch velocities
	// -------------------------------------------------------------
	float velVert = (float)sqrt(2.0f*fGravity*throwHeight);

	float divisor = velVert;
	divisor += (float)sqrt((2.0f*(-fGravity)*(stepHeight-throwHeight)));

	float velHorz = (distance * fGravity)/divisor;

	// -----------------------------------------------------------
	// Make the horizontal throw vector and add vertical component
	// -----------------------------------------------------------
	Vector throwVel = targetDir2D * velHorz;
	throwVel.z = velVert;

	return throwVel;
}

bool CAI_BaseNPC::ShouldMoveWait()
{
	return (m_flMoveWaitFinished > gpGlobals->curtime);
}


//-----------------------------------------------------------------------------
// Purpose: execute any movement this sequence may have
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::AutoMovement( )
{
	bool ignored;
	Vector newPos;
	QAngle newAngles;

	if (GetIntervalMovement( m_flAnimTime - m_flPrevAnimTime, ignored, newPos, newAngles ))
	{
	// Msg( "%.2f : %.1f %.1f %.1f\n", gpGlobals->curtime, newPos.x, newPos.y, newAngles.y );
	
		if (GetMoveType() == MOVETYPE_STEP)
		{
			if (!(GetFlags() & FL_FLY))
			{
				return ( GetMotor()->MoveGroundStep( newPos, GetNavTargetEntity(), newAngles.y, false ) == AIM_SUCCESS );
			}
			else
			{
				// FIXME: here's no direct interface to a fly motor, plus this needs to support a state where going through the world is okay.
				// FIXME: add callbacks into the script system for validation
				// FIXME: add function on scripts to force only legal movements
				// FIXME: GetIntervalMovement deals in Local space, nor global.  Currently now way to communicate that through these interfaces.
				SetLocalOrigin( newPos );
				SetLocalAngles( newAngles );
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: return max 1/10 second rate of turning
// Input  :
// Output :
//-----------------------------------------------------------------------------

float CAI_BaseNPC::MaxYawSpeed( void )
{
	return 45;
}

//=============================================================================
