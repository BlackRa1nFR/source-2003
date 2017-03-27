//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "AI_Default.h"
#include "AI_Task.h"
#include "AI_Schedule.h"
#include "AI_Node.h"
#include "AI_Hull.h"
#include "AI_Hint.h"
#include "AI_Squad.h"
#include "AI_Senses.h"
#include "AI_Navigator.h"
#include "AI_Motor.h"
#include "ai_behavior.h"
#include "ai_baseactor.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "soundent.h"
#include "game.h"
#include "NPCEvent.h"
#include "EntityList.h"
#include "activitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

ConVar	sk_barney_health( "sk_barney_health","0");

#define CONCEPT_PLAYER_USE "CONCEPT_PLAYER_USE"

//=========================================================
// Barney activities
//=========================================================
int ACT_BARNEY_HANDSUP;
int ACT_BARNEY_HANDSDOWN;
int ACT_BARNEY_HANDSUPIDLE;
int ACT_BARNEY_COWERFLINCH;
int ACT_BARNEY_COWERIDLE;
int ACT_BARNEY_COWERSTAND;
int ACT_BARNEY_COWER;
int ACT_BARNEY_IDLEWALL;
int ACT_BARNEY_LEANONWALL;
int ACT_BARNEY_LEANOFFWALL;

//=========================================================
// Private conditions
//=========================================================

//=========================================================
// Barney schedules
//=========================================================
enum
{
	SCHED_BARNEY_COWER = LAST_SHARED_SCHEDULE,
	SCHED_BARNEY_HANDS_UP,
	SCHED_BARNEY_FLIP_OFF,
	SCHED_BARNEY_NAP,
	SCHED_BARNEY_STAND_LOOK,
	SCHED_BARNEY_RANGE_ATTACK_PISTOL
};

//=========================================================
// Barney tasks
//=========================================================
enum 
{
	TASK_BARNEY_TARGET_POLICE = LAST_SHARED_TASK,
	TASK_BARNEY_GET_LOOK_TARGET,
	TASK_BARNEY_FACE_GROUP,
};


class CNPC_Barney : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CNPC_Barney, CAI_BaseActor );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	bool	CreateBehaviors();
	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void );
	int		GetSoundInterests ( void );

	void PrescheduleThink( void );

	void HandleAnimEvent( animevent_t *pEvent );

	void OnSeeEntity( CBaseEntity *pEntity );
	void RunAI( void );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	virtual int SelectSchedule( void );
	virtual int TranslateSchedule( int scheduleType );
	virtual bool OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule );

	Activity	NPC_TranslateActivity( Activity eNewActivity );

	bool		FValidateHintType ( CAI_Hint *pHint );
	bool		IsWeaponPistol( void );

	virtual bool CanBeSelected() { return true; }

	void FollowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Touch( CBaseEntity *pOther );

	bool FInAimCone( const Vector &vecSpot );

	
	int ObjectCaps( void ) { return UsableNPCObjectCaps( BaseClass::ObjectCaps() ); }

#if 0
	// true if this citizen is rebelling against the 
	// police and combots.
	bool	m_fIsRebelling;
	float	m_flBreatheTime;
#endif

	// How long have I had no groundspeed?
	float	m_flStandStillTime;

	int		m_cVisiblePolice;
	int		m_cVisibleCitizens;

	// How long until a citizen can flip off a metropolice?
	static float	m_flTimeFlipOff;

	// How long till I can conspire with my fellow citizens?
	static float	m_flTimeConspire;
	static bool		m_fCanSleep;

	CAI_LeadBehavior 	 m_LeadBehavior;
	CAI_FollowBehavior 	 m_FollowBehavior;
	CAI_StandoffBehavior m_StandoffBehavior;
	CAI_AssaultBehavior	 m_AssaultBehavior;

	DEFINE_CUSTOM_AI;
};


LINK_ENTITY_TO_CLASS( npc_barney, CNPC_Barney );

float	CNPC_Barney::m_flTimeFlipOff = 0;
float	CNPC_Barney::m_flTimeConspire = 0;
bool	CNPC_Barney::m_fCanSleep = true;


//---------------------------------------------------------
// 
//---------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST(CNPC_Barney, DT_NPC_Barney)
END_SEND_TABLE()


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Barney )

#if 0
	DEFINE_FIELD( CNPC_Barney, m_fIsRebelling, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Barney, m_flBreatheTime, FIELD_FLOAT ),
#endif

	DEFINE_FIELD( CNPC_Barney, m_flStandStillTime, FIELD_TIME ),
	// DEFINE_FIELD( CNPC_Barney, m_cVisiblePolice, FIELD_INTEGER ) //recalc'd each think
	// DEFINE_FIELD( CNPC_Barney, m_cVisibleCitizens, FIELD_INTEGER ) //recalc'd each think

//	DEFINE_FIELD( CNPC_Barney, m_LeadBehavior, FIELD_EMBEDDED ),
//	DEFINE_FIELD( CNPC_Barney, m_FollowBehavior, FIELD_EMBEDDED ),
//	DEFINE_FIELD( CNPC_Barney, m_StandoffBehavior, FIELD_EMBEDDED ),
//	DEFINE_FIELD( CNPC_Barney, m_AssaultBehavior, FIELD_EMBEDDED ),

	// Function Pointers
	DEFINE_USEFUNC( CNPC_Barney, FollowUse ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barney::Precache( void )
{
	engine->PrecacheModel( "models/barney.mdl" );

	enginesound->PrecacheSound("weapons/punch_block1.wav");

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Barney::Spawn( void )
{
	Precache();

	SetModel( "models/barney.mdl" );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= sk_barney_health.GetFloat();
	m_flFieldOfView		= 0.2;
	m_NPCState		= NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_SQUAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_CLIMB );
	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_USE_WEAPONS | bits_CAP_AIM_GUN | bits_CAP_MOVE_SHOOT );
	CapabilitiesAdd( bits_CAP_DUCK );

	m_flStandStillTime	= -1;
#if 0
	m_fIsRebelling		= false;
#endif
	m_fCanSleep			= true;
	m_flTimeFlipOff		= gpGlobals->curtime + random->RandomFloat( 10, 30 );
	m_flTimeConspire	= gpGlobals->curtime + random->RandomFloat( 30, 30 );

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_iszIdleExpression = MAKE_STRING("scenes/Expressions/BarneyIdle.vcd");
	m_iszAlertExpression = MAKE_STRING("scenes/Expressions/BarneyAlert.vcd");
	m_iszCombatExpression = MAKE_STRING("scenes/Expressions/BarneyCombat.vcd");

	NPCInit();
	SetUse( FollowUse );
}

bool CNPC_Barney::CreateBehaviors()
{
	AddBehavior( &m_AssaultBehavior );
	AddBehavior( &m_StandoffBehavior );
	AddBehavior( &m_LeadBehavior );
	AddBehavior( &m_FollowBehavior );
	
	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Barney::Classify( void )
{
	return	CLASS_PLAYER_ALLY_VITAL;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Barney::GetSoundInterests ( void )
{
	return	SOUND_WORLD		|
			SOUND_COMBAT	|
			SOUND_PLAYER;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Barney::IsWeaponPistol( void )
{
	if( GetActiveWeapon() )
	{
		return FClassnameIs( GetActiveWeapon(), "weapon_pistol" );
	}
	
	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Barney::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case NPC_EVENT_LEFTFOOT:
		{
			EmitSound( "NPC_Barney.FootstepLeft" );
		}
		break;
	case NPC_EVENT_RIGHTFOOT:
		{
			EmitSound( "NPC_Barney.FootstepRight" );
		}
		break;
	case EVENT_WEAPON_RELOAD:
		if ( GetActiveWeapon() )
		{
			GetActiveWeapon()->WeaponSound( RELOAD_NPC );
			GetActiveWeapon()->m_iClip1 = GetActiveWeapon()->GetMaxClip1(); 
			ClearCondition(COND_LOW_PRIMARY_AMMO);
			ClearCondition(COND_NO_PRIMARY_AMMO);
			ClearCondition(COND_NO_SECONDARY_AMMO);
		}
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Barney::OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule )
{
	bool interrupt = BaseClass::OnBehaviorChangeStatus( pBehavior, fCanFinishSchedule );
	if ( !interrupt )
	{
		interrupt = ( pBehavior == &m_FollowBehavior && ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT ) );
	}
	return interrupt;

}

//---------------------------------------------------------
//---------------------------------------------------------

int CNPC_Barney::SelectSchedule( void )
{
	if ( !BehaviorSelectSchedule() )
	{
		switch ( m_NPCState )
		{
		case NPC_STATE_IDLE:
			if( m_cVisiblePolice == 0 && random->RandomInt( 0, 1 ) == 0 && m_fCanSleep ) 
			{
				m_fCanSleep = false;
				return SCHED_BARNEY_NAP;
			}

			if( m_cVisiblePolice > 0 && gpGlobals->curtime > m_flTimeFlipOff )
			{
				// hee!
				return SCHED_BARNEY_FLIP_OFF;
			}
			break;

		case NPC_STATE_ALERT:
			break;

		case NPC_STATE_COMBAT:
			// UNDONE: Better heuristic here for looking for weapons
			CBaseEntity *pEnemy;
			
			pEnemy = GetEnemy();

			if ( !GetActiveWeapon() && !HasCondition( COND_TASK_FAILED ) && Weapon_FindUsable(Vector(500,500,500)) )
			{
				return SCHED_NEW_WEAPON;
			}

			if ( HasCondition ( COND_NO_PRIMARY_AMMO ) )
			{
				return SCHED_HIDE_AND_RELOAD;
			}

			break;	
		}
	}

	return BaseClass::SelectSchedule();
}


//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Barney::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_IDLE_STAND:
		return SCHED_BARNEY_STAND_LOOK;
		break;
	case SCHED_RANGE_ATTACK1:
		if( !IsRunningBehavior() && IsWeaponPistol() )
		{
			return SCHED_BARNEY_RANGE_ATTACK_PISTOL;
		}
		break;
	}
	return BaseClass::TranslateSchedule( scheduleType );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Barney::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	// How long have I been standing still? 
	if ( m_flGroundSpeed == 0.0 )
	{
		if( m_flStandStillTime == -1 )
		{
			// I was previously moving. 
			m_flStandStillTime = gpGlobals->curtime;
		}
	}
	else
	{
		// Reset the standing still time!
		m_flStandStillTime = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CNPC_Barney::NPC_TranslateActivity( Activity activity )
{
	activity = BaseClass::NPC_TranslateActivity( activity );
	if ( activity == ACT_IDLE && (m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT) )
	{
		if (gpGlobals->curtime - m_flLastAttackTime < 3)
		{
			activity = ACT_IDLE_ANGRY;
		}
	}
	return activity;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Barney::RunAI( void )
{
	m_cVisibleCitizens = 0;
	m_cVisiblePolice = 0;

	BaseClass::RunAI();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Barney::OnSeeEntity( CBaseEntity *pEntity )
{
	if( FClassnameIs( pEntity, "npc_barney" ) )
	{
		m_cVisibleCitizens++;
	}
	else if( FClassnameIs( pEntity, "NPC_metropolice" ) )
	{
		m_cVisiblePolice++;
	}
	BaseClass::OnSeeEntity( pEntity );
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Barney::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_BARNEY_GET_LOOK_TARGET:
		{
			// In order, Citizens want to look at 
			// Player
			// Metropolice
			// Other Citizen
			AISightIter_t iter;
			CBaseEntity *pEnt;

			SetTarget( NULL );
			pEnt = GetSenses()->GetFirstSeenEntity( &iter );

			while( pEnt )
			{
				// Search all of the NPCs visible to me and find 
				// a metrocop.
				if( FClassnameIs( pEnt, "NPC_metropolice" ) )
				{
					SetTarget( pEnt );
					break;
				}

				pEnt = GetSenses()->GetNextSeenEntity( &iter );
			}
		}
		TaskComplete();
		break;

	case TASK_BARNEY_TARGET_POLICE:
		{
			AISightIter_t iter;
			CBaseEntity *pEnt;
			CAI_BaseNPC *pNPC;

			SetTarget( NULL );
			pEnt = GetSenses()->GetFirstSeenEntity( &iter );

			while( pEnt )
			{
				// Search all of the NPCs visible to me and find 
				// a metrocop.
				if( FClassnameIs( pEnt, "NPC_metropolice" ) )
				{
					pNPC = pEnt->MyNPCPointer();

					if( pNPC )
					{
						// Now, if this metrocop can't see me, make him my target!
						// Later, target the nearest cop.
						if( !pNPC->FInViewCone( this ) )
						{
							// This officer can't see me. Target him.
							SetTarget( pNPC );
							break;
						}
						else
						{
							// Oops! This officer can see me. Don't target any
							// officers for taunts/attacks, or I'll be caught.
							SetTarget( NULL );
							break;
						}
					}
				}

				pEnt = GetSenses()->GetNextSeenEntity( &iter );
			}
		}

		if( GetTarget() )
		{
			m_flTimeFlipOff = gpGlobals->curtime + random->RandomFloat( 10, 30 );
			TaskComplete();
		}
		else
		{
			m_flTimeFlipOff = gpGlobals->curtime + 5;
			TaskFail(FAIL_NO_TARGET);
		}

		break;

	case TASK_BARNEY_FACE_GROUP:
		TaskComplete();
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Barney::RunTask( const Task_t *pTask )
{
	BaseClass::RunTask( pTask );
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Barney::FValidateHintType( CAI_Hint *pHint )
{
	if ( !IsRunningBehavior() )
	{
		switch( pHint->HintType() )
		{
		case HINT_URBAN_SHELTER:
			return true;
			break;

		default:
			return false;
			break;
		}
	}

	return BaseClass::FValidateHintType( pHint );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Barney::FollowUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Speak( CONCEPT_PLAYER_USE );

	CBaseEntity *pLeader = ( !m_FollowBehavior.GetFollowTarget() ) ? pCaller : NULL;
	m_FollowBehavior.SetFollowTarget( pLeader );

	if ( m_pSquad && m_pSquad->NumMembers() > 1 )
	{
		AISquadIter_t iter;
		for (CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
		{
			CNPC_Barney *pBarney = dynamic_cast<CNPC_Barney *>(pSquadMember);
			if ( pBarney && pBarney != this)
			{
				pBarney->m_FollowBehavior.SetFollowTarget( pLeader );
			}
		}

	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Barney::Touch( CBaseEntity *pOther )
{
	BaseClass::Touch( pOther );

	// Did the player touch me?
	if ( pOther->IsPlayer() )
	{
		// Ignore if pissed at player
		if ( m_afMemory & bits_MEMORY_PROVOKED )
			return;

		// Stay put during speech
		if ( GetExpresser()->IsSpeaking() )
			return;
			
		TestPlayerPushing( pOther );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Barney::FInAimCone( const Vector &vecSpot )
{
	Vector los = ( vecSpot - GetAbsOrigin() );

	// do this in 2D
	los.z = 0;
	VectorNormalize( los );

	Vector facingDir = BodyDirection2D( );

	float flDot = DotProduct( los, facingDir );

	if ( flDot > 0.866 ) //30 degrees
		return true;

	return false;
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_barney, CNPC_Barney )

	DECLARE_USES_SCHEDULE_PROVIDER( CAI_LeadBehavior )

	DECLARE_TASK(	TASK_BARNEY_TARGET_POLICE)
	DECLARE_TASK(	TASK_BARNEY_GET_LOOK_TARGET)
	DECLARE_TASK(	TASK_BARNEY_FACE_GROUP)

	DECLARE_ACTIVITY(	ACT_BARNEY_HANDSUP)
	DECLARE_ACTIVITY(	ACT_BARNEY_HANDSDOWN)
	DECLARE_ACTIVITY(	ACT_BARNEY_HANDSUPIDLE)
	DECLARE_ACTIVITY(	ACT_BARNEY_COWERFLINCH)
	DECLARE_ACTIVITY(	ACT_BARNEY_COWERIDLE)
	DECLARE_ACTIVITY(	ACT_BARNEY_COWERSTAND)
	DECLARE_ACTIVITY(	ACT_BARNEY_COWER)
	DECLARE_ACTIVITY(	ACT_BARNEY_IDLEWALL)
	DECLARE_ACTIVITY(	ACT_BARNEY_LEANONWALL)
	DECLARE_ACTIVITY(	ACT_BARNEY_LEANOFFWALL)

	//=========================================================
	// > SCHED_BARNEY_STAND_LOOK
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_BARNEY_STAND_LOOK,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_BARNEY_GET_LOOK_TARGET	0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		"		TASK_WAIT					4"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_GIVE_WAY"
	)

	//=========================================================
	// > SCHED_BARNEY_COWER
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_BARNEY_COWER,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_BARNEY_COWERIDLE"
		"		TASK_WAIT					5"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_BARNEY_HANDSUPIDLE"
		"		TASK_WAIT					5"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_BARNEY_HANDS_UP
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_BARNEY_HANDS_UP,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_BARNEY_HANDSUPIDLE"
		"		TASK_WAIT_INDEFINITE			0"
		""
		"	Interrupts "
	)

	//=========================================================
	// > SCHED_BARNEY_FLIP_OFF
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_BARNEY_FLIP_OFF,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_BARNEY_TARGET_POLICE			0"
		"		TASK_FACE_TARGET					0"
		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_BARNEY_HANDSUPIDLE "
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							1"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_BARNEY_NAP
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_BARNEY_NAP,

		"	Tasks"
		"		TASK_FIND_LOCK_HINTNODE		0"
		"		TASK_GET_PATH_TO_HINTNODE	0"
		"		TASK_FACE_PATH				0"
		"		TASK_WALK_PATH				0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_FACE_HINTNODE			0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_BARNEY_IDLEWALL"
		"		TASK_WAIT					30"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
		""
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_BARNEY_RANGE_ATTACK_PISTOL,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"		TASK_FACE_ENEMY			0"
		"		TASK_RANGE_ATTACK1		0"
		"		TASK_WAIT				.1"
		"		TASK_WAIT_RANDOM		.4"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_RESET"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_HEAR_DANGER"
	)

AI_END_CUSTOM_NPC()
