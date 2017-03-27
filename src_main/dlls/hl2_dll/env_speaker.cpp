//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// HL2 Speaker entity. 
//
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "mathlib.h"
#include "AI_Speech.h"
#include "stringregistry.h"
#include "gamerules.h"
#include "game.h"
#include <ctype.h>
#include "mem_fgets.h"
#include "EntityList.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "ndebugoverlay.h"
#include "soundscape.h"

// ===================================================================================
//
// Speaker class. Used for announcements per level, for door lock/unlock spoken voice. 
//

#define SF_SPEAKER_START_SILENT		1
#define SF_SPEAKER_EVERYWHERE		2
class CSpeaker : public CPointEntity
{
public:
	DECLARE_CLASS( CSpeaker, CPointEntity );

	virtual void Activate();
	void Spawn( void );
	void Precache( void );
	
	DECLARE_DATADESC();

	virtual int	ObjectCaps( void ) { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

protected:

	void SpeakerThink( void );

	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	float	m_delayMin;
	float	m_delayMax;

	string_t	m_iszRuleScriptFile;
	string_t	m_iszConcept;
};

LINK_ENTITY_TO_CLASS( env_speaker, CSpeaker );

BEGIN_DATADESC( CSpeaker )

	DEFINE_KEYFIELD( CSpeaker, m_delayMin, FIELD_FLOAT, "delaymin" ),
	DEFINE_KEYFIELD( CSpeaker, m_delayMax, FIELD_FLOAT, "delaymax" ),
	DEFINE_KEYFIELD( CSpeaker, m_iszRuleScriptFile, FIELD_STRING, "rulescript" ),
	DEFINE_KEYFIELD( CSpeaker, m_iszConcept, FIELD_STRING, "concept" ),

	// Function Pointers
	DEFINE_FUNCTION( CSpeaker, SpeakerThink ),

	DEFINE_INPUTFUNC( CSpeaker, FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( CSpeaker, FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( CSpeaker, FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()


void CSpeaker::Spawn( void )
{
	const char *soundfile = (const char *)STRING( m_iszRuleScriptFile );

	if ( Q_strlen( soundfile ) < 1 )
	{
		Warning( "'speaker' entity with no Level/Sentence! at: %f, %f, %f\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		SetNextThink( gpGlobals->curtime + 0.1f );
		SetThink( SUB_Remove );
		return;
	}

//	const char *concept = (const char *)STRING( m_iszConcept );
//	if ( Q_strlen( concept ) < 1 )
//	{
//		Warning( "'speaker' entity using rule set %s with empty concept string\n", soundfile );
//	}

    SetSolid( SOLID_NONE );
    SetMoveType( MOVETYPE_NONE );
	
	SetThink(SpeakerThink);
	SetNextThink( TICK_NEVER_THINK );

	// allow on/off switching via 'use' function.

	Precache( );
}


void CSpeaker::Precache( void )
{
	if ( !FBitSet (m_spawnflags, SF_SPEAKER_START_SILENT ) )
	{
		// set first announcement time for random n second
		SetNextThink( gpGlobals->curtime + random->RandomFloat(5.0, 15.0) );
	}
}


void CSpeaker::Activate()
{
	BaseClass::Activate();

	// This is done here, instead of in Spawn, for save/load to work
	InstallCustomResponseSystem( STRING( m_iszRuleScriptFile ) );
}


void CSpeaker::SpeakerThink( void )
{
	// Wait for the talking characters to finish first.
	if ( !g_AITalkSemaphore.IsAvailable() )
	{
		SetNextThink( gpGlobals->curtime + g_AITalkSemaphore.GetReleaseTime() + random->RandomFloat( 5, 10 ) );
		return;
	}
	
	DispatchResponse( m_iszConcept.ToCStr() );

	SetNextThink( gpGlobals->curtime + random->RandomFloat(m_delayMin, m_delayMax) );

	// time delay until it's ok to speak: used so that two NPCs don't talk at once
	g_AITalkSemaphore.Acquire( 5 );		
}


void CSpeaker::InputTurnOn( inputdata_t &inputdata )
{
	// turn on announcements
	SetNextThink( gpGlobals->curtime + 0.1 );
}


void CSpeaker::InputTurnOff( inputdata_t &inputdata )
{
	// turn off announcements
	SetNextThink( TICK_NEVER_THINK );
}


//
// If an announcement is pending, cancel it.  If no announcement is pending, start one.
//
void CSpeaker::InputToggle( inputdata_t &inputdata )
{
	int fActive = (GetNextThink() > 0.0 );

	// fActive is true only if an announcement is pending
	if ( fActive )
	{
		// turn off announcements
		SetNextThink( TICK_NEVER_THINK );
	}
	else 
	{
		// turn on announcements
		SetNextThink( gpGlobals->curtime + 0.1f );
	} 
}
