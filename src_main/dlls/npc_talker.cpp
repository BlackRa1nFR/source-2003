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
#include "npc_talker.h"
#include "scripted.h"
#include "soundent.h"
#include "animation.h"
#include "entitylist.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_hint.h"
#include "player.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "npcevent.h"

// NOTE: m_voicePitch & m_szGrp should be fixed up by precache each save/restore

BEGIN_DATADESC( CNPCSimpleTalker )
	DEFINE_FIELD( CNPCSimpleTalker, m_useTime, FIELD_TIME ),
	// 							m_FollowBehavior (auto saved by AI)
END_DATADESC()

BEGIN_DATADESC( CAI_PlayerAlly )

	// @TODO (toml 09-09-02): DEFINE_FIELD( CAI_PlayerAlly, m_bitsSaid, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_PlayerAlly, m_hTalkTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CAI_PlayerAlly, m_nSpeak, FIELD_INTEGER ),
	DEFINE_FIELD( CAI_PlayerAlly, m_flLastHealthKitSearch, FIELD_FLOAT ),

	// Recalc'ed in Precache()
	//	DEFINE_FIELD( CAI_PlayerAlly, m_voicePitch, FIELD_INTEGER ),
	//	DEFINE_FIELD( CAI_PlayerAlly, m_szGrp, FIELD_??? ),
	DEFINE_FIELD( CAI_PlayerAlly, m_iszUse, FIELD_STRING ),
	DEFINE_FIELD( CAI_PlayerAlly, m_iszUnUse, FIELD_STRING ),
	//DEFINE_FIELD( CAI_PlayerAlly, m_flLastSaidSmelled, FIELD_TIME ),
	// @TODO (toml 09-09-02): DEFINE_FIELD( CAI_PlayerAlly, m_flStopTalkTime, FIELD_TIME ),
	// @TODO (toml 09-09-02): DEFINE_FIELD( CAI_PlayerAlly, m_hTalkTarget, FIELD_EHANDLE ),
	// @TODO (toml 09-09-02): DEFINE_FIELD( CAI_PlayerAlly, m_hMonologTalkTarget, FIELD_EHANDLE ),
	// @TODO (toml 09-09-02): DEFINE_FIELD( CAI_PlayerAlly, m_iMonologIndex, FIELD_INTEGER ),
	// @TODO (toml 09-09-02): DEFINE_FIELD( CAI_PlayerAlly, m_fMonologSuspended, FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_USEFUNC( CNPCSimpleTalker, FollowerUse ),

END_DATADESC()

//-----------------------------------------------------------------------------
bool CAI_PlayerAlly::IsPlayerAlly() 
{
#if 0
	// This needs to be thought about more! 
	// right now it messes up the ally count in the template spawner.
	if( HasMemory( bits_MEMORY_PROVOKED ) )
	{
		// I'm no ally to the player if I saw him kill another ally.
		return false;
	}
#endif 

	return BaseClass::IsPlayerAlly();
}


//-----------------------------------------------------------------------------
void CAI_PlayerAlly::PlayerSelect( bool select )
{
	if( select && HasMemory( bits_MEMORY_PROVOKED ) )
	{
		Speak( TLK_BETRAYED );
		return;
	}

	BaseClass::PlayerSelect( select );
}


// array of friend names
char *CAI_PlayerAlly::m_szFriends[TLK_CFRIENDS] = 
{
	"NPC_barney",
	"NPC_scientist",
	"NPC_sitting_scientist",
};

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------

Activity CAI_PlayerAlly::NPC_TranslateActivity( Activity eNewActivity )
{
	if ((eNewActivity == ACT_IDLE)										&& 
		(GetExpresser()->IsSpeaking())										&&
		(SelectWeightedSequence ( ACT_SIGNAL3 ) != ACTIVITY_NOT_AVAILABLE)	)
	{
		return ACT_SIGNAL3;
	}
	else if ((eNewActivity == ACT_SIGNAL3)									&& 
			 (SelectWeightedSequence ( ACT_SIGNAL3 ) == ACTIVITY_NOT_AVAILABLE)	)
	{
		return ACT_IDLE;
	}
	return BaseClass::NPC_TranslateActivity( eNewActivity );
}


void CAI_PlayerAlly::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_TALKER_SPEAK:
		// ask question or make statement
		FIdleSpeak();
		TaskComplete();
		break;

	case TASK_TALKER_RESPOND:
		// respond to question
		IdleRespond();
		TaskComplete();
		break;

	case TASK_TALKER_HELLO:
		// greet player
		FIdleHello();
		TaskComplete();
		break;
	

	case TASK_TALKER_STARE:
		// let the player know I know he's staring at me.
		FIdleStare();
		TaskComplete();
		break;

	case TASK_TALKER_LOOK_AT_CLIENT:
	case TASK_TALKER_CLIENT_STARE:
		// track head to the client for a while.
		m_flWaitFinished = gpGlobals->curtime + pTask->flTaskData;
		break;

	case TASK_TALKER_EYECONTACT:
		break;

	case TASK_TALKER_IDEALYAW:
		if (GetSpeechTarget() != NULL)
		{
			GetMotor()->SetYawSpeed( 60 );

			GetMotor()->SetIdealYawToTarget( GetSpeechTarget()->GetLocalOrigin() );
/*
			float yaw = VecToYaw(GetSpeechTarget()->GetLocalOrigin() - GetLocalOrigin()) - GetLocalAngles().y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;

			if (yaw < 0)
			{
				m_IdealYaw = min( yaw + 45, 0 ) + GetLocalAngles().y;
			}
			else
			{
				m_IdealYaw = max( yaw - 45, 0 ) + GetLocalAngles().y;
			}
*/
		}
		TaskComplete();
		break;

	case TASK_TALKER_HEADRESET:
		// reset head position after looking at something
		SetSpeechTarget( NULL );
		TaskComplete();
		break;

	case TASK_TALKER_BETRAYED:
		Speak( TLK_BETRAYED );
		TaskComplete();
		break;

	case TASK_TALKER_STOPSHOOTING:
		// tell player to stop shooting
		Speak( TLK_NOSHOOT );
		TaskComplete();
		break;

	case TASK_PLAY_SCRIPT:
		SetSpeechTarget( NULL );
		BaseClass::StartTask( pTask );
		break;

	case TASK_FIND_LOCK_HINTNODE_HEALTH:
		{
			m_flLastHealthKitSearch = gpGlobals->curtime + 5.0;
			if (!m_pHintNode)
			{
				m_pHintNode = CAI_Hint::FindHint( this, HINT_HEALTH_KIT, pTask->flTaskData, 1024 );
			}
			if (m_pHintNode && m_pHintNode->Lock( this ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail(FAIL_NO_HINT_NODE);
			}
		}
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}


void CAI_PlayerAlly::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_TALKER_CLIENT_STARE:
	case TASK_TALKER_LOOK_AT_CLIENT:

		if ( pTask->iTask == TASK_TALKER_CLIENT_STARE )
		{
			// Get edict for one player
			CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( engine->PEntityOfEntIndex( 1 ) );
			Assert( pPlayer );

			// fail out if the player looks away or moves away.
			if ( ( pPlayer->GetLocalOrigin() - GetLocalOrigin() ).Length2D() > TLK_STARE_DIST )
			{
				// player moved away.
				TaskFail("Player moved away");
			}

			Vector forward;
			AngleVectors( pPlayer->GetLocalAngles(), &forward );
			if ( UTIL_DotPoints( pPlayer->GetLocalOrigin(), GetLocalOrigin(), forward ) < m_flFieldOfView )
			{
				// player looked away
				TaskFail("Player looked away");
			}
		}

		if ( gpGlobals->curtime > m_flWaitFinished )
		{
			TaskComplete();
		}
		break;

	case TASK_TALKER_EYECONTACT:
		if (IsMoving() || !GetExpresser()->IsSpeaking() || GetSpeechTarget() == NULL)
		{
			TaskComplete();
		}
		break;

	case TASK_WAIT_FOR_MOVEMENT:
		if (!GetExpresser()->IsSpeaking() || GetSpeechTarget() == NULL)
		{
			// override so that during walk, a scientist may talk and greet player
			FIdleHello();
			if (random->RandomInt(0,m_nSpeak * 20) == 0)
			{
				FIdleSpeak();
			}
		}

		BaseClass::RunTask( pTask );
		break;

	default:
		BaseClass::RunTask( pTask );
	}
}

void CNPCSimpleTalker::Event_Killed( const CTakeDamageInfo &info )
{
	if ( info.GetAttacker()->GetFlags() & FL_CLIENT )
	{
		LimitFollowers( info.GetAttacker(), 0 );
	}
	BaseClass::Event_Killed( info );
}

void CAI_PlayerAlly::Event_Killed( const CTakeDamageInfo &info )
{
	AlertFriends( info.GetAttacker() );

	SetTarget( NULL );
	// Don't finish that sentence
	StopTalking();
	SetUse( NULL );
	BaseClass::Event_Killed( info );
}



CBaseEntity	*CAI_PlayerAlly::EnumFriends( CBaseEntity *pPrevious, int listNumber, bool bTrace )
{
	CBaseEntity *pFriend = pPrevious;
	char *pszFriend;
	trace_t tr;
	Vector vecCheck;

	pszFriend = m_szFriends[ FriendNumber(listNumber) ];
	while (pFriend = gEntList.FindEntityByClassname( pFriend, pszFriend ))
	{
		if (pFriend == this || !pFriend->IsAlive())
			// don't talk to self or dead people
			continue;
		if ( bTrace )
		{
			vecCheck = pFriend->GetAbsOrigin();
			vecCheck.z = pFriend->GetAbsMaxs().z;

			UTIL_TraceLine( GetAbsOrigin(), vecCheck, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);
		}
		else
			tr.fraction = 1.0;

		if (tr.fraction == 1.0)
		{
			return pFriend;
		}
	}

	return NULL;
}


void CAI_PlayerAlly::AlertFriends( CBaseEntity *pKiller )
{
	CBaseEntity *pFriend = NULL;
	int i;

	// for each friend in this bsp...
	for ( i = 0; i < TLK_CFRIENDS; i++ )
	{
		while (pFriend = EnumFriends( pFriend, i, true ))
		{
			CAI_BaseNPC *pNPC = pFriend->MyNPCPointer();
			if ( pNPC->IsAlive() )
			{
				// If a client killed me, make everyone else mad/afraid of him
				if ( pKiller->GetFlags() & FL_CLIENT )
				{
					pNPC->SetSchedule( SCHED_TALKER_BETRAYED );
#ifdef ALLIES_CAN_BE_PROVOKED
					pNPC->Remember( bits_MEMORY_PROVOKED );

					if( IsSelected() )
					{
						PlayerSelect( false );
					}
#endif
				}
				else
				{
					if( IRelationType(pKiller) == D_HT)
					{
						// Killed by an enemy!!!
						CAI_PlayerAlly *pAlly = (CAI_PlayerAlly *)pNPC;
						
						if( pAlly && pAlly->GetExpresser()->CanSpeakConcept( TLK_ALLY_KILLED ) )
						{
							pAlly->Speak( TLK_ALLY_KILLED );
						}
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Implemented to look at talk target
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *CAI_PlayerAlly::EyeLookTarget( void )
{
	if (GetExpresser()->IsSpeaking() && GetSpeechTarget() != NULL)
	{
		return GetSpeechTarget();
	}
	return NULL;
}

void CAI_PlayerAlly::ShutUpFriends( void )
{
	CBaseEntity *pFriend = NULL;
	int i;

	// for each friend in this bsp...
	for ( i = 0; i < TLK_CFRIENDS; i++ )
	{
		while (pFriend = EnumFriends( pFriend, i, true ))
		{
			CAI_BaseNPC *pNPC = pFriend->MyNPCPointer();
			if ( pNPC )
			{
				pNPC->SentenceStop();
			}
		}
	}
}


// UNDONE: Keep a follow time in each follower, make a list of followers in this function and do LRU
// UNDONE: Check this in Restore to keep restored NPCs from joining a full list of followers
void CNPCSimpleTalker::LimitFollowers( CBaseEntity *pPlayer, int maxFollowers )
{
	CBaseEntity *pFriend = NULL;
	int i, count;

	count = 0;
	// for each friend in this bsp...
	for ( i = 0; i < TLK_CFRIENDS; i++ )
	{
		while (pFriend = EnumFriends( pFriend, i, false ))
		{
			CAI_BaseNPC *pNPC = pFriend->MyNPCPointer();
			CNPCSimpleTalker *pTalker;
			if ( pNPC )
			{
				if ( pNPC->GetTarget() == pPlayer )
				{
					count++;
					if ( count > maxFollowers && (pTalker = dynamic_cast<CNPCSimpleTalker *>( pNPC ) ) != NULL )
						pTalker->StopFollowing();
				}
			}
		}
	}
}


/*
float CAI_PlayerAlly::TargetDistance( void )
{
	// If we lose the player, or he dies, return a really large distance
	if ( GetTarget() == NULL || !GetTarget()->IsAlive() )
		return 1e6;

	return (GetTarget()->GetLocalOrigin() - GetLocalOrigin()).Length();
}
*/


//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CAI_PlayerAlly::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{		
	case SCRIPT_EVENT_SENTENCE_RND1:		// Play a named sentence group 25% of the time
		if (random->RandomInt(0,99) < 75)
			break;
		// fall through...
	case SCRIPT_EVENT_SENTENCE:				// Play a named sentence group
		ShutUpFriends();
		PlaySentence( pEvent->options, random->RandomFloat(2.8, 3.4) );
		//Msg( "script event speak\n");
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

// NPCs derived from CNPCSimpleTalker should call this in precache()

void CAI_PlayerAlly::TalkInit( void )
{
	// every new talking NPC must reset this global, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	
	// @Q (toml 09-12-02): what happens if an NPC is created later in the level? This seems suspicious
	g_AITalkSemaphore.Release();
}	

//=========================================================
// FindNearestFriend
// Scan for nearest, visible friend. If fPlayer is true, look for
// nearest player
//=========================================================
CBaseEntity *CAI_PlayerAlly::FindNearestFriend(bool fPlayer)
{
	CBaseEntity *pFriend = NULL;
	CBaseEntity *pNearest = NULL;
	float range = 10000000.0;
	trace_t tr;
	Vector vecStart = GetAbsOrigin();
	Vector vecCheck;
	int i;
	const char *pszFriend;
	int cfriends;

	vecStart.z = GetAbsMaxs().z;
	
	if (fPlayer)
		cfriends = 1;
	else
		cfriends = TLK_CFRIENDS;

	// for each type of friend...

	for (i = cfriends-1; i > -1; i--)
	{
		if (fPlayer)
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex(1);
			if ( pPlayer )
			{
				pszFriend = STRING(pPlayer->m_iClassname);
			}
			else
			{
				pszFriend = "player";
			}
		}
		else
			pszFriend = m_szFriends[FriendNumber(i)];

		if (!pszFriend)
			continue;

		// for each friend in this bsp...
		while (pFriend = gEntList.FindEntityByClassname( pFriend, pszFriend ))
		{
			if (pFriend == this || !pFriend->IsAlive())
				// don't talk to self or dead people
				continue;

			CAI_BaseNPC *pNPC = pFriend->MyNPCPointer();

			// If not a NPC for some reason, or in a script.
			if ( pNPC && (pNPC->m_NPCState == NPC_STATE_SCRIPT || pNPC->m_NPCState == NPC_STATE_PRONE))
				continue;

			if( pNPC && pNPC->IsSelected() )
			{
				// Don't bother people that are awaiting orders.
				return false;
			}

			vecCheck = pFriend->GetLocalOrigin();
			vecCheck.z = pFriend->GetAbsMaxs().z;

			// if closer than previous friend, and in range, see if he's visible

			if (range > (vecStart - vecCheck).Length())
			{
				UTIL_TraceLine(vecStart, vecCheck, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);

				if (tr.fraction == 1.0)
				{
					// visible and in range, this is the new nearest scientist
					if ((vecStart - vecCheck).Length() < TALKRANGE_MIN)
					{
						pNearest = pFriend;
						range = (vecStart - vecCheck).Length();
					}
				}
			}
		}
	}
	return pNearest;
}

void CAI_PlayerAlly::Touch( CBaseEntity *pOther )
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



//=========================================================
// IdleRespond
// Respond to a previous question
//=========================================================
void CAI_PlayerAlly::IdleRespond( void )
{
	//int pitch = GetVoicePitch();
	
	// play response
	Speak( TLK_ANSWER );
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_PlayerAlly::IsOkToSpeak( void )
{
	if ( !IsOkToCombatSpeak() )
		return false;
		
	// don't talk if you're in combat
	if (GetEnemy() != NULL && FVisible( GetEnemy() ))
		return false;

	return ( !GetExpresser()->IsSpeaking() );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_PlayerAlly::IsOkToCombatSpeak( void )
{
	// if not alive, certainly don't speak
	if ( !IsAlive() )
		return false;

	// if someone else is talking, don't speak
	if ( !GetExpresser()->SemaphoreIsAvailable() )
		return false;

	if ( m_spawnflags & SF_NPC_GAG )
		return false;

	// Don't speak if playing a script.
	if ( m_NPCState == NPC_STATE_SCRIPT )
		return false;

	// if player is not in pvs, don't speak
	if ( !UTIL_FindClientInPVS(edict()) )
		return false;

	return true;
}

bool CAI_PlayerAlly::CanPlaySentence( bool fDisregardState ) 
{ 
	if ( fDisregardState )
		return BaseClass::CanPlaySentence( fDisregardState );
	return IsOkToSpeak(); 
}

//=========================================================
// FIdleStare
//=========================================================
int CAI_PlayerAlly::FIdleStare( void )
{
	if (!IsOkToSpeak())
		return false;

	Speak( TLK_STARE );

	SetSpeechTarget( FindNearestFriend( true ) );
	return true;
}

//=========================================================
// IdleHello
// Try to greet player first time he's seen
//=========================================================
int CAI_PlayerAlly::FIdleHello( void )
{
	if (!IsOkToSpeak())
		return false;

	// if this is first time scientist has seen player, greet him
	if (!GetExpresser()->SpokeConcept(TLK_HELLO))
	{
		// get a player
		CBaseEntity *pPlayer = FindNearestFriend(true);

		if (pPlayer)
		{
			if (FInViewCone(pPlayer) && FVisible(pPlayer))
			{
				SetSpeechTarget( pPlayer );

				Speak( TLK_HELLO );
				
				return true;
			}
		}
	}
	return false;
}


//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CAI_PlayerAlly::FIdleSpeak ( void )
{ 
	// try to start a conversation, or make statement
	int pitch;

	if (m_NPCState == NPC_STATE_COMBAT )
	{
		// Don't Idle Speak in combat.
		return false;
	}

	if (!IsOkToSpeak())
		return false;

	Assert( GetExpresser()->SemaphoreIsAvailable() );
	
	pitch = GetExpresser()->GetVoicePitch();
		
	// player using this entity is alive and wounded?
	CBaseEntity *pTarget = GetTarget();

	if ( pTarget != NULL )
	{
		if ( pTarget->IsPlayer() )
		{
			if ( pTarget->IsAlive() )
			{
				SetSpeechTarget( GetTarget() );
				if (GetExpresser()->CanSpeakConcept( TLK_PLHURT3) && 
					(GetTarget()->m_iHealth <= GetTarget()->m_iMaxHealth / 8))
				{
					//UTIL_EmitSoundDyn(edict(), CHAN_VOICE, m_szGrp[TLK_PLHURT3], 1.0, SNDLVL_TALKING, 0, pitch);
					Speak( TLK_PLHURT3 );
					return true;
				}
				else if (GetExpresser()->CanSpeakConcept( TLK_PLHURT2) && 
					(GetTarget()->m_iHealth <= GetTarget()->m_iMaxHealth / 4))
				{
					//UTIL_EmitSoundDyn(edict(), CHAN_VOICE, m_szGrp[TLK_PLHURT2], 1.0, SNDLVL_TALKING, 0, pitch);
					Speak( TLK_PLHURT2 );
					return true;
				}
				else if (GetExpresser()->CanSpeakConcept( TLK_PLHURT1) &&
					(GetTarget()->m_iHealth <= GetTarget()->m_iMaxHealth / 2))
				{
					//UTIL_EmitSoundDyn(edict(), CHAN_VOICE, m_szGrp[TLK_PLHURT1], 1.0, SNDLVL_TALKING, 0, pitch);
					Speak( TLK_PLHURT1 );
					return true;
				}
			}
			else
			{
				//!!!KELLY - here's a cool spot to have the talkNPC talk about the dead player if we want.
				// "Oh dear, Gordon Freeman is dead!" -Scientist
				// "Damn, I can't do this without you." -Barney
			}
		}
	}

	// if there is a friend nearby to speak to, play sentence, set friend's response time, return
	CBaseEntity *pFriend = FindNearestFriend(false);

	if (pFriend && !(pFriend->IsMoving()) && (random->RandomInt(0,99) < 60))
	{
		Speak( TLK_QUESTION );

		// force friend to answer
		CAI_PlayerAlly *pTalkNPC = dynamic_cast<CNPCSimpleTalker *>(pFriend);
		if (pTalkNPC)
		{
			SetSpeechTarget( pFriend );
			pTalkNPC->SetAnswerQuestion( this ); // UNDONE: This is EVIL!!!
			pTalkNPC->GetExpresser()->BlockSpeechUntil( GetExpresser()->GetTimeSpeechComplete() );

			m_nSpeak++;
			return true;
		}
	}

	// otherwise, play an idle statement, try to face client when making a statement.
	if ( random->RandomInt(0,1) )
	{
		//SENTENCEG_PlayRndSz( edict(), szIdleGroup, 1.0, SNDLVL_TALKING, 0, pitch );
		CBaseEntity *pFriend = FindNearestFriend(true);

		if ( pFriend )
		{
			SetSpeechTarget( pFriend );
			Speak( TLK_IDLE );
			m_nSpeak++;
			return true;
		}
	}

	// didn't speak
	GetExpresser()->BlockSpeechUntil( gpGlobals->curtime + 3 );
	return false;
}

int CNPCSimpleTalker::PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener )
{
	int sentenceIndex = BaseClass::PlayScriptedSentence( pszSentence, delay, volume, soundlevel, bConcurrent, pListener );
	delay += engine->SentenceLength( sentenceIndex );
	if ( delay < 0 )
		delay = 0;
	m_useTime = gpGlobals->curtime + delay;
	return sentenceIndex;
}

int CAI_PlayerAlly::PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener )
{
	if ( !bConcurrent )
		ShutUpFriends();

	ClearCondition( COND_PLAYER_PUSHING );	// Forget about moving!  I've got something to say!
	int sentenceIndex = BaseClass::PlayScriptedSentence( pszSentence, delay, volume, soundlevel, bConcurrent, pListener );
	SetSpeechTarget( pListener );
	
	return sentenceIndex;
}

void CAI_PlayerAlly::OnStartSpeaking()
{
	// If you say anything, don't greet the player - you may have already spoken to them
	GetExpresser()->SetSpokeConcept( TLK_HELLO, NULL );
}



//=========================================================
// Talk - set a timer that tells us when the NPC is done
// talking.
//=========================================================

// Prepare this talking NPC to answer question
void CAI_PlayerAlly::SetAnswerQuestion( CAI_PlayerAlly *pSpeaker )
{
	if ( !m_hCine )
		SetSchedule( SCHED_TALKER_IDLE_RESPONSE );
	SetSpeechTarget( (CAI_BaseNPC *)pSpeaker );
}

int CAI_PlayerAlly::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// if player damaged this entity, have other friends talk about it.
	if (info.GetAttacker() && (info.GetAttacker()->GetFlags() & FL_CLIENT) && info.GetDamage() < GetHealth() )
	{
		CBaseEntity *pFriend = FindNearestFriend(false);

		if (pFriend && pFriend->IsAlive())
		{
			// only if not dead or dying!
			CNPCSimpleTalker *pTalkNPC = (CNPCSimpleTalker *)pFriend;

			pTalkNPC->SetSchedule( SCHED_TALKER_IDLE_STOP_SHOOTING );
		}
	}
	return BaseClass::OnTakeDamage_Alive( info );
}

int CAI_PlayerAlly::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_TARGET_FACE:
		// speak during 'use'
		if (random->RandomInt(0,99) < 2)
			//Msg("target chase speak\n" );
			return SCHED_TALKER_IDLE_SPEAK_WAIT;

		// If we're hurt, look for a healthkit
		if ( m_flLastHealthKitSearch < gpGlobals->curtime && GetHealth() <= GetMaxHealth() / 2 )
			return SCHED_TALKER_LOOK_FOR_HEALTHKIT;

		break;
	
	case SCHED_ALERT_STAND:
		// If we're hurt, look for a healthkit
		if ( m_flLastHealthKitSearch < gpGlobals->curtime && GetHealth() <= GetMaxHealth() / 2 )
			return SCHED_TALKER_LOOK_FOR_HEALTHKIT;

		break;
		
	case SCHED_IDLE_STAND:
		{	
			// if never seen player, try to greet him
			if (GetExpresser()->CanSpeakConcept( TLK_HELLO) && !GetExpresser()->SpokeConcept(TLK_HELLO))
			{
				return SCHED_TALKER_IDLE_HELLO;
			}

			// If we're hurt, look for a healthkit
			if ( m_flLastHealthKitSearch < gpGlobals->curtime && GetHealth() <= GetMaxHealth() / 2 )
				return SCHED_TALKER_LOOK_FOR_HEALTHKIT;

			// sustained light wounds?
			if (GetExpresser()->CanSpeakConcept( TLK_WOUND) && !GetExpresser()->SpokeConcept(TLK_WOUND) && (m_iHealth <= (m_iMaxHealth * 0.75)))
			{
				Speak( TLK_WOUND );
				return SCHED_IDLE_STAND;
			}
			// sustained heavy wounds?
			else if (GetExpresser()->CanSpeakConcept( TLK_MORTAL) && (m_iHealth <= (m_iMaxHealth * 0.5)))
			{
				Speak( TLK_MORTAL );
				return SCHED_IDLE_STAND;
			}

			// talk about world
			if (IsOkToSpeak() && random->RandomInt(0,m_nSpeak * 2) == 0)
			{
				//Msg("standing idle speak\n" );
				return SCHED_TALKER_IDLE_SPEAK;
			}
			
			if ( !GetExpresser()->IsSpeaking() && HasCondition ( COND_SEE_PLAYER ) && random->RandomInt( 0, 6 ) == 0 )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( engine->PEntityOfEntIndex( 1 ) );
				Assert( pPlayer );

				if ( pPlayer )
				{
					// watch the client.
					Vector forward;
					AngleVectors( pPlayer->GetLocalAngles(), &forward );
					if ( ( pPlayer->GetLocalOrigin() - GetLocalOrigin() ).Length2D() < TLK_STARE_DIST	&& 
						 UTIL_DotPoints( pPlayer->GetLocalOrigin(), GetLocalOrigin(), forward ) >= m_flFieldOfView )
					{
						// go into the special STARE schedule if the player is close, and looking at me too.
						return SCHED_TALKER_IDLE_WATCH_CLIENT_STARE;
					}

					return SCHED_TALKER_IDLE_WATCH_CLIENT;
				}
			}
			else
			{
				if (GetExpresser()->IsSpeaking())
					// look at who we're talking to
					return SCHED_TALKER_IDLE_EYE_CONTACT;
				else
					// regular standing idle
					return SCHED_IDLE_STAND;
			}

		}
		break;
	case SCHED_CHASE_ENEMY_FAILED:
		{
			int baseType = BaseClass::TranslateSchedule(scheduleType);
			if ( baseType != SCHED_CHASE_ENEMY_FAILED )
				return baseType;

			return SCHED_TAKE_COVER_FROM_ENEMY;
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//=========================================================
// If there's a player around, watch him.
//=========================================================
void CAI_PlayerAlly::GatherConditions( void )
{
	BaseClass::GatherConditions();
	
	if ( !HasCondition ( COND_SEE_PLAYER ) )
	{
		SetCondition ( COND_TALKER_CLIENTUNSEEN );
	}
}

void CAI_PlayerAlly::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();
	
	GetExpresser()->SpeakMonolog();
}

// try to smell something
void CAI_PlayerAlly::TrySmellTalk( void )
{
	if ( !IsOkToSpeak() )
		return;

	if ( HasCondition( COND_SMELL ) && GetExpresser()->CanSpeakConcept( TLK_SMELL ) )
		Speak( TLK_SMELL );
}

//---------------------------------------------------------
void CAI_PlayerAlly::OnPlayerSelect()
{
	if ( IsOkToCombatSpeak() )
		Speak( TLK_SELECTED );

	BaseClass::OnPlayerSelect();
}

//---------------------------------------------------------
void CAI_PlayerAlly::OnMoveOrder()
{
	if ( IsOkToCombatSpeak() )
		Speak( TLK_COMMANDED );

	BaseClass::OnMoveOrder();
}
//---------------------------------------------------------
void CAI_PlayerAlly::OnTargetOrder()
{
	if ( IsOkToCombatSpeak() )
		Speak( TLK_COMMANDED );

	BaseClass::OnTargetOrder();
}


Disposition_t CAI_PlayerAlly::IRelationType( CBaseEntity *pTarget )
{
#if 0 
	if ( pTarget->IsPlayer() )
		if ( m_afMemory & bits_MEMORY_PROVOKED )
			return D_NU;
#endif 
	return BaseClass::IRelationType( pTarget );
}


void CAI_PlayerAlly::OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior )
{
	BaseClass::OnChangeRunningBehavior( pOldBehavior,  pNewBehavior );
	CAI_FollowBehavior *pFollowBehavior;
	if ( dynamic_cast<CAI_FollowBehavior *>(pNewBehavior) != NULL )
	{
		GetExpresser()->SetSpokeConcept( TLK_HELLO, NULL );	// Don't say hi after you've started following
		if ( IsOkToCombatSpeak() )
			Speak( TLK_USE );
		SetSpeechTarget( GetTarget() );
		ClearCondition( COND_PLAYER_PUSHING );
	}
	else if ( ( pFollowBehavior = dynamic_cast<CAI_FollowBehavior *>(pOldBehavior) ) != NULL  )
	{
		if ( !(m_afMemory & bits_MEMORY_PROVOKED) )
		{
			if ( IsOkToCombatSpeak() )
			{
				if ( pFollowBehavior->GetFollowTarget() == NULL )
					Speak( TLK_UNUSE );
				else
					Speak( TLK_STOP );
			}
			SetSpeechTarget( FindNearestFriend(true) );
		}
	}
}


bool CAI_PlayerAlly::OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule )
{
	bool interrupt = BaseClass::OnBehaviorChangeStatus( pBehavior, fCanFinishSchedule );
	if ( !interrupt )
	{
		interrupt = ( dynamic_cast<CAI_FollowBehavior *>(pBehavior) != NULL && ( m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT ) );
	}
	return interrupt;

}

void CNPCSimpleTalker::FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Don't allow use during a scripted_sentence
	if ( m_useTime > gpGlobals->curtime )
		return;

	if ( pCaller != NULL && pCaller->IsPlayer() )
	{
		if ( !m_FollowBehavior.GetFollowTarget() && IsInterruptable() )
		{
#if TOML_TODO
			LimitFollowers( pCaller , 1 );
#endif

			if ( m_afMemory & bits_MEMORY_PROVOKED )
				Msg( "I'm not following you, you evil person!\n" );
			else
			{
				StartFollowing( pCaller );
			}
		}
		else
		{
			StopFollowing();
		}
	}
}

bool CAI_PlayerAlly::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "UseSentence"))
	{
		m_iszUse = AllocPooledString(szValue);
	}
	else if (FStrEq(szKeyName, "UnUseSentence"))
	{
		m_iszUnUse = AllocPooledString(szValue);
	}
	else 
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

void CAI_PlayerAlly::Precache( void )
{
	/*
	// FIXME:  Need to figure out how to hook these...
	if ( m_iszUse != NULL_STRING )
		GetExpresser()->ModifyConcept( TLK_USE, STRING( m_iszUse ) );
	if ( m_iszUnUse != NULL_STRING )
		GetExpresser()->ModifyConcept( TLK_UNUSE, STRING( m_iszUnUse ) );

	*/
	BaseClass::Precache();
}

bool CAI_PlayerAlly::ShouldSuspendMonolog( void )
{
	float flDist;

	flDist = (GetExpresser()->GetMonologueTarget()->GetLocalOrigin() - GetLocalOrigin()).Length();
	
	if( flDist >= 384 )
	{
		return true;
	}

	return false;
}

bool CAI_PlayerAlly::ShouldResumeMonolog( void )
{
	float flDist;

	if( HasCondition( COND_SEE_PLAYER ) )
	{
		flDist = (GetExpresser()->GetMonologueTarget()->GetLocalOrigin() - GetLocalOrigin()).Length();
		
		if( flDist <= 256 )
		{
			return true;
		}
	}

	return false;
}

void CAI_PlayerAlly::ModifyOrAppendCriteria( AI_CriteriaSet& set )
{
	BaseClass::ModifyOrAppendCriteria( set );

	// TODO:  Add any talker specific keys
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_PlayerAlly::OnKilledNPC( CBaseCombatCharacter *pKilled )
{
	SpeakIfAllowed( TLK_ENEMY_DEAD );
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(talk_monster_base,CAI_PlayerAlly)

	DECLARE_USES_SCHEDULE_PROVIDER( CAI_FollowBehavior )
	
	DECLARE_TASK(TASK_TALKER_RESPOND)
	DECLARE_TASK(TASK_TALKER_SPEAK)
	DECLARE_TASK(TASK_TALKER_HELLO)
	DECLARE_TASK(TASK_TALKER_BETRAYED)
	DECLARE_TASK(TASK_TALKER_HEADRESET)
	DECLARE_TASK(TASK_TALKER_STOPSHOOTING)
	DECLARE_TASK(TASK_TALKER_STARE)
	DECLARE_TASK(TASK_TALKER_LOOK_AT_CLIENT)
	DECLARE_TASK(TASK_TALKER_CLIENT_STARE)
	DECLARE_TASK(TASK_TALKER_EYECONTACT)
	DECLARE_TASK(TASK_TALKER_IDEALYAW)
	DECLARE_TASK(TASK_FIND_LOCK_HINTNODE_HEALTH)

	DECLARE_CONDITION(COND_TALKER_CLIENTUNSEEN)

	//=========================================================
	// > SCHED_TALKER_IDLE_RESPONSE
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_RESPONSE,

		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"	// Stop and listen
		"		TASK_WAIT					0.5"				// Wait until sure it's me they are talking to
		"		TASK_TALKER_EYECONTACT		0"					// Wait until speaker is done
		"		TASK_TALKER_RESPOND			0"					// Wait and then say my response
		"		TASK_TALKER_IDEALYAW		0"					// look at who I'm talking to
		"		TASK_FACE_IDEAL				0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_SIGNAL3"
		"		TASK_TALKER_EYECONTACT		0"					// Wait until speaker is done
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_SPEAK
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_SPEAK,

		"	Tasks"
		"		TASK_TALKER_SPEAK			0"			// question or remark
		"		TASK_TALKER_IDEALYAW		0"			// look at who I'm talking to
		"		TASK_FACE_IDEAL				0"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_SIGNAL3"
		"		TASK_TALKER_EYECONTACT		0"
		"		TASK_WAIT_RANDOM			0.5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_SPEAK_WAIT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_SPEAK_WAIT,

		"	Tasks"
		"		TASK_SET_ACTIVITY			ACTIVITY:ACT_SIGNAL3"	// Stop and talk
		"		TASK_TALKER_SPEAK			0"						// question or remark
		"		TASK_TALKER_EYECONTACT		0"						//
		"		TASK_WAIT					2"						// wait - used when sci is in 'use' mode to keep head turned
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_HELLO
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_HELLO,

		"	Tasks"
		"		 TASK_SET_ACTIVITY				ACTIVITY:ACT_SIGNAL3"	// Stop and talk
		"		 TASK_TALKER_HELLO				0"			// Try to say hello to player
		"		 TASK_TALKER_EYECONTACT			0"
		"		 TASK_WAIT						0.5"		// wait a bit
		"		 TASK_TALKER_HELLO				0"			// Try to say hello to player
		"		 TASK_TALKER_EYECONTACT			0"
		"		 TASK_WAIT						0.5"		// wait a bit
		"		 TASK_TALKER_HELLO				0"			// Try to say hello to player
		"		 TASK_TALKER_EYECONTACT			0"
		"		 TASK_WAIT						0.5"		// wait a bit
		"		 TASK_TALKER_HELLO				0"			// Try to say hello to player
		"		 TASK_TALKER_EYECONTACT			0"
		"		 TASK_WAIT						0.5	"		// wait a bit
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_DANGER"
		"		COND_PLAYER_PUSHING"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_STOP_SHOOTING
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_STOP_SHOOTING,

		"	Tasks"
		"		 TASK_TALKER_STOPSHOOTING	0"	// tell player to stop shooting friend
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// Scold the player before attacking.
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_BETRAYED,

		"	Tasks"
		"		TASK_TALKER_BETRAYED	0"
		"		TASK_WAIT				0.5"
		""
		"	Interrupts"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_WATCH_CLIENT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_WATCH_CLIENT,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_TALKER_LOOK_AT_CLIENT		6"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"		// sound flags - change these and you'll break the talking code.
		"		COND_HEAR_DANGER"
		"		COND_SMELL"
		"		COND_PLAYER_PUSHING"
		"		COND_TALKER_CLIENTUNSEEN"
	)
	 
	//=========================================================
	// > SCHED_TALKER_IDLE_WATCH_CLIENT_STARE
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_WATCH_CLIENT_STARE,

		"	Tasks"
		"		 TASK_STOP_MOVING				0"
		"		 TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		 TASK_TALKER_CLIENT_STARE		6"
		"		 TASK_TALKER_STARE				0"
		"		 TASK_TALKER_IDEALYAW			0"			// look at who I'm talking to
		"		 TASK_FACE_IDEAL				0			 "
		"		 TASK_SET_ACTIVITY				ACTIVITY:ACT_SIGNAL3"
		"		 TASK_TALKER_EYECONTACT			0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"		// sound flags - change these and you'll break the talking code.
		"		COND_HEAR_DANGER"
		"		COND_SMELL"
		"		COND_PLAYER_PUSHING"
		"		COND_TALKER_CLIENTUNSEEN"
	)

	//=========================================================
	// > SCHED_TALKER_IDLE_EYE_CONTACT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_IDLE_EYE_CONTACT,

		"	Tasks"
		"		TASK_TALKER_IDEALYAW			0"			// look at who I'm talking to
		"		TASK_FACE_IDEAL					0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_SIGNAL3"
		"		TASK_TALKER_EYECONTACT			0"			// Wait until speaker is done
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
	)

	//=========================================================
	// > SCHED_TALKER_LOOK_FOR_HEALTHKIT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_LOOK_FOR_HEALTHKIT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_IDLE_STAND"
		"		TASK_FIND_LOCK_HINTNODE_HEALTH			0"
		"		TASK_GET_PATH_TO_HINTNODE				0"
		"		TASK_STORE_POSITION_IN_SAVEPOSITION		0"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"		TASK_FACE_HINTNODE						0"
		"		TASK_PLAY_HINT_ACTIVITY					0"
		"		TASK_ADD_HEALTH							50"
		"		TASK_SET_SCHEDULE						SCHEDULE:SCHED_TALKER_RETURN_FROM_HEALTHKIT"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SMELL"
		"		COND_PROVOKED"
		"		COND_GIVE_WAY"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_BULLET_IMPACT"
		
	)

	//=========================================================
	// > SCHED_TALKER_RETURN_FROM_HEALTHKIT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_TALKER_RETURN_FROM_HEALTHKIT,

		"	Tasks"
		"		TASK_GET_PATH_TO_SAVEPOSITION			0"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SMELL"
		"		COND_PROVOKED"
		"		COND_GIVE_WAY"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_BULLET_IMPACT"
		
	)

AI_END_CUSTOM_NPC()

AI_BEGIN_CUSTOM_NPC(talk_monster,CNPCSimpleTalker)
AI_END_CUSTOM_NPC()
	
