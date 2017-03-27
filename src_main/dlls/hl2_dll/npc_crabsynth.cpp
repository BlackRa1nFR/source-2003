#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "ai_route.h"
#include "AI_Navigator.h"
#include "AI_Memory.h"
#include "IEffects.h"

class CCrabSynth : public CAI_BaseNPC
{
	DECLARE_CLASS( CCrabSynth, CAI_BaseNPC );
	DECLARE_DATADESC();

public:
	void Spawn( void );
	void Precache( void );

	Class_T Classify( void ) { return CLASS_MORTAR_SYNTH; }
	virtual void SetObjectCollisionBox( void )
	{
		// HACKHACK: Crab is much bigger than this!
		SetAbsMins( GetAbsOrigin() + Vector( -64.f, -64.f, 0.f) );
		SetAbsMaxs( GetAbsOrigin() + Vector(64.f, 64.f, 80.f) );
	}
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual int	SelectSchedule( void );
	virtual int TranslateSchedule( int scheduleType );
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );
	virtual float MaxYawSpeed( void );

#if 0
	void HandleAnimEvent( animevent_t *pEvent );

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

#endif

protected:
	DEFINE_CUSTOM_AI;
#if 0
	DECLARE_DATADESC();

#endif

	float		m_nextCharge;
};


LINK_ENTITY_TO_CLASS( npc_crabsynth, CCrabSynth );
IMPLEMENT_CUSTOM_AI( npc_crabsynth, CCrabSynth );


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CCrabSynth )

	DEFINE_FIELD( CCrabSynth, m_nextCharge,	FIELD_TIME ),

END_DATADESC()


//=========================================================
// private activities
//=========================================================
int ACT_CHARGE_START;
int ACT_CHARGE_START_MOVE;
int ACT_CHARGE_LOOP;
int ACT_CHARGE_END;
int ACT_BODY_THROW;
int ACT_CHARGE_HITLEFT;
int ACT_CHARGE_HITRIGHT;

//=========================================================
// schedules
//=========================================================
enum
{
	SCHED_CRAB_CHARGE = LAST_SHARED_SCHEDULE,
	SCHED_CRAB_CHARGE_STOP,
	SCHED_CRAB_FLINCH,
	SCHED_CRAB_CHARGE_HITLEFT,
	SCHED_CRAB_CHARGE_HITRIGHT,
};

//=========================================================
// tasks
//=========================================================
enum 
{
	TASK_CHARGE = LAST_SHARED_TASK,
	TASK_GET_CHARGE_PATH,
	TASK_CHARGE_HIT,
	TASK_CHARGE_HITLEFT,
	TASK_CHARGE_HITRIGHT,
};

//=========================================================
//=========================================================
#if 0
BEGIN_DATADESC( CCrabSynth )


END_DATADESC()
#endif


void CCrabSynth::Precache( void )
{
	engine->PrecacheModel( "models/synth.mdl" );
	BaseClass::Precache();
}

void CCrabSynth::Spawn( void )
{
	Precache();
	SetModel( "models/synth.mdl" );
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();
	SetDefaultEyeOffset();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_GREEN );
	m_iHealth			= 251;
	m_flFieldOfView		= 0.5f;
	m_NPCState			= NPC_STATE_NONE;
	
	m_nextCharge = 0.f;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 );

	NPCInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrabSynth::InitCustomSchedules( void )
{
	INIT_CUSTOM_AI( CCrabSynth );

	ADD_CUSTOM_TASK(CCrabSynth,			TASK_CHARGE);
	ADD_CUSTOM_TASK(CCrabSynth,			TASK_GET_CHARGE_PATH);
	ADD_CUSTOM_TASK(CCrabSynth,			TASK_CHARGE_HIT);
	ADD_CUSTOM_TASK(CCrabSynth,			TASK_CHARGE_HITLEFT);
	ADD_CUSTOM_TASK(CCrabSynth,			TASK_CHARGE_HITRIGHT);
	
	ADD_CUSTOM_SCHEDULE(CCrabSynth,		SCHED_CRAB_CHARGE);
	ADD_CUSTOM_SCHEDULE(CCrabSynth,		SCHED_CRAB_CHARGE_STOP);
	ADD_CUSTOM_SCHEDULE(CCrabSynth,		SCHED_CRAB_FLINCH);
	ADD_CUSTOM_SCHEDULE(CCrabSynth,		SCHED_CRAB_CHARGE_HITLEFT);
	ADD_CUSTOM_SCHEDULE(CCrabSynth,		SCHED_CRAB_CHARGE_HITRIGHT);

//	ADD_CUSTOM_CONDITION(CCrabSynth,	COND_);
	
	ADD_CUSTOM_ACTIVITY(CCrabSynth,		ACT_CHARGE_START);
	ADD_CUSTOM_ACTIVITY(CCrabSynth,		ACT_CHARGE_START_MOVE);
	ADD_CUSTOM_ACTIVITY(CCrabSynth,		ACT_CHARGE_LOOP);
	ADD_CUSTOM_ACTIVITY(CCrabSynth,		ACT_CHARGE_END);
	ADD_CUSTOM_ACTIVITY(CCrabSynth,		ACT_BODY_THROW);
	ADD_CUSTOM_ACTIVITY(CCrabSynth,		ACT_CHARGE_HITLEFT);
	ADD_CUSTOM_ACTIVITY(CCrabSynth,		ACT_CHARGE_HITRIGHT);

	AI_LOAD_SCHEDULE(CCrabSynth,		SCHED_CRAB_CHARGE);
	AI_LOAD_SCHEDULE(CCrabSynth,		SCHED_CRAB_CHARGE_STOP);
	AI_LOAD_SCHEDULE(CCrabSynth,		SCHED_CRAB_FLINCH);
	AI_LOAD_SCHEDULE(CCrabSynth,		SCHED_CRAB_CHARGE_HITLEFT);
	AI_LOAD_SCHEDULE(CCrabSynth,		SCHED_CRAB_CHARGE_HITRIGHT);
}

void CCrabSynth::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
	{
		g_pEffects->Ricochet( ptr->endpos, (vecDir*-1.0f) );
		info.SetDamage( 0.01 );
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

int CCrabSynth::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;
	info.SetDamage( 0.01f );
	if ( info.GetDamageType() & DMG_BLAST )
	{
		if ( info.GetInflictor() )
		{
			// damage from roughly below?
			if ( info.GetInflictor()->GetAbsOrigin().z - GetAbsOrigin().z < ((GetAbsMaxs().z-GetAbsMins().z)*0.5f) )
			{
				info.SetDamage( inputInfo.GetDamage() );
			}
		}
	}
	return BaseClass::OnTakeDamage_Alive( info );
}

int CCrabSynth::SelectSchedule( void )
{
	switch ( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if( HasCondition( COND_HEAVY_DAMAGE ) )
		{
			return SCHED_CRAB_FLINCH;
		}
		if ( HasCondition( COND_SEE_ENEMY ) )
		{
			if ( m_nextCharge < gpGlobals->curtime )
			{
				m_nextCharge = gpGlobals->curtime + 20.f;
				return SCHED_CRAB_CHARGE;
			}
		}
		break;

	default:
		return BaseClass::SelectSchedule();
		break;
	}

	return BaseClass::SelectSchedule();
}

int CCrabSynth::TranslateSchedule( int scheduleType )
{
	return BaseClass::TranslateSchedule( scheduleType );
}


float CCrabSynth::MaxYawSpeed( void )
{
	if ( GetActivity() == ACT_CHARGE_LOOP )
	{
		return 20.f;
	}
	return 90.f;
}

void CCrabSynth::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CHARGE:
		break;
	case TASK_GET_CHARGE_PATH:
		{
			if (IsUnreachable(GetEnemy()))
			{
				TaskFail(FAIL_NO_ROUTE);
				return;
			}
			Vector flEnemyLKP = GetEnemyLKP();
			if ( !GetNavigator()->SetGoal( flEnemyLKP, AIN_CLEAR_TARGET ) )
			{
				// no way to get there =(
				DevWarning( 2, "GetPathToEnemyLKP failed!!\n" );
				RememberUnreachable(GetEnemy());
				TaskFail(FAIL_NO_ROUTE);
			}
			GetNavigator()->SetMovementActivity( (Activity)ACT_CHARGE_LOOP );
			TaskComplete();
			break;
		}
		break;
	
	case TASK_CHARGE_HIT:
		GetNavigator()->SetMovementActivity( (Activity)ACT_CHARGE_END );
		SetIdealActivity( (Activity)ACT_CHARGE_END );
		break;
	case TASK_CHARGE_HITLEFT:
		GetNavigator()->SetMovementActivity( (Activity)ACT_CHARGE_HITLEFT );
		SetIdealActivity( (Activity)ACT_CHARGE_HITLEFT );
		break;
	case TASK_CHARGE_HITRIGHT:
		GetNavigator()->SetMovementActivity( (Activity)ACT_CHARGE_HITRIGHT );
		SetIdealActivity( (Activity)ACT_CHARGE_HITRIGHT );
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}

#define CHARGE_ATTACK_HIT		120.f
#define CHARGE_ATTACK_WIDTH		100.f
#define CHARGE_ATTACK_TRAMPLE	40.f

void CCrabSynth::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CHARGE:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if ( GetNavigator()->GetGoalType() == GOALTYPE_NONE || !GetNavigator()->IsGoalActive() || !pEnemy )
			{
				TaskComplete();
				GetNavigator()->ClearGoal();		// Stop moving
				SetSchedule( SCHED_CRAB_CHARGE_STOP );
			}
			else
			{
				// Check validity of goal type
				ValidateNavGoal();
				SetIdealActivity( GetNavigator()->GetMovementActivity() );

				Vector forward, right;
				AngleVectors( GetLocalAngles(), &forward, &right, NULL );
				float dot = DotProduct( forward, pEnemy->GetAbsOrigin() );
				float dist = dot - DotProduct( forward, GetAbsOrigin() );
				// about to cross player
				// HACKHACK: Tune this magic number
				if ( dist < CHARGE_ATTACK_HIT )
				{
					float lateral = DotProduct( right, pEnemy->GetAbsOrigin() ) - DotProduct( right, GetAbsOrigin() );
					Msg( "Lateral dist: %.2f\n", lateral );
					if ( lateral < -CHARGE_ATTACK_WIDTH || lateral > CHARGE_ATTACK_WIDTH )
					{
						Msg( "Stop!\n" );
						// play miss/stop animation.  No damage to enemy
						SetSchedule( SCHED_CRAB_CHARGE_STOP );
					}
					else if ( lateral < -CHARGE_ATTACK_TRAMPLE )
					{
						Msg( "Hit left!\n" );
						// hit left side
						SetSchedule( SCHED_CRAB_CHARGE_HITLEFT );
					}
					else if ( lateral > CHARGE_ATTACK_TRAMPLE )
					{
						Msg( "Hit Right!\n" );
						// hit right side
						SetSchedule( SCHED_CRAB_CHARGE_HITLEFT );
					}
					else
					{
						Msg( "Head On!!\n" );
						// head on collision
						TaskComplete();
					}
				}
			}
		}
		break;
	case TASK_CHARGE_HITRIGHT:
	case TASK_CHARGE_HITLEFT:
	case TASK_CHARGE_HIT:
		if ( IsActivityFinished() )
		{
			GetNavigator()->ClearGoal();
			TaskComplete();
		}
		break;

	default:
		BaseClass::RunTask( pTask );
	}
}



//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
// Charge!
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CRAB_CHARGE,

	"	Tasks"
	"		TASK_FACE_ENEMY					0"
	"		TASK_GET_CHARGE_PATH			0"
	"		TASK_CHARGE						0"
	"		TASK_CHARGE_HIT					0"
	"	"
	"	Interrupts"
	"		COND_HEAVY_DAMAGE"
);

//=========================================================
// Charge Hit Left!
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CRAB_CHARGE_HITLEFT,

	"	Tasks"
	"		TASK_CHARGE_HITLEFT				0"
	"	"
	"	Interrupts"
);

//=========================================================
// Charge Hit Right!
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CRAB_CHARGE_HITRIGHT,

	"	Tasks"
	"		TASK_CHARGE_HITRIGHT			0"
	"	"
	"	Interrupts"
);

//=========================================================
// Charge Stop
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CRAB_CHARGE_STOP,

	"	Tasks"
	"		TASK_PLAY_PRIVATE_SEQUENCE		ACTIVITY:ACT_CHARGE_END"
	"	"
	"	Interrupts"
);

//=========================================================
// Flop / Flinch
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CRAB_FLINCH,

	"	Tasks"
	"		TASK_PLAY_PRIVATE_SEQUENCE		ACTIVITY:ACT_BODY_THROW"
	"	"
	"	Interrupts"
);