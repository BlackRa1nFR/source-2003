//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: Entities relating to in-level sound effects.
//
//			ambient_generic: a sound emitter used for one-shot and looping sounds.
//
//			env_speaker: used for public address announcements over loudspeakers.
//				This tries not to drown out talking NPCs.
//
//			env_soundscape: controls what sound script an area uses.
//
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "mathlib.h"
#include "ai_speech.h"
#include "stringregistry.h"
#include "gamerules.h"
#include "game.h"
#include <ctype.h>
#include "mem_fgets.h"
#include "entitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "ndebugoverlay.h"
#include "soundscape.h"


//-----------------------------------------------------------------------------
// Purpose: Compute a suitable attenuation value given an audible radius
// Input  : radius - 
//			playEverywhere - (disable attenuation)
//-----------------------------------------------------------------------------
#define REFERENCE_dB			60.0
#define	REFERENCE_dB_DISTANCE	36.0

static soundlevel_t ComputeSoundlevel( float radius, bool playEverywhere )
{
	soundlevel_t soundlevel = SNDLVL_NONE;

	if ( radius > 0 && !playEverywhere )
	{
		// attenuation is set to a distance, compute falloff

		float dB_loss = 20 * log10( radius / REFERENCE_dB_DISTANCE );

		soundlevel = (soundlevel_t)(int)(40 + dB_loss); // sound at 40dB at reference distance
	}

	return soundlevel;
}

// ==================== GENERIC AMBIENT SOUND ======================================

// runtime pitch shift and volume fadein/out structure

// NOTE: IF YOU CHANGE THIS STRUCT YOU MUST CHANGE THE SAVE/RESTORE VERSION NUMBER
// SEE BELOW (in the typedescription for the class)
typedef struct dynpitchvol
{
	// NOTE: do not change the order of these parameters 
	// NOTE: unless you also change order of rgdpvpreset array elements!
	int preset;

	int pitchrun;		// pitch shift % when sound is running 0 - 255
	int pitchstart;		// pitch shift % when sound stops or starts 0 - 255
	int spinup;			// spinup time 0 - 100
	int spindown;		// spindown time 0 - 100

	int volrun;			// volume change % when sound is running 0 - 10
	int volstart;		// volume change % when sound stops or starts 0 - 10
	int fadein;			// volume fade in time 0 - 100
	int fadeout;		// volume fade out time 0 - 100

						// Low Frequency Oscillator
	int	lfotype;		// 0) off 1) square 2) triangle 3) random
	int lforate;		// 0 - 1000, how fast lfo osciallates
	
	int lfomodpitch;	// 0-100 mod of current pitch. 0 is off.
	int lfomodvol;		// 0-100 mod of current volume. 0 is off.

	int cspinup;		// each trigger hit increments counter and spinup pitch


	int	cspincount;

	int pitch;			
	int spinupsav;
	int spindownsav;
	int pitchfrac;

	int vol;
	int fadeinsav;
	int fadeoutsav;
	int volfrac;

	int	lfofrac;
	int	lfomult;


} dynpitchvol_t;

#define CDPVPRESETMAX 27

// presets for runtime pitch and vol modulation of ambient sounds

dynpitchvol_t rgdpvpreset[CDPVPRESETMAX] = 
{
// pitch	pstart	spinup	spindwn	volrun	volstrt	fadein	fadeout	lfotype	lforate	modptch modvol	cspnup		
{1,	255,	 75,	95,		95,		10,		1,		50,		95, 	0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0}, 
{2,	255,	 85,	70,		88,		10,		1,		20,		88,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0}, 
{3,	255,	100,	50,		75,		10,		1,		10,		75,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{4,	100,	100,	0,		0,		10,		1,		90,		90,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{5,	100,	100,	0,		0,		10,		1,		80,		80,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{6,	100,	100,	0,		0,		10,		1,		50,		70,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{7,	100,	100,	0,		0,		 5,		1,		40,		50,		1,		50,		0,		10,		0,		0,0,0,0,0,0,0,0,0,0},
{8,	100,	100,	0,		0,		 5,		1,		40,		50,		1,		150,	0,		10,		0,		0,0,0,0,0,0,0,0,0,0},
{9,	100,	100,	0,		0,		 5,		1,		40,		50,		1,		750,	0,		10,		0,		0,0,0,0,0,0,0,0,0,0},
{10,128,	100,	50,		75,		10,		1,		30,		40,		2,		 8,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{11,128,	100,	50,		75,		10,		1,		30,		40,		2,		25,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{12,128,	100,	50,		75,		10,		1,		30,		40,		2,		70,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{13,50,		 50,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{14,70,		 70,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{15,90,		 90,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{16,120,	120,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{17,180,	180,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{18,255,	255,	0,		0,		10,		1,		20,		50,		0,		0,		0,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{19,200,	 75,	90,		90,		10,		1,		50,		90,		2,		100,	20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{20,255,	 75,	97,		90,		10,		1,		50,		90,		1,		40,		50,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{21,100,	100,	0,		0,		10,		1,		30,		50,		3,		15,		20,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{22,160,	160,	0,		0,		10,		1,		50,		50,		3,		500,	25,		0,		0,		0,0,0,0,0,0,0,0,0,0},
{23,255,	 75,	88,		0,		10,		1,		40,		0,		0,		0,		0,		0,		5,		0,0,0,0,0,0,0,0,0,0}, 
{24,200,	 20,	95,	    70,		10,		1,		70,		70,		3,		20,		50,		0,		0,		0,0,0,0,0,0,0,0,0,0}, 
{25,180,	100,	50,		60,		10,		1,		40,		60,		2,		90,		100,	100,	0,		0,0,0,0,0,0,0,0,0,0}, 
{26,60,		 60,	0,		0,		10,		1,		40,		70,		3,		80,		20,		50,		0,		0,0,0,0,0,0,0,0,0,0}, 
{27,128,	 90,	10,		10,		10,		1,		20,		40,		1,		5,		10,		20,		0,		0,0,0,0,0,0,0,0,0,0}
};

class CAmbientGeneric : public CPointEntity
{
public:
	DECLARE_CLASS( CAmbientGeneric, CPointEntity );

	bool KeyValue( const char *szKeyName, const char *szValue );
	void Spawn( void );
	void Precache( void );
	void Activate( void );
	void RampThink( void );
	void InitModulationParms(void);

	bool IsSoundSource( void ) { return true; };
	void ToggleSound();

	// Input handlers
	void InputPlaySound( inputdata_t &inputdata );
	void InputStopSound( inputdata_t &inputdata );
	void InputToggleSound( inputdata_t &inputdata );
	void InputPitch( inputdata_t &inputdata );
	void InputVolume( inputdata_t &inputdata );

	DECLARE_DATADESC();

	float m_radius;
	soundlevel_t m_iSoundLevel;		// dB value
	dynpitchvol_t m_dpv;	

	bool m_fActive;		// only true when the entity is playing a looping sound
	bool m_fLooping;		// true when the sound played will loop

	string_t m_iszSound;			// Path/filename of WAV file to play.
	string_t m_sSourceEntName;
	EHANDLE m_hSoundSource;	// entity from which the sound comes
};

LINK_ENTITY_TO_CLASS( ambient_generic, CAmbientGeneric );

BEGIN_DATADESC( CAmbientGeneric )

	DEFINE_KEYFIELD( CAmbientGeneric, m_iszSound, FIELD_SOUNDNAME, "message" ),
	DEFINE_KEYFIELD( CAmbientGeneric, m_radius,			FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( CAmbientGeneric, m_sSourceEntName,	FIELD_STRING, "SourceEntityName" ),
	// recomputed in Activate()
	// DEFINE_FIELD( CAmbientGeneric, m_hSoundSource, EHANDLE ),

	DEFINE_FIELD( CAmbientGeneric, m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAmbientGeneric, m_fLooping, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAmbientGeneric, m_iSoundLevel, FIELD_INTEGER ),

	// HACKHACK - This is not really in the spirit of the save/restore design, but save this
	// out as a binary data block.  If the dynpitchvol_t is changed, old saved games will NOT
	// load these correctly, so bump the save/restore version if you change the size of the struct
	// The right way to do this is to split the input parms (read in keyvalue) into members and re-init this
	// struct in Precache(), but it's unlikely that the struct will change, so it's not worth the time right now.
	DEFINE_ARRAY( CAmbientGeneric, m_dpv, FIELD_CHARACTER, sizeof(dynpitchvol_t) ),

	// Function Pointers
	DEFINE_FUNCTION( CAmbientGeneric, RampThink ),

	// Inputs
	DEFINE_INPUTFUNC(CAmbientGeneric, FIELD_VOID, "PlaySound", InputPlaySound ),
	DEFINE_INPUTFUNC(CAmbientGeneric, FIELD_VOID, "StopSound", InputStopSound ),
	DEFINE_INPUTFUNC(CAmbientGeneric, FIELD_VOID, "ToggleSound", InputToggleSound ),
	DEFINE_INPUTFUNC(CAmbientGeneric, FIELD_FLOAT, "Pitch", InputPitch ),
	DEFINE_INPUTFUNC(CAmbientGeneric, FIELD_FLOAT, "Volume", InputVolume ),

END_DATADESC()


#define SF_AMBIENT_SOUND_EVERYWHERE		1
#define SF_AMBIENT_SOUND_START_SILENT		16
#define SF_AMBIENT_SOUND_NOT_LOOPING		32


void CAmbientGeneric::Spawn( void )
{
	m_iSoundLevel = ComputeSoundlevel( m_radius, FBitSet( m_spawnflags, SF_AMBIENT_SOUND_EVERYWHERE )?true:false );

	char *szSoundFile = (char *)STRING( m_iszSound );
	if ( !m_iszSound || strlen( szSoundFile ) < 1 )
	{
		Warning( "EMPTY AMBIENT AT: %f, %f, %f\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove(this);
		return;
	}

    SetSolid( SOLID_NONE );
    SetMoveType( MOVETYPE_NONE );

	// Set up think function for dynamic modification 
	// of ambient sound's pitch or volume. Don't
	// start thinking yet.

	SetThink(&CAmbientGeneric::RampThink);
	SetNextThink( TICK_NEVER_THINK );

	m_fActive = false;

	if ( FBitSet ( m_spawnflags, SF_AMBIENT_SOUND_NOT_LOOPING ) )
	{
		m_fLooping = false;
	}
	else
	{
		m_fLooping = true;
	}

	m_hSoundSource = NULL;

	Precache( );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for changing pitch.
// Input  : Float new pitch from 0 - 255 (100 = as recorded).
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputPitch( inputdata_t &inputdata )
{
	m_dpv.pitch = clamp( inputdata.value.Float(), 0, 255 );

	CBaseEntity* pSoundSource = m_hSoundSource;
	if (pSoundSource)
	{
		UTIL_EmitAmbientSound(pSoundSource, pSoundSource->GetAbsOrigin(), STRING( m_iszSound ), m_dpv.vol * 0.01, m_iSoundLevel, SND_CHANGE_PITCH, m_dpv.pitch);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for changing volume.
// Input  : Float new volume, from 0 - 10.
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputVolume( inputdata_t &inputdata )
{
	//
	// Multiply the input value by ten since volumes are expected to be from 0 - 100.
	//
	m_dpv.vol = clamp( inputdata.value.Float(), 0, 10 ) * 10;
	CBaseEntity* pSoundSource = m_hSoundSource;
	if (pSoundSource)
	{	
		UTIL_EmitAmbientSound(pSoundSource, pSoundSource->GetAbsOrigin(), STRING( m_iszSound ), m_dpv.vol * 0.01, m_iSoundLevel, SND_CHANGE_VOL, m_dpv.pitch);
	}
}


void CAmbientGeneric::Precache( void )
{
	char *szSoundFile = (char *)STRING( m_iszSound );
	if ( m_iszSound != NULL_STRING && strlen( szSoundFile ) > 1 )
	{
		if (*szSoundFile != '!')
		{
			enginesound->PrecacheSound(szSoundFile);
		}
	}

	// init all dynamic modulation parms
	InitModulationParms();

	if ( !FBitSet (m_spawnflags, SF_AMBIENT_SOUND_START_SILENT ) )
	{
		// start the sound ASAP
		if (m_fLooping)
			m_fActive = true;
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CAmbientGeneric::Activate( void )
{
	BaseClass::Activate();

	// Initialize sound source.  If no source was given, or source can't be found
	// then this is the source
	if (m_hSoundSource == NULL)
	{
		if (m_sSourceEntName != NULL_STRING)
		{
			m_hSoundSource = gEntList.FindEntityByName( NULL, m_sSourceEntName, NULL );
		}
		if (m_hSoundSource == NULL)
		{
			m_hSoundSource = this;
		}
	}

	// If active start the sound
	if ( m_fActive )
	{
		CBaseEntity* pSoundSource = m_hSoundSource;
		if (pSoundSource)
		{
			UTIL_EmitAmbientSound ( pSoundSource, pSoundSource->GetAbsOrigin(), STRING( m_iszSound ), 
				(m_dpv.vol * 0.01), m_iSoundLevel, SND_SPAWNING, m_dpv.pitch);
		}

		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Think at 5hz if we are dynamically modifying pitch or volume of the
//			playing sound.  This function will ramp pitch and/or volume up or
//			down, modify pitch/volume with lfo if active.
//-----------------------------------------------------------------------------
void CAmbientGeneric::RampThink( void )
{
	char *szSoundFile = (char *)STRING( m_iszSound );
	int pitch = m_dpv.pitch; 
	int vol = m_dpv.vol;
	int flags = 0;
	int fChanged = 0;		// false if pitch and vol remain unchanged this round
	int	prev;

	if (!m_dpv.spinup && !m_dpv.spindown && !m_dpv.fadein && !m_dpv.fadeout && !m_dpv.lfotype)
		return;						// no ramps or lfo, stop thinking

	// ==============
	// pitch envelope
	// ==============
	if (m_dpv.spinup || m_dpv.spindown)
	{
		prev = m_dpv.pitchfrac >> 8;

		if (m_dpv.spinup > 0)
			m_dpv.pitchfrac += m_dpv.spinup;
		else if (m_dpv.spindown > 0)
			m_dpv.pitchfrac -= m_dpv.spindown;

		pitch = m_dpv.pitchfrac >> 8;
		
		if (pitch > m_dpv.pitchrun)
		{
			pitch = m_dpv.pitchrun;
			m_dpv.spinup = 0;				// done with ramp up
		}

		if (pitch < m_dpv.pitchstart)
		{
			pitch = m_dpv.pitchstart;
			m_dpv.spindown = 0;				// done with ramp down

			// shut sound off
			CBaseEntity* pSoundSource = m_hSoundSource;
			if (pSoundSource)
			{
				UTIL_EmitAmbientSound(pSoundSource, pSoundSource->GetAbsOrigin(), szSoundFile, 
						0, SNDLVL_NONE, SND_STOP, 0);
			}

			// return without setting m_flNextThink
			return;
		}

		if (pitch > 255) pitch = 255;
		if (pitch < 1) pitch = 1;

		m_dpv.pitch = pitch;

		fChanged |= (prev != pitch);
		flags |= SND_CHANGE_PITCH;
	}

	// ==================
	// amplitude envelope
	// ==================
	if (m_dpv.fadein || m_dpv.fadeout)
	{
		prev = m_dpv.volfrac >> 8;

		if (m_dpv.fadein > 0)
			m_dpv.volfrac += m_dpv.fadein;
		else if (m_dpv.fadeout > 0)
			m_dpv.volfrac -= m_dpv.fadeout;

		vol = m_dpv.volfrac >> 8;

		if (vol > m_dpv.volrun)
		{
			vol = m_dpv.volrun;
			m_dpv.fadein = 0;				// done with ramp up
		}

		if (vol < m_dpv.volstart)
		{
			vol = m_dpv.volstart;
			m_dpv.fadeout = 0;				// done with ramp down
			
			// shut sound off
			CBaseEntity* pSoundSource = m_hSoundSource;
			if (pSoundSource)
			{
				UTIL_EmitAmbientSound(pSoundSource, pSoundSource->GetAbsOrigin(), szSoundFile, 
						0, SNDLVL_NONE, SND_STOP, 0);
			}

			// return without setting m_flNextThink
			return;
		}

		if (vol > 100) vol = 100;
		if (vol < 1) vol = 1;

		m_dpv.vol = vol;

		fChanged |= (prev != vol);
		flags |= SND_CHANGE_VOL;
	}

	// ===================
	// pitch/amplitude LFO
	// ===================
	if (m_dpv.lfotype)
	{
		int pos;

		if (m_dpv.lfofrac > 0x6fffffff)
			m_dpv.lfofrac = 0;

		// update lfo, lfofrac/255 makes a triangle wave 0-255
		m_dpv.lfofrac += m_dpv.lforate;
		pos = m_dpv.lfofrac >> 8;

		if (m_dpv.lfofrac < 0)
		{
			m_dpv.lfofrac = 0;
			m_dpv.lforate = abs(m_dpv.lforate);
			pos = 0;
		}
		else if (pos > 255)
		{
			pos = 255;
			m_dpv.lfofrac = (255 << 8);
			m_dpv.lforate = -abs(m_dpv.lforate);
		}

		switch(m_dpv.lfotype)
		{
		case LFO_SQUARE:
			if (pos < 128)
				m_dpv.lfomult = 255;
			else
				m_dpv.lfomult = 0;
			
			break;
		case LFO_RANDOM:
			if (pos == 255)
				m_dpv.lfomult = random->RandomInt(0, 255);
			break;
		case LFO_TRIANGLE:
		default: 
			m_dpv.lfomult = pos;
			break;
		}

		if (m_dpv.lfomodpitch)
		{
			prev = pitch;

			// pitch 0-255
			pitch += ((m_dpv.lfomult - 128) * m_dpv.lfomodpitch) / 100;

			if (pitch > 255) pitch = 255;
			if (pitch < 1) pitch = 1;

			
			fChanged |= (prev != pitch);
			flags |= SND_CHANGE_PITCH;
		}

		if (m_dpv.lfomodvol)
		{
			// vol 0-100
			prev = vol;

			vol += ((m_dpv.lfomult - 128) * m_dpv.lfomodvol) / 100;

			if (vol > 100) vol = 100;
			if (vol < 0) vol = 0;
			
			fChanged |= (prev != vol);
			flags |= SND_CHANGE_VOL;
		}

	}

	// Send update to playing sound only if we actually changed
	// pitch or volume in this routine.

	if (flags && fChanged) 
	{
		if (pitch == PITCH_NORM)
			pitch = PITCH_NORM + 1; // don't send 'no pitch' !

		CBaseEntity* pSoundSource = m_hSoundSource;
		if (pSoundSource)
		{
			UTIL_EmitAmbientSound(pSoundSource, pSoundSource->GetAbsOrigin(), szSoundFile, 
					(vol * 0.01), m_iSoundLevel, flags, pitch);
		}
	}

	// update ramps at 5hz
	SetNextThink( gpGlobals->curtime + 0.2 );
	return;
}


//-----------------------------------------------------------------------------
// Purpose: Init all ramp params in preparation to play a new sound.
//-----------------------------------------------------------------------------
void CAmbientGeneric::InitModulationParms(void)
{
	int pitchinc;

	m_dpv.volrun = m_iHealth * 10;	// 0 - 100
	if (m_dpv.volrun > 100) m_dpv.volrun = 100;
	if (m_dpv.volrun < 0) m_dpv.volrun = 0;

	// get presets
	if (m_dpv.preset != 0 && m_dpv.preset <= CDPVPRESETMAX)
	{
		// load preset values
		m_dpv = rgdpvpreset[m_dpv.preset - 1];

		// fixup preset values, just like
		// fixups in KeyValue routine.
		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;
		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;

		m_dpv.volstart *= 10;
		m_dpv.volrun *= 10;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;
		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;

		m_dpv.lforate *= 256;

		m_dpv.fadeinsav = m_dpv.fadein;
		m_dpv.fadeoutsav = m_dpv.fadeout;
		m_dpv.spinupsav = m_dpv.spinup;
		m_dpv.spindownsav = m_dpv.spindown;
	}

	m_dpv.fadein = m_dpv.fadeinsav;
	m_dpv.fadeout = 0; 
	
	if (m_dpv.fadein)
		m_dpv.vol = m_dpv.volstart;
	else
		m_dpv.vol = m_dpv.volrun;

	m_dpv.spinup = m_dpv.spinupsav;
	m_dpv.spindown = 0; 

	if (m_dpv.spinup)
		m_dpv.pitch = m_dpv.pitchstart;
	else
		m_dpv.pitch = m_dpv.pitchrun;

	if (m_dpv.pitch == 0)
		m_dpv.pitch = PITCH_NORM;

	m_dpv.pitchfrac = m_dpv.pitch << 8;
	m_dpv.volfrac = m_dpv.vol << 8;

	m_dpv.lfofrac = 0;
	m_dpv.lforate = abs(m_dpv.lforate);

	m_dpv.cspincount = 1;
	
	if (m_dpv.cspinup) 
	{
		pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;

		m_dpv.pitchrun = m_dpv.pitchstart + pitchinc;
		if (m_dpv.pitchrun > 255) m_dpv.pitchrun = 255;
	}

	if ((m_dpv.spinupsav || m_dpv.spindownsav || (m_dpv.lfotype && m_dpv.lfomodpitch))
		&& (m_dpv.pitch == PITCH_NORM))
		m_dpv.pitch = PITCH_NORM + 1; // must never send 'no pitch' as first pitch
									  // if we intend to pitch shift later!
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that begins playing the sound.
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputPlaySound( inputdata_t &inputdata )
{
	if (!m_fActive)
	{
		ToggleSound();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that stops playing the sound.
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputStopSound( inputdata_t &inputdata )
{
	if (m_fActive)
	{
		ToggleSound();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that stops playing the sound.
//-----------------------------------------------------------------------------
void CAmbientGeneric::InputToggleSound( inputdata_t &inputdata )
{
	ToggleSound();
}


//-----------------------------------------------------------------------------
// Purpose: Turns an ambient sound on or off.  If the ambient is a looping sound,
//			mark sound as active (m_fActive) if it's playing, innactive if not.
//			If the sound is not a looping sound, never mark it as active.
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CAmbientGeneric::ToggleSound()
{
	char *szSoundFile = (char *)STRING( m_iszSound );

	// m_fActive is true only if a looping sound is playing.
	
	if ( m_fActive )
	{// turn sound off

		if (m_dpv.cspinup)
		{
			// Don't actually shut off. Each toggle causes
			// incremental spinup to max pitch

			if (m_dpv.cspincount <= m_dpv.cspinup)
			{	
				int pitchinc;

				// start a new spinup
				m_dpv.cspincount++;
				
				pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;

				m_dpv.spinup = m_dpv.spinupsav;
				m_dpv.spindown = 0;

				m_dpv.pitchrun = m_dpv.pitchstart + pitchinc * m_dpv.cspincount;
				if (m_dpv.pitchrun > 255) m_dpv.pitchrun = 255;

				SetNextThink( gpGlobals->curtime + 0.1f );
			}
			
		}
		else
		{
			m_fActive = false;
			
			// HACKHACK - this makes the code in Precache() work properly after a save/restore
			m_spawnflags |= SF_AMBIENT_SOUND_START_SILENT;

			if (m_dpv.spindownsav || m_dpv.fadeoutsav)
			{
				// spin it down (or fade it) before shutoff if spindown is set
				m_dpv.spindown = m_dpv.spindownsav;
				m_dpv.spinup = 0;

				m_dpv.fadeout = m_dpv.fadeoutsav;
				m_dpv.fadein = 0;
				SetNextThink( gpGlobals->curtime + 0.1f );
			}
			else
			{
				CBaseEntity* pSoundSource = m_hSoundSource;
				if (pSoundSource)
				{
					UTIL_EmitAmbientSound(pSoundSource, pSoundSource->GetAbsOrigin(), szSoundFile, 
						0, SNDLVL_NONE, SND_STOP, 0);
				}
			}
		}
	}
	else 
	{// turn sound on

		// only toggle if this is a looping sound.  If not looping, each
		// trigger will cause the sound to play.  If the sound is still
		// playing from a previous trigger press, it will be shut off
		// and then restarted.

		if (m_fLooping)
			m_fActive = true;
		else
		{
			// shut sound off now - may be interrupting a long non-looping sound
			CBaseEntity* pSoundSource = m_hSoundSource;
			if (pSoundSource)
			{
				UTIL_EmitAmbientSound(pSoundSource, pSoundSource->GetAbsOrigin(), szSoundFile, 
					0, SNDLVL_NONE, SND_STOP, 0);
			}
		}
			
		// init all ramp params for startup

		InitModulationParms();

		CBaseEntity* pSoundSource = m_hSoundSource;
		if (pSoundSource)
		{
			UTIL_EmitAmbientSound(pSoundSource, pSoundSource->GetAbsOrigin(), szSoundFile, 
					(m_dpv.vol * 0.01), m_iSoundLevel, 0, m_dpv.pitch);
		}			
		SetNextThink( gpGlobals->curtime + 0.1f );

	} 
}


// KeyValue - load keyvalue pairs into member data of the
// ambient generic. NOTE: called BEFORE spawn!
bool CAmbientGeneric::KeyValue( const char *szKeyName, const char *szValue )
{
	// NOTE: changing any of the modifiers in this code
	// NOTE: also requires changing InitModulationParms code.

	// preset
	if (FStrEq(szKeyName, "preset"))
	{
		m_dpv.preset = atoi(szValue);
	}
	// pitchrun
	else if (FStrEq(szKeyName, "pitch"))
	{
		m_dpv.pitchrun = atoi(szValue);
		
		if (m_dpv.pitchrun > 255) m_dpv.pitchrun = 255;
		if (m_dpv.pitchrun < 0) m_dpv.pitchrun = 0;
	}		
	// pitchstart
	else if (FStrEq(szKeyName, "pitchstart"))
	{
		m_dpv.pitchstart = atoi(szValue);
		
		if (m_dpv.pitchstart > 255) m_dpv.pitchstart = 255;
		if (m_dpv.pitchstart < 0) m_dpv.pitchstart = 0;
	}
	// spinup
	else if (FStrEq(szKeyName, "spinup"))
	{
		m_dpv.spinup = atoi(szValue);
		
		if (m_dpv.spinup > 100) m_dpv.spinup = 100;
		if (m_dpv.spinup < 0) m_dpv.spinup = 0;

		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;
		m_dpv.spinupsav = m_dpv.spinup;
	}		
	// spindown
	else if (FStrEq(szKeyName, "spindown"))
	{
		m_dpv.spindown = atoi(szValue);
		
		if (m_dpv.spindown > 100) m_dpv.spindown = 100;
		if (m_dpv.spindown < 0) m_dpv.spindown = 0;

		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;
		m_dpv.spindownsav = m_dpv.spindown;
	}
	// volstart
	else if (FStrEq(szKeyName, "volstart"))
	{
		m_dpv.volstart = atoi(szValue);

		if (m_dpv.volstart > 10) m_dpv.volstart = 10;
		if (m_dpv.volstart < 0) m_dpv.volstart = 0;
		
		m_dpv.volstart *= 10;	// 0 - 100
	}
	// fadein
	else if (FStrEq(szKeyName, "fadein"))
	{
		m_dpv.fadein = atoi(szValue);
		
		if (m_dpv.fadein > 100) m_dpv.fadein = 100;
		if (m_dpv.fadein < 0) m_dpv.fadein = 0;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;
		m_dpv.fadeinsav = m_dpv.fadein;
	}
	// fadeout
	else if (FStrEq(szKeyName, "fadeout"))
	{
		m_dpv.fadeout = atoi(szValue);
		
		if (m_dpv.fadeout > 100) m_dpv.fadeout = 100;
		if (m_dpv.fadeout < 0) m_dpv.fadeout = 0;

		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;
		m_dpv.fadeoutsav = m_dpv.fadeout;
	}
	// lfotype
	else if (FStrEq(szKeyName, "lfotype"))
	{
		m_dpv.lfotype = atoi(szValue);
		if (m_dpv.lfotype > 4) m_dpv.lfotype = LFO_TRIANGLE;
	}
	// lforate
	else if (FStrEq(szKeyName, "lforate"))
	{
		m_dpv.lforate = atoi(szValue);
		
		if (m_dpv.lforate > 1000) m_dpv.lforate = 1000;
		if (m_dpv.lforate < 0) m_dpv.lforate = 0;

		m_dpv.lforate *= 256;
	}
	// lfomodpitch
	else if (FStrEq(szKeyName, "lfomodpitch"))
	{
		m_dpv.lfomodpitch = atoi(szValue);
		if (m_dpv.lfomodpitch > 100) m_dpv.lfomodpitch = 100;
		if (m_dpv.lfomodpitch < 0) m_dpv.lfomodpitch = 0;
	}

	// lfomodvol
	else if (FStrEq(szKeyName, "lfomodvol"))
	{
		m_dpv.lfomodvol = atoi(szValue);
		if (m_dpv.lfomodvol > 100) m_dpv.lfomodvol = 100;
		if (m_dpv.lfomodvol < 0) m_dpv.lfomodvol = 0;
	}
	// cspinup
	else if (FStrEq(szKeyName, "cspinup"))
	{
		m_dpv.cspinup = atoi(szValue);
		if (m_dpv.cspinup > 100) m_dpv.cspinup = 100;
		if (m_dpv.cspinup < 0) m_dpv.cspinup = 0;
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}


// =================== ROOM SOUND FX ==========================================
#include "igamesystem.h"
#include <KeyValues.h>
#include "filesystem.h"

#define SOUNDSCAPE_MANIFEST_FILE				"scripts/soundscapes_manifest.txt"

class CSoundscapeSystem : public CAutoGameSystem
{
public:
	virtual void AddSoundScapeFile( const char *filename )
	{
		// Open the soundscape data file, and abort if we can't
		KeyValues *pKeyValuesData = new KeyValues( filename );
		if ( pKeyValuesData->LoadFromFile( filesystem, filename ) )
		{
			// parse out all of the top level sections and save their names
			KeyValues *pKeys = pKeyValuesData;
			while ( pKeys )
			{
				if ( pKeys->GetFirstSubKey() )
				{
					m_soundscapes.AddString( pKeys->GetName(), m_soundscapeCount );
					m_soundscapeCount++;
				}
				pKeys = pKeys->GetNextKey();
			}
		}
		pKeyValuesData->deleteThis();
	}

	virtual bool Init()
	{
		m_soundscapeCount = 0;

		KeyValues *manifest = new KeyValues( SOUNDSCAPE_MANIFEST_FILE );
		if ( manifest->LoadFromFile( filesystem, SOUNDSCAPE_MANIFEST_FILE, "GAME" ) )
		{
			for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
			{
				if ( !Q_stricmp( sub->GetName(), "file" ) )
				{
					// Add
					AddSoundScapeFile( sub->GetString() );
					continue;
				}

				Warning( "CSoundscapeSystem::Init:  Manifest '%s' with bogus file type '%s', expecting 'file'\n", 
					SOUNDSCAPE_MANIFEST_FILE, sub->GetName() );
			}
		}
		else
		{
			Error( "Unable to load manifest file '%s'\n", SOUNDSCAPE_MANIFEST_FILE );
		}

		return true;
	}

	int	GetSoundscapeIndex( const char *pName )
	{
		return m_soundscapes.GetStringID( pName );
	}

	bool IsValidIndex( int index )
	{
		if ( index >= 0 && index < m_soundscapeCount )
			return true;
		return false;
	}

private:
	CStringRegistry		m_soundscapes;
	int					m_soundscapeCount;
};

static CSoundscapeSystem g_SoundscapeSystem;


LINK_ENTITY_TO_CLASS( env_soundscape, CEnvSoundscape );

BEGIN_DATADESC( CEnvSoundscape )

	DEFINE_KEYFIELD( CEnvSoundscape, m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_FIELD( CEnvSoundscape, m_flRange, FIELD_FLOAT ),
	// don't save, recomputed on load
	//DEFINE_FIELD( CEnvSoundscape, m_soundscapeIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvSoundscape, m_soundscapeName, FIELD_STRING ),

// Silence, Classcheck!
//	DEFINE_ARRAY( CEnvSoundscape, m_positionNames, FIELD_STRING, 4 ),

	DEFINE_KEYFIELD( CEnvSoundscape, m_positionNames[0], FIELD_STRING, "position0" ),
	DEFINE_KEYFIELD( CEnvSoundscape, m_positionNames[1], FIELD_STRING, "position1" ),
	DEFINE_KEYFIELD( CEnvSoundscape, m_positionNames[2], FIELD_STRING, "position2" ),
	DEFINE_KEYFIELD( CEnvSoundscape, m_positionNames[3], FIELD_STRING, "position3" ),

END_DATADESC()

CEnvSoundscape::CEnvSoundscape()
{
	m_soundscapeIndex = -1;
}

bool CEnvSoundscape::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "soundscape"))
	{
		m_soundscapeName = AllocPooledString( szValue );
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}

// returns true if the given sound entity is in range 
// and can see the given player entity (pTarget)

bool CEnvSoundscape::InRangeOfPlayer( CBasePlayer *pTarget ) 
{
	Vector vecSpot1 = EarPosition();
	Vector vecSpot2 = pTarget->EarPosition();

	// calc range from sound entity to player
	Vector vecRange = vecSpot2 - vecSpot1;
	float range = vecRange.Length();
	if ( m_flRadius > range )
	{
		trace_t tr;

		UTIL_TraceLine( vecSpot1, vecSpot2, MASK_SOLID_BRUSHONLY|MASK_WATER, pTarget, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1 && !tr.startsolid )
		{
			m_flRange = range;
			return true;
		}
	}

	m_flRange = 0;
	return false;
}


void CEnvSoundscape::WriteAudioParamsTo( audioparams_t &audio )
{
	audio.ent.Set( this );
	audio.soundscapeIndex = m_soundscapeIndex;
	audio.localBits = 0;
	for ( int i = 0; i < ARRAYSIZE(m_positionNames); i++ )
	{
		if ( m_positionNames[i] != NULL_STRING )
		{
			CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, m_positionNames[i], this );
			if ( pEntity )
			{
				audio.localBits |= 1<<i;
				audio.localSound.Set( i, pEntity->GetAbsOrigin() );
			}
		}
	}
}

//
// A client that is visible and in range of a sound entity will
// have its soundscape set by that sound entity.  If two or more
// sound entities are contending for a client, then the nearest
// sound entity to the client will set the client's soundscape.
// A client's soundscape will remain set to its prior value until
// a new in-range, visible sound entity resets a new soundscape.
//

// CONSIDER: if player in water state, autoset and underwater soundscape? 
void CEnvSoundscape::Think( void )
{
	// get pointer to client if visible; UTIL_FindClientInPVS will
	// cycle through visible clients on consecutive calls.

	CBasePlayer *pPlayer = ToBasePlayer( GetContainingEntity( UTIL_FindClientInPVS(edict()) ) );
	if ( !pPlayer )
	{
		SetNextThink( gpGlobals->curtime + 1.0f );
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.25 );
	// check to see if this is the sound entity that is 
	// currently affecting this player

	audioparams_t &audio = pPlayer->GetAudioParams();

	CEnvSoundscape *pCurrent = (CEnvSoundscape *)audio.ent.Get();
	if ( pCurrent == this ) 
	{
		// this is the entity currently affecting player, check
		// for validity

		// we're looking at a valid sound entity affecting
		// player, make sure it's still valid, update range
		if ( !InRangeOfPlayer( pPlayer ) ) 
		{
			// current sound entity affecting player is no longer valid,
			// flag this state by clearing range.
			m_flRange = 0;
			// think less often
			SetNextThink( gpGlobals->curtime + 0.5f );
		}
	}
	// if we got this far, we're looking at an entity that is contending
	// for current player sound. the closest entity to player wins.
	else if ( InRangeOfPlayer( pPlayer ) )
	{
		if ( !pCurrent || pCurrent->m_flRange == 0 || pCurrent->m_flRange > m_flRange ) 
		{
			// new entity is closer to player, so it wins.
			WriteAudioParamsTo( audio );
		}
	} 
}

//
// env_soundscape - spawn a sound entity that will set player soundscape
// when player moves in range and sight.
//
//
void CEnvSoundscape::Spawn( )
{
	Precache();
	// spread think times
	SetNextThink( gpGlobals->curtime + random->RandomFloat(0.0, 0.5) ); 

	// Because the soundscape has no model, need to make sure it doesn't get culled from the PVS for this reason and therefore
	//  never exist on the client, etc.
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

void CEnvSoundscape::Precache()
{
	m_soundscapeIndex = g_SoundscapeSystem.GetSoundscapeIndex( STRING(m_soundscapeName) );
	if ( !g_SoundscapeSystem.IsValidIndex(m_soundscapeIndex) )
	{
		DevMsg("Can't find soundscape: %s\n", STRING(m_soundscapeName) );
	}
}

void CEnvSoundscape::DrawDebugGeometryOverlays( void )
{
	if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
		if ( pPlayer )
		{
			audioparams_t &audio = pPlayer->GetAudioParams();
			if ( audio.ent.Get() == this )
			{
				NDebugOverlay::Line(GetAbsOrigin(), pPlayer->WorldSpaceCenter(), (m_flRange != 0) ? 255 : 0, 0, 255, false, 0 );
			}
		}
	}
	BaseClass::DrawDebugGeometryOverlays();
}


// ==================== SENTENCE GROUPS, UTILITY FUNCTIONS  ======================================

int fSentencesInit = false;

// ===================== SENTENCE GROUPS, MAIN ROUTINES ========================

// given sentence group index, play random sentence for given entity.
// returns sentenceIndex - which sentence was picked 
// Ipick is only needed if you plan on stopping the sound before playback is done (see SENTENCEG_Stop). 
// sentenceIndex can be used to find the name/length of the sentence

int SENTENCEG_PlayRndI(edict_t *entity, int isentenceg, 
					  float volume, soundlevel_t soundlevel, int flags, int pitch)
{
	char name[64];
	int ipick;

	if (!fSentencesInit)
		return -1;

	name[0] = 0;

	ipick = engine->SentenceGroupPick( isentenceg, name, sizeof( name ) );
	if (ipick > 0 && name)
	{
		int sentenceIndex = SENTENCEG_Lookup( name );
		CPASAttenuationFilter filter( GetContainingEntity( entity ), soundlevel );
		enginesound->EmitSentenceByIndex( filter, ENTINDEX(entity), CHAN_VOICE, sentenceIndex, volume, soundlevel, flags, pitch );
		return sentenceIndex;
	}

	return -1;
}

// same as above, but takes sentence group name instead of index

int SENTENCEG_PlayRndSz(edict_t *entity, const char *szgroupname, 
					  float volume, soundlevel_t soundlevel, int flags, int pitch)
{
	char name[64];
	int ipick;
	int isentenceg;

	if (!fSentencesInit)
		return -1;

	name[0] = 0;

	isentenceg = engine->SentenceGroupIndexFromName(szgroupname);
	if (isentenceg < 0)
	{
		Warning( "No such sentence group %s\n", szgroupname );
		return -1;
	}

	ipick = engine->SentenceGroupPick(isentenceg, name, sizeof( name ));
	if (ipick >= 0 && name[0])
	{
		int sentenceIndex = SENTENCEG_Lookup( name );
		CPASAttenuationFilter filter( GetContainingEntity( entity ), soundlevel );
		enginesound->EmitSentenceByIndex( filter, ENTINDEX(entity), CHAN_VOICE, sentenceIndex, volume, soundlevel, flags, pitch );
		return sentenceIndex;
	}

	return -1;
}

// play sentences in sequential order from sentence group.  Reset after last sentence.

int SENTENCEG_PlaySequentialSz(edict_t *entity, const char *szgroupname, 
					  float volume, soundlevel_t soundlevel, int flags, int pitch, int ipick, int freset)
{
	char name[64];
	int ipicknext;
	int isentenceg;

	if (!fSentencesInit)
		return -1;

	name[0] = 0;

	isentenceg = engine->SentenceGroupIndexFromName(szgroupname);
	if (isentenceg < 0)
		return -1;

	ipicknext = engine->SentenceGroupPickSequential(isentenceg, name, sizeof( name ), ipick, freset);
	if (ipicknext >= 0 && name[0])
	{
		int sentenceIndex = SENTENCEG_Lookup( name );
		CPASAttenuationFilter filter( GetContainingEntity( entity ), soundlevel );
		enginesound->EmitSentenceByIndex( filter, ENTINDEX(entity), CHAN_VOICE, sentenceIndex, volume, soundlevel, flags, pitch );
		return sentenceIndex;
	}
	
	return -1;
}


#if 0
// for this entity, for the given sentence within the sentence group, stop
// the sentence.

void SENTENCEG_Stop(edict_t *entity, int isentenceg, int ipick)
{
	char buffer[64];
	char sznum[8];
	
	if (!fSentencesInit)
		return;

	if (isentenceg < 0 || ipick < 0)
		return;

	Q_snprintf(buffer,sizeof(buffer),"!%s%d", engine->SentenceGroupNameFromIndex( isentenceg ), ipick );

	UTIL_StopSound(entity, CHAN_VOICE, buffer);
}
#endif

// open sentences.txt, scan for groups, build rgsentenceg
// Should be called from world spawn, only works on the
// first call and is ignored subsequently.

void SENTENCEG_Init()
{
	if (fSentencesInit)
		return;

	engine->PrecacheSentenceFile( "scripts/sentences.txt" );
	fSentencesInit = true;
}

// convert sentence (sample) name to !sentencenum, return !sentencenum

int SENTENCEG_Lookup(const char *sample)
{
	return engine->SentenceIndexFromName( sample + 1 );
}


int SENTENCEG_GetIndex(const char *szrootname)
{
	return engine->SentenceGroupIndexFromName( szrootname );
}


// play a specific sentence over the HEV suit speaker - just pass player entity, and !sentencename

void UTIL_EmitSoundSuit(edict_t *entity, const char *sample)
{
	float fvol;
	int pitch = PITCH_NORM;

	fvol = suitvolume.GetFloat();
	if (random->RandomInt(0,1))
		pitch = random->RandomInt(0,6) + 98;

	if (fvol > 0.05)
	{
		CPASAttenuationFilter filter( GetContainingEntity( entity ) );
		filter.MakeReliable();
		CBaseEntity::EmitSound( filter, ENTINDEX(entity), CHAN_STATIC, sample, fvol, ATTN_NORM, 0, pitch );
	}
}

// play a sentence, randomly selected from the passed in group id, over the HEV suit speaker

int UTIL_EmitGroupIDSuit(edict_t *entity, int isentenceg)
{
	float fvol;
	int pitch = PITCH_NORM;
	int sentenceIndex = -1;

	fvol = suitvolume.GetFloat();
	if (random->RandomInt(0,1))
		pitch = random->RandomInt(0,6) + 98;

	if (fvol > 0.05)
		sentenceIndex = SENTENCEG_PlayRndI(entity, isentenceg, fvol, SNDLVL_NORM, 0, pitch);

	return sentenceIndex;
}

// play a sentence, randomly selected from the passed in groupname

int UTIL_EmitGroupnameSuit(edict_t *entity, const char *groupname)
{
	float fvol;
	int pitch = PITCH_NORM;
	int sentenceIndex = -1;

	fvol = suitvolume.GetFloat();
	if (random->RandomInt(0,1))
		pitch = random->RandomInt(0,6) + 98;

	if (fvol > 0.05)
		sentenceIndex = SENTENCEG_PlayRndSz(entity, groupname, fvol, SNDLVL_NORM, 0, pitch);

	return sentenceIndex;
}

// ===================== MATERIAL TYPE DETECTION, MAIN ROUTINES ========================
// 
// Used to detect the texture the player is standing on, map the
// texture name to a material type.  Play footstep sound based
// on material type.

char TEXTURETYPE_Find( trace_t *ptr )
{
	surfacedata_t *psurfaceData = physprops->GetSurfaceData( ptr->surface.surfaceProps );

	return psurfaceData->gameMaterial;
}
