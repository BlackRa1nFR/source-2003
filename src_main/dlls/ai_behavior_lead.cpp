//========= Copyright © 2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"

#include "ai_behavior_lead.h"

#include "ai_goalentity.h"
#include "ai_navigator.h"
#include "ai_speech.h"

//-----------------------------------------------------------------------------
// class CAI_LeadBehavior
//
// Purpose:
//
//-----------------------------------------------------------------------------

const int GOAL_LEAD_MAX_PLAYER_LAG = 256;

//-----------------------------------------------------------------------------

void CAI_LeadBehavior::LeadPlayer( const AI_LeadArgs_t &leadArgs, CAI_LeadBehaviorHandler *pSink )
{
	if( SetGoal( leadArgs ) )
	{
		Connect( pSink );
		NotifyChangeBehaviorStatus();
	}
	else
	{
		Msg( "*** Warning! LeadPlayer() has a NULL Goal Ent\n" );
	}
}

//-------------------------------------

void CAI_LeadBehavior::StopLeading( void )
{
	ClearGoal();
	m_pSink = NULL;
	NotifyChangeBehaviorStatus();
}

//-------------------------------------

bool CAI_LeadBehavior::CanSelectSchedule()
{
	bool fNonCombat = ( GetNpcState() == NPC_STATE_IDLE || GetNpcState() == NPC_STATE_ALERT );
	return ( fNonCombat && HasGoal() );
}

//-------------------------------------

void CAI_LeadBehavior::BeginScheduleSelection()
{
	SetTarget( UTIL_PlayerByIndex( 1 ) );
	CAI_Expresser *pExpresser = GetOuter()->GetExpresser();
	if ( pExpresser )
		pExpresser->ClearSpokeConcept( CONCEPT_LEAD_ARRIVAL );
}

//-------------------------------------

bool CAI_LeadBehavior::SetGoal( const AI_LeadArgs_t &args )
{
	CBaseEntity *pGoalEnt;
	pGoalEnt = gEntList.FindEntityByName( NULL, args.pszGoal, NULL );
	
	if ( !pGoalEnt )
		return false;

	m_args 		= args;	// @Q (toml 08-13-02): need to copy string?
	m_goal 		= pGoalEnt->GetLocalOrigin();
	m_goalyaw 	= (args.flags & AILF_USE_GOAL_FACING) ? pGoalEnt->GetLocalAngles().y : -1;

	return true;
}

//-------------------------------------

void CAI_LeadBehavior::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if ( HasGoal() )
	{
		// We have to collect data about the person we're leading around.
		CBaseEntity *pFollower = UTIL_PlayerByIndex( 1 );

		if( pFollower )
		{
			float flFollowerDist;

			flFollowerDist = ( WorldSpaceCenter() - pFollower->WorldSpaceCenter() ).Length();
			
			if ( flFollowerDist < 64 )
				SetCondition( COND_LEAD_FOLLOWER_VERY_CLOSE );
			else
				ClearCondition( COND_LEAD_FOLLOWER_VERY_CLOSE );

			if( flFollowerDist > GOAL_LEAD_MAX_PLAYER_LAG )
			{
				SetCondition( COND_LEAD_FOLLOWER_LAGGING );
				ClearCondition( COND_LEAD_FOLLOWER_NOT_LAGGING );
			}
			else
			{
				SetCondition( COND_LEAD_FOLLOWER_NOT_LAGGING );
				ClearCondition( COND_LEAD_FOLLOWER_LAGGING );
			}


			// Now we want to see if the follower is lost. Being lost means
			// being (far away || out of LOS ) && some time has passed
			if( /*!HasCondition( COND_SEE_PLAYER ) ||*/ HasCondition( COND_LEAD_FOLLOWER_LAGGING ) )
			{
				if( m_LostTimer.IsRunning() )
				{
					if( m_LostTimer.Expired() )
					{
						SetCondition( COND_LEAD_FOLLOWER_LOST );
					}
				}
				else
				{
					m_LostTimer.Start();
				}
			}
			else
			{
				m_LostTimer.Stop();
				ClearCondition( COND_LEAD_FOLLOWER_LOST );
			}

			//Evaluate for success
			// Success right now means being stationary, close to the goal, and having the player close by
			if ( !( m_args.flags & AILF_NO_DEF_SUCCESS ) )
			{
				if( HasCondition( COND_LEAD_FOLLOWER_VERY_CLOSE )	&&
					(GetLocalOrigin() - m_goal).Length2D() <= 64 )
				{

					SetCondition( COND_LEAD_SUCCESS );
				}
				else
				{
					ClearCondition( COND_LEAD_SUCCESS );
				}
			}
		}
	}
}


//-------------------------------------

int CAI_LeadBehavior::SelectSchedule()
{
	if ( HasGoal() )
	{
		if( HasCondition(COND_LEAD_SUCCESS) )
		{
			return SCHED_LEAD_SUCCEED;
		}
		if( HasCondition( COND_LEAD_FOLLOWER_LOST ) )
		{
			Speak( CONCEPT_LEAD_COMING_BACK );
			return SCHED_LEAD_RETRIEVE;
		}
		if( HasCondition( COND_LEAD_FOLLOWER_LAGGING ) )
		{
			Speak( CONCEPT_LEAD_CATCHUP );
			return SCHED_LEAD_PAUSE;
		}
		else
		{
			return SCHED_LEAD_PLAYER;
		}
	}
	return BaseClass::SelectSchedule();
}

//-------------------------------------

void CAI_LeadBehavior::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_LEAD_FACE_GOAL:
		{
			if ( m_goalyaw != -1 )
			{
				GetMotor()->SetIdealYaw( m_goalyaw ); 
			}

			TaskComplete();
			break;
		}

		case TASK_LEAD_SUCCEED:
		{
			Speak( CONCEPT_LEAD_SUCCESS );
			NotifyEvent( LBE_SUCCESS );

			break;
		}

		case TASK_LEAD_ARRIVE:
		{
			Speak( CONCEPT_LEAD_ARRIVAL );
			NotifyEvent( LBE_ARRIVAL );
			
			break;
		}
		
		case TASK_STOP_LEADING:
		{
			ClearGoal();
			TaskComplete();
			break;
		}

		case TASK_GET_PATH_TO_LEAD_GOAL:
		{
			if ( GetNavigator()->SetGoal( m_goal ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail("NO PATH");
			}
			break;
		}
		
		default:
			BaseClass::StartTask( pTask);
	}
}

//-------------------------------------

void CAI_LeadBehavior::RunTask( const Task_t *pTask )		
{ 
	switch ( pTask->iTask )
	{
		case TASK_LEAD_SUCCEED:
		{
			if ( !IsSpeaking() )
			{
				TaskComplete();
				NotifyEvent( LBE_DONE );
			}
		}
		case TASK_LEAD_ARRIVE:
		{
			if ( !IsSpeaking() )
			{
				TaskComplete();
				NotifyEvent( LBE_ARRIVAL_DONE );
			}
			break;
		}

		default:
			BaseClass::RunTask( pTask);
	}
}

//-------------------------------------

bool CAI_LeadBehavior::Speak( AIConcept_t concept )
{
	CAI_Expresser *pExpresser = GetOuter()->GetExpresser();
	if ( !pExpresser )
		return false;
		
	return pExpresser->Speak( concept, GetConceptModifiers( concept ) );
}

//-------------------------------------

bool CAI_LeadBehavior::IsSpeaking()
{
	CAI_Expresser *pExpresser = GetOuter()->GetExpresser();
	if ( !pExpresser )
		return false;
		
	return pExpresser->IsSpeaking();
}

//-------------------------------------

bool CAI_LeadBehavior::Connect( CAI_LeadBehaviorHandler *pSink )
{
	m_pSink = pSink;
	return true;
}

//-------------------------------------

bool CAI_LeadBehavior::Disconnect( CAI_LeadBehaviorHandler *pSink )
{
	Assert( pSink == m_pSink );
	m_pSink = NULL;
	return true;
}

//-------------------------------------

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_LeadBehavior )

	DECLARE_CONDITION( COND_LEAD_FOLLOWER_LOST )
	DECLARE_CONDITION( COND_LEAD_FOLLOWER_LAGGING )
	DECLARE_CONDITION( COND_LEAD_FOLLOWER_NOT_LAGGING )
	DECLARE_CONDITION( COND_LEAD_FOLLOWER_VERY_CLOSE )
	DECLARE_CONDITION( COND_LEAD_SUCCESS )

	//---------------------------------
	//
	// Lead
	//
	DECLARE_TASK( TASK_GET_PATH_TO_LEAD_GOAL )
	DECLARE_TASK( TASK_STOP_LEADING )
	DECLARE_TASK( TASK_LEAD_ARRIVE )
	DECLARE_TASK( TASK_LEAD_SUCCEED )
	DECLARE_TASK( TASK_LEAD_FACE_GOAL )

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_RETRIEVE,

		"	Tasks"
		"		TASK_GET_PATH_TO_PLAYER			0"
		"		TASK_MOVE_TO_TARGET_RANGE		80"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT						0.5"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_LEAD_RETRIEVE_WAIT"
		"		"
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_VERY_CLOSE"
	)

	//-------------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_RETRIEVE_WAIT,

		"	Tasks"
		"		TASK_WAIT_INDEFINITE			0"
		"		"
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_LOST"
		"		COND_LEAD_FOLLOWER_LAGGING"
		"		COND_LEAD_FOLLOWER_VERY_CLOSE"
	)

	//---------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_PLAYER,

		"	Tasks"
		"		TASK_GET_PATH_TO_LEAD_GOAL	0"
		"		TASK_WALK_PATH				0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_STOP_MOVING			0"
		"		TASK_LEAD_FACE_GOAL			0"
		"		TASK_FACE_IDEAL				0"
		"		TASK_LEAD_ARRIVE			0"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_LEAD_AWAIT_SUCCESS"
		""
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_LAGGING"
	)


	//---------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_AWAIT_SUCCESS,

		"	Tasks"
		"		TASK_WAIT_INDEFINITE		0"
		""
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_LAGGING"
		"		COND_LEAD_SUCCESS"
	)

	//---------------------------------

	DEFINE_SCHEDULE
	(
		SCHED_LEAD_SUCCEED,

		"	Tasks"
		"		TASK_LEAD_SUCCEED			0"
		"		TASK_STOP_LEADING			0"
		""
	)

	//---------------------------------
	// This is the schedule Odell uses to pause the tour momentarily
	// if the player lags behind. If the player shows up in a 
	// couple of seconds, the tour will resume. Otherwise, Odell
	// moves to retrieve.
	//---------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_PAUSE,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_TARGET			0"
		"		TASK_WAIT					5"
		"		TASK_WAIT_RANDOM			5"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_LEAD_RETRIEVE"
		""
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_VERY_CLOSE"
		"		COND_LEAD_FOLLOWER_LOST"
	)

AI_END_CUSTOM_SCHEDULE_PROVIDER()


//-----------------------------------------------------------------------------
//
// Purpose: A level tool to control the lead behavior. Use is not required
//			in order to use behavior.
//

class CAI_LeadGoal : public CAI_GoalEntity,
					 public CAI_LeadBehaviorHandler
{
	DECLARE_CLASS( CAI_LeadGoal, CAI_GoalEntity );
public:
	CAI_LeadGoal()
	 :	m_fArrived( false )
	{
	}
	
private:

	CAI_LeadBehavior *GetLeadBehavior();

	virtual void OnEvent( int event );
	virtual const char *GetConceptModifiers( const char *pszConcept );
	virtual void InputActivate( inputdata_t &inputdata );
	virtual void InputDeactivate( inputdata_t &inputdata );
	void InputSetSuccess( inputdata_t &inputdata );
	void InputSetFailure( inputdata_t &inputdata );
	
	bool	 m_fArrived; // @TODO (toml 08-16-02): move arrived tracking onto behavior
	
	string_t m_iszArrivalConceptModifier;
	string_t m_iszPostArrivalConceptModifier;
	string_t m_iszSuccessConceptModifier;
	string_t m_iszFailureConceptModifier;

	// Output handlers
	COutputEvent	m_OnArrival;
	COutputEvent	m_OnArrivalDone;
	COutputEvent	m_OnSuccess;
	COutputEvent	m_OnFailure;
	COutputEvent	m_OnDone;
	
	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------
//
// CAI_LeadGoal implementation
//

LINK_ENTITY_TO_CLASS( ai_goal_lead, CAI_LeadGoal );

BEGIN_DATADESC( CAI_LeadGoal )

	DEFINE_FIELD( CAI_LeadGoal, m_fArrived, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD(CAI_LeadGoal, m_iszArrivalConceptModifier, 		FIELD_STRING, 	"ArrivalConceptModifier"),
	DEFINE_KEYFIELD(CAI_LeadGoal, m_iszPostArrivalConceptModifier,	FIELD_STRING,	"PostArrivalConceptModifier"),
	DEFINE_KEYFIELD(CAI_LeadGoal, m_iszSuccessConceptModifier,		FIELD_STRING,	"SuccessConceptModifier"),
	DEFINE_KEYFIELD(CAI_LeadGoal, m_iszFailureConceptModifier,		FIELD_STRING,	"FailureConceptModifier"),

	DEFINE_OUTPUT( CAI_LeadGoal, m_OnSuccess, 		"OnSuccess" ),
	DEFINE_OUTPUT( CAI_LeadGoal, m_OnArrival, 		"OnArrival" ),
	DEFINE_OUTPUT( CAI_LeadGoal, m_OnArrivalDone, 	"OnArrivalDone" ),
	DEFINE_OUTPUT( CAI_LeadGoal, m_OnFailure, 		"OnFailure" ),
	DEFINE_OUTPUT( CAI_LeadGoal, m_OnDone,	  		"OnDone" ),

	// Inputs
	DEFINE_INPUTFUNC( CAI_LeadGoal, FIELD_VOID, "SetSuccess", InputSetSuccess ),
	DEFINE_INPUTFUNC( CAI_LeadGoal, FIELD_VOID, "SetFailure", InputSetFailure ),

END_DATADESC()


//-----------------------------------------------------------------------------

CAI_LeadBehavior *CAI_LeadGoal::GetLeadBehavior()
{
	CAI_BaseNPC *pActor = GetActor();
	if ( !pActor )
		return NULL;

	CAI_LeadBehavior *pBehavior;
	if ( !pActor->GetBehavior( &pBehavior ) )
	{
		return NULL;
	}
	
	return pBehavior;
}

//-----------------------------------------------------------------------------

void CAI_LeadGoal::InputSetSuccess( inputdata_t &inputdata )
{
	CAI_LeadBehavior *pBehavior = GetLeadBehavior();
	if ( !pBehavior )
		return;
		
	// @TODO (toml 02-14-03): Hackly!
	pBehavior->SetCondition( CAI_LeadBehavior::COND_LEAD_SUCCESS);
}


//-----------------------------------------------------------------------------

void CAI_LeadGoal::InputSetFailure( inputdata_t &inputdata )
{
	Msg( "SetFailure unimplemented\n" );
}


//-----------------------------------------------------------------------------

void CAI_LeadGoal::InputActivate( inputdata_t &inputdata )
{
	BaseClass::InputActivate( inputdata );
	
	CAI_LeadBehavior *pBehavior = GetLeadBehavior();
	if ( !pBehavior )
	{
		Msg( "Lead goal entity activated for an NPC that doesn't have the lead behavior\n" );
		return;
	}
	
	AI_LeadArgs_t leadArgs = { GetGoalEntityName(), m_spawnflags };
	
	pBehavior->LeadPlayer( leadArgs, this );
}

//-----------------------------------------------------------------------------

void CAI_LeadGoal::InputDeactivate( inputdata_t &inputdata )
{
	BaseClass::InputDeactivate( inputdata );

	CAI_LeadBehavior *pBehavior = GetLeadBehavior();
	if ( !pBehavior )
		return;
		
	// @TODO (toml 02-19-03)
}

//-----------------------------------------------------------------------------

void CAI_LeadGoal::OnEvent( int event )
{
	COutputEvent *pOutputEvent = NULL;

	switch ( event )
	{
		case LBE_ARRIVAL:		pOutputEvent = &m_OnArrival;		break;
		case LBE_ARRIVAL_DONE:	pOutputEvent = &m_OnArrivalDone;	break;
		case LBE_SUCCESS:		pOutputEvent = &m_OnSuccess;		break;
		case LBE_FAILURE:		pOutputEvent = &m_OnFailure;		break;
		case LBE_DONE:			pOutputEvent = &m_OnDone;			break;
	}
	
	// @TODO (toml 08-16-02): move arrived tracking onto behavior
	if ( event == LBE_ARRIVAL )
		m_fArrived = true;

	if ( pOutputEvent )
		pOutputEvent->FireOutput( this, this );
}

//-----------------------------------------------------------------------------

const char *CAI_LeadGoal::GetConceptModifiers( const char *pszConcept )	
{ 
	if ( m_iszArrivalConceptModifier != NULL_STRING && *STRING(m_iszArrivalConceptModifier) && strcmp( pszConcept, CONCEPT_LEAD_ARRIVAL) == 0 )
		return STRING( m_iszArrivalConceptModifier );
		
	if ( m_iszSuccessConceptModifier != NULL_STRING && *STRING(m_iszSuccessConceptModifier) && strcmp( pszConcept, CONCEPT_LEAD_SUCCESS) == 0 )
		return STRING( m_iszSuccessConceptModifier );
		
	if (m_iszFailureConceptModifier != NULL_STRING && *STRING(m_iszFailureConceptModifier) && strcmp( pszConcept, CONCEPT_LEAD_FAILURE) == 0 )
		return STRING( m_iszFailureConceptModifier );
		
	if ( m_fArrived && m_iszPostArrivalConceptModifier != NULL_STRING && *STRING(m_iszPostArrivalConceptModifier) )
		return STRING( m_iszPostArrivalConceptModifier );
	
	return NULL; 
}


//-----------------------------------------------------------------------------
