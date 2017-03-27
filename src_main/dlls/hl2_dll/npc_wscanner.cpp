//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "beam_shared.h"
#include "npc_wscanner.h"
#include "AI_Default.h"
#include "AI_Node.h"
#include "AI_Hint.h"
#include "AI_Navigator.h"
#include "ai_moveprobe.h"
#include "AI_Squad.h"
#include "NPCEvent.h"
#include "Sprite.h"
#include "explode.h"
#include "basegrenade_shared.h"
#include "ndebugoverlay.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"

#define	WSCANNER_MAX_SPEED				400
#define WSCANNER_GIB_COUNT		5   
#define WSCANNER_HOVER_NEAR_DIST	200
#define WSCANNER_HOVER_FAR_DIST		300
#define WSCANNER_SPLIT_DIST			350
#define	WSCANNER_SINGLE_BANK_RATE	35
#define	WSCANNER_PAIR_BANK_RATE		10
#define WSCANNER_LASER_WARMUP_TIME  2
#define WSCANNER_JOIN_TIME			1
#define WSCANNER_JOIN_DIST			100
#define WSCANNER_MIN_GROUND_DIST	96

#define WSCANNER_ATTACHMENT_SHOOT		1
#define WSCANNER_ATTACHMENT_TRI_RIGHT	1
#define WSCANNER_ATTACHMENT_TRI_LEFT	3
#define WSCANNER_ATTACHMENT_TRI_TOP		2

ConVar	sk_wscanner_health( "sk_wscanner_health","0");
ConVar	sk_wscanner_laser_dmg( "sk_wscanner_laser_dmg","0");

extern float	GetFloorZ(const Vector &origin);

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
int				CNPC_WScanner::m_nNumAttackers   = 0;

//-----------------------------------------------------------------------------
// Animation events.
//-----------------------------------------------------------------------------
#define		WSCANNER_AE_SHOOT		( 1 )
#define		WSCANNER_AE_ATTACH		( 2 )

//-----------------------------------------------------------------------------
// Private activities.
//-----------------------------------------------------------------------------
int	ACT_WSCANNER_JOIN;
int	ACT_WSCANNER_FLINCH_BACK;
int	ACT_WSCANNER_FLINCH_FRONT;
int	ACT_WSCANNER_FLINCH_LEFT;
int	ACT_WSCANNER_FLINCH_RIGHT;

//-----------------------------------------------------------------------------
// WScanner schedules.
//-----------------------------------------------------------------------------
enum WScannerSchedules
{
	SCHED_WSCANNER_ATTACK_HOVER = LAST_SHARED_SCHEDULE,
	SCHED_WSCANNER_PATROL,
	SCHED_WSCANNER_GET_HELP,
	SCHED_WSCANNER_CHASE_ENEMY,
	SCHED_WSCANNER_SHOOT_LASER,
};


//-----------------------------------------------------------------------------
// WScanner tasks.
//-----------------------------------------------------------------------------
enum WScannerTasks
{
	TASK_WSCANNER_JOIN	= LAST_SHARED_TASK,
	TASK_LASER_WARMUP,
	TASK_LASER_SHOOT,
	TASK_WSCANNER_RAISE_ANTENNA,
};

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
int	g_interactionWScannerBomb		= 0;

BEGIN_DATADESC( CNPC_WScanner )

	DEFINE_KEYFIELD( CNPC_WScanner, m_nPodSize,					FIELD_INTEGER, "NumberOfPods" ),

	DEFINE_FIELD( CNPC_WScanner,	m_hHelper,					FIELD_EHANDLE),
	DEFINE_FIELD( CNPC_WScanner,	m_nWScannerState,			FIELD_INTEGER),
	DEFINE_ARRAY( CNPC_WScanner,	m_pBeam,					FIELD_CLASSPTR,		WSCANNER_NUM_BEAMS ),
	DEFINE_ARRAY( CNPC_WScanner,	m_pLaserGlow,				FIELD_CLASSPTR,		WSCANNER_NUM_GLOWS ),
	DEFINE_FIELD( CNPC_WScanner,	m_bBankFace,				FIELD_BOOLEAN),
	DEFINE_FIELD( CNPC_WScanner,	m_fNextGrenadeTime,			FIELD_TIME),
	DEFINE_FIELD( CNPC_WScanner,	m_fNextLaserTime,			FIELD_TIME),

	DEFINE_FIELD( CNPC_WScanner,	m_fNextMoveEvadeTime,		FIELD_TIME),
	DEFINE_FIELD( CNPC_WScanner,	m_fNextAlarmSoundTime,		FIELD_TIME),
	DEFINE_FIELD( CNPC_WScanner,	m_fNextFlySoundTime,		FIELD_TIME),
	DEFINE_FIELD( CNPC_WScanner,	m_fStartJoinTime,			FIELD_TIME),
	DEFINE_FIELD( CNPC_WScanner,	m_fSwitchToHelpTime,		FIELD_TIME),

	DEFINE_FIELD( CNPC_WScanner,	m_fAntennaPos,				FIELD_FLOAT),
	DEFINE_FIELD( CNPC_WScanner,	m_fAntennaTarget,			FIELD_FLOAT),

	DEFINE_FIELD( CNPC_WScanner,	m_pLightGlow,				FIELD_CLASSPTR),

END_DATADESC()


LINK_ENTITY_TO_CLASS( npc_wscanner, CNPC_WScanner );
IMPLEMENT_CUSTOM_AI( npc_wscanner, CNPC_WScanner );

CNPC_WScanner::CNPC_WScanner()
{
#ifdef _DEBUG
	m_vCurrentBanking.Init();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_WScanner::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CNPC_WScanner);

	ADD_CUSTOM_TASK(CNPC_WScanner,	TASK_WSCANNER_JOIN);
	ADD_CUSTOM_TASK(CNPC_WScanner,	TASK_LASER_WARMUP);
	ADD_CUSTOM_TASK(CNPC_WScanner,	TASK_LASER_SHOOT);
	ADD_CUSTOM_TASK(CNPC_WScanner,	TASK_WSCANNER_RAISE_ANTENNA);

	ADD_CUSTOM_SCHEDULE(CNPC_WScanner,	SCHED_WSCANNER_ATTACK_HOVER);
	ADD_CUSTOM_SCHEDULE(CNPC_WScanner,	SCHED_WSCANNER_PATROL);
	ADD_CUSTOM_SCHEDULE(CNPC_WScanner,	SCHED_WSCANNER_GET_HELP);
	ADD_CUSTOM_SCHEDULE(CNPC_WScanner,	SCHED_WSCANNER_CHASE_ENEMY);
	ADD_CUSTOM_SCHEDULE(CNPC_WScanner,	SCHED_WSCANNER_SHOOT_LASER);

	ADD_CUSTOM_ACTIVITY(CNPC_WScanner,	ACT_WSCANNER_JOIN);
	ADD_CUSTOM_ACTIVITY(CNPC_WScanner,	ACT_WSCANNER_FLINCH_BACK);
	ADD_CUSTOM_ACTIVITY(CNPC_WScanner,	ACT_WSCANNER_FLINCH_FRONT);
	ADD_CUSTOM_ACTIVITY(CNPC_WScanner,	ACT_WSCANNER_FLINCH_LEFT);
	ADD_CUSTOM_ACTIVITY(CNPC_WScanner,	ACT_WSCANNER_FLINCH_RIGHT);

	g_interactionWScannerBomb = CBaseCombatCharacter::GetInteractionID();

	CNPC_WScanner::m_nNumAttackers   = 0;

	AI_LOAD_SCHEDULE(CNPC_WScanner,	SCHED_WSCANNER_ATTACK_HOVER);
	AI_LOAD_SCHEDULE(CNPC_WScanner,	SCHED_WSCANNER_PATROL);
	AI_LOAD_SCHEDULE(CNPC_WScanner,	SCHED_WSCANNER_GET_HELP);
	AI_LOAD_SCHEDULE(CNPC_WScanner,	SCHED_WSCANNER_CHASE_ENEMY);
	AI_LOAD_SCHEDULE(CNPC_WScanner,	SCHED_WSCANNER_SHOOT_LASER);
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this NPC's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_WScanner::Classify(void)
{
	return(CLASS_WASTE_SCANNER);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_WScanner::StopLoopingSounds(void)
{
	StopSound(entindex(), "NPC_WScanner.HoverAlarm" );
	StopSound(entindex(), "NPC_WScanner.Hover" );
	StopSound(entindex(), "NPC_WScanner.Alarm" );
	StopSound(entindex(), "NPC_WScanner.Split" );
	StopSound(entindex(), "NPC_WScanner.Attach" );
	StopSound(entindex(), "NPC_WScanner.LaserWarmUp" );
	StopSound(entindex(), "NPC_WScanner.LaserShoot" );
	BaseClass::StopLoopingSounds();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
int CNPC_WScanner::TranslateSchedule( int scheduleType ) 
{
	return BaseClass::TranslateSchedule(scheduleType);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_WScanner::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );
	LaserKill();
	ExplosionCreate(GetLocalOrigin(), GetLocalAngles(), NULL, random->RandomInt(5, 10), 0, true);
	UTIL_Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_WScanner::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( info.GetDamageType() & DMG_BULLET)
	{
		g_pEffects->Ricochet(ptr->endpos,ptr->plane.normal);
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//------------------------------------------------------------------------------
// Purpose: Override to split in two when attacked
//------------------------------------------------------------------------------
int CNPC_WScanner::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	// Don't die during joining process or remaining pods are undefined
	if (GetActivity() == ACT_WSCANNER_JOIN  &&
		info.GetDamage()		 >= m_iHealth			)
	{
		return 1;
	}

	// -------------------------------------
	//  If hit by stalker beam, blow up
	// -------------------------------------
	if (info.GetAttacker()->Classify() == CLASS_STALKER)
	{
		info.SetDamage( m_iHealth+1 );
	}
	// Split into two waste scanners
	else if (m_nWScannerState == WSCANNER_STATE_ATTACHED)
	{
		LaserKill();
		Split();
		SetSchedule(SCHED_WSCANNER_ATTACK_HOVER);
	}
	// Flinch in the direction of the attack
	else
	{
		float vAttackYaw = VecToYaw(g_vecAttackDir);

		float vAngleDiff = UTIL_AngleDiff( vAttackYaw, m_fHeadYaw );

		if		(vAngleDiff > -45 && vAngleDiff < 45)
		{
			SetActivity((Activity)ACT_WSCANNER_FLINCH_BACK);
		}
		else if (vAngleDiff < -45 && vAngleDiff > -135)
		{
			SetActivity((Activity)ACT_WSCANNER_FLINCH_LEFT);
		}
		else if (vAngleDiff >  45 && vAngleDiff <  135)
		{
			SetActivity((Activity)ACT_WSCANNER_FLINCH_RIGHT);
		}
		else
		{
			SetActivity((Activity)ACT_WSCANNER_FLINCH_FRONT);
		}
	}

	return (BaseClass::OnTakeDamage_Alive( info ));
}

//-----------------------------------------------------------------------------
// Purpose: Handles movement towards the last move target.
// Input  : flInterval - 
//-----------------------------------------------------------------------------

bool CNPC_WScanner::OverridePathMove( float flInterval )
{
	CBaseEntity *pMoveTarget = GetEnemy();

	Vector waypointDir = GetNavigator()->GetCurWaypointPos() - GetLocalOrigin();
	float flWaypointDist = VectorNormalize(waypointDir);

	// -----------------------------------------------------------------
	// Check route is blocked
	// ------------------------------------------------------------------
	Vector checkPos = GetLocalOrigin() + (waypointDir * (m_flSpeed * flInterval));

	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( NAV_FLY, GetLocalOrigin(), checkPos, MASK_NPCSOLID|CONTENTS_WATER, pMoveTarget, &moveTrace);
	if (IsMoveBlocked(moveTrace))
	{
		TaskFail(FAIL_NO_ROUTE);
		GetNavigator()->ClearGoal();
		return true;
	}

	// --------------------------------------------------
	
	if (GetNavigator()->CurWaypointIsGoal() && 
	    IsCurSchedule(SCHED_WSCANNER_GET_HELP) && 
	    flWaypointDist < 100)
	{
		// If I'm geting help allow a wide berth
		TaskMovementComplete();
		return true;
	}

	// --------------------------------------------------
	//  Check if I've reached my goal
	// --------------------------------------------------
	
	Vector lastPatrolDir = GetNavigator()->GetCurWaypointPos() - GetLocalOrigin();
	
	if ( ProgressFlyPath( flInterval, GetEnemy(), MASK_NPCSOLID|CONTENTS_WATER,
						  !IsCurSchedule( SCHED_WSCANNER_PATROL ) ) == AINPP_COMPLETE )
	{
		if (IsCurSchedule( SCHED_WSCANNER_PATROL ))
		{
			m_vLastPatrolDir = lastPatrolDir;
			VectorNormalize(m_vLastPatrolDir);
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

bool CNPC_WScanner::OverrideMove(float flInterval)
{
	// -----------------------------------------------------------------
	// If I have a route, keep it updated and move toward target
	// ------------------------------------------------------------------
	if (GetNavigator()->IsGoalActive())
	{
		if ( OverridePathMove( flInterval ) )
			return true;
	}

	// ----------------------------------------------
	//	If attacking 
	// ----------------------------------------------
	else if (	GetEnemy()			!= NULL && 
				m_nWScannerState	!= WSCANNER_STATE_GETTING_HELP)
	{
		MoveToAttack(flInterval);
	}
	// -----------------------------------------------------------------
	// If I don't have a route, just decelerate
	// -----------------------------------------------------------------
	else if (!GetNavigator()->IsGoalActive())
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

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_WScanner::MoveToTarget(float flInterval, const Vector &MoveTarget)
{
	// -------------------------------------
	// Move towards are target
	// -------------------------------------
	Vector targetDir = MoveTarget - GetLocalOrigin();
	VectorNormalize(targetDir);

	// ---------------------------------------
	//  If I have an enemy attempt to evade it
	// ---------------------------------------
	CBaseCombatCharacter *pEnemy  = GetEnemyCombatCharacterPointer();
	if (pEnemy)
	{
		targetDir = targetDir + 0.2 * MoveToEvade(pEnemy);
	}

	const float	myAccel	 = 300.0;
	const float	myDecay	 = 0.35; // decay to 35% in 1 second
	TurnHeadToTarget( flInterval, MoveTarget );
	MoveInDirection(flInterval, targetDir, myAccel, (2 * myAccel), myDecay);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CNPC_WScanner::MoveToEvade(CBaseCombatCharacter *pEnemy)
{
	// Don't evade if I just shot a grenade
	if (gpGlobals->curtime < m_fNextMoveEvadeTime)
	{
		return vec3_origin;
	}
	// -----------------------------------------
	//  Keep out of enemy's shooting position
	// -----------------------------------------
	Vector vEnemyFacing = pEnemy->BodyDirection2D( );
	Vector	vEnemyDir   = pEnemy->EyePosition() - GetLocalOrigin();
	VectorNormalize(vEnemyDir);
	float  fDotPr		= DotProduct(vEnemyFacing,vEnemyDir);

	if (fDotPr < -0.95)
	{
		Vector vDirUp(0,0,1);
		Vector vDir;
		CrossProduct( vEnemyFacing, vDirUp, vDir);

		Vector crossProduct;
		CrossProduct(vEnemyFacing, vEnemyDir, crossProduct);
		if (crossProduct.y < 0)
		{
			vDir = vDir * -1;
		}
		return (2*vDir);
	}
	else if (fDotPr < -0.9)
	{
		Vector vDirUp(0,0,1);
		Vector vDir;
		CrossProduct( vEnemyFacing, vDirUp, vDir);

		Vector crossProduct;
		CrossProduct(vEnemyFacing, vEnemyDir, crossProduct);
		if (crossProduct.y < 0)
		{
			vDir = vDir * -1;
		}
		return (vDir);
	}
	else if (fDotPr < -0.85)
	{
		Vector vDirUp(0,0,1);
		Vector vDir;
		CrossProduct( vEnemyFacing, vDirUp, vDir);

		Vector crossProduct;
		CrossProduct(vEnemyFacing, vEnemyDir, crossProduct);
		if (random->RandomInt(0,1))
		{
			vDir = vDir * -1;
		}
		return (vDir);
	}
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_WScanner::MoveToAttack(float flInterval)
{
	CBaseCombatCharacter* pEnemy  = GetEnemyCombatCharacterPointer();

	if (!pEnemy)
	{
		return;
	}

	Vector vFlyDirection = vec3_origin;


	// -----------------------------------------
	//  Keep within range of enemy
	// -----------------------------------------
	Vector	vEnemyDir  = pEnemy->EyePosition() - GetLocalOrigin();
	float	fEnemyDist = VectorNormalize(vEnemyDir);
	if (fEnemyDist < WSCANNER_HOVER_NEAR_DIST)
	{
		vFlyDirection = -vEnemyDir;
	}
	else if (fEnemyDist > WSCANNER_HOVER_FAR_DIST)
	{
		vFlyDirection = vEnemyDir;
	}
	else if (gpGlobals->curtime > m_fNextGrenadeTime)
	{
		if (m_nPodSize			!= 3						||
			m_nWScannerState	!= WSCANNER_STATE_ATTACHED	)
		{
			TossGrenade();
		}
	}


	// -------------------------------------
	//	Add some evasion
	// -------------------------------------
	vFlyDirection = vFlyDirection + MoveToEvade(pEnemy);

	// -------------------------------------
	// Set net velocity 
	// -------------------------------------
	float myAccel = 300.0;
	float myDecay = 0.35; // decay to 35% in 1 second
	MoveInDirection( flInterval, vFlyDirection, myAccel, 2 * myAccel, myDecay);
		
	TurnHeadToTarget( flInterval, GetEnemy()->GetLocalOrigin() );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_WScanner::PlayAlarmSound(void)
{
	if (gpGlobals->curtime > m_fNextAlarmSoundTime)
	{
		if (!m_pHintNode)
		{
			return;
		}
		m_pLightGlow = CSprite::SpriteCreate( "sprites/glow01.vmt", GetLocalOrigin(), FALSE );
		m_pLightGlow->SetTransparency( kRenderGlow, 255, 200, 200, 0, kRenderFxNoDissipation );
		m_pLightGlow->SetAttachment( this, 2 );
		m_pLightGlow->SetBrightness( 255 );
		m_pLightGlow->SetColor(255,0,0);
		m_pLightGlow->SetScale( 0.7 );
		m_pLightGlow->Expand( 0, 10 );
		m_pLightGlow->AnimateForTime( 1, 0.2 );
		m_pLightGlow = NULL;

		EmitSound( "NPC_WScanner.Alarm" );

		// Alarm sound and light frequency more often as wscanner nears join point
		Vector vHintPos;
		m_pHintNode->GetPosition(this,&vHintPos);

		float fHintDist		= (vHintPos - GetLocalOrigin()).Length();
		float fDelayTime	= fHintDist/2500;

		if (fDelayTime > 3.0) fDelayTime = 3.0;
		if (fDelayTime < 1.0) fDelayTime = 1.0;
		m_fNextAlarmSoundTime	= gpGlobals->curtime + fDelayTime;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_WScanner::PlayFlySound(void)
{
	if (gpGlobals->curtime > m_fNextFlySoundTime)
	{
		switch (m_nWScannerState)
		{
			case WSCANNER_STATE_SINGLE:
			case WSCANNER_STATE_ATTACHED:
			{
				EmitSound( "NPC_WScanner.Hover" );
				break;
			}
			case WSCANNER_STATE_GETTING_HELP:
			{
				EmitSound( "NPC_WScanner.HoverAlarm" );
				break;
			}
		}
		m_fNextFlySoundTime	= gpGlobals->curtime + 1.0;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_WScanner::AdjustAntenna(float flInterval)
{	
	if (m_fAntennaPos != m_fAntennaTarget)
	{
		// Make frame rate independent
		float timeToUse = flInterval;
		float iRate	= 0.8;
		while (timeToUse > 0)
		{
			m_fAntennaPos = (iRate * m_fAntennaPos) + (1-iRate)*m_fAntennaTarget;
			timeToUse =- 0.1;
		}
		SetBoneController( 1, m_fAntennaPos );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CNPC_WScanner::MinGroundDist(void)
{
	return WSCANNER_MIN_GROUND_DIST;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_WScanner::SetBanking(float flInterval)
{
	// Make frame rate independent
	float iRate = 0.5;
	float timeToUse = flInterval;
	while (timeToUse > 0)
	{
		Vector vFlyDirection	= GetCurrentVelocity();
		VectorNormalize(vFlyDirection);
		
		// If I'm shooting a grenade don't bank
		if (GetActivity() == ACT_RANGE_ATTACK1)
		{
			vFlyDirection		= vec3_origin;
		}
		else if (m_nWScannerState == WSCANNER_STATE_ATTACHED)
		{
			vFlyDirection		= vFlyDirection * WSCANNER_PAIR_BANK_RATE;
		}
		else
		{
			vFlyDirection		= vFlyDirection * WSCANNER_SINGLE_BANK_RATE;
		}

		Vector vBankDir;
		if (m_bBankFace && GetEnemy()!=NULL)
		{
			// Bank to face enemy
			Vector vRawDir = GetEnemy()->EyePosition() - GetLocalOrigin();
			VectorNormalize( vRawDir );
			vRawDir *= 50; 

			// Account for head rotation
			vBankDir.x = vRawDir.z * -cos(m_fHeadYaw*M_PI/180);
			vBankDir.z = vRawDir.z *  sin(m_fHeadYaw*M_PI/180);
		}
		else
		{
			// Bank based on fly direction
			vBankDir = vFlyDirection;
		}

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
void CNPC_WScanner::MoveExecute_Alive(float flInterval)
{
	// -------------------------------------------
	//  Avoid obstacles
	// -------------------------------------------
	SetCurrentVelocity( GetCurrentVelocity() + VelocityToAvoidObstacles(flInterval) );

	// ----------------------------------------------------------------------------------------
	// Add time-coherent noise to the current velocity so that it never looks bolted in place.
	// ----------------------------------------------------------------------------------------
	float noiseScale = 7.0;
	if ((CBaseEntity*)GetEnemy() && (GetLocalOrigin() - GetEnemy()->GetLocalOrigin()).Length2D() < 80.0) 
	{
		// Less noise when I'm close to enemy (attacking)
		noiseScale = 2.0;
	}

	AddNoiseToVelocity( noiseScale );
	
	// ---------------------
	//  Limit overall speed
	// ---------------------
	LimitSpeed( 200 );

	SetBanking(flInterval);

	// -------------------------------------------------------
	// If travelling on a path and getting help play an alarm
	// -------------------------------------------------------
	if (m_nWScannerState == WSCANNER_STATE_GETTING_HELP &&
		GetNavigator()->IsGoalActive()			)
	{
		PlayAlarmSound();
	}
	PlayFlySound();
	AdjustAntenna(flInterval);
	WalkMove( GetCurrentVelocity() * flInterval, MASK_NPCSOLID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_WScanner::Precache(void)
{

	//
	// Model.
	//
	engine->PrecacheModel("models/wscanner_single.mdl");
	engine->PrecacheModel("models/wscanner_pair.mdl");
	engine->PrecacheModel("models/wscanner_tri.mdl");
	
	engine->PrecacheModel("sprites/lgtning.vmt");	
	engine->PrecacheModel("sprites/glow01.vmt");

	UTIL_PrecacheOther("grenade_scanner");

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_WScanner::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_LASER_WARMUP:
		{
			// -------------------
			//  Adjust beams
			// -------------------
			int i;
			float fScale = (1-(m_flWaitFinished - gpGlobals->curtime)/WSCANNER_LASER_WARMUP_TIME);
			for (i=0;i<WSCANNER_NUM_BEAMS-1;i++)
			{
				if (m_pBeam[i])
				{
					m_pBeam[i]->SetBrightness(255*fScale);
					m_pBeam[i]->SetColor( 255, 255*fScale, 255*fScale );
				}
			}

			// -------------------
			//  Adjust glow
			// -------------------
			for (i=0;i<WSCANNER_NUM_GLOWS-1;i++)
			{
				if (m_pLaserGlow[i])
				{
					m_pLaserGlow[i]->SetColor( 255, 255*fScale, 255*fScale );
					m_pLaserGlow[i]->SetBrightness( 100 + (155*fScale) );
				}
			}

			if ( gpGlobals->curtime >= m_flWaitFinished )
			{
				TaskComplete();
			}
			break;
		}
		case TASK_WSCANNER_JOIN:
		{
			if ( GetActivity() != ACT_WSCANNER_JOIN )
			{
				Join();
				SetWScannerState(WSCANNER_STATE_ATTACHED);
				TaskComplete();
			}
			else
			{
				float fScale = 255*((gpGlobals->curtime - m_fStartJoinTime)/WSCANNER_JOIN_TIME);
				if (fScale > 255) fScale = 255;

				for (int i=0;i<WSCANNER_NUM_BEAMS-1;i++)
				{
					if (m_pBeam[i])
					{
						m_pBeam[i]->SetBrightness(fScale);
					}
				}
			}
			break;
		}
		// If my enemy has moved significantly, update my path
		case TASK_WAIT_FOR_MOVEMENT:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if (pEnemy														 &&
				IsCurSchedule(SCHED_WSCANNER_CHASE_ENEMY) && 
				GetNavigator()->IsGoalActive()												 )
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
			else if (IsCurSchedule(SCHED_WSCANNER_GET_HELP) && m_pHintNode)
			{
				Vector vHintPos;
				m_pHintNode->GetPosition(this,&vHintPos);

				float fHintDist = (vHintPos - GetLocalOrigin()).Length();
				if (fHintDist < 100)
				{
					TaskComplete();
					GetNavigator()->ClearGoal();		// Stop moving
					break;
				}
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
void CNPC_WScanner::Spawn(void)
{
	Precache();

	// Pod size comes in 0 based, we want it one based
	m_nPodSize++;

	if (m_nPodSize == 1)
	{
		SetModel( "models/wscanner_single.mdl" );
		m_nWScannerState = WSCANNER_STATE_SINGLE;
	}
	else if (m_nPodSize == 2)
	{
		SetModel( "models/wscanner_pair.mdl" );
		m_nWScannerState = WSCANNER_STATE_ATTACHED;
	}
	else
	{
		SetModel( "models/wscanner_tri.mdl" );
		m_nWScannerState = WSCANNER_STATE_ATTACHED;
	}

	SetHullType( HULL_SMALL_CENTERED ); 
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetMoveType( MOVETYPE_STEP );
	SetGravity(0.001);

	m_iHealth			= sk_wscanner_health.GetFloat();
	SetViewOffset( Vector(0, 0, 10) );		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_NPCState			= NPC_STATE_NONE;
	SetBloodColor( DONT_BLEED );
	SetCurrentVelocity( vec3_origin );
	m_vCurrentBanking	= vec3_origin;
	SetNavType(NAV_FLY);
	AddFlag( FL_FLY );

	CapabilitiesAdd( bits_CAP_MOVE_FLY );
	CapabilitiesAdd( bits_CAP_SQUAD );

	m_bBankFace					= false;

	m_vLastPatrolDir			= vec3_origin;
	m_fNextGrenadeTime			= 0;
	m_fNextLaserTime			= 0;
	m_fNextMoveEvadeTime		= 0;
	m_fNextAlarmSoundTime		= 0;
	m_fNextFlySoundTime			= 0;
	m_fSwitchToHelpTime			= 0;

	int i;
	for (i=0;i<WSCANNER_NUM_BEAMS;i++)
	{
		m_pBeam[i] = NULL;
	}
	for (i=0;i<WSCANNER_NUM_GLOWS;i++)
	{
		m_pLaserGlow[i] = NULL;
	}

	SetNoiseMod( 2, 2, 2 );

	m_fHeadYaw			= 0;
	m_fAntennaPos		= 0;
	m_fAntennaTarget	= 0;

	// Put this guy into a squad by defualt if not in a squad
	// @TODO (toml 12-05-02): RECONCILE WITH SAVE/RESTORE	
	if (!m_pSquad)
	{
		// Create a random squad name
		char pSquadName[255];
		Q_snprintf(pSquadName,sizeof(pSquadName),"WScannerSquad%i%i",random->RandomInt(0,10),random->RandomInt(0,10));
		m_pSquad = g_AI_SquadManager.FindCreateSquad(this, AllocPooledString(pSquadName));
	}

	NPCInit();

	// Let this guy look far
	m_flDistTooFar = 99999999.0;
	SetDistLook( 4000.0 );
	m_flSpeed = WSCANNER_MAX_SPEED;

	// Keep track of the number of live WScanners
	CNPC_WScanner::m_nNumAttackers++;
}


void CNPC_WScanner::OnScheduleChange()
{
	m_flSpeed = WSCANNER_MAX_SPEED;

	BaseClass::OnScheduleChange();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_WScanner::SetWScannerState(WScannerState_t pNewState)
{
	m_nWScannerState = pNewState;
}

//------------------------------------------------------------------------------
// Purpose : Get pod split positions
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_WScanner::SplitPos(int nPos)
{
	// Only 4 attachment points are available, so use shooting attachment 
	// points to determine split position and offset from model center
	if (m_nPodSize == 3)
	{
		Vector vAttachPos;
		QAngle vAttachAng;
		GetAttachment( nPos, vAttachPos, vAttachAng );
		Vector vDir			= 2*(vAttachPos - GetLocalOrigin());
		vAttachPos			= GetLocalOrigin() + vDir;
		return vAttachPos;
	}
	else
	{
		Vector vForward,vRight,vUp;
		AngleVectors( GetLocalAngles(), &vForward, &vRight, &vUp );
		if (nPos == WSCANNER_ATTACHMENT_TRI_RIGHT)
		{
			return GetLocalOrigin() - vRight * 35;
		}
		else if (nPos == WSCANNER_ATTACHMENT_TRI_LEFT)
		{
			return GetLocalOrigin() + vRight * 35;
		}
		else
		{
			return GetLocalOrigin() + vUp * 35;
		}
	}
}

//------------------------------------------------------------------------------
// Purpose : Waste Scanner splits into two
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_WScanner::Split(void)
{
	Vector vLeftAttach  = SplitPos(WSCANNER_ATTACHMENT_TRI_LEFT);
	Vector vRightAttach	= SplitPos(WSCANNER_ATTACHMENT_TRI_RIGHT);
	Vector vTopAttach	= SplitPos(WSCANNER_ATTACHMENT_TRI_TOP);

	// ----------------------------------
	// Change my own model and position
	// ----------------------------------
	SetModel( "models/wscanner_single.mdl" );
	SetHullSizeNormal();
	SetLocalOrigin( vLeftAttach );
	SetWScannerState(WSCANNER_STATE_SINGLE);
	// Don't go for help or grenade attack right away
	m_fSwitchToHelpTime = gpGlobals->curtime + 3.0;  
	m_fNextGrenadeTime = gpGlobals->curtime  + random->RandomFloat(1.3,1.7);  

	// ----------------------------------
	// Create second pod
	// ----------------------------------
	CNPC_WScanner *pScanner		= (CNPC_WScanner*) CreateEntityByName( "npc_wscanner" );

	// Add to squad before spawning
	m_pSquad->AddToSquad(pScanner);
	pScanner->Spawn();
	pScanner->SetWScannerState(WSCANNER_STATE_SINGLE);
	pScanner->SetModel( "models/wscanner_single.mdl" );

	//<<TEMP>> until yahn fixed interp problem
	pScanner->SetMoveType( MOVETYPE_FLY );

	pScanner->SetLocalOrigin( vRightAttach );
	pScanner->SetLocalAngles( GetLocalAngles() );
	pScanner->SetCurrentVelocity( GetCurrentVelocity() );
	pScanner->m_fHeadYaw			= m_fHeadYaw;
	pScanner->m_vCurrentBanking		= m_vCurrentBanking;
	pScanner->SetEnemy( GetEnemy() );
	pScanner->m_nPodSize			= m_nPodSize;
	pScanner->SetState(NPC_STATE_COMBAT);
	pScanner->SetHullSizeNormal();
	pScanner->m_fSwitchToHelpTime = gpGlobals->curtime + 3.0;	// Don't go for help right away
	pScanner->m_fNextGrenadeTime = gpGlobals->curtime  + random->RandomFloat(1.3,1.7); 

	// ----------------------------------
	// Create third pod
	// ----------------------------------
	if (m_nPodSize == 3)
	{
		pScanner = (CNPC_WScanner*) CreateEntityByName( "npc_wscanner" );

		// Add to squad before spawning
		m_pSquad->AddToSquad(pScanner);
		pScanner->Spawn();
		pScanner->SetWScannerState(WSCANNER_STATE_SINGLE);
		pScanner->SetModel( "models/wscanner_single.mdl" );

		//<<TEMP>> until yahn fixed interp problem
		pScanner->SetMoveType( MOVETYPE_FLY );

		pScanner->SetLocalOrigin( vTopAttach );
		pScanner->SetLocalAngles( GetLocalAngles() );
		pScanner->SetCurrentVelocity( GetCurrentVelocity() );
		pScanner->m_fHeadYaw			= m_fHeadYaw;
		pScanner->m_vCurrentBanking		= m_vCurrentBanking;
		pScanner->SetEnemy( GetEnemy() );
		pScanner->m_nPodSize			= m_nPodSize;
		pScanner->SetState(NPC_STATE_COMBAT);
		pScanner->SetHullSizeNormal();
		pScanner->m_fSwitchToHelpTime	= gpGlobals->curtime + 3.0;	// Don't go for help right away
		pScanner->m_fNextGrenadeTime	= gpGlobals->curtime + random->RandomFloat(1.3,1.7); 
		
		// Play detach sound
		EmitSound( "NPC_WScanner.Split" );
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_WScanner::UpdateSquadHelper(CBaseEntity* pHelper)
{
	AISquadIter_t iter;
	for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
	{
		CNPC_WScanner *pWScanner = dynamic_cast<CNPC_WScanner*>(pSquadMember);
		Assert( pWScanner != NULL);
		pWScanner->m_hHelper = pHelper;
	}
}

//------------------------------------------------------------------------------
// Purpose : Waste Scanner gets other half
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_WScanner::Join(void)
{
	for (int i=0;i<WSCANNER_NUM_BEAMS;i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = NULL;
		}
	}

	// Retract antenna
	m_fAntennaTarget = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Gets the appropriate next schedule based on current condition
//			bits.
//-----------------------------------------------------------------------------
int CNPC_WScanner::SelectSchedule(void)
{
	 //<<TEMP>> waiting for yahn interp fix
	if (GetMoveType() == MOVETYPE_FLY)
	{
		SetMoveType( MOVETYPE_STEP );
		Relink();
	}

	// Kill laser if its still on
	LaserKill();
	m_fAntennaTarget = 0;

	switch ( m_NPCState )
	{
		case NPC_STATE_IDLE:
		{
			return SCHED_WSCANNER_PATROL;
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
			// ------------------------------------------------
			//  If I'm attached:
			// ------------------------------------------------
			if (m_nWScannerState == WSCANNER_STATE_ATTACHED)
			{
				// ----------------------------------------------------------
				//  If I have an enemy
				// ----------------------------------------------------------
				if (GetEnemy() != NULL	&& 
					GetEnemy()->IsAlive()	)
				{
					// --------------------------------------------
					//  Track the enemy
					// --------------------------------------------
					if (HasCondition(COND_SEE_ENEMY))
					{
						float	fEnemyDist = (GetEnemy()->EyePosition() - GetLocalOrigin()).Length();
						if (m_nPodSize == 3 &&
							gpGlobals->curtime > m_fNextLaserTime && fEnemyDist < 1000)
						{
							return SCHED_WSCANNER_SHOOT_LASER;
						}
						else
						{
							return SCHED_WSCANNER_ATTACK_HOVER;
						}
					}
					else
					{
						return SCHED_WSCANNER_CHASE_ENEMY;
					}
				}
				else
				{
					return SCHED_WSCANNER_PATROL;
				}
				
			}
			// --------------------------------------------------
			//  If I'm attacking		
			// --------------------------------------------------
			else if (m_nWScannerState == WSCANNER_STATE_SINGLE)
			{
				// --------------------------------------------------
				//  Otherwise hover and attack
				// --------------------------------------------------
				if (GetEnemy() != NULL && GetEnemy()->IsAlive())
				{
					// --------------------------------------------------------------
					// If pod size isn't single, running out of help and no helper
					// --------------------------------------------------------------
					if (m_nPodSize > 1								&&
						gpGlobals->curtime > m_fSwitchToHelpTime		&&
						CNPC_WScanner::m_nNumAttackers < 5			&& 
						(m_hHelper == NULL || m_hHelper == this)	)
					{
						UpdateSquadHelper(this);
						SetWScannerState(WSCANNER_STATE_GETTING_HELP);
						return SCHED_WSCANNER_GET_HELP;
					}
					if (HasCondition(COND_SEE_ENEMY))
					{
						return SCHED_WSCANNER_ATTACK_HOVER;
					}
					else
					{
						return SCHED_WSCANNER_CHASE_ENEMY;
					}
				}
				else
				{
					return SCHED_WSCANNER_PATROL;
				}
			}
			// --------------------------------------------------
			//  If I'm getting help
			// --------------------------------------------------
			if (m_nWScannerState == WSCANNER_STATE_GETTING_HELP)
			{
				// If enemy is dead, go back to patrol
				if ( GetEnemy() == NULL		|| 
					!GetEnemy()->IsAlive()	)
				{
					SetWScannerState(WSCANNER_STATE_SINGLE);
					return SCHED_WSCANNER_PATROL;
				}
				if (CNPC_WScanner::m_nNumAttackers > 4)
				{
					UpdateSquadHelper(NULL);
					SetWScannerState(WSCANNER_STATE_SINGLE);
					return SCHED_WSCANNER_ATTACK_HOVER;
				}
				return SCHED_WSCANNER_GET_HELP;
			}
			return SCHED_FAIL;
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTask - 
//-----------------------------------------------------------------------------
void CNPC_WScanner::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{	
		// Create
		case TASK_LASER_WARMUP:
		{
			LaserWarmup();
			// set a future time that tells us when the warmup is over.
			m_flWaitFinished = gpGlobals->curtime + WSCANNER_LASER_WARMUP_TIME;
			break;
		}
		case TASK_WSCANNER_RAISE_ANTENNA:
		{
			m_fAntennaTarget	= 24;
			TaskComplete();
			break;
		}
		case TASK_LASER_SHOOT:
		{
			LaserShoot();
			TaskComplete();
			break;
		}
		case TASK_WSCANNER_JOIN:
		{
			// If I'm not right next to my hint node bail out
			AI_PathNode_t node = GetNavigator()->GetNearestNode();
			if (node == AIN_NO_NODE)
			{
				TaskFail(FAIL_NO_REACHABLE_NODE);
				return;
			}
			float flNodeDist = (GetNavigator()->GetNodePos(node) - GetLocalOrigin()).Length();
			if (flNodeDist > WSCANNER_JOIN_DIST) 
			{
				m_pHintNode->Unlock(0);
				TaskFail("Not in split position");
				return;
			}
			else 
			{
				// Change my model
				if (m_nPodSize == 2)
				{
					SetModel( "models/wscanner_pair.mdl" );
				}
				else if (m_nPodSize == 3)
				{
					SetModel( "models/wscanner_tri.mdl" );
				}
				SetHullSizeNormal();
				Relink();

				// Start beams a-going
				StartJoinBeams();

				// Play join activity
				SetActivity((Activity)ACT_WSCANNER_JOIN);
			}
			break;
		}
		// Override so can find hint nodes that are much further away
		case TASK_FIND_HINTNODE:
		{
			if (!m_pHintNode)
			{
				m_pHintNode = CAI_Hint::FindHint( this, HINT_NONE, pTask->flTaskData, 5000 );
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
void CNPC_WScanner::StartJoinBeams(void)
{
	// --------------------------
	//  Blue beams between pods
	// --------------------------
	if (m_nPodSize == 3)
	{
		m_pBeam[0] = CBeam::BeamCreate( "sprites/lgtning.vmt", 2.0 );
		m_pBeam[0]->SetColor( 50, 50, 255 ); 
		m_pBeam[0]->SetBrightness( 0 );
		m_pBeam[0]->SetNoise( 100 );
		m_pBeam[0]->SetWidth( 3.0 );
		m_pBeam[0]->EntsInit( this, this );
		m_pBeam[0]->SetStartAttachment( 1 );
		m_pBeam[0]->SetEndAttachment( 2 );	

		m_pBeam[1] = CBeam::BeamCreate( "sprites/lgtning.vmt", 2.0 );
		m_pBeam[1]->SetColor( 50, 50, 255 ); 
		m_pBeam[1]->SetBrightness( 0 );
		m_pBeam[1]->SetNoise( 16 );
		m_pBeam[1]->SetWidth( 3.0 );
		m_pBeam[1]->EntsInit( this, this );
		m_pBeam[1]->SetStartAttachment( 2 );
		m_pBeam[1]->SetEndAttachment( 3 );

		m_pBeam[2] = CBeam::BeamCreate( "sprites/lgtning.vmt", 2.0 );
		m_pBeam[2]->SetColor( 50, 50, 255 ); 
		m_pBeam[2]->SetBrightness( 0 );
		m_pBeam[2]->SetNoise( 16 );
		m_pBeam[2]->SetWidth( 3.0 );
		m_pBeam[2]->EntsInit( this, this );
		m_pBeam[2]->SetStartAttachment( 1 );
		m_pBeam[2]->SetEndAttachment( 2 );	

		m_pBeam[3] = CBeam::BeamCreate( "sprites/lgtning.vmt", 2.0 );
		m_pBeam[3]->SetColor( 50, 50, 255 ); 
		m_pBeam[3]->SetBrightness( 0 );
		m_pBeam[3]->SetWidth( 3.0 );
		m_pBeam[3]->SetNoise( 16 );
		m_pBeam[3]->EntsInit( this, this );
		m_pBeam[3]->SetStartAttachment( 2 );
		m_pBeam[3]->SetEndAttachment( 3 );
	}
	else	
	{
		m_pBeam[0] = CBeam::BeamCreate( "sprites/lgtning.vmt", 2.0 );
		m_pBeam[0]->SetColor( 50, 50, 255 ); 
		m_pBeam[0]->SetBrightness( 0 );
		m_pBeam[0]->SetNoise( 100 );
		m_pBeam[0]->SetWidth( 3.0 );
		m_pBeam[0]->EntsInit( this, this );
		m_pBeam[0]->SetStartAttachment( 2 );
		m_pBeam[0]->SetEndAttachment( 3 );

		m_pBeam[1] = CBeam::BeamCreate( "sprites/lgtning.vmt", 2.0 );
		m_pBeam[1]->SetColor( 50, 50, 255 ); 
		m_pBeam[1]->SetBrightness( 0 );
		m_pBeam[1]->SetNoise( 16 );
		m_pBeam[1]->SetWidth( 3.0 );
		m_pBeam[1]->EntsInit( this, this );
		m_pBeam[1]->SetStartAttachment( 2 );
		m_pBeam[1]->SetEndAttachment( 3 );
	}

	// Play with no attenuation so player can here from whereever
	EmitSound( "NPC_WScanner.Attach" );

	m_fStartJoinTime = gpGlobals->curtime;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_WScanner::LaserWarmup(void)
{
	// -------------------
	//  Converging beams
	// -------------------
	int i;
	for (i=0;i<WSCANNER_NUM_BEAMS-1;i++)
	{
		m_pBeam[i] = CBeam::BeamCreate( "sprites/lgtning.vmt", 2.0 );
		m_pBeam[i]->SetColor( 255, 0, 0 ); 
		m_pBeam[i]->SetBrightness( 100 );
		m_pBeam[i]->SetNoise( 8 );
		m_pBeam[i]->EntsInit( this, this );
		m_pBeam[i]->SetStartAttachment( i+1 );
		m_pBeam[i]->SetEndAttachment( 4 );
	}

	// -------------
	//  Glow
	// -------------
	for (i=0;i<WSCANNER_NUM_GLOWS-1;i++)
	{
		m_pLaserGlow[i] = CSprite::SpriteCreate( "sprites/glow01.vmt", GetLocalOrigin(), FALSE );
		m_pLaserGlow[i]->SetAttachment( this, 4 );
		m_pLaserGlow[i]->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
		m_pLaserGlow[i]->SetBrightness( 100 );
		m_pLaserGlow[i]->SetScale( 0.3 );
	}

	EmitSound( "NPC_WScanner.LaserWarmUp" );

	// Bank to face player
	m_bBankFace = true;

	// Don't shoot laser again for a while
	m_fNextLaserTime = gpGlobals->curtime + 10;

	// After firing sit still for a second to make easier to hit
	SetCurrentVelocity( vec3_origin );
	m_fNextMoveEvadeTime = gpGlobals->curtime + WSCANNER_LASER_WARMUP_TIME;

}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_WScanner::LaserShoot(void)
{
	if (GetEnemy() != NULL)
	{
		Vector shootPos = Weapon_ShootPosition();
		Vector vTargetDir = (GetEnemy()->EyePosition() - shootPos);
		vTargetDir.z -= 12;  // A little below the eyes

		trace_t tr;
		AI_TraceLine ( shootPos, vTargetDir * 2048, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

		CBaseEntity *pEntity = tr.m_pEnt;
		if (pEntity != NULL && m_takedamage)
		{
			ClearMultiDamage();
			CTakeDamageInfo info( this, this, sk_wscanner_laser_dmg.GetFloat(), DMG_SHOCK );
			CalculateMeleeDamageForce( &info, vTargetDir, tr.endpos );
			pEntity->DispatchTraceAttack( info, vTargetDir, &tr );
			ApplyMultiDamage();
		}

		// -------------
		//  Shoot beam
		// -------------
		int nBeamNum = WSCANNER_NUM_BEAMS-1;
		m_pBeam[nBeamNum] = CBeam::BeamCreate( "sprites/lgtning.vmt", 2.0 );
		m_pBeam[nBeamNum]->SetColor( 255, 255, 255 );
		m_pBeam[nBeamNum]->PointEntInit( tr.endpos, this );
		m_pBeam[nBeamNum]->SetEndAttachment( 4 );  
		m_pBeam[nBeamNum]->SetBrightness( 255 );
		m_pBeam[nBeamNum]->SetNoise( 0 );
		

		// -------------
		//  Impact Glow
		// -------------
		if (pEntity && pEntity->IsPlayer())
		{
			int nGlowNum = WSCANNER_NUM_GLOWS-1;
			m_pLaserGlow[nGlowNum] = CSprite::SpriteCreate( "sprites/glow01.vmt", tr.endpos, true );
			m_pLaserGlow[nGlowNum]->SetTransparency( kRenderGlow, 255, 200, 200, 0, kRenderFxNoDissipation );
			m_pLaserGlow[nGlowNum]->SetBrightness( 255 );
			m_pLaserGlow[nGlowNum]->SetScale( 0.8 );
			m_pLaserGlow[nGlowNum]->Expand( 10, 100 );
			m_pLaserGlow[nGlowNum] = NULL;
		}

		EmitSound( "NPC_WScanner.LaserShoot" );
	}

	for (int i=0;i<WSCANNER_NUM_BEAMS;i++)
	{
		if (m_pBeam[i])
		{
			// Let beam kill itself
			m_pBeam[i]->LiveForTime(0.2);
			m_pBeam[i] = NULL;
		}
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_WScanner::LaserKill(void)
{
	// -------------------------------
	//  Kill beams if not set to die
	// ------------------------------
	int i;
	for (i=0;i<WSCANNER_NUM_BEAMS;i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = NULL;
		}
	}

	// ---------------------
	//  Kill laser
	// ---------------------
	for (i=0;i<WSCANNER_NUM_GLOWS;i++)
	{
		if (m_pLaserGlow[i])
		{
			UTIL_Remove( m_pLaserGlow[i] );
			m_pLaserGlow[i] = NULL;
		}
	}

	// No longer bank to face player
	m_bBankFace = false;
 
	// Kill charge sound in case still going
	StopSound(entindex(), "NPC_WScanner.LaserWarmUp" );
}

//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific messages that occur when tagged
//			animation frames are played.
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CNPC_WScanner::HandleAnimEvent( animevent_t *pEvent )
{
	switch ( pEvent->event )
	{
		case WSCANNER_AE_SHOOT:
		{
			if (GetEnemy() != NULL)
			{
				Vector vShootPos;
				QAngle vShootAng;
				GetAttachment( WSCANNER_ATTACHMENT_SHOOT, vShootPos, vShootAng );
				
				EmitSound( "NPC_WScanner.ShootEvent" );

				Vector vTargetDir = (GetEnemy()->GetLocalOrigin() - vShootPos);
				vTargetDir.z += 400;
				CBaseGrenade *pGrenade = (CBaseGrenade*)CreateNoSpawn( "grenade_scanner", vShootPos, vec3_angle, this );
				pGrenade->KeyValue( "velocity", vTargetDir );
				pGrenade->Spawn( );
				pGrenade->SetOwner( this );
				pGrenade->SetOwnerEntity( this );

				if (GetEnemy()->GetFlags() & FL_NPC)
				{
					GetEnemy()->MyNPCPointer()->HandleInteraction( g_interactionWScannerBomb, pGrenade, this );
				}
			}
		}
		break;

		case WSCANNER_AE_ATTACH:
		{
			EmitSound( "NPC_WScanner.AttachEvent" );
			break;
		}

	default:
		{
			BaseClass::HandleAnimEvent( pEvent );
			break;
		}
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_WScanner::TossGrenade(void)
{
	// Play grenate toss activity (event launches actual grenade)
	SetActivity((Activity)ACT_RANGE_ATTACK1);

	m_fNextGrenadeTime = gpGlobals->curtime + random->RandomFloat(1.8,2.2); 

	// After firing sit still for a second to make easier to hit
	SetCurrentVelocity( vec3_origin );
	m_fNextMoveEvadeTime = gpGlobals->curtime + 1.5;

}

//-----------------------------------------------------------------------------
// Purpose: Tells use whether or not the NPC cares about a given type of hint node.
// Input  : sHint - 
// Output : TRUE if the NPC is interested in this hint type, FALSE if not.
//-----------------------------------------------------------------------------
bool CNPC_WScanner::FValidateHintType(CAI_Hint *pHint)
{
	return(pHint->HintType() == HINT_TACTICAL_SPAWN);
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CNPC_WScanner::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// Print state
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"WS State: ");
		switch (m_nWScannerState)
		{
			case WSCANNER_STATE_ATTACHED:			strcat(tempstr,"Attached");		break;
			case WSCANNER_STATE_SINGLE:				strcat(tempstr,"Single");		break;
			case WSCANNER_STATE_GETTING_HELP:		strcat(tempstr,"Getting Help");	break;
		}
		NDebugOverlay::EntityText(entindex(),text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CNPC_WScanner::~CNPC_WScanner(void)
{
	// Decrease count of scanners that are on the attack
	CNPC_WScanner::m_nNumAttackers--;
}


/*
==================================================================
	Attachment points for the wasteland scanner models
==================================================================
Pair:
Right scanner:
Center point = Dummy (Root)
Main Root Node = Base Dummy
Where bomb shoots from = BombStart
Bone that moves the big eye = Pupil Base
Speed particle type effect = SpeedParticles
Right scanner breaks off to = Break_Point_Right

Left scanner:
Center point = bDummy (Root)
Main Root Node = Base Dummy
Where bomb shoots from = bBombStart
Bone that moves the big eye = bPupil Base
Speed particle type effect = bSpeedParticles
Left scanner breaks off to = Break_Point_Left



Tri:
Right scanner:
Center point = Dummy (Root)
Main Root Node = Base Dummy
Where bomb shoots from = BombStart
Bone that moves the big eye = Pupil Base
Speed particle type effect = SpeedParticles
Right scanner breaks off to = Break_Point_Right
Electric eye pulse starts from = Pupilz

Left scanner:
Center point = cDummy (Root)
Main Root Node = Base Dummy
Where bomb shoots from = cBombStart
Bone that moves the big eye = cPupil Base
Speed particle type effect = cSpeedParticles
Left scanner breaks off to = Break_Point_Left
Electric eye pulse starts from = cPupilz

Middle:
Center point = bDummy (Root)
Main Root Node = Base Dummy
Where bomb shoots from = bBombStart
Bone that moves the big eye = bPupil Base
Speed particle type effect = bSpeedParticles
Top scanner breaks off to = Break_Point_Top
Electric eye pulse starts from = bPupilz

Electrical converge = beamconverge
*/


//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

//=========================================================
// > SCHED_WSCANNER_ATTACK_HOVER
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_WSCANNER_ATTACK_HOVER,

	"	Tasks"
	"		TASK_WAIT				5"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
);

//=========================================================
// > SCHED_WSCANNER_SHOOT_LASER
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_WSCANNER_SHOOT_LASER,

	"	Tasks"
	"		TASK_LASER_WARMUP		0"
	"		TASK_LASER_SHOOT		0"
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
);

//=========================================================
// > SCHED_WSCANNER_PATROL
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_WSCANNER_PATROL,

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
// > SCHED_WSCANNER_CHASE_ENEMY
//
//  Different interrupts than normal chase enemy.  
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_WSCANNER_CHASE_ENEMY,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_WSCANNER_PATROL"
	"		TASK_SET_TOLERANCE_DISTANCE			120"
	"		TASK_GET_PATH_TO_ENEMY				0"
	"		TASK_RUN_PATH						0"
	"		TASK_WAIT_FOR_MOVEMENT				0"
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
);

//=========================================================
// > SCHED_WSCANNER_GET_HELP
//
// Don't lock hit nodes and split rather than doing anim
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_WSCANNER_GET_HELP,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_WSCANNER_GET_HELP"
	"		TASK_WAIT_PVS					0 "
	"		TASK_FIND_HINTNODE				0 "
	"		TASK_GET_PATH_TO_HINTNODE		0 "
	"		TASK_SET_TOLERANCE_DISTANCE		0"
	"		TASK_WSCANNER_RAISE_ANTENNA		0"
	"		TASK_RUN_PATH					0 "
	"		TASK_WAIT_FOR_MOVEMENT			0 "
	"		TASK_STOP_MOVING				0"
	"		TASK_WSCANNER_JOIN				0"
	""
	"	Interrupts"
	"		COND_ENEMY_DEAD"
);