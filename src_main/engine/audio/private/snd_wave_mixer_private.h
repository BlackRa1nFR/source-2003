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

#ifndef SND_WAVE_MIXER_PRIVATE_H
#define SND_WAVE_MIXER_PRIVATE_H
#pragma once

#include "snd_audio_source.h"
#include "snd_wave_mixer.h"
#include "sound_private.h"
#include "snd_wave_source.h"

class IWaveData;

class CAudioMixerWave : public CAudioMixer
{
public:
							CAudioMixerWave( IWaveData *data );
	virtual					~CAudioMixerWave( void );

	int						MixDataToDevice( IAudioDevice *pDevice, channel_t *pChannel, int sampleCount, int outputRate, int outputOffset );
	int						SkipSamples( int startSample, int sampleCount, int outputRate, int outputOffset );
	bool					ShouldContinueMixing( void );

	virtual void			Mix( IAudioDevice *pDevice, channel_t *pChannel, void *pData, int outputOffset, int inputOffset, fixedint fracRate, int outCount, int timecompress ) = 0;
	virtual int				GetOutputData( void **pData, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] );

	virtual CAudioSource*	GetSource( void );
	virtual int				GetSamplePosition( void );
	virtual float			ModifyPitch( float pitch );
	virtual float			GetVolumeScale( void );
	
	// Move the current position to newPosition
	virtual void			SetSampleStart( int newPosition );
	
	// End playback at newEndPosition
	virtual void			SetSampleEnd( int newEndPosition );

	virtual void			SetStartupDelaySamples( int delaySamples );
protected:
	double				m_sample;

	IWaveData			*m_pData;
	double 				m_forcedEndSample;
	bool				m_finished;
	int					m_delaySamples;
};


#endif // SND_WAVE_MIXER_PRIVATE_H
