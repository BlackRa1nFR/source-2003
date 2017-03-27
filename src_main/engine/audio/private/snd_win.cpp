
#include <windows.h>
#include "tier0/dbg.h"
#include "convar.h"
#include "sound.h"
#include "sound_private.h"

#include "snd_device.h"
#include "snd_dev_direct.h"
#include "snd_dev_wave.h"
#include "snd_sfx.h"
#include "snd_audio_source.h"
#include "voice_sound_engine_interface.h"
#include "snd_channels.h"
#include "snd_wave_source.h"

#include "SoundService.h"


// In other C files.
extern ConVar s_surround;


qboolean	snd_firsttime = true;

#if DEAD
// starts at 0 for disabled
static int	snd_buffer_count = 0;
static int	snd_sent, snd_completed;
#endif



/* 
 * Global variables. Must be visible to window-procedure function 
 *  so it can unlock and free the data block after it has been played. 
 */ 
IAudioDevice	*g_AudioDevice = NULL;

/////////////////////////////////////////////////////////////////////////////////////////////

class CAudioDeviceNull : public IAudioDevice
{
public:
	bool		IsActive( void ) { return false; }

	bool		Init( void ) { return true; }
	void		Shutdown( void ) {}
	int			GetOutputPosition( void ) { return 0; }
	void		Pause( void ) {} 
	void		UnPause( void ) {}
	
	float		MixDryVolume( void ) { return 0; }
	bool		Should3DMix( void ) { return false; }

	void		SetActivation( bool active ) {}
	void		StopAllSounds( void ) {}

	int			PaintBegin( int, int ) { return 0; }
	void		PaintEnd( void ) {}
	void		ClearBuffer( void ) {}
	void		UpdateListener( const Vector&, const Vector&, const Vector&, const Vector& ) {}

	void		MixBegin( int ) {}
	void		MixUpsample( int sampleCount, int filtertype ) {}
	void		ChannelReset( int, int, float ) {}
	void		TransferSamples( int end ) {}
	void		SpatializeChannel( int volume[4], int master_vol, const Vector& sourceDir, float gain, float dotRight, float dotFront ) {}
	void		ApplyDSPEffects( int idsp, portable_samplepair_t *pbuffront, portable_samplepair_t *pbufrear, int samplecount ) {}
	
	// return some bogus data here
	const char *DeviceName( void )			{ return "Audio Disabled"; }
	int			DeviceChannels( void )		{ return 2; }
	int			DeviceSampleBits( void )	{ return 16; }
	int			DeviceSampleBytes( void )	{ return 2; }
	int			DeviceDmaSpeed( void )		{ return SOUND_DMA_SPEED; }
	int			DeviceSampleCount( void )	{ return 0; }
	void		*DeviceLockBuffer( void ) { return NULL; }
	void		DeviceUnlockBuffer( void * ) {}


	// sink sound data
	void		Mix8Mono( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress ) {}
	void		Mix8Stereo( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress ) {}
	void		Mix16Mono( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress ) {}
	void		Mix16Stereo( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress ) {}
};


IAudioDevice *Audio_GetNullDevice( void )
{
	// singeton device here
	static CAudioDeviceNull nullDevice;
	
	return &nullDevice;
}


/*
==================
S_BlockSound
==================
*/
void S_BlockSound (void)
{
	if ( !g_AudioDevice )
		return;

	g_AudioDevice->Pause();
}


/*
==================
S_UnblockSound
==================
*/
void S_UnblockSound (void)
{
	if ( !g_AudioDevice )
		return;

	g_AudioDevice->UnPause();
}

/*
==============
S_LoadSound
Check to see if wave data is in the cache. If so, return pointer to data.
If not, allocate cache space for wave data, load wave file into temporary heap
space, and dump/convert file data into cache.
==============
*/
CAudioSource *S_LoadSound( CSfxTable *s, channel_t *ch )
{
	if ( !s->pSource )
	{
		// Names that begin with "*" are streaming.
		// Skip over the * and create a streamed source
		if ( TestSoundChar( s->getname(), CHAR_STREAM ))
		{
			s->pSource = Audio_CreateStreamedWave( PSkipSoundChars(s->getname()) );
		}
		else
		{
			if ( TestSoundChar( s->getname(), CHAR_USERVOX ))
			{
				s->pSource = Voice_SetupAudioSource(ch->soundsource, ch->entchannel);
			}
			else 
			{
				// These are loaded into memory directly - skip sound characters
				s->pSource = Audio_CreateMemoryWave( PSkipSoundChars(s->getname()) );
			}
			
		}
	}

	if ( !s->pSource )
		return NULL;

#if 0	// 44k: UNDONE if hisound is 0, unload any 44khz sounds
	if ( s->pSource->IsCached() )
	{
		if ( hisound.GetInt() == 0 && s->pSource->SampleRate() == SOUND_44k )
		{
			// this sample was cached at high sample rate:
			// discard cached data and reload at new sample rate
			// NOTE: this can happen because the hisound cvar
			// is initialized from default.cfg AFTER local precaching
			if ( TestSoundChar(s->name, CHAR_USERVOX ))
			{
				DevMsg("S_LoadSound: Freeing voice data prematurely!\n");
			}
			s->pSource->CacheUnload();
		}
	}
#endif
	// 44k: - UNDONE, if hisound is 0 and wav is 44khz, downsample to 22khz when loading
	s->pSource->CacheLoad();
	
	// first time to load?  Create the mixer
	if ( ch && !ch->pMixer )
	{
		ch->pMixer = s->pSource->CreateMixer();
		if ( !ch->pMixer )
		{
			return NULL;
		}
	}

	return s->pSource;
}



/*
==================
AutoDetectInit

Try to find a sound device to mix for.
Returns a CAudioNULLDevice if nothing is found.
==================
*/
IAudioDevice *IAudioDevice::AutoDetectInit( bool waveOnly )
{
	IAudioDevice *pDevice = NULL;

	// JAY: UNDONE: Handle fake sound device here "fakedma"
//	if (g_pSoundServices->CheckParm("-simsound"))
//		fakedma = true;

	if ( waveOnly )
	{
		pDevice = Audio_CreateWaveDevice();
		if ( !pDevice )
			goto NULLDEVICE;
	}

	if ( !pDevice )
	{
		/* Init DirectSound */
		if ( g_pSoundServices->IsDirectSoundSupported() )
		{
			if (snd_firsttime)
			{
				pDevice = Audio_CreateDirectSoundDevice();
			}
		}
	}

// if DirectSound didn't succeed in initializing, try to initialize
// waveOut sound, unless DirectSound failed because the hardware is
// already allocated (in which case the user has already chosen not
// to have sound)

	// UNDONE: JAY: This doesn't test for the hardware being in use anymore, REVISIT
	if ( !pDevice )
	{
		pDevice = Audio_CreateWaveDevice();
	}


NULLDEVICE:

	snd_firsttime = false;

	if ( !pDevice )
	{
		if (snd_firsttime)
			DevMsg ("No sound device initialized\n");
		return Audio_GetNullDevice();
	}

	return pDevice;
}


/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
	g_AudioDevice->PaintEnd();
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown(void)
{
	if ( g_AudioDevice != Audio_GetNullDevice() )
	{
		if ( g_AudioDevice )
		{
			g_AudioDevice->Shutdown();
		}

		delete g_AudioDevice;

		// the NULL device is always valid
		g_AudioDevice = Audio_GetNullDevice();
	}
}

