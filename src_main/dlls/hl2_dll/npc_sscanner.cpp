//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "npc_sscanner.h"
#include "AI_Default.h"
#include "AI_Hull.h"
#include "AI_Hint.h"
#include "AI_Node.h" // just for GetFloorZ()
#include "AI_Navigator.h"
#include "ai_moveprobe.h"
#include "AI_Squad.h"
#include "explode.h"
#include "grenade_energy.h"
#include "ndebugoverlay.h"
#include "scanner_shield.h"
#include "npc_sscanner_beam.h"
#include "gib.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"

#define	SSCANNER_MAX_SPEED				400
#define	SSCANNER_NUM_GLOWS				2
#define SSCANNER_GIB_COUNT			5   
#define SSCANNER_ATTACK_NEAR_DIST		600
#define SSCANNER_ATTACK_FAR_DIST		800

#define SSCANNER_COVER_NEAR_DIST		60
#define SSCANNER_COVER_FAR_DIST		250
#define SSCANNER_MAX_SHEILD_DIST		400

#define	SSCANNER_BANK_RATE			5
#define SSCANNER_ENERGY_WARMUP_TIME	0.5
#define SSCANNER_MIN_GROUND_DIST		150

#define SSCANNER_GIB_COUNT			5 

ConVar	sk_sscanner_health( "sk_sscanner_health","0");

extern float	GetFloorZ(const Vector &origin, float fMaxDrop);

//-----------------------------------------------------------------------------
// Private activities.
//-----------------------------------------------------------------------------
int	ACT_SSCANNER_FLINCH_BACK;
int	ACT_SSCANNER_FLINCH_FRONT;
int	ACT_SSCANNER_FLINCH_LEFT;
int	ACT_SSCANNER_FLINCH_RIGHT;
int	ACT_SSCANNER_LOOK;
int	ACT_SSCANNER_OPEN;
int	ACT_SSCANNER_CLOSE;

//-----------------------------------------------------------------------------
// SScanner schedules.
//-----------------------------------------------------------------------------
enum SScannerSchedules
{
	SCHED_SSCANNER_HOVER = LAST_SHARED_SCHEDULE,
	SCHED_SSCANNER_PATROL,
	SCHED_SSCANNER_CHASE_ENEMY,
	SCHED_SSCANNER_CHASE_TARGET,
	SCHED_SSCANNER_OPEN,
	SCHED_SSCANNER_CLOSE,
};

//-----------------------------------------------------------------------------
// Custom tasks.
//-----------------------------------------------------------------------------
enum SScannerTasks
{
	TASK_SSCANNER_OPEN = LAST_SHARED_TASK,
	TASK_SSCANNER_CLOSE,
};

//-----------------------------------------------------------------------------
// Custom Conditions
//-----------------------------------------------------------------------------
enum SScanner_Conds
{
	COND_SSCANNER_FLY_BLOCKED	= LAST_SHARED_CONDITION,
	COND_SSCANNER_FLY_CLEAR,
	COND_SSCANNER_PISSED_OFF,
};


BEGIN_DATADESC( CNPC_SScanner )

	DEFINE_FIELD( CNPC_SScanner,	m_lastHurtTime,				FIELD_FLOAT ),

	DEFINE_FIELD( CNPC_SScanner,	m_nState,					FIELD_INTEGER),
	DEFINE_FIELD( CNPC_SScanner,	m_bShieldsDisabled,			FIELD_BOOLEAN),
	DEFINE_FIELD( CNPC_SScanner,	m_pShield,					FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_SScanner,	m_pShieldBeamL,				FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_SScanner,	m_pShieldBeamR,				FIELD_CLASSPTR ),
	DEFINE_FIELD( CNPC_SScanner,	m_fNextShieldCheckTime,		FIELD_TIME),

	DEFINE_FIELD( CNPC_SScanner,	m_fNextFlySoundTime,		FIELD_TIME),

	DEFINE_FIELD( CNPC_SScanner,	m_vCoverPosition,			FIELD_POSITION_VECTOR),

END_DATADESC()


LINK_ENTITY_TO_CLASS( npc_sscanner, CNPC_SScanner );
IMPLEMENT_CUSTOM_AI( npc_sscanner, CNPC_SScanner );

CNPC_SScanner::CNPC_SScanner()
{
#ifdef _DEBUG
	m_vCurrentBanking.Init();
	m_vCoverPosition.Init();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_SScanner::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CNPC_SScanner);

	ADD_CUSTOM_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_HOVER);
	ADD_CUSTOM_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_PATROL);
	ADD_CUSTOM_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_CHASE_ENEMY);
	ADD_CUSTOM_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_CHASE_TARGET);
	ADD_CUSTOM_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_OPEN);
	ADD_CUSTOM_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_CLOSE);

	ADD_CUSTOM_CONDITION(CNPC_SScanner,	COND_SSCANNER_FLY_BLOCKED);
	ADD_CUSTOM_CONDITION(CNPC_SScanner,	COND_SSCANNER_FLY_CLEAR);
	ADD_CUSTOM_CONDITION(CNPC_SScanner,	COND_SSCANNER_PISSED_OFF);

	ADD_CUSTOM_TASK(CNPC_SScanner,		TASK_SSCANNER_OPEN);
	ADD_CUSTOM_TASK(CNPC_SScanner,		TASK_SSCANNER_CLOSE);

	ADD_CUSTOM_ACTIVITY(CNPC_SScanner,	ACT_SSCANNER_FLINCH_BACK);
	ADD_CUSTOM_ACTIVITY(CNPC_SScanner,	ACT_SSCANNER_FLINCH_FRONT);
	ADD_CUSTOM_ACTIVITY(CNPC_SScanner,	ACT_SSCANNER_FLINCH_LEFT);
	ADD_CUSTOM_ACTIVITY(CNPC_SScanner,	ACT_SSCANNER_FLINCH_RIGHT);
	ADD_CUSTOM_ACTIVITY(CNPC_SScanner,	ACT_SSCANNER_OPEN);
	ADD_CUSTOM_ACTIVITY(CNPC_SScanner,	ACT_SSCANNER_CLOSE);

	AI_LOAD_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_HOVER);
	AI_LOAD_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_PATROL);
	AI_LOAD_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_CHASE_ENEMY);
	AI_LOAD_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_CHASE_TARGET);
	AI_LOAD_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_OPEN);
	AI_LOAD_SCHEDULE(CNPC_SScanner,	SCHED_SSCANNER_CLOSE);
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this NPC's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_SScanner::Classify(void)
{
	return(CLASS_SCANNER);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_SScanner::StopLoopingSounds(void)
{
	StopSound( "NPC_SScanner.FlySound" );
	BaseClass::StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
// Output : int - new schedule type
//-----------------------------------------------------------------------------
int CNPC_SScanner::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_FAIL_TAKE_COVER:
		return SCHED_SSCANNER_PATROL;
		break;
	}
	return BaseClass::TranslateSchedule(scheduleType);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_SScanner::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );
	StopShield();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_SScanner::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( info.GetDamageType() & DMG_BULLET)
	{
		g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}


//------------------------------------------------------------------------------
// Purpose : Override to split in two when attacked
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_SScanner::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	// Don't take friendly fire from combine
	if (info.GetAttacker()->Classify() == CLASS_COMBINE)
	{
		info.SetDamage( 0 );
	}
	else if (m_nState == SSCANNER_OPEN)
	{
		// Flinch in the direction of the attack
		float vAttackYaw = VecToYaw(g_vecAttackDir);

		float vAngleDiff = UTIL_AngleDiff( vAttackYaw, m_fHeadYaw );

		if (vAngleDiff > -45 && vAngleDiff < 45)
		{
			SetActivity((Activity)ACT_SSCANNER_FLINCH_BACK);
		}
		else if (vAngleDiff < -45 && vAngleDiff > -135)
		{
			SetActivity((Activity)ACT_SSCANNER_FLINCH_LEFT);
		}
		else if (vAngleDiff >  45 && vAngleDiff <  135)
		{
			SetActivity((Activity)ACT_SSCANNER_FLINCH_RIGHT);
		}
		else
		{
			SetActivity((Activity)ACT_SSCANNER_FLINCH_FRONT);
		}

		m_lastHurtTime = gpGlobals->curtime;
	}
	return (BaseClass::OnTakeDamage_Alive( info ));
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_SScanner::IsValidShieldCover( Vector &vCoverPos )
{
	if (GetEnemy() == NULL)
	{
		return true;
	}

	// Make sure I can get here
	trace_t tr;
	AI_TraceEntity( this, GetAbsOrigin(), vCoverPos, MASK_NPCSOLID, &tr );
	if (tr.fraction != 1.0)
	{
		//NDebugOverlay::Cross3D(vCoverPos,Vector(-15,-15,-15),Vector(5,5,5),255,0,0,true,1.0);
		return false;
	}

	// Make sure position is in cover
	Vector vThreatEyePos = GetEnemy()->EyePosition();
	AI_TraceLine ( vCoverPos, vThreatEyePos, MASK_OPAQUE,  this, COLLISION_GROUP_NONE, &tr );
	if (tr.fraction == 1.0)
	{
		//NDebugOverlay::Cross3D(vCoverPos,Vector(-15,-15,-15),Vector(5,5,5),0,0,255,true,1.0);
		return false;
	}
	else
	{
		//NDebugOverlay::Cross3D(vCoverPos,Vector(-15,-15,-15),Vector(5,5,5),0,255,0,true,1.0);
		return true;
	}
}

//------------------------------------------------------------------------------
// Purpose : Attempts to find a position that is in cover from the enemy
//			 but with in range to use shield.
// Input   :
// Output  : True if a cover position was set
//------------------------------------------------------------------------------
bool CNPC_SScanner::SetShieldCoverPosition( void )
{
	// Make sure I'm shielding someone and have an enemy
	if (GetTarget() == NULL	||
		GetEnemy()	 == NULL	)
	{
		m_vCoverPosition = vec3_origin;
		return false;
	}

	// If I have a current cover position check if it's valid
	if (m_vCoverPosition != vec3_origin)
	{
		// If in range of my shield target, and valid cover keep same cover position
		if ((m_vCoverPosition - GetTarget()->GetLocalOrigin()).Length() < SSCANNER_COVER_FAR_DIST &&
			IsValidShieldCover(m_vCoverPosition)								)
		{
			return true;
		}
	}

	// Otherwise try a random direction
	QAngle vAngle	= QAngle(0,random->RandomInt(-180,180),0);
	Vector vForward;
	AngleVectors( vAngle, &vForward );
	
	// Now get the position
	Vector vTestPos = GetTarget()->GetLocalOrigin() + vForward * (random->RandomInt(150,240)); 
	vTestPos.z		= GetLocalOrigin().z;

	// Is it a valid cover position
	if (IsValidShieldCover(vTestPos))
	{
		m_vCoverPosition = vTestPos;
		return true;
	}
	else
	{
		m_vCoverPosition = vec3_origin;
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles movement towards the last move target.
// Input  : flInterval - 
//-----------------------------------------------------------------------------
bool CNPC_SScanner::OverridePathMove( float flInterval )
{
	CBaseEntity *pMoveTarget = (GetTarget()) ? GetTarget() : GetEnemy();
	Vector waypointDir = GetNavigator()->GetCurWaypointPos() - GetLocalOrigin();
	VectorNormalize(waypointDir);

	// -----------------------------------------------------------------
	// Check route is blocked
	// ------------------------------------------------------------------
	Vector checkPos = GetLocalOrigin() + (waypointDir * (m_flSpeed * flInterval));

	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( NAV_FLY, GetLocalOrigin(), checkPos, MASK_NPCSOLID|CONTENTS_WATER,
		pMoveTarget,&moveTrace);

	if (IsMoveBlocked( moveTrace ))
	{
		TaskFail(FAIL_NO_ROUTE);
		GetNavigator()->ClearGoal();
		return true;
	}
	
	// --------------------------------------------------
	//  Check if I've reached my goal
	// --------------------------------------------------
	
	Vector lastPatrolDir = GetNavigator()->GetCurWaypointPos() - GetLocalOrigin();
	
	
	if ( ProgressFlyPath( flInterval, pMoveTarget, MASK_NPCSOLID,
						  !IsCurSchedule( SCHED_SSCANNER_PATROL ) ) == AINPP_COMPLETE )
	{
		if (IsCurSchedule( SCHED_SSCANNER_PATROL ))
		{
			m_vLastPatrolDir = lastPatrolDir;
			VectorNormalize(m_vLastPatrolDir);
		}
		return true;
	}
	return false;
}

bool CNPC_SScanner::OverrideMove(float flInterval)
{
	// ----------------------------------------------
	//	Select move target 
	// ----------------------------------------------
	CBaseEntity*	pMoveTarget = NULL;
	float			fNearDist	= 0;
	float			fFarDist	= 0;
	if (GetTarget() != NULL )
	{
		pMoveTarget = GetTarget();
		fNearDist	= SSCANNER_COVER_NEAR_DIST;
		fFarDist	= SSCANNER_COVER_FAR_DIST;
	}
	else if (GetEnemy() != NULL )
	{
		pMoveTarget = GetEnemy();		
		fNearDist	= SSCANNER_ATTACK_NEAR_DIST;
		fFarDist	= SSCANNER_ATTACK_FAR_DIST;
	}

	// -----------------------------------------
	//  See if we can fly there directly
	// -----------------------------------------
	if (pMoveTarget)
	{
		trace_t tr;
		Vector endPos = GetAbsOrigin() + GetCurrentVelocity()*flInterval;
		AI_TraceHull(GetAbsOrigin(), pMoveTarget->GetAbsOrigin() + Vector(0,0,150),
			GetHullMins(), GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction != 1.0)
		{
			/*
			NDebugOverlay::Cross3D(GetLocalOrigin()+Vector(0,0,-60),Vector(-15,-15,-15),Vector(5,5,5),0,255,255,true,0.1);
			*/
			SetCondition( COND_SSCANNER_FLY_BLOCKED );
		}
		else
		{
			SetCondition( COND_SSCANNER_FLY_CLEAR );
		}
	}

	// -----------------------------------------------------------------
	// If I have a route, keep it updated and move toward target
	// ------------------------------------------------------------------
	if (GetNavigator()->IsGoalActive())
	{
		if ( OverridePathMove( flInterval ) )
			return true;
	}

	// ----------------------------------------------
	//	Move to target directly if path is clear
	// ----------------------------------------------
	else if ( pMoveTarget && HasCondition( COND_SSCANNER_FLY_CLEAR ) )
	{
		MoveToEntity(flInterval, pMoveTarget, fNearDist, fFarDist);
	}
	// -----------------------------------------------------------------
	// Otherwise just decelerate
	// -----------------------------------------------------------------
	else 
	{
		float	myDecay	 = 9.5;
		Decelerate( flInterval, myDecay );
		// -------------------------------------
		// If I have an enemy turn to face him
		// -------------------------------------
		if (GetEnemy())
		{
			TurnHeadToTarget(flInterval, GetEnemy()->GetLocalOrigin() );
		}
	}
	MoveExecute_Alive(flInterval);

	UpdateShields();
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CNPC_SScanner::NPC_TranslateActivity( Activity eNewActivity )
{
	if ( eNewActivity == ACT_IDLE)
	{
		if (m_nState == SSCANNER_OPEN)
		{
			return ACT_IDLE_ANGRY;
		}
		else
		{
			return ACT_IDLE;
		}
	}
	return eNewActivity;
}

//------------------------------------------------------------------------------
// Purpose : Choose which entity to shield
// Input   :
// Output  :
//------------------------------------------------------------------------------
CBaseEntity* CNPC_SScanner::PickShieldEntity(void)
{
	if (m_bShieldsDisabled)
	{
		return NULL;
	}

	//	Don't pick every frame as it gets expensive
	if (gpGlobals->curtime > m_fNextShieldCheckTime)
	{
		m_fNextShieldCheckTime = gpGlobals->curtime + 1.0;

		CBaseEntity* pBestEntity = NULL;
		float		 fBestDist	 = MAX_COORD_RANGE;
		bool		 bBestLOS	 = false;
		float		 fTestDist;
		bool		 bTestLOS	 = false;;

		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			CAI_BaseNPC* pTestNPC = pSquadMember;
			if (pTestNPC != NULL && 
				pTestNPC != this &&
				pTestNPC->Classify() != CLASS_SCANNER	)
			{
				bTestLOS  = pTestNPC->HasCondition( COND_HAVE_ENEMY_LOS);
				fTestDist = (GetLocalOrigin() - pTestNPC->GetLocalOrigin()).Length();

				// -----------------------------------------
				//  Test has LOS and best doesn't, pick test
				// -----------------------------------------
				if (bTestLOS && !bBestLOS)
				{
					// Skip if I can't see this entity
					if (IsValidShieldTarget(pTestNPC) )
					{
						fBestDist	= fTestDist;
						pBestEntity	= pTestNPC;
						bBestLOS	= bTestLOS;
					}
				}
				// -----------------------------------------
				//  Best has LOS and test doesn't, skip
				// -----------------------------------------
				else if (!bTestLOS && bBestLOS)
				{
					continue;
				}
				// -----------------------------------------------
				//  Otherwise pick by distance
				// -----------------------------------------------
				if (fTestDist <	fBestDist )					
				{
					// Skip if I can't see this entity
					if (IsValidShieldTarget(pTestNPC) )
					{
						fBestDist	= fTestDist;
						pBestEntity	= pTestNPC;
						bBestLOS	= bTestLOS;
					}
				}
			}
		}
		return pBestEntity;
	}
	else
	{
		return GetTarget();
	}
}

//------------------------------------------------------------------------------
// Purpose : Override to set pissed level of sscanner
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_SScanner::NPCThink(void)
{
	// --------------------------------------------------
	//  COND_SSCANNER_PISSED_OFF
	// --------------------------------------------------
	float fHurtAge		= gpGlobals->curtime - m_lastHurtTime;

	if (fHurtAge > 5.0)
	{
		ClearCondition(COND_SSCANNER_PISSED_OFF);
		m_bShieldsDisabled = false;
	}
	else
	{
		SetCondition(COND_SSCANNER_PISSED_OFF);
		m_bShieldsDisabled = true;
	}

	BaseClass::NPCThink();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_SScanner::PowerShield(void)
{
	// -----------------------------
	//	Start the shield chase beam
	// -----------------------------
	if (!m_pShieldBeamL)
	{
		m_pShieldBeamL = CShieldBeam::ShieldBeamCreate( this, 2 );
		m_pShieldBeamR = CShieldBeam::ShieldBeamCreate( this, 3 );
	}

	if (!m_pShieldBeamL->IsShieldBeamOn())
	{
		m_pShieldBeamL->ShieldBeamStart(this, GetTarget(), GetCurrentVelocity(), 500 );
		m_pShieldBeamR->ShieldBeamStart(this, GetTarget(), GetCurrentVelocity(), 500 );
		m_pShieldBeamL->m_vTailOffset		= Vector(0,0,88);
		m_pShieldBeamR->m_vTailOffset		= Vector(0,0,88);
	}

	// -------------------------------------------------
	//	Start the shield when chaser reaches the shield
	// ------------------------------------------------- 
	if (m_pShieldBeamL->IsShieldBeamOn() && m_pShieldBeamL->ReachedTail())
	{

		if (!m_pShield)
		{
			m_pShield = (CScannerShield *)CreateEntityByName("scanner_shield" );
			m_pShield->Spawn();
		}
		m_pShield->SetTarget(GetTarget());
		m_pShieldBeamL->SetNoise(GetCurrentVelocity().Length());
		m_pShieldBeamR->SetNoise(GetCurrentVelocity().Length());
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_SScanner::StopShield(void)
{
	// -----------------------------
	//	Stop shield chase beam
	// -----------------------------
	if (m_pShieldBeamL)
	{
		m_pShieldBeamL->ShieldBeamStop();
	}
	if (m_pShieldBeamR)
	{
		m_pShieldBeamR->ShieldBeamStop();
	}

	// -----------------------------
	//	Stop the shield
	// -----------------------------
	if (m_pShield)
	{
		m_pShield->SetTarget(NULL);
	}
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_SScanner::UpdateShields(void)
{
	// -------------------------------------------------------
	//  Turn of shield and don't reset m_hTarget if in script
	// -------------------------------------------------------
	if (m_IdealNPCState != NPC_STATE_SCRIPT ||
		m_NPCState != NPC_STATE_SCRIPT)
	{
		StopShield();
		return;
	}

	// -------------------------------------------------------------------------
	//  If I'm not in a squad there's no one to shield
	// -------------------------------------------------------------------------
	if (!m_pSquad)
	{
		return;
	}

	// -------------------------------------------------------------------------
	// If I'm dead, stop the shields
	// -------------------------------------------------------------------------
	if ( m_IdealNPCState == NPC_STATE_DEAD )
	{
		SetTarget( NULL );
		StopShield();
	}

	// -------------------------------------------------------------------------
	// Pick the best entity to shield
	// -------------------------------------------------------------------------
	CBaseEntity* pShielded = PickShieldEntity();

	// -------------------------------------------------------------------------
	// If there was no best entity stop the shields 
	// -------------------------------------------------------------------------
	if (pShielded == NULL)
	{
		SetTarget( NULL );
		StopShield();
		return;
	}

	// -------------------------------------------------------------------------
	// If I'm too far to shield set best entity as target but stop the shields
	// -------------------------------------------------------------------------
	float fDist = (GetLocalOrigin() - pShielded->GetLocalOrigin()).Length();
	if (fDist > SSCANNER_MAX_SHEILD_DIST)
	{
		SetTarget( pShielded );
		StopShield();
		return;
	}

	// -------------------------------------------------------------------------
	// If this is a new target, stop the shield
	// -------------------------------------------------------------------------
	if (m_pShieldBeamL				&&
		GetTarget() != pShielded	&&
		m_pShieldBeamL->IsShieldBeamOn()	)
	{
		StopShield();
	}

	// -------------------------------------------------------------------------
	// Reset my target entity and power the shield
	// -------------------------------------------------------------------------
	SetTarget( pShielded );

		// -------------------------------------------------------------------------
	// If I'm closed, stop the shields
	// -------------------------------------------------------------------------
	if ( m_nState == SSCANNER_CLOSED )
	{
		StopShield();
	}
	else
	{
		PowerShield();
	}
	
	/* DEBUG TOOL 
	if (GetTarget())
	{
		int blue = 0;
		int red  = 0;
		if (GetTarget())
		{
			red = 255;
		}
		else
		{
			blue = 255;
		}
		NDebugOverlay::Cross3D(GetTarget()->GetLocalOrigin()+Vector(0,0,60),Vector(-15,-15,-15),Vector(5,5,5),red,0,blue,true,0.1);
		NDebugOverlay::Cross3D(GetLocalOrigin()+Vector(0,0,60),Vector(-15,-15,-15),Vector(5,5,5),red,0,blue,true,0.1);
		NDebugOverlay::Line(GetLocalOrigin(), GetTarget()->GetLocalOrigin(), red,0,blue, true, 0.1);
	}
	else
	{
		NDebugOverlay::Cross3D(GetLocalOrigin()+Vector(0,0,60),Vector(-15,-15,-15),Vector(5,5,5),0,255,0,true,0.1);
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_SScanner::MoveToTarget(float flInterval, const Vector &MoveTarget)
{
	const float	myAccel	 = 300.0;
	const float	myDecay	 = 9.0;
	
	TurnHeadToTarget( flInterval, MoveTarget );
	MoveToLocation( flInterval, MoveTarget, myAccel, (2 * myAccel), myDecay );
}



//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_SScanner::MoveToEntity(float flInterval, CBaseEntity* pMoveTarget, float fMinRange, float fMaxRange)
{
	// -----------------------------------------
	//  Make sure we have a move target
	// -----------------------------------------
	if (!pMoveTarget)
	{
		return;
	}

	// -----------------------------------------
	//  Keep within range of enemy
	// -----------------------------------------
	Vector vFlyDirection = vec3_origin;
	Vector	vEnemyDir  = pMoveTarget->EyePosition() - GetAbsOrigin();
	float	fEnemyDist = VectorNormalize(vEnemyDir);
	if (fEnemyDist < fMinRange)
	{
		vFlyDirection = -vEnemyDir;
	}
	else if (fEnemyDist > fMaxRange)
	{
		vFlyDirection = vEnemyDir;
	}
	// If I'm shielding someone see if I can go to cover
	else if (SetShieldCoverPosition())
	{
		vFlyDirection = m_vCoverPosition - GetAbsOrigin();
		VectorNormalize(vFlyDirection);
	}

	TurnHeadToTarget( flInterval, pMoveTarget->GetAbsOrigin() );

	// -------------------------------------
	// Set net velocity 
	// -------------------------------------
	float	myAccel	 = 500.0;
	float	myDecay	 = 0.35; // decay to 35% in 1 second
	MoveInDirection(flInterval, vFlyDirection, myAccel, (2 * myAccel), myDecay);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_SScanner::PlayFlySound(void)
{
	if (gpGlobals->curtime > m_fNextFlySoundTime)
	{
		EmitSound( "NPC_SScanner.FlySound" );
		m_fNextFlySoundTime	= gpGlobals->curtime + 1.0;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CNPC_SScanner::MinGroundDist(void)
{
	return SSCANNER_MIN_GROUND_DIST;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_SScanner::MoveExecute_Alive(float flInterval)
{
	// ----------------------------------------------------------------------------------------
	// Add time-coherent noise to the current velocity so that it never looks bolted in place.
	// ----------------------------------------------------------------------------------------
	AddNoiseToVelocity( 2.0 );

	// -------------------------------------------
	//  Avoid obstacles
	// -------------------------------------------
	SetCurrentVelocity( GetCurrentVelocity() + VelocityToAvoidObstacles(flInterval) );

	// ---------------------
	//  Limit overall speed
	// ---------------------
	LimitSpeed( 200 );

	// If I'm right over the ground limit my banking so my blades
	// don't sink into the floor
	float floorZ = GetFloorZ(GetLocalOrigin());
	if (abs(GetLocalOrigin().z - floorZ) < 36)
	{
		QAngle angles = GetLocalAngles();
		if (angles.x < -20) angles.x = -20;
		if (angles.x >  20) angles.x =  20;
		if (angles.z < -20) angles.z = -20;
		if (angles.z >  20) angles.z =  20;
		SetLocalAngles( angles );
	}

	PlayFlySound();
	WalkMove( GetCurrentVelocity() * flInterval, MASK_NPCSOLID );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_SScanner::Precache(void)
{
	//
	// Model.
	//
	engine->PrecacheModel("models/shield_scanner.mdl");
	engine->PrecacheModel("models/scanner_shield.mdl");
	engine->PrecacheModel("models/gibs/mortarsynth_gibs.mdl");
	
	enginesound->PrecacheSound( "npc/waste_scanner/grenade_fire.wav");
	enginesound->PrecacheSound( "npc/waste_scanner/hover.wav");

	engine->PrecacheModel("sprites/physbeam.vmt");	
	engine->PrecacheModel("sprites/glow01.vmt");

	engine->PrecacheModel("sprites/physring1.vmt");

	BaseClass::Precache();
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_SScanner::Gib(void)
{
	// Sparks
	for (int i = 0; i < 4; i++)
	{
		Vector sparkPos = GetAbsOrigin();
		sparkPos.x += random->RandomFloat(-12,12);
		sparkPos.y += random->RandomFloat(-12,12);
		sparkPos.z += random->RandomFloat(-12,12);
		g_pEffects->Sparks(sparkPos);
	}
	// Smoke
	UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);

	// Light
	CBroadcastRecipientFilter filter;
	te->DynamicLight( filter, 0.0,
			&GetAbsOrigin(), 255, 180, 100, 0, 100, 0.1, 0 );

	// Throw scanner gibs
	CGib::SpawnSpecificGibs( this, SSCANNER_GIB_COUNT, 800, 1000, "models/gibs/mortarsynth_gibs.mdl");

	ExplosionCreate(GetAbsOrigin(), GetAbsAngles(), NULL, random->RandomInt(30, 40), 0, true);

	UTIL_Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_SScanner::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		// If my enemy has moved significantly, update my path
		case TASK_WAIT_FOR_MOVEMENT:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if (pEnemy && IsCurSchedule(SCHED_SSCANNER_CHASE_ENEMY) && GetNavigator()->IsGoalActive())
			{
				Vector flEnemyLKP = GetEnemyLKP();
				if ((GetNavigator()->GetGoalPos() - pEnemy->EyePosition()).Length() > 40 )
				{
					GetNavigator()->UpdateGoalPos(pEnemy->EyePosition());
				}
				// If final position is enemy, exit my schedule (will go to attack hover)
				if (GetNavigator()->IsGoalActive() && 
					GetNavigator()->GetCurWaypointPos() == pEnemy->EyePosition())
				{
					TaskComplete();
					GetNavigator()->ClearGoal();		// Stop moving
					break;
				}
			}
			if (m_nState == SSCANNER_OPEN)
			{
				GetNavigator()->SetMovementActivity(ACT_IDLE_ANGRY);
			}
			else
			{
				GetNavigator()->SetMovementActivity(ACT_IDLE);
			}

			BaseClass::RunTask(pTask);
			break;
		}

		default:
		{
			BaseClass::RunTask(pTask);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_SScanner::Spawn(void)
{
	Precache();

	SetModel( "models/shield_scanner.mdl");

	SetHullType(HULL_SMALL_CENTERED); 
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	// Make bounce so I bounce off the surfaces I hit, but turn gravity off
	SetMoveType( MOVETYPE_STEP );
	SetGravity(0.001);

	m_iHealth			= sk_sscanner_health.GetFloat();
	SetViewOffset( Vector(0, 0, 10) );		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_NPCState			= NPC_STATE_NONE;
	SetNavType( NAV_FLY );
	m_bloodColor		= DONT_BLEED;
	SetCurrentVelocity( Vector(0, 0, 0) );
	m_vCurrentBanking	= Vector(0, 0, 0);

	AddFlag( FL_FLY );

	CapabilitiesAdd( bits_CAP_MOVE_FLY );
	CapabilitiesAdd( bits_CAP_SQUAD);

	m_nState					= SSCANNER_CLOSED;
	m_bShieldsDisabled			= false;

	m_vLastPatrolDir			= vec3_origin;
	m_fNextFlySoundTime			= 0;

	m_fNextShieldCheckTime		= 0;

	SetNoiseMod( 2, 2, 2 );

	m_fHeadYaw			= 0;

	NPCInit();

	// Let this guy look far
	m_flDistTooFar	= 99999999.0;
	SetDistLook( 4000.0 );
	m_flSpeed = SSCANNER_MAX_SPEED;
}


void CNPC_SScanner::OnScheduleChange()
{
	m_flSpeed = SSCANNER_MAX_SPEED;

	BaseClass::OnScheduleChange();
}


//-----------------------------------------------------------------------------
// Purpose: Gets the appropriate next schedule based on current condition
//			bits.
//-----------------------------------------------------------------------------
int CNPC_SScanner::SelectSchedule(void)
{
	switch ( m_NPCState )
	{
		// -------------------------------------------------
		//  In idle attemp to pair up with a squad 
		//  member that could use shielding
		// -------------------------------------------------
		case NPC_STATE_IDLE:
		{
			// --------------------------------------------------
			//  If I'm blocked find a path to my goal entity
			// --------------------------------------------------
			if (HasCondition( COND_SSCANNER_FLY_BLOCKED ) && (GetTarget() != NULL))
			{  
				return SCHED_SSCANNER_CHASE_TARGET;
			}

			return SCHED_SSCANNER_HOVER;
			break;
		}
		case NPC_STATE_DEAD:
		case NPC_STATE_SCRIPT:
		{
			return BaseClass::SelectSchedule();
			break;
		}
		default:
		{

			// --------------------------------------------------
			//  If I'm blocked find a path to my goal entity
			// --------------------------------------------------
			if (HasCondition( COND_SSCANNER_FLY_BLOCKED ))
			{  
				if ((GetTarget() != NULL) && !HasCondition( COND_SSCANNER_PISSED_OFF))
				{
					return SCHED_SSCANNER_CHASE_TARGET;
				}
			}
			// --------------------------------------------------
			//  If I have a live enemy
			// --------------------------------------------------
			if (GetEnemy() != NULL && GetEnemy()->IsAlive())
			{
				// --------------------------------------------------
				//  If I'm open 
				// --------------------------------------------------
				if (m_nState == SSCANNER_OPEN)
				{	
					if (GetTarget()	== NULL				)
					{
						return SCHED_SSCANNER_CLOSE;
					}
					else if (HasCondition( COND_SSCANNER_PISSED_OFF))
					{
						return SCHED_SSCANNER_CLOSE;
					}
					else
					{
						return SCHED_SSCANNER_HOVER;
					}
				}
				else if (m_nState == SSCANNER_CLOSED)
				{
					if (GetTarget()	!= NULL				)
					{
						float fDist = (GetAbsOrigin() - GetTarget()->GetAbsOrigin()).Length();
						if (fDist <= SSCANNER_MAX_SHEILD_DIST)
						{
							return SCHED_SSCANNER_OPEN;
						}
						else
						{
							return SCHED_SSCANNER_HOVER;
						}
					}	
					else if (HasCondition( COND_SSCANNER_PISSED_OFF))
					{
						return SCHED_TAKE_COVER_FROM_ENEMY;
					}
					else
					{
						return SCHED_SSCANNER_HOVER;
					}
				}
				else
				{
					return SCHED_SSCANNER_HOVER;
				}
			}
			// --------------------------------------------------
			//  I have no enemy so patrol
			// --------------------------------------------------
			else if (GetEnemy() == NULL)
			{
				return SCHED_SSCANNER_PATROL;
			}
			// --------------------------------------------------
			//  I have no enemy so patrol
			// --------------------------------------------------
			else
			{
				return SCHED_SSCANNER_HOVER;
			}
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_SScanner::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{	
		case TASK_SSCANNER_OPEN:  
		{
			m_nState = SSCANNER_OPEN;
			TaskComplete();
			break;
		}
		case TASK_SSCANNER_CLOSE:  
		{
			m_nState = SSCANNER_CLOSED;
			TaskComplete();
			break;
		}
		// Override so can find hint nodes that are much further away
		case TASK_FIND_HINTNODE:
		{
			if (!m_pHintNode)
			{
				m_pHintNode = CAI_Hint::FindHint(this, HINT_NONE, pTask->flTaskData, 5000 );
			}
			if ( m_pHintNode )
			{
				TaskComplete();
			}
			else
			{
				// No hint node run from enemy
				SetSchedule( SCHED_RUN_FROM_ENEMY );
			}
			break;
		}
		default:
		{
			BaseClass::StartTask(pTask);
		}
	}
}


//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_SScanner::IsValidShieldTarget(CBaseEntity *pEntity)
{
	// Reject if already shielded by another squad member
	AISquadIter_t iter;
	for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
	{
		CNPC_SScanner *pScanner = dynamic_cast<CNPC_SScanner*>(pSquadMember);
		if (pScanner										&&
			pScanner			   != this					&&
			pScanner->Classify() == CLASS_SCANNER	)
		{
			if (pScanner->GetTarget() == pEntity)
			{
				// Reject unless I'm significantly closer
				float fNPCDist	= (pScanner->GetLocalOrigin() - pEntity->GetLocalOrigin()).Length();
				float fMyDist	= (GetLocalOrigin() - pEntity->GetLocalOrigin()).Length();
				if ((fNPCDist - fMyDist) > 150 )
				{
					// Steal from other scanner
					pScanner->StopShield();
					pScanner->SetTarget( NULL );
					return true;
				}
				return false;
			}
		}
	}
	return true;
}


//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

//=========================================================
// > SCHED_SSCANNER_HOVER
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SSCANNER_HOVER,

	"	Tasks"
	"		TASK_RESET_ACTIVITY		0"
	"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_IDLE"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_SSCANNER_FLY_BLOCKED"
	"		COND_LIGHT_DAMAGE"
);

//=========================================================
// > SCHED_SSCANNER_OPEN
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SSCANNER_OPEN,

	"	Tasks"
	"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_SSCANNER_OPEN"
	"		TASK_SSCANNER_OPEN		0"
	"	"
	"	Interrupts"
);

//=========================================================
// > SCHED_SSCANNER_CLOSE
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SSCANNER_CLOSE,

	"	Tasks"
	"		TASK_SSCANNER_CLOSE		0"
	"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_SSCANNER_CLOSE"
	"	"
	"	Interrupts"
);

//=========================================================
// > SCHED_SSCANNER_PATROL
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SSCANNER_PATROL,

	"	Tasks"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_SET_ROUTE_SEARCH_TIME		5"	// Spend 5 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_RANDOM_NODE	2000"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0		"
	"	"
	"	Interrupts"
	"		COND_GIVE_WAY"
	"		COND_NEW_ENEMY"
	"		COND_SEE_ENEMY"
	"		COND_SEE_FEAR"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
	"		COND_HEAR_PLAYER"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_PROVOKED"
);

//=========================================================
// > SCHED_SSCANNER_CHASE_ENEMY
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SSCANNER_CHASE_ENEMY,

	"	Tasks"
	"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_SSCANNER_PATROL"
	"		 TASK_SET_TOLERANCE_DISTANCE		120"
	"		 TASK_GET_PATH_TO_ENEMY				0"
	"		 TASK_RUN_PATH						0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	"	"
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_SSCANNER_FLY_CLEAR"
);

//=========================================================
// > SCHED_SSCANNER_CHASE_TARGET
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_SSCANNER_CHASE_TARGET,

	"	Tasks"
	"		 TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_SSCANNER_HOVER"
	"		 TASK_SET_TOLERANCE_DISTANCE		120"
	"		 TASK_GET_PATH_TO_TARGET			0"
	"		 TASK_RUN_PATH						0"
	"		 TASK_WAIT_FOR_MOVEMENT				0"
	"	"
	"	"
	"	Interrupts"
	"		COND_SSCANNER_FLY_CLEAR"
	"		COND_SSCANNER_PISSED_OFF"
);
