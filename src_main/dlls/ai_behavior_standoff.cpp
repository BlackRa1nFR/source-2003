//============= Copyright © 2003, Valve LLC, All rights reserved. =============
//
// Purpose: Combat behaviors for AIs in a relatively self-preservationist mode.
//			Lots of cover taking and attempted shots out of cover.
//
//=============================================================================

#include "cbase.h"

#include "ai_hint.h"
#include "ai_node.h"
#include "ai_navigator.h"
#include "ai_tacticalservices.h"
#include "ai_behavior_standoff.h"
#include "ai_senses.h"
#include "ai_squad.h"
#include "ai_goalentity.h"
#include "ndebugoverlay.h"

#define GOAL_POSITION_INVALID	Vector( FLT_MAX, FLT_MAX, FLT_MAX )

ConVar DrawBattleLines( "ai_drawbattlelines", "0", FCVAR_CHEAT );

static AI_StandoffParams_t AI_DEFAULT_STANDOFF_PARAMS = { AIHCR_MOVE_ON_COVER, true, 1.5, 2.5, 1, 3, 25 };

Activity ACT_RANGE_AIM_SMG1_LOW;
Activity ACT_RANGE_AIM_PISTOL_LOW;
Activity ACT_RANGE_AIM_AR2_LOW;

#define MAKE_ACTMAP_KEY( posture, activity ) ( (((unsigned)(posture)) << 16) | ((unsigned)(activity)) )


#ifdef DEBUG_STANDOFF
#define StandoffMsg( msg ) 					Msg( GetOuter(), msg )
#define StandoffMsg1( msg, a ) 				Msg( GetOuter(), msg, a )
#define StandoffMsg2( msg, a, b ) 			Msg( GetOuter(), msg, a, b )
#define StandoffMsg3( msg, a, b, c ) 		Msg( GetOuter(), msg, a, b, c )
#define StandoffMsg4( msg, a, b, c, d ) 	Msg( GetOuter(), msg, a, b, c, d )
#define StandoffMsg5( msg, a, b, c, d, e )	Msg( GetOuter(), msg, a, b, c, d, e )
#else
#define StandoffMsg( msg ) 					((void)0)
#define StandoffMsg1( msg, a ) 				((void)0)
#define StandoffMsg2( msg, a, b ) 			((void)0)
#define StandoffMsg3( msg, a, b, c ) 		((void)0)
#define StandoffMsg4( msg, a, b, c, d ) 	((void)0)
#define StandoffMsg5( msg, a, b, c, d, e )	((void)0)
#endif

//-----------------------------------------------------------------------------
//
// CAI_BattleLine
//
//-----------------------------------------------------------------------------

const float AIBL_THINK_INTERVAL = 0.3;

class CAI_BattleLine : public CBaseEntity
{
	DECLARE_CLASS( CAI_BattleLine, CBaseEntity );

public:
	string_t		m_iszActor;
	bool			m_fActive;
	bool			m_fStrict;

	void Spawn()
	{
		if ( m_fActive )
		{
			SetThink(&CAI_BattleLine::MovementThink);
			SetNextThink( gpGlobals->curtime + AIBL_THINK_INTERVAL );
			m_SelfMoveMonitor.SetMark( this, 60 );
		}
	}

	virtual void InputActivate( inputdata_t &inputdata )		
	{ 
		if ( !m_fActive )
		{
			m_fActive = true; 
			NotifyChangeTacticalConstraints(); 

			SetThink(&CAI_BattleLine::MovementThink);
			SetNextThink( gpGlobals->curtime + AIBL_THINK_INTERVAL );
			m_SelfMoveMonitor.SetMark( this, 60 );
		}
	}
	
	virtual void InputDeactivate( inputdata_t &inputdata )	
	{ 
		if ( m_fActive )
		{
			m_fActive = false; 
			NotifyChangeTacticalConstraints(); 

			SetThink(NULL);
		}
	}
	
	void UpdateOnRemove()
	{
		if ( m_fActive )
		{
			m_fActive = false; 
			NotifyChangeTacticalConstraints(); 
		}
		BaseClass::UpdateOnRemove();
	}

	bool Affects( CAI_BaseNPC *pNpc )
	{
		const char *pszNamedActor = STRING( m_iszActor );

		if ( pNpc->NameMatches( pszNamedActor ) ||
			 pNpc->ClassMatches( pszNamedActor ) ||
			 ( pNpc->GetSquad() && stricmp( pNpc->GetSquad()->GetName(), pszNamedActor ) == 0 ) )
		{
			return true;
		}
		return false;
	}
	
	void MovementThink()
	{
		if ( m_SelfMoveMonitor.TargetMoved( this ) )
		{
			NotifyChangeTacticalConstraints();
			m_SelfMoveMonitor.SetMark( this, 60 );
		}
		SetNextThink( gpGlobals->curtime + AIBL_THINK_INTERVAL );
	}

private:
	void NotifyChangeTacticalConstraints()
	{
		for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			CAI_BaseNPC *pNpc = (g_AI_Manager.AccessAIs())[i];
			if ( Affects( pNpc ) )
			{
				CAI_StandoffBehavior *pBehavior;
				if ( pNpc->GetBehavior( &pBehavior ) )
				{
					pBehavior->OnChangeTacticalConstraints();
				}
			}
		}
	}

	CAI_MoveMonitor m_SelfMoveMonitor;
	
	DECLARE_DATADESC();
};

//-------------------------------------

LINK_ENTITY_TO_CLASS( ai_battle_line, CAI_BattleLine );

BEGIN_DATADESC( CAI_BattleLine )
	DEFINE_KEYFIELD(	CAI_BattleLine, m_iszActor,				FIELD_STRING, 	"Actor"					),
	DEFINE_KEYFIELD(	CAI_BattleLine, m_fActive,				FIELD_BOOLEAN,  "Active"				),
	DEFINE_KEYFIELD(	CAI_BattleLine, m_fStrict,				FIELD_BOOLEAN,  "Strict"				),
	DEFINE_EMBEDDED( 	CAI_BattleLine, m_SelfMoveMonitor ),

	// Inputs
	DEFINE_INPUTFUNC( CAI_BattleLine, FIELD_VOID, "Activate", 		InputActivate ),
	DEFINE_INPUTFUNC( CAI_BattleLine, FIELD_VOID, "Deactivate",		InputDeactivate ),
	
	DEFINE_THINKFUNC( CAI_BattleLine, MovementThink ),

END_DATADESC()


//-----------------------------------------------------------------------------
//
// CAI_StandoffBehavior
//
//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( AI_StandoffParams_t )
	DEFINE_FIELD( AI_StandoffParams_t, hintChangeReaction, FIELD_INTEGER ),
	DEFINE_FIELD( AI_StandoffParams_t, fPlayerIsBattleline, FIELD_BOOLEAN ),
	DEFINE_FIELD( AI_StandoffParams_t, fCoverOnReload, FIELD_BOOLEAN ),
	DEFINE_FIELD( AI_StandoffParams_t, minTimeShots, FIELD_FLOAT ),
	DEFINE_FIELD( AI_StandoffParams_t, maxTimeShots, FIELD_FLOAT ),
	DEFINE_FIELD( AI_StandoffParams_t, minShots, FIELD_INTEGER ),
	DEFINE_FIELD( AI_StandoffParams_t, maxShots, FIELD_INTEGER ),
	DEFINE_FIELD( AI_StandoffParams_t, oddsCover, FIELD_INTEGER ),
	DEFINE_FIELD( AI_StandoffParams_t, fStayAtCover, FIELD_BOOLEAN ),
END_DATADESC();

BEGIN_DATADESC( CAI_StandoffBehavior )
	DEFINE_FIELD( 		CAI_StandoffBehavior, 	m_fActive, 						FIELD_BOOLEAN ),
	DEFINE_FIELD(		CAI_StandoffBehavior,	m_fTestNoDamage,				FIELD_BOOLEAN ),
	DEFINE_FIELD( 		CAI_StandoffBehavior, 	m_vecStandoffGoalPosition, 		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( 		CAI_StandoffBehavior, 	m_posture, 						FIELD_INTEGER ),
	DEFINE_EMBEDDED(	CAI_StandoffBehavior, 	m_params ),
	DEFINE_EMBEDDED( 	CAI_StandoffBehavior, 	m_ShotRegulator ),
	DEFINE_FIELD( 		CAI_StandoffBehavior, 	m_fTakeCover, 					FIELD_BOOLEAN ),
	DEFINE_FIELD( 		CAI_StandoffBehavior, 	m_SavedDistTooFar, 				FIELD_FLOAT ),
	DEFINE_FIELD( 		CAI_StandoffBehavior, 	m_fForceNewEnemy, 				FIELD_BOOLEAN ),
	DEFINE_EMBEDDED( 	CAI_StandoffBehavior, 	m_PlayerMoveMonitor ),
	DEFINE_EMBEDDED( 	CAI_StandoffBehavior, 	m_TimeForceCoverHint ),
	DEFINE_EMBEDDED( 	CAI_StandoffBehavior, 	m_TimePreventForceNewEnemy ),
	DEFINE_EMBEDDED( 	CAI_StandoffBehavior, 	m_RandomCoverChangeTimer ),
	// 											m_UpdateBattleLinesSemaphore 	(not saved, only an in-think item)
	// 											m_BattleLines 					(not saved, rebuilt)
	DEFINE_FIELD( 		CAI_StandoffBehavior, 	m_fIgnoreFronts, 				FIELD_BOOLEAN ),
	//											m_ActivityMap 					(not saved, rebuilt)
END_DATADESC();

//-------------------------------------

CAI_StandoffBehavior::CAI_StandoffBehavior( CAI_BaseNPC *pOuter )
 :	CAI_SimpleBehavior( pOuter )
{
	m_fActive = false;
	SetParameters( AI_DEFAULT_STANDOFF_PARAMS );
	SetPosture( AIP_INDIFFERENT );
	m_SavedDistTooFar = FLT_MAX;
	m_fForceNewEnemy = false;
	m_TimePreventForceNewEnemy.Set( 3.0, 6.0 );
	m_fIgnoreFronts = false;

	SetDefLessFunc( m_ActivityMap );
}

//-------------------------------------

void CAI_StandoffBehavior::SetActive( bool fActive )
{
	if ( fActive !=	m_fActive )
	{
		m_fActive = fActive;
		NotifyChangeBehaviorStatus();
	}
}

//-------------------------------------

void CAI_StandoffBehavior::SetParameters( const AI_StandoffParams_t &params )
{
	m_params = params;
	m_ShotRegulator.SetParameters( m_params.minShots, m_params.maxShots, params.minTimeShots, params.maxTimeShots );
	m_vecStandoffGoalPosition = GOAL_POSITION_INVALID;
}

//-------------------------------------

bool CAI_StandoffBehavior::CanSelectSchedule()
{
	if ( !m_fActive )
		return false;

	if ( !( CapabilitiesGet() & bits_CAP_DUCK ) || !HaveSequenceForActivity( ACT_COVER_LOW ) )
	{
		m_fActive = false;
		return false;
	}

	return ( GetNpcState() == NPC_STATE_COMBAT && GetOuter()->GetActiveWeapon() != NULL );
}

//-------------------------------------

void CAI_StandoffBehavior::BeginScheduleSelection()
{
	m_fTakeCover = true;

	m_ShotRegulator.Reset();

	m_SavedDistTooFar = GetOuter()->m_flDistTooFar;
	GetOuter()->m_flDistTooFar = FLT_MAX;
	
	m_TimeForceCoverHint.Set( 8, false );
	m_RandomCoverChangeTimer.Set( 8, 16, false );
	UpdateTranslateActivityMap();
}

//-------------------------------------

void CAI_StandoffBehavior::EndScheduleSelection()
{
	UnlockHintNode();

	m_vecStandoffGoalPosition = GOAL_POSITION_INVALID;
	GetOuter()->m_flDistTooFar = m_SavedDistTooFar;
}

//-------------------------------------

void CAI_StandoffBehavior::PrescheduleThink()
{
	VPROF_BUDGET( "CAI_StandoffBehavior::PrescheduleThink", VPROF_BUDGETGROUP_NPCS );

	BaseClass::PrescheduleThink();
	
	if( DrawBattleLines.GetInt() != 0 )
	{
		CBaseEntity *pEntity = NULL;
		while ((pEntity = gEntList.FindEntityByClassname( pEntity, "ai_battle_line" )) != NULL)
		{
			// Visualize the battle line and its normal.
			CAI_BattleLine *pLine = dynamic_cast<CAI_BattleLine *>(pEntity);

			if( pLine->m_fActive )
			{
				Vector normal;

				pLine->GetVectors( &normal, NULL, NULL );

				NDebugOverlay::Line( pLine->GetAbsOrigin() - Vector( 0, 0, 64 ), pLine->GetAbsOrigin() + Vector(0,0,64), 0,255,0, false, 0.1 );
			}
		}
	}
}

//-------------------------------------

void CAI_StandoffBehavior::GatherConditions()
{
	CBaseEntity *pLeader = GetPlayerLeader();
	if ( pLeader && m_TimeForceCoverHint.Expired() )
	{
		if ( m_PlayerMoveMonitor.IsMarkSet() )
		{
			if (m_PlayerMoveMonitor.TargetMoved( pLeader ) )
			{
				OnChangeTacticalConstraints();
				m_PlayerMoveMonitor.ClearMark();
			}
		}
		else
		{
			m_PlayerMoveMonitor.SetMark( pLeader, 60 );
		}
	}

	if ( m_fForceNewEnemy )
	{
		m_TimePreventForceNewEnemy.Reset();
		GetOuter()->SetEnemy( NULL );
	}
	BaseClass::GatherConditions();
	m_fForceNewEnemy = false;
	
}

//-------------------------------------

int CAI_StandoffBehavior::SelectScheduleUpdateWeapon( void )
{
	// Check if need to reload
	if ( HasCondition ( COND_NO_PRIMARY_AMMO ) || HasCondition ( COND_LOW_PRIMARY_AMMO ))
	{
		if ( m_params.fCoverOnReload )
			return SCHED_HIDE_AND_RELOAD;
		else
			return SCHED_RELOAD;
	}
	
	// Otherwise, update planned shots to fire before taking cover again
	if ( HasCondition( COND_LIGHT_DAMAGE ) )
	{
		// if hurt:
		int iPercent = random->RandomInt(0,99);

		if ( iPercent <= m_params.oddsCover && GetEnemy() != NULL )
		{
			SetReuseCurrentCover();
			m_ShotRegulator.SetShots( ( m_ShotRegulator.GetShots() > 1 ) ? 1 : 0 );
		}
	}

	m_ShotRegulator.Update();

	return SCHED_NONE;
}

//-------------------------------------

int CAI_StandoffBehavior::SelectScheduleCheckCover( void )
{
	if ( m_fTakeCover )
	{
		m_fTakeCover = false;
		if ( GetEnemy() )
			return SCHED_TAKE_COVER_FROM_ENEMY;
	}
		
	if ( !m_ShotRegulator.ShouldShoot() )
	{
		if ( GetHintType() == HINT_TACTICAL_COVER_LOW )
			SetPosture( AIP_CROUCHING );
		else
			SetPosture( AIP_INDIFFERENT );

		if ( random->RandomInt(0,99) < 80 )
			SetReuseCurrentCover();
		return SCHED_TAKE_COVER_FROM_ENEMY;
	}
	
	return SCHED_NONE;
}

//-------------------------------------

int CAI_StandoffBehavior::SelectScheduleEstablishAim( void )
{
	if ( HasCondition( COND_ENEMY_OCCLUDED ) )
	{
		if ( GetPosture() == AIP_CROUCHING )
		{
			// force a stand up, just in case
			SetPosture( AIP_PEEKING );
			return SCHED_STANDOFF;
		}
		else if ( GetPosture() == AIP_PEEKING )
		{
			if ( m_TimePreventForceNewEnemy.Expired() )
				// Look for a new enemy
				m_fForceNewEnemy = true;
		}
#if 0
		else
		{
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		}
#endif
	}

	return SCHED_NONE;
}

//-------------------------------------

int CAI_StandoffBehavior::SelectScheduleAttack( void )
{
	if ( GetPosture() == AIP_PEEKING || GetPosture() == AIP_STANDING )
	{
		if ( !HasCondition( COND_CAN_RANGE_ATTACK1 ) && 
			 !HasCondition( COND_CAN_MELEE_ATTACK1 ) &&
			  HasCondition( COND_TOO_FAR_TO_ATTACK ) )
		{
			if ( !HasCondition( COND_ENEMY_OCCLUDED ) || random->RandomInt(0,99) < 50 )
				// Don't advance, just fire anyway
				return SCHED_RANGE_ATTACK1;
		}
	}

	return SCHED_NONE;
}

//-------------------------------------

int CAI_StandoffBehavior::SelectSchedule( void )
{
	switch ( GetNpcState() )
	{
		case NPC_STATE_COMBAT:
		{
			int schedule = SCHED_NONE;
			
			schedule = SelectScheduleUpdateWeapon();
			if ( schedule != SCHED_NONE )
				return schedule;
				
			schedule = SelectScheduleCheckCover();
			if ( schedule != SCHED_NONE )
				return schedule;
				
			schedule = SelectScheduleEstablishAim();
			if ( schedule != SCHED_NONE )
				return schedule;
				
			schedule = SelectScheduleAttack();
			if ( schedule != SCHED_NONE )
				return schedule;
				
			break;
		}
	}

	return BaseClass::SelectSchedule();
}

//-------------------------------------

int CAI_StandoffBehavior::TranslateSchedule( int schedule )
{
	if ( schedule == SCHED_CHASE_ENEMY )
	{
		return SCHED_ESTABLISH_LINE_OF_FIRE;
	}
	return BaseClass::TranslateSchedule( schedule );
}

//-------------------------------------

void CAI_StandoffBehavior::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if ( GetOuter()->IsCurSchedule( SCHED_TAKE_COVER_FROM_ENEMY ) )
		GetOuter()->ClearCustomInterruptCondition( COND_NEW_ENEMY );
}

//-------------------------------------

Activity CAI_StandoffBehavior::NPC_TranslateActivity( Activity activity )
{
	Activity coverActivity = GetCoverActivity();
	if ( coverActivity != ACT_INVALID )
	{
		if ( activity == ACT_IDLE )
			activity = coverActivity;
		if ( GetPosture() == AIP_INDIFFERENT && coverActivity == ACT_COVER_LOW )
			SetPosture( AIP_CROUCHING );
	}
	
	if ( GetPosture() != AIP_INDIFFERENT )
	{
		unsigned short iActivityTranslation = m_ActivityMap.Find( MAKE_ACTMAP_KEY( GetPosture(), activity ) );
		if ( iActivityTranslation != m_ActivityMap.InvalidIndex() )
		{
			Activity result = m_ActivityMap[iActivityTranslation];
			return result;
		}
	}

	return BaseClass::NPC_TranslateActivity( activity );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecPos - 
//-----------------------------------------------------------------------------
void CAI_StandoffBehavior::SetStandoffGoalPosition( const Vector &vecPos )
{
	m_vecStandoffGoalPosition = vecPos;
	UpdateBattleLines();
	OnChangeTacticalConstraints();
	GetOuter()->ClearSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecPos - 
//-----------------------------------------------------------------------------
void CAI_StandoffBehavior::ClearStandoffGoalPosition()
{
	if ( m_vecStandoffGoalPosition != GOAL_POSITION_INVALID )
	{
		m_vecStandoffGoalPosition = GOAL_POSITION_INVALID;
		UpdateBattleLines();
		OnChangeTacticalConstraints();
		GetOuter()->ClearSchedule();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CAI_StandoffBehavior::GetStandoffGoalPosition() 
{
	if( m_vecStandoffGoalPosition != GOAL_POSITION_INVALID )
	{
		return m_vecStandoffGoalPosition;
	}
	else if( PlayerIsLeading() )
	{
		return UTIL_PlayerByIndex( 1 )->GetAbsOrigin();
	}
	else
	{
		CAI_BattleLine *pBattleLine = NULL;
		for (;;)
		{
			pBattleLine = (CAI_BattleLine *)gEntList.FindEntityByClassname( pBattleLine, "ai_battle_line" );
			
			if ( !pBattleLine )
				break;
				
			if ( pBattleLine->m_fActive && pBattleLine->Affects( GetOuter() ) )
			{
				StandoffMsg1( "Using battleline %s as goal\n", STRING( pBattleLine->GetEntityName() ) );
				return pBattleLine->GetAbsOrigin();
			}
		}
	}

	return GetAbsOrigin();
}

//-------------------------------------

void CAI_StandoffBehavior::UpdateBattleLines()
{
	if ( m_UpdateBattleLinesSemaphore.EnterThink() )
	{
		// @TODO (toml 06-19-03): This is the quick to code thing. Could use some optimization/caching to not recalc everything (up to) each think
		m_BattleLines.RemoveAll();

		bool bHaveGoalPosition = ( m_vecStandoffGoalPosition != GOAL_POSITION_INVALID );

		if ( bHaveGoalPosition )
		{
			// If we have a valid standoff goal position, it takes precendence.
			const float DIST_GOAL_PLANE = 180;
			
			BattleLine_t goalLine;

			if ( GetDirectionOfStandoff( &goalLine.normal ) )
			{
				goalLine.point = GetStandoffGoalPosition() + goalLine.normal * DIST_GOAL_PLANE;
				m_BattleLines.AddToTail( goalLine );
			}
		}
		else if ( PlayerIsLeading() && GetEnemy() )
		{
			if ( m_params.fPlayerIsBattleline )
			{
				const float DIST_PLAYER_PLANE = 180;
				CBaseEntity *pPlayer = UTIL_PlayerByIndex( 1 );
				
				BattleLine_t playerLine;

				if ( GetDirectionOfStandoff( &playerLine.normal ) )
				{
					playerLine.point = pPlayer->GetAbsOrigin() + playerLine.normal * DIST_PLAYER_PLANE;
					m_BattleLines.AddToTail( playerLine );
				}
			}
		}
		
		CAI_BattleLine *pBattleLine = NULL;
		for (;;)
		{
			pBattleLine = (CAI_BattleLine *)gEntList.FindEntityByClassname( pBattleLine, "ai_battle_line" );
			
			if ( !pBattleLine )
				break;
				
			if ( pBattleLine->m_fActive && (!bHaveGoalPosition || !pBattleLine->m_fStrict ) && pBattleLine->Affects( GetOuter() ) )
			{
				BattleLine_t battleLine;
				
				battleLine.point = pBattleLine->GetAbsOrigin();
				battleLine.normal = UTIL_YawToVector( pBattleLine->GetAbsAngles().y );

				m_BattleLines.AddToTail( battleLine );
			}
				
		}
	}
}

//-------------------------------------

bool CAI_StandoffBehavior::IsBehindBattleLines( const Vector &point )
{
	UpdateBattleLines();

	Vector vecToPoint;
	
	for ( int i = 0; i < m_BattleLines.Count(); i++ )
	{
		vecToPoint = point - m_BattleLines[i].point;
		VectorNormalize( vecToPoint );
		vecToPoint.z = 0;
		
		if ( DotProduct( m_BattleLines[i].normal, vecToPoint ) > 0 )
			return false;
	}
	
	return true;
}

//-------------------------------------

bool CAI_StandoffBehavior::IsValidCover( const Vector &vecCoverLocation, const CAI_Hint *pHint )
{
	if ( !BaseClass::IsValidCover( vecCoverLocation, pHint ) )
		return false;

	return ( m_fIgnoreFronts || IsBehindBattleLines( vecCoverLocation ) );
}

//-------------------------------------

bool CAI_StandoffBehavior::IsValidShootPosition( const Vector &vLocation, const CAI_Hint *pHint )
{
	if ( !BaseClass::IsValidShootPosition( vLocation, pHint ) )
		return false;

	return ( m_fIgnoreFronts || IsBehindBattleLines( vLocation ) );
}

//-------------------------------------

void CAI_StandoffBehavior::StartTask( const Task_t *pTask )
{
	bool fCallBase = false;
	
	switch ( pTask->iTask )
	{
		case TASK_RANGE_ATTACK1:
		{
			m_ShotRegulator.OnFiredWeapon();
			fCallBase = true;
			break;
		}
		
		case TASK_FIND_COVER_FROM_ENEMY:
		{
			StandoffMsg( "TASK_FIND_COVER_FROM_ENEMY\n" );

			// If within time window to force change
			if ( !m_params.fStayAtCover && (!m_TimeForceCoverHint.Expired() || m_RandomCoverChangeTimer.Expired()) )
			{
				m_TimeForceCoverHint.Force();
				m_RandomCoverChangeTimer.Set( 8, 16, false );

				// @TODO (toml 03-24-03):  clean this up be tool-izing base tasks. Right now, this is here to force to not use lateral cover search
				CBaseEntity *pEntity = GetEnemy();

				if ( pEntity == NULL )
				{
					// Find cover from self if no enemy available
					pEntity = GetOuter();
				}

				CBaseEntity *pLeader = GetPlayerLeader();
				if ( pLeader )
				{
					m_PlayerMoveMonitor.SetMark( pLeader, 60 );
				}

				Vector					coverPos			= vec3_origin;
				CAI_TacticalServices *	pTacticalServices	= GetTacticalServices();
				const Vector &			enemyPos			= pEntity->GetAbsOrigin();
				Vector					enemyEyePos			= pEntity->EyePosition();
				float					coverRadius			= GetOuter()->CoverRadius();
				const Vector &			goalPos				= GetStandoffGoalPosition();
				bool					bTryGoalPosFirst	= true;

				if( pLeader && m_vecStandoffGoalPosition == GOAL_POSITION_INVALID )
				{
					if( random->RandomInt(1, 100) <= 50 )
					{
						// Half the time, if the player is leading, try to find a spot near them
						bTryGoalPosFirst = false;
						StandoffMsg( "Not trying goal pos\n" );
					}
				}

				if( bTryGoalPosFirst )
				{
					// Firstly, try to find cover near the goal position.
					pTacticalServices->FindCoverPos( goalPos, enemyPos, enemyEyePos, 0, 15*12, &coverPos );

					if ( coverPos == vec3_origin )
						pTacticalServices->FindCoverPos( goalPos, enemyPos, enemyEyePos, 15*12-0.1, 40*12, &coverPos );

					StandoffMsg1( "Trying goal pos, %s\n", ( coverPos == vec3_origin  ) ? "failed" :  "succeeded" );
				}

				if ( coverPos == vec3_origin  ) 
				{
					// Otherwise, find a node near to self
					StandoffMsg( "Looking for near cover\n" );
					if ( !GetTacticalServices()->FindCoverPos( enemyPos, enemyEyePos, 0, coverRadius, &coverPos ) ) 
					{
						// Try local lateral cover
						if ( !GetTacticalServices()->FindLateralCover( enemyEyePos, &coverPos ) )
						{
							// At this point, try again ignoring front lines. Any cover probably better than hanging out in the open
							m_fIgnoreFronts = true;
							if ( !GetTacticalServices()->FindCoverPos( enemyPos, enemyEyePos, 0, coverRadius, &coverPos ) ) 
							{
								if ( !GetTacticalServices()->FindLateralCover( enemyEyePos, &coverPos ) )
								{
									Assert( coverPos == vec3_origin );
								}
							}
							m_fIgnoreFronts = false;
						}
					}
				}

				if ( coverPos != vec3_origin )
				{
					AI_NavGoal_t goal(GOALTYPE_COVER, coverPos, ACT_RUN, AIN_HULL_TOLERANCE, AIN_DEF_FLAGS);
					GetNavigator()->SetGoal( goal );

					GetOuter()->m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
					TaskComplete();
				}
				else
					TaskFail(FAIL_NO_COVER);
			}
			else
			{
				fCallBase = true;
			}
			break;
		}

		default:
		{
			fCallBase = true;
		}
	}
	
	if ( fCallBase )
		BaseClass::StartTask( pTask );
}

//-------------------------------------

void CAI_StandoffBehavior::OnChangeHintGroup( string_t oldGroup, string_t newGroup )
{
	OnChangeTacticalConstraints();
}

//-------------------------------------

void CAI_StandoffBehavior::OnChangeTacticalConstraints()
{
	if ( m_params.hintChangeReaction > AIHCR_DEFAULT_AI )
		m_TimeForceCoverHint.Set( 8.0, false );
	if ( m_params.hintChangeReaction == AIHCR_MOVE_IMMEDIATE )
		m_fTakeCover = true;
}

//-------------------------------------

bool CAI_StandoffBehavior::PlayerIsLeading()
{
	CBaseEntity *pPlayer = UTIL_PlayerByIndex( 1 );
	return ( pPlayer && GetOuter()->IRelationType( pPlayer ) == D_LI );
}

//-------------------------------------

CBaseEntity *CAI_StandoffBehavior::GetPlayerLeader()
{
	CBaseEntity *pPlayer = UTIL_PlayerByIndex( 1 );
	if ( pPlayer && GetOuter()->IRelationType( pPlayer ) == D_LI )
		return pPlayer;
	return NULL;
}

//-------------------------------------

bool CAI_StandoffBehavior::GetDirectionOfStandoff( Vector *pDir )
{
	if ( GetEnemy() )
	{
		*pDir = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
		VectorNormalize( *pDir );
		pDir->z = 0;
		return true;
	}
	return false;
}

//-------------------------------------

Hint_e CAI_StandoffBehavior::GetHintType()
{
	CAI_Hint *pHintNode = GetOuter()->m_pHintNode;
	if ( pHintNode )
		return pHintNode->HintType();
	return HINT_NONE;
}

//-------------------------------------

void CAI_StandoffBehavior::SetReuseCurrentCover()
{
	CAI_Hint *pHintNode = GetOuter()->m_pHintNode;
	if ( pHintNode && pHintNode->GetNode() && pHintNode->GetNode()->IsLocked() )
		pHintNode->GetNode()->Unlock();
}

//-------------------------------------

void CAI_StandoffBehavior::UnlockHintNode()
{
	CAI_Hint *pHintNode = GetOuter()->m_pHintNode;
	if ( pHintNode )
	{
		if ( pHintNode->IsLocked() && pHintNode->IsLockedBy( GetOuter() ) )
			pHintNode->Unlock();
		CAI_Node *pNode = pHintNode->GetNode();
		if ( pNode && pNode->IsLocked() )
			pNode->Unlock();
		GetOuter()->m_pHintNode = NULL;
	}
}


//-------------------------------------

Activity CAI_StandoffBehavior::GetCoverActivity()
{
	CAI_Hint *pHintNode = GetOuter()->m_pHintNode;
	if ( pHintNode && pHintNode->HintType() == HINT_TACTICAL_COVER_LOW )
		return GetOuter()->GetCoverActivity( pHintNode );
	return ACT_INVALID;
}


//-------------------------------------

void CAI_StandoffBehavior::UpdateTranslateActivityMap()
{
	struct ActivityMapping
	{
		AI_Posture_t	posture;
		Activity		activity;
		const char *	pszWeapon;
		Activity		translation;
	};
	
	static ActivityMapping mappings[] =
	{
		{	AIP_CROUCHING, 	ACT_IDLE, 				NULL, 				ACT_COVER_LOW 				},
		{	AIP_CROUCHING, 	ACT_WALK, 				NULL, 				ACT_WALK_CROUCH 			},
		{	AIP_CROUCHING, 	ACT_RUN, 				NULL, 				ACT_RUN_CROUCH 				},
		{	AIP_CROUCHING, 	ACT_WALK_AIM, 			NULL, 				ACT_WALK_CROUCH_AIM 		},
		{	AIP_CROUCHING, 	ACT_RUN_AIM, 			NULL, 				ACT_RUN_CROUCH_AIM 			},
		{	AIP_CROUCHING,	ACT_RELOAD_PISTOL, 		NULL, 				ACT_RELOAD_PISTOL_LOW		},
		{	AIP_CROUCHING,	ACT_RELOAD_SMG1, 		NULL, 				ACT_RELOAD_SMG1_LOW			},
		//----
		{	AIP_STANDING,	ACT_COVER_LOW,			NULL, 				ACT_IDLE 					},
		//----
		{	AIP_PEEKING, 	ACT_IDLE, 				"weapon_smg1", 		ACT_RANGE_AIM_SMG1_LOW		},
		{	AIP_PEEKING, 	ACT_COVER_LOW, 			"weapon_smg1", 		ACT_RANGE_AIM_SMG1_LOW		},
		{	AIP_PEEKING, 	ACT_IDLE,				"weapon_pistol",	ACT_RANGE_AIM_PISTOL_LOW	},
		{	AIP_PEEKING, 	ACT_COVER_LOW,			"weapon_pistol",	ACT_RANGE_AIM_PISTOL_LOW	},
		{	AIP_PEEKING,	ACT_IDLE,				"weapon_ar2",		ACT_RANGE_AIM_AR2_LOW		},
		{	AIP_PEEKING,	ACT_COVER_LOW,			"weapon_ar2",		ACT_RANGE_AIM_AR2_LOW		},
		{	AIP_PEEKING, 	ACT_RANGE_ATTACK_PISTOL, NULL, 				ACT_RANGE_ATTACK_PISTOL_LOW },
		{	AIP_PEEKING, 	ACT_RANGE_ATTACK_SMG1, 	NULL, 				ACT_RANGE_ATTACK_SMG1_LOW	},
		{	AIP_PEEKING,	ACT_RELOAD_PISTOL, 		NULL, 				ACT_RELOAD_PISTOL_LOW		},
		{	AIP_PEEKING,	ACT_RELOAD_SMG1, 		NULL, 				ACT_RELOAD_SMG1_LOW			},
	};

	m_ActivityMap.RemoveAll();
	
	CBaseCombatWeapon *pWeapon = GetOuter()->GetActiveWeapon();
	const char *pszWeaponClass = ( pWeapon ) ? pWeapon->GetClassname() : "";
	for ( int i = 0; i < ARRAYSIZE(mappings); i++ )
	{
		if ( !mappings[i].pszWeapon || stricmp( mappings[i].pszWeapon, pszWeaponClass ) == 0 )
		{
			if ( HaveSequenceForActivity( mappings[i].translation ) )
				m_ActivityMap.Insert( MAKE_ACTMAP_KEY( mappings[i].posture, mappings[i].activity ), mappings[i].translation );
		}
	}
}

//-------------------------------------

void CAI_StandoffBehavior::OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	UpdateTranslateActivityMap();
}

//-------------------------------------

void CAI_StandoffBehavior::OnRestore()
{
	UpdateTranslateActivityMap();
}

//-------------------------------------

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER(CAI_StandoffBehavior)

	DECLARE_ACTIVITY( ACT_RANGE_AIM_SMG1_LOW )
	DECLARE_ACTIVITY( ACT_RANGE_AIM_PISTOL_LOW )
	DECLARE_ACTIVITY( ACT_RANGE_AIM_AR2_LOW )

AI_END_CUSTOM_SCHEDULE_PROVIDER()

//-----------------------------------------------------------------------------
//
// CAI_StandoffGoal
//
// Purpose: A level tool to control the standoff behavior. Use is not required
//			in order to use behavior.
//
//-----------------------------------------------------------------------------

AI_StandoffParams_t g_StandoffParamsByAgression[] =
{
	//	hintChangeReaction,		fCoverOnReload, PlayerBtlLn,	minTimeShots,	maxTimeShots, 	minShots, 	maxShots, 	oddsCover
	{ 	AIHCR_MOVE_ON_COVER,	true,			true, 			10.0,			15.0,			0, 			1, 			90,			false 		},	// AGGR_VERY_LOW
	{ 	AIHCR_MOVE_ON_COVER,	true,			true, 			4.0, 			8.0, 			1, 			2, 			50,			false 		},	// AGGR_LOW
	{ 	AIHCR_MOVE_ON_COVER,	true,			true, 			2.0, 			4.0, 			1, 			4, 			25,			false 		},	// AGGR_MEDIUM
	{ 	AIHCR_MOVE_ON_COVER,	true,			true, 			1.0, 			3.0, 			2, 			4, 			10,			false 		},	// AGGR_HIGH
	{ 	AIHCR_MOVE_ON_COVER,	false,			true, 			0, 				0, 				100,		100, 		0,			false 		},	// AGGR_VERY_HIGH
};

//-------------------------------------

class CAI_StandoffGoal : public CAI_GoalEntity
{
	DECLARE_CLASS( CAI_StandoffGoal, CAI_GoalEntity );

public:
	CAI_StandoffGoal()
	{
		m_aggressiveness = AGGR_MEDIUM;	
		m_fPlayerIsBattleline = true;
		m_HintChangeReaction = AIHCR_DEFAULT_AI;
		m_fStayAtCover = false;
		m_customParams = AI_DEFAULT_STANDOFF_PARAMS;
	}

	//---------------------------------

	void EnableGoal( CAI_BaseNPC *pAI )	
	{
		CAI_StandoffBehavior *pBehavior;
		if ( !pAI->GetBehavior( &pBehavior ) )
			return;
		
		pBehavior->SetActive( true );
		SetBehaviorParams( pBehavior);
	}

	void DisableGoal( CAI_BaseNPC *pAI  ) 
	{
		// @TODO (toml 04-07-03): remove the no damage spawn flag once stable. The implementation isn't very good.
		CAI_StandoffBehavior *pBehavior;
		if ( !pAI->GetBehavior( &pBehavior ) )
			return;
		pBehavior->SetActive( false );
		SetBehaviorParams( pBehavior);
	}

	void InputActivate( inputdata_t &inputdata )
	{
		ValidateAggression();
		BaseClass::InputActivate( inputdata );
	}
	
	void InputDeactivate( inputdata_t &inputdata ) 	
	{
		ValidateAggression();
		BaseClass::InputDeactivate( inputdata );
	}
	
	void InputSetAggressiveness( inputdata_t &inputdata )
	{
		int newVal = inputdata.value.Int();
		
		m_aggressiveness = (Aggressiveness_t)newVal;
		ValidateAggression();
		
		UpdateActors();

		const CUtlVector<AIHANDLE> &actors = AccessActors();
		for ( int i = 0; i < actors.Count(); i++ )
		{
			CAI_BaseNPC *pAI = actors[i];
			CAI_StandoffBehavior *pBehavior;
			if ( !pAI->GetBehavior( &pBehavior ) )
				continue;
			SetBehaviorParams( pBehavior);
		}
	}

	void SetBehaviorParams( CAI_StandoffBehavior *pBehavior )
	{
		AI_StandoffParams_t params;

		if ( m_aggressiveness != AGGR_CUSTOM )
			params = g_StandoffParamsByAgression[m_aggressiveness];
		else
			params = m_customParams;

		params.hintChangeReaction = m_HintChangeReaction;
		params.fPlayerIsBattleline = m_fPlayerIsBattleline;
		params.fStayAtCover = m_fStayAtCover;

		pBehavior->SetParameters( params );
	}
	
	void ValidateAggression()
	{
		if ( m_aggressiveness < AGGR_VERY_LOW || m_aggressiveness > AGGR_VERY_HIGH )
		{
			if ( m_aggressiveness != AGGR_CUSTOM )
			{
				Msg( "Invalid aggressiveness value %d\n", m_aggressiveness );
				
				if ( m_aggressiveness < AGGR_VERY_LOW )
					m_aggressiveness = AGGR_VERY_LOW;
				else if ( m_aggressiveness > AGGR_VERY_HIGH )
					m_aggressiveness = AGGR_VERY_HIGH;
			}
		}
	}

private:
	//---------------------------------

	DECLARE_DATADESC();

	enum Aggressiveness_t
	{
		AGGR_VERY_LOW,
		AGGR_LOW,
		AGGR_MEDIUM,
		AGGR_HIGH,
		AGGR_VERY_HIGH,
		
		AGGR_CUSTOM,
	};

	Aggressiveness_t 		m_aggressiveness;	
	AI_HintChangeReaction_t m_HintChangeReaction;
	bool					m_fPlayerIsBattleline;
	bool					m_fStayAtCover;
	AI_StandoffParams_t		m_customParams;
};

//-------------------------------------

LINK_ENTITY_TO_CLASS( ai_goal_standoff, CAI_StandoffGoal );

BEGIN_DATADESC( CAI_StandoffGoal )
	DEFINE_KEYFIELD( CAI_StandoffGoal, m_aggressiveness,				FIELD_INTEGER, 	"Aggressiveness" ),
	//								   m_customParams  (individually)
	DEFINE_KEYFIELD( CAI_StandoffGoal, m_HintChangeReaction,			FIELD_INTEGER, 	"HintGroupChangeReaction" ),
	DEFINE_KEYFIELD( CAI_StandoffGoal, m_fPlayerIsBattleline,			FIELD_BOOLEAN,	"PlayerBattleline" ),
	DEFINE_KEYFIELD( CAI_StandoffGoal, m_fStayAtCover,					FIELD_BOOLEAN,	"StayAtCover" ),
	DEFINE_KEYFIELD( CAI_StandoffGoal, m_customParams.fCoverOnReload,	FIELD_BOOLEAN, 	"CustomCoverOnReload" ),
	DEFINE_KEYFIELD( CAI_StandoffGoal, m_customParams.minTimeShots,		FIELD_FLOAT, 	"CustomMinTimeShots" ),
	DEFINE_KEYFIELD( CAI_StandoffGoal, m_customParams.maxTimeShots,		FIELD_FLOAT, 	"CustomMaxTimeShots" ),
	DEFINE_KEYFIELD( CAI_StandoffGoal, m_customParams.minShots,			FIELD_INTEGER, 	"CustomMinShots" ),
	DEFINE_KEYFIELD( CAI_StandoffGoal, m_customParams.maxShots,			FIELD_INTEGER, 	"CustomMaxShots" ),
	DEFINE_KEYFIELD( CAI_StandoffGoal, m_customParams.oddsCover,		FIELD_INTEGER, 	"CustomOddsCover" ),

	// Inputs
	DEFINE_INPUTFUNC( CAI_StandoffGoal, FIELD_INTEGER, "SetAggressiveness", InputSetAggressiveness ),
END_DATADESC()

///-----------------------------------------------------------------------------
