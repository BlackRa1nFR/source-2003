//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Soundscapes.txt resource file processor
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include <KeyValues.h>
#include "engine/ienginesound.h"
#include "filesystem.h"
#include "soundemittersystembase.h"

// Only allow recursive references to be 8 levels deep.
// This test will flag any circular references and bail.
#define MAX_SOUNDSCAPE_RECURSION	8

// Keep an array of all looping sounds so they can be faded in/out
// OPTIMIZE: Get a handle/pointer to the engine's sound channel instead 
//			of searching each frame!
struct loopingsound_t
{
	Vector		position;		// position (if !isAmbient)
	const char *pWaveName;		// name of the wave file
	float		volumeTarget;	// target volume level (fading towards this)
	float		volumeCurrent;	// current volume level
	soundlevel_t soundlevel;	// sound level (if !isAmbient)
	int			pitch;			// pitch shift
	int			id;				// Used to fade out sounds that don't belong to the most current setting
	bool		isAmbient;		// Ambient sounds have no spatialization - they play from everywhere
};

ConVar soundscape_fadetime( "soundscape_fadetime", "3.0", 0, "Time to crossfade sound effects between soundscapes" );

#include "interval.h"

struct randomsound_t
{
	Vector		position;
	float		nextPlayTime;	// time to play a sound from the set
	interval_t	time;
	interval_t	volume;
	interval_t	pitch;
	interval_t	soundlevel;
	float		masterVolume;
	int			waveCount;
	bool		isAmbient;
	KeyValues	*pWaves;

	void Init()
	{
		memset( this, 0, sizeof(*this) );
	}
};

struct subsoundscapeparams_t
{
	int		recurseLevel;		// test for infinite loops in the script / circular refs
	float	masterVolume;
	int		startingPosition;
	int		positionOverride;	// forces all sounds to this position
	int		ambientPositionOverride;	// forces all ambient sounds to this position
	bool	allowDSP;
};

class C_SoundscapeSystem : public IGameSystem
{
public:
	~C_SoundscapeSystem() {}

	// IClientSystem hooks, not needed
	// Level init, shutdown
	virtual void LevelInitPreEntity() {}
	virtual void LevelInitPostEntity() {}

	// The level is shutdown in two parts
	virtual void LevelShutdownPreEntity() {}
	// Entities are deleted / released here...
	virtual void LevelShutdownPostEntity()
	{
		m_params.ent.Set( NULL );
		m_params.soundscapeIndex = -1;
		m_loopingSounds.Purge();
		m_randomSounds.Purge();
	}

	virtual void OnSave() {}
	virtual void OnRestore() {}

	// Called before rendering
	virtual void PreRender() {}

	// IClientSystem hooks used
	virtual bool Init();
	virtual void Shutdown();
	// Gets called each frame
	virtual void Update( float frametime );

	
	// local functions
	void UpdateAudioParams( audioparams_t &audio );
	void GetAudioParams( audioparams_t &out ) const { out = m_params; }
	void UpdateLoopingSounds( float frametime );
	int AddLoopingAmbient( const char *pSoundName, float volume, int pitch );
	void UpdateLoopingSound( loopingsound_t &loopSound );
	void StopLoopingSound( loopingsound_t &loopSound );
	int AddLoopingSound( const char *pSoundName, bool isAmbient, float volume, 
		soundlevel_t soundLevel, int pitch, const Vector &position );
	int AddRandomSound( const randomsound_t &sound );
	void PlayRandomSound( const randomsound_t &sound );
	void UpdateRandomSounds( float gameClock );

	KeyValues *FindSoundscapeByName( const char *pSoundscapeName );
	
	// main-level soundscape processing, called on new soundscape
	void StartNewSoundscape( KeyValues *pSoundscape );
	void StartSubSoundscape( KeyValues *pSoundscape, const subsoundscapeparams_t &params );

	// root level soundscape keys
	// add a process for each new command here
	// "dsp"
	void ProcessDSP( KeyValues *pDSP );
	// "playlooping"
	void ProcessPlayLooping( KeyValues *pPlayLooping, const subsoundscapeparams_t &params );	
	// "playrandom"
	void ProcessPlayRandom( KeyValues *pPlayRandom, const subsoundscapeparams_t &params );
	// "playsoundscape"
	void ProcessPlaySoundscape( KeyValues *pPlaySoundscape, const subsoundscapeparams_t &params );


private:

	void	AddSoundScapeFile( const char *filename );

	CUtlVector< KeyValues * >	m_SoundscapeScripts;	// The whole script file in memory
	CUtlVector<KeyValues *>		m_soundscapes;			// Lookup by index of each root section
	audioparams_t				m_params;				// current player audio params
	CUtlVector<loopingsound_t>	m_loopingSounds;		// list of currently playing sounds
	CUtlVector<randomsound_t>	m_randomSounds;			// list of random sound commands
	float						m_nextRandomTime;		// next time to play a random sound
	int							m_loopingSoundId;		// marks when the sound was issued
};


// singleton system
C_SoundscapeSystem g_SoundscapeSystem;

IGameSystem *ClientSoundscapeSystem()
{
	return &g_SoundscapeSystem;
}


// player got a network update
void Soundscape_Update( audioparams_t &audio )
{
	g_SoundscapeSystem.UpdateAudioParams( audio );
}

#define SOUNDSCAPE_MANIFEST_FILE				"scripts/soundscapes_manifest.txt"

void C_SoundscapeSystem::AddSoundScapeFile( const char *filename )
{
	KeyValues *script = new KeyValues( filename );
	if ( script->LoadFromFile( filesystem, filename ) )
	{
		// parse out all of the top level sections and save their names
		KeyValues *pKeys = script;
		while ( pKeys )
		{
			// save pointers to all sections in the root
			// each one is a soundscape
			if ( pKeys->GetFirstSubKey() )
			{
				m_soundscapes.AddToTail( pKeys );
			}
			pKeys = pKeys->GetNextKey();
		}

		// Keep pointer around so we can delete it at exit
		m_SoundscapeScripts.AddToTail( script );
	}
	else
	{
		script->deleteThis();
	}
}

// parse the script file, setup index table
bool C_SoundscapeSystem::Init()
{
	m_loopingSoundId = 0;

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

			Warning( "C_SoundscapeSystem::Init:  Manifest '%s' with bogus file type '%s', expecting 'file'\n", 
				SOUNDSCAPE_MANIFEST_FILE, sub->GetName() );
		}
	}
	else
	{
		Error( "Unable to load manifest file '%s'\n", SOUNDSCAPE_MANIFEST_FILE );
	}

	return true;
}


KeyValues *C_SoundscapeSystem::FindSoundscapeByName( const char *pSoundscapeName )
{
	// UNDONE: Bad perf, linear search!
	for ( int i = m_soundscapes.Count()-1; i >= 0; --i )
	{
		if ( !Q_stricmp( m_soundscapes[i]->GetName(), pSoundscapeName ) )
			return m_soundscapes[i];
	}

	return NULL;
}

void C_SoundscapeSystem::Shutdown()
{
	for ( int i = m_loopingSounds.Count() - 1; i >= 0; --i )
	{
		loopingsound_t &sound = m_loopingSounds[i];

		// sound is done, remove from list.
		StopLoopingSound( sound );
	}
	
	// These are only necessary so we can use shutdown/init calls
	// to flush soundscape data
	m_loopingSounds.RemoveAll();
	m_randomSounds.RemoveAll();
	m_soundscapes.RemoveAll();
	m_params.ent.Set( NULL );
	m_params.soundscapeIndex = -1;

	while ( m_SoundscapeScripts.Count() > 0 )
	{
		KeyValues *kv = m_SoundscapeScripts[ 0 ];
		m_SoundscapeScripts.Remove( 0 );
		kv->deleteThis();
	}
}

// NOTE: This will not flush the server side so you cannot add or remove
// soundscapes from the list, only change their parameters!!!!
CON_COMMAND(soundscape_flush, "Flushes the client side soundscapes")
{
	// save the current soundscape
	audioparams_t tmp;
	g_SoundscapeSystem.GetAudioParams( tmp );

	// kill the system
	g_SoundscapeSystem.Shutdown();

	// restart the system
	g_SoundscapeSystem.Init();

	// reload the soundscape params from the temp copy
	Soundscape_Update( tmp );
}


// This makes all currently playing loops fade toward their target volume
void C_SoundscapeSystem::UpdateLoopingSounds( float frametime )
{
	float period = soundscape_fadetime.GetFloat();
	float amount = frametime;
	if ( period > 0 )
	{
		amount *= 1.0 / period;
	}

	int fadeCount = m_loopingSounds.Count();
	while ( fadeCount > 0 )
	{
		fadeCount--;
		loopingsound_t &sound = m_loopingSounds[fadeCount];

		if ( sound.volumeCurrent != sound.volumeTarget )
		{
			sound.volumeCurrent = Approach( sound.volumeTarget, sound.volumeCurrent, amount );
			if ( sound.volumeTarget == 0 && sound.volumeCurrent == 0 )
			{
				// sound is done, remove from list.
				StopLoopingSound( sound );
				m_loopingSounds.FastRemove( fadeCount );
			}
			else
			{
				// tell the engine about the new volume
				UpdateLoopingSound( sound );
			}
		}
	}
}

void C_SoundscapeSystem::Update( float frametime ) 
{
	// fade out the old sounds over soundscape_fadetime seconds
	UpdateLoopingSounds( frametime );
	UpdateRandomSounds( gpGlobals->curtime );
}


void C_SoundscapeSystem::UpdateAudioParams( audioparams_t &audio )
{
	if ( m_params.soundscapeIndex == audio.soundscapeIndex && m_params.ent.Get() == audio.ent.Get() )
		return;

	m_params = audio;
	if ( audio.ent.Get() && audio.soundscapeIndex >= 0 && audio.soundscapeIndex < m_soundscapes.Count() )
	{
		DevMsg( 1, "Soundscape: %s\n", m_soundscapes[audio.soundscapeIndex]->GetName() );
		StartNewSoundscape( m_soundscapes[audio.soundscapeIndex] );
	}
	else
	{
		// bad index?
		if ( audio.ent.Get() != 0 )
		{
			DevMsg(1, "Error: Bad soundscape!\n");
		}
	}
}



// Called when a soundscape is activated (leading edge of becoming the active soundscape)
void C_SoundscapeSystem::StartNewSoundscape( KeyValues *pSoundscape )
{
	int i;

	// Reset the system
	// fade out the current loops
	for ( i = m_loopingSounds.Count()-1; i >= 0; --i )
	{
		m_loopingSounds[i].volumeTarget = 0;
	}
	// update ID
	m_loopingSoundId++;

	// clear all random sounds
	m_randomSounds.RemoveAll();
	m_nextRandomTime = gpGlobals->curtime;

	subsoundscapeparams_t params;
	params.allowDSP = true;
	params.masterVolume = 1.0;
	params.startingPosition = 0;
	params.recurseLevel = 0;
	params.positionOverride = -1;
	params.ambientPositionOverride = -1;
	StartSubSoundscape( pSoundscape, params );
}

void C_SoundscapeSystem::StartSubSoundscape( KeyValues *pSoundscape, const subsoundscapeparams_t &params )
{
	// Parse/process all of the commands
	KeyValues *pKey = pSoundscape->GetFirstSubKey();
	while ( pKey )
	{
		if ( !Q_strcasecmp( pKey->GetName(), "dsp" ) )
		{
			if ( params.allowDSP )
			{
				ProcessDSP( pKey );
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "playlooping" ) )
		{
			ProcessPlayLooping( pKey, params );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "playrandom" ) )
		{
			ProcessPlayRandom( pKey, params );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "playsoundscape" ) )
		{
			ProcessPlaySoundscape( pKey, params );
		}
		// add new commands here
		pKey = pKey->GetNextKey();
	}
}

// add a process for each new command here

// change DSP effect
void C_SoundscapeSystem::ProcessDSP( KeyValues *pDSP )
{
	int roomType = pDSP->GetInt();
	CLocalPlayerFilter filter;
	enginesound->SetRoomType( filter, roomType );
}


// start a new looping sound
void C_SoundscapeSystem::ProcessPlayLooping( KeyValues *pAmbient, const subsoundscapeparams_t &params )
{
	float volume = 0;
	soundlevel_t soundlevel = ATTN_TO_SNDLVL(ATTN_NORM);
	const char *pSoundName = NULL;
	int pitch = PITCH_NORM;
	int positionIndex = -1;
	KeyValues *pKey = pAmbient->GetFirstSubKey();
	while ( pKey )
	{
		if ( !Q_strcasecmp( pKey->GetName(), "volume" ) )
		{
			volume = params.masterVolume * RandomInterval( ReadInterval( pKey->GetString() ) );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "pitch" ) )
		{
			pitch = RandomInterval( ReadInterval( pKey->GetString() ) );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "wave" ) )
		{
			pSoundName = pKey->GetString();
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "position" ) )
		{
			positionIndex = params.startingPosition + pKey->GetInt();
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "attenuation" ) )
		{
			soundlevel = ATTN_TO_SNDLVL( RandomInterval( ReadInterval( pKey->GetString() ) ) );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "soundlevel" ) )
		{
			if ( !Q_strncasecmp( pKey->GetString(), "SNDLVL_", strlen( "SNDLVL_" ) ) )
			{
				soundlevel = TextToSoundLevel( pKey->GetString() );
			}
			else
			{
				soundlevel = (soundlevel_t)((int)RandomInterval( ReadInterval( pKey->GetString() ) ));
			}
		}
		pKey = pKey->GetNextKey();
	}

	if ( positionIndex < 0 )
	{
		positionIndex = params.ambientPositionOverride;
	}
	else if ( params.positionOverride >= 0 )
	{
		positionIndex = params.positionOverride;
	}
	if ( volume != 0 && pSoundName != NULL )
	{
		if ( positionIndex < 0 )
		{
			AddLoopingAmbient( pSoundName, volume, pitch );
		}
		else
		{
			if ( positionIndex > 31 || !(m_params.localBits & (1<<positionIndex) ) )
			{
				// suppress sounds if the position isn't available
				//DevMsg( 1, "Bad position %d\n", positionIndex );
				return;
			}
			AddLoopingSound( pSoundName, false, volume, soundlevel, pitch, m_params.localSound[positionIndex] );
		}
	}
}


// puts a recurring random sound event into the queue
void C_SoundscapeSystem::ProcessPlayRandom( KeyValues *pPlayRandom, const subsoundscapeparams_t &params )
{
	randomsound_t sound;
	sound.Init();
	sound.masterVolume = params.masterVolume;
	int positionIndex = -1;
	KeyValues *pKey = pPlayRandom->GetFirstSubKey();
	while ( pKey )
	{
		if ( !Q_strcasecmp( pKey->GetName(), "volume" ) )
		{
			sound.volume = ReadInterval( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "pitch" ) )
		{
			sound.pitch = ReadInterval( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "attenuation" ) )
		{
			interval_t atten = ReadInterval( pKey->GetString() );
			sound.soundlevel.start = ATTN_TO_SNDLVL( atten.start );
			sound.soundlevel.range = ATTN_TO_SNDLVL( atten.start + atten.range ) - sound.soundlevel.start;
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "soundlevel" ) )
		{
			if ( !Q_strncasecmp( pKey->GetString(), "SNDLVL_", strlen( "SNDLVL_" ) ) )
			{
				sound.soundlevel.start = TextToSoundLevel( pKey->GetString() );
				sound.soundlevel.range = 0;
			}
			else
			{
				sound.soundlevel = ReadInterval( pKey->GetString() );
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "time" ) )
		{
			sound.time = ReadInterval( pKey->GetString() );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "rndwave" ) )
		{
			KeyValues *pWaves = pKey->GetFirstSubKey();
			sound.pWaves = pWaves;
			sound.waveCount = 0;
			while ( pWaves )
			{
				sound.waveCount++;
				pWaves = pWaves->GetNextKey();
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "position" ) )
		{
			positionIndex = params.startingPosition + pKey->GetInt();
		}
		pKey = pKey->GetNextKey();
	}

	if ( positionIndex < 0 )
	{
		positionIndex = params.ambientPositionOverride;
	}
	else if ( params.positionOverride >= 0 )
	{
		positionIndex = params.positionOverride;
	}

	if ( sound.waveCount != 0 )
	{
		if ( positionIndex < 0 )
		{
			sound.isAmbient = true;
			AddRandomSound( sound );
		}
		else
		{
			if ( positionIndex > 31 || !(m_params.localBits & (1<<positionIndex) ) )
			{
				// suppress sounds if the position isn't available
				//DevMsg( 1, "Bad position %d\n", positionIndex );
				return;
			}
			sound.isAmbient = false;
			sound.position = m_params.localSound[positionIndex];
			AddRandomSound( sound );
		}
	}
}

void C_SoundscapeSystem::ProcessPlaySoundscape( KeyValues *pPlaySoundscape, const subsoundscapeparams_t &paramsIn )
{
	subsoundscapeparams_t subParams = paramsIn;
	
	// sub-soundscapes NEVER set the DSP effects
	subParams.allowDSP = false;
	subParams.recurseLevel++;
	if ( subParams.recurseLevel > MAX_SOUNDSCAPE_RECURSION )
	{
		DevMsg( "Error!  Soundscape recursion overrun!\n" );
		return;
	}
	KeyValues *pKey = pPlaySoundscape->GetFirstSubKey();
	const char *pSoundscapeName = NULL;
	while ( pKey )
	{
		if ( !Q_strcasecmp( pKey->GetName(), "volume" ) )
		{
			subParams.masterVolume = paramsIn.masterVolume * RandomInterval( ReadInterval( pKey->GetString() ) );
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "position" ) )
		{
			subParams.startingPosition = paramsIn.startingPosition + pKey->GetInt();
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "positionoverride" ) )
		{
			if ( paramsIn.positionOverride < 0 )
			{
				subParams.positionOverride = paramsIn.startingPosition + pKey->GetInt();
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "ambientpositionoverride" ) )
		{
			if ( paramsIn.ambientPositionOverride < 0 )
			{
				subParams.ambientPositionOverride = paramsIn.startingPosition + pKey->GetInt();
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "name" ) )
		{
			pSoundscapeName = pKey->GetString();
		}
		pKey = pKey->GetNextKey();
	}

	if ( pSoundscapeName )
	{
		KeyValues *pSoundscapeKeys = FindSoundscapeByName( pSoundscapeName );
		if ( pSoundscapeKeys )
		{
			StartSubSoundscape( pSoundscapeKeys, subParams );
		}
	}
}


// special kind of looping sound with no spatialization
int C_SoundscapeSystem::AddLoopingAmbient( const char *pSoundName, float volume, int pitch )
{
	return AddLoopingSound( pSoundName, true, volume, SNDLVL_NORM, pitch, vec3_origin );
}

// add a looping sound to the list
// NOTE: will reuse existing entry (fade from current volume) if possible
//		this prevents pops
int C_SoundscapeSystem::AddLoopingSound( const char *pSoundName, bool isAmbient, float volume, soundlevel_t soundlevel, int pitch, const Vector &position )
{
	loopingsound_t *pSoundSlot = NULL;
	int soundSlot = m_loopingSounds.Count() - 1;
	while ( soundSlot >= 0 )
	{
		loopingsound_t &sound = m_loopingSounds[soundSlot];
		if ( sound.id != m_loopingSoundId && sound.isAmbient == isAmbient && sound.pitch == pitch && 
			!Q_strcasecmp( pSoundName, sound.pWaveName ) && sound.position == position )
		{
			// reuse this sound
			pSoundSlot = &sound;
			break;
		}
		soundSlot--;
	}

	if ( soundSlot < 0 )
	{
		// can't find the sound in the list, make a new one
		soundSlot = m_loopingSounds.AddToTail();
		if ( isAmbient )
		{
			// start at 0 and fade in
			enginesound->EmitAmbientSound( pSoundName, 0, pitch );
			m_loopingSounds[soundSlot].volumeCurrent = 0.0;
		}
		else
		{
			// non-ambients at 0 volume are culled, so start at 0.05
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, CHAN_STATIC, pSoundName, 0.05, soundlevel, 0, pitch, &position );
			m_loopingSounds[soundSlot].volumeCurrent = 0.05;
		}
	}
	loopingsound_t &sound = m_loopingSounds[soundSlot];
	// fill out the slot
	sound.pWaveName = pSoundName;
	sound.volumeTarget = volume;
	sound.pitch = pitch;
	sound.id = m_loopingSoundId;
	sound.isAmbient = isAmbient;
	sound.position = position;
	sound.soundlevel = soundlevel;

	return soundSlot;
}

// stop this loop forever
void C_SoundscapeSystem::StopLoopingSound( loopingsound_t &loopSound )
{
	if ( loopSound.isAmbient )
	{
		enginesound->EmitAmbientSound( loopSound.pWaveName, 0, 0, SND_STOP );
	}
	else
	{
		C_BaseEntity::StopSound( SOUND_FROM_WORLD, CHAN_STATIC, loopSound.pWaveName );
	}
}

// update with new volume
void C_SoundscapeSystem::UpdateLoopingSound( loopingsound_t &loopSound )
{
	if ( loopSound.isAmbient )
	{
		enginesound->EmitAmbientSound( loopSound.pWaveName, loopSound.volumeCurrent, loopSound.pitch, SND_CHANGE_VOL );
	}
	else
	{
		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, CHAN_STATIC, loopSound.pWaveName, loopSound.volumeCurrent, loopSound.soundlevel, SND_CHANGE_VOL, loopSound.pitch, &loopSound.position );
	}
}

// add a recurring random sound event
int C_SoundscapeSystem::AddRandomSound( const randomsound_t &sound )
{
	int index = m_randomSounds.AddToTail( sound );
	m_randomSounds[index].nextPlayTime = gpGlobals->curtime + 0.5 * RandomInterval( sound.time );
	
	return index;
}

// play a random sound randomly from this parameterization table
void C_SoundscapeSystem::PlayRandomSound( const randomsound_t &sound )
{
	Assert( sound.waveCount > 0 );

	int waveId = random->RandomInt( 0, sound.waveCount-1 );
	KeyValues *pWaves = sound.pWaves;
	while ( waveId > 0 && pWaves )
	{
		pWaves = pWaves->GetNextKey();
		waveId--;
	}
	if ( !pWaves )
		return;
	
	const char *pWaveName = pWaves->GetString();
	
	if ( !pWaveName )
		return;

	if ( sound.isAmbient )
	{
		enginesound->EmitAmbientSound( pWaveName, sound.masterVolume * RandomInterval( sound.volume ), (int)RandomInterval( sound.pitch ) );
	}
	else
	{
		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_WORLD, CHAN_STATIC, pWaveName, 
			sound.masterVolume * RandomInterval( sound.volume ), RandomInterval( sound.soundlevel ), 0, 
			(int)RandomInterval( sound.pitch ), &sound.position );
	}
}

// walk the list of random sound commands and update
void C_SoundscapeSystem::UpdateRandomSounds( float gameTime )
{
	if ( gameTime < m_nextRandomTime )
		return;

	m_nextRandomTime = gameTime + 3600;	// add some big time to check again (an hour)

	for ( int i = m_randomSounds.Count()-1; i >= 0; i-- )
	{
		// time to play?
		if ( gameTime >= m_randomSounds[i].nextPlayTime )
		{
			// UNDONE: add this in to fix range?
			// float dt = m_randomSounds[i].nextPlayTime - gameTime;
			PlayRandomSound( m_randomSounds[i] );

			// now schedule the next occurrance
			// UNDONE: add support for "play once" sounds? FastRemove() here.
			m_randomSounds[i].nextPlayTime = gameTime + RandomInterval( m_randomSounds[i].time );
		}

		// update next time to check the queue
		if ( m_randomSounds[i].nextPlayTime < m_nextRandomTime )
		{
			m_nextRandomTime = m_randomSounds[i].nextPlayTime;
		}
	}
}

