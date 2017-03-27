//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Roller what has spikes
//
// $NoKeywords: $
//=============================================================================


//=============================================================================
//=============================================================================
// TODO:
//
// -Get rid of constants.
// -Create sound arrays where necessary
// -Rename "standoff" schedule, since it isn't a standoff anymore.
//=============================================================================
//=============================================================================


#include "cbase.h"
#include	"AI_Default.h"
#include	"AI_Task.h"
#include	"AI_Schedule.h"
#include	"AI_Hull.h"
#include	"AI_SquadSlot.h"
#include	"AI_BaseNPC.h"
#include 	"AI_Navigator.h"
#include	"ndebugoverlay.h"
#include	"NPC_Roller.h"
#include	"explode.h"
#include	"bitstring.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

#define ROLLERBULL_MAX_TORQUE_FACTOR	4

// Doing the sounds like this right now makes it easier to change the
// filenames for a certain sound event while we're experimenting.
#define ROLLERBULL_SOUND_MOVE			"npc/roller/bull_slow.wav"
#define ROLLERBULL_SOUND_MOVEFAST		"npc/roller/bull_fast.wav"
#define ROLLERBULL_SOUND_MOVEFASTEST	"npc/roller/bull_buzz.wav"

#define ROLLERBULL_IDLE_SEE_DIST	384
#define ROLLERBULL_NORMAL_SEE_DIST	4096

#define ROLLERBULL_MIN_ATTACK_DIST	384
#define ROLLERBULL_MAX_ATTACK_DIST	4096


enum RollerbullSquadMessages
{
	ROLLERBULL_SQUAD_NEW_ENEMY = 3000,
};

//=========================================================
// Custom schedules
//=========================================================
enum
{
	SCHED_ROLLERBULL_RANGE_ATTACK1 = LAST_ROLLER_SCHED,
	SCHED_ROLLERBULL_INSPECT,
	SCHED_ROLLERBULL_STANDOFF,
	SCHED_ROLLERBULL_TEST,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_ROLLERBULL_CHARGE_ENEMY = LAST_ROLLER_TASK,
	TASK_ROLLERBULL_ENABLE_ENEMY,
	TASK_ROLLERBULL_INSPECT,
	TASK_ROLLERBULL_OPEN,
	TASK_ROLLERBULL_CLOSE,			
	TASK_ROLLERBULL_WAIT,		// wait, as usual, but open blades if enemy comes close!
	TASK_ROLLERBULL_WAIT_RANDOM,
	TASK_ROLLERBULL_GET_BACK_UP_PATH,
	TASK_ROLLERBULL_SOUND,
};

//=========================================================
// Custom Conditions
//=========================================================
enum 
{
	COND_ROLLERBULL_FIRST_SIGHT = LAST_ROLLER_CONDITION,
	COND_ROLLERBULL_LOST_SIGHT,
};


//=========================================================
// Squad Slots
//=========================================================
enum
{
	SQUAD_SLOT_ROLLERBULL_GUARD	= LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_ROLLERBULL_SETUP_CHARGE,
	SQUAD_SLOT_ROLLERBULL_CHARGE,
};


// This are little 'sound event' flags. Set the flag after you play the
// sound, and the sound will not be allowed to play until the flag is then cleared.
#define ROLLERBULL_SE_CLEAR		0x00000000
#define ROLLERBULL_SE_CHARGE	0x00000001
#define ROLLERBULL_SE_TAUNT		0x00000002
#define ROLLERBULL_SE_SHARPEN	0x00000004
#define ROLLERBULL_SE_TOSSED	0x00000008

//=========================================================
//=========================================================
class CNPC_RollerBull : public CNPC_Roller
{
	DECLARE_CLASS( CNPC_RollerBull, CNPC_Roller );

public:
	void Spawn( void );
	int RangeAttack1Conditions ( float flDot, float flDist );
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	virtual int	SelectSchedule ( void );

	void Open( void );
	void Close( void );

	void Activate( void );

	void PrescheduleThink( void );

	CBaseEntity *BestEnemy ( void );

	void SpikeTouch( CBaseEntity *pOther );

	void Precache( void );
	Class_T	Classify( void ) { return CLASS_COMBINE; }

	virtual bool HandleInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt );
	void BuildScheduleTestBits( void );

	void PowerOnSound( void );
	void PowerOffSound( void );

	int RollerPhysicsDamageMask( void );
	void OnStateChange( NPC_STATE OldState, NPC_STATE NewState );
	void EnableEnemy ( int iEnable );

	void IdleSound( void );

	void Bark( void );

	void PowerOn( void );
	void PowerOff( void );

	float m_flChargeTime;
	bool m_fIsOpen;

	bool m_fHackVisibilityFlag;

	bool m_fEnableEnemy;

	int m_iSoundEventFlags;

	CSoundPatch *m_pRollSoundFast;

	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( npc_rollerbull, CNPC_RollerBull );

IMPLEMENT_CUSTOM_AI( npc_rollerbull, CNPC_RollerBull );


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CNPC_RollerBull )

	// Function Pointers
	DEFINE_ENTITYFUNC( CNPC_RollerBull, SpikeTouch ),

	DEFINE_FIELD( CNPC_RollerBull, m_flChargeTime, FIELD_TIME ),
	DEFINE_FIELD( CNPC_RollerBull, m_fIsOpen, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_RollerBull, m_fHackVisibilityFlag, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_RollerBull, m_fEnableEnemy, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_RollerBull, m_iSoundEventFlags, FIELD_INTEGER ),
	DEFINE_SOUNDPATCH( CNPC_RollerBull, m_pRollSoundFast ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI( CNPC_RollerBull );

	ADD_CUSTOM_SCHEDULE( CNPC_RollerBull,		SCHED_ROLLERBULL_RANGE_ATTACK1 );
	ADD_CUSTOM_SCHEDULE( CNPC_RollerBull,		SCHED_ROLLERBULL_INSPECT );
	ADD_CUSTOM_SCHEDULE( CNPC_RollerBull,		SCHED_ROLLERBULL_STANDOFF );
	ADD_CUSTOM_SCHEDULE( CNPC_RollerBull,		SCHED_ROLLERBULL_TEST );

	ADD_CUSTOM_TASK( CNPC_RollerBull,			TASK_ROLLERBULL_CHARGE_ENEMY );
	ADD_CUSTOM_TASK( CNPC_RollerBull,			TASK_ROLLERBULL_ENABLE_ENEMY );
	ADD_CUSTOM_TASK( CNPC_RollerBull,			TASK_ROLLERBULL_INSPECT );
	ADD_CUSTOM_TASK( CNPC_RollerBull,			TASK_ROLLERBULL_CLOSE );
	ADD_CUSTOM_TASK( CNPC_RollerBull,			TASK_ROLLERBULL_OPEN );
	ADD_CUSTOM_TASK( CNPC_RollerBull,			TASK_ROLLERBULL_WAIT );
	ADD_CUSTOM_TASK( CNPC_RollerBull,			TASK_ROLLERBULL_WAIT_RANDOM );
	ADD_CUSTOM_TASK( CNPC_RollerBull,			TASK_ROLLERBULL_GET_BACK_UP_PATH );
	ADD_CUSTOM_TASK( CNPC_RollerBull,			TASK_ROLLERBULL_SOUND );

	ADD_CUSTOM_CONDITION( CNPC_RollerBull,		COND_ROLLERBULL_FIRST_SIGHT );
	ADD_CUSTOM_CONDITION( CNPC_RollerBull,		COND_ROLLERBULL_LOST_SIGHT );

	ADD_CUSTOM_SQUADSLOT( CNPC_RollerBull,		SQUAD_SLOT_ROLLERBULL_GUARD );
	ADD_CUSTOM_SQUADSLOT( CNPC_RollerBull,		SQUAD_SLOT_ROLLERBULL_SETUP_CHARGE );
	ADD_CUSTOM_SQUADSLOT( CNPC_RollerBull,		SQUAD_SLOT_ROLLERBULL_CHARGE );

	AI_LOAD_SCHEDULE( CNPC_RollerBull,		SCHED_ROLLERBULL_RANGE_ATTACK1 );
	AI_LOAD_SCHEDULE( CNPC_RollerBull,		SCHED_ROLLERBULL_INSPECT );
	AI_LOAD_SCHEDULE( CNPC_RollerBull,		SCHED_ROLLERBULL_STANDOFF );
	AI_LOAD_SCHEDULE( CNPC_RollerBull,		SCHED_ROLLERBULL_TEST );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::Precache( void )
{
	engine->PrecacheModel( "models/roller.mdl" );
	engine->PrecacheModel( "models/roller_spikes.mdl" );

	enginesound->PrecacheSound( ROLLERBULL_SOUND_MOVE );
	enginesound->PrecacheSound( ROLLERBULL_SOUND_MOVEFAST );
	enginesound->PrecacheSound( ROLLERBULL_SOUND_MOVEFASTEST );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::Spawn( void )
{
	BaseClass::Spawn();

	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_SQUAD );

	Close();

	m_flForwardSpeed = -1200;

	m_fHackVisibilityFlag = false;

	EnableEnemy( false );
}

//-----------------------------------------------------------------------------
// Purpose: Get my sound patches started!
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::Activate( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CPASAttenuationFilter filter( this );

	m_pRollSoundFast = controller.SoundCreate( filter, entindex(), CHAN_BODY, ROLLERBULL_SOUND_MOVEFAST, 1.5 );

	BaseClass::Activate();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_RollerBull::RangeAttack1Conditions( float flDot, float flDist )
{
	if( !HasCondition( COND_SEE_ENEMY ) )
	{
		return COND_NONE;
	}

	if(flDist > ROLLERBULL_MAX_ATTACK_DIST  )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDist < ROLLERBULL_MIN_ATTACK_DIST )
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::PrescheduleThink( void )
{
	if( m_NPCState == NPC_STATE_IDLE )
	{
		// Don't see very far in IDLE state.
		SetDistLook( ROLLERBULL_IDLE_SEE_DIST );
	}
	else
	{
		SetDistLook( ROLLERBULL_NORMAL_SEE_DIST );
	}

	if( HasCondition( COND_SEE_HATE ) && m_NPCState != NPC_STATE_COMBAT )
	{
		// See father during this time so player can't simply back up a couple of 
		// feet outside of the limited IDLE state vision radius
		SetDistLook( ROLLERBULL_NORMAL_SEE_DIST );

		if( !m_fHackVisibilityFlag )
		{
			// Start 'tracking' suspect.
			m_fHackVisibilityFlag = true;
			SetCondition( COND_ROLLERBULL_FIRST_SIGHT );
			ClearCondition( COND_ROLLERBULL_LOST_SIGHT );
		}
	}
	else
	{
		m_fHackVisibilityFlag = false;
		ClearCondition( COND_ROLLERBULL_FIRST_SIGHT );
		SetCondition( COND_ROLLERBULL_LOST_SIGHT );
	}

	BaseClass::PrescheduleThink();
}


//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_RollerBull::BuildScheduleTestBits( void )
{
	//Interrupt IDLE schedule with FIRST_SIGHT
	if ( IsCurSchedule( SCHED_IDLE_STAND ) || IsCurSchedule( SCHED_ROLLER_PATROL ) )
	{
		SetCustomInterruptCondition( COND_ROLLERBULL_FIRST_SIGHT );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_RollerBull::SelectSchedule ( void )
{
//	return SCHED_ROLLERBULL_TEST;

	if( HasCondition( COND_ROLLER_PHYSICS ) )
	{
		// Call the base class, right now!
		return BaseClass::SelectSchedule();
	}

	if( HasCondition( COND_ROLLERBULL_FIRST_SIGHT ) )
	{
		// See if we should make this an enemy!
		return SCHED_ROLLERBULL_INSPECT;
	}

	switch( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		if( HasCondition( COND_TOO_CLOSE_TO_ATTACK ) )
		{
			return SCHED_ROLLERBULL_STANDOFF;
		}

		if( HasCondition( COND_CAN_RANGE_ATTACK1 ) && !HasCondition( COND_ROLLER_PHYSICS ) )
		{
			return SCHED_ROLLERBULL_RANGE_ATTACK1;
		}
		
		if( !HasCondition( COND_CAN_RANGE_ATTACK1 ) && !HasCondition( COND_ROLLER_PHYSICS ) )
		{
			return SCHED_CHASE_ENEMY;
		}
		break;

	default:
		break;
	}

	return BaseClass::SelectSchedule();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLERBULL_SOUND:
		if( pTask->flTaskData == 0 )
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.SoundFadeOut( m_pRollSound, 1 );
		}
		else
		{
			CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
			controller.Play( m_pRollSound, 1.0, 100 );
		}

		TaskComplete();

		break;

	case TASK_ROLLERBULL_GET_BACK_UP_PATH:
		if( GetEnemy() == NULL )
		{
			TaskFail( "no enemy!" );
		}
		else
		{
			Vector vecDir;
			trace_t tr;

			VectorSubtract( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), vecDir );

			vecDir.z = 0;

			AI_TraceLine( GetAbsOrigin(), GetAbsOrigin() + vecDir * 300, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			if ( GetNavigator()->SetGoal( tr.endpos, AIN_CLEAR_TARGET ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there
				TaskFail(FAIL_NO_ROUTE);
			}
		}
		break;

	case TASK_ROLLERBULL_WAIT:
		m_flWaitFinished = gpGlobals->curtime + pTask->flTaskData;
		break;

	case TASK_ROLLERBULL_WAIT_RANDOM:
		m_flWaitFinished = gpGlobals->curtime + random->RandomFloat( 0.1, pTask->flTaskData );
		break;

	case TASK_ROLLERBULL_OPEN:
		Open();
		TaskComplete();
		break;

	case TASK_ROLLERBULL_CLOSE:
		Close();
		TaskComplete();
		break;

	case TASK_ROLLERBULL_INSPECT:
		{
			Vector vecForce;

			vecForce.z = 10;
			vecForce.x = random->RandomFloat( -0.3, 0.3 );
			vecForce.y = random->RandomFloat( -0.3, 0.3 );

			VPhysicsGetObject()->Wake();
			VPhysicsGetObject()->ApplyForceCenter( vecForce * 500 );
			VPhysicsGetObject()->ApplyTorqueCenter( vecForce * 255 );

			EmitSound( "NPC_RollerBull.Measure" );
			TaskComplete();
		}
		break;

	case TASK_ROLLERBULL_ENABLE_ENEMY:
		EnableEnemy( pTask->flTaskData );
		TaskComplete();
		break;

	case TASK_ROLLER_WAIT_FOR_PHYSICS:
		if( m_fIsOpen )
		{
			Close();
		}

		m_iSoundEventFlags &= ~ROLLERBULL_SE_TOSSED;
		BaseClass::StartTask( pTask );
		break;

	case TASK_ROLLERBULL_CHARGE_ENEMY:
		VPhysicsGetObject()->Wake();
		m_flChargeTime = gpGlobals->curtime;

		//CHIACHIN
		{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		controller.Play( m_pRollSoundFast, 0.0, 80 );
		controller.SoundChangeVolume( m_pRollSoundFast, 0.5, 1.0 );
		controller.SoundChangePitch( m_pRollSoundFast, 110, 2.0 );
		
		controller.SoundChangePitch( m_pRollSound, 85, 1.0 );
		controller.SoundFadeOut( m_pRollSound, 1.0 );
		}
		//CHIACHIN

		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ROLLERBULL_WAIT_RANDOM:
	case TASK_ROLLERBULL_WAIT:
		if( GetEnemy() != NULL )
		{
			float flDist = UTIL_DistApprox( GetLocalOrigin(), GetEnemy()->GetLocalOrigin() );

			if( flDist <= 150.0 && UTIL_DistApprox2D( GetLocalOrigin(), GetEnemy()->GetLocalOrigin() ) > 50 )
			{
				if( !m_fIsOpen )
				{
					Vector vecForce;

					vecForce.z = 10;
					vecForce.x = random->RandomFloat( -0.3, 0.3 );
					vecForce.y = random->RandomFloat( -0.3, 0.3 );

					VPhysicsGetObject()->Wake();
					VPhysicsGetObject()->ApplyForceCenter( vecForce * 500 );
					VPhysicsGetObject()->ApplyTorqueCenter( vecForce * 200 );
					Open();
				}
			}
			else if( flDist > 200.0 && m_fIsOpen )
			{
				Vector vecForce;

				vecForce.z = 10;
				vecForce.x = random->RandomFloat( -0.3, 0.3 );
				vecForce.y = random->RandomFloat( -0.3, 0.3 );

				VPhysicsGetObject()->Wake();
				VPhysicsGetObject()->ApplyTorqueCenter( vecForce * 100 );

				Close();
			}

			if( !m_fIsOpen )
			{
				if( random->RandomInt( 0, 15 ) == 1 )
				{
					Bark();
				}
			}
		}
		
		if ( gpGlobals->curtime >= m_flWaitFinished )
		{
			TaskComplete();
		}
		break;

	case TASK_ROLLER_WAIT_FOR_PHYSICS:
		if( !(m_iSoundEventFlags & ROLLERBULL_SE_TOSSED ) )
		{
			// Squeal if we get tossed around whilst waiting for physics
			IPhysicsObject *pPhysicsObject;
			Vector vecVelocity;

			pPhysicsObject = VPhysicsGetObject();
			pPhysicsObject->GetVelocity( &vecVelocity, NULL );

			if( vecVelocity.Length() >= 400 )
			{
				m_iSoundEventFlags |= ROLLERBULL_SE_TOSSED;
				EmitSound( "NPC_RollerBull.Tossed" );
			}
		}

		BaseClass::RunTask( pTask );
		break;

	case TASK_ROLLERBULL_CHARGE_ENEMY:
		{
			float flTorqueFactor;
			float yaw = UTIL_VecToYaw( GetEnemy()->GetLocalOrigin() - GetLocalOrigin() );
			Vector vecRight;

			AngleVectors( QAngle( 0, yaw, 0 ), NULL, &vecRight, NULL );

			//NDebugOverlay::Line( GetLocalOrigin(), GetLocalOrigin() + (GetEnemy()->GetLocalOrigin() - GetLocalOrigin()), 0,255,0, true, 0.1 );

			float flDot;

			// Figure out whether to continue the charge.
			// (Have I overrun the target?)
			Vector vecVelocity, vecToTarget;
			IPhysicsObject *pPhysicsObject ;

			pPhysicsObject = VPhysicsGetObject();
			pPhysicsObject->GetVelocity( &vecVelocity, NULL );
			VectorNormalize( vecVelocity );

			vecToTarget = GetEnemy()->GetLocalOrigin() - GetLocalOrigin();
			VectorNormalize( vecToTarget );

			flDot = DotProduct( vecVelocity, vecToTarget );

			// more torque the longer the roller has been going.
			flTorqueFactor = 1 + (gpGlobals->curtime - m_flChargeTime) * 2;

			if( flTorqueFactor < 1 )
			{
				flTorqueFactor = 1;
			}
			else if( flTorqueFactor > ROLLERBULL_MAX_TORQUE_FACTOR)
			{
				flTorqueFactor = ROLLERBULL_MAX_TORQUE_FACTOR;
			}

			Vector vecCompensate;

			vecCompensate.x = vecVelocity.y;
			vecCompensate.y = -vecVelocity.x;
			vecCompensate.z = 0;
			VectorNormalize( vecCompensate );

			m_RollerController.m_vecAngular = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecCompensate, m_flForwardSpeed * -0.75 );
			m_RollerController.m_vecAngular += WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, m_flForwardSpeed  * flTorqueFactor );
		
			// Taunt when I get closer
			if( !(m_iSoundEventFlags & ROLLERBULL_SE_TAUNT) && UTIL_DistApprox( GetLocalOrigin(), GetEnemy()->GetLocalOrigin() ) <= 400 )
			{
				m_iSoundEventFlags |= ROLLERBULL_SE_TAUNT; // Don't repeat.

				EmitSound( "NPC_RollerBull.Taunt" );
			}

			// Open the spikes if i'm close enough to cut the enemy!!
			if( !m_fIsOpen && UTIL_DistApprox( GetLocalOrigin(), GetEnemy()->GetLocalOrigin() ) <= 300 )
			{
				Open();
			}
			else if ( m_fIsOpen && UTIL_DistApprox( GetLocalOrigin(), GetEnemy()->GetLocalOrigin() ) >= 400 )
			{
				// Otherwise close them if the enemy is getting away!
				Close();
			}

			// If we drive past, close the blades and make a new plan.
			if( vecVelocity.x != 0 && vecVelocity.y != 0 )
			{
				if( gpGlobals->curtime - m_flChargeTime > 1.0 && flTorqueFactor > 1 &&  flDot < 0.0 )
				{
					if( m_fIsOpen )
					{
						PowerOff();
						Close();
					}

					TaskComplete();
				}
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Bullets will affect a bull's path, but won't cause it to close.
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_RollerBull::RollerPhysicsDamageMask( void )
{
	return ( DMG_PHYSGUN | DMG_BLAST | DMG_SONIC );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::Open( void )
{
	SetModel( "models/roller_spikes.mdl" );
	EmitSound( "NPC_RollerBull.OpenSpikes" );

// CHIACHIN
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	controller.SoundChangeVolume( m_pRollSoundFast, 1.0, 0.0 );
	controller.SoundChangePitch( m_pRollSoundFast, 130, 0.0 );
// CHIACHIN

	SetTouch( SpikeTouch );
	m_fIsOpen = true;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::Close( void )
{
	SetModel( "models/roller.mdl" );
	EmitSound( "NPC_RollerBull.CloseSpikes" );

	if( m_pRollSoundFast )	
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundChangePitch( m_pRollSoundFast, 85, 1.0 );
		controller.SoundFadeOut( m_pRollSoundFast, 1.0 );
	}

	SetTouch( NULL );
	m_fIsOpen = false;

	m_iSoundEventFlags = ROLLERBULL_SE_CLEAR;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::SpikeTouch( CBaseEntity *pOther )
{
	if( pOther->m_takedamage != DAMAGE_NO && pOther->m_iClassname != m_iClassname )
	{
		EmitSound( "NPC_RollerBull.Cut" );

		Vector vecVelocity;
		VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );

#if 0
		Msg( "COLLIDE VEL:%f\f",UTIL_DistApprox( vec3_origin, vecVelocity ) );
#endif

		CTakeDamageInfo info( this, this, 15, DMG_SLASH );
		if ( UTIL_DistApprox( vec3_origin, vecVelocity ) < 100 )
		{
			// Less damage for slow speeds and little contact nips
			info.SetDamage( 5 );
		}
		trace_t tr = GetTouchTrace();
		CalculateMeleeDamageForce( &info, vecVelocity, tr.endpos );
		pOther->TakeDamage( info );

		Close();
		TaskComplete();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Play an "Alert Sound"
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	if( NewState == NPC_STATE_COMBAT )
	{
		EmitSound( "NPC_RollerBull.Summon" );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::EnableEnemy ( int iEnable )
{
	if( iEnable )
	{
		m_fEnableEnemy = true;
	}
	else
	{
		m_fEnableEnemy = false;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_RollerBull::BestEnemy ( void )
{
	if( m_fEnableEnemy )
	{
		return BaseClass::BestEnemy();
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CNPC_RollerBull::HandleInteraction( int interactionType, void *data, CBaseCombatCharacter* sourceEnt )
{
	const int fut = interactionType;

	switch( fut )
	{
	case ROLLERBULL_SQUAD_NEW_ENEMY:
		ExplosionCreate( GetLocalOrigin(), vec3_angle, this, 16.0f, 0, false );
		break;

	default:
		return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
		break;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::IdleSound( void )
{
	EmitSound( "NPC_RollerBull.Idle" );	
}


//-----------------------------------------------------------------------------
// Purpose: Harrass the player.
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::Bark( void )
{
	Vector vecForce;

	vecForce.z = 10;
	vecForce.x = random->RandomFloat( -0.2, 0.2 );
	vecForce.y = random->RandomFloat( -0.2, 0.2 );

	VPhysicsGetObject()->Wake();
	VPhysicsGetObject()->ApplyForceCenter( vecForce * 200 );

	EmitSound( "NPC_RollerBull.Bark" );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::PowerOn( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.Play( m_pRollSound, 0.0, 85 );
	controller.SoundChangeVolume( m_pRollSound, 1.0, 0.5 );
	controller.SoundChangePitch( m_pRollSound, 100, 0.5 );

	BaseClass::PowerOn();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::PowerOff( void )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundChangePitch( m_pRollSound, 85, 0.5 );
	controller.SoundFadeOut( m_pRollSound, 0.5 );

	BaseClass::PowerOff();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::PowerOnSound( void )
{
	Msg( "POWER ON SOUND\n" );
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_RollerBull::PowerOffSound( void )
{
	Msg( "POWER OFF SOUND\n" );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERBULL_TEST,

	"	Tasks"
	"		TASK_WAIT				2"
	"		TASK_ROLLERBULL_SOUND			1"
	"		TASK_WAIT				5"
	"		TASK_ROLLERBULL_SOUND			0"
	"		TASK_WAIT				2"
	"		TASK_ROLLERBULL_SOUND			1"
	"	"
	"	Interrupts"
);

//=========================================================
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERBULL_RANGE_ATTACK1,

	"	Tasks"
	"		TASK_ROLLER_ON				0"
	"		TASK_ROLLERBULL_CHARGE_ENEMY		0"
	"	"
	"	Interrupts"
	"		COND_ENEMY_DEAD"
	"		COND_NEW_ENEMY"
	"		COND_ROLLER_PHYSICS"
	"		COND_ENEMY_OCCLUDED"
);

//=========================================================
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERBULL_INSPECT,

	"	Tasks"
	"		TASK_ROLLER_OFF				0"
	"		TASK_WAIT				1"
	"		TASK_ROLLERBULL_INSPECT			0"
	"		TASK_WAIT				3"
	"		TASK_ROLLERBULL_ENABLE_ENEMY		1"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ROLLERBULL_LOST_SIGHT"
);

//=========================================================
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_ROLLERBULL_STANDOFF,

	"	Tasks"
	"		TASK_ROLLERBULL_GET_BACK_UP_PATH	0"
	"		TASK_RUN_PATH				0"
	"		TASK_WAIT_FOR_MOVEMENT			0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_OCCLUDED"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_ROLLER_PHYSICS"
);
