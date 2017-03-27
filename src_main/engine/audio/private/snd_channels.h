//========= Copyright (c) 1996-2003, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
//=============================================================================

#ifndef SND_CHANNELS_H
#define SND_CHANNELS_H

#include "vector.h"

#if defined( _WIN32 )
#pragma once
#endif

class CSfxTable;
class CAudioMixer;
typedef int SoundSource;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

struct channel_t
{
	CSfxTable	*sfx;			// the actual sound
	CAudioMixer	*pMixer;		// The sound's instance data for this channel
	portable_samplepair_t *pbuf;// stereo buffer to mix this sound into
	
	// UNDONE: convert volumes to struct and get rid of this crappy volume array hack

	// NOTE These are used as an 8 element array and must appear sequentially in this structure!
	int			leftvol;		// 0-255 front left volume
	int			rightvol;		// 0-255 front right volume
	int			rleftvol;		// 0-255 rear left volume
	int			rrightvol;		// 0-255 rear right volume

	int			dleftvol;		// 0-255 front left volume	- doppler outgoing wav
	int			drightvol;		// 0-255 front right volume - doppler outgoing wav
	int			drleftvol;		// 0-255 rear left volume	- doppler outgoing wav
	int			drrightvol;		// 0-255 rear right volume	- doppler outgoing wav

	SoundSource	soundsource;	// see iclientsound.h for description.
	int			entchannel;		// sound channel (CHAN_STREAM, CHAN_VOICE, etc.)
	Vector		origin;			// origin of sound effect
	Vector		direction;		// direction of the sound
	bool		bUpdatePositions; // if true, assume sound source can move and update according to entity
	vec_t		dist_mult;		// distance multiplier (attenuation/clipK)
	int			master_vol;		// 0-255 master volume
	int			isSentence;		// true if playing linked sentence
	int			basePitch;		// base pitch percent (100% is normal pitch playback)
	float		pitch;			// real-time pitch after any modulation or shift by dynamic data

	float		dspmix;			// 0 - 1.0 proportion of dsp to mix with original sound, based on distance
	float		dspface;		// -1.0 - 1.0 (1.0 = facing listener)
	float		distmix;		// 0 - 1.0 proportion based on distance from listner (1.0 - 100% wav right - far)

	bool		bdry;			// if true, bypass all dsp processing for this sound (ie: music)
	bool		bstereowav;		// if true, a stereo .wav file is the sample data source
	char		wavtype;		// 0 default, CHAR_DOPPLER, CHAR_DIRECTIONAL, CHAR_DISTVARIANT
	float		radius;			// Radius of this sound effect (spatialization is different within the radius)

	bool		delayed_start;  // If true, sound had a delay and so same sound on same channel won't channel steal from it
	bool		fromserver;		// for snd_show, networked sounds get colored differently than local sounds

	bool		bfirstpass;		// true if this is first time sound is spatialized
	float		ob_gain;		// gain drop if sound source obscured from listener
	float		ob_gain_target;	// target gain while crossfading between ob_gain & ob_gain_target
	float		ob_gain_inc;	// crossfade increment
	bool		bTraced;		// true if channel was already checked this frame for obscuring

};

#define IFRONT_LEFT		0			// NOTE: must correspond to order of leftvol...drrightvol above!
#define IFRONT_RIGHT	1
#define	IREAR_LEFT		2
#define IREAR_RIGHT		3
#define IFRONT_LEFTD	4			// start of doppler right array			
#define IFRONT_RIGHTD	5
#define	IREAR_LEFTD		6
#define IREAR_RIGHTD	7

#define CCHANVOLUMES	8

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define	MAX_CHANNELS			128
#define	MAX_DYNAMIC_CHANNELS	24

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

extern	channel_t   channels[MAX_CHANNELS];
// 0 to MAX_DYNAMIC_CHANNELS-1	= normal entity sounds
// MAX_DYNAMIC_CHANNELS to total_channels = static sounds

extern	int			total_channels;

//=============================================================================

#endif // SND_CHANNELS_H
