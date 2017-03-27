/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"cbase.h"

#if 0

#include	"npc_talker.h"
#include	"ai_schedule.h"
#include	"scripted.h"
#include	"basecombatweapon.h"
#include	"soundent.h"
#include	"NPCEvent.h"
#include	"AI_Hull.h"
#include	"AI_Node.h"
#include	"AI_Network.h"
#include	"npc_conscript.h"
#include	"activitylist.h"
#include "AI_Interactions.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

ConVar	sk_conscript_health( "sk_conscript_health","0");

#define CONSCRIPT_MAD 		"CONSCRIPT_MAD"
#define CONSCRIPT_SHOT 		"CONSCRIPT_SHOT"
#define CONSCRIPT_KILL 		"CONSCRIPT_KILL"
#define CONSCRIPT_OUT_AMMO 	"CONSCRIPT_OUT_AMMO"
#define CONSCRIPT_ATTACK 	"CONSCRIPT_ATTACK"
#define CONSCRIPT_LINE_FIRE "CONSCRIPT_LINE_FIRE"
#define CONSCRIPT_POK 		"CONSCRIPT_POK"

#define CONSCRIPT_PAIN1		"CONSCRIPT_PAIN1"
#define CONSCRIPT_PAIN2		"CONSCRIPT_PAIN2"
#define CONSCRIPT_PAIN3		"CONSCRIPT_PAIN3"

#define CONSCRIPT_DIE1		"CONSCRIPT_DIE1"
#define CONSCRIPT_DIE2		"CONSCRIPT_DIE2"
#define CONSCRIPT_DIE3		"CONSCRIPT_DIE3"

//=========================================================
// Combine activities
//=========================================================
int	ACT_CONSCRIPT_AIM;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		CONSCRIPT_AE_RELOAD		( 1 )
#define		CONSCRIPT_AE_DRAW		( 2 )
#define		CONSCRIPT_AE_SHOOT		( 3 )
#define		CONSCRIPT_AE_HOLSTER	( 4 )

#define		CONSCRIPT_BODY_GUNHOLSTERED	0
#define		CONSCRIPT_BODY_GUNDRAWN		1
#define		CONSCRIPT_BODY_GUNGONE		2

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Conscript::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CNPC_Conscript);

	ADD_CUSTOM_TASK(CNPC_Conscript,	TASK_CONSCRIPT_CROUCH);

	ADD_CUSTOM_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_FOLLOW);
	ADD_CUSTOM_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_DRAW);
	ADD_CUSTOM_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_FACE_TARGET);
	ADD_CUSTOM_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_STAND);
	ADD_CUSTOM_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_AIM);
	ADD_CUSTOM_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_BARNACLE_HIT);
	ADD_CUSTOM_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_BARNACLE_PULL);
	ADD_CUSTOM_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_BARNACLE_CHOMP);
	ADD_CUSTOM_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_BARNACLE_CHEW);

	ADD_CUSTOM_ACTIVITY(CNPC_Combine,	ACT_CONSCRIPT_AIM);

	AI_LOAD_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_FOLLOW);
	AI_LOAD_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_DRAW);
	AI_LOAD_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_FACE_TARGET);
	AI_LOAD_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_STAND);
	AI_LOAD_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_AIM);
	AI_LOAD_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_BARNACLE_HIT);
	AI_LOAD_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_BARNACLE_PULL);
	AI_LOAD_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_BARNACLE_CHOMP);
	AI_LOAD_SCHEDULE(CNPC_Conscript,	SCHED_CONSCRIPT_BARNACLE_CHEW);
}

LINK_ENTITY_TO_CLASS( npc_conscript, CNPC_Conscript );
IMPLEMENT_CUSTOM_AI( npc_conscript, CNPC_Conscript );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Conscript )

	DEFINE_FIELD( CNPC_Conscript, m_fGunDrawn,			FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Conscript, m_painTime,			FIELD_TIME ),
	DEFINE_FIELD( CNPC_Conscript, m_checkAttackTime,	FIELD_TIME ),
	DEFINE_FIELD( CNPC_Conscript, m_nextLineFireTime,	FIELD_TIME ),
	DEFINE_FIELD( CNPC_Conscript, m_lastAttackCheck,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CNPC_Conscript, m_bInBarnacleMouth,	FIELD_BOOLEAN ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Conscript::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (GetEnemy() != NULL && (GetEnemy()->IsPlayer()))
		{
			m_flPlaybackRate = 1.5;
		}
		BaseClass::RunTask( pTask );
		break;
	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CNPC_Conscript::NPC_TranslateActivity( Activity eNewActivity )
{
	return BaseClass::NPC_TranslateActivity( eNewActivity );
}


//=========================================================
// GetSoundInterests - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CNPC_Conscript::GetSoundInterests ( void) 
{
	return	SOUND_WORLD		|
			SOUND_COMBAT	|
			SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_DANGER	|
			SOUND_PLAYER;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Class_T	CNPC_Conscript::Classify ( void )
{
	return	CLASS_CONSCRIPT;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CNPC_Conscript::AlertSound( void )
{
	if ( GetEnemy() != NULL )
	{
		if ( IsOkToCombatSpeak() )
		{
			Speak( CONSCRIPT_ATTACK );
		}
	}

}
//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CNPC_Conscript::MaxYawSpeed ( void )
{
	switch ( GetActivity() )
	{
	case ACT_IDLE:		
		return 70;
		break;
	case ACT_WALK:
		return 70;
		break;
	case ACT_RUN:
		return 90;
		break;
	default:
		return 70;
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Set proper blend for shooting
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Conscript::ConscriptFirePistol ( void )
{
	Vector vecShootOrigin;

	vecShootOrigin = GetLocalOrigin() + Vector( 0, 0, 55 );
	Vector vecShootDir = GetShootEnemyDir( vecShootOrigin );

	QAngle angDir;
	VectorAngles( vecShootDir, angDir );
	SetPoseParameter( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CNPC_Conscript::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case CONSCRIPT_AE_RELOAD:
		
		// We never actually run out of ammo, just need to refill the clip
		if (GetActiveWeapon())
		{
			GetActiveWeapon()->WeaponSound( RELOAD_NPC );
			GetActiveWeapon()->m_iClip1 = GetActiveWeapon()->GetMaxClip1(); 
			GetActiveWeapon()->m_iClip2 = GetActiveWeapon()->GetMaxClip2(); 
		}
		ClearCondition(COND_NO_PRIMARY_AMMO);
		ClearCondition(COND_NO_SECONDARY_AMMO);
		break;

	case CONSCRIPT_AE_SHOOT:
		ConscriptFirePistol();
		break;

	case CONSCRIPT_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		m_nBody = CONSCRIPT_BODY_GUNDRAWN;
		m_fGunDrawn = true;
		break;

	case CONSCRIPT_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		m_nBody = CONSCRIPT_BODY_GUNHOLSTERED;
		m_fGunDrawn = false;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CNPC_Conscript::Spawn()
{
	Precache( );

	SetModel( "models/conscript.mdl" );
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= sk_conscript_health.GetFloat();
	SetViewOffset( Vector ( 0, 0, 50 ) );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_NPCState			= NPC_STATE_NONE;

	m_nBody			= 0; // gun in holster
	m_fGunDrawn			= false;
	m_bInBarnacleMouth	= false;

	m_nextLineFireTime	= 0;

	m_HackedGunPos		= Vector ( 0, 0, 55 );

	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB | bits_CAP_NO_HIT_PLAYER);
	CapabilitiesAdd	( bits_CAP_USE_WEAPONS );
	CapabilitiesAdd	( bits_CAP_DUCK );			// In reloading and cover

	NPCInit();
	SetUse( FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_Conscript::Precache()
{
	engine->PrecacheModel("models/conscript.mdl");

	enginesound->PrecacheSound("barney/ba_pain1.wav");
	enginesound->PrecacheSound("barney/ba_pain2.wav");
	enginesound->PrecacheSound("barney/ba_pain3.wav");

	enginesound->PrecacheSound("barney/ba_die1.wav");
	enginesound->PrecacheSound("barney/ba_die2.wav");
	enginesound->PrecacheSound("barney/ba_die3.wav");
	
	enginesound->PrecacheSound("barney/ba_close.wav");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	BaseClass::Precache();
}	

// Init talk data
void CNPC_Conscript::TalkInit()
{
	
	BaseClass::TalkInit();

		// vortigaunt will try to talk to friends in this order:
	m_szFriends[0] = "npc_conscript";
//	m_szFriends[1] = "npc_vortigaunt"; disable for now

	// get voice for head - just one barney voice for now
	GetExpresser()->SetVoicePitch( 100 );
}


static bool IsFacing( CBaseCombatCharacter *pBCC, const Vector &reference )
{
	Vector vecDir = (reference - pBCC->GetLocalOrigin());
	vecDir.z = 0;
	VectorNormalize( vecDir );
	Vector vecForward = pBCC->BodyDirection2D( );

	// He's facing me, he meant it
	if ( DotProduct( vecForward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_Conscript::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = BaseClass::OnTakeDamage_Alive( info );
	if (!IsAlive())
	{
		return ret;
	}

	if ( m_NPCState != NPC_STATE_PRONE && (info.GetAttacker()->GetFlags() & FL_CLIENT) )
	{

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( GetEnemy() == NULL )
		{
			// If I'm already suspicious, get mad
			if (m_afMemory & bits_MEMORY_SUSPICIOUS)
			{
				// Alright, now I'm pissed!
				Speak( CONSCRIPT_MAD );

				Remember( bits_MEMORY_PROVOKED );

				// Allowed to hit the player now!
				CapabilitiesRemove(bits_CAP_NO_HIT_PLAYER);

				StopFollowing();
			}
			else
			{
				// Hey, be careful with that
				Speak( CONSCRIPT_SHOT );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(GetEnemy()->IsPlayer()) && (m_lifeState != LIFE_DEAD ))
		{
			Speak( CONSCRIPT_SHOT );
		}
	}
	return ret;
}

//------------------------------------------------------------------------------
// Purpose : Override to always shoot at eyes (for ducking behind things)
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_Conscript::BodyTarget( const Vector &posSrc, bool bNoisy ) 
{
	return EyePosition();
}

//=========================================================
// PainSound
//=========================================================
void CNPC_Conscript::PainSound ( void )
{
	if (gpGlobals->curtime < m_painTime)
		return;

	AIConcept_t concepts[] =
	{
		CONSCRIPT_PAIN1,
		CONSCRIPT_PAIN2,
		CONSCRIPT_PAIN3,
	};
	
	m_painTime = gpGlobals->curtime + random->RandomFloat(0.5, 0.75);

	Speak( concepts[random->RandomInt(0,2)] );
}

//=========================================================
// DeathSound 
//=========================================================
void CNPC_Conscript::DeathSound ( void )
{
	AIConcept_t concepts[] =
	{
		CONSCRIPT_DIE1,
		CONSCRIPT_DIE2,
		CONSCRIPT_DIE3,
	};
	
	Speak( concepts[random->RandomInt(0,2)] );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Conscript::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	switch( ptr->hitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			info.ScaleDamage( 0.5f );
		}
		break;
	case 10:
		if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			info.SetDamage( info.GetDamage() - 20 );
			if (info.GetDamage() <= 0)
			{
				g_pEffects->Ricochet( ptr->endpos, (vecDir*-1.0f) );
				info.SetDamage( 0 );
			}
		}
		// always a head shot
		ptr->hitgroup = HITGROUP_HEAD;
		break;
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Conscript::TranslateSchedule( int scheduleType )
{
	int baseType;

	switch( scheduleType )
	{
	case SCHED_CONSCRIPT_DRAW:
		if ( GetEnemy() != NULL )
		{
			// face enemy, then draw.
			return SCHED_CONSCRIPT_DRAW;
		}
		else
		{
			// BUGBUG: What is this code supposed to do when there isn't an enemy?
			Warning( "BUG IN CONSCRIPT AI!\n");
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		{
			// call base class default so that barney will talk
			// when 'used' 
			baseType = BaseClass::TranslateSchedule(scheduleType);

			if (baseType == SCHED_IDLE_STAND)
				return SCHED_CONSCRIPT_FACE_TARGET;	// override this for different target face behavior
			else
				return baseType;
		}
		break;

	case SCHED_CHASE_ENEMY:
		{
			// ---------------------------------------------
			// If I'm in ducking, cover pause for while
			// before running towards my enemy.  See if they
			// come out first as this is a good place to be!
			// ---------------------------------------------
			if (HasMemory(bits_MEMORY_INCOVER))
			{
				Forget( bits_MEMORY_INCOVER );
				return SCHED_COMBAT_SWEEP;
			}
		}
		break;
	case SCHED_TARGET_CHASE:
		return SCHED_CONSCRIPT_FOLLOW;
		break;

	case SCHED_IDLE_STAND:
		{
			// call base class default so that scientist will talk
			// when standing during idle
			baseType = BaseClass::TranslateSchedule(scheduleType);

			if (baseType == SCHED_IDLE_STAND)
			{
				// just look straight ahead
				return SCHED_CONSCRIPT_STAND;
			}
			return baseType;
			break;

		}
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{
			return SCHED_CONSCRIPT_AIM;
			break;
		}
	}
	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Conscript::SelectSchedule ( void )
{
	// These things are done in any state but dead and prone
	if (m_NPCState != NPC_STATE_DEAD && m_NPCState != NPC_STATE_PRONE)
	{
		if ( HasCondition( COND_HEAR_DANGER ) )
		{
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
		}
		if ( HasCondition( COND_ENEMY_DEAD ) && IsOkToCombatSpeak() )
		{
			Speak( CONSCRIPT_KILL );
		}
	}
	switch( m_NPCState )
	{
	case NPC_STATE_PRONE:
		{
			if (m_bInBarnacleMouth)
			{
				return SCHED_CONSCRIPT_BARNACLE_CHOMP;
			}
			else
			{
				return SCHED_CONSCRIPT_BARNACLE_HIT;
			}
		}
	case NPC_STATE_COMBAT:
		{
// dead enemy
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			// always act surprized with a new enemy
			if ( HasCondition( COND_NEW_ENEMY ) && HasCondition( COND_LIGHT_DAMAGE) )
				return SCHED_SMALL_FLINCH;
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return SCHED_CONSCRIPT_DRAW;

			if ( HasCondition( COND_HEAVY_DAMAGE ) )
				return SCHED_TAKE_COVER_FROM_ENEMY;

			// ---------------------
			// no ammo
			// ---------------------
			if ( HasCondition ( COND_NO_PRIMARY_AMMO ) )
			{
				Speak( CONSCRIPT_OUT_AMMO );
				return SCHED_HIDE_AND_RELOAD;
			}
			else if (!HasCondition(COND_CAN_RANGE_ATTACK1) && HasCondition( COND_NO_SECONDARY_AMMO ))
			{
				Speak( CONSCRIPT_OUT_AMMO );
				return SCHED_HIDE_AND_RELOAD;
			}

			/* UNDONE: check distance for genade attacks...
			// If player is next to what I'm trying to attack...
			if ( HasCondition( COND_WEAPON_PLAYER_NEAR_TARGET ))
			{
				return SCHED_CONSCRIPT_AIM;
			}
			*/			

			// -------------------------------------------
			// If I might hit the player shooting...
			// -------------------------------------------
			if ( HasCondition( COND_WEAPON_PLAYER_IN_SPREAD ))
			{
				if ( IsOkToCombatSpeak() && m_nextLineFireTime	< gpGlobals->curtime)
				{
					Speak( CONSCRIPT_LINE_FIRE );
					m_nextLineFireTime = gpGlobals->curtime + 3.0f;
				}

				// Run to a new location or stand and aim
				if (random->RandomInt(0,2) == 0)
				{
					return SCHED_ESTABLISH_LINE_OF_FIRE;
				}
				else
				{
					return SCHED_CONSCRIPT_AIM;
				}
			}

			// -------------------------------------------
			// If I'm in cover and I don't have a line of
			// sight to my enemy, wait randomly before attacking
			// -------------------------------------------

		}
		break;

	case NPC_STATE_ALERT:	
	case NPC_STATE_IDLE:
		if ( HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return SCHED_SMALL_FLINCH;
		}

	
		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_Conscript::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionBarnacleVictimDangle)
	{
		// Force choosing of a new schedule
		ClearSchedule();
		m_bInBarnacleMouth	= true;
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		m_IdealNPCState = NPC_STATE_IDLE;

		CPASAttenuationFilter filter( this );
		EmitSound( filter, entindex(), CHAN_VOICE, "barney/ba_close.wav", 1, SNDLVL_TALKING, 0, GetExpresser()->GetVoicePitch());

		m_bInBarnacleMouth	= false;
		SetAbsVelocity( vec3_origin );
		SetMoveType( MOVETYPE_STEP );
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimGrab )
	{
		if ( GetFlags() & FL_ONGROUND )
		{
			RemoveFlag( FL_ONGROUND );
		}
		m_IdealNPCState = NPC_STATE_PRONE;
		PainSound();
		return true;
	}
	return false;
}

void CNPC_Conscript::DeclineFollowing( void )
{
	Speak( CONSCRIPT_POK );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

//=========================================================
// > SCHED_CONSCRIPT_FOLLOW
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CONSCRIPT_FOLLOW,

	"	Tasks"
	"		TASK_GET_PATH_TO_TARGET			0"
	"		TASK_MOVE_TO_TARGET_RANGE		128"	// Move within 128 of target ent (client)
	"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_TARGET_FACE "
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_PROVOKED"
);

//=========================================================
//  > SCHED_CONSCRIPT_DRAW
//		much better looking draw schedule for when
//		conscript knows who he's gonna attack.
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CONSCRIPT_DRAW,

	"	Tasks"
	"		 TASK_STOP_MOVING					0"
	"		 TASK_FACE_ENEMY					0"
	"		 TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ARM "
	""
	"	Interrupts"
);

//===============================================
//	> SCHED_CONSCRIPT_AIM
//
//	Stand in place and aim at enemy (used when
//  line of sight blocked by player)
//===============================================
AI_DEFINE_SCHEDULE
(
	SCHED_CONSCRIPT_AIM,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_FACE_ENEMY			0"
	"		TASK_PLAY_SEQUENCE		ACTIVITY:ACT_CONSCRIPT_AIM"
	""
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_NO_PRIMARY_AMMO"
	"		COND_WEAPON_HAS_LOS"
	"		COND_CAN_MELEE_ATTACK1 "
	"		COND_CAN_MELEE_ATTACK2 "
	"		COND_HEAR_DANGER"
);

//=========================================================
// > SCHED_CONSCRIPT_FACE_TARGET
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CONSCRIPT_FACE_TARGET,

	"	Tasks"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_FACE_TARGET			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_TARGET_CHASE"
	""
	"	Interrupts"
	//"		CLIENT_PUSH			<<TODO>>
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_HEAR_DANGER"
	"		COND_PROVOKED"
);

//=========================================================
// > SCHED_CONSCRIPT_STAND
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CONSCRIPT_STAND,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE "
	"		TASK_WAIT					2"	// repick IDLESTAND every two seconds.
	"		TASK_TALKER_HEADRESET		0"	// reset head position
	""
	"	Interrupts	 "
	"		COND_NEW_ENEMY"
	"		COND_LIGHT_DAMAGE"
	"		COND_HEAVY_DAMAGE"
	"		COND_SMELL"
	"		COND_PROVOKED"
	"		COND_HEAR_COMBAT"
	"		COND_HEAR_DANGER"
);

//=========================================================
// > SCHED_CONSCRIPT_BARNACLE_HIT
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CONSCRIPT_BARNACLE_HIT,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_HIT"
	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_CONSCRIPT_BARNACLE_PULL"
	""
	"	Interrupts"
);

//=========================================================
// > SCHED_CONSCRIPT_BARNACLE_PULL
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CONSCRIPT_BARNACLE_PULL,

	"	Tasks"
	"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_PULL"
	""
	"	Interrupts"
);

//=========================================================
// > SCHED_CONSCRIPT_BARNACLE_CHOMP
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CONSCRIPT_BARNACLE_CHOMP,

	"	Tasks"
	"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHOMP"
	"		 TASK_SET_SCHEDULE			SCHEDULE:SCHED_CONSCRIPT_BARNACLE_CHEW"
	""
	"	Interrupts"
);

//=========================================================
// > SCHED_CONSCRIPT_BARNACLE_CHEW
//=========================================================
AI_DEFINE_SCHEDULE
(
	SCHED_CONSCRIPT_BARNACLE_CHEW,

	"	Tasks"
	"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHEW"
);


#endif
