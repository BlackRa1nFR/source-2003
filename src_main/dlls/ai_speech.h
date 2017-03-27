//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef AI_SPEECH_H
#define AI_SPEECH_H

#include "utlmap.h"

#include "soundflags.h"
#include "ai_component.h"
#include "ai_basenpc.h"
#include "utldict.h"

#if defined( _WIN32 )
#pragma once
#endif

class KeyValues;

//-----------------------------------------------------------------------------
// Purpose: Used to share a global resource or prevent a system stepping on
//			own toes.
//-----------------------------------------------------------------------------

class CAI_TimedSemaphore
{
public:
	CAI_TimedSemaphore()
	 :	m_ReleaseTime( 0 )
	{
	}
	
	void Acquire( float time)		{ m_ReleaseTime = gpGlobals->curtime + time; }
	void Release()					{ m_ReleaseTime = 0; }
	
	bool IsAvailable() const		{ return (gpGlobals->curtime > m_ReleaseTime); }
	float GetReleaseTime() const 	{ return m_ReleaseTime; }

private:
	float m_ReleaseTime;
};

//-----------------------------------------------------------------------------

extern CAI_TimedSemaphore g_AITalkSemaphore;

//-----------------------------------------------------------------------------
// Basic speech system types
//-----------------------------------------------------------------------------

//-------------------------------------
// Constants


const float AIS_DEF_MIN_DELAY 	= 2.8; // Minimum amount of time an NPCs will wait after someone has spoken before considering speaking again
const float AIS_DEF_MAX_DELAY 	= 3.2; // Maximum amount of time an NPCs will wait after someone has spoken before considering speaking again
const float AIS_NO_DELAY  		= 0;
const soundlevel_t AIS_DEF_SNDLVL 	 	= SNDLVL_TALKING;
#define AI_NULL_CONCEPT NULL
#define MONOLOGNAME_LEN	16				// sentence names passed as monolog may be no longer than this.

#define AI_NULL_SENTENCE NULL

// Sentence prefix constants
#define AI_SP_START_MONOLOG 	'~'
#define AI_SP_MONOLOG_LINE  	'@'
#define AI_SP_SPECIFIC_SENTENCE	'!'
#define AI_SP_WAVFILE			'^'
#define AI_SP_SCENE_GROUP		'='
#define AI_SP_SPECIFIC_SCENE	'?'

#define AI_SPECIFIC_SENTENCE(str_constant)	"!" str_constant
#define AI_WAVFILE(str_constant)			"^" str_constant
// @Note (toml 09-12-02): as scene groups are not currently implemented, the string is a semi-colon delimited list
#define AI_SCENE_GROUP(str_constant)		"=" str_constant
#define AI_SPECIFIC_SCENE(str_constant)		"?" str_constant

// Designer overriding modifiers
#define AI_SPECIFIC_SCENE_MODIFIER "scene:"

//-------------------------------------

enum AIConceptFlags_t
{
	AICF_DEFAULT 	= 0,
	AICF_SPEAK_ONCE = 0x01
};

//-------------------------------------
// An id that represents the core meaning of a spoken phrase, 
// eventually to be mapped to a sentence group or scene

typedef const char *AIConcept_t;

//-------------------------------------
// Specifies and stores the base timing and attentuation values for concepts
//
class AI_Response;

//-----------------------------------------------------------------------------
// CAI_Expresser
//
// Purpose: Provides the functionality of going from abstract concept ("hello")
//			to specific sentence/scene/.wav
//

//-------------------------------------
// Sink supports behavior control and receives notifications of internal events

class CAI_ExpresserSink
{
public:
	virtual bool ShouldSuspendMonolog() { return false; }
	virtual bool ShouldResumeMonolog() 	{ return true; }
	virtual void OnResumeMonolog() 		{}
	virtual void OnStartSpeaking()		{}
};

struct ConceptHistory_t
{
	DECLARE_SIMPLE_DATADESC();

	ConceptHistory_t(float timeSpoken = -1 )
	 : timeSpoken( timeSpoken ), response( NULL )
	{
	}

	ConceptHistory_t( const ConceptHistory_t& src );
	ConceptHistory_t& operator = ( const ConceptHistory_t& src );

	~ConceptHistory_t();
	
	float		timeSpoken;
	AI_Response *response;
};
//-------------------------------------

class CAI_Expresser : public CAI_Component
{
public:
	CAI_Expresser( CAI_BaseNPC *pOuter = NULL )
	 :	CAI_Component( pOuter ),
	 	m_pSink( NULL ),
	 	m_flStopTalkTime( 0 ),
		m_voicePitch( 100 ),
		m_iMonologIndex( 0 ),
		m_fMonologSuspended( false )
	{
		m_szMonologSentence[MONOLOGNAME_LEN] = 0;
		m_hMonologTalkTarget = NULL;
	}

	// --------------------------------
	
	bool Connect( CAI_ExpresserSink *pSink )		{ m_pSink = pSink; return true; }
	bool Disconnect( CAI_ExpresserSink *pSink )	{ m_pSink = NULL; return true;}

	// --------------------------------
	
	bool Speak( AIConcept_t concept, const char *modifiers = NULL );

	int SpeakRawSentence( const char *pszSentence, float delay, float volume = VOL_NORM, soundlevel_t soundlevel = SNDLVL_TALKING, CBaseEntity *pListener = NULL );
	
	bool SemaphoreIsAvailable();

	// --------------------------------
	
	bool IsSpeaking();
	float GetTimeSpeechComplete() const 	{ return m_flStopTalkTime; }
	void  BlockSpeechUntil( float time ) 	{ m_flStopTalkTime = time; }

	// --------------------------------
	
	// --------------------------------
	
	bool CanSpeakConcept( AIConcept_t concept );
	bool SpokeConcept( AIConcept_t concept );
	float GetTimeSpokeConcept( AIConcept_t concept ); // returns -1 if never
	void SetSpokeConcept( AIConcept_t concept, AI_Response *response );
	void ClearSpokeConcept( AIConcept_t concept );
	
	// --------------------------------
	
	void SetVoicePitch( int voicePitch )	{ m_voicePitch = voicePitch; }
	int GetVoicePitch() const;

	// --------------------------------
	//
	// Monologue operations
	//
	
	bool HasMonolog( void ) { return m_iMonologIndex != -1; };
	void BeginMonolog( char *pszSentenceName, CBaseEntity *pListener );
	void EndMonolog( void );
	void SpeakMonolog( void );

	void SuspendMonolog( float flInterval );
	void ResumeMonolog( void );

	CBaseEntity *GetMonologueTarget()						{ return m_hMonologTalkTarget.Get(); }
	void NoteSpeaking( float duration, float delay = 0 );

protected:
	bool SpeakRawScene( const char *pszScene, float delay );

	void DumpHistories();

private:
	// --------------------------------
	
	CAI_ExpresserSink *GetSink() { return m_pSink; }

	// --------------------------------
	//
	// Internal types
	//


	
	// --------------------------------
	
	CAI_ExpresserSink *m_pSink;
	
	// --------------------------------
	//
	// Speech concept data structures
	//

	CUtlDict< ConceptHistory_t, int > m_ConceptHistories;
	
	// --------------------------------
	//
	// Speaking states
	//

	float				m_flStopTalkTime;// when in the future that I'll be done saying this sentence.
	int					m_voicePitch;					// pitch of voice for this head
	
	// --------------------------------
	//
	// Monologue data
	//
	char		m_szMonologSentence[MONOLOGNAME_LEN];	// The name of the sentence group for the monolog I'm speaking.
	int			m_iMonologIndex;						// Which sentence from the group I should be speaking.
	bool		m_fMonologSuspended;
	EHANDLE		m_hMonologTalkTarget;					// Who I'm trying to deliver my monolog to. 

	DECLARE_SIMPLE_DATADESC();

};

//-----------------------------------------------------------------------------
//
// An NPC base class to assist a branch of the inheritance graph
// in utilizing CAI_Expresser
//

// @TODO (toml 09-12-02): Not real happy with the naming of this class, and probably "Expresser" as well...

class CAI_ExpressiveNPC : public CAI_BaseNPC, private CAI_ExpresserSink
{
	DECLARE_CLASS( CAI_ExpressiveNPC, CAI_BaseNPC );

public:
	virtual void		NoteSpeaking( float duration, float delay );

	DECLARE_DATADESC();
protected:
	CAI_ExpressiveNPC()
	 :	m_pExpresser( NULL )
	{
	}

	~CAI_ExpressiveNPC();

	virtual bool CreateComponents();
	virtual CAI_Expresser *CreateExpresser();

	virtual CAI_Expresser *GetExpresser();
	
	virtual bool Speak( AIConcept_t concept, const char *modifiers = NULL );
	int PlaySentence( const char *pszSentence, float delay, float volume = VOL_NORM, soundlevel_t soundlevel = SNDLVL_TALKING, CBaseEntity *pListener = NULL );

	virtual void		ModifyOrAppendCriteria( AI_CriteriaSet& set );

	virtual IResponseSystem *GetResponseSystem();

	// Override of base entity response input handler
	virtual void	DispatchResponse( const char *conceptName )
	{
		Speak( (AIConcept_t)conceptName );
	}

private:
	//---------------------------------

	CAI_Expresser *m_pExpresser;

};


inline void CAI_ExpressiveNPC::NoteSpeaking( float duration, float delay )
{ 
	m_pExpresser->NoteSpeaking( duration, delay ); 
}

//-------------------------------------

inline CAI_Expresser *CAI_ExpressiveNPC::GetExpresser() 
{ 
	return m_pExpresser; 
}
	
//-------------------------------------

inline bool CAI_ExpressiveNPC::Speak( AIConcept_t concept, const char *modifiers /*= NULL*/ ) 
{ 
	return m_pExpresser->Speak( concept, modifiers ); 
}

//-----------------------------------------------------------------------------

#endif // AI_SPEECH_H
