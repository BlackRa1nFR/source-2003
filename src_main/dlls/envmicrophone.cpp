//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Implements an entity that measures sound volume at a point in a map.
//
//			This entity listens as though it is an NPC, meaning it will only
//			hear sounds that were emitted using the CSound::InsertSound function.
//
//			It does not hear danger sounds since they are not technically sounds.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "entityinput.h"
#include "entityoutput.h"
#include "eventqueue.h"
#include "mathlib.h"
#include "soundent.h"
#include "envmicrophone.h"
#include "soundflags.h"

//#define DEBUG_MICROPHONE

// HACKHACK: This is evil. Share out the one from the engine.
#define SNDLVL_TO_DIST_MULT( sndlvl ) ( sndlvl ? ((pow( 10, 60.0/*snd_refdb.GetFloat()*/ / 20 ) / pow( 10, (float)sndlvl / 20 )) / 36.0/*snd_refdist.GetFloat()*/) : 0 )

const int SF_MICROPHONE_SOUND_COMBAT			= 0x01;
const int SF_MICROPHONE_SOUND_WORLD				= 0x02;
const int SF_MICROPHONE_SOUND_PLAYER			= 0x04;
const int SF_MICROPHONE_SOUND_BULLET_IMPACT		= 0x08;
const int SF_MICROPHONE_SWALLOW_ROUTED_SOUNDS	= 0x10;

const float MICROPHONE_SETTLE_EPSILON = 0.005;

LINK_ENTITY_TO_CLASS(env_microphone, CEnvMicrophone);

BEGIN_DATADESC( CEnvMicrophone )

	DEFINE_KEYFIELD(CEnvMicrophone, m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),
	DEFINE_FIELD(CEnvMicrophone, m_hMeasureTarget, FIELD_EHANDLE),
	DEFINE_KEYFIELD(CEnvMicrophone, m_nSoundMask, FIELD_INTEGER, "SoundMask"),
	DEFINE_KEYFIELD(CEnvMicrophone, m_flSensitivity, FIELD_FLOAT, "Sensitivity"),
	DEFINE_KEYFIELD(CEnvMicrophone, m_flSmoothFactor, FIELD_FLOAT, "SmoothFactor"),
	DEFINE_KEYFIELD(CEnvMicrophone, m_iszSpeakerName, FIELD_STRING, "SpeakerName"),
	DEFINE_FIELD(CEnvMicrophone, m_hSpeaker, FIELD_EHANDLE),
	// DEFINE_FIELD(CEnvMicrophone, m_bAvoidFeedback, FIELD_BOOLEAN),	// DONT SAVE
	DEFINE_KEYFIELD(CEnvMicrophone, m_flMaxRange, FIELD_FLOAT, "MaxRange"),

	DEFINE_INPUTFUNC(CEnvMicrophone, FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(CEnvMicrophone, FIELD_VOID, "Disable", InputDisable),

	DEFINE_OUTPUT(CEnvMicrophone, m_SoundLevel, "SoundLevel"),
	DEFINE_OUTPUT(CEnvMicrophone, m_OnRoutedSound, "OnRoutedSound" ),

END_DATADESC()

// Global list of env_microphones who want to be told whenever a sound is started
CUtlVector< CHandle<CEnvMicrophone> >	g_Microphones;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEnvMicrophone::~CEnvMicrophone( void )
{
	g_Microphones.FindAndRemove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CEnvMicrophone::Spawn(void)
{
	//
	// Build our sound type mask from our spawnflags.
	//
	static int nFlags[][2] =
	{
		{ SF_MICROPHONE_SOUND_COMBAT,			SOUND_COMBAT },
		{ SF_MICROPHONE_SOUND_WORLD,			SOUND_WORLD },
		{ SF_MICROPHONE_SOUND_PLAYER,			SOUND_PLAYER },
		{ SF_MICROPHONE_SOUND_BULLET_IMPACT,	SOUND_BULLET_IMPACT },
	};

	for (int i = 0; i < sizeof(nFlags) / sizeof(nFlags[0]); i++)
	{
		if (m_spawnflags & nFlags[i][0])
		{
			m_nSoundMask |= nFlags[i][1];
		}
	}

	if (m_flSensitivity == 0)
	{
		//
		// Avoid a divide by zero in CanHearSound.
		//
		m_flSensitivity = 1;
	}
	else if (m_flSensitivity > 10)
	{
		m_flSensitivity = 10;
	}

	m_flSmoothFactor = clamp(m_flSmoothFactor, 0, 0.9);

	if (!m_bDisabled)
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called after all entities have spawned and after a load game.
//			Finds the reference point at which to measure sound level.
//-----------------------------------------------------------------------------
void CEnvMicrophone::Activate(void)
{
	BaseClass::Activate();

	if (m_target != NULL_STRING)
	{
		m_hMeasureTarget = gEntList.FindEntityByName(NULL, m_target, NULL);

		//
		// If we were given a bad measure target, just measure sound where we are.
		//
		if ((m_hMeasureTarget == NULL) || (m_hMeasureTarget->pev == NULL))
		{
			Warning( "EnvMicrophone - Measure target not found or measure target with no origin!\n");
			m_hMeasureTarget = this;
		}
	}
	else
	{
		m_hMeasureTarget = this;
	}

	if ( m_iszSpeakerName != NULL_STRING )
	{
		m_hSpeaker = gEntList.FindEntityByName(NULL, STRING(m_iszSpeakerName), NULL);

		if ( !m_hSpeaker )
		{
			Warning( "EnvMicrophone %s specifies a non-existent speaker name: %s\n", STRING(GetEntityName()), STRING(m_iszSpeakerName) );
		}
		else
		{
			// We've got a speaker to play heard sounds through. To do this, we need to add ourselves 
			// to the list of microphones who want to be told whenever a sound is played.
			g_Microphones.AddToTail( this );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Stops the microphone from sampling the sound level and firing the
//			SoundLevel output.
//-----------------------------------------------------------------------------
void CEnvMicrophone::InputEnable( inputdata_t &inputdata )
{
	if (m_bDisabled)
	{
		m_bDisabled = false;
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Resumes sampling the sound level and firing the SoundLevel output.
//-----------------------------------------------------------------------------
void CEnvMicrophone::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
	SetNextThink( TICK_NEVER_THINK );
}


//-----------------------------------------------------------------------------
// Purpose: Checks whether this microphone can hear a given sound, and at what
//			relative volume level.
// Input  : pSound - Sound to test.
//			flVolume - Returns with the relative sound volume from 0 - 1.
// Output : Returns true if the sound could be heard at the sample point, false if not.
//-----------------------------------------------------------------------------
bool CEnvMicrophone::CanHearSound(CSound *pSound, float &flVolume)
{
	float flDistance = (pSound->GetSoundOrigin() - m_hMeasureTarget->GetAbsOrigin()).Length();

	if (flDistance == 0)
	{
		flVolume = 1.0;
		return true;
	}

	// Over our max range?
	if ( m_flMaxRange && flDistance > m_flMaxRange )
		return false;

	if (flDistance <= pSound->Volume() * m_flSensitivity)
	{
		flVolume = 1 - (flDistance / (pSound->Volume() * m_flSensitivity));
		flVolume = clamp(flVolume, 0, 1);
		return true;
	}

	flVolume = 0;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the microphone can hear the specified sound
//-----------------------------------------------------------------------------
bool CEnvMicrophone::CanHearSound( int entindex, soundlevel_t soundlevel, float &flVolume, const Vector *pOrigin )
{
	// Sound might be coming from an origin or from an entity.
	float flDistance = 0;
	if ( pOrigin )
	{
		flDistance = (*pOrigin - m_hMeasureTarget->GetAbsOrigin()).Length();
	}
	else if ( entindex )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( engine->PEntityOfEntIndex(entindex));
		if ( pEntity )
		{
			flDistance = (pEntity->WorldSpaceCenter() - m_hMeasureTarget->GetAbsOrigin()).Length();
		}
	}

	// Over our max range?
	if ( m_flMaxRange && flDistance > m_flMaxRange )
		return false;

#ifdef DEBUG_MICROPHONE
	Msg(" flVolume %f ", flVolume );
#endif

	// Reduce the volume by the amount it fell to get to the microphone
	float dist_mult = SNDLVL_TO_DIST_MULT(soundlevel);
	float relative_dist = flDistance * dist_mult;
	float gain = 1.0;
	if (relative_dist > 0.1)
	{
		gain *= (1/relative_dist);
	}
	else
	{
		gain *= 10.0;
	}
	gain = clamp( gain, 0.0, 1.0 );
	flVolume *= gain;

#ifdef DEBUG_MICROPHONE
	Msg("dist %2f, soundlevel %d, dist_mult %f, relative_dist %f, gain %f", flDistance, (int)soundlevel, dist_mult, relative_dist, gain );
	if ( !flVolume )
	{
		Msg(" : REJECTED\n" );
	}
	else
	{
		Msg(" : SENT\n" );
	}
#endif

	return ( flVolume > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Listens for sounds and updates the value of the SoundLevel output.
//-----------------------------------------------------------------------------
void CEnvMicrophone::Think(void)
{
	int nSound = CSoundEnt::ActiveList();

	float flMaxVolume = 0;
	
	//
	// Find the loudest sound that this microphone cares about.
	//
	while (nSound != SOUNDLIST_EMPTY)
	{
		CSound *pCurrentSound = CSoundEnt::SoundPointerForIndex(nSound);

		if (pCurrentSound)
		{
			if (m_nSoundMask & pCurrentSound->SoundType())
			{
				float flVolume = 0;
				if (CanHearSound(pCurrentSound, flVolume) && (flVolume > flMaxVolume))
				{
					flMaxVolume = flVolume;
				}
			}
		}

		nSound = pCurrentSound->NextSound();
	}

	if (flMaxVolume != m_SoundLevel.Get())
	{
		//
		// Don't smooth if we are within an epsilon. This allows the output to stop firing
		// much more quickly.
		//
		if (fabs(flMaxVolume - m_SoundLevel.Get()) < MICROPHONE_SETTLE_EPSILON)
		{
			m_SoundLevel.Set(flMaxVolume, this, this);
		}
		else
		{
			m_SoundLevel.Set(flMaxVolume * (1 - m_flSmoothFactor) + m_SoundLevel.Get() * m_flSmoothFactor, this, this);
		}
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Hook for the sound system to tell us when a sound's been played
//-----------------------------------------------------------------------------
bool CEnvMicrophone::SoundPlayed( int entindex, const char *soundname, soundlevel_t soundlevel, float flVolume, int iFlags, int iPitch, const Vector *pOrigin, float soundtime )
{
	if ( m_bAvoidFeedback )
		return false;

	// Don't hear sounds that have already been heard by a microphone to avoid feedback!
	if ( iFlags & SND_SPEAKER )
		return false;

#ifdef DEBUG_MICROPHONE
	Msg("%s heard %s: ", STRING(GetEntityName()), soundname );
#endif

	if ( !CanHearSound( entindex, soundlevel, flVolume, pOrigin ) )
		return false;

	// We've heard it. Play it out our speaker. If our speaker's gone away, we're done.
	if ( !m_hSpeaker )
	{
		g_Microphones.FindAndRemove( this );
		return false;
	}

	m_bAvoidFeedback = true;

	// Add the speaker flag. Detected at playback and applies the speaker filter.
	iFlags |= SND_SPEAKER;
	CPASAttenuationFilter filter( m_hSpeaker );
	CBaseEntity::EmitSound( filter, m_hSpeaker->entindex(), CHAN_STATIC, soundname, flVolume, soundlevel, iFlags, iPitch, &m_hSpeaker->GetAbsOrigin(), soundtime );
	m_OnRoutedSound.FireOutput( this, this, 0 );

	m_bAvoidFeedback = false;

	// Do we want to allow the original sound to play?
	if ( m_spawnflags & SF_MICROPHONE_SWALLOW_ROUTED_SOUNDS )
		return true;

	return false;
}

