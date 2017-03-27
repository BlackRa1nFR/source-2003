//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SOUNDEMITTERSYSTEMBASE_H
#define SOUNDEMITTERSYSTEMBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "utldict.h"
#include "soundflags.h"
#include "interval.h"

class KeyValues;

soundlevel_t TextToSoundLevel( const char *key );

class CSoundEmitterSystemBase;

struct CSoundParameters
{
	CSoundParameters()
	{
		channel		= CHAN_AUTO; // 0
		volume		= VOL_NORM;  // 1.0f
		pitch		= PITCH_NORM; // 100

		pitchlow	= PITCH_NORM;
		pitchhigh	= PITCH_NORM;

		soundlevel	= SNDLVL_NORM; // 75dB
		soundname[ 0 ] = 0;
		play_to_owner_only = false;
		count		= 0;
	}

	int				channel;
	float			volume;
	int				pitch;
	int				pitchlow, pitchhigh;
	soundlevel_t	soundlevel;
	// For weapon sounds...
	bool			play_to_owner_only;

	int				count;
	char 			soundname[ 128 ];
};



//-----------------------------------------------------------------------------
// Purpose: Base class for sound emitter system handling (can be used by tools)
//-----------------------------------------------------------------------------
class CSoundEmitterSystemBase
{
public:
	struct CSoundParametersInternal
	{
		CSoundParametersInternal()
		{
			channel			= CHAN_AUTO; // 0

			Q_strcpy( m_szChannel, "CHAN_AUTO" );

			volume.start	= VOL_NORM;  // 1.0f
			volume.range	= 0.0f;

			Q_strcpy( m_szVolume, "VOL_NORM" );

			pitch.start		= (float)PITCH_NORM; // 100
			pitch.range		= 0.0f;

			Q_strcpy( m_szPitch, "PITCH_NORM" );

			soundlevel.start = (float)SNDLVL_NORM; // 75dB
			soundlevel.range = 0.0f;

			Q_strcpy( m_szSoundLevel, "SNDLVL_NORM" );

			play_to_owner_only = false;
			precache			= true;

			had_missing_wave_files = false;
		}

		CSoundParametersInternal( const CSoundParametersInternal& src )
		{
			channel = src.channel;
			volume = src.volume;
			pitch = src.pitch;
			soundlevel = src.soundlevel;
			play_to_owner_only = src.play_to_owner_only;

			int c = src.soundnames.Count();
			for ( int i = 0; i < c; i++ )
			{
				soundnames.AddToTail( src.soundnames[ i ] );
			}
			precache = src.precache;
			had_missing_wave_files = src.had_missing_wave_files;

			memcpy( m_szChannel, src.m_szChannel, sizeof( m_szChannel ) );
			memcpy( m_szVolume, src.m_szVolume, sizeof( m_szVolume ) );
			memcpy( m_szPitch, src.m_szPitch, sizeof( m_szPitch ) );
			memcpy( m_szSoundLevel, src.m_szSoundLevel, sizeof( m_szSoundLevel ) );
		}

		bool CompareInterval( const interval_t& i1, const interval_t& i2 ) const
		{
			if ( i1.start != i2.start )
				return false;
			if ( i1.range != i2.range )
				return false;
			return true;
		}

		bool CSoundParametersInternal::operator == ( const CSoundParametersInternal& other ) const
		{
			if ( this == &other )
				return true;

			if ( channel != other.channel )
				return false;
			if ( !CompareInterval( volume, other.volume ) )
				return false;
			if ( !CompareInterval( pitch, other.pitch ) )
				return false;
			if ( !CompareInterval( soundlevel, other.soundlevel ) )
				return false;

			if ( Q_stricmp( m_szChannel, other.m_szChannel ) )
				return false;
			if ( Q_stricmp( m_szVolume, other.m_szVolume ) )
				return false;
			if ( Q_stricmp( m_szPitch, other.m_szPitch ) )
				return false;
			if ( Q_stricmp( m_szSoundLevel, other.m_szSoundLevel ) )
				return false;

			if ( play_to_owner_only != other.play_to_owner_only )
				return false;

			if ( precache != other.precache )
				return false;

			if ( soundnames.Count() != other.soundnames.Count() )
				return false;

			// Compare items
			int c = soundnames.Count();
			for ( int i = 0; i < c; i++ )
			{
				if ( soundnames[ i ] != other.soundnames[ i ] )
					return false;
			}

			return true;
		}

		const char *VolumeToString( void );
		const char *ChannelToString( void );
		const char *SoundLevelToString( void );
		const char *PitchToString( void );

		void		VolumeFromString( const char *sz );
		void		ChannelFromString( const char *sz );
		void		PitchFromString( const char *sz );
		void		SoundLevelFromString( const char *sz );

		int				channel;
		interval_t		volume;
		interval_t		pitch;
		interval_t		soundlevel;
		// For weapon sounds...
		bool			play_to_owner_only;
		bool			precache;

		CUtlVector< CUtlSymbol >	soundnames;
		// Internal use, for warning about missing .wav files
		bool			had_missing_wave_files;

		enum
		{
			MAX_SOUND_ATTRIB_STRING = 32,
		};

		char			m_szVolume[ MAX_SOUND_ATTRIB_STRING ];
		char			m_szChannel[ MAX_SOUND_ATTRIB_STRING ];
		char			m_szSoundLevel[ MAX_SOUND_ATTRIB_STRING ];
		char			m_szPitch[ MAX_SOUND_ATTRIB_STRING ];
	};
protected:

	struct CSoundEntry
	{
		bool		m_bRemoved;

		bool		m_bPrecacheAlways;
		int			m_nScriptFileIndex;

		CSoundParametersInternal m_SoundParams;
	};

public:
	CSoundEmitterSystemBase() {}
	virtual ~CSoundEmitterSystemBase() { }

	// 
	virtual bool BaseInit();
	virtual void BaseShutdown();


public:

	void BuildPrecacheSoundList( const char *scriptfile, CUtlVector< int >& list );

	int	GetSoundIndex( const char *pName ) const;
	bool IsValidIndex( int index );
	int		GetSoundCount( void );
	bool		GetShouldAlwaysPrecacheSound( int index );

	const char *GetSoundName( int index );
	bool	GetParametersForSound( const char *soundname, CSoundParameters& params );
	const char *GetWaveName( CUtlSymbol& sym );
	CUtlSymbol	AddWaveName( const char *name );

	soundlevel_t LookupSoundLevel( const char *soundname );
	const char *GetWavFileForSound( const char *soundname );

	int		CheckForMissingWavFiles( bool verbose );

	const char *SoundLevelToString( soundlevel_t level );
	const char *ChannelToString( int channel );
	const char *VolumeToString( float volume );
	const char *PitchToString( float pitch );

	const char *GetSourceFileForSound( int index ) const;

	// Iteration methods
	int		First() const;
	int		Next( int i ) const;
	int		InvalidIndex() const;

	CSoundParametersInternal *InternalGetParametersForSound( int index );

	// The host application is responsible for dealing with dirty sound scripts, etc.
	bool		AddSound( const char *soundname, const char *scriptfile, const CSoundParametersInternal& params );
	void		RemoveSound( const char *soundname );

	void		MoveSound( const char *soundname, const char *newscript );

	void		RenameSound( const char *soundname, const char *newname );
	void		UpdateSoundParameters( const char *soundname, const CSoundParametersInternal& params );

	int			GetNumSoundScripts() const;
	char const	*GetSoundScriptName( int index ) const;
	bool		IsSoundScriptDirty( int index ) const;
	int			FindSoundScript( const char *name ) const;

	void		SaveChangesToSoundScript( int scriptindex );
private:

	void AddSoundsFromFile( const char *filename, bool precachealways );

	bool		InitSoundInternalParameters( const char *soundname, KeyValues *kv, CSoundParametersInternal& params );


	float	TranslateAttenuation( const char *key );
	soundlevel_t	TranslateSoundLevel( const char *key );
	int TranslateChannel( const char *name );

	CUtlDict< CSoundEntry, int >		m_Sounds;

	struct CSoundScriptFile
	{
		bool		dirty;
		bool		precachealways;
		char		filename[ 128 ];
	};

	CUtlVector< CSoundScriptFile >			m_SoundKeyValues;

	CUtlSymbolTable		m_Waves;
};

#endif // SOUNDEMITTERSYSTEMBASE_H
