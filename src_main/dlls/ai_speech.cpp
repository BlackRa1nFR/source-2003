//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "ai_speech.h"

#include "engine/IEngineSound.h"
#include <keyvalues.h>
#include "ai_basenpc.h"
#include "AI_Criteria.h"
#include "AI_ResponseSystem.h"
#include "isaverestore.h"

//-----------------------------------------------------------------------------

CAI_TimedSemaphore g_AITalkSemaphore;

ConceptHistory_t::~ConceptHistory_t()
{
	delete response;
}

ConceptHistory_t::ConceptHistory_t( const ConceptHistory_t& src )
{
	timeSpoken = src.timeSpoken;
	response = NULL;
	if ( src.response )
	{
		response = new AI_Response( *src.response );
	}
}

ConceptHistory_t& ConceptHistory_t::operator =( const ConceptHistory_t& src )
{
	if ( this == &src )
		return *this;

	timeSpoken = src.timeSpoken;
	response = NULL;
	if ( src.response )
	{
		response = new AI_Response( *src.response );
	}

	return *this;
}

BEGIN_SIMPLE_DATADESC( ConceptHistory_t )
	DEFINE_FIELD( ConceptHistory_t,	timeSpoken,	FIELD_TIME ),  // Relative to server time
	// DEFINE_EMBEDDED( CAI_Expresser::ConceptHistory_t,	response,	FIELD_??? ),	// This is manually saved/restored by the ConceptHistory saverestore ops below
END_DATADESC()

class CConceptHistoriesDataOps : public CDefSaveRestoreOps
{
public:
	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		CUtlDict< ConceptHistory_t, int > *ch = ((CUtlDict< ConceptHistory_t, int > *)fieldInfo.pField);
		int count = ch->Count();
		pSave->WriteInt( &count );
		for ( int i = 0 ; i < count; i++ )
		{
			ConceptHistory_t *pHistory = &(*ch)[ i ];

			pSave->StartBlock();
			{

				// Write element name
				pSave->WriteString( ch->GetElementName( i ) );

				// Write data
				pSave->WriteAll( pHistory );
				// Write response blob
				bool hasresponse = pHistory->response != NULL ? true : false;
				pSave->WriteBool( &hasresponse );
				if ( hasresponse )
				{
					pSave->WriteAll( pHistory->response );
				}
				// TODO: Could blat out pHistory->criteria pointer here, if it's needed
			}
			pSave->EndBlock();
		}
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		CUtlDict< ConceptHistory_t, int > *ch = ((CUtlDict< ConceptHistory_t, int > *)fieldInfo.pField);

		int count = pRestore->ReadInt();
		Assert( count >= 0 );
		for ( int i = 0 ; i < count; i++ )
		{
			char conceptname[ 512 ];
			conceptname[ 0 ] = 0;
			ConceptHistory_t history;

			pRestore->StartBlock();
			{
				pRestore->ReadString( conceptname, sizeof( conceptname ), 0 );

				pRestore->ReadAll( &history );

				bool hasresponse = false;

				pRestore->ReadBool( &hasresponse );
				if ( hasresponse )
				{
					history.response = new AI_Response();
					pRestore->ReadAll( history.response );
				}
			}

			pRestore->EndBlock();

			// TODO: Could restore pHistory->criteria pointer here, if it's needed

			// Add to utldict
			if ( conceptname[0] != 0 )
			{
				ch->Insert( conceptname, history );
			}
			else
			{
				Assert( !"Error restoring ConceptHistory_t, discarding!" );
			}
		}
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		CUtlDict< ConceptHistory_t, int > *ch = ((CUtlDict< ConceptHistory_t, int > *)fieldInfo.pField);
		return ch->Count() == 0 ? true : false;
	}
};

CConceptHistoriesDataOps g_ConceptHistoriesSaveDataOps;

//-----------------------------------------------------------------------------
//
// CLASS: CAI_Expresser
//

BEGIN_SIMPLE_DATADESC( CAI_Expresser )
	//									m_pSink		(reconnected on load)
	DEFINE_CUSTOM_FIELD( CAI_Expresser,  m_ConceptHistories,	&g_ConceptHistoriesSaveDataOps ),
	DEFINE_FIELD(		CAI_Expresser,	m_flStopTalkTime,		FIELD_FLOAT		),
	DEFINE_FIELD(		CAI_Expresser,	m_voicePitch,			FIELD_INTEGER	),
	DEFINE_AUTO_ARRAY(	CAI_Expresser,	m_szMonologSentence,	FIELD_CHARACTER	),
	DEFINE_FIELD(		CAI_Expresser,	m_iMonologIndex,		FIELD_INTEGER	),
	DEFINE_FIELD(		CAI_Expresser,	m_fMonologSuspended,	FIELD_BOOLEAN	),
	DEFINE_FIELD(		CAI_Expresser,	m_hMonologTalkTarget,	FIELD_EHANDLE	),
END_DATADESC()

//-------------------------------------

bool CAI_Expresser::SemaphoreIsAvailable()
{
	return g_AITalkSemaphore.IsAvailable();
}

//-------------------------------------

int CAI_Expresser::GetVoicePitch() const
{
	return m_voicePitch + random->RandomInt(0,3);
}

//-------------------------------------

static const int LEN_SPECIFIC_SCENE_MODIFIER = strlen( AI_SPECIFIC_SCENE_MODIFIER );

//-----------------------------------------------------------------------------
// Purpose: Placeholder for rules based response system
// Input  : concept - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_Expresser::Speak( AIConcept_t concept, const char *modifiers /*= NULL*/ )
{
	IResponseSystem *rs = GetOuter()->GetResponseSystem();
	if ( !rs )
	{
		Assert( !"No response system installed for CAI_Expresser::GetOuter()!!!" );
		return false;
	}

	AI_CriteriaSet set;
	// Always include the concept name
	set.AppendCriteria( "concept", concept, CONCEPT_WEIGHT );
	// Always include any optional modifiers
	if ( modifiers != NULL )
	{
		set.AppendCriteria( "modifiers", modifiers, CONCEPT_WEIGHT );
	}
	// Let NPC fill in most match criteria
	GetOuter()->ModifyOrAppendCriteria( set );
	// Append local player criteria to set,too
	CBasePlayer::ModifyOrAppendPlayerCriteria( set );

	// Now that we have a criteria set, ask for a suitable response
	AI_Response *result = new AI_Response;
	bool found = rs->FindBestResponse( set, *result );
	if ( !found )
	{
		//Assert( !"rs->FindBestResponse: Returned a NULL AI_Response!" );
		return false;
	}

	const char *response = result->GetResponse();

	if ( !response || !response[0] )
		return false;

	if ( result->GetOdds() < 100 && random->RandomInt( 1, 100 ) <= result->GetOdds() )
		return false;
	
	float delay = result->GetDelay();
	
	bool spoke = false;

	soundlevel_t soundlevel = result->GetSoundLevel();

	switch ( result->GetType() )
	{
	default:
	case AI_Response::RESPONSE_NONE:
		break;
	case AI_Response::RESPONSE_SPEAK:
		{
			GetOuter()->EmitSound( response );

			// HACK:  Server needs a way of figuring out how long a .wav is when played through this API.
			NoteSpeaking( 1.5f, delay );
			spoke = true;
		}
		break;
	case AI_Response::RESPONSE_SENTENCE:
		{
			spoke = ( -1 != SpeakRawSentence( response, delay, VOL_NORM, soundlevel ) ) ? true : false;
		}
		break;
	case AI_Response::RESPONSE_SCENE:
		{
			spoke = SpeakRawScene( response, delay );
		}
		break;
	case AI_Response::RESPONSE_WAV:
		{
			// FIXME:  Get pitch from npc?

			CPASAttenuationFilter filter( GetOuter() );
			CBaseEntity::EmitSound( filter, GetOuter()->entindex(), CHAN_VOICE, response, 1, soundlevel, 0, PITCH_NORM );

			// HACK:  Server needs a way of figuring out how long a .wav is...
			NoteSpeaking( 1.5f, delay );
			spoke = true;
		}
		break;
	case AI_Response::RESPONSE_RESPONSE:
		{
			// This should have been recursively resolved already
			Assert( 0 );
		}
		break;
	}

	if ( spoke )
	{
		SetSpokeConcept( concept, result );
	}

	return spoke;
}

bool CAI_Expresser::SpeakRawScene( const char *pszScene, float delay )
{
	float speakTime = GetOuter()->PlayScene( pszScene );
	if ( speakTime > 0 )
	{
		NoteSpeaking( speakTime, delay );
		return true;
	}
	return false;
}

//-------------------------------------

int CAI_Expresser::SpeakRawSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, CBaseEntity *pListener )
{
	int sentenceIndex = -1;

	if ( !pszSentence )
		return sentenceIndex;

	if ( pszSentence[0] == AI_SP_START_MONOLOG )
	{
		// this sentence command will start this NPC speaking 
		// lengthy monolog from smaller sentences. 
		BeginMonolog( (char *)pszSentence, pListener );
		return -1;
	}
	else if ( pszSentence[0] != AI_SP_MONOLOG_LINE )
	{
		// this bit of speech is interrupting my monolog!
		SuspendMonolog( 0 );
	}

	if ( pszSentence[0] == AI_SP_SPECIFIC_SENTENCE || pszSentence[0] == AI_SP_MONOLOG_LINE )
	{
		sentenceIndex = SENTENCEG_Lookup( pszSentence );

		if( sentenceIndex == -1 )
		{
			// sentence not found
			return -1;
		}

		CPASAttenuationFilter filter( GetOuter(), soundlevel );
		enginesound->EmitSentenceByIndex( filter, GetOuter()->entindex(), CHAN_VOICE, sentenceIndex, volume, soundlevel, 0, GetVoicePitch());
	}
	else
	{
		sentenceIndex = SENTENCEG_PlayRndSz( GetEdict(), pszSentence, volume, soundlevel, 0, GetVoicePitch() );
	}

	NoteSpeaking( engine->SentenceLength( sentenceIndex ), delay );

	return sentenceIndex;
}

//-------------------------------------

void CAI_Expresser::NoteSpeaking( float duration, float delay )
{
	duration += delay;
	
	GetSink()->OnStartSpeaking();

	if ( duration <= 0 )
	{
		// no duration :( 
		m_flStopTalkTime = gpGlobals->curtime + 3;
		duration = 0;
	}
	else
	{
		m_flStopTalkTime = gpGlobals->curtime + duration;
	}
	g_AITalkSemaphore.Acquire( duration );
}

//-------------------------------------

bool CAI_Expresser::IsSpeaking( void )
{
	return ( m_flStopTalkTime > gpGlobals->curtime );
}

//-------------------------------------
bool CAI_Expresser::CanSpeakConcept( AIConcept_t concept )
{
	// Not in history?
	int iter = m_ConceptHistories.Find( concept );
	if ( iter == m_ConceptHistories.InvalidIndex() )
	{
		return true;
	}

	ConceptHistory_t *history = &m_ConceptHistories[iter];
	Assert( history );

	AI_Response *response = history->response;
	if ( !response )
		return true;

	if ( response->GetSpeakOnce() ) 
		return false;

	float respeakDelay = response->GetRespeakDelay();

	if ( respeakDelay != 0.0f )
	{
		if ( history->timeSpoken != -1 && ( gpGlobals->curtime < history->timeSpoken + respeakDelay ) )
			return false;
	}

	return true;
}

//-------------------------------------

bool CAI_Expresser::SpokeConcept( AIConcept_t concept )
{
	return GetTimeSpokeConcept( concept ) != -1.f;
}

//-------------------------------------

float CAI_Expresser::GetTimeSpokeConcept( AIConcept_t concept )
{
	int iter = m_ConceptHistories.Find( concept );
	if ( iter == m_ConceptHistories.InvalidIndex() ) 
		return -1;
	
	ConceptHistory_t *h = &m_ConceptHistories[iter];
	return h->timeSpoken;
}

//-------------------------------------

void CAI_Expresser::SetSpokeConcept( AIConcept_t concept, AI_Response *response )
{
	int idx = m_ConceptHistories.Find( concept );
	if ( idx == m_ConceptHistories.InvalidIndex() )
	{
		ConceptHistory_t h;
		h.timeSpoken = gpGlobals->curtime;
		idx = m_ConceptHistories.Insert( concept, h );
	}

	ConceptHistory_t *slot = &m_ConceptHistories[ idx ];

	slot->timeSpoken = gpGlobals->curtime;
	// Update response info
	if ( response )
	{
		AI_Response *r = slot->response;
		if ( r )
		{
			delete r;
		}

		// FIXME:  Are we leaking AI_Responses?
		slot->response = response;
	}
}

//-------------------------------------

void CAI_Expresser::ClearSpokeConcept( AIConcept_t concept )
{
	m_ConceptHistories.Remove( concept );
}

void CAI_Expresser::DumpHistories()
{
	int c = 1;
	for ( int i = m_ConceptHistories.First(); i != m_ConceptHistories.InvalidIndex(); i = m_ConceptHistories.Next(i ) )
	{
		ConceptHistory_t *h = &m_ConceptHistories[ i ];

		Msg( "%i: %s at %f\n", c++, m_ConceptHistories.GetElementName( i ), h->timeSpoken );
	}
}

//-------------------------------------

void CAI_Expresser::BeginMonolog( char *pszSentenceName, CBaseEntity *pListener )
{
	if( pListener )
	{
		m_hMonologTalkTarget = pListener;
	}
	else
	{
		Warning( "NULL Listener in BeginMonolog()!\n" );
		Assert(0);
		EndMonolog();
		return;
	}

	Q_strncpy( m_szMonologSentence, pszSentenceName ,sizeof(m_szMonologSentence));

	// change the "AI_SP_START_MONOLOG" to an "AI_SP_MONOLOG_LINE". m_sMonologSentence is now the 
	// string we'll tack numbers onto to play sentences from this group in 
	// sequential order.
	m_szMonologSentence[0] = AI_SP_MONOLOG_LINE;

	m_fMonologSuspended = false;

	m_iMonologIndex = 0;
}

//-------------------------------------

void CAI_Expresser::EndMonolog( void )
{
	m_szMonologSentence[0] = 0;
	m_iMonologIndex = -1;
	m_fMonologSuspended = false;
	m_hMonologTalkTarget = NULL;
}

//-------------------------------------

void CAI_Expresser::SpeakMonolog( void )
{
	int i;
	char szSentence[ MONOLOGNAME_LEN ];

	if( !HasMonolog() )
	{
		return;
	}

	if( !IsSpeaking() )
	{
		if( m_fMonologSuspended )
		{
			if ( GetSink()->ShouldResumeMonolog() )
			{
				ResumeMonolog();
			}

			return;
		}

		Q_snprintf( szSentence,sizeof(szSentence), "%s%d", m_szMonologSentence, m_iMonologIndex );
		m_iMonologIndex++;

		i = SpeakRawSentence( szSentence, 0, VOL_NORM );

		if ( i == -1 )
		{
			EndMonolog();
		}
	}
	else
	{
		if( GetSink()->ShouldSuspendMonolog() )
		{
			SuspendMonolog( 0 );
		}
	}
}

//-------------------------------------

void CAI_Expresser::SuspendMonolog( float flInterval )
{
	if( HasMonolog() )
	{
		m_fMonologSuspended = true;
	}
	
	// free up other characters to speak.
	g_AITalkSemaphore.Release();
}

//-------------------------------------

void CAI_Expresser::ResumeMonolog( void )
{
	if( m_iMonologIndex > 0 )
	{
		// back up and repeat what I was saying
		// when interrupted.
		m_iMonologIndex--;
	}

	GetSink()->OnResumeMonolog();
	m_fMonologSuspended = false;
}

//-----------------------------------------------------------------------------
//
// An NPC base class to assist a branch of the inheritance graph
// in utilizing CAI_Expresser
//

BEGIN_DATADESC(CAI_ExpressiveNPC)
	DEFINE_EMBEDDEDBYREF( CAI_ExpressiveNPC,	m_pExpresser ),
END_DATADESC()


CAI_ExpressiveNPC::~CAI_ExpressiveNPC()
{
	delete m_pExpresser;
}

//-------------------------------------

bool CAI_ExpressiveNPC::CreateComponents()
{
	if ( !BaseClass::CreateComponents() )
		return false;

	m_pExpresser = CreateExpresser();
	if ( !m_pExpresser)
		return false;

	m_pExpresser->SetOuter(this);
	m_pExpresser->Connect(this);
	m_pExpresser->EndMonolog();

	return true;
}

//-------------------------------------

CAI_Expresser *CAI_ExpressiveNPC::CreateExpresser()
{
	return new CAI_Expresser;
}

//-------------------------------------

int CAI_ExpressiveNPC::PlaySentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, CBaseEntity *pListener )
{
	return GetExpresser()->SpeakRawSentence( pszSentence, delay, volume, soundlevel, pListener );
}

//-----------------------------------------------------------------------------

void CAI_ExpressiveNPC::ModifyOrAppendCriteria( AI_CriteriaSet& set )
{
	BaseClass::ModifyOrAppendCriteria( set );

	// Append current activity name
  	set.AppendCriteria( "activity", GetActivityName( GetActivity() ) );

	static const char *pStateNames[] = { "None", "Idle", "Alert", "Combat", "Scripted", "PlayDead", "Dead" };
	if ( (int)m_NPCState < ARRAYSIZE(pStateNames) )
	{
		set.AppendCriteria( "npcstate", UTIL_VarArgs( "[NPCState::%s]", pStateNames[m_NPCState] ) );
	}

	if ( GetEnemy() )
	{
		set.AppendCriteria( "enemy", GetEnemy()->GetClassname() );
	}

	set.AppendCriteria( "speed", UTIL_VarArgs( "%.3f", GetAbsVelocity().Length() ) );

	CBaseCombatWeapon *weapon = GetActiveWeapon();
	if ( weapon )
	{
		set.AppendCriteria( "weapon", weapon->GetClassname() );
	}
	else
	{
		set.AppendCriteria( "weapon", "none" );
	}

	CBasePlayer *player = ( CBasePlayer * )CBaseEntity::Instance( engine->PEntityOfEntIndex( 1 ) );
	if ( player )
	{
		Vector distance = player->GetAbsOrigin() - GetAbsOrigin();

		set.AppendCriteria( "distancetoplayer", UTIL_VarArgs( "%f", distance.Length() ) );

	}
	else
	{
		set.AppendCriteria( "distancetoplayer", UTIL_VarArgs( "%i", MAX_COORD_RANGE ) );
	}
}

IResponseSystem *CAI_ExpressiveNPC::GetResponseSystem()
{
	extern IResponseSystem *g_pResponseSystem;
	// Expressive NPC's use the general response system
	return g_pResponseSystem;
}
