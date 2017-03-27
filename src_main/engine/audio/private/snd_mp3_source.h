//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SND_MP3_SOURCE_H
#define SND_MP3_SOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "snd_audio_source.h"

class IWaveData;
class CAudioMixer;

class CAudioSourceMP3 : public CAudioSource
{
public:

	CAudioSourceMP3( const char *pFileName );
	// Create an instance (mixer) of this audio source
	virtual CAudioMixer			*CreateMixer( void ) = 0;
	
	// Provide samples for the mixer. You can point pData at your own data, or if you prefer to copy the data,
	// you can copy it into copyBuf and set pData to copyBuf.
	virtual int					GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] ) = 0;
	
	virtual int					SampleRate( void ) { return m_sampleRate; }

	// Returns true if the source is a voice source.
	// This affects the voice_overdrive behavior (all sounds get quieter when
	// someone is speaking).
	virtual bool				IsVoiceSource() { return false; }
	virtual int					SampleSize( void ) { return 1; }

	// Total number of samples in this source.  NOTE: Some sources are infinite (mic input), they should return
	// a count equal to one second of audio at their current rate.
	virtual int					SampleCount( void ) { return m_fileSize; }
	

	virtual bool				IsLooped( void ) { return false; }
	virtual bool				IsStereoWav( void ) { return false; }
	virtual bool				IsStreaming( void ) { return false; } 
	virtual bool				IsCached( void ) { return true; }
	virtual void				CacheLoad( void ) {}
	virtual void				CacheUnload( void ) {}
	virtual void				CacheTouch( void ) {}
	virtual CSentence			*GetSentence( void ) { return NULL; }

	virtual int					ZeroCrossingBefore( int sample ) { return sample; }
	virtual int					ZeroCrossingAfter( int sample ) { return sample; }
	
	// mixer's references
	virtual void				ReferenceAdd( CAudioMixer *pMixer );
	virtual void				ReferenceRemove( CAudioMixer *pMixer );
	// check reference count, return true if nothing is referencing this
	virtual bool				CanDelete( void );

protected:

	const char *m_pName;
	int m_sampleRate;
	int	m_fileSize;
	int m_refCount;
};

bool Audio_IsMP3( const char *pName );
CAudioSource *Audio_CreateStreamedMP3( const char *pName );
CAudioSource *Audio_CreateMemoryMP3( const char *pName );
CAudioMixer *CreateMP3Mixer( IWaveData *data );

#endif // SND_MP3_SOURCE_H
