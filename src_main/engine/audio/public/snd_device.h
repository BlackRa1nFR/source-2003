//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: This abstracts the various hardware dependent implementations of sound
//			At the time of this writing there are Windows WAVEOUT, Direct Sound,
//			and Null implementations.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "snd_fixedint.h"
#include "snd_mix_buf.h"

#ifndef SND_DEVICE_H
#define SND_DEVICE_H
#pragma once

// sound engine rate defines
#define SOUND_DMA_SPEED		44100		// hardware playback rate
#define SOUND_11k			11025		// 11khz sample rate
#define SOUND_22k			22050		// 22khz sample rate
#define SOUND_44k			44100		// 44khz sample rate
#define SOUND_ALL_RATES		1			// mix all sample rates

#define SOUND_MIX_WET		0			// mix only samples that don't have channel set to 'dry' (default)
#define SOUND_MIX_DRY		1			// mix only samples with channel set to 'dry' (ie: music)

#define	SOUND_BUSS_ROOM			(1<<0)		// mix samples using channel dspmix value (based on distance from player)
#define SOUND_BUSS_FACING		(1<<1)		// mix samples using channel dspface value (source facing)
#define	SOUND_BUSS_FACINGAWAY	(1<<2)		// mix samples using 1-dspface

class Vector;
struct channel_t;

// UNDONE: Create a simulated audio device to replace the old -simsound functionality?

// General interface to an audio device
class IAudioDevice
{
public:
	// Detect the sound hardware and create a compatible device
	// NOTE: This should NEVER fail.  There is a function called Audio_GetNullDevice
	// which will create a "null" device that makes no sound.  If we can't create a real 
	// sound device, this will return a device of that type.  All of the interface
	// functions can be called on the null device, but it will not, of course, make sound.
	static IAudioDevice *AutoDetectInit( bool waveOnly );

	// This is needed by some of the routines to avoid doing work when you've got a null device
	virtual bool		IsActive( void ) = 0;
	// This initializes the sound hardware.  true on success, false on failure
	virtual bool		Init( void ) = 0;
	// This releases all sound hardware
	virtual void		Shutdown( void ) = 0;
	// stop outputting sound, but be ready to resume on UnPause
	virtual void		Pause( void ) = 0;
	// return to normal operation after a Pause()
	virtual void		UnPause( void ) = 0;
	// The volume of the "dry" mix (no effects).
	// This should return 0 on all implementations that don't need a separate dry mix
	virtual float		MixDryVolume( void ) = 0;
	// Should we mix sounds to a 3D (quadraphonic) sound buffer (front/rear both stereo)
	virtual bool		Should3DMix( void ) = 0;

	// This is called when the application stops all sounds 
	// NOTE: Stopping each channel and clearing the sound buffer are done separately
	virtual void		StopAllSounds( void ) = 0;
	
	// Called before painting channels, must calculated the endtime and return it (once per frame)
	virtual int			PaintBegin( int soundtime, int paintedtime ) = 0;
	// Called when all channels are painted (once per frame)
	virtual void		PaintEnd( void ) = 0;

	// Called to set the volumes on a channel with the given gain & dot parameters
	virtual void		SpatializeChannel( int volume[4], int master_vol, const Vector& sourceDir, float gain, float dotRight, float dotFront ) = 0;
	
	// The device should apply DSP up to endtime in the current paint buffer
	// this is called during painting
	virtual void		ApplyDSPEffects( int idsp, portable_samplepair_t *pbuffront, portable_samplepair_t *pbufrear, int samplecount ) = 0;
	
	// replaces SNDDMA_GetDMAPos, gets the output sample position for tracking
	virtual int			GetOutputPosition( void ) = 0;

	// Fill the output buffer with silence (e.g. during pause)
	virtual void		ClearBuffer( void ) = 0;

	// Called each frame with the listener's coordinate system
	virtual void		UpdateListener( const Vector& position, const Vector& forward, const Vector& right, const Vector& up ) = 0;
	
	// Called each time a new paint buffer is mixed (may be multiple times per frame)
	virtual void		MixBegin( int sampleCount ) = 0;
	virtual void		MixUpsample( int sampleCount, int filtertype ) = 0;

	// sink sound data
	virtual void		Mix8Mono( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress ) = 0;
	virtual void		Mix8Stereo( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress ) = 0;
	virtual void		Mix16Mono( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress ) = 0;
	virtual void		Mix16Stereo( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress ) = 0;

	// Reset a channel
	virtual void		ChannelReset( int entnum, int channelIndex, float distanceMod ) = 0;
	virtual void		TransferSamples( int end ) = 0;

	// device parameters
	virtual const char *DeviceName( void ) = 0;
	virtual int			DeviceChannels( void ) = 0;		// 1 = mono, 2 = stereo
	virtual int			DeviceSampleBits( void ) = 0;	// bits per sample (8 or 16)
	virtual int			DeviceSampleBytes( void ) = 0;	// above / 8
	virtual int			DeviceDmaSpeed( void ) = 0;		// Actual DMA speed
	virtual int			DeviceSampleCount( void ) = 0;	// Total samples in buffer

	// Lock the buffer memory for access (may be shared DMA memory)
	virtual void		*DeviceLockBuffer( void ) = 0;
	// Done accessing the buffer
	virtual void		DeviceUnlockBuffer( void *pbuffer ) = 0;
};

extern IAudioDevice *Audio_GetNullDevice( void );

#endif // SND_DEVICE_H
