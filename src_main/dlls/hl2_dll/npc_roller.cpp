//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		This is the base version of the combine (not instanced only subclassed)
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "AI_Default.h"
#include "AI_Task.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Hint.h"
#include "AI_Navigator.h"
#include "soundent.h"
#include "game.h"
#include "NPCEvent.h"
#include "activitylist.h"
#include "AI_BaseNPC.h"
#include "AI_Node.h"
#include "ndebugoverlay.h"
#include "EntityList.h"
#include "NPC_Roller.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "physics_saverestore.h"


#define TIME_NEVER	1e9

#define ROLLER_UPRIGHT_SPEED	100
#define ROLLER_FORWARD_SPEED	-900.0
#define ROLLER_CHECK_DIST		24.0

#define ROLLER_MARCO_FREQUENCY	6

#define ROLLER_MARCOPOLO_DIST	300
#define ROLLER_NUM_FAILS		2

#define ROLLER_MAX_PUSH_MASS	50

#define ROLLER_BASH_FORCE		10000

#define ROLLER_DEBUG

static const char *pCodeSounds[] =
{
	"npc/roller/code1.wav",
	"npc/roller/code2.wav",
	"npc/roller/code3.wav",
	"npc/roller/code4.wav",
	"npc/roller/code5.wav",
	"npc/roller/code6.wav",
};


BEGIN_SIMPLE_DATADESC( CRollerController )

	DEFINE_FIELD( CRollerController, m_vecAngular, FIELD_VECTOR ),
	DEFINE_FIELD( CRollerController, m_vecLinear, FIELD_VECTOR ),
	DEFINE_FIELD( CRollerController, m_fIsStopped, FIELD_BOOLEAN ),

END_DATADESC()


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
IMotionEvent::simresult_e CRollerController::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	if( m_fIsStopped )
	{
		return SIM_NOTHING;
	}

	linear = m_vecLinear;
	angular = m_vecAngular;
	
	return IMotionEvent::SIM_LOCAL_ACCELERATION;
}


LINK_ENTITY_TO_CLASS( npc_roller, CNPC_Roller );
IMPLEMENT_CUSTOM_AI( npc_roller, CNPC_Roller );


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Roller )

	DEFINE_FIELD( CNPC_Roller, m_flTimeMarcoSound, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Roller, m_flTimePoloSound, FIELD_TIME ),
	DEFINE_FIELD( CNPC_Roller, m_fHACKJustSpawned, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Roller, m_vecUnstickDirection, FIELD_VECTOR ),
	DEFINE_FIELD( CNPC_Roller, m_flLastZPos, FIELD_FLOAT ),
	DEFINE_FIELD( CNPC_Roller, m_iFail, FIELD_INTEGER ),
	DEFINE_FIELD( CNPC_Roller, m_flForwardSpeed, FIELD_FLOAT ),
	DEFINE_ARRAY( CNPC_Roller, m_iAccessCode, FIELD_STRING,  ROLLER_CODE_DIGITS  ),
	DEFINE_FIELD( CNPC_Roller, m_iCodeProgress, FIELD_INTEGER ),
	DEFINE_EMBEDDED( CNPC_Roller, m_RollerController ),
	DEFINE_PHYSPTR( CNPC_Roller, m_pMotionController ),
	DEFINE_SOUNDPATCH( CNPC_Roller,  m_pRollSound ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Roller::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI( CNPC_Roller );

	ADD_CUSTOM_SCHEDULE( CNPC_Roller,	SCHED_ROLLER_PATROL );
	ADD_CUSTOM_SCHEDULE( CNPC_Roller,	SCHED_ROLLER_WAIT_FOR_PHYSICS );
	ADD_CUSTOM_SCHEDULE( CNPC_Roller,	SCHED_ROLLER_UNSTICK );

	ADD_CUSTOM_CONDITION( CNPC_Roller,	COND_ROLLER_PHYSICS );

	ADD_CUSTOM_TASK( CNPC_Roller,		TASK_ROLLER_WAIT_FOR_PHYSICS );
	ADD_CUSTOM_TASK( CNPC_Roller,		TASK_ROLLER_FIND_PATROL_NODE );
	ADD_CUSTOM_TASK( CNPC_Roller,		TASK_ROLLER_UNSTICK );
	ADD_CUSTOM_TASK( CNPC_Roller,		TASK_ROLLER_ON );
	ADD_CUSTOM_TASK( CNPC_Roller,		TASK_ROLLER_OFF );
	ADD_CUSTOM_TASK( CNPC_Roller,		TASK_ROLLER_ISSUE_CODE );

	AI_LOAD_SCHEDULE( CNPC_Roller,	SCHED_ROLLER_PATROL );
	AI_LOAD_SCHEDULE( CNPC_Roller,	SCHED_ROLLER_WAIT_FOR_PHYSICS );
	AI_LOAD_SCHEDULE( CNPC_Roller,	SCHED_ROLLER_UNSTICK );

#if 0
	ADD_CUSTOM_ACTIVITY(CNPC_Roller,	ACT_MYCUSTOMACTIVITY);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: cache all the pre's`
//
//
//-----------------------------------------------------------------------------
void CNPC_Roller::Precache( void )
{
	engine->PrecacheModel( "models/roller.mdl" );

	PRECACHE_SOUND_ARRAY( pCodeSounds );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Roller::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	Vector vecSparkDir;

	vecSparkDir.x = random->RandomFloat( -0.5, 0.5 );
	vecSparkDir.y = random->RandomFloat( -0.5, 0.5 );
	vecSparkDir.z = random->RandomFloat( -0.5, 0.5 );
	
	g_pEffects->Ricochet( ptr->endpos, vecSparkDir );

	BaseClass::TraceAttack( info, vecDir, ptr );
}


//-----------------------------------------------------------------------------
// Purpose: This indicates which types of damage are considered to have physics
//			implications for this type of roller.
//
//-----------------------------------------------------------------------------
int CNPC_Roller::RollerPhysicsDamageMask( void )
{
	return ( DMG_PHYSGUN | DMG_BULLET | DMG_BLAST | DMG_SONIC );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Roller::Spawn( void )
{
	Precache();

	m_pRollSound = NULL;
	
	m_flForwardSpeed = ROLLER_FORWARD_SPEED;

	SetModel( "models/roller.mdl" );
	SetHullType( HULL_TINY_CENTERED );
	SetHullSizeNormal();

	m_bloodColor		= DONT_BLEED;
	m_iHealth			= 20;
	m_flFieldOfView		= 0.5;
	m_NPCState			= NPC_STATE_NONE;

	m_fHACKJustSpawned = true;

	m_RollerController.Off();

	m_flTimeMarcoSound = gpGlobals->curtime + ROLLER_MARCO_FREQUENCY * random->RandomFloat( 1, 3 );
	m_flTimePoloSound = TIME_NEVER;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );

	m_iFail = 0;

	// Create the object in the physics system
	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );
	m_pMotionController = physenv->CreateMotionController( &m_RollerController );
	m_pMotionController->AttachObject( pPhysicsObject );

	NPCInit();

	// Generate me an access code
	int i;
	int iLastDigit = -10;

	for( i = 0 ; i < ROLLER_CODE_DIGITS ; i++ )
	{
		// Generate a digit, and make sure it's not the same
		// or sequential to the previous digit.
		do
		{
			m_iAccessCode[ i ] = rand() % 6;
		} while( abs(m_iAccessCode[ i ] - iLastDigit) <= 1 );

		iLastDigit = m_iAccessCode[ i ];
	}

	// this suppresses boatloads of warnings in the movement code.
	// (code that assumes everyone is propelled by animation).
	m_flGroundSpeed = 20;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CNPC_Roller::~CNPC_Roller( void )
{
	physenv->DestroyMotionController( m_pMotionController );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
bool CNPC_Roller::FInViewCone( CBaseEntity *pEntity )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
int CNPC_Roller::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	return VPhysicsTakeDamage( info );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Roller::Classify( void )
{
	return	CLASS_NONE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
bool CNPC_Roller::FValidateHintType(CAI_Hint *pHint)
{
	return(pHint->HintType() == HINT_ROLLER_PATROL_POINT);
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Vector vecDelta( 64, 64, 64 );
void CNPC_Roller::Unstick( void )
{
	CBaseEntity		*pList[ 16 ];
	IPhysicsObject	*pPhysObj;
	int				i;
	Vector			vecDirToEnemy;
	Vector			vecDirToObject;

	m_flWaitFinished = gpGlobals->curtime;

	int count = UTIL_EntitiesInBox( pList, 16, GetAbsOrigin() - vecDelta, GetAbsOrigin() + vecDelta, 0 );

	m_vecUnstickDirection = vec3_origin;

	for( i = 0 ; i < count ; i++ )
	{
		pPhysObj = pList[ i ]->VPhysicsGetObject();

		if( !pPhysObj || pList[ i ]->m_iClassname == m_iClassname )
		{
			// Only consider physics objects. Exclude rollers.
			continue;
		}

		if( pPhysObj->GetMass() <= ROLLER_MAX_PUSH_MASS )
		{
			// Try to bash this physics object.
			trace_t tr;

			AI_TraceLine( GetAbsOrigin(), pList[ i ]->GetAbsOrigin(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr ); 

			if( tr.fraction == 1.0 || tr.m_pEnt == pList[ i ] )
			{
				// Roll towards this item if the trace hits nothing, or
				// the trace hits the object.
				Vector vecBashDir;

				vecBashDir = pList[ i ]->GetAbsOrigin() - GetAbsOrigin();
				VectorNormalize( vecBashDir );
				vecBashDir.z = 0.0;

				//NDebugOverlay::Line( GetAbsOrigin(), pList[ i ]->GetAbsOrigin(), 0,255,0, true, 2 );

				m_vecUnstickDirection = vecBashDir * 80;

				return;
			}
		}
	}

	// No physics objects. Just pick a direction with some clearance and go there.

#define ROLLER_UNSTICK_DIST 80
	Vector vecDirections[ 4 ] = 
	{
		Vector(  0,	ROLLER_UNSTICK_DIST, 0 ),
		Vector(  ROLLER_UNSTICK_DIST,	0, 0 ),
		Vector(  0, -ROLLER_UNSTICK_DIST, 0 ),
		Vector( -ROLLER_UNSTICK_DIST,  0, 0 )
	};

	trace_t tr;

	for( i = 0 ; i < 4 ; i++ )
	{
		AI_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vecDirections[ i ], MASK_SHOT, this, COLLISION_GROUP_NONE, &tr ); 

		if( tr.fraction == 1.0 )
		{
			m_vecUnstickDirection = vecDirections[ i ];

			//NDebugOverlay::Line( GetLocalOrigin(), GetLocalOrigin() + m_vecUnstickDirection, 255,255,0, true, 2 );

			// Roll in this direction for a couple of seconds.
			Msg( "unsticking!\n" );
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Keeps wobbling down by trying to keep the roller upright. 
//
//
// Output : 
//-----------------------------------------------------------------------------
void CNPC_Roller::RemainUpright( void )
{
	if( !m_RollerController.IsOn() )
	{
		// Don't bother with the math if the controller is off.
		return;
	}

	// We're going to examine the Z component of the Right vector 
	// to see how upright we are.
	Vector vecRight;

	AngleVectors( GetLocalAngles(), NULL, &vecRight, NULL );
	
	//Msg( "%f\n", vecRight.z );

	if( vecRight.z > 0.0001 )
	{
		Msg( "-torque\n" );
		m_RollerController.m_vecAngular.x = ROLLER_UPRIGHT_SPEED;
	}
	else if ( vecRight.z < -0.0001 )
	{
		Msg( "+torque\n" );
		m_RollerController.m_vecAngular.x = -ROLLER_UPRIGHT_SPEED;
	}
	else 
	{
		m_RollerController.m_vecAngular.x = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
void CNPC_Roller::PowerOnSound( void )
{
	EmitSound( "NPC_Roller.PowerOn" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
void CNPC_Roller::PowerOffSound( void )
{
	EmitSound( "NPC_Roller.PowerOff" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
void CNPC_Roller::PowerOn( void )
{
	if( !m_RollerController.IsOn() )
	{
		// If we're about to actually turn the controller on, make a sound.
		// Don't make the sound if the controller is already on.
		PowerOnSound();
	}

	m_RollerController.On();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
void CNPC_Roller::PowerOff( void )
{
	if( m_RollerController.IsOn() )
	{
		// If we're about to actually turn the controller off, make a sound.
		// Don't make the sound if the controller is already off.
		PowerOffSound();
	}

	m_RollerController.Off();
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Roller::MarcoSound( void )
{
	if( gpGlobals->curtime < m_flTimeMarcoSound )
	{
		return;
	}

	EmitSound( "NPC_Roller.Marco" );


	// Look for all the other rollers in the effective radius and tell them to polo me.
	CBaseEntity *pEntity = gEntList.FindEntityByClassname(  NULL, "npc_roller" );
	while( pEntity )
	{
		if( UTIL_DistApprox( GetAbsOrigin(), pEntity->GetAbsOrigin() ) <= ROLLER_MARCOPOLO_DIST )
		{
			CNPC_Roller *pRoller;

			pRoller = dynamic_cast<CNPC_Roller *>(pEntity);

			if( pRoller && pRoller != this )
			{
				// Don't interfere with a roller that's 
				// supposed to be replying to someone else.
				if( pRoller->m_flTimePoloSound == TIME_NEVER )
				{
					// Tell this Roller to polo in a couple seconds.
					pRoller->m_flTimePoloSound = gpGlobals->curtime + 1 + random->RandomFloat( 0, 2 );

					// Tell him also not to marco for quite some time.
					pRoller->m_flTimeMarcoSound = gpGlobals->curtime + ROLLER_MARCO_FREQUENCY * 2;
				}
			}
		}

		pEntity = gEntList.FindEntityByClassname( pEntity, "npc_roller" );
	}
	
	
	
	m_flTimeMarcoSound = gpGlobals->curtime + ROLLER_MARCO_FREQUENCY;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Roller::PoloSound( void )
{
	if( gpGlobals->curtime < m_flTimePoloSound )
	{
		return;
	}

	EmitSound( "NPC_Roller.Polo" );

	m_flTimePoloSound = TIME_NEVER;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Roller::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
#if 0
	MarcoSound();
	PoloSound();
#endif
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Roller::SelectSchedule ( void )
{
	if( m_fHACKJustSpawned )
	{
		m_fHACKJustSpawned = false;
		return SCHED_ROLLER_WAIT_FOR_PHYSICS;
	}

	switch( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if( HasCondition( COND_ROLLER_PHYSICS ) )
		{
			return SCHED_ROLLER_WAIT_FOR_PHYSICS;
		}
		break;

	default:
		if( HasCondition( COND_ROLLER_PHYSICS ) )
		{
			return SCHED_ROLLER_WAIT_FOR_PHYSICS;
		}

		return SCHED_IDLE_STAND;
		break;
	}

	return SCHED_IDLE_STAND;
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Roller::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY_FAILED:
	case SCHED_FAIL:
		if( m_iFail > ROLLER_NUM_FAILS )
		{
			// If the roller has failed a lot, try to unstick him. 
			// Usually by rolling backwards.
			return SCHED_ROLLER_UNSTICK;
		}
		else
		{
			return SCHED_FAIL;
		}

		break;

	case SCHED_IDLE_STAND:
		// Don't stand around. Patrol.
		return SCHED_ROLLER_PATROL;
		break;
	}
	return BaseClass::TranslateSchedule( scheduleType );
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Roller::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLER_ON:
		PowerOn();
		TaskComplete();
		break;

	case TASK_ROLLER_OFF:
		PowerOff();
		TaskComplete();
		break;

	case TASK_ROLLER_WAIT_FOR_PHYSICS:
		// Stop rolling.
		m_flWaitFinished = gpGlobals->curtime;
		m_RollerController.m_vecAngular = vec3_origin;
		PowerOff();
		break;

	case TASK_ROLLER_FIND_PATROL_NODE:

		m_pHintNode = CAI_Hint::FindHint( this, HINT_ROLLER_PATROL_POINT, 0, 1024 ); 

		if(!m_pHintNode)
		{
			TaskFail("Couldn't find roller patrol node");
		}
		else
		{
			TaskComplete();
		}
		break;

	case TASK_ROLLER_UNSTICK:
		Unstick();

		if( m_vecUnstickDirection == vec3_origin )
		{
			TaskComplete();
		}
		else
		{
			VPhysicsGetObject()->Wake();
			PowerOn();
			
			// Cut back on the fail count, but not all the way.
			// Only a successful opportunity move sets this to 0.
			m_iFail /= 2;

			m_flWaitFinished = gpGlobals->curtime + 2;
		}

		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// Make sure the controller isn't turned off.
		m_iFail = 0; // Successfully starting to move.
		VPhysicsGetObject()->Wake();
		PowerOn();
		break;

	case TASK_ROLLER_ISSUE_CODE:
		m_iCodeProgress = 0;
		m_flWaitFinished = gpGlobals->curtime;
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Roller::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLER_UNSTICK:
		{
			float yaw = UTIL_VecToYaw( m_vecUnstickDirection );

			Vector vecRight;
			AngleVectors( QAngle( 0, yaw, 0 ), NULL, &vecRight, NULL );
			m_RollerController.m_vecAngular = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, m_flForwardSpeed );
		}
			
		if( gpGlobals->curtime > m_flWaitFinished )
		{
			TaskComplete();
		}
		break;

	case TASK_ROLLER_WAIT_FOR_PHYSICS:
		{
			Vector vecVelocity;

			VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );

			if( VPhysicsGetObject()->IsAsleep() )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_RUN_PATH:
	case TASK_WALK_PATH:

		// Start turning early
		if( (GetLocalOrigin() - GetNavigator()->GetCurWaypointPos() ).Length() <= 64 )
		{
			if( GetNavigator()->CurWaypointIsGoal() )
			{
				// Hit the brakes a bit.
				float yaw = UTIL_VecToYaw( GetNavigator()->GetCurWaypointPos() - GetLocalOrigin() );
				Vector vecRight;
				AngleVectors( QAngle( 0, yaw, 0 ), NULL, &vecRight, NULL );

				m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, -m_flForwardSpeed * 5 );

				TaskComplete();
				return;
			}

			GetNavigator()->AdvancePath();	
		}

		{
			float yaw = UTIL_VecToYaw( GetNavigator()->GetCurWaypointPos() - GetLocalOrigin() );

			Vector vecRight;
			Vector vecToPath; // points at the path
			AngleVectors( QAngle( 0, yaw, 0 ), &vecToPath, &vecRight, NULL );

			// figure out if the roller is turning. If so, cut the throttle a little.
			float flDot;
			Vector vecVelocity;
			VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );

			VectorNormalize( vecVelocity );

			vecVelocity.z = 0;

			flDot = DotProduct( vecVelocity, vecToPath );

			m_RollerController.m_vecAngular = vec3_origin;

			if( flDot > 0.25 && flDot < 0.7 )
			{
				// Feed a little torque backwards into the axis perpendicular to the velocity.
				// This will help get rid of momentum that would otherwise make us overshoot our goal.
				Vector vecCompensate;

				vecCompensate.x = vecVelocity.y;
				vecCompensate.y = -vecVelocity.x;
				vecCompensate.z = 0;

				m_RollerController.m_vecAngular = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecCompensate, m_flForwardSpeed * -0.75 );
			}

			m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, m_flForwardSpeed );
		}
		break;

	case TASK_ROLLER_ISSUE_CODE:
		if( gpGlobals->curtime >= m_flWaitFinished )
		{
			if( m_iCodeProgress == ROLLER_CODE_DIGITS )
			{
				TaskComplete();
			}
			else
			{
				m_flWaitFinished = gpGlobals->curtime + ROLLER_TONE_TIME;
				CPASAttenuationFilter filter( this );
				EmitSound( filter, entindex(), CHAN_BODY, pCodeSounds[ m_iAccessCode[ m_iCodeProgress ] ], 1.0, ATTN_NORM );
				m_iCodeProgress++;
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Roller::IsBlocked( const Vector &vecCheck )
{
	trace_t tr;

	return false;

#if 0
	NDebugOverlay::Line( GetLocalOrigin(), GetLocalOrigin() + vecCheck, 255,255,0, true, 0.1 );
	
	Vector vecRight;
	Vector vecForward;

	AngleVectors( GetLocalAngles(), &vecForward, &vecRight, NULL );

	NDebugOverlay::Line( GetLocalOrigin() + vecRight * 32, GetLocalOrigin() + vecRight * -32, 0,255,0, true, 0.1 );

#endif ROLLER_DEBUG

	AI_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vecCheck, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	return ( tr.fraction != 1.0 );
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Roller::IsBlockedForward( void )
{
	Vector vecLinear;

	VPhysicsGetObject()->GetVelocity( &vecLinear, NULL );

	VectorNormalize( vecLinear );

	vecLinear *= ROLLER_CHECK_DIST;

	return IsBlocked( vecLinear );
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Roller::IsBlockedBackward( void )
{
	Vector vecLinear;
	VPhysicsGetObject()->GetVelocity( &vecLinear, NULL );

	VectorNormalize( vecLinear );

	vecLinear *= ROLLER_CHECK_DIST;

	return IsBlocked( vecLinear );
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Roller::VPhysicsTakeDamage( const CTakeDamageInfo &info )
{
	if( RollerPhysicsDamageMask() & info.GetDamageType() )
	{
		SetCondition( COND_ROLLER_PHYSICS );
	}

	return BaseClass::VPhysicsTakeDamage( info );
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Roller::OverrideMove( float flInterval )
{
	// Do nothing
	return true;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Roller::TaskFail( AI_TaskFailureCode_t code )
{
	const Task_t *pTask = GetTask();

	if( pTask )
	{
		Msg( "Failed to: %s\n", TaskName( pTask->iTask ) );

		switch( pTask->iTask )
		{
		case TASK_WALK_PATH:
		case TASK_RUN_PATH:
		case TASK_GET_PATH_TO_ENEMY:
		case TASK_GET_PATH_TO_ENEMY_LKP:
		case TASK_GET_PATH_TO_HINTNODE:
			m_iFail++;
			break;
		}
	}

	BaseClass::TaskFail( code );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
// Patrol - roll around from hint node to hint node.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLER_PATROL,

	"	Tasks"
	"		TASK_ROLLER_FIND_PATROL_NODE	0"
	"		TASK_GET_PATH_TO_HINTNODE		0"
	"		TASK_WALK_PATH					0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ROLLER_PHYSICS"
	"		COND_CAN_RANGE_ATTACK1"
);

//=========================================================
// Unstick - try to get unstuck.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLER_UNSTICK,

	"	Tasks"
	"		TASK_ROLLER_UNSTICK				0"
	"	"
	"	Interrupts"
	"		COND_ROLLER_PHYSICS"
);

//=========================================================
// WAIT_FOR_PHYSICS
//
// Don't do ANYTHING until done simulating.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLER_WAIT_FOR_PHYSICS,

	"	Tasks"
	"		TASK_ROLLER_WAIT_FOR_PHYSICS 0"
	"	"
	"	Interrupts"
);


