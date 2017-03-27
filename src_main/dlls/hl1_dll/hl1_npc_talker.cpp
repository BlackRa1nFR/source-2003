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
#include "cbase.h"
#include "hl1_npc_talker.h"
#include "scripted.h"
#include "soundent.h"
#include "animation.h"
#include "EntityList.h"
#include "AI_Navigator.h"
#include "AI_Motor.h"
#include "player.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "NPCevent.h"
#include "ai_interactions.h"

#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "hl1_ai_basenpc.h"

//#define TALKER_LOOK 0

BEGIN_DATADESC( CHL1NPCTalker )

	DEFINE_ENTITYFUNC( CHL1NPCTalker, Touch ),
	DEFINE_FIELD( CHL1NPCTalker, m_bInBarnacleMouth,	FIELD_BOOLEAN ),

END_DATADESC()

void CHL1NPCTalker::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_HL1TALKER_FOLLOW_WALK_PATH_FOR_UNITS:
			{
				float distance;

				distance = (m_vecLastPosition - GetLocalOrigin()).Length2D();

				// Walk path until far enough away
				if ( distance > pTask->flTaskData || 
					 GetNavigator()->GetGoalType() == GOALTYPE_NONE )
				{
					TaskComplete();
					GetNavigator()->ClearGoal();		// Stop moving
				}
				break;
			}



		case TASK_TALKER_CLIENT_STARE:
		case TASK_TALKER_LOOK_AT_CLIENT:
		{
			CBasePlayer *pPlayer;
			
			pPlayer = (CBasePlayer *)CBaseEntity::Instance( engine->PEntityOfEntIndex( 1 ) );

			// track head to the client for a while.
			if ( m_NPCState == NPC_STATE_IDLE		&& 
				 !IsMoving()								&&
				 !GetExpresser()->IsSpeaking() )
			{
			
				if ( pPlayer )
				{
					IdleHeadTurn( pPlayer->GetAbsOrigin() );
				}
			}
			else
			{
				// started moving or talking
				TaskFail( "moved away" );
				return;
			}

			if ( pTask->iTask == TASK_TALKER_CLIENT_STARE )
			{
				// fail out if the player looks away or moves away.
				if ( ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).Length2D() > TLK_STARE_DIST )
				{
					// player moved away.
					TaskFail( NO_TASK_FAILURE );
				}

				Vector vForward;
				AngleVectors( GetAbsAngles(), &vForward );
				if ( UTIL_DotPoints( pPlayer->GetAbsOrigin(), GetAbsOrigin(), vForward ) < m_flFieldOfView )
				{
					// player looked away
					TaskFail( "looked away" );
				}
			}

			if ( gpGlobals->curtime > m_flWaitFinished )
			{
				TaskComplete( NO_TASK_FAILURE );
			}

			break;
		}

		case TASK_WAIT_FOR_MOVEMENT:
		{
			if ( GetExpresser()->IsSpeaking() && GetSpeechTarget() != NULL)
			{
				// ALERT(at_console, "walking, talking\n");
				IdleHeadTurn( GetSpeechTarget()->GetAbsOrigin(), GetExpresser()->GetTimeSpeechComplete() - gpGlobals->curtime );
			}
			else if ( GetEnemy() )
			{
				IdleHeadTurn( GetEnemy()->GetAbsOrigin() );
			}
			else
				IdleHeadTurn( vec3_origin );

			BaseClass::RunTask( pTask );

			if ( TaskIsComplete() )
				 IdleHeadTurn( vec3_origin );

			break;
		}

		case TASK_FACE_PLAYER:
		{
			CBasePlayer *pPlayer;
			
			pPlayer = (CBasePlayer *)CBaseEntity::Instance( engine->PEntityOfEntIndex( 1 ) );

			if ( pPlayer )
			{
				//GetMotor()->SetIdealYaw( pPlayer->GetAbsOrigin() );
				IdleHeadTurn( pPlayer->GetAbsOrigin() );
				if ( gpGlobals->curtime > m_flWaitFinished && GetMotor()->DeltaIdealYaw() < 10 )
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail( FAIL_NO_PLAYER );
			}

			break;
		}

		case TASK_TALKER_EYECONTACT:
		{
			if (!IsMoving() && GetExpresser()->IsSpeaking() && GetSpeechTarget() != NULL)
			{
				// ALERT( at_console, "waiting %f\n", m_flStopTalkTime - gpGlobals->time );
				IdleHeadTurn( GetSpeechTarget()->GetAbsOrigin(), GetExpresser()->GetTimeSpeechComplete() - gpGlobals->curtime );
			}
			
			BaseClass::RunTask( pTask );
			
			break;

		}

				
		default:
		{
		
			if ( GetExpresser()->IsSpeaking() && GetSpeechTarget() != NULL)
			{
				IdleHeadTurn( GetSpeechTarget()->GetAbsOrigin(), GetExpresser()->GetTimeSpeechComplete() - gpGlobals->curtime );
			}
			else if ( GetFollowTarget() )
			{
				IdleHeadTurn( GetFollowTarget()->GetAbsOrigin() );
			}
			else if ( GetEnemy() && m_NPCState == NPC_STATE_COMBAT )
			{
				IdleHeadTurn( GetEnemy()->GetAbsOrigin() );
			}
			else
			{
				IdleHeadTurn( vec3_origin );
			}

			BaseClass::RunTask( pTask );
			break;
		}
	}
}

bool CHL1NPCTalker::ShouldGib( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & DMG_NEVERGIB )
		 return false;

	if ( ( info.GetDamageType() & DMG_GIB_CORPSE && m_iHealth < GIB_HEALTH_VALUE ) || ( info.GetDamageType() & DMG_ALWAYSGIB ) )
		 return true;
	
	return false;
	
}

void CHL1NPCTalker::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
		case TASK_HL1TALKER_FOLLOW_WALK_PATH_FOR_UNITS:
		{
			GetNavigator()->SetMovementActivity( ACT_WALK );
			break;
		}
		default:
			BaseClass::StartTask( pTask );
			break;
	}
}

int CHL1NPCTalker::SelectSchedule ( void )
{
	switch( m_NPCState )
	{
	case NPC_STATE_PRONE:
		{
			if (m_bInBarnacleMouth)
			{
				return SCHED_HL1TALKER_BARNACLE_CHOMP;
			}
			else
			{
				return SCHED_HL1TALKER_BARNACLE_HIT;
			}
		}
	}

	return BaseClass::SelectSchedule();
}

bool CHL1NPCTalker::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
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
		SetState ( NPC_STATE_IDLE );

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
		
		if ( GetState() == NPC_STATE_SCRIPT )
		{
			 m_hCine->CancelScript();
			 ClearSchedule();
		}

		SetState( NPC_STATE_PRONE );
		PainSound();
		return true;
	}
	return false;
}

void CHL1NPCTalker::StartFollowing(	CBaseEntity *pLeader )
{
	if ( !HasSpawnFlags( SF_NPC_GAG ) )
	{
		Speak( TLK_USE );
		SetSpeechTarget( pLeader );
	}

	BaseClass::StartFollowing( pLeader );
}

Disposition_t CHL1NPCTalker::IRelationType( CBaseEntity *pTarget )
{
	if ( pTarget->IsPlayer() )
	{
		if ( HasMemory( bits_MEMORY_PROVOKED ) )
		{
			return D_HT;
		}
	}

	return BaseClass::IRelationType( pTarget );
}

void CHL1NPCTalker::Touch( CBaseEntity *pOther )
{
	if ( m_NPCState == NPC_STATE_SCRIPT )
		 return;

	BaseClass::Touch(pOther);
}

void CHL1NPCTalker::StopFollowing( void )
{
	if ( !(m_afMemory & bits_MEMORY_PROVOKED) )
	{
		if ( !HasSpawnFlags( SF_NPC_GAG ) )
		{
			Speak( TLK_UNUSE );
			SetSpeechTarget( GetFollowTarget() );
		}
	}

	BaseClass::StopFollowing();
}

void CHL1NPCTalker::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( info.GetDamage() >= 1.0 && !(info.GetDamageType() & DMG_SHOCK ) )
	{
		UTIL_BloodSpray( ptr->endpos, vecDir, BloodColor(), 4, FX_BLOODSPRAY_ALL );	
	}

	BaseClass::TraceAttack( info, vecDir, ptr );
}

void CHL1NPCTalker::FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Don't allow use during a scripted_sentence
	if ( GetUseTime() > gpGlobals->curtime )
		return;

	if ( m_hCine && !m_hCine->CanInterrupt() )
		 return;

	if ( pCaller != NULL && pCaller->IsPlayer() )
	{
		// Pre-disaster followers can't be used
		if ( m_spawnflags & SF_NPC_PREDISASTER )
		{
			SetSpeechTarget( pCaller );
			DeclineFollowing();
			return;
		}
	}

	BaseClass::FollowerUse( pActivator, pCaller, useType, value );
}

int CHL1NPCTalker::TranslateSchedule( int scheduleType )
{
	return BaseClass::TranslateSchedule( scheduleType );
}

float CHL1NPCTalker::PickRandomLookTarget( bool bExcludePlayers, float minTime, float maxTime )
{
	return random->RandomFloat( 5.0f, 10.0f );
}

void CHL1NPCTalker::IdleHeadTurn( const Vector &vTargetPos, float flDuration, float flImportance )
{
	if (!(CapabilitiesGet() & bits_CAP_TURN_HEAD))
		return;

	if ( flDuration == 0.0f )
		 flDuration = random->RandomFloat( 2.0, 4.0 );

	if ( vTargetPos == vec3_origin || m_NPCState == NPC_STATE_SCRIPT )
	{
		SetHeadDirection( vTargetPos, GetAnimTimeInterval() );
	}
	else
	{
		 AddLookTarget( vTargetPos, 1.0, flDuration );
	}
}

void CHL1NPCTalker::SetHeadDirection( const Vector &vTargetPos, float flInterval)
{

#ifdef TALKER_LOOK
	// Draw line in body, head, and eye directions
	Vector vEyePos = EyePosition();
	Vector vHeadDir = HeadDirection3D();
	Vector vBodyDir = BodyDirection2D();

	//UNDONE <<TODO>>
	// currently eye dir just returns head dir, so use vTargetPos for now
	//Vector vEyeDir;	w
	//EyeDirection3D(&vEyeDir);
	NDebugOverlay::Line( vEyePos, vEyePos+(50*vHeadDir), 255, 0, 0, false, 0.1 );
	NDebugOverlay::Line( vEyePos, vEyePos+(50*vBodyDir), 0, 255, 0, false, 0.1 );
	NDebugOverlay::Line( vEyePos, vTargetPos, 0, 0, 255, false, 0.1 );
#endif

	if ( m_NPCState != NPC_STATE_SCRIPT )
		 BaseClass::SetHeadDirection( vTargetPos, flInterval );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL1NPCTalker::CorpseGib( const CTakeDamageInfo &info )
{
	CEffectData	data;
	
	data.m_vOrigin = WorldSpaceCenter();
	data.m_vNormal = data.m_vOrigin - info.GetDamagePosition();
	VectorNormalize( data.m_vNormal );
	
	data.m_flScale = RemapVal( m_iHealth, 0, -500, 1, 3 );
	data.m_flScale = clamp( data.m_flScale, 1, 3 );

    data.m_nMaterial = 1;
	data.m_nHitBox = -m_iHealth;

	data.m_nColor = BloodColor();
	
	DispatchEffect( "HL1Gib", data );

	CSoundEnt::InsertSound( SOUND_MEAT, GetAbsOrigin(), 256, 0.5f, this );

	return true;
}

AI_BEGIN_CUSTOM_NPC( monster_hl1talker, CHL1NPCTalker )

	DECLARE_TASK( TASK_HL1TALKER_FOLLOW_WALK_PATH_FOR_UNITS )

	//=========================================================
	// > SCHED_HL1TALKER_MOVE_AWAY_FOLLOW
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_HL1TALKER_FOLLOW_MOVE_AWAY,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_TARGET_FACE"
		"		 TASK_STORE_LASTPOSITION				0"
		"		 TASK_MOVE_AWAY_PATH					100"
		"		 TASK_HL1TALKER_FOLLOW_WALK_PATH_FOR_UNITS	100"
		"		 TASK_STOP_MOVING						0"
		"		 TASK_FACE_PLAYER						0"
		"		 TASK_SET_ACTIVITY						ACT_IDLE"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_HL1TALKER_IDLE_SPEAK_WAIT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_HL1TALKER_IDLE_SPEAK_WAIT,

		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"	// Stop and talk
		"		TASK_FACE_PLAYER			0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// > SCHED_HL1TALKER_BARNACLE_HIT
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HL1TALKER_BARNACLE_HIT,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_HIT"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_HL1TALKER_BARNACLE_PULL"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_HL1TALKER_BARNACLE_PULL
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HL1TALKER_BARNACLE_PULL,

		"	Tasks"
		"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_PULL"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_HL1TALKER_BARNACLE_CHOMP
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HL1TALKER_BARNACLE_CHOMP,

		"	Tasks"
		"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHOMP"
		"		 TASK_SET_SCHEDULE			SCHEDULE:SCHED_HL1TALKER_BARNACLE_CHEW"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_HL1TALKER_BARNACLE_CHEW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_HL1TALKER_BARNACLE_CHEW,

		"	Tasks"
		"		 TASK_PLAY_SEQUENCE			ACTIVITY:ACT_BARNACLE_CHEW"
	)

AI_END_CUSTOM_NPC()
