//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:		Ichthyosaur - buh bum...  buh bum...
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "beam_shared.h"
#include "AI_Task.h"
#include "AI_Default.h"
#include "AI_Navigator.h"
#include "AI_Schedule.h"
#include "AI_Hull.h"
#include "AI_Route.h"
#include "activitylist.h"
#include "game.h"
#include "NPCEvent.h"
#include "Player.h"
#include "EntityList.h"
#include "soundenvelope.h"
#include "shake.h"
#include "ndebugoverlay.h"
#include "splash.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "hl1_npc_ichthyosaur.h"

ConVar sk_ichthyosaur_health ( "sk_ichthyosaur_health", "0" );
ConVar sk_ichthyosaur_shake ( "sk_ichthyosaur_shake", "0" );

#define	ICH_SWIM_SPEED_WALK		150
#define	ICH_SWIM_SPEED_RUN		500


enum IchthyosaurMoveType_t
{
	ICH_MOVETYPE_SEEK = 0,	// Fly through the target without stopping.
	ICH_MOVETYPE_ARRIVE		// Slow down and stop at target.
};

const char *CNPC_Ichthyosaur::pIdleSounds[] = 
{
	"ichy/ichy_idle1.wav",
	"ichy/ichy_idle2.wav",
	"ichy/ichy_idle3.wav",
	"ichy/ichy_idle4.wav",
};

const char *CNPC_Ichthyosaur::pAlertSounds[] = 
{
	"ichy/ichy_alert2.wav",
	"ichy/ichy_alert3.wav",
};

const char *CNPC_Ichthyosaur::pAttackSounds[] = 
{
	"ichy/ichy_attack1.wav",
	"ichy/ichy_attack2.wav",
};

const char *CNPC_Ichthyosaur::pBiteSounds[] = 
{
	"ichy/ichy_bite1.wav",
	"ichy/ichy_bite2.wav",
};

const char *CNPC_Ichthyosaur::pPainSounds[] = 
{
	"ichy/ichy_pain2.wav",
	"ichy/ichy_pain3.wav",
	"ichy/ichy_pain5.wav",
};

const char *CNPC_Ichthyosaur::pDieSounds[] = 
{
	"ichy/ichy_die2.wav",
	"ichy/ichy_die4.wav",
};

enum
{
	SCHED_SWIM_AROUND = LAST_SHARED_SCHEDULE + 1,
	SCHED_SWIM_AGITATED,
	SCHED_CIRCLE_ENEMY,
	SCHED_TWITCH_DIE,
};


//=========================================================
// monster-specific tasks and states
//=========================================================
enum 
{
	TASK_ICHTHYOSAUR_CIRCLE_ENEMY = LAST_SHARED_TASK + 1,
	TASK_ICHTHYOSAUR_SWIM,
	TASK_ICHTHYOSAUR_FLOAT,
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


AI_BEGIN_CUSTOM_NPC( monster_ichthyosaur, CNPC_Ichthyosaur )

DECLARE_TASK ( TASK_ICHTHYOSAUR_SWIM )
DECLARE_TASK ( TASK_ICHTHYOSAUR_CIRCLE_ENEMY )
DECLARE_TASK ( TASK_ICHTHYOSAUR_FLOAT )

	//=========================================================
	// > SCHED_SWIM_AROUND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SWIM_AROUND,

		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_WALK"
		"		TASK_ICHTHYOSAUR_SWIM		0.0"
		" "
		"	Interrupts"
		"   COND_LIGHT_DAMAGE"
		"   COND_HEAVY_DAMAGE"
		"   COND_SEE_ENEMY"
		"   COND_NEW_ENEMY"
		"   COND_HEAR_PLAYER"
		"   COND_HEAR_COMBAT"
	)
	//=========================================================
	// > SCHED_SWIM_AGITATED
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_SWIM_AGITATED,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_RUN"
		"		TASK_WAIT					2.0"
		" "
	)
	//=========================================================
	// > SCHED_CIRCLE_ENEMY
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_CIRCLE_ENEMY,

		"	Tasks"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_WALK"
		"		TASK_ICHTHYOSAUR_CIRCLE_ENEMY		0.0"
		" "
		"	Interrupts"
		"   COND_LIGHT_DAMAGE"
		"   COND_HEAVY_DAMAGE"
		"   COND_NEW_ENEMY"
		"   COND_CAN_MELEE_ATTACK1"
		"   COND_CAN_RANGE_ATTACK1"
	)
	//=========================================================
	// > SCHED_TWITCH_DIE
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_TWITCH_DIE,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SOUND_DIE				0"
		"		TASK_DIE					0"
		"		TASK_ICHTHYOSAUR_FLOAT		0"
		" "
	)

AI_END_CUSTOM_NPC()


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Ichthyosaur::Precache()
{
	engine->PrecacheModel("models/icky.mdl");

	engine->PrecacheModel("sprites/lgtning.vmt");

	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pBiteSounds );
	PRECACHE_SOUND_ARRAY( pDieSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );

	BaseClass::Precache();
}

void CNPC_Ichthyosaur::Spawn( void )
{
	Precache( );

	SetModel( "models/icky.mdl");
	UTIL_SetSize( this, Vector( -32, -32, -32 ), Vector( 32, 32, 32 ) );
	
	SetHullType(HULL_LARGE_CENTERED);
	SetHullSizeNormal();
	SetDefaultEyeOffset();

	SetNavType( NAV_FLY );
	m_NPCState			= NPC_STATE_NONE;
	
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_FLY );
	m_bloodColor		= BLOOD_COLOR_RED;
	m_iHealth			= sk_ichthyosaur_health.GetFloat();
	m_iMaxHealth		= m_iHealth;
	m_flFieldOfView		= -0.707;	// 270 degrees
	
	AddFlag( FL_SWIM );

	m_flGroundSpeed		= ICHTHYOSAUR_SPEED;

	SetDistLook( 1024 );

	
	SetTouch( BiteTouch );
//	SetUse( CombatUse ); 

	m_idealDist = 384;
	m_flMinSpeed = 80;
	m_flMaxSpeed = 500;
	m_flMaxDist = 275;

	Vector vforward;
	AngleVectors(GetAbsAngles(), &vforward );
	VectorNormalize ( vforward );
	SetAbsVelocity( m_flGroundSpeed * vforward );
	m_SaveVelocity = GetAbsVelocity();

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_FLY | bits_CAP_INNATE_MELEE_ATTACK1 );

	NPCInit();
}


//=========================================================
//=========================================================
int CNPC_Ichthyosaur::TranslateSchedule( int scheduleType )
{
#if 0
	// ALERT( at_console, "GetScheduleOfType( %d ) %d\n", Type, m_bOnAttack );
	switch	( scheduleType )
	{
	case SCHED_IDLE_WALK:
		return SCHED_SWIM_AROUND;
	case SCHED_STANDOFF:
		return SCHED_CIRCLE_ENEMY;
	case SCHED_FAIL:
		return SCHED_SWIM_AGITATED;
	case SCHED_DIE:
		return SCHED_TWITCH_DIE;
#if 0
	case SCHED_CHASE_ENEMY:
		{
//		AttackSound( );
			CPASAttenuationFilter filter( this );
			enginesound->EmitSound( filter, entindex(), CHAN_VOICE, pAttackSounds[ RandomInt(0,ARRAYSIZE(pAttackSounds)-1)], 1.0, ATTN_NORM );
		}
	default:
		return SCHED_SWIM_AROUND;
#endif
	}
#endif
	return BaseClass::TranslateSchedule( scheduleType );
}

#if 0
int CNPC_Ichthyosaur::SelectSchedule( void ) 
{

	int retSchedule;

	if ( HasCondition( COND_SEE_ENEMY ) )
	{
		Msg("Chase\n");
		retSchedule = SCHED_CHASE_ENEMY;
	}
	else
	{
		Msg("Swimming\n");
		retSchedule = SCHED_SWIM_AROUND;
	}
	if ( !IsAlive() )
	{
		Msg("Dead...\n");
		retSchedule = SCHED_TWITCH_DIE;
	}

	return( retSchedule );
}
#endif

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================
void CNPC_Ichthyosaur::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_ICHTHYOSAUR_CIRCLE_ENEMY:
		break;
	case TASK_ICHTHYOSAUR_SWIM:
		break;
	case TASK_SMALL_FLINCH:
		if (m_idealDist > 128)
		{
			m_flMaxDist = 512;
			m_idealDist = 512;
		}
		else
		{
			m_bOnAttack = TRUE;
		}
		BaseClass::StartTask(pTask);
		break;

	case TASK_ICHTHYOSAUR_FLOAT:
		m_nSkin = EYE_BASE;
		SetSequenceByName( "bellyup" );
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

void CNPC_Ichthyosaur::RunTask(const Task_t *pTask )
{
	QAngle angles = GetAbsAngles();

	switch ( pTask->iTask )
	{
	case TASK_ICHTHYOSAUR_CIRCLE_ENEMY:

		if (GetEnemy() == NULL )
		{
			TaskComplete( );
		}
		else if (FVisible( GetEnemy() ))
		{
			Vector vecFrom = GetEnemy()->EyePosition( );

			Vector vecDelta = GetAbsOrigin() - vecFrom;
			VectorNormalize( vecDelta );
			Vector vecSwim = CrossProduct( vecDelta, Vector( 0, 0, 1 ) );
			VectorNormalize( vecSwim );
			
			if (DotProduct( vecSwim, m_SaveVelocity ) < 0)
			{
				vecSwim = vecSwim * -1.0;
			}

			Vector vecPos = vecFrom + vecDelta * m_idealDist + vecSwim * 32;

			// ALERT( at_console, "vecPos %.0f %.0f %.0f\n", vecPos.x, vecPos.y, vecPos.z );

			trace_t tr;
		
//			UTIL_TraceHull( vecFrom, vecPos, ignore_monsters, large_hull, m_hEnemy->edict(), &tr );
			UTIL_TraceEntity( this, vecFrom, vecPos, MASK_NPCSOLID, &tr );

			if (tr.fraction > 0.5)
			{
				vecPos = tr.endpos;
			}

			Vector vecNorm = vecPos - GetAbsOrigin();
			VectorNormalize( vecNorm );
			m_SaveVelocity = m_SaveVelocity * 0.8 + 0.2 * vecNorm * m_flGroundSpeed;

			// ALERT( at_console, "m_SaveVelocity %.2f %.2f %.2f\n", m_SaveVelocity.x, m_SaveVelocity.y, m_SaveVelocity.z );

			if (HasCondition( COND_ENEMY_FACING_ME ) && GetEnemy()->FVisible( this ))
			{
				m_flNextAlert -= 0.1;

				if (m_idealDist < m_flMaxDist)
				{
					m_idealDist += 4;
				}
				if (m_flGroundSpeed > m_flMinSpeed)
				{
					m_flGroundSpeed -= 2;
				}
				else if (m_flGroundSpeed < m_flMinSpeed)
				{
					m_flGroundSpeed += 2;
				}
				if (m_flGroundSpeed < m_flMaxSpeed)
				{
					m_flGroundSpeed += 0.5;
				}
			}
			else 
			{
				m_flNextAlert += 0.1;

				if (m_idealDist > 128)
				{
					m_idealDist -= 4;
				}
				if (m_flGroundSpeed < m_flMaxSpeed)
				{
					m_flGroundSpeed += 4;
				}
			}

			// ALERT( at_console, "%.0f\n", m_idealDist );
		}
		else
		{
			m_flNextAlert = gpGlobals->curtime + 0.2;
		}

		if (m_flNextAlert < gpGlobals->curtime)
		{
			// ALERT( at_console, "AlertSound()\n");
			AlertSound( );
			m_flNextAlert = gpGlobals->curtime + RandomFloat( 3, 5 );
		}

		break;
	case TASK_ICHTHYOSAUR_SWIM:
		if ( IsSequenceFinished() )
		{
			TaskComplete( );
		}
		break;
	case TASK_DIE:
		if ( IsSequenceFinished() )
		{
//			pev->deadflag = DEAD_DEAD;

			TaskComplete( );
		}
		break;

	case TASK_ICHTHYOSAUR_FLOAT:
		angles.x = UTIL_ApproachAngle( 0, angles.x, 20 );
		SetAbsAngles( angles );

//		SetAbsVelocity( GetAbsVelocity() * 0.8 );
//		if (pev->waterlevel > 1 && GetAbsVelocity().z < 64)
//		{
//			pev->velocity.z += 8;
//		}
//		else 
//		{
//			pev->velocity.z -= 8;
//		}
		// ALERT( at_console, "%f\n", m_vecAbsVelocity.z );
		break;

	default: 
		BaseClass::RunTask( pTask );
		break;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
int CNPC_Ichthyosaur::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if ( flDot >= 0.7 && m_flEnemyTouched > gpGlobals->curtime - 0.2 )
	{
		return COND_CAN_MELEE_ATTACK1;
	}
	return COND_NONE;
}

void CNPC_Ichthyosaur::BiteTouch( CBaseEntity *pOther )
{
	// bite if we hit who we want to eat
	if ( pOther == GetEnemy() ) 
	{
		m_flEnemyTouched = gpGlobals->curtime;
		m_bOnAttack = TRUE;
	}
}

#define ICHTHYOSAUR_AE_SHAKE_RIGHT 1
#define ICHTHYOSAUR_AE_SHAKE_LEFT  2

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_Ichthyosaur::HandleAnimEvent( animevent_t *pEvent )
{
	int bDidAttack = FALSE;
	Vector vForward, vRight;
	QAngle angles = GetAbsAngles();
	AngleVectors( angles, &vForward, &vRight, NULL );

	switch( pEvent->event )
	{
	case ICHTHYOSAUR_AE_SHAKE_RIGHT:
	case ICHTHYOSAUR_AE_SHAKE_LEFT:
		{
			CBaseEntity* hEnemy = GetEnemy();

			if (hEnemy != NULL && FVisible( hEnemy ))
			{
				CBaseEntity *pHurt = GetEnemy();


//				if (m_flEnemyTouched < gpGlobals->curtime - 0.2 /*&& (hEnemy->BodyTarget( GetAbsOrigin() ) - GetAbsOrigin()).Length() > (32+16+32)*/)
//					break;

				Vector vecShootOrigin = Weapon_ShootPosition();
				Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

				if (DotProduct( vecShootDir, vForward ) > 0.707)
				{
					angles = pHurt->GetAbsAngles();
					m_bOnAttack = TRUE;
					pHurt->SetAbsVelocity( pHurt->GetAbsVelocity() - vRight * 300 );
					if (pHurt->IsPlayer())
					{

						angles.x += RandomFloat( -35, 35 );
						angles.y += RandomFloat( -90, 90 );
						angles.z = 0;
						((CBasePlayer*) pHurt)->SetPunchAngle( angles );
					}

					CTakeDamageInfo info( this, this, 10, DMG_SLASH );
					CalculateMeleeDamageForce( &info, vForward, pHurt->GetAbsOrigin() );
					pHurt->TakeDamage( info );
				}
			}
			CPASAttenuationFilter filter( this );
			enginesound->EmitSound( filter, entindex(), CHAN_VOICE, pBiteSounds[ RandomInt(0,ARRAYSIZE(pBiteSounds)-1)], 1.0, ATTN_NORM );

			bDidAttack = TRUE;
		}
		break;
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
	// make bubbles when he attacks
	if (bDidAttack)
	{
		Vector vecSrc = GetAbsOrigin() + vForward * 32;
		UTIL_Bubbles( vecSrc - Vector( 8, 8, 8 ), vecSrc + Vector( 8, 8, 8 ), 16 );
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
Class_T CNPC_Ichthyosaur::Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

void CNPC_Ichthyosaur::NPCThink ( void )
{
	BaseClass::NPCThink( );

	if ( m_lifeState == LIFE_ALIVE )
	{
		if ( m_NPCState != NPC_STATE_SCRIPT)
		{
			m_flGroundSpeed = GetGroundSpeed();

			if (GetNavigator()->IsGoalActive())
			{
				Vector vecDir =  ( GetNavigator()->GetPath()->CurWaypointPos() - GetAbsOrigin());	
				VectorNormalize( vecDir );

				m_SaveVelocity = vecDir * m_flGroundSpeed;
			}

//			Msg("%f\n", m_flGroundSpeed);

			Swim();

			// blink the eye
			if (m_flBlink < gpGlobals->curtime)
			{
				m_nSkin = EYE_CLOSED;
				if (m_flBlink + 0.2 < gpGlobals->curtime)
				{
					m_flBlink = gpGlobals->curtime + random->RandomFloat( 3, 4 );
					if (m_bOnAttack)
						m_nSkin = EYE_MAD;
					else
						m_nSkin = EYE_BASE;
				}
			}
		}
	}

}

void CNPC_Ichthyosaur::Swim( void )
{
	Vector start = GetAbsOrigin();

	QAngle angAngles;
	Vector Forward, Right, Up;
	QAngle angRotation;


	angRotation = GetAbsAngles();

	if (GetFlags() & FL_ONGROUND)
	{
		angRotation.x = 0;
		angRotation.y += RandomFloat( -45, 45 );
		SetAbsAngles( angRotation );
		RemoveFlag( FL_ONGROUND );

		angAngles = QAngle( -GetAbsAngles().x, GetAbsAngles().y, GetAbsAngles().z );
		AngleVectors( angAngles, &Forward, &Right, &Up );

		SetAbsVelocity( Forward * 200 + Up * 200 );

		return;
	}

	float dist = 500;
	CBaseEntity* enemy = GetEnemy();
	if ( enemy != NULL )
	{
		dist = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()).Length();
	}

//	Msg("%d\n", HasCondition(COND_SEE_ENEMY));
#if 0
	if ( (m_bOnAttack && !HasCondition( COND_SEE_ENEMY )) || dist > 200.0f )
	{
		m_bOnAttack = false;
	}

	if ( !m_bOnAttack  && m_flGroundSpeed > m_flMinSpeed ) 
	{	
		m_flGroundSpeed -= 15;
	}
#endif
	if (m_bOnAttack && m_flGroundSpeed < m_flMaxSpeed)
	{
		m_flGroundSpeed += 40;
	}
	if ( m_flGroundSpeed < 180 )
	{
		if ( GetIdealActivity() == ACT_RUN)
			SetActivity( ACT_WALK );
		if ( GetIdealActivity() == ACT_WALK)
			m_flPlaybackRate = m_flGroundSpeed / 150.0;
		// ALERT( at_console, "walk %.2f\n", pev->framerate );
	}
	else
	{
		if ( GetIdealActivity() == ACT_WALK)
			SetActivity( ACT_RUN );
		if ( GetIdealActivity() == ACT_RUN)
			m_flPlaybackRate = m_flGroundSpeed / 150.0;
		// ALERT( at_console, "run  %.2f\n", pev->framerate );
	}


#define PROBE_LENGTH 150

	VectorAngles( m_SaveVelocity, angAngles );
	angAngles.x = -angAngles.x;
	AngleVectors(angAngles, &Forward, &Right, &Up);

	m_flGroundSpeed = GetGroundSpeed();

	Vector f, u, l, r, d;
	f = DoProbe(start + PROBE_LENGTH   * Forward);
	r = DoProbe(start + PROBE_LENGTH/3 * Forward+Right);
	l = DoProbe(start + PROBE_LENGTH/3 * Forward-Right);
	u = DoProbe(start + PROBE_LENGTH/3 * Forward+Up);
	d = DoProbe(start + PROBE_LENGTH/3 * Forward-Up);

	Vector SteeringVector = f+r+l+u+d;
	m_SaveVelocity = ( m_SaveVelocity + SteeringVector / 2 );
	VectorNormalize( m_SaveVelocity );

	angAngles = QAngle( -GetAbsAngles().x, GetAbsAngles().y, GetAbsAngles().z );
	AngleVectors(angAngles, &Forward, &Right, &Up);
	// ALERT( at_console, "%f : %f\n", Angles.x, Forward.z );

	float flDot = DotProduct( Forward, m_SaveVelocity );
	if (flDot > 0.5)
		SetAbsVelocity( m_SaveVelocity = m_SaveVelocity * m_flGroundSpeed );
	else if (flDot > 0)
		SetAbsVelocity( m_SaveVelocity = m_SaveVelocity * m_flGroundSpeed * (flDot + 0.5) );
	else
		SetAbsVelocity( m_SaveVelocity = m_SaveVelocity * 80 );

	// ALERT( at_console, "%.0f %.0f\n", m_flightSpeed, pev->velocity.Length() );


	//Msg( "Steer %f %f %f\n", SteeringVector.x, SteeringVector.y, SteeringVector.z );

	// ALERT( at_console, "speed %f\n", m_flightSpeed );
	VectorAngles( m_SaveVelocity, angAngles );

	// Smooth Pitch
	//
	if (angAngles.x > 180)
		angAngles.x = angAngles.x - 360;
	angRotation.x = UTIL_Approach(angAngles.x, angRotation.x, 50 * 0.1 );
	if (angRotation.x < -80)
		angRotation.x = -80;
	if (angRotation.x >  80)
		angRotation.x =  80;

	// Smooth Yaw and generate Roll
	//
	float turn = 360;
	// ALERT( at_console, "Y %.0f %.0f\n", Angles.y, GetAbsAngles().y );

	if (fabs(angAngles.y - angRotation.y) < fabs(turn))
	{
		turn = angAngles.y - angRotation.y;
	}
	if (fabs(angAngles.y - angRotation.y + 360) < fabs(turn))
	{
		turn = angAngles.y - angRotation.y + 360;
	}
	if (fabs(angAngles.y - angRotation.y - 360) < fabs(turn))
	{
		turn = angAngles.y - angRotation.y - 360;
	}

	float speed = m_flGroundSpeed * 0.1;

	// ALERT( at_console, "speed %.0f %f\n", turn, speed );
	if (fabs(turn) > speed)
	{
		if (turn < 0.0)
		{
			turn = -speed;
		}
		else
		{
			turn = speed;
		}
	}
	angRotation.y += turn;
	angRotation.z -= turn;
	angRotation.y = fmod((angRotation.y + 360.0), 360.0);

	static float yaw_adj;

	yaw_adj = yaw_adj * 0.8 + turn;

   // Msg( "yaw %f : %f\n", turn, yaw_adj );

	SetBoneController( 0, -yaw_adj / 4.0 );

	// Roll Smoothing
	//
	turn = 360;
	if (fabs(angAngles.z - angRotation.z) < fabs(turn))
	{
		turn = angAngles.z - angRotation.z;
	}
	if (fabs(angAngles.z - angRotation.z + 360) < fabs(turn))
	{
		turn = angAngles.z - angRotation.z + 360;
	}
	if (fabs(angAngles.z - angRotation.z - 360) < fabs(turn))
	{
		turn = angAngles.z - angRotation.z - 360;
	}
	speed = m_flGroundSpeed/2 * 0.1;
	if (fabs(turn) < speed)
	{
		angRotation.z += turn;
	}
	else
	{
		if (turn < 0.0)
		{
			angRotation.z -= speed;
		}
		else
		{
			angRotation.z += speed;
		}
	}

	if (angRotation.z < -20)
		angRotation.z = -20;
	if (angRotation.z >  20)
		angRotation.z =  20;

	SetAbsAngles( angRotation );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : speed to move at
//-----------------------------------------------------------------------------
float CNPC_Ichthyosaur::GetGroundSpeed( void )
{
	if ( GetIdealActivity() == ACT_WALK )
		return ICH_SWIM_SPEED_WALK;

	return ICH_SWIM_SPEED_RUN;
}


Vector CNPC_Ichthyosaur::DoProbe( const Vector &Probe )
{

	Vector WallNormal = Vector(0,0,-1); // WATER normal is Straight Down for fish.
	float frac;
	BOOL bBumpedSomething = ProbeZ(GetAbsOrigin(), Probe, &frac);

	trace_t tr;
	UTIL_TraceEntity( this, GetAbsOrigin(), Probe, MASK_NPCSOLID, &tr );
	if ( tr.allsolid || tr.fraction < 0.99 )
	{
		if (tr.fraction < 0.0) tr.fraction = 0.0;
		if (tr.fraction > 1.0) tr.fraction = 1.0;
		if (tr.fraction < frac)
		{
			frac = tr.fraction;
			bBumpedSomething = TRUE;
			WallNormal = tr.plane.normal;
		}
	}

	if (bBumpedSomething && (GetEnemy() == NULL || !tr.m_pEnt || tr.m_pEnt->entindex() != GetEnemy()->entindex()))
	{
		Vector ProbeDir = Probe - GetAbsOrigin();

		Vector NormalToProbeAndWallNormal = CrossProduct(ProbeDir, WallNormal);
		Vector SteeringVector = CrossProduct( NormalToProbeAndWallNormal, ProbeDir);

		VectorNormalize( WallNormal );
		VectorNormalize( m_SaveVelocity );

		float SteeringForce = m_flGroundSpeed * (1-frac) * (DotProduct( WallNormal, m_SaveVelocity));
		if (SteeringForce < 0.0)
		{
			SteeringForce = -SteeringForce;
		}

		Vector vSteering = SteeringVector;

		VectorNormalize( vSteering );
		SteeringVector = SteeringForce * vSteering;
		
		return SteeringVector;
	}
	return Vector(0, 0, 0);
}

bool CNPC_Ichthyosaur::ProbeZ( const Vector &position, const Vector &probe, float *pFraction)
{

	if ( UTIL_PointContents( probe ) & MASK_WATER )
		 return FALSE;
	else
	{
		Vector vProbePosition = (probe-position);
		VectorNormalize( vProbePosition );

		Vector ProbeUnit = vProbePosition;
		float ProbeLength = (probe-position).Length();
		float maxProbeLength = ProbeLength;
		float minProbeLength = 0;

		float diff = maxProbeLength - minProbeLength;
		while (diff > 1.0)
		{
			float midProbeLength = minProbeLength + diff/2.0;
			Vector midProbeVec = midProbeLength * ProbeUnit;
			if ( !(UTIL_PointContents(position+midProbeVec) & MASK_WATER) )
			{
				minProbeLength = midProbeLength;
			}
			else
			{
				maxProbeLength = midProbeLength;
			}
			diff = maxProbeLength - minProbeLength;
		}
		*pFraction = minProbeLength/ProbeLength;

		 return TRUE;
	}

	int conPosition = UTIL_PointContents(position);
	if ( ((GetFlags() & FL_SWIM) == FL_SWIM) ^ (conPosition & MASK_WATER))
	{
		//    SWIMING & !WATER
		// or FLYING  & WATER
		//
		*pFraction = 0.0;
		return TRUE; // We hit a water boundary because we are where we don't belong.
	}
	int conProbe = UTIL_PointContents(probe);
	if ( conProbe == conPosition )
	{
		// The probe is either entirely inside the water (for fish) or entirely
		// outside the water (for birds).
		//
		*pFraction = 1.0;
		return FALSE;
	}
	return TRUE;

}
