//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 1)
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>			// adpcm format
#pragma warning(pop)

#include <assert.h>
#include <math.h>
#include "vector.h"
#include "convar.h"
#include "const.h"
#include "voice_sound_engine_interface.h"
#include "sound.h"
#include "snd_wave_source.h"
#include "snd_wave_mixer_private.h"
#include "voice.h"
#include "snd_sfx.h"
#include "soundflags.h"
#include "SoundService.h"
#include "ivoicecodec.h"
#include "snd_wave_data.h"

// ------------------------------------------------------------------------- //
// CAudioSourceVoice.
// This feeds the data from an incoming voice channel (a guy on the server
// who is speaking) into the sound engine.
// ------------------------------------------------------------------------- //

class CAudioSourceVoice : public CAudioSourceWave
{
public:
								CAudioSourceVoice(int iEntity);
								virtual ~CAudioSourceVoice();
	
	virtual CAudioMixer			*CreateMixer();
	virtual int					GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] );
	virtual int					SampleRate( void );
	
	// Sample size is in bytes.  It will not be accurate for compressed audio.  This is a best estimate.
	// The compressed audio mixers understand this, but in general do not assume that SampleSize() * SampleCount() = filesize
	// or even that SampleSize() is 100% accurate due to compression.
	virtual int					SampleSize( void );

	// Total number of samples in this source.  NOTE: Some sources are infinite (mic input), they should return
	// a count equal to one second of audio at their current rate.
	virtual int					SampleCount( void );

	virtual bool				IsVoiceSource()				{return true;}

	virtual bool				IsLooped()					{return false;}
	virtual bool				IsStreaming()				{return true;}
	virtual bool				IsStereoWav()				{return false;}
	virtual bool				IsCached()					{return true;}
	virtual void				CacheLoad()					{}
	virtual void				CacheUnload()				{}
	virtual void				CacheTouch()				{}
	virtual CSentence			*GetSentence()				{return NULL;}

	virtual int					ZeroCrossingBefore( int sample )	{return sample;}
	virtual int					ZeroCrossingAfter( int sample )		{return sample;}
	
	// mixer's references
	virtual void				ReferenceAdd( CAudioMixer *pMixer );
	virtual void				ReferenceRemove( CAudioMixer *pMixer );

	// check reference count, return true if nothing is referencing this
	virtual bool				CanDelete();


private:

	class CWaveDataVoice : public IWaveData
	{
	public:
							CWaveDataVoice( CAudioSourceWave &source ) : m_source(source) {}
							~CWaveDataVoice( void ) {}

		virtual CAudioSource &Source( void )
		{
			return m_source;
		}
		
		// this file is in memory, simply pass along the data request to the source
		virtual int ReadSourceData( void **pData, int sampleIndex, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
		{
			return m_source.GetOutputData( pData, sampleIndex, sampleCount, copyBuf );
		}
	private:
		CAudioSourceWave		&m_source;	// pointer to source
	};


private:
	CAudioSourceVoice( const CAudioSourceVoice & );

	// Which entity's voice this is for.
	int							m_iChannel;
	
	// How many mixers are referencing us.
	int							m_refCount;
};



// ----------------------------------------------------------------------------- //
// Globals.
// ----------------------------------------------------------------------------- //

// The format we sample voice in.
extern WAVEFORMATEX g_VoiceSampleFormat;

class CVoiceSfx : public CSfxTable
{
public:
	CVoiceSfx()
	{
		setname( "?VoiceSfx" );
	}
};

static CVoiceSfx g_CVoiceSfx[VOICE_NUM_CHANNELS];

static float	g_VoiceOverdriveDuration = 0;
static bool		g_bVoiceOverdriveOn = false;

// When voice is on, all other sounds are decreased by this factor.
static ConVar voice_overdrive( "voice_overdrive", "2" );
static ConVar voice_overdrivefadetime( "voice_overdrivefadetime", "0.4" ); // How long it takes to fade in and out of the voice overdrive.

// The sound engine uses this to lower all sound volumes.
// All non-voice sounds are multiplied by this and divided by 256.
int g_SND_VoiceOverdriveInt;



// ----------------------------------------------------------------------------- //
// CAudioSourceVoice implementation.
// ----------------------------------------------------------------------------- //

CAudioSourceVoice::CAudioSourceVoice( int iChannel )
	: CAudioSourceWave("")
{
	m_iChannel = iChannel;
	m_refCount = 0;

	Init((char*)&g_VoiceSampleFormat, sizeof(g_VoiceSampleFormat));
	m_sampleCount = g_VoiceSampleFormat.nSamplesPerSec;
}

CAudioSourceVoice::~CAudioSourceVoice()
{
	Voice_OnAudioSourceShutdown( m_iChannel );
}

CAudioMixer *CAudioSourceVoice::CreateMixer()
{
	CWaveDataVoice *pVoice = new CWaveDataVoice(*this);
	if(!pVoice)
		return NULL;

	CAudioMixer *pMixer = CreateWaveMixer(pVoice, WAVE_FORMAT_PCM, 1, BYTES_PER_SAMPLE*8);
	if(!pMixer)
	{
		delete pVoice;
		return NULL;
	}

	ReferenceAdd( pMixer );
	return pMixer;
}

int CAudioSourceVoice::GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
{
	int nSamplesGotten = Voice_GetOutputData(
		m_iChannel,
		copyBuf,
		AUDIOSOURCE_COPYBUF_SIZE,
		samplePosition,
		sampleCount );
	
	// If there weren't enough bytes in the received data channel, pad it with zeros.
	if( nSamplesGotten < sampleCount )
	{
		memset( &copyBuf[nSamplesGotten], 0, (sampleCount - nSamplesGotten) * BYTES_PER_SAMPLE );
		nSamplesGotten = sampleCount;
	}

	*pData = copyBuf;
	return nSamplesGotten;
}

int CAudioSourceVoice::SampleRate()
{
	return VOICE_OUTPUT_SAMPLE_RATE;
}

int CAudioSourceVoice::SampleSize()
{
	return BYTES_PER_SAMPLE;
}

int CAudioSourceVoice::SampleCount()
{
	return VOICE_OUTPUT_SAMPLE_RATE;
}

void CAudioSourceVoice::ReferenceAdd(CAudioMixer *pMixer)
{
	m_refCount++;
}

void CAudioSourceVoice::ReferenceRemove(CAudioMixer *pMixer)
{
	m_refCount--;
	if ( m_refCount <= 0 )
		delete this;
}

bool CAudioSourceVoice::CanDelete()
{
	return m_refCount == 0;
}


// ----------------------------------------------------------------------------- //
// Interface implementation.
// ----------------------------------------------------------------------------- //

bool VoiceSE_Init()
{
	if( !snd_initialized )
		return false;

	return true;
}

void VoiceSE_Term()
{
}


void VoiceSE_Idle(float frametime)
{
	g_SND_VoiceOverdriveInt = 256;

	if( g_bVoiceOverdriveOn )
	{
		g_VoiceOverdriveDuration = min( g_VoiceOverdriveDuration+frametime, voice_overdrivefadetime.GetFloat() );
	}
	else
	{
		if(g_VoiceOverdriveDuration == 0)
			return;

		g_VoiceOverdriveDuration = max(g_VoiceOverdriveDuration-frametime, 0);
	}

	float percent = g_VoiceOverdriveDuration / voice_overdrivefadetime.GetFloat();
	percent = (float)(-cos(percent * 3.1415926535) * 0.5 + 0.5);		// Smooth it out..
	float voiceOverdrive = 1 + (voice_overdrive.GetFloat() - 1) * percent;
	g_SND_VoiceOverdriveInt = (int)(256 / voiceOverdrive);
}


void VoiceSE_StartChannel(
	int iChannel	//! Which channel to start.
	)
{
	assert( iChannel >= 0 && iChannel < VOICE_NUM_CHANNELS );

	// Start the sound.
	CSfxTable *sfx = &g_CVoiceSfx[iChannel];
	sfx->pSource = NULL;
	Vector vOrigin(0,0,0);
	S_StartDynamicSound( g_pSoundServices->GetViewEntity(), (CHAN_VOICE_BASE+iChannel), sfx, vOrigin, 1.0, SNDLVL_IDLE, 0, PITCH_NORM );
}

void VoiceSE_EndChannel(
	int iChannel	//! Which channel to stop.
	)
{
	S_StopSound( g_pSoundServices->GetViewEntity(), CHAN_VOICE_BASE+iChannel );
}

void VoiceSE_StartOverdrive()
{
	g_bVoiceOverdriveOn = true;
}

void VoiceSE_EndOverdrive()
{
	g_bVoiceOverdriveOn = false;
}


void VoiceSE_InitMouth(int entnum)
{
}

void VoiceSE_CloseMouth(int entnum)
{
}

void VoiceSE_MoveMouth(int entnum, short *pSamples, int nSamples)
{
}


CAudioSource* Voice_SetupAudioSource( int soundsource, int entchannel )
{
	int iChannel = entchannel - CHAN_VOICE_BASE;
	if( iChannel >= 0 && iChannel < VOICE_NUM_CHANNELS )
		return new CAudioSourceVoice( iChannel );
	else
		return NULL;
}


