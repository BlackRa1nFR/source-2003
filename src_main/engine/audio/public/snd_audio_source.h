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

#ifndef SND_AUDIO_SOURCE_H
#define SND_AUDIO_SOURCE_H
#pragma once


#define AUDIOSOURCE_COPYBUF_SIZE	4096


struct channel_t;
class CSentence;

class CAudioSource;
class IAudioDevice;

//-----------------------------------------------------------------------------
// Purpose: This is an instance of an audio source.
//			Mixers are attached to channels and reference an audio source.
//			Mixers are specific to the sample format and source format.
//			Mixers are never re-used, so they can track instance data like
//			sample position, fractional sample, stream cache, faders, etc.
//-----------------------------------------------------------------------------
class CAudioMixer
{
public:
	virtual ~CAudioMixer( void ) {}

	// return number of samples mixed
	virtual int MixDataToDevice( IAudioDevice *pDevice, channel_t *pChannel, int sampleCount, int outputRate, int outputOffset ) = 0;
	virtual int SkipSamples( int startSample, int sampleCount, int outputRate, int outputOffset ) = 0;
	virtual bool ShouldContinueMixing( void ) = 0;

	virtual CAudioSource *GetSource( void ) = 0;
	
	// get the current position (next sample to be mixed)
	virtual int GetSamplePosition( void ) = 0;

	// Allow the mixer to modulate pitch and volume. 
	// returns a floating point modulator
	virtual float ModifyPitch( float pitch ) = 0;
	virtual float GetVolumeScale( void ) = 0;

	// NOTE: Playback is optimized for linear streaming.  These calls will usually cost performance
	// It is currently optimal to call them before any playback starts, but some audio sources may not
	// guarantee this.  Also, some mixers may choose to ignore these calls for internal reasons (none do currently).

	// Move the current position to newPosition 
	// BUGBUG: THIS CALL DOES NOT SUPPORT MOVING BACKWARD, ONLY FORWARD!!!
	virtual void SetSampleStart( int newPosition ) = 0;

	// End playback at newEndPosition
	virtual void SetSampleEnd( int newEndPosition ) = 0;

	// How many samples to skip before commencing actual data reading ( to allow sub-frametime sound
	//  offsets and avoid synchronizing sounds to various 100 msec clock intervals throughout the
	//  engine and game code)
	virtual void SetStartupDelaySamples( int delaySamples ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: A source is an abstraction for a stream, cached file, or procedural
//			source of audio.
//-----------------------------------------------------------------------------
class CAudioSource
{
public:
	virtual ~CAudioSource( void ) {}
	// Create an instance (mixer) of this audio source
	virtual CAudioMixer			*CreateMixer( void ) = 0;
	
	// Provide samples for the mixer. You can point pData at your own data, or if you prefer to copy the data,
	// you can copy it into copyBuf and set pData to copyBuf.
	virtual int					GetOutputData( void **pData, int samplePosition, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] ) = 0;
	
	virtual int					SampleRate( void ) = 0;

	// Returns true if the source is a voice source.
	// This affects the voice_overdrive behavior (all sounds get quieter when
	// someone is speaking).
	virtual bool				IsVoiceSource() = 0;
	
	// Sample size is in bytes.  It will not be accurate for compressed audio.  This is a best estimate.
	// The compressed audio mixers understand this, but in general do not assume that SampleSize() * SampleCount() = filesize
	// or even that SampleSize() is 100% accurate due to compression.
	virtual int					SampleSize( void ) = 0;

	// Total number of samples in this source.  NOTE: Some sources are infinite (mic input), they should return
	// a count equal to one second of audio at their current rate.
	virtual int					SampleCount( void ) = 0;

	virtual bool				IsLooped( void ) = 0;
	virtual bool				IsStereoWav( void ) = 0;
	virtual bool				IsStreaming( void ) = 0;
	virtual bool				IsCached( void ) = 0;
	virtual void				CacheLoad( void ) = 0;
	virtual void				CacheUnload( void ) = 0;
	virtual void				CacheTouch( void ) = 0;
	virtual CSentence			*GetSentence( void ) = 0;

	// these are used to find good splice/loop points.
	// If not implementing these, simply return sample
	virtual int					ZeroCrossingBefore( int sample ) = 0;
	virtual int					ZeroCrossingAfter( int sample ) = 0;
	
	// mixer's references
	virtual void				ReferenceAdd( CAudioMixer *pMixer ) = 0;
	virtual void				ReferenceRemove( CAudioMixer *pMixer ) = 0;

	// check reference count, return true if nothing is referencing this
	virtual bool				CanDelete( void ) = 0;
};


extern CAudioSource *AudioSource_Create( const char *pName );

#endif // SND_AUDIO_SOURCE_H
