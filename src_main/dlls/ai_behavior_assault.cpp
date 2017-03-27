//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "ai_behavior_assault.h"
#include "ai_navigator.h"
#include "ai_memory.h"


BEGIN_DATADESC( CRallyPoint )
	DEFINE_KEYFIELD( CRallyPoint, m_AssaultPointName, FIELD_STRING, "assaultpoint" ),
	DEFINE_KEYFIELD( CRallyPoint, m_RallySequenceName, FIELD_STRING, "rallysequence" ),
	DEFINE_KEYFIELD( CRallyPoint, m_flAssaultDelay, FIELD_FLOAT, "assaultdelay" ),
	DEFINE_KEYFIELD( CRallyPoint, m_iPriority, FIELD_INTEGER, "priority" ),
	DEFINE_FIELD( CRallyPoint, m_IsLocked, FIELD_BOOLEAN ),

	DEFINE_OUTPUT( CRallyPoint, m_OnArrival, "OnArrival" ),
END_DATADESC();

BEGIN_DATADESC( CAssaultPoint )
	DEFINE_KEYFIELD( CAssaultPoint, m_AssaultHintGroup, FIELD_STRING, "assaultgroup" ),
	DEFINE_KEYFIELD( CAssaultPoint, m_NextAssaultPointName, FIELD_STRING, "nextassaultpoint" ),
	DEFINE_KEYFIELD( CAssaultPoint, m_flAssaultTimeout, FIELD_FLOAT, "assaulttimeout" ),

	DEFINE_OUTPUT( CAssaultPoint, m_OnArrival, "OnArrival" ),
	DEFINE_OUTPUT( CAssaultPoint, m_OnAssaultClear, "OnAssaultClear" ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( assault_rallypoint, CRallyPoint );	// just a copy of info_target for now
LINK_ENTITY_TO_CLASS( assault_assaultpoint, CAssaultPoint ); // has its own class because it needs the entity I/O

BEGIN_DATADESC( CAI_AssaultBehavior )
	DEFINE_FIELD( CAI_AssaultBehavior, m_hAssaultPoint, FIELD_EHANDLE ),
	DEFINE_FIELD( CAI_AssaultBehavior, m_hRallyPoint, FIELD_EHANDLE ),
	DEFINE_EMBEDDED( CAI_AssaultBehavior, m_ShotRegulator ),
	DEFINE_FIELD( CAI_AssaultBehavior, m_AssaultCue, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_AssaultBehavior, m_ReceivedAssaultCue, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_AssaultBehavior, m_bHitRallyPoint, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAI_AssaultBehavior, m_bHitAssaultPoint, FIELD_BOOLEAN )
END_DATADESC();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_AssaultBehavior::CAI_AssaultBehavior()
{
	m_AssaultCue = CUE_NO_ASSAULT;

	m_ShotRegulator.SetParameters( 2, 6, 0.3, 0.8 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::GatherConditions( void )
{
	BaseClass::GatherConditions();

	m_ShotRegulator.Update();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : cue - 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::ReceiveAssaultCue( AssaultCue_t cue )
{
	// This is not intended to be a multi-write function. It should be
	// used once to deliver the same cue to all actors associated
	// with this behavior.
	if( m_ReceivedAssaultCue != CUE_NO_ASSAULT )
	{
		Msg("***\nWARNING: overwriting Received Assault Cue. Tell WEDGE!\n***\n");
	}

	m_ReceivedAssaultCue = cue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::ClearAssaultPoint( void )
{
	// To announce clear means that this NPC hasn't seen any live targets in
	// the assault area for the length of time the level designer has 
	// specified that they should be vigilant. This effectively deactivates
	// the assault behavior.
	Msg("*** CLEAR!!!!\n");

	// Do we need to move to another assault point?
	if( m_hAssaultPoint->m_NextAssaultPointName != NULL_STRING )
	{
		CAssaultPoint *pNextPoint = (CAssaultPoint *)gEntList.FindEntityByName( NULL, m_hAssaultPoint->m_NextAssaultPointName, NULL );

		if( pNextPoint )
		{
			m_hAssaultPoint = pNextPoint;
			
			// Send our NPC to the next assault point!
			m_bHitAssaultPoint = false;

			return;
		}
		else
		{
			Msg("**ERROR: Can't find next assault point: %s\n", m_hAssaultPoint->m_NextAssaultPointName );

			// Bomb out of assault behavior.
			m_AssaultCue = CUE_NO_ASSAULT;
			GetOuter()->ClearSchedule();

			return;
		}
	}

	// Just set the cue back to NO_ASSAULT. This disables the behavior.
	m_AssaultCue = CUE_NO_ASSAULT;

	m_hRallyPoint->Lock( false );

	// If this assault behavior has changed the NPC's hint group,
	// slam that NPC's hint group back to null.
	// !!!TODO: if the NPC had a different hint group before the 
	// assault began, we're slamming that, too! We might want
	// to cache it off if this becomes a problem (sjb)
	if( m_hAssaultPoint->m_AssaultHintGroup != NULL_STRING )
	{
		GetOuter()->SetHintGroup( NULL_STRING );
	}

	m_hAssaultPoint->m_OnAssaultClear.FireOutput( GetOuter(), GetOuter(), 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		m_ShotRegulator.OnFiredWeapon();
		BaseClass::StartTask( pTask );
		break;

	case TASK_ASSAULT_MOVE_AWAY_PATH:
		break;

	case TASK_ANNOUNCE_CLEAR:
		{
			ClearAssaultPoint();
			TaskComplete();
		}
		break;

	case TASK_WAIT_ASSAULT_DELAY:
		{
			if( m_hRallyPoint )
			{
				GetOuter()->m_flWaitFinished = gpGlobals->curtime + m_hRallyPoint->m_flAssaultDelay;
			}
			else
			{
				TaskComplete();
			}
		}
		break;

	case TASK_AWAIT_ASSAULT_TIMEOUT:
		// Maintain vigil for as long as the level designer has asked. Wait
		// and look for targets.
		GetOuter()->m_flWaitFinished = gpGlobals->curtime + m_hAssaultPoint->m_flAssaultTimeout;
		break;

	case TASK_GET_PATH_TO_RALLY_POINT:
		{
			AI_NavGoal_t goal( m_hRallyPoint->GetAbsOrigin() );
			goal.pTarget = m_hRallyPoint;
			GetNavigator()->SetGoal( goal );
			GetNavigator()->SetArrivalDirection( m_hRallyPoint->GetAbsAngles() );
		}
		break;

	case TASK_FACE_RALLY_POINT:
		{
			GetMotor()->SetIdealYaw( m_hRallyPoint->GetAbsAngles().y );
			GetOuter()->SetTurnActivity(); 
		}
		break;

	case TASK_GET_PATH_TO_ASSAULT_POINT:
		{
			AI_NavGoal_t goal( m_hAssaultPoint->GetAbsOrigin() );
			goal.pTarget = m_hAssaultPoint;
			GetNavigator()->SetGoal( goal );
			GetNavigator()->SetArrivalDirection( m_hAssaultPoint->GetAbsAngles() );
		}
		break;

	case TASK_FACE_ASSAULT_POINT:
		{
			GetMotor()->SetIdealYaw( m_hAssaultPoint->GetAbsAngles().y );
			GetOuter()->SetTurnActivity(); 
		}
		break;

	case TASK_AWAIT_CUE:
		if( PollAssaultCue() )
		{
			TaskComplete();
		}
		else
		{
			// The cue hasn't been given yet, so set to the rally sequence, if any.
			if( m_hRallyPoint->m_RallySequenceName != NULL_STRING )
			{
				int sequence;

				sequence = GetOuter()->LookupSequence( STRING( m_hRallyPoint->m_RallySequenceName ) );

				if( sequence != -1 )
				{
					GetOuter()->ResetSequence( sequence );
					GetOuter()->SetIdealActivity( ACT_DO_NOT_DISTURB );
				}
			}
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT_ASSAULT_DELAY:
	case TASK_AWAIT_ASSAULT_TIMEOUT:
		if( gpGlobals->curtime > GetOuter()->m_flWaitFinished )
		{
			TaskComplete();
		}
		break;

	case TASK_FACE_RALLY_POINT:
	case TASK_FACE_ASSAULT_POINT:
		GetMotor()->UpdateYaw();

		if ( GetOuter()->FacingIdeal() )
		{
			if( pTask->iTask == TASK_FACE_RALLY_POINT )
			{
				// Once we're stading on it and facing the correct direction,
				// we have arrived at rally point.
				m_bHitRallyPoint = true;
				m_hRallyPoint->m_OnArrival.FireOutput( GetOuter(), m_hRallyPoint, 0 );
			}
			else
			{
				// same for assault.
				m_bHitAssaultPoint = true;
				m_hAssaultPoint->m_OnArrival.FireOutput( GetOuter(), m_hAssaultPoint, 0 );

				// Set the assault hint group
				SetHintGroup( m_hAssaultPoint->m_AssaultHintGroup );
			}

			TaskComplete();
		}
		break;

	case TASK_AWAIT_CUE:
		if( PollAssaultCue() )
		{
			TaskComplete();
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CRallyPoint *CAI_AssaultBehavior::FindBestRallyPointInRadius( const Vector &vecCenter, float flRadius )
{
	VPROF_BUDGET( "CAI_AssaultBehavior::FindBestRallyPointInRadius", VPROF_BUDGETGROUP_NPCS );

	const int RALLY_SEARCH_ENTS	= 30;
	CBaseEntity *pEntities[RALLY_SEARCH_ENTS];
	int iNumEntities = UTIL_EntitiesInSphere( pEntities, RALLY_SEARCH_ENTS, vecCenter, flRadius, 0 );

	CRallyPoint *pBest = NULL;
	int iBestPriority = -1;

	for ( int i = 0; i < iNumEntities; i++ )
	{
		CRallyPoint *pRallyEnt = dynamic_cast<CRallyPoint *>(pEntities[i]);

		if( pRallyEnt )
		{
			if( !pRallyEnt->IsLocked() )
			{
				// Consider this point.
				if( pRallyEnt->m_iPriority > iBestPriority )
				{
					pBest = pRallyEnt;
					iBestPriority = pRallyEnt->m_iPriority;
				}
			}
		}
	}

	return pBest;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pRallyPoint - 
//			assaultcue - 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::SetParameters( CBaseEntity *pRallyEnt, AssaultCue_t assaultcue )
{
	VPROF_BUDGET( "CAI_AssaultBehavior::SetParameters", VPROF_BUDGETGROUP_NPCS );

	// Firstly, find a rally point. 
	CRallyPoint *pRallyPoint = dynamic_cast<CRallyPoint *>(pRallyEnt);

	if( pRallyPoint )
	{
		if( !pRallyPoint->IsLocked() )
		{
			// Claim it.
			m_hRallyPoint = pRallyPoint;
			m_hRallyPoint->Lock( true );
			
			m_AssaultCue = assaultcue;
			InitializeBehavior();
			return;
		}
		else
		{
			Msg("**ERROR: Specified a rally point that is LOCKED!\n" );
		}
	}
	else
	{
		Msg("**ERROR: Bad RallyPoint in SetParameters\n" );

		// Bomb out of assault behavior.
		m_AssaultCue = CUE_NO_ASSAULT;
		GetOuter()->ClearSchedule();
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : rallypointname - 
//			assaultcue - 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::SetParameters( string_t rallypointname, AssaultCue_t assaultcue )
{
	VPROF_BUDGET( "CAI_AssaultBehavior::SetParameters", VPROF_BUDGETGROUP_NPCS );

	// Firstly, find a rally point. 
	CRallyPoint *pRallyEnt = dynamic_cast<CRallyPoint *>(gEntList.FindEntityByName( NULL, rallypointname, NULL ) );

	CRallyPoint *pBest = NULL;
	int iBestPriority = -1;

	while( pRallyEnt )
	{
		if( !pRallyEnt->IsLocked() )
		{
			// Consider this point.
			if( pRallyEnt->m_iPriority > iBestPriority )
			{
				pBest = pRallyEnt;
				iBestPriority = pRallyEnt->m_iPriority;
			}
		}

		pRallyEnt = dynamic_cast<CRallyPoint *>(gEntList.FindEntityByName( pRallyEnt, rallypointname, NULL ) );
	}

	if( !pBest )
	{
		Msg("Didn't find a best rally point!\n");
		return;
	}

	pBest->Lock( true );
	m_hRallyPoint = pBest;

	if( !m_hRallyPoint )
	{
		Msg("**ERROR: Can't find a rally point named '%s'\n", STRING( rallypointname ));

		// Bomb out of assault behavior.
		m_AssaultCue = CUE_NO_ASSAULT;
		GetOuter()->ClearSchedule();
		return;
	}

	m_AssaultCue = assaultcue;
	InitializeBehavior();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::InitializeBehavior()
{
	// initialize the variables that track whether the NPC has reached (hit)
	// his rally and assault points already. Be advised, having hit the point
	// only means you have been to it at some point. Doesn't mean you're standing
	// there still. Mainly used to understand which 'phase' of the assault an NPC
	// is in.
	m_bHitRallyPoint = false;
	m_bHitAssaultPoint = false;

	m_hAssaultPoint = false;

	// Also reset the status of externally received assault cues
	m_ReceivedAssaultCue = CUE_NO_ASSAULT;

	CAssaultPoint *pAssaultEnt = (CAssaultPoint *)gEntList.FindEntityByName( NULL, m_hRallyPoint->m_AssaultPointName, NULL );
	if( pAssaultEnt )
	{
		m_hAssaultPoint = pAssaultEnt;
	}
	else
	{
		Msg("**ERROR: Can't find assault point named: %s\n", STRING( m_hRallyPoint->m_AssaultPointName ));

		// Bomb out of assault behavior.
		m_AssaultCue = CUE_NO_ASSAULT;
		GetOuter()->ClearSchedule();
		return;
	}

	// Slam the NPC's schedule so that he starts picking Assault schedules right now.
	GetOuter()->ClearSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: Check conditions and see if the cue to being an assault has come.
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::PollAssaultCue( void )
{
	// right now, always go when the commander says.
	if( m_ReceivedAssaultCue == CUE_COMMANDER )
	{
		return true;
	}

	switch( m_AssaultCue )
	{
	case CUE_NO_ASSAULT:
		// NO_ASSAULT never ever triggers.
		return false;
		break;

	case CUE_ENTITY_INPUT:
		return m_ReceivedAssaultCue == CUE_ENTITY_INPUT;
		break;

	case CUE_PLAYER_GUNFIRE:
		// Any gunfire will trigger this right now (sjb)
		if( HasCondition( COND_HEAR_COMBAT ) )
		{
			return true;
		}
		break;

	case CUE_DONT_WAIT:
		// Just keep going!
		return true;
		break;

	case CUE_COMMANDER:
		// Player told me to go, so go!
		return m_ReceivedAssaultCue == CUE_COMMANDER;
		break;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_AssaultBehavior::CanSelectSchedule()
{
	if ( !GetOuter()->IsInterruptable() )
		return false;

	if ( GetOuter()->HasCondition( COND_RECEIVED_ORDERS ) )
	{
		return false;
	}

	// No schedule selection if no assault is being conducted.
	if( m_AssaultCue == CUE_NO_ASSAULT )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::BeginScheduleSelection()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultBehavior::EndScheduleSelection()
{
	m_bHitRallyPoint = false;
	m_bHitAssaultPoint = false;

	if( m_hRallyPoint )
	{
		m_hRallyPoint->Lock( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scheduleType - 
// Output : int
//-----------------------------------------------------------------------------
int CAI_AssaultBehavior::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_RANGE_ATTACK1:
		if ( !m_ShotRegulator.ShouldShoot() )				
		{
			return SCHED_IDLE_STAND; // @TODO (toml 07-02-03): Should do something more tactically sensible
		}
		break;

	case SCHED_CHASE_ENEMY:
		if( m_bHitAssaultPoint )
		{
			return SCHED_WAIT_AND_CLEAR;
		}
		else
		{
			return SCHED_MOVE_TO_ASSAULT_POINT;
		}
		break;

	case SCHED_HOLD_RALLY_POINT:
		if( HasCondition(COND_NO_PRIMARY_AMMO) | HasCondition(COND_LOW_PRIMARY_AMMO) )
		{
			return SCHED_RELOAD;
		}
		break;

	case SCHED_HIDE_AND_RELOAD: //!!!HACKHACK
		// Dirty the assault point flag so that we return to assault point
		m_bHitAssaultPoint = false;
		return BaseClass::TranslateSchedule( scheduleType );
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_AssaultBehavior::SelectSchedule()
{
	if( HasCondition( COND_PLAYER_PUSHING ) )
	{
		return SCHED_ASSAULT_MOVE_AWAY; 
	}

	if( m_bHitRallyPoint && !m_bHitAssaultPoint)
	{
		// If I have hit my rally point, but I haven't hit my assault point yet, 
		// Make sure I'm still on my rally point, cause another behavior may have moved me.
		// 2D check to be within 32 units of my rallypoint.
		Vector vecDiff = GetAbsOrigin() - m_hRallyPoint->GetAbsOrigin();
		float flMaxDistSqr = 32 * 32;
		vecDiff.z = 0.0;

		if( vecDiff.LengthSqr() > flMaxDistSqr )
		{
			// Someone moved me away. Get back to rally point.
			m_bHitRallyPoint = false;
			return SCHED_MOVE_TO_RALLY_POINT;
		}
	}
	else if( m_bHitAssaultPoint )
	{
		// Likewise. If I have hit my assault point, make sure I'm still there. Another
		// behavior (hide and reload) may have moved me away. 
		Vector vecDiff = GetAbsOrigin() - m_hAssaultPoint->GetAbsOrigin();
		float flMaxDistSqr = 32 * 32;
		vecDiff.z = 0.0;

		if( vecDiff.LengthSqr() > flMaxDistSqr )
		{
			// Someone moved me away.
			m_bHitAssaultPoint = false;
		}
	}

	if( !m_bHitRallyPoint )
	{
		return SCHED_MOVE_TO_RALLY_POINT;
	}

	if( !m_bHitAssaultPoint )
	{
		if( m_ReceivedAssaultCue == m_AssaultCue || m_ReceivedAssaultCue == CUE_COMMANDER )
		{
			return SCHED_MOVE_TO_ASSAULT_POINT;
		}
		else if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		{
			return SCHED_RANGE_ATTACK1;
		}
		else if( HasCondition( COND_NO_PRIMARY_AMMO ) )
		{
			// Don't run off to reload.
			return SCHED_RELOAD;
		}
		else if( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) )
		{
			return SCHED_ALERT_FACE;
		}
		else
		{
			return SCHED_HOLD_RALLY_POINT;
		}
	}

	if( HasCondition( COND_NO_PRIMARY_AMMO ) )
	{
		return SCHED_HIDE_AND_RELOAD;
	}

	if( GetEnemy() == NULL )
	{
		return SCHED_WAIT_AND_CLEAR;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//
// CAI_AssaultGoal
//
// Purpose: 
//			
//
//-----------------------------------------------------------------------------
class CAI_AssaultGoal : public CAI_GoalEntity
{
	typedef CAI_GoalEntity BaseClass;

	virtual void EnableGoal( CAI_BaseNPC *pAI );
	virtual void DisableGoal( CAI_BaseNPC *pAI );

	string_t		m_RallyPoint;
	int				m_AssaultCue;

	void InputBeginAssault( inputdata_t &inputdata );

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CAI_AssaultGoal )
	DEFINE_KEYFIELD( CAI_AssaultGoal, m_RallyPoint, FIELD_STRING, "rallypoint" ),
	DEFINE_KEYFIELD( CAI_AssaultGoal, m_AssaultCue, FIELD_INTEGER, "AssaultCue" ),

	DEFINE_INPUTFUNC( CAI_AssaultGoal, FIELD_VOID, "BeginAssault", InputBeginAssault ),
END_DATADESC();


//-------------------------------------
LINK_ENTITY_TO_CLASS( ai_goal_assault, CAI_AssaultGoal );
//-------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultGoal::EnableGoal( CAI_BaseNPC *pAI )
{
	CAI_AssaultBehavior *pBehavior;

	if ( !pAI->GetBehavior( &pBehavior ) )
	{
		return;
	}

	pBehavior->SetParameters( m_RallyPoint, (AssaultCue_t)m_AssaultCue );

	// Duplicate the output
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_AssaultGoal::DisableGoal( CAI_BaseNPC *pAI )
{ 
	CAI_AssaultBehavior *pBehavior;

	if ( pAI->GetBehavior( &pBehavior ) )
	{
		pBehavior->Disable();
	
		if( pBehavior->m_hRallyPoint )
		{
			// Don't leave any hanging rally points locked.
			pBehavior->m_hRallyPoint->Lock( false );
		}
	}

	pAI->ClearSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: ENTITY I/O method for telling the assault behavior to cue assault
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_AssaultGoal::InputBeginAssault( inputdata_t &inputdata )
{
	int i;

	for( i = 0 ; i < NumActors() ; i++ )
	{
		CAI_BaseNPC *pActor = GetActor( i );

		if( pActor )
		{
			// Now use this actor to lookup the Behavior
			CAI_AssaultBehavior *pBehavior;

			if( pActor->GetBehavior( &pBehavior ) )
			{
				// GOT IT! Now tell the behavior that entity i/o wants to cue the assault.
				pBehavior->ReceiveAssaultCue( CUE_ENTITY_INPUT );
			}
		}
	}
}


AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER(CAI_AssaultBehavior)

	DECLARE_TASK(TASK_GET_PATH_TO_RALLY_POINT)
	DECLARE_TASK(TASK_FACE_RALLY_POINT)
	DECLARE_TASK(TASK_GET_PATH_TO_ASSAULT_POINT)
	DECLARE_TASK(TASK_FACE_ASSAULT_POINT)
	DECLARE_TASK(TASK_AWAIT_CUE)
	DECLARE_TASK(TASK_AWAIT_ASSAULT_TIMEOUT)
	DECLARE_TASK(TASK_ANNOUNCE_CLEAR)
	DECLARE_TASK(TASK_WAIT_ASSAULT_DELAY)
	
	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_MOVE_TO_RALLY_POINT,

		"	Tasks"
		"		TASK_GET_PATH_TO_RALLY_POINT			0"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"		TASK_STOP_MOVING						0"
		"		TASK_FACE_RALLY_POINT					0"
		"		TASK_SET_SCHEDULE						SCHEDULE:SCHED_HOLD_RALLY_POINT"
		"	"
		"	Interrupts"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_HOLD_RALLY_POINT,

		"	Tasks"
		"		TASK_AWAIT_CUE							0"
		"		TASK_WAIT_ASSAULT_DELAY					0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PLAYER_PUSHING"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_HOLD_ASSAULT_POINT,

		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_WAIT					3"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_LOST_ENEMY"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_BULLET_IMPACT"
		"		COND_NO_PRIMARY_AMMO"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_MOVE_TO_ASSAULT_POINT,

		"	Tasks"
		"		TASK_GATHER_CONDITIONS					0"
		"		TASK_GET_PATH_TO_ASSAULT_POINT			0"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"		TASK_FACE_ASSAULT_POINT					0"
		"	"
		"	Interrupts"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_WAIT_AND_CLEAR,

		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_AWAIT_ASSAULT_TIMEOUT	0"
		"		TASK_ANNOUNCE_CLEAR			0"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_BETTER_WEAPON_AVAILABLE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_BULLET_IMPACT"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_ASSAULT_MOVE_AWAY,

		"	Tasks"
		"		TASK_MOVE_AWAY_PATH						100"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"	"
		"	Interrupts"
	)

AI_END_CUSTOM_SCHEDULE_PROVIDER()
