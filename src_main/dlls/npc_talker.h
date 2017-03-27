//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef TALKNPC_H
#define TALKNPC_H

#ifdef _LINUX
#undef time
#include <time.h>
#endif

#pragma warning(push)
#include <set>
#pragma warning(pop)

#ifdef _WIN32
#pragma once
#endif

#include "soundflags.h"

#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_default.h"
#include "ai_speech.h"
#include "ai_basenpc.h"
#include "ai_baseactor.h"
#include "ai_behavior.h"
#include "ai_behavior_follow.h"

#define TLK_ANSWER 		"TLK_ANSWER"
#define TLK_QUESTION 	"TLK_QUESTION"
#define TLK_IDLE 		"TLK_IDLE"
#define TLK_STARE 		"TLK_STARE"
#define TLK_USE 		"TLK_USE"
#define TLK_UNUSE 		"TLK_UNUSE"
#define TLK_STOP 		"TLK_STOP"
#define TLK_NOSHOOT 	"TLK_NOSHOOT"
#define TLK_HELLO 		"TLK_HELLO"
#define TLK_PHELLO 		"TLK_PHELLO"
#define TLK_PIDLE 		"TLK_PIDLE"
#define TLK_PQUESTION 	"TLK_PQUESTION"
#define TLK_PLHURT1 	"TLK_PLHURT1"
#define TLK_PLHURT2 	"TLK_PLHURT2"
#define TLK_PLHURT3 	"TLK_PLHURT3"
#define TLK_SMELL 		"TLK_SMELL"
#define TLK_WOUND 		"TLK_WOUND"
#define TLK_MORTAL 		"TLK_MORTAL"
#define TLK_DANGER		"TLK_DANGER"
#define TLK_SEE_COMBINE	"TLK_SEE_COMBINE"
#define TLK_ENEMY_DEAD	"TLK_ENEMY_DEAD"
#define TLK_SELECTED	"TLK_SELECTED"	// selected by player in command mode.
#define TLK_COMMANDED	"TLK_COMMANDED" // received orders from player in command mode
#define TLK_BETRAYED	"TLK_BETRAYED"	// player killed an ally in front of me.
#define TLK_ALLY_KILLED	"TLK_ALLY_KILLED" // witnessed an ally die some other way.

// resume is "as I was saying..." or "anyhow..."
#define TLK_RESUME 		"TLK_RESUME"

// tourguide stuff below
#define TLK_TGSTAYPUT 	"TLK_TGSTAYPUT"
#define TLK_TGFIND 		"TLK_TGFIND"
#define TLK_TGSEEK 		"TLK_TGSEEK"
#define TLK_TGLOSTYOU 	"TLK_TGLOSTYOU"
#define TLK_TGCATCHUP 	"TLK_TGCATCHUP"
#define TLK_TGENDTOUR 	"TLK_TGENDTOUR"

//=========================================================
// Talking NPC base class
// Used for scientists and barneys
//=========================================================

#define TALKRANGE_MIN 500.0				// don't talk to anyone farther away than this

#define TLK_STARE_DIST	128				// anyone closer than this and looking at me is probably staring at me.

#define TLK_CFRIENDS		3

//=============================================================================
// >> CNPCSimpleTalker
//=============================================================================

class CAI_PlayerAlly : public CAI_BaseActor
{
	DECLARE_CLASS( CAI_PlayerAlly, CAI_BaseActor );
public:
	// AI functions
	Activity		NPC_TranslateActivity( Activity newActivity );
	virtual int		TranslateSchedule( int scheduleType );
	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );
	void			HandleAnimEvent( animevent_t *pEvent );
	void			GatherConditions( void );
	void			PrescheduleThink( void );

	virtual void OnPlayerSelect();
	virtual void OnMoveOrder();
	virtual void OnTargetOrder();
	virtual void PlayerSelect( bool select );

	virtual bool IsPlayerAlly();
	
	CBaseEntity *GetSpeechTarget()						{ return m_hTalkTarget.Get(); }
	void SetSpeechTarget( CBaseEntity *pSpeechTarget ) 	{ m_hTalkTarget = pSpeechTarget; }

	bool			OnBehaviorChangeStatus( CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule );
	int ObjectCaps( void ) { return UsableNPCObjectCaps(BaseClass::ObjectCaps()); }

	inline bool SpeakIfAllowed( AIConcept_t concept, const char *modifiers = NULL ) 
	{ 
		if( IsOkToSpeak() )
			return Speak( concept, modifiers );
		return false;
	}


	//=========================================================
	// TalkNPC schedules
	//=========================================================
	enum
	{
		SCHED_TALKER_IDLE_RESPONSE = LAST_SHARED_SCHEDULE,
		SCHED_TALKER_IDLE_SPEAK,
		SCHED_TALKER_IDLE_HELLO,
		SCHED_TALKER_IDLE_SPEAK_WAIT,
		SCHED_TALKER_IDLE_STOP_SHOOTING,
		SCHED_TALKER_IDLE_WATCH_CLIENT,
		SCHED_TALKER_IDLE_WATCH_CLIENT_STARE,
		SCHED_TALKER_IDLE_EYE_CONTACT,
		SCHED_TALKER_BETRAYED,
		SCHED_TALKER_LOOK_FOR_HEALTHKIT,
		SCHED_TALKER_RETURN_FROM_HEALTHKIT,

		// !ALWAYS LAST!
		NEXT_SCHEDULE,	
	};

	//=========================================================
	// TalkNPC tasks
	//=========================================================
	enum
	{
		TASK_TALKER_RESPOND = LAST_SHARED_TASK,		// say my response
		TASK_TALKER_SPEAK,			// question or remark
		TASK_TALKER_HELLO,			// Try to say hello to player
		TASK_TALKER_BETRAYED,		// Player killed an ally
		TASK_TALKER_HEADRESET,		// reset head position
		TASK_TALKER_STOPSHOOTING,	// tell player to stop shooting friend
		TASK_TALKER_STARE,			// let the player know I know he's staring at me.
		TASK_TALKER_LOOK_AT_CLIENT,// faces player if not moving and not talking and in idle.
		TASK_TALKER_CLIENT_STARE,	// same as look at client, but says something if the player stares.
		TASK_TALKER_EYECONTACT,	// maintain eyecontact with person who I'm talking to
		TASK_TALKER_IDEALYAW,		// set ideal yaw to face who I'm talking to
		TASK_FIND_LOCK_HINTNODE_HEALTH, // Find & lock a nearby healthkit hintnode to heal myself at

		// !ALWAYS LAST!
		NEXT_TASK,		
	};

	//=========================================================
	// TalkNPC Conditions
	//=========================================================
	enum 
	{
		COND_TALKER_CLIENTUNSEEN = LAST_SHARED_CONDITION,
		
		// !ALWAYS LAST!
		NEXT_CONDITION
	};

	virtual void	 ModifyOrAppendCriteria( AI_CriteriaSet& set );

public:
	CBaseEntity		*FindNearestFriend(bool fPlayer);
	virtual int		FriendNumber( int arrayNumber )	{ return arrayNumber; }
	CBaseEntity		*EnumFriends( CBaseEntity *pentPrevious, int listNumber, bool bTrace );
	virtual	void	OnKilledNPC( CBaseCombatCharacter *pKilled );


	static char *m_szFriends[TLK_CFRIENDS];		// array of friend names

protected:
	void			TalkInit( void );				
	// Base NPC functions
	void			Precache( void );

	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void			Touch(	CBaseEntity *pOther );
	void			Event_Killed( const CTakeDamageInfo &info );
	Disposition_t	IRelationType ( CBaseEntity *pTarget );
	virtual bool	CanPlaySentence( bool fDisregardState );

	virtual int		PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener );
	virtual bool	KeyValue( const char *szKeyName, const char *szValue );

	CBaseEntity		*EyeLookTarget( void );		// Override to look at talk target

	bool			IsOkToSpeak( void );
	bool			IsOkToCombatSpeak( void );
	void			TrySmellTalk( void );

	virtual void	OnChangeRunningBehavior( CAI_BehaviorBase *pOldBehavior,  CAI_BehaviorBase *pNewBehavior );

	//---------------------------------
	
	void OnStartSpeaking();
	bool ShouldSuspendMonolog( void );
	bool ShouldResumeMonolog( void );
	void OnResumeMonolog() 		{	Speak( TLK_RESUME ); }
	
	//---------------------------------
	
	string_t	m_iszUse;						// Custom +USE sentence group (follow)
	string_t	m_iszUnUse;						// Custom +USE sentence group (stop following)

private:

	//float			TargetDistance( void );
	void			StopTalking( void ) { SentenceStop(); }

	// Conversations / communication
	void			IdleRespond( void );
	int				FIdleSpeak( void );
	int				FIdleStare( void );
	int				FIdleHello( void );

	void			AlertFriends( CBaseEntity *pKiller );
	void			ShutUpFriends( void );

	virtual void	SetAnswerQuestion( CAI_PlayerAlly *pSpeaker );

	//---------------------------------
	
	EHANDLE		m_hTalkTarget;	// who to look at while talking
	int			m_nSpeak;						// number of times initiated talking

	//float		m_flLastSaidSmelled;// last time we talked about something that stinks
	float		m_flLastHealthKitSearch;	// last time we looked for a healthkit

protected:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};

class CNPCSimpleTalker : public CAI_PlayerAlly
{
	DECLARE_CLASS( CNPCSimpleTalker, CAI_PlayerAlly );
public:
	virtual void			StartFollowing( CBaseEntity *pLeader ) { m_FollowBehavior.SetFollowTarget( pLeader ); DeferSchedulingToBehavior( &m_FollowBehavior ); }
	virtual void			StopFollowing( ) { m_FollowBehavior.SetFollowTarget( NULL ); DeferSchedulingToBehavior( NULL ); }
	CBaseEntity		*GetFollowTarget( void ) { return m_FollowBehavior.GetFollowTarget(); }

	int				PlayScriptedSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, bool bConcurrent, CBaseEntity *pListener );
	virtual void 	FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void			Event_Killed( const CTakeDamageInfo &info );

	bool CreateBehaviors()
	{
		AddBehavior( &m_FollowBehavior );
		return BaseClass::CreateBehaviors();
	}

	// For following
	virtual void	DeclineFollowing( void ) {}
	void			LimitFollowers( CBaseEntity *pPlayer, int maxFollowers );

	float			GetUseTime() const { return m_useTime; }

private:
	CAI_FollowBehavior m_FollowBehavior;
	float		m_useTime;						// Don't allow +USE until this time

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};

#endif		//TALKNPC_H
