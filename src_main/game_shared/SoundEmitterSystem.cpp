//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include <KeyValues.h>
#include "engine/IEngineSound.h"
#include "SoundEmitterSystemBase.h"
#include "igamesystem.h"

#ifndef CLIENT_DLL
#include "envmicrophone.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSoundEmitterSystem : public CSoundEmitterSystemBase, public CAutoGameSystem
{
public:
	CSoundEmitterSystem()
	{
	}

	// IServerSystem stuff
	virtual bool Init()
	{
		return BaseInit();
	}

	virtual void Shutdown()
	{
		BaseShutdown();
	}

#if !defined( CLIENT_DLL )
	// Precache all .wav files referenced in wave or rndwave keys
	virtual void LevelInitPreEntity()
	{
		int c = GetSoundCount();
		int i;

		for ( i = 0; i < c; i++ )
		{
			bool alwaysPrecache = GetShouldAlwaysPrecacheSound( i );
			if ( !alwaysPrecache )
				continue;

			CSoundParametersInternal *internal = InternalGetParametersForSound( i );
			if ( !internal )
				continue;

			if ( !internal->precache )
				continue;

			int waveCount = internal->soundnames.Count();

			if ( !waveCount )
			{
				DevMsg( "CSoundEmitterSystem:  sounds.txt entry '%s' has no waves listed under 'wave' or 'rndwave' key!!!\n",
					GetSoundName( i ));
			}
			else
			{
				for( int wave = 0; wave < waveCount; wave++ )
				{
					enginesound->PrecacheSound( GetWaveName( internal->soundnames[ wave ] ) );
				}
			}
		}
	}
#endif

	void PrecacheSoundScript( const char *scriptfile )
	{
		CUtlVector< const char * >	waves;

		// Build a list of indices to precache
		CUtlVector< int >	precacheList;

		BuildPrecacheSoundList( scriptfile, precacheList );

		int c = precacheList.Count();
		for ( int slot = 0; slot < c; slot++ )
		{
			int soundIndex = precacheList[ slot ];

			CSoundParametersInternal *internal = InternalGetParametersForSound( soundIndex );
			if ( !internal )
				continue;

			if ( !internal->precache )
				continue;

			int waveCount = internal->soundnames.Count();

			if ( !waveCount )
			{
				DevMsg( "CSoundEmitterSystem:  sounds.txt entry '%s' has no waves listed under 'wave' or 'rndwave' key!!!\n",
					GetSoundName( soundIndex ) );
			}
			else
			{
				for( int wave = 0; wave < waveCount; wave++ )
				{
					enginesound->PrecacheSound( GetWaveName( internal->soundnames[ wave ] ) );
				}
			}
		}
	}

	void PrecacheScriptSound( const char *soundname, bool preload )
	{
		int soundIndex = GetSoundIndex( soundname );
		if ( !IsValidIndex( soundIndex ) )
			return;

		CSoundParametersInternal *internal = InternalGetParametersForSound( soundIndex );
		if ( !internal )
			return;

		if ( !internal->precache )
			return;

		int waveCount = internal->soundnames.Count();

		if ( !waveCount )
		{
			DevMsg( "CSoundEmitterSystem:  sounds.txt entry '%s' has no waves listed under 'wave' or 'rndwave' key!!!\n",
				soundname );
		}
		else
		{
			for( int wave = 0; wave < waveCount; wave++ )
			{
				enginesound->PrecacheSound( GetWaveName( internal->soundnames[ wave ] ), preload );
			}
		}
	}

public:

	void EmitSound( IRecipientFilter& filter, int entindex, const char *soundname, float flVolume, int iFlags, int iPitch, const Vector *pOrigin, float soundtime /*= 0.0f*/ )
	{
		// Pull data from parameters
		CSoundParameters params;

		if ( !GetParametersForSound( soundname, params ) )
		{
			return;
		}

		if ( !params.soundname[0] )
			return;

		if ( !Q_strncasecmp( params.soundname, "vo", 2 ) &&
			!( params.channel == CHAN_STREAM ||
			   params.channel == CHAN_VOICE ) )
		{
			DevMsg( "EmitSound:  Voice .wav file %s doesn't specify CHAN_VOICE or CHAN_STREAM for sound %s\n",
				params.soundname, soundname );
		}

		// handle SND_CHANGEPITCH/SND_CHANGEVOL and other sound flags.etc.
		if( iFlags & SND_CHANGE_PITCH )
		{
			params.pitch = iPitch;
		}

		if( iFlags & SND_CHANGE_VOL )
		{
			params.volume = flVolume;
		}

#if !defined( CLIENT_DLL )
		// Loop through all registered microphones and tell them the sound was just played
		int iCount = g_Microphones.Count();
		bool bSwallowed = false;
		for ( int i = 0; i < iCount; i++ )
		{
			if ( g_Microphones[i] )
			{
				if ( g_Microphones[i]->SoundPlayed( entindex, params.soundname, params.soundlevel, params.volume, iFlags, params.pitch, pOrigin, soundtime ) )
				{
					// Microphone told us to swallow it
					bSwallowed = true;
				}
			}
		}
		if ( bSwallowed )
			return;
#endif

		enginesound->EmitSound( 
			filter, 
			entindex, 
			params.channel, 
			params.soundname,
			params.volume,
			(soundlevel_t)params.soundlevel,
			iFlags,
			params.pitch,
			pOrigin,
			NULL,
			true,
			soundtime );
	}

	void EmitAmbientSound( CBaseEntity *entity, const Vector& origin, const char *soundname, float flVolume, int iFlags, int iPitch, float soundtime /*= 0.0f*/ )
	{
		// Pull data from parameters
		CSoundParameters params;

		if ( !GetParametersForSound( soundname, params ) )
		{
			return;
		}

		if( iFlags & SND_CHANGE_PITCH )
		{
			params.pitch = iPitch;
		}

		if( iFlags & SND_CHANGE_VOL )
		{
			params.volume = flVolume;
		}

#if defined( CLIENT_DLL )
		enginesound->EmitAmbientSound( params.soundname, params.volume, params.pitch, iFlags, soundtime );
#else
		engine->EmitAmbientSound( entity->edict(), origin, params.soundname, params.volume, params.soundlevel, iFlags, params.pitch, soundtime );
#endif
	}

	void StopSound( int entindex, const char *soundname )
	{
		// Pull data from parameters
		CSoundParameters params;
		if ( !GetParametersForSound( soundname, params ) )
		{
			return;
		}

		if ( !params.soundname[ 0 ] )
			return;

		// TODO:  SND_CHANGEPITCH/SND_CHANGEVOL and other sound flags need persistent state to handle modulation, etc.
		enginesound->StopSound( 
			entindex, 
			params.channel, 
			params.soundname );
	}

	// HACK:  Pass all sound calls through here so that if the sample doesn't end in .wav, we can try to play it from
	// sounds.txt as a named token, instead
	void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, float flAttenuation, int iFlags, int iPitch, 
		const Vector *pOrigin, float soundtime /*= 0.0f*/ )
	{
		if ( pSample && ( Q_stristr( pSample, ".wav" ) || pSample[0] == '!' ) )
		{
			enginesound->EmitSound( filter, iEntIndex, iChannel, pSample, flVolume, flAttenuation, iFlags, iPitch, pOrigin, NULL, true, soundtime );
		}
		else
		{
			// Look it up in sounds.txt and ignore other parameters
			EmitSound( filter, iEntIndex, pSample, flVolume, iFlags, iPitch, pOrigin, soundtime );
		}
	}

	void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, soundlevel_t iSoundlevel, int iFlags, int iPitch, const Vector *pOrigin, float soundtime /*= 0.0f*/ )
	{
		if ( pSample && ( Q_stristr( pSample, ".wav" ) || pSample[0] == '!' ) )
		{
			enginesound->EmitSound( filter, iEntIndex, iChannel, pSample, flVolume, iSoundlevel, iFlags, iPitch, pOrigin, NULL, true, soundtime );
		}
		else
		{
			// Look it up in sounds.txt and ignore other parameters
			EmitSound( filter, iEntIndex, pSample, flVolume, iFlags, iPitch, pOrigin, soundtime );
		}
	}

	void StopSound( int iEntIndex, int iChannel, const char *pSample )
	{
		if ( pSample && ( Q_stristr( pSample, ".wav" ) || pSample[0] == '!' ) )
		{
			enginesound->StopSound( iEntIndex, iChannel, pSample );
		}
		else
		{
			// Look it up in sounds.txt and ignore other parameters
			StopSound( iEntIndex, pSample );
		}
	}

	void EmitAmbientSound( CBaseEntity *entity, const Vector &origin, const char *pSample, float volume, soundlevel_t soundlevel, int flags, int pitch, float soundtime /*= 0.0f*/ )
	{
		if ( pSample && Q_stristr( pSample, ".wav" ) )
		{
#if defined( CLIENT_DLL )
			enginesound->EmitAmbientSound( pSample, volume, pitch, flags, soundtime );
#else
			engine->EmitAmbientSound( entity->pev, origin, pSample, volume, soundlevel, flags, pitch, soundtime );
#endif
		}
		else
		{
			EmitAmbientSound( entity, origin, pSample, volume, flags, pitch, soundtime );
		}
	}
};

static CSoundEmitterSystem g_SoundEmitterSystem;

#if defined( CLIENT_DLL )
CON_COMMAND( cl_soundemitter_flush, "Flushes the sounds.txt system (client only)" )
#else
CON_COMMAND( sv_soundemitter_flush, "Flushes the sounds.txt system (server only)" )
#endif
{
	// save the current soundscape
	// kill the system
	g_SoundEmitterSystem.Shutdown();

	// restart the system
	g_SoundEmitterSystem.Init();

#if !defined( CLIENT_DLL )
	// Redo precache all .wav files... (this should work now that we have dynamic string tables)
	g_SoundEmitterSystem.LevelInitPreEntity();
#endif

	// TODO:  when we go to a handle system, we'll need to invalidate handles somehow
}

#if !defined( CLIENT_DLL )
CON_COMMAND( sv_soundemitter_filecheck, "Report missing .wav files for sounds and game_sounds files." )
{
	int missing = g_SoundEmitterSystem.CheckForMissingWavFiles( true );
	DevMsg( "---------------------------\nTotal missing files %i\n", missing );
}
#endif
//-----------------------------------------------------------------------------
// Purpose:  Non-static override for doing the general case of CPASAttenuationFilter( this ), and EmitSound( filter, entindex(), etc. );
// Input  : *soundname - 
//-----------------------------------------------------------------------------
void CBaseEntity::EmitSound( const char *soundname, float soundtime /*= 0.0f*/ )
{
	CPASAttenuationFilter filter( this, soundname );
	EmitSound( filter, entindex(), soundname, NULL, soundtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *soundname - 
//-----------------------------------------------------------------------------
void CBaseEntity::StopSound( const char *soundname )
{
	StopSound( entindex(), soundname );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : filter - 
//			iEntIndex - 
//			*soundname - 
//			*pOrigin - 
//-----------------------------------------------------------------------------
void CBaseEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, const char *soundname, const Vector *pOrigin /*= NULL*/, float soundtime /*= 0.0f*/ )
{
	g_SoundEmitterSystem.EmitSound( filter, iEntIndex, soundname, 0.0, 0, 0, pOrigin, soundtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iEntIndex - 
//			*soundname - 
//-----------------------------------------------------------------------------
void CBaseEntity::StopSound( int iEntIndex, const char *soundname )
{
	g_SoundEmitterSystem.StopSound( iEntIndex, soundname );
}

void CBaseEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
	float flVolume, float flAttenuation, int iFlags /* = 0*/, int iPitch /*= PITCH_NORM*/, 
	const Vector *pOrigin /*= NULL*/, float soundtime /*= 0.0f*/ )
{
	g_SoundEmitterSystem.EmitSound( filter, iEntIndex, iChannel, pSample, flVolume, flAttenuation, iFlags, iPitch, pOrigin, soundtime );
}

void CBaseEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
	float flVolume, soundlevel_t iSoundlevel, int iFlags /* = 0*/, int iPitch /*= PITCH_NORM*/, 
	const Vector *pOrigin /*= NULL*/, float soundtime /*= 0.0f*/ )
{
	g_SoundEmitterSystem.EmitSound( filter, iEntIndex, iChannel, pSample, flVolume, iSoundlevel, iFlags, iPitch, pOrigin, soundtime );
}

void CBaseEntity::StopSound( int iEntIndex, int iChannel, const char *pSample )
{
	g_SoundEmitterSystem.StopSound( iEntIndex, iChannel, pSample );
}

soundlevel_t CBaseEntity::LookupSoundLevel( const char *soundname )
{
	return g_SoundEmitterSystem.LookupSoundLevel( soundname );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *entity - 
//			origin - 
//			flags - 
//			*soundname - 
//-----------------------------------------------------------------------------
void CBaseEntity::EmitAmbientSound( CBaseEntity *entity, const Vector& origin, const char *soundname, int flags, float soundtime /*= 0.0f*/ )
{
	g_SoundEmitterSystem.EmitAmbientSound( entity, origin, soundname, 0.0, flags, 0, soundtime );
}

// HACK HACK:  Do we need to pull the entire SENTENCEG_* wrapper over to the client .dll?
#if defined( CLIENT_DLL )
int SENTENCEG_Lookup(const char *sample)
{
	return engine->SentenceIndexFromName( sample + 1 );
}
#endif

void UTIL_EmitAmbientSound( CBaseEntity *entity, const Vector &vecOrigin, const char *samp, float vol, soundlevel_t soundlevel, int fFlags, int pitch, float soundtime /*= 0.0f*/ )
{
	if (samp && *samp == '!')
	{
		int sentenceIndex = SENTENCEG_Lookup(samp);
		if (sentenceIndex >= 0)
		{
			char name[32];
			Q_snprintf( name, sizeof(name), "!%d", sentenceIndex );
#if !defined( CLIENT_DLL )
			engine->EmitAmbientSound( entity->pev, vecOrigin, name, vol, soundlevel, fFlags, pitch, soundtime );
#else
			enginesound->EmitAmbientSound( name, vol, pitch, fFlags, soundtime );
#endif
		}
	}
	else
	{
		g_SoundEmitterSystem.EmitAmbientSound( entity, vecOrigin, samp, vol, soundlevel, fFlags, pitch, soundtime );
	}
}

const char *UTIL_TranslateSoundName( const char *soundname )
{
	Assert( soundname );

	if ( Q_stristr( soundname, ".wav" ) )
		return soundname;

	return g_SoundEmitterSystem.GetWavFileForSound( soundname );
}

bool CBaseEntity::GetParametersForSound( const char *soundname, CSoundParameters &params )
{
	return g_SoundEmitterSystem.GetParametersForSound( soundname, params );
}

const char *CBaseEntity::GetWavFileForSound( const char *soundname )
{
	return g_SoundEmitterSystem.GetWavFileForSound( soundname );
}

void CBaseEntity::PrecacheSoundScript( const char *scriptfile )
{
// Ignore this on the client
#if !defined( CLIENT_DLL )
	g_SoundEmitterSystem.PrecacheSoundScript( scriptfile );
#endif
}

void CBaseEntity::PrecacheScriptSound( const char *soundname, bool preload /*= false*/ )
{
#if !defined( CLIENT_DLL )
	g_SoundEmitterSystem.PrecacheScriptSound( soundname, preload );
#endif
}
