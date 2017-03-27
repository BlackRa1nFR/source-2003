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
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef SND_WAVE_SOURCE_H
#define SND_WAVE_SOURCE_H
#pragma once

#include "snd_audio_source.h"
class IterateRIFF;
#include "sentence.h"

//=============================================================================
// Functions to create audio sources from wave files or from wave data.
//=============================================================================
extern CAudioSource* Audio_CreateMemoryWave( const char *pName );
extern CAudioSource* Audio_CreateStreamedWave( const char *pName );



class CAudioSourceWave : public CAudioSource
{
public:
	CAudioSourceWave( const char *pName );
	~CAudioSourceWave( void );

	void Setup( const char *pFormat, int formatSize, IterateRIFF &walk );

	virtual int				SampleRate( void );
	virtual int				SampleSize( void );
	virtual int				SampleCount( void );
	void					*GetHeader( void );
	virtual bool			IsVoiceSource();

	virtual	void			ParseChunk( IterateRIFF &walk, int chunkName );
	virtual void			ParseSentence( IterateRIFF &walk );

	void					ConvertSamples( char *pData, int sampleCount );
	bool					IsLooped( void );
	bool					IsStereoWav( void );
	bool					IsStreaming( void );
	bool					IsCached( void );
	int						ConvertLoopedPosition( int samplePosition );
	void					CacheLoad( void );
	void					CacheUnload( void );
	void					CacheTouch( void );
	virtual int				ZeroCrossingBefore( int sample );
	virtual int				ZeroCrossingAfter( int sample );
	virtual void			ReferenceAdd( CAudioMixer *pMixer );
	virtual void			ReferenceRemove( CAudioMixer *pMixer );
	virtual bool			CanDelete( void );
	virtual CSentence		*GetSentence( void );

protected:
	// returns the loop start from a cue chunk
	void					ParseCueChunk( IterateRIFF &walk );
	void					ParseSamplerChunk( IterateRIFF &walk );
	void					Init( const char *pHeaderBuffer, int headerSize );

	int				m_bits;
	int				m_rate;
	int				m_channels;
	int				m_format;
	int				m_sampleSize;
	int				m_loopStart;
	int				m_sampleCount;
	const char		*m_pName;

private:
	CAudioSourceWave( const CAudioSourceWave & ); // not implemented, not allowed
	char			*m_pHeader;
	int				m_refCount;

	CSentence		m_Sentence;
};


#endif // SND_WAVE_SOURCE_H
