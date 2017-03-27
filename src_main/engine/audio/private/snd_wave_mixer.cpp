//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include "convar.h"
#include "sound_private.h"
#include "snd_audio_source.h"
#include "snd_wave_source.h"
#include "snd_wave_mixer_private.h"
#include "snd_wave_mixer_adpcm.h"
#include "snd_channels.h"
#include "snd_device.h"
#include "snd_wave_data.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// These mixers provide an abstraction layer between the audio device and 
// mixing/decoding code.  They allow data to be decoded and mixed using 
// optimized, format sensitive code by calling back into the device that
// controls them.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: maps mixing to 8-bit mono mixer
//-----------------------------------------------------------------------------
class CAudioMixerWave8Mono : public CAudioMixerWave
{
public:
	CAudioMixerWave8Mono( IWaveData *data ) : CAudioMixerWave( data ) {}
	virtual void Mix( IAudioDevice *pDevice, channel_t *pChannel, void *pData, int outputOffset, int inputOffset, fixedint fracRate, int outCount, int timecompress )

	{
		pDevice->Mix8Mono( pChannel, (char *)pData, outputOffset, inputOffset, fracRate, outCount, timecompress );
	}
};

//-----------------------------------------------------------------------------
// Purpose: maps mixing to 8-bit stereo mixer
//-----------------------------------------------------------------------------
class CAudioMixerWave8Stereo : public CAudioMixerWave
{
public:
	CAudioMixerWave8Stereo( IWaveData *data ) : CAudioMixerWave( data ) {}
	virtual void Mix( IAudioDevice *pDevice, channel_t *pChannel, void *pData, int outputOffset, int inputOffset, fixedint fracRate, int outCount, int timecompress )

	{
		pDevice->Mix8Stereo( pChannel, (char *)pData, outputOffset, inputOffset, fracRate, outCount, timecompress );
	}
};

//-----------------------------------------------------------------------------
// Purpose: maps mixing to 16-bit mono mixer
//-----------------------------------------------------------------------------
class CAudioMixerWave16Mono : public CAudioMixerWave
{
public:
	CAudioMixerWave16Mono( IWaveData *data ) : CAudioMixerWave( data ) {}
	virtual void Mix( IAudioDevice *pDevice, channel_t *pChannel, void *pData, int outputOffset, int inputOffset, fixedint fracRate, int outCount, int timecompress )

	{
		pDevice->Mix16Mono( pChannel, (short *)pData, outputOffset, inputOffset, fracRate, outCount, timecompress );
	}
};


//-----------------------------------------------------------------------------
// Purpose: maps mixing to 16-bit stereo mixer
//-----------------------------------------------------------------------------
class CAudioMixerWave16Stereo : public CAudioMixerWave
{
public:
	CAudioMixerWave16Stereo( IWaveData *data ) : CAudioMixerWave( data ) {}
	virtual void Mix( IAudioDevice *pDevice, channel_t *pChannel, void *pData, int outputOffset, int inputOffset, fixedint fracRate, int outCount, int timecompress )

	{
		pDevice->Mix16Stereo( pChannel, (short *)pData, outputOffset, inputOffset, fracRate, outCount, timecompress );
	}
};


#define WAVE_FORMAT_PCM		1
#define WAVE_FORMAT_ADPCM	2

//-----------------------------------------------------------------------------
// Purpose: Create an approprite mixer type given the data format
// Input  : *data - data access abstraction
//			format - pcm or adpcm (1 or 2 -- RIFF format)
//			channels - number of audio channels (1 = mono, 2 = stereo)
//			bits - bits per sample
// Output : CAudioMixer * abstract mixer type that maps mixing to appropriate code
//-----------------------------------------------------------------------------
CAudioMixer *CreateWaveMixer( IWaveData *data, int format, int channels, int bits )
{
	if ( format == WAVE_FORMAT_PCM )
	{
		if ( channels > 1 )
		{
			if ( bits == 8 )
				return new CAudioMixerWave8Stereo( data );
			else
				return new CAudioMixerWave16Stereo( data );
		}
		else
		{
			if ( bits == 8 )
				return new CAudioMixerWave8Mono( data );
			else
				return new CAudioMixerWave16Mono( data );
		}
	}
	else if ( format == WAVE_FORMAT_ADPCM )
	{
		return CreateADPCMMixer( data );
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Init the base WAVE mixer.
// Input  : *data - data access object
//-----------------------------------------------------------------------------
CAudioMixerWave::CAudioMixerWave( IWaveData *data ) : m_pData(data)
{
	m_sample = 0;
	m_finished = false;
	m_forcedEndSample = 0;
	m_delaySamples = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Frees the data access object (we own it after construction)
//-----------------------------------------------------------------------------
CAudioMixerWave::~CAudioMixerWave( void )
{
	GetSource()->ReferenceRemove( this );
	delete m_pData;
}


//-----------------------------------------------------------------------------
// Purpose: Decode and read the data
//			by default we just pass the request on to the data access object
//			other mixers may need to buffer or decode the data for some reason
//
// Input  : **pData - dest pointer
//			sampleCount - number of samples needed
// Output : number of samples available in this batch
//-----------------------------------------------------------------------------
int	CAudioMixerWave::GetOutputData( void **pData, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
{
	return m_pData->ReadSourceData( pData, m_sample, sampleCount, copyBuf );
}


//-----------------------------------------------------------------------------
// Purpose: calls through the wavedata to get the audio source
// Output : CAudioSource
//-----------------------------------------------------------------------------
CAudioSource *CAudioMixerWave::GetSource( void )
{
	if ( m_pData )
		return &m_pData->Source();

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Gets the current sample location in playback
// Output : int (samples from start of wave)
//-----------------------------------------------------------------------------
int CAudioMixerWave::GetSamplePosition( void )
{
	return m_sample;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : delaySamples - 
//-----------------------------------------------------------------------------
void CAudioMixerWave::SetStartupDelaySamples( int delaySamples )
{
	m_delaySamples = delaySamples;
}

// Move the current position to newPosition
void CAudioMixerWave::SetSampleStart( int newPosition )
{
	CAudioSource *pSource = GetSource();
	if ( pSource )
		newPosition = pSource->ZeroCrossingAfter( newPosition );

	m_sample = newPosition;
}

// End playback at newEndPosition
void CAudioMixerWave::SetSampleEnd( int newEndPosition )
{
	// forced end of zero means play the whole sample
	if ( !newEndPosition )
		newEndPosition = 1;

	CAudioSource *pSource = GetSource();
	if ( pSource )
		newEndPosition = pSource->ZeroCrossingBefore( newEndPosition );

	// past current position?  limit.
	if ( newEndPosition < m_sample )
		newEndPosition = m_sample;

	m_forcedEndSample = newEndPosition;
}

//-----------------------------------------------------------------------------
// Purpose: The device calls this to request data.  The mixer must provide the
//			full amount of samples or have silence in its output stream.
// Input  : *pDevice - requesting device
//			sampleCount - number of samples at the output rate
//			outputRate - sampling rate of the request
// Output : Returns true to keep mixing, false to delete this mixer
//-----------------------------------------------------------------------------
int CAudioMixerWave::SkipSamples( int startSample, int sampleCount, int outputRate, int outputOffset )
{
	// shouldn't be playing this if finished, but return if we are
	if ( m_finished )
		return 0;

	// save this to compute total output
	int startingOffset = outputOffset;

	float inputRate = ( m_pData->Source().SampleRate());
	float rate = inputRate / outputRate;


	// If we are terminating this wave prematurely, then make sure we detect the limit
	if ( m_forcedEndSample )
	{
		// How many total input samples will we need?
		int samplesRequired = (int)(sampleCount * rate);
		// will this hit the end?
		if ( m_sample + samplesRequired >= m_forcedEndSample )
		{
			// yes, mark finished and truncate the sample request
			m_finished = true;
			sampleCount = (int)( (m_forcedEndSample - m_sample) / rate );
		}
	}

	while ( sampleCount > 0 )
	{
		bool advanceSample = true;
		int availableSamples, outputSampleCount;
		char *pData = NULL;

		// compute number of input samples required
		double end = m_sample + rate * sampleCount;
		int inputSampleCount = (int)(ceil(end) - floor(m_sample));

		// ask the source for the data
		char copyBuf[AUDIOSOURCE_COPYBUF_SIZE];

		if ( m_delaySamples > 0 )
		{
			int num_zero_samples = min( m_delaySamples, inputSampleCount );

			// Decrement data amount
			m_delaySamples -= num_zero_samples;

			int sampleSize = m_pData->Source().SampleSize();
			int readBytes = sampleSize * num_zero_samples;

			Assert( readBytes <= sizeof( copyBuf ) );

			pData = &copyBuf[0];
			// Now copy in some zeroes
			memset( pData, 0, readBytes );

			availableSamples = num_zero_samples;

			advanceSample = false;
		}
		else
		{
			availableSamples = GetOutputData((void**)&pData, inputSampleCount, copyBuf);
		}

		// none available, bail out
		if ( !availableSamples )
		{
			break;
		}

		double sampleFraction = m_sample - floor(m_sample);
		if ( availableSamples < inputSampleCount )
		{
			// How many samples are there given the number of input samples and the rate.
			outputSampleCount = (int)ceil((availableSamples - sampleFraction) / rate);
		}
		else
		{
			outputSampleCount = sampleCount;
		}

		// Verify that we won't get a buffer overrun.
		assert(floor(sampleFraction + rate * (outputSampleCount-1)) <= availableSamples);

		if ( advanceSample )
		{
			m_sample += outputSampleCount * rate;
		}
		outputOffset += outputSampleCount;
		sampleCount -= outputSampleCount;
	}

	// Did we run out of samples? if so, mark finished
	if ( sampleCount > 0 )
	{
		m_finished = true;
	}

	// total number of samples mixed !!! at the output clock rate !!!
	return outputOffset - startingOffset;
}

//-----------------------------------------------------------------------------
// Purpose: The device calls this to request data.  The mixer must provide the
//			full amount of samples or have silence in its output stream.
//			Mix channel to all active paintbuffers.
//			NOTE: cannot be called consecutively to mix into multiple paintbuffers!
// Input  : *pDevice - requesting device
//			sampleCount - number of samples at the output rate
//			outputRate - sampling rate of the request
// Output : Returns true to keep mixing, false to delete this mixer
//-----------------------------------------------------------------------------
int CAudioMixerWave::MixDataToDevice( IAudioDevice *pDevice, channel_t *pChannel, int sampleCount, int outputRate, int outputOffset )
{
	// shouldn't be playing this if finished, but return if we are
	if ( m_finished )
		return 0;

	// save this to compute total output
	int startingOffset = outputOffset;

	float inputRate = (pChannel->pitch * m_pData->Source().SampleRate());
	float rate = inputRate / outputRate;


	// If we are terminating this wave prematurely, then make sure we detect the limit
	if ( m_forcedEndSample )
	{
		// How many total input samples will we need?
		int samplesRequired = (int)(sampleCount * rate);
		// will this hit the end?
		if ( m_sample + samplesRequired >= m_forcedEndSample )
		{
			// yes, mark finished and truncate the sample request
			m_finished = true;
			sampleCount = (int)( (m_forcedEndSample - m_sample) / rate );
		}
	}

	while ( sampleCount > 0 )
	{
		bool advanceSample = true;
		int availableSamples, outputSampleCount;
		char *pData = NULL;

		// compute number of input samples required
		double end = m_sample + rate * sampleCount;
		int inputSampleCount = (int)(ceil(end) - floor(m_sample));

		// ask the source for the data
		char copyBuf[AUDIOSOURCE_COPYBUF_SIZE];

		if ( m_delaySamples > 0 )
		{
			int num_zero_samples = min( m_delaySamples, inputSampleCount );

			// Decrement data amount
			m_delaySamples -= num_zero_samples;

			int sampleSize = m_pData->Source().SampleSize();
			int readBytes = sampleSize * num_zero_samples;

			Assert( readBytes <= sizeof( copyBuf ) );

			pData = &copyBuf[0];
			// Now copy in some zeroes
			memset( pData, 0, readBytes );

			availableSamples = num_zero_samples;

			advanceSample = false;
		}
		else
		{
			availableSamples = GetOutputData((void**)&pData, inputSampleCount, copyBuf);
		}

		// none available, bail out
		if ( !availableSamples )
		{
			break;
		}

		double sampleFraction = m_sample - floor(m_sample);
		if ( availableSamples < inputSampleCount )
		{
			// How many samples are there given the number of input samples and the rate.
			outputSampleCount = (int)ceil((availableSamples - sampleFraction) / rate);
		}
		else
		{
			outputSampleCount = sampleCount;
		}

		// Verify that we won't get a buffer overrun.
		assert(floor(sampleFraction + rate * (outputSampleCount-1)) <= availableSamples);

		// mix this data to all active paintbuffers
		int i, j;
		
		// save current paintbuffer
		j = MIX_GetCurrentPaintbufferIndex();
		
		for (i = 0 ; i < CPAINTBUFFERS; i++)
		{
			if (paintbuffers[i].factive)
			{
				// mix chan into all active paintbuffers
				MIX_SetCurrentPaintbuffer(i);

				Mix( 
					pDevice,						// Device.
					pChannel,						// Channel.
					pData,							// Input buffer.
					outputOffset,					// Output position.
					FIX_FLOAT(sampleFraction),		// Iterators.
					FIX_FLOAT(rate), 
					outputSampleCount,	
					0 
					);
			}
		}
		MIX_SetCurrentPaintbuffer(j);

		if ( advanceSample )
		{
			m_sample += outputSampleCount * rate;
		}
		outputOffset += outputSampleCount;
		sampleCount -= outputSampleCount;
	}

	// Did we run out of samples? if so, mark finished
	if ( sampleCount > 0 )
	{
		m_finished = true;
	}

	// total number of samples mixed !!! at the output clock rate !!!
	return outputOffset - startingOffset;
}


bool CAudioMixerWave::ShouldContinueMixing( void )
{
	return !m_finished;
}

float CAudioMixerWave::ModifyPitch( float pitch )
{
	return pitch;
}

float CAudioMixerWave::GetVolumeScale( void )
{
	return 1.0f;
}

