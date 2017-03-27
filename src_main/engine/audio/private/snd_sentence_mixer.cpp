#include "convar.h"

#include "vox_private.h"

#include "sound_private.h"
#include "snd_audio_source.h"
#include "snd_wave_source.h"
#include "snd_channels.h"
#include "snd_sfx.h"


//-----------------------------------------------------------------------------
// Purpose: This replaces the old sentence logic that was integrated with the
//			sound code.  Now it is a hierarchical mixer.
//-----------------------------------------------------------------------------
class CSentenceMixer : public CAudioMixer
{
public:
	CSentenceMixer( voxword_t *pwords );
	~CSentenceMixer( void );

	// return number of samples mixed
	virtual int MixDataToDevice( IAudioDevice *pDevice, channel_t *pChannel, int sampleCount, int outputRate, int outputOffset );
	virtual int SkipSamples( int startSample, int sampleCount, int outputRate, int outputOffset );
	virtual bool ShouldContinueMixing( void );

	virtual CAudioSource *GetSource( void );
	
	// get the current position (next sample to be mixed)
	virtual int GetSamplePosition( void );
	virtual float ModifyPitch( float pitch );
	virtual float GetVolumeScale( void );

	// BUGBUG: These are only applied to the current word, not the whole sentence!!!!
	virtual void SetSampleStart( int newPosition );
	virtual void SetSampleEnd( int newEndPosition );

	virtual void SetStartupDelaySamples( int delaySamples );
private:
	void				LoadWord( void );
	void				FreeWord( void );

	CAudioMixer			*m_pCurrentWord;
	int					m_wordIndex;
	voxword_t			m_words[CVOXWORDMAX];		// UNDONE: Dynamically allocate this?
};


CAudioMixer *CreateSentenceMixer( voxword_t *pwords )
{
	if ( pwords )
		return new CSentenceMixer(pwords);

	return NULL;
}


CSentenceMixer::CSentenceMixer( voxword_t *pwords )
{
	// copy each pointer in the sfx temp array into the
	// sentence array, and set the channel to point to the
	// sentence array
	int j = 0;
	while( pwords[j].sfx != NULL )
		m_words[j] = pwords[j++];
		
	m_words[j].sfx = NULL;

	m_wordIndex = 0;
	m_pCurrentWord = NULL;
	LoadWord();
}


CSentenceMixer::~CSentenceMixer( void )
{
	if ( m_pCurrentWord )
		delete m_pCurrentWord;
}


bool CSentenceMixer::ShouldContinueMixing( void )
{
	if ( m_pCurrentWord )
		return true;

	return false;
}

CAudioSource *CSentenceMixer::GetSource( void )
{
	if ( m_pCurrentWord )
		return m_pCurrentWord->GetSource();
	return NULL;
}
	
// get the current position (next sample to be mixed)
int CSentenceMixer::GetSamplePosition( void )
{
	if ( m_pCurrentWord )
		return m_pCurrentWord->GetSamplePosition();
	return 0;
}

void CSentenceMixer::SetSampleStart( int newPosition )
{
	if ( m_pCurrentWord )
	{
		m_pCurrentWord->SetSampleStart( newPosition );
	}
}

// End playback at newEndPosition
void CSentenceMixer::SetSampleEnd( int newEndPosition )
{
	if ( m_pCurrentWord )
	{
		m_pCurrentWord->SetSampleEnd( newEndPosition );
	}
}

void CSentenceMixer::SetStartupDelaySamples( int delaySamples )
{
	if ( m_pCurrentWord )
	{
		m_pCurrentWord->SetStartupDelaySamples( delaySamples );
	}
}

void CSentenceMixer::FreeWord( void )
{
	delete m_pCurrentWord;
	m_pCurrentWord = NULL;

	if ( m_words[m_wordIndex].sfx )
	{
		// If this wave wasn't precached by the game code
		if (!m_words[m_wordIndex].fKeepCached)
		{
			// If this was the last mixer that had a reference
			if ( m_words[m_wordIndex].sfx->pSource->CanDelete() )
			{
				// free the source
				delete m_words[m_wordIndex].sfx->pSource;
				m_words[m_wordIndex].sfx->pSource = NULL;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make sure the current word is loaded
//-----------------------------------------------------------------------------
void CSentenceMixer::LoadWord( void )
{
	if ( m_words[m_wordIndex].sfx )
	{
		CAudioSource *pSource = S_LoadSound( m_words[m_wordIndex].sfx, NULL );
		if ( pSource )
		{
			m_pCurrentWord = pSource->CreateMixer();
			if ( m_pCurrentWord )
			{
				int start = m_words[m_wordIndex].start;
				int end = m_words[m_wordIndex].end;
				
				// don't allow overlapped ranges
				if ( end <= start )
					end = 0;

				if ( start || end )
				{
					int sampleCount = m_pCurrentWord->GetSource()->SampleCount();
					if ( start )
					{
						m_pCurrentWord->SetSampleStart( (int)(sampleCount * 0.01f * start) );
					}
					if ( end )
					{
						m_pCurrentWord->SetSampleEnd( (int)(sampleCount * 0.01f * end) );
					}
				}
			}
		}
	}
}


float CSentenceMixer::ModifyPitch( float pitch )
{
	if ( m_pCurrentWord )
	{
		if ( m_words[m_wordIndex].pitch > 0 )
		{
			pitch += (m_words[m_wordIndex].pitch - 100) * 0.01f;
		}
	}
	return pitch;
}

float CSentenceMixer::GetVolumeScale( void )
{
	if ( m_pCurrentWord )
	{
		if ( m_words[m_wordIndex].volume )
		{
			float volume = m_words[m_wordIndex].volume * 0.01;
			if ( volume < 1.0f )
				return volume;
		}
	}
	return 1.0f;
}

int  CSentenceMixer::SkipSamples( int startSample, int sampleCount, int outputRate, int outputOffset )
{
	Assert( 0 );
	return 0;
}


// return number of samples mixed
int CSentenceMixer::MixDataToDevice( IAudioDevice *pDevice, channel_t *pChannel, int sampleCount, int outputRate, int outputOffset )
{
	if ( !m_pCurrentWord )
		return 0;

	// save this to compute total output
	int startingOffset = outputOffset;

	while ( sampleCount > 0 && m_pCurrentWord )
	{
		int outputCount = m_pCurrentWord->MixDataToDevice( pDevice, pChannel, sampleCount, outputRate, outputOffset );
		outputOffset += outputCount;
		sampleCount -= outputCount;
		if ( !m_pCurrentWord->ShouldContinueMixing() )
		{
			bool mouth = SND_IsMouth( pChannel );

			if ( mouth )
			{
				SND_ClearMouth( pChannel );
			}

			FreeWord();
			m_wordIndex++;
			LoadWord();
			if ( m_pCurrentWord )
			{
				pChannel->sfx = m_words[m_wordIndex].sfx;
				if ( mouth )
				{
					SND_UpdateMouth( pChannel );
				}
			}
		}
	}

	return outputOffset - startingOffset;
}
