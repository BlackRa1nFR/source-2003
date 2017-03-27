//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "beam_shared.h"
#include "npc_mortarsynth.h"
#include "AI_Default.h"
#include "AI_Node.h"
#include "AI_Hull.h"
#include "AI_Hint.h"
#include "AI_Navigator.h"
#include "ai_moveprobe.h"
#include "AI_Memory.h"
#include "Sprite.h"
#include "explode.h"
#include "grenade_energy.h"
#include "ndebugoverlay.h"
#include "game.h"			
#include "gib.h"
#include "AI_Interactions.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"

#define MSYNTH_GIB_COUNT			5   
#define MSYNTH_ATTACK_NEAR_DIST		600
#define MSYNTH_ATTACK_FAR_DIST		800

#define MSYNTH_COVER_NEAR_DIST		60
#define MSYNTH_COVER_FAR_DIST		250
#define MSYNTH_MAX_SHEILD_DIST		400

#define	MSYNTH_BANK_RATE			5
#define MSYNTH_ENERGY_WARMUP_TIME	0.5
#define MSYNTH_MIN_GROUND_DIST		40
#define MSYNTH_MAX_GROUND_DIST		50

#define MSYNTH_GIB_COUNT			5 

ConVar	sk_mortarsynth_health( "sk_mortarsynth_health","0");

extern float	GetFloorZ(const Vector &origin, float fMaxDrop);

//-----------------------------------------------------------------------------
// Private activities.
//-----------------------------------------------------------------------------
int	ACT_MSYNTH_FLINCH_BACK;
int	ACT_MSYNTH_FLINCH_FRONT;
int	ACT_MSYNTH_FLINCH_LEFT;
int	ACT_MSYNTH_FLINCH_RIGHT;

//-----------------------------------------------------------------------------
// MSynth schedules.
//-----------------------------------------------------------------------------
enum MSynthSchedules
{
	SCHED_MSYNTH_HOVER = LAST_SHARED_SCHEDULE,
	SCHED_MSYNTH_PATROL,
	SCHED_MSYNTH_CHASE_ENEMY,
	SCHED_MSYNTH_CHASE_TARGET,
	SCHED_MSYNTH_SHOOT_ENERGY,
};


//-----------------------------------------------------------------------------
// MSynth tasks.
//-----------------------------------------------------------------------------
enum MSynthTasks
{
	TASK_ENERGY_WARMUP = LAST_SHARED_TASK,
	TASK_ENERGY_SHOOT,
};

//-----------------------------------------------------------------------------
// Custom Conditions
//-----------------------------------------------------------------------------
enum MSynth_Conds
{
	COND_MSYNTH_FLY_BLOCKED	= LAST_SHARED_CONDITION,
	COND_MSYNTH_FLY_CLEAR,
	COND_MSYNTH_PISSED_OFF,
};

BEGIN_DATADESC( CNPC_MSynth )

	DEFINE_FIELD( CNPC_MSynth,	m_lastHurtTime,				FIELD_FLOAT ),

	DEFINE_FIELD( CNPC_MSynth,	m_pEnergyBeam,				FIELD_CLASSPTR ),
	DEFINE_ARRAY( CNPC_MSynth,	m_pEnergyGlow,				FIELD_CLASSPTR,		MSYNTH_NUM_GLOWS ),

	DEFINE_FIELD( CNPC_MSynth,	m_fNextEnergyAttackTime,	FIELD_TIME),
	DEFINE_FIELD( CNPC_MSynth,	m_fNextFlySoundTime,		FIELD_TIME),

	DEFINE_FIELD( CNPC_MSynth,	m_vCoverPosition,			FIELD_POSITION_VECTOR),

END_DATADESC()


LINK_ENTITY_TO_CLASS( npc_mortarsynth, CNPC_MSynth );
IMPLEMENT_CUSTOM_AI( npc_mortarsynth, CNPC_MSynth );

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_MSynth::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CNPC_MSynth);

	ADD_CUSTOM_TASK(CNPC_MSynth,	TASK_ENERGY_WARMUP);
	ADD_CUSTOM_TASK(CNPC_MSynth,	TASK_ENERGY_SHOOT);

	ADD_CUSTOM_SCHEDULE(CNPC_MSynth,	SCHED_MSYNTH_HOVER);
	ADD_CUSTOM_SCHEDULE(CNPC_MSynth,	SCHED_MSYNTH_PATROL);
	ADD_CUSTOM_SCHEDULE(CNPC_MSynth,	SCHED_MSYNTH_CHASE_ENEMY);
	ADD_CUSTOM_SCHEDULE(CNPC_MSynth,	SCHED_MSYNTH_CHASE_TARGET);
	ADD_CUSTOM_SCHEDULE(CNPC_MSynth,	SCHED_MSYNTH_SHOOT_ENERGY);

	ADD_CUSTOM_CONDITION(CNPC_MSynth,	COND_MSYNTH_FLY_BLOCKED);
	ADD_CUSTOM_CONDITION(CNPC_MSynth,	COND_MSYNTH_FLY_CLEAR);
	ADD_CUSTOM_CONDITION(CNPC_MSynth,	COND_MSYNTH_PISSED_OFF);

	ADD_CUSTOM_ACTIVITY(CNPC_MSynth,	ACT_MSYNTH_FLINCH_BACK);
	ADD_CUSTOM_ACTIVITY(CNPC_MSynth,	ACT_MSYNTH_FLINCH_FRONT);
	ADD_CUSTOM_ACTIVITY(CNPC_MSynth,	ACT_MSYNTH_FLINCH_LEFT);
	ADD_CUSTOM_ACTIVITY(CNPC_MSynth,	ACT_MSYNTH_FLINCH_RIGHT);

	AI_LOAD_SCHEDULE(CNPC_MSynth,	SCHED_MSYNTH_HOVER);
	AI_LOAD_SCHEDULE(CNPC_MSynth,	SCHED_MSYNTH_PATROL);
	AI_LOAD_SCHEDULE(CNPC_MSynth,	SCHED_MSYNTH_CHASE_ENEMY);
	AI_LOAD_SCHEDULE(CNPC_MSynth,	SCHED_MSYNTH_CHASE_TARGET);
	AI_LOAD_SCHEDULE(CNPC_MSynth,	SCHED_MSYNTH_SHOOT_ENERGY);
}

CNPC_MSynth::CNPC_MSynth()
{
#ifdef _DEBUG
	m_vCurrentBanking.Init();
	m_vCoverPosition.Init();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this NPC's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_MSynth::Classify(void)
{
	return(CLASS_MORTAR_SYNTH);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_MSynth::StopLoopingSounds(void)
{
	StopSound( "NPC_MSynth.Hover" );
	StopSound( "NPC_MSynth.WarmUp" );
	StopSound( "NPC_MSynth.Shoot" );

	BaseClass::StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
int CNPC_MSynth::TranslateSchedule( int scheduleType ) 
{
	return BaseClass::TranslateSchedule(scheduleType);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_MSynth::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	EnergyKill();
	Gib();
}

//------------------------------------------------------------------------------
// Purpose : Override to split in two when attacked
//------------------------------------------------------------------------------
int CNPC_MSynth::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;
	
	// Don't take friendly fire from combine
	if (info.GetAttacker()->Classify() == CLASS_COMBINE)
	{
		info.SetDamage( 0 );
	}
	else
	{
		// Flinch in the direction of the attack
		float vAttackYaw = VecToYaw(g_vecAttackDir);

		float vAngleDiff = UTIL_AngleDiff( vAttackYaw, m_fHeadYaw );

		if		(vAngleDiff > -45 && vAngleDiff < 45)
		{
			SetActivity((Activity)ACT_MSYNTH_FLINCH_BACK);
		}
		else if (vAngleDiff < -45 && vAngleDiff > -135)
		{
			SetActivity((Activity)ACT_MSYNTH_FLINCH_LEFT);
		}
		else if (vAngleDiff >  45 && vAngleDiff <  135)
		{
			SetActivity((Activity)ACT_MSYNTH_FLINCH_RIGHT);
		}
		else
		{
			SetActivity((Activity)ACT_MSYNTH_FLINCH_FRONT);
		}

		m_lastHurtTime = gpGlobals->curtime;
	}
	return (BaseClass::OnTakeDamage_Alive( info ));
}

//-----------------------------------------------------------------------------
// Purpose: Handles movement towards the last move target.
// Input  : flInterval - 
//-----------------------------------------------------------------------------
bool CNPC_MSynth::OverrideMove(float flInterval)
{
	if (IsActivityFinished())
	{
		SetActivity(ACT_IDLE);
	}

	// ----------------------------------------------
	//	Select move target 
	// ----------------------------------------------
	CBaseEntity*	pMoveTarget = NULL;
	float			fNearDist	= 0;
	float			fFarDist	= 0;
	if (GetTarget() != NULL )
	{
		pMoveTarget = GetTarget();
		fNearDist	= MSYNTH_COVER_NEAR_DIST;
		fFarDist	= MSYNTH_COVER_FAR_DIST;
	}
	else if (GetEnemy() != NULL )
	{
		pMoveTarget = GetEnemy();		
		fNearDist	= MSYNTH_ATTACK_NEAR_DIST;
		fFarDist	= MSYNTH_ATTACK_FAR_DIST;
	}

	// -----------------------------------------
	//  See if we can fly there directly
	// -----------------------------------------
	if (pMoveTarget)
	{
		trace_t tr;
		Vector endPos = GetAbsOrigin() + GetCurrentVelocity()*flInterval;
		AI_TraceHull(GetAbsOrigin(), pMoveTarget->GetAbsOrigin() + Vector(0,0,150), 
			GetHullMins(), GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, 
			this, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction != 1.0)
		{
			/*
			NDebugOverlay::Cross3D(GetLocalOrigin()+Vector(0,0,-60),Vector(-15,-15,-15),Vector(5,5,5),0,255,255,true,0.1);
			*/
			SetCondition( COND_MSYNTH_FLY_BLOCKED );
		}
		else
		{
			SetCondition( COND_MSYNTH_FLY_CLEAR );
		}
	}

	// -----------------------------------------------------------------
	// If I have a route, keep it updated and move toward target
	// ------------------------------------------------------------------
	if (GetNavigator()->IsGoalActive())
	{
		// -----------------------------------------------------------------
		// Check route is blocked
		// ------------------------------------------------------------------
		AIMoveTrace_t moveTrace;
		GetMoveProbe()->MoveLimit( NAV_FLY, GetLocalOrigin(), GetNavigator()->GetCurWaypointPos(), 
			MASK_NPCSOLID, GetEnemy(), &moveTrace);

		if (IsMoveBlocked( moveTrace ))
		{
			TaskFail(FAIL_NO_ROUTE);
			GetNavigator()->ClearGoal();
			return true;
		}
		
		// --------------------------------------------------
		
  		Vector lastPatrolDir = GetNavigator()->GetCurWaypointPos() - GetLocalOrigin();
  		
  		if ( ProgressFlyPath( flInterval, GetEnemy(), MASK_NPCSOLID,
  							  !IsCurSchedule( SCHED_MSYNTH_PATROL ) ) == AINPP_COMPLETE )
  		{
			if (IsCurSchedule( SCHED_MSYNTH_PATROL ))
			{
				m_vLastPatrolDir = lastPatrolDir;
				VectorNormalize(m_vLastPatrolDir);
			}
			return true;
  		}
	}
		
	// ----------------------------------------------
	//	Move to target directly if path is clear
	// ----------------------------------------------
	else if ( pMoveTarget && HasCondition( COND_MSYNTH_FLY_CLEAR ) )
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

	return true;
}

//------------------------------------------------------------------------------
// Purpose : Override to set pissed level of msynth
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MSynth::NPCThink(void)
{
	// --------------------------------------------------
	//  COND_MSYNTH_PISSED_OFF
	// --------------------------------------------------
	float fHurtAge		= gpGlobals->curtime - m_lastHurtTime;

	if (fHurtAge > 5.0)
	{
		ClearCondition(COND_MSYNTH_PISSED_OFF);
	}
	else
	{
		SetCondition(COND_MSYNTH_PISSED_OFF);
	}

	Vector pos = GetAbsOrigin()+Vector(0,0,-30);
	CBroadcastRecipientFilter filter;
	te->DynamicLight( filter, 0.0, &pos, 0, 0, 255, 0, 45, 0.1, 50 );

	BaseClass::NPCThink();
}

//------------------------------------------------------------------------------
// Purpose : Is head facing the given position
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_MSynth::IsHeadFacing( const Vector &vFaceTarget )
{
	float fDesYaw = UTIL_AngleDiff(VecToYaw(vFaceTarget - GetLocalOrigin()), GetLocalAngles().y);

	// If I've flipped completely around, reverse angles
	float fYawDiff = m_fHeadYaw - fDesYaw;
	if (fYawDiff > 180)
	{
		m_fHeadYaw -= 360;
	}
	else if (fYawDiff < -180)
	{
		m_fHeadYaw += 360;
	}

	if (fabs(m_fHeadYaw - fDesYaw) > 20)
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_MSynth::MoveToTarget(float flInterval, const Vector &MoveTarget)
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
void CNPC_MSynth::MoveToEntity(float flInterval, CBaseEntity* pMoveTarget, float fMinRange, float fMaxRange)
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
	TurnHeadToTarget( flInterval, pMoveTarget->GetAbsOrigin() );

	// -------------------------------------
	// Set net velocity 
	// -------------------------------------
	float	myAccel	 = 500.0;
	float	myDecay	 = 0.35; // decay to 35% in 1 second
	MoveInDirection( flInterval, vFlyDirection, myAccel, 2 * myAccel, myDecay);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_MSynth::PlayFlySound(void)
{
	if (gpGlobals->curtime > m_fNextFlySoundTime)
	{
		EmitSound( "NPC_MSynth.Hover" );
		m_fNextFlySoundTime	= gpGlobals->curtime + 1.0;
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CNPC_MSynth::MinGroundDist(void)
{
	return MSYNTH_MIN_GROUND_DIST;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_MSynth::SetBanking(float flInterval)
{
	// Make frame rate independent
	float	iRate	 = 0.5;
	float timeToUse = flInterval;
	while (timeToUse > 0)
	{
		Vector vFlyDirection = GetCurrentVelocity();
		VectorNormalize(vFlyDirection);
		vFlyDirection *= MSYNTH_BANK_RATE;
		
		Vector vBankDir;
		// Bank based on fly direction
		vBankDir	= vFlyDirection;

		m_vCurrentBanking.x		= (iRate * m_vCurrentBanking.x) + (1 - iRate)*(vBankDir.x);
		m_vCurrentBanking.z		= (iRate * m_vCurrentBanking.z) + (1 - iRate)*(vBankDir.z);
		timeToUse =- 0.1;
	}
	SetLocalAngles( QAngle( m_vCurrentBanking.x, 0, m_vCurrentBanking.z ) );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_MSynth::MoveExecute_Alive(float flInterval)
{
	// ----------------------------------------------------------------------------------------
	// Add time-coherent noise to the current velocity so that it never looks bolted in place.
	// ----------------------------------------------------------------------------------------

	AddNoiseToVelocity();
	
	// -------------------------------------------
	//  Avoid obstacles
	// -------------------------------------------
	SetCurrentVelocity( GetCurrentVelocity() + VelocityToAvoidObstacles(flInterval) );

	// ---------------------
	//  Limit overall speed
	// ---------------------
	LimitSpeed( 200, MSYNTH_MAX_SPEED);

	SetBanking(flInterval);

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
void CNPC_MSynth::Precache(void)
{

	//
	// Model.
	//
	engine->PrecacheModel("models/mortarsynth.mdl");
	engine->PrecacheModel("models/gibs/mortarsynth_gibs.mdl");
	
	engine->PrecacheModel("sprites/physbeam.vmt");	
	engine->PrecacheModel("sprites/glow01.vmt");

	engine->PrecacheModel("sprites/physring1.vmt");

	UTIL_PrecacheOther("grenade_energy");

	BaseClass::Precache();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MSynth::Gib(void)
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

	// Throw mortar synth gibs
	CGib::SpawnSpecificGibs( this, MSYNTH_GIB_COUNT, 800, 1000, "models/gibs/mortarsynth_gibs.mdl");

	ExplosionCreate(GetAbsOrigin(), GetAbsAngles(), NULL, random->RandomInt(30, 40), 0, true);

	UTIL_Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_MSynth::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_ENERGY_WARMUP:
		{
			// ---------------------------
			//  Make sure I have an enemy
			// -----------------------------
			if (GetEnemy() == NULL)
			{
				EnergyKill();
				TaskFail(FAIL_NO_ENEMY);
			}

			if (gpGlobals->curtime < m_flWaitFinished)
			{
				// -------------------
				//  Adjust beam	
				// -------------------
				float fScale = (1-(m_flWaitFinished - gpGlobals->curtime)/MSYNTH_ENERGY_WARMUP_TIME);
				if (m_pEnergyBeam)
				{
					m_pEnergyBeam->SetBrightness(255*fScale);
					m_pEnergyBeam->SetColor( 255, 255*fScale, 255*fScale );
				}

				// -------------------
				//  Adjust glow
				// -------------------
				for (int i=0;i<MSYNTH_NUM_GLOWS-1;i++)
				{
					if (m_pEnergyGlow[i])
					{
						m_pEnergyGlow[i]->SetColor( 255, 255*fScale, 255*fScale );
						m_pEnergyGlow[i]->SetBrightness( 100 + (155*fScale) );
					}
				}
			}
			// --------------------
			//	Face Enemy
			// --------------------


			if ( gpGlobals->curtime >= m_flWaitFinished  &&
				 IsHeadFacing(GetEnemy()->GetLocalOrigin())  )
			{
				TaskComplete();
			}
			break;
		}
		// If my enemy has moved significantly, update my path
		case TASK_WAIT_FOR_MOVEMENT:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if (pEnemy														&&
				IsCurSchedule(SCHED_MSYNTH_CHASE_ENEMY)  && 
				GetNavigator()->IsGoalActive()											   )
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
			CAI_BaseNPC::RunTask(pTask);
			break;
		}

		default:
		{
			CAI_BaseNPC::RunTask(pTask);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_MSynth::Spawn(void)
{
	Precache();

	SetModel( "models/mortarsynth.mdl" );

	SetHullType(HULL_LARGE_CENTERED); 
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	// Make bounce so I bounce off the surfaces I hit, but turn gravity off
	SetMoveType( MOVETYPE_STEP );
	SetGravity(0.001);

	m_iHealth			= sk_mortarsynth_health.GetFloat();
	SetViewOffset( Vector(0, 0, 10) );		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView		= 0.2;
	m_NPCState			= NPC_STATE_NONE;
	SetNavType( NAV_FLY );
	SetBloodColor( DONT_BLEED );
	SetCurrentVelocity( Vector(0, 0, 0) );
	m_vCurrentBanking	= Vector(0, 0, 0);

	AddFlag( FL_FLY );

	// This is a flying creature, but it uses grond nodes as it hovers right over the ground
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );
	CapabilitiesAdd( bits_CAP_SQUAD);

	m_vLastPatrolDir			= vec3_origin;
	m_fNextEnergyAttackTime		= 0;
	m_fNextFlySoundTime			= 0;

	m_pEnergyBeam				= NULL;
	
	for (int i=0;i<MSYNTH_NUM_GLOWS;i++)
	{
		m_pEnergyGlow[i] = NULL;
	}

	SetNoiseMod( 2, 2, 2 );

	m_fHeadYaw			= 0;

	NPCInit();

	// Let this guy look far
	m_flDistTooFar	= 99999999.0;
	SetDistLook( 4000.0 );
}

//-----------------------------------------------------------------------------
// Purpose: Gets the appropriate next schedule based on current condition
//			bits.
//-----------------------------------------------------------------------------
int CNPC_MSynth::SelectSchedule(void)
{
	// Kill laser if its still on
	EnergyKill();

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
			if (HasCondition( COND_MSYNTH_FLY_BLOCKED ) && (GetTarget() != NULL))
			{  
				return SCHED_MSYNTH_CHASE_TARGET;
			}

			return SCHED_MSYNTH_HOVER;
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
			if (HasCondition( COND_MSYNTH_FLY_BLOCKED ))
			{  
				if ((GetTarget() != NULL) && !HasCondition( COND_MSYNTH_PISSED_OFF))
				{
					return SCHED_MSYNTH_CHASE_TARGET;
				}
				else if (GetEnemy() != NULL)
				{
					return SCHED_MSYNTH_CHASE_ENEMY;
				}
			}
			// --------------------------------------------------
			//  If I have a live enemy
			// --------------------------------------------------
			if (GetEnemy() != NULL && GetEnemy()->IsAlive())
			{
				// --------------------------------------------------
				//  If I'm not shielding someone or I'm pissed off
				//  attack my enemy
				// --------------------------------------------------
				if (GetTarget()	== NULL					|| 
					HasCondition( COND_MSYNTH_PISSED_OFF)	)
				{
					// ------------------------------------------------
					//  If I don't have a line of sight, establish one
					// ------------------------------------------------
					if (!HasCondition(COND_HAVE_ENEMY_LOS))
					{
						return SCHED_MSYNTH_CHASE_ENEMY;
					}
					// ------------------------------------------------
					//  If I can't attack go for fover
					// ------------------------------------------------
					else if (gpGlobals->curtime <  m_fNextEnergyAttackTime)
					{
						return SCHED_MSYNTH_HOVER;
					}
					// ------------------------------------------------
					//  Otherwise shoot at my enemy
					// ------------------------------------------------
					else
					{
						return SCHED_MSYNTH_SHOOT_ENERGY;
					}
				}
				// --------------------------------------------------
				//  Otherwise hover in place
				// --------------------------------------------------
				else 
				{
					return SCHED_MSYNTH_HOVER;
				}
			}
			// --------------------------------------------------
			//  I have no enemy so patrol
			// --------------------------------------------------
			else if (GetEnemy() == NULL)
			{
				return SCHED_MSYNTH_HOVER;
			}
			// --------------------------------------------------
			//  I have no enemy so patrol
			// --------------------------------------------------
			else
			{
				return SCHED_MSYNTH_PATROL;
			}
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_MSynth::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{	
		// Create
		case TASK_ENERGY_WARMUP:
		{
			if (GetEnemy() == NULL)
			{
				TaskFail(FAIL_NO_ENEMY);
			}
			else
			{
				GetMotor()->SetIdealYawToTarget( GetEnemy()->GetLocalOrigin() );
				SetTurnActivity(); 

				EnergyWarmup();
				// set a future time that tells us when the warmup is over.
				m_flWaitFinished = gpGlobals->curtime + MSYNTH_ENERGY_WARMUP_TIME;
			}
			break;
		}
		case TASK_ENERGY_SHOOT:
		{
			EnergyShoot();
			TaskComplete();
			break;
		}

		// Override so can find hint nodes that are much further away
		case TASK_FIND_HINTNODE:
		{
			if (!m_pHintNode )
			{
				m_pHintNode = CAI_Hint::FindHint( this, HINT_NONE, pTask->flTaskData, 5000 );
			}
			if ( m_pHintNode)
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
void CNPC_MSynth::EnergyWarmup(void)
{
	// -------------------
	//  Beam beams
	// -------------------
	m_pEnergyBeam = CBeam::BeamCreate( "sprites/physbeam.vmt", 2.0 );
	m_pEnergyBeam->SetColor( 255, 0, 0 ); 
	m_pEnergyBeam->SetBrightness( 100 );
	m_pEnergyBeam->SetNoise( 8 );
	m_pEnergyBeam->EntsInit( this, this );
	m_pEnergyBeam->SetStartAttachment( 1 );
	m_pEnergyBeam->SetEndAttachment( 2 );
	
	// -------------
	//  Glow
	// -------------
	m_pEnergyGlow[0] = CSprite::SpriteCreate( "sprites/glow01.vmt", GetLocalOrigin(), FALSE );
	m_pEnergyGlow[0]->SetAttachment( this, 1 );
	m_pEnergyGlow[0]->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
	m_pEnergyGlow[0]->SetBrightness( 100 );
	m_pEnergyGlow[0]->SetScale( 0.3 );

	m_pEnergyGlow[1] = CSprite::SpriteCreate( "sprites/glow01.vmt", GetLocalOrigin(), FALSE );
	m_pEnergyGlow[1]->SetAttachment( this, 2 );
	m_pEnergyGlow[1]->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
	m_pEnergyGlow[1]->SetBrightness( 100 );
	m_pEnergyGlow[1]->SetScale( 0.3 );

	EmitSound( "NPC_MSynth.WarmUp" );

	// After firing sit still for a second to make easier to hit
	SetCurrentVelocity( vec3_origin );

}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MSynth::EnergyShoot(void)
{
	CBaseEntity* pEnemy = GetEnemy();
	if (pEnemy)
	{
		//<<TEMP>> need new sound
		EmitSound( "NPC_MSynth.EnergyShoot" );

		for (int i=1;i<3;i++)
		{
			Vector vShootStart;
			QAngle vShootAng;
			GetAttachment( i, vShootStart, vShootAng );
			Vector vShootDir	= ( pEnemy->EyePosition() ) - vShootStart;
			VectorNormalize(vShootDir);

			CGrenadeEnergy::Shoot( this, vShootStart, vShootDir * 800 );
		}
	}

	EmitSound( "NPC_MSynth.Shoot" );

	if (m_pEnergyBeam)
	{
		// Let beam kill itself
		m_pEnergyBeam->LiveForTime(0.2);
		m_pEnergyBeam = NULL;
	}

	m_fNextEnergyAttackTime = gpGlobals->curtime + random->RandomFloat(1.8,2.2); 
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MSynth::EnergyKill(void)
{
	// -------------------------------
	//  Kill beams if not set to die
	// ------------------------------
	if (m_pEnergyBeam)
	{
		UTIL_Remove(m_pEnergyBeam);
		m_pEnergyBeam = NULL;
	}

	// ---------------------
	//  Kill laser
	// ---------------------
	for (int i=0;i<MSYNTH_NUM_GLOWS;i++)
	{
		if (m_pEnergyGlow[i])
		{
			UTIL_Remove( m_pEnergyGlow[i] );
			m_pEnergyGlow[i] = NULL;
		}
	}

	// Kill charge sound in case still going
	StopSound(entindex(), "NPC_MSynth.WarmUp" );
}


//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

//=========================================================
// > SCHED_MSYNTH_HOVER
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_MSYNTH_HOVER,

	"	Tasks"
	"		TASK_WAIT			5"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_MSYNTH_FLY_BLOCKED"
);

//=========================================================
// > SCHED_MSYNTH_SHOOT_ENERGY
//
// Shoot and go for cover
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_MSYNTH_SHOOT_ENERGY,

	"	Tasks"
	"		TASK_ENERGY_WARMUP		0"
	"		TASK_ENERGY_SHOOT		0"
	//"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
);

//=========================================================
// > SCHED_MSYNTH_PATROL
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_MSYNTH_PATROL,

	"	Tasks"
	"		TASK_SET_TOLERANCE_DISTANCE		48"
	"		TASK_SET_ROUTE_SEARCH_TIME		5"	// Spend 5 seconds trying to build a path if stuck
	"		TASK_GET_PATH_TO_RANDOM_NODE	2000"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
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
// > SCHED_MSYNTH_CHASE_ENEMY
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_MSYNTH_CHASE_ENEMY,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_MSYNTH_PATROL"
	"		TASK_SET_TOLERANCE_DISTANCE		120"
	"		TASK_GET_PATH_TO_ENEMY			0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_MSYNTH_FLY_CLEAR"
);

//=========================================================
// > SCHED_MSYNTH_CHASE_TARGET
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_MSYNTH_CHASE_TARGET,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_MSYNTH_HOVER"
	"		TASK_SET_TOLERANCE_DISTANCE		120"
	"		TASK_GET_PATH_TO_TARGET			0"
	"		TASK_RUN_PATH					0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	""
	"	Interrupts"
	"		COND_MSYNTH_FLY_CLEAR"
	"		COND_MSYNTH_PISSED_OFF"
);

