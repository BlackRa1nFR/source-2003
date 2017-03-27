//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================


#include "sound_private.h"
#include "snd_mp3_source.h"
#include "snd_wave_mixer_private.h"
#include "snd_device.h"
#include "snd_wave_data.h"
#include "vaudio/ivaudio.h"

extern IVAudio *vaudio;

static const int MP3_BUFFER_SIZE = 16384;

//-----------------------------------------------------------------------------
// Purpose: Mixer for ADPCM encoded audio
//-----------------------------------------------------------------------------
class CAudioMixerWaveMP3 : public CAudioMixerWave, public IAudioStreamEvent
{
public:
	CAudioMixerWaveMP3( IWaveData *data );
	~CAudioMixerWaveMP3( void );
	
	virtual void Mix( IAudioDevice *pDevice, channel_t *pChannel, void *pData, int outputOffset, int inputOffset, fixedint fracRate, int outCount, int timecompress );
	virtual int	 GetOutputData( void **pData, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] );

	// need to override this to fixup blocks
	void SetSampleStart( int newPosition );

	// IAudioStreamEvent
	virtual int StreamRequestData( void *pBuffer, int bytesRequested, int offset );

	virtual void SetStartupDelaySamples( int delaySamples );

private:
	bool					DecodeBlock( void );

	IAudioStream			*m_pStream;
	char					m_samples[MP3_BUFFER_SIZE];
	int						m_sampleCount;
	int						m_samplePosition;
	int						m_channelCount;
	int						m_offset;
	int						m_delaySamples;
};


CAudioMixerWaveMP3::CAudioMixerWaveMP3( IWaveData *data ) : CAudioMixerWave( data ) 
{
	m_sampleCount = 0;
	m_samplePosition = 0;
	m_offset = 0;
	m_delaySamples = 0;
	m_pStream = vaudio->CreateMP3StreamDecoder( static_cast<IAudioStreamEvent *>(this) );
	m_channelCount = m_pStream->GetOutputChannels();
	Assert( m_pStream->GetOutputRate() == m_pData->Source().SampleRate() );
}


CAudioMixerWaveMP3::~CAudioMixerWaveMP3( void )
{
}


void CAudioMixerWaveMP3::Mix( IAudioDevice *pDevice, channel_t *pChannel, void *pData, int outputOffset, int inputOffset, fixedint fracRate, int outCount, int timecompress )
{
	if ( m_channelCount == 1 )
		pDevice->Mix16Mono( pChannel, (short *)pData, outputOffset, inputOffset, fracRate, outCount, timecompress );
	else
		pDevice->Mix16Stereo( pChannel, (short *)pData, outputOffset, inputOffset, fracRate, outCount, timecompress );
}


int CAudioMixerWaveMP3::StreamRequestData( void *pBuffer, int bytesRequested, int offset )
{
	if ( offset < 0 )
	{
		offset = m_offset;
	}
	else
	{
		m_offset = offset;
	}
	// read the data out of the source
	int totalBytesRead = 0;

	while ( bytesRequested > 0 )
	{
		char *pOutputBuffer = (char *)pBuffer;
		pOutputBuffer += totalBytesRead;

		void *pData = NULL;
		int bytesRead = m_pData->ReadSourceData( &pData, offset + totalBytesRead, bytesRequested, pOutputBuffer );
		
		if ( !bytesRead )
			break;
		if ( bytesRead > bytesRequested )
		{
			bytesRead = bytesRequested;
		}
		// if the source is buffering it, copy it to the MP3 decomp buffer
		if ( pData != pOutputBuffer )
		{
			memcpy( pOutputBuffer, pData, bytesRead );
		}
		totalBytesRead += bytesRead;
		bytesRequested -= bytesRead;
	}

	m_offset += totalBytesRead;
	return totalBytesRead;
}

bool CAudioMixerWaveMP3::DecodeBlock()
{
	m_sampleCount = m_pStream->Decode( m_samples, sizeof(m_samples) );
	m_samplePosition = 0;
	return m_sampleCount > 0;
}

//-----------------------------------------------------------------------------
// Purpose: Read existing buffer or decompress a new block when necessary
// Input  : **pData - output data pointer
//			sampleCount - number of samples (or pairs)
// Output : int - available samples (zero to stop decoding)
//-----------------------------------------------------------------------------
int CAudioMixerWaveMP3::GetOutputData( void **pData, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] )
{
	if ( m_samplePosition >= m_sampleCount )
	{
		if ( !DecodeBlock() )
			return 0;
	}

	if ( m_samplePosition < m_sampleCount )
	{
		int sampleSize = m_channelCount * 2;
		*pData = (void *)(m_samples + m_samplePosition);
		int available = m_sampleCount - m_samplePosition;
		int bytesRequired = sampleCount * sampleSize;
		if ( available > bytesRequired )
			available = bytesRequired;

		m_samplePosition += available;
		return available / sampleSize;
	}

	return 0;
}



//-----------------------------------------------------------------------------
// Purpose: Seek to a new position in the file
//			NOTE: In most cases, only call this once, and call it before playing
//			any data.
// Input  : newPosition - new position in the sample clocks of this sample
//-----------------------------------------------------------------------------
void CAudioMixerWaveMP3::SetSampleStart( int newPosition )
{
	// UNDONE: Implement this?
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : delaySamples - 
//-----------------------------------------------------------------------------
void CAudioMixerWaveMP3::SetStartupDelaySamples( int delaySamples )
{
	m_delaySamples = delaySamples;
}

//-----------------------------------------------------------------------------
// Purpose: Abstract factory function for ADPCM mixers
// Input  : *data - wave data access object
//			channels - 
// Output : CAudioMixer
//-----------------------------------------------------------------------------
CAudioMixer *CreateMP3Mixer( IWaveData *data )
{
	return new CAudioMixerWaveMP3( data );
}
