//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include <KeyValues.h>
#include "filesystem.h"
#include "utldict.h"
#include "interval.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystemBase.h"
#include "utlbuffer.h"

#define MANIFEST_FILE				"scripts/game_sounds_manifest.txt"
#define GAME_SOUNDS_HEADER_BLOCK	"scripts/game_sounds_header.txt"

//=============================================================================

// HACK:  These should match sound.h
#define CHAR_STREAM			'*'		// as one of 1st 2 chars in name, indicates streaming wav data
#define CHAR_USERVOX		'?'		// as one of 1st 2 chars in name, indicates user realtime voice data
#define CHAR_SENTENCE		'!'		// as one of 1st 2 chars in name, indicates sentence wav
#define CHAR_DRYMIX			'#'		// as one of 1st 2 chars in name, indicates wav bypasses dsp fx
#define CHAR_DOPPLER		'>'		// as one of 1st 2 chars in name, indicates doppler encoded stereo wav
#define CHAR_DIRECTIONAL	'<'		// as one of 1st 2 chars in name, indicates mono or stereo wav has direction cone
#define CHAR_DISTVARIANT	'^'		// as one of 1st 2 chars in name, indicates distance variant encoded stereo wav
#define CHAR_OMNI			'@'		// as one of 1st 2 chars in name, indicates non-directional wav (default mono or stereo)


static inline bool IsSoundChar(char c)
{
	bool b;

	b = (c == CHAR_STREAM || c == CHAR_USERVOX || c == CHAR_SENTENCE || c == CHAR_DRYMIX);
	b = b || (c == CHAR_DOPPLER || c == CHAR_DIRECTIONAL || c == CHAR_DISTVARIANT );

	return b;
}

// return pointer to first valid character in file name
// by skipping over CHAR_STREAM...CHAR_DRYMIX

static char *PSkipSoundChars(const char *pch)
{
	int i;
	char *pcht = (char *)pch;

	// check first 2 characters

	for (i = 0; i < 2; i++)
	{
		if (!IsSoundChar(*pcht))
			break;
		pcht++;
	}

	return pcht;
}

struct SoundChannels
{
	int			channel;
	const char *name;
};

// NOTE:  This will need to be updated if channel names are added/removed
static SoundChannels g_pChannelNames[] =
{
	{ CHAN_AUTO, "CHAN_AUTO" },
	{ CHAN_WEAPON, "CHAN_WEAPON" },
	{ CHAN_VOICE, "CHAN_VOICE" },
	{ CHAN_ITEM, "CHAN_ITEM" },
	{ CHAN_BODY, "CHAN_BODY" },
	{ CHAN_STREAM, "CHAN_STREAM" },
	{ CHAN_STATIC, "CHAN_STATIC" },
};

struct VolumeLevel
{
	float		volume;
	const char *name;
};

static VolumeLevel g_pVolumeLevels[] = 
{
	{ VOL_NORM, "VOL_NORM" },
};

struct PitchLookup
{
	float		pitch;
	const char *name;
};

static PitchLookup g_pPitchLookup[] =
{
	{ PITCH_NORM,	"PITCH_NORM" },
	{ PITCH_LOW,	"PITCH_LOW" },
	{ PITCH_HIGH,	"PITCH_HIGH" },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct SoundLevelLookup
{
	soundlevel_t	level;
	char const		*name;
};

// NOTE:  Needs to reflect the soundlevel_t enum defined in soundflags.h
static SoundLevelLookup g_pSoundLevels[] =
{
	{ SNDLVL_NONE, "SNDLVL_NONE" },
	{ SNDLVL_25dB, "SNDLVL_25dB" },
	{ SNDLVL_30dB, "SNDLVL_30dB" },
	{ SNDLVL_35dB, "SNDLVL_35dB" },
	{ SNDLVL_40dB, "SNDLVL_40dB" },
	{ SNDLVL_45dB, "SNDLVL_45dB" },
	{ SNDLVL_50dB, "SNDLVL_50dB" },
	{ SNDLVL_55dB, "SNDLVL_55dB" },
	{ SNDLVL_IDLE, "SNDLVL_IDLE" },
	{ SNDLVL_TALKING, "SNDLVL_TALKING" },
	{ SNDLVL_60dB, "SNDLVL_60dB" },
	{ SNDLVL_65dB, "SNDLVL_65dB" },
	{ SNDLVL_STATIC, "SNDLVL_STATIC" },
	{ SNDLVL_70dB, "SNDLVL_70dB" },
	{ SNDLVL_NORM, "SNDLVL_NORM" },
	{ SNDLVL_75dB, "SNDLVL_75dB" },
	{ SNDLVL_80dB, "SNDLVL_80dB" },
	{ SNDLVL_85dB, "SNDLVL_85dB" },
	{ SNDLVL_90dB, "SNDLVL_90dB" },
	{ SNDLVL_95dB, "SNDLVL_95dB" },
	{ SNDLVL_100dB, "SNDLVL_100dB" },
	{ SNDLVL_105dB, "SNDLVL_105dB" },
	{ SNDLVL_110dB, "SNDLVL_110dB" },
	{ SNDLVL_120dB, "SNDLVL_120dB" },
	{ SNDLVL_130dB, "SNDLVL_130dB" },
	{ SNDLVL_GUNFIRE, "SNDLVL_GUNFIRE" },
	{ SNDLVL_140dB, "SNDLVL_140dB" },
	{ SNDLVL_150dB, "SNDLVL_150dB" },
};

static const char *_SoundLevelToString( soundlevel_t level )
{
	int c = ARRAYSIZE( g_pSoundLevels );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		SoundLevelLookup *entry = &g_pSoundLevels[ i ];
		if ( entry->level == level )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i", (int)level );
	return sz;
}

static const char *_ChannelToString( int channel )
{
	int c = ARRAYSIZE( g_pChannelNames );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		SoundChannels *entry = &g_pChannelNames[ i ];
		if ( entry->channel == channel )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i", (int)channel );
	return sz;
}

static const char *_VolumeToString( float volume )
{
	int c = ARRAYSIZE( g_pVolumeLevels );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		VolumeLevel *entry = &g_pVolumeLevels[ i ];
		if ( entry->volume == volume )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%.3f", volume );
	return sz;
}

static const char *_PitchToString( float pitch )
{
	int c = ARRAYSIZE( g_pPitchLookup );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		PitchLookup *entry = &g_pPitchLookup[ i ];
		if ( entry->pitch == pitch )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%.3f", pitch );
	return sz;
}

//-----------------------------------------------------------------------------
// Purpose: Convert "chan_xxx" into integer value for channel
// Input  : *name - 
// Output : static int
//-----------------------------------------------------------------------------
static int TextToChannel( const char *name )
{
	if ( !name )
	{
		Assert( 0 );
		// CHAN_AUTO
		return CHAN_AUTO;
	}

	if ( Q_strncasecmp( name, "chan_", strlen( "chan_" ) ) )
	{
		return atoi( name );
	}

	int c = ARRAYSIZE( g_pChannelNames );
	int i;

	for ( i = 0; i < c; i++ )
	{
		if ( !Q_strcasecmp( name, g_pChannelNames[ i ].name ) )
		{
			return g_pChannelNames[ i ].channel;
		}
	}

	// At this point, it starts with chan_ but is not recognized
	// atoi would return 0, so just do chan auto
	DevMsg( "CSoundEmitterSystem:  Warning, unknown channel type in sounds.txt (%s)\n", name );

	return CHAN_AUTO;
}

const char *CSoundEmitterSystemBase::CSoundParametersInternal::VolumeToString( void )
{
	if ( volume.range == 0.0f )
	{
		return _VolumeToString( volume.start );
	}

	static char sz[ 64 ];
	Q_snprintf( sz, sizeof( sz ),  "%.3f, %.3f", volume.start, volume.start + volume.range );
	return sz;
}

const char *CSoundEmitterSystemBase::CSoundParametersInternal::ChannelToString( void )
{
	return _ChannelToString( channel );
}

const char *CSoundEmitterSystemBase::CSoundParametersInternal::SoundLevelToString( void )
{
	if ( soundlevel.range == 0.0f )
	{
		return _SoundLevelToString( (soundlevel_t)(int)soundlevel.start );
	}

	static char sz[ 64 ];
	Q_snprintf( sz, sizeof( sz ),  "%i, %i", (soundlevel_t)(int)soundlevel.start, (soundlevel_t)(int)(soundlevel.start + soundlevel.range ) );
	return sz;
}

const char *CSoundEmitterSystemBase::CSoundParametersInternal::PitchToString( void )
{
	if ( pitch.range == 0.0f )
	{
		return _PitchToString( (int)pitch.start );
	}

	static char sz[ 64 ];
	Q_snprintf( sz, sizeof( sz ),  "%i, %i", (int)pitch.start, (int)(pitch.start + pitch.range ) );
	return sz;
}

void CSoundEmitterSystemBase::CSoundParametersInternal::VolumeFromString( const char *sz )
{
	if ( !Q_strcasecmp( sz, "VOL_NORM" ) )
	{
		volume.start = VOL_NORM;
		volume.range = 0.0f;
	}
	else
	{
		volume = ReadInterval( sz );
	}

	Q_strncpy( m_szVolume, sz, sizeof( m_szVolume ) );
}

void CSoundEmitterSystemBase::CSoundParametersInternal::ChannelFromString( const char *sz )
{
	channel = TextToChannel( sz );

	Q_strncpy( m_szChannel, sz, sizeof( m_szChannel ) );
}

void CSoundEmitterSystemBase::CSoundParametersInternal::PitchFromString( const char *sz )
{
	if ( !Q_strcasecmp( sz, "PITCH_NORM" ) )
	{
		pitch.start	= PITCH_NORM;
		pitch.range = 0.0f;
	}
	else if ( !Q_strcasecmp( sz, "PITCH_LOW" ) )
	{
		pitch.start	= PITCH_LOW;
		pitch.range = 0.0f;
	}
	else if ( !Q_strcasecmp( sz, "PITCH_HIGH" ) )
	{
		pitch.start	= PITCH_HIGH;
		pitch.range = 0.0f;
	}
	else
	{
		pitch= ReadInterval( sz ) ;
	}

	Q_strncpy( m_szPitch, sz, sizeof( m_szPitch ) );
}

void CSoundEmitterSystemBase::CSoundParametersInternal::SoundLevelFromString( const char *sz )
{
	if ( !Q_strncasecmp( sz, "SNDLVL_", strlen( "SNDLVL_" ) ) )
	{
		soundlevel.start = TextToSoundLevel( sz );
		soundlevel.range = 0.0f;
	}
	else
	{
		soundlevel = ReadInterval( sz );
	}

	Q_strncpy( m_szSoundLevel, sz, sizeof( m_szSoundLevel ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int	
//-----------------------------------------------------------------------------
int	 CSoundEmitterSystemBase::First() const
{
	return m_Sounds.First();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : i - 
// Output : int
//-----------------------------------------------------------------------------
int CSoundEmitterSystemBase::Next( int i ) const
{
	return m_Sounds.Next( i );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CSoundEmitterSystemBase::InvalidIndex() const
{
	return m_Sounds.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scriptfile - 
//-----------------------------------------------------------------------------
void CSoundEmitterSystemBase::BuildPrecacheSoundList( const char *scriptfile, CUtlVector< int >& list )
{
	int i, c;

	list.RemoveAll();

	int scriptindex = FindSoundScript( scriptfile );
	if ( scriptindex == m_SoundKeyValues.InvalidIndex() )
	{
		Warning( "Can't PrecacheSoundScript( '%s' ), file not in manifest '%s'\n",
			scriptfile, MANIFEST_FILE );
		return;
	}

	// Walk sounds adding appropriate indices to list
	c = GetSoundCount();
	for ( i = 0 ; i < c; i++ )
	{
		CSoundEntry *se = &m_Sounds[ i ];
		Assert( se );
		if ( se->m_nScriptFileIndex == scriptindex )
		{
			list.AddToTail( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSoundEmitterSystemBase::BaseInit()
{
	if ( m_SoundKeyValues.Count() > 0 )
	{
		BaseShutdown();
	}

	KeyValues *manifest = new KeyValues( MANIFEST_FILE );
	if ( manifest->LoadFromFile( filesystem, MANIFEST_FILE, "GAME" ) )
	{
		for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
		{
			if ( !Q_stricmp( sub->GetName(), "precache_file" ) )
			{
				// Add and always precache
				AddSoundsFromFile( sub->GetString(), true );
				continue;
			}
			else if ( !Q_stricmp( sub->GetName(), "declare_file" ) )
			{
				// Add but don't precache
				AddSoundsFromFile( sub->GetString(), false );
				continue;
			}

			Warning( "CSoundEmitterSystemBase::BaseInit:  Manifest '%s' with bogus file type '%s', expecting 'declare_file' or 'precache_file'\n", 
				MANIFEST_FILE, sub->GetName() );
		}
	}
	else
	{
		Error( "Unable to load manifest file '%s'\n", MANIFEST_FILE );
	}


// Only print total once, on server
#if !defined( CLIENT_DLL )
	int missing = CheckForMissingWavFiles( false );

	if ( missing > 0 )
	{
		DevMsg( 1, "CSoundEmitterSystem:  Registered %i sounds ( %i missing .wav files referenced )\n",
			m_Sounds.Count(), missing );
	}
	else
	{
		DevMsg( 1, "CSoundEmitterSystem:  Registered %i sounds\n",
			m_Sounds.Count() );
	}
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSoundEmitterSystemBase::BaseShutdown()
{
	m_SoundKeyValues.RemoveAll();
	m_Sounds.Purge();
	m_Waves.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pName - 
//-----------------------------------------------------------------------------
int	CSoundEmitterSystemBase::GetSoundIndex( const char *pName ) const
{
	int idx = m_Sounds.Find( pName );
	if ( idx == m_Sounds.InvalidIndex() )
		return -1;

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSoundEmitterSystemBase::IsValidIndex( int index )
{
	if ( index >= 0 && index < (int)m_Sounds.Count() )
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CSoundEmitterSystemBase::GetSoundName( int index )
{
	if ( !IsValidIndex( index ) )
		return "";

	return m_Sounds.GetElementName( index );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CSoundEmitterSystemBase::GetSoundCount( void )
{
	return m_Sounds.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *soundname - 
//			params - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSoundEmitterSystemBase::GetParametersForSound( const char *soundname, CSoundParameters& params )
{
	int index = GetSoundIndex( soundname );
	if ( index == -1 )
	{
		DevMsg( "CSoundEmitterSystemBase::GetParametersForSound:  No such sound %s\n", soundname );
		return false;
	}

	CSoundParametersInternal *internal = InternalGetParametersForSound( index );
	if ( !internal )
	{
		Assert( 0 );
		DevMsg( "CSoundEmitterSystemBase::GetParametersForSound:  No such sound %s\n", soundname );
		return false;
	}

	params.channel = internal->channel;
	params.volume = RandomInterval( internal->volume );
	params.pitch = RandomInterval( internal->pitch );
	params.pitchlow = internal->pitch.start;
	params.pitchhigh = params.pitchlow + internal->pitch.range;
	params.count = internal->soundnames.Count();
	params.soundname[ 0 ] = 0;
	if ( params.count >= 1 )
	{
		CUtlSymbol sym = internal->soundnames[ random->RandomInt( 0, params.count - 1 ) ];

		Q_strncpy( params.soundname, m_Waves.String( sym ), sizeof( params.soundname ) );
	}

	params.soundlevel = (soundlevel_t)(int)RandomInterval( internal->soundlevel );
	params.play_to_owner_only = internal->play_to_owner_only;

	if ( !params.soundname[ 0 ] )
	{
		DevMsg( "CSoundEmitterSystemBase::GetParametersForSound:  sound %s has no wave or rndwave key!\n", soundname );
		return false;
	}

	if ( internal->had_missing_wave_files &&
		params.soundname[ 0 ] != CHAR_SENTENCE )
	{
		char testfile[ 256 ];
		Q_snprintf( testfile, sizeof( testfile ), "sound/%s", PSkipSoundChars( params.soundname ) );

		if ( !filesystem->FileExists( testfile ) )
		{
			DevMsg( "CSoundEmitterSystemBase::GetParametersForSound:  sound '%s' references wave '%s' which doesn't exist on disk!\n", 
				soundname,
				params.soundname );
			return false;
		}
	}

	return true;
}

CSoundEmitterSystemBase::CSoundParametersInternal *CSoundEmitterSystemBase::InternalGetParametersForSound( int index )
{
	if ( index < 0 || index >= (int)m_Sounds.Count() )
	{
		Assert( !"CSoundEmitterSystemBase::InternalGetParametersForSound:  Bogus index" );
		return NULL;
	}

	return &m_Sounds[ index ].m_SoundParams;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *soundname - 
//			params - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSoundEmitterSystemBase::InitSoundInternalParameters( const char *soundname, KeyValues *kv, CSoundParametersInternal& params )
{
	KeyValues *pKey = kv->GetFirstSubKey();
	while ( pKey )
	{
		if ( !Q_strcasecmp( pKey->GetName(), "channel" ) )
		{
			params.ChannelFromString( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "volume" ) )
		{
			params.VolumeFromString( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "pitch" ) )
		{
			params.PitchFromString( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "wave" ) )
		{
			CUtlSymbol sym = m_Waves.AddString( pKey->GetString() );
			params.soundnames.AddToTail( sym );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "rndwave" ) )
		{
			KeyValues *pWaves = pKey->GetFirstSubKey();
			while ( pWaves )
			{
				CUtlSymbol sym = m_Waves.AddString( pWaves->GetString() );
				params.soundnames.AddToTail( sym );

				pWaves = pWaves->GetNextKey();
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "attenuation" ) )
		{
			if ( !Q_strncasecmp( pKey->GetString(), "SNDLVL_", strlen( "SNDLVL_" ) ) )
			{
				DevMsg( "CSoundEmitterSystemBase::GetParametersForSound:  sound %s has \"attenuation\" with %s value!\n",
					soundname, pKey->GetString() );
			}

			if ( !Q_strncasecmp( pKey->GetString(), "ATTN_", strlen( "ATTN_" ) ) )
			{
				params.soundlevel.start = ATTN_TO_SNDLVL( TranslateAttenuation( pKey->GetString() ) );
				params.soundlevel.range = 0.0f;
			}
			else
			{
				interval_t interval;
				interval = ReadInterval( pKey->GetString() );

				// Translate from attenuation to soundlevel
				float start = interval.start;
				float end	= interval.start + interval.range;

				params.soundlevel.start = ATTN_TO_SNDLVL( start );
				params.soundlevel.range = ATTN_TO_SNDLVL( end ) - ATTN_TO_SNDLVL( start );
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "soundlevel" ) )
		{
			if ( !Q_strncasecmp( pKey->GetString(), "ATTN_", strlen( "ATTN_" ) ) )
			{
				DevMsg( "CSoundEmitterSystemBase::GetParametersForSound:  sound %s has \"soundlevel\" with %s value!\n",
					soundname, pKey->GetString() );
			}

			params.SoundLevelFromString( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "play_to_owner_only" ) )
		{
			params.play_to_owner_only = pKey->GetInt() ? true : false;
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "precache" ) )
		{
			params.precache = pKey->GetInt() ? true : false;
		}

		pKey = pKey->GetNextKey();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *soundname - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CSoundEmitterSystemBase::GetWavFileForSound( const char *soundname )
{
	CSoundParameters params;
	if ( !GetParametersForSound( soundname, params ) )
	{
		return soundname;
	}

	if ( !params.soundname[ 0 ] )
	{
		return soundname;
	}

	static char outsound[ 512 ];
	Q_strncpy( outsound, params.soundname, sizeof( outsound ) );
	return outsound;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *soundname - 
// Output : soundlevel_t
//-----------------------------------------------------------------------------
soundlevel_t CSoundEmitterSystemBase::LookupSoundLevel( const char *soundname )
{
	CSoundParameters params;
	if ( !GetParametersForSound( soundname, params ) )
	{
		return SNDLVL_NORM;
	}

	return params.soundlevel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSoundEmitterSystemBase::GetShouldAlwaysPrecacheSound( int index )
{
	if ( !IsValidIndex( index ) )
		return false;

	CSoundEntry const *entry = &m_Sounds[ index ];
	return entry->m_bPrecacheAlways;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void CSoundEmitterSystemBase::AddSoundsFromFile( const char *filename, bool precachealways )
{
	CSoundScriptFile sf;
	Assert( Q_strlen( filename ) < sizeof( sf.filename ) );
	Q_strncpy( sf.filename, filename, sizeof( sf.filename ) );
	sf.dirty = false;
	sf.precachealways = precachealways;

	int scriptindex = m_SoundKeyValues.AddToTail( sf );

	// Open the soundscape data file, and abort if we can't
	KeyValues *kv = new KeyValues( filename );

	if ( kv->LoadFromFile( filesystem, filename, "GAME" ) )
	{
		// parse out all of the top level sections and save their names
		KeyValues *pKeys = kv;
		while ( pKeys )
		{
			if ( pKeys->GetFirstSubKey() )
			{
				CSoundEntry entry;
				entry.m_bRemoved			= false;
				entry.m_nScriptFileIndex	= scriptindex;
				entry.m_bPrecacheAlways		= precachealways;

				int lookup = m_Sounds.Find( pKeys->GetName() );
				if ( lookup != m_Sounds.InvalidIndex() )
				{
					DevMsg( "CSoundEmitterSystem::AddSoundsFromFile(%s):  Entry %s duplicated, skipping\n", filename, pKeys->GetName() );
				}
				else
				{
					InitSoundInternalParameters( pKeys->GetName(), pKeys, entry.m_SoundParams );
					m_Sounds.Insert( pKeys->GetName(), entry );
				}
			}
			pKeys = pKeys->GetNextKey();
		}
	}
	else
	{
		Msg( "CSoundEmitterSystem::AddSoundsFromFile:  No such file %s\n", filename );

		// Discard
		m_SoundKeyValues.Remove( scriptindex );

		kv->deleteThis();
		return;
	}

	Assert( scriptindex >= 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSoundEmitterSystemBase::CheckForMissingWavFiles( bool verbose )
{
	int missing = 0;

	int c = GetSoundCount();
	int i;
	char testfile[ 512 ];

	for ( i = 0; i < c; i++ )
	{
		CSoundParametersInternal *internal = InternalGetParametersForSound( i );
		if ( !internal )
		{
			Assert( 0 );
			continue;
		}

		int waveCount = internal->soundnames.Count();
		for ( int wave = 0; wave < waveCount; wave++ )
		{
			CUtlSymbol sym = internal->soundnames[ wave ];
			const char *name = m_Waves.String( sym );
			if ( !name || !name[ 0 ] )
			{
				Assert( 0 );
				continue;
			}

			// Skip ! sentence stuff
			if ( name[0] == CHAR_SENTENCE )
				continue;

			Q_snprintf( testfile, sizeof( testfile ), "sound/%s", PSkipSoundChars( name ) );

			if ( filesystem->FileExists( testfile ) )
				continue;

			internal->had_missing_wave_files = true;

			++missing;

			if ( verbose )
			{
				DevMsg( "Sound %s references missing file %s\n", GetSoundName( i ), name );
			}
		}
	}

	return missing;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *key - 
// Output : float
//-----------------------------------------------------------------------------
float CSoundEmitterSystemBase::TranslateAttenuation( const char *key )
{
	if ( !key )
	{
		Assert( 0 );
		return ATTN_NORM;
	}

	if ( !Q_strcasecmp( key, "ATTN_NONE" ) )
		return ATTN_NONE;

	if ( !Q_strcasecmp( key, "ATTN_NORM" ) )
		return ATTN_NORM;

	if ( !Q_strcasecmp( key, "ATTN_IDLE" ) )
		return ATTN_IDLE;

	if ( !Q_strcasecmp( key, "ATTN_STATIC" ) )
		return ATTN_STATIC;

	if ( !Q_strcasecmp( key, "ATTN_RICOCHET" ) )
		return ATTN_RICOCHET;

	if ( !Q_strcasecmp( key, "ATTN_GUNFIRE" ) )
		return ATTN_GUNFIRE;

	DevMsg( "CSoundEmitterSystem:  Unknown attenuation key %s\n", key );

	return ATTN_NORM;
}

soundlevel_t TextToSoundLevel( const char *key )
{
	if ( !key )
	{
		Assert( 0 );
		return SNDLVL_NORM;
	}

	int c = ARRAYSIZE( g_pSoundLevels );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		SoundLevelLookup *entry = &g_pSoundLevels[ i ];
		if ( !Q_strcasecmp( key, entry->name ) )
			return entry->level;
	}

	DevMsg( "CSoundEmitterSystem:  Unknown sound level %s\n", key );

	return SNDLVL_NORM;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *key - 
// Output : soundlevel_t
//-----------------------------------------------------------------------------
soundlevel_t CSoundEmitterSystemBase::TranslateSoundLevel( const char *key )
{
	return TextToSoundLevel( key );
}

const char *CSoundEmitterSystemBase::SoundLevelToString( soundlevel_t level )
{
	int c = ARRAYSIZE( g_pSoundLevels );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		SoundLevelLookup *entry = &g_pSoundLevels[ i ];
		if ( entry->level == level )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i", (int)level );
	return sz;
}

const char *CSoundEmitterSystemBase::ChannelToString( int channel )
{
	int c = ARRAYSIZE( g_pChannelNames );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		SoundChannels *entry = &g_pChannelNames[ i ];
		if ( entry->channel == channel )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%i", (int)channel );
	return sz;
}

const char *CSoundEmitterSystemBase::VolumeToString( float volume )
{
	int c = ARRAYSIZE( g_pVolumeLevels );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		VolumeLevel *entry = &g_pVolumeLevels[ i ];
		if ( entry->volume == volume )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%.3f", volume );
	return sz;
}

const char *CSoundEmitterSystemBase::PitchToString( float pitch )
{
	int c = ARRAYSIZE( g_pPitchLookup );

	int i;

	for ( i = 0 ; i < c; i++ )
	{
		PitchLookup *entry = &g_pPitchLookup[ i ];
		if ( entry->pitch == pitch )
			return entry->name;
	}

	static char sz[ 32 ];
	Q_snprintf( sz, sizeof( sz ), "%.3f", pitch );
	return sz;
}

//-----------------------------------------------------------------------------
// Purpose: Convert "chan_xxx" into integer value for channel
// Input  : *name - 
// Output : static int
//-----------------------------------------------------------------------------
int CSoundEmitterSystemBase::TranslateChannel( const char *name )
{
	return TextToChannel( name );
}

const char *CSoundEmitterSystemBase::GetSourceFileForSound( int index ) const
{
	if ( index < 0 || index >= (int)m_Sounds.Count() )
	{
		Assert( 0 );
		return "";
	}

	CSoundEntry const *entry = &m_Sounds[ index ];
	int scriptindex = entry->m_nScriptFileIndex;
	if ( scriptindex < 0 || scriptindex >= m_SoundKeyValues.Count() )
	{
		Assert( 0 );
		return "";
	}

	return m_SoundKeyValues[ scriptindex ].filename;
}

const char *CSoundEmitterSystemBase::GetWaveName( CUtlSymbol& sym )
{
	return m_Waves.String( sym );
}


int	CSoundEmitterSystemBase::FindSoundScript( const char *name ) const
{
	int i, c;

	// First, make sure it's known
	c = m_SoundKeyValues.Count();
	for ( i = 0; i < c ; i++ )
	{
		if ( !Q_stricmp( m_SoundKeyValues[ i ].filename, name ) )
		{
			return i;
		}
	}

	return m_SoundKeyValues.InvalidIndex();
}

bool CSoundEmitterSystemBase::AddSound( const char *soundname, const char *scriptfile, const CSoundParametersInternal& params )
{
	int idx = GetSoundIndex( soundname );


	int i = FindSoundScript( scriptfile );
	if ( i == m_SoundKeyValues.InvalidIndex() )
	{
		Warning( "CSoundEmitterSystemBase::AddSound( '%s', '%s', ... ), script file not list in manifest '%s'\n",
			soundname, scriptfile, MANIFEST_FILE );
		return false;
	}

	// More like an update...
	if ( IsValidIndex( idx ) )
	{
		CSoundEntry *entry = &m_Sounds[ idx ];

		entry->m_bRemoved				= false;
		entry->m_nScriptFileIndex		= i;
		entry->m_bPrecacheAlways		= m_SoundKeyValues[ i ].precachealways;
		entry->m_SoundParams			= params;

		m_SoundKeyValues[ i ].dirty = true;

		return true;
	}

	CSoundEntry entry;
	entry.m_bRemoved			= false;
	entry.m_nScriptFileIndex	= i;
	entry.m_bPrecacheAlways		= m_SoundKeyValues[ i ].precachealways;
	entry.m_SoundParams			= params;

	m_Sounds.Insert( soundname, entry );

	m_SoundKeyValues[ i ].dirty = true;

	return true;
}

void CSoundEmitterSystemBase::RemoveSound( const char *soundname )
{
	int idx = GetSoundIndex( soundname );
	if ( !IsValidIndex( idx ) )
	{
		Warning( "Can't remove %s, no such sound!\n", soundname );
		return;
	}

	m_Sounds[ idx ].m_bRemoved = true;

	// Mark script as dirty
	int scriptindex = m_Sounds[ idx ].m_nScriptFileIndex;
	if ( scriptindex < 0 || scriptindex >= m_SoundKeyValues.Count() )
	{
		Assert( 0 );
		return;
	}

	m_SoundKeyValues[ scriptindex ].dirty = true;
}

void CSoundEmitterSystemBase::MoveSound( const char *soundname, const char *newscript )
{
	int idx = GetSoundIndex( soundname );
	if ( !IsValidIndex( idx ) )
	{
		Warning( "Can't move '%s', no such sound!\n", soundname );
		return;
	}

	int oldscriptindex = m_Sounds[ idx ].m_nScriptFileIndex;
	if ( oldscriptindex < 0 || oldscriptindex >= m_SoundKeyValues.Count() )
	{
		Assert( 0 );
		return;
	}

	int newscriptindex = FindSoundScript( newscript );
	if ( newscriptindex == m_SoundKeyValues.InvalidIndex() )
	{
		Warning( "CSoundEmitterSystemBase::MoveSound( '%s', '%s' ), script file not list in manifest '%s'\n",
			soundname, newscript, MANIFEST_FILE );
		return;
	}

	// No actual change
	if ( oldscriptindex == newscriptindex )
	{
		return;
	}

	// Move it
	m_Sounds[ idx ].m_nScriptFileIndex = newscriptindex;

	// Mark both scripts as dirty
	m_SoundKeyValues[ oldscriptindex ].dirty = true;
	m_SoundKeyValues[ newscriptindex ].dirty = true;
}

int CSoundEmitterSystemBase::GetNumSoundScripts() const
{
	return m_SoundKeyValues.Count();
}

const char *CSoundEmitterSystemBase::GetSoundScriptName( int index ) const
{
	if ( index < 0 || index >= m_SoundKeyValues.Count() )
		return NULL;

	return m_SoundKeyValues[ index ].filename;
}

bool CSoundEmitterSystemBase::IsSoundScriptDirty( int index ) const
{
	if ( index < 0 || index >= m_SoundKeyValues.Count() )
		return false;

	return m_SoundKeyValues[ index ].dirty;
}

void CSoundEmitterSystemBase::SaveChangesToSoundScript( int scriptindex )
{
	const char *outfile = GetSoundScriptName( scriptindex );
	if ( !outfile )
	{
		Msg( "CSoundEmitterSystemBase::SaveChangesToSoundScript:  No script file for index %i\n", scriptindex );
		return;
	}

	if ( filesystem->FileExists( outfile ) &&
		 !filesystem->IsFileWritable( outfile ) )
	{
		Warning( "%s is not writable, can't save data to file\n", outfile );
		return;
	}

	CUtlBuffer buf( 0, 0, true );

	// FIXME:  Write sound script header
	if ( filesystem->FileExists( GAME_SOUNDS_HEADER_BLOCK ) )
	{
		FileHandle_t header = filesystem->Open( GAME_SOUNDS_HEADER_BLOCK, "rt", NULL );
		if ( header != FILESYSTEM_INVALID_HANDLE )
		{
			int len = filesystem->Size( header );
			
			unsigned char *data = new unsigned char[ len + 1 ];
			Q_memset( data, 0, len + 1 );
			
			filesystem->Read( data, len, header );
			filesystem->Close( header );

			data[ len ] = 0;

			buf.Put( data, Q_strlen( (char *)data ) );

			delete[] data;
		}

		buf.Printf( "\n" );
	}


	int c = GetSoundCount();
	for ( int i = 0; i < c; i++ )
	{
		if ( Q_stricmp( outfile, GetSourceFileForSound( i ) ) )
			continue;

		// It's marked for deletion, just skip it
		if ( m_Sounds[ i ].m_bRemoved )
			continue;

		CSoundEmitterSystemBase::CSoundParametersInternal *p = InternalGetParametersForSound( i );
		if ( !p )
			continue;
		
		buf.Printf( "\"%s\"\n{\n", GetSoundName( i ) );

		buf.Printf( "\t\"channel\"\t\t\"%s\"\n", p->m_szChannel );
		buf.Printf( "\t\"volume\"\t\t\"%s\"\n", p->m_szVolume );
		buf.Printf( "\t\"pitch\"\t\t\t\"%s\"\n", p->m_szPitch );
		buf.Printf( "\n" );
		buf.Printf( "\t\"soundlevel\"\t\"%s\"\n", p->m_szSoundLevel );

		if ( p->play_to_owner_only )
		{
			buf.Printf( "\t\"play_to_owner_only\"\t\"1\"\n" );
		}
		if ( !p->precache )
		{
			buf.Printf( "\t\"precache\"\t\"0\"\n" );

		}

		int waveCount = p->soundnames.Count();

		if  ( waveCount > 0 )
		{
			buf.Printf( "\n" );

			if ( waveCount == 1 )
			{
				buf.Printf( "\t\"wave\"\t\t\t\"%s\"\n", GetWaveName( p->soundnames[ 0 ] ) );
			}
			else
			{
				buf.Printf( "\t\"rndwave\"\n" );
				buf.Printf( "\t{\n" );

				for ( int wave = 0; wave < waveCount; wave++ )
				{
					buf.Printf( "\t\t\"wave\"\t\"%s\"\n", GetWaveName( p->soundnames[ wave ] ) );
				}

				buf.Printf( "\t}\n" );
			}

		}

		buf.Printf( "}\n" );

		if ( i != c - 1 )
		{
			buf.Printf( "\n" );
		}
	}

	// Write it out baby
	FileHandle_t fh = filesystem->Open( outfile, "wt" );
	if (fh)
	{
		filesystem->Write( buf.Base(), buf.TellPut(), fh );
		filesystem->Close(fh);

		// Changed saved successfully
		m_SoundKeyValues[ scriptindex ].dirty = false;
	}
	else
	{
		Warning( "SceneManager_SaveSoundsToScriptFile:  Unable to write file %s!!!\n", outfile );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : CUtlSymbol
//-----------------------------------------------------------------------------
CUtlSymbol CSoundEmitterSystemBase::AddWaveName( const char *name )
{
	return m_Waves.AddString( name );
}

void CSoundEmitterSystemBase::RenameSound( const char *soundname, const char *newname )
{
	// Same name?
	if ( !Q_stricmp( soundname, newname ) )
	{
		return;
	}

	int oldindex = GetSoundIndex( soundname );
	if ( !IsValidIndex( oldindex ) )
	{
		Msg( "Can't rename %s, no such sound\n", soundname );
		return;
	}

	int check = GetSoundIndex( newname );
	if ( IsValidIndex( check ) )
	{
		Msg( "Can't rename %s to %s, new name already in list\n", soundname, newname );
		return;
	}

	// Copy out old entry
	CSoundEntry entry = m_Sounds[ oldindex ];
	// Remove it
	m_Sounds.Remove( soundname );
	// Re-insert in new spot
	m_Sounds.Insert( newname, entry );

	// Mark associated script as dirty
	m_SoundKeyValues[ entry.m_nScriptFileIndex ].dirty = true;
}

void CSoundEmitterSystemBase::UpdateSoundParameters( const char *soundname, const CSoundParametersInternal& params )
{
	int idx = GetSoundIndex( soundname );
	if ( !IsValidIndex( idx ) )
	{
		Msg( "Can't UpdateSoundParameters %s, no such sound\n", soundname );
		return;
	}

	CSoundEntry *entry = &m_Sounds[ idx ];

	if ( entry->m_SoundParams == params )
	{
		// No changes
		return;
	}

	// Update parameters
	entry->m_SoundParams = params;
	// Set dirty flag
	m_SoundKeyValues[ entry->m_nScriptFileIndex ].dirty = true;
}


