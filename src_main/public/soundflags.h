//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef SOUNDFLAGS_H
#define SOUNDFLAGS_H

#if defined( _WIN32 )
#pragma once
#endif


//-----------------------------------------------------------------------------
// channels
//-----------------------------------------------------------------------------
enum
{
	CHAN_REPLACE	= -1,
	CHAN_AUTO		= 0,
	CHAN_WEAPON		= 1,
	CHAN_VOICE		= 2,
	CHAN_ITEM		= 3,
	CHAN_BODY		= 4,
	CHAN_STREAM		= 5,		// allocate stream channel from the static or dynamic area
	CHAN_STATIC		= 6,		// allocate channel from the static area 
	CHAN_VOICE_BASE	= 7,		// allocate channel for network voice data
	CHAN_USER_BASE	= (CHAN_VOICE_BASE+128)		// Anything >= this number is allocated to game code.
};

//-----------------------------------------------------------------------------
// common volume values
//-----------------------------------------------------------------------------
#define VOL_NORM		1.0f


//-----------------------------------------------------------------------------
// common attenuation values
//-----------------------------------------------------------------------------
#define ATTN_NONE		0.0f
#define ATTN_NORM		0.8f
#define ATTN_IDLE		2.0f
#define ATTN_STATIC		1.25f 
#define ATTN_RICOCHET	1.5f

// HL2 world is 8x bigger now! We want to hear gunfire from farther.
// Don't change this without consulting Kelly or Wedge (sjb).
#define ATTN_GUNFIRE	0.27f

enum soundlevel_t
{
	SNDLVL_NONE			= 0,

	SNDLVL_25dB			= 25,
	SNDLVL_30dB			= 30,
	SNDLVL_35dB			= 35,
	SNDLVL_40dB			= 40,
	SNDLVL_45dB			= 45,

	SNDLVL_50dB			= 50,	// 3.9
	SNDLVL_55dB			= 55,	// 3.0

	SNDLVL_IDLE			= 60,	// 2.0
	SNDLVL_60dB			= 60,	// 2.0

	SNDLVL_65dB			= 65,	// 1.5
	SNDLVL_STATIC		= 66,	// 1.25

	SNDLVL_70dB			= 70,	// 1.0

	SNDLVL_NORM			= 75,
	SNDLVL_75dB			= 75,	// 0.8

	SNDLVL_80dB			= 80,	// 0.7
	SNDLVL_TALKING		= 80,	// 0.7
	SNDLVL_85dB			= 85,	// 0.6
	SNDLVL_90dB			= 90,	// 0.5
	SNDLVL_95dB			= 95,
	SNDLVL_100dB		= 100,	// 0.4
	SNDLVL_105dB		= 105,
	SNDLVL_110dB		= 110,
	SNDLVL_120dB		= 120,
	SNDLVL_130dB		= 130,

	SNDLVL_GUNFIRE		= 140,	// 0.27
	SNDLVL_140dB		= 140,	// 0.2

	SNDLVL_150dB		= 150,	// 0.2
};

#define ATTN_TO_SNDLVL( a ) (soundlevel_t)(int)((a) ? (50 + 20 / ((float)a)) : 0 )
#define SNDLVL_TO_ATTN( a ) ((a > 50) ? (20.0f / (float)(a - 50)) : 4.0 )

/*
#define SNDLVL_20dB		0.0	// rustling leaves
#define SNDLVL_25dB		0.0	// whispering
#define SNDLVL_30dB		0.0	// library
#define SNDLVL_45dB		0.0	// refrigerator
#define SNDLVL_50dB		0.0	// average home
#define SNDLVL_60dB		0.0	// normal conversation, clothes dryer
#define SNDLVL_65dB		0.0	// washing machine, dishwasher
#define SNDLVL_70dB		0.0	// car, vacuum cleaner, mixer, electric sewing machine
#define SNDLVL_75dB		0.0	// busy traffic
#define SNDLVL_80dB		0.0	// mini-bike, alarm clock, noisy restaurant, office tabulator, outboard motor, passing snowmobile
#define SNDLVL_85dB		0.0	// average factory, electric shaver
#define SNDLVL_90dB		0.0	// screaming child, passing motorcycle, convertible ride on frw
#define SNDLVL_100dB	0.0	// subway train, diesel truck, woodworking shop, pneumatic drill, boiler shop, jackhammer
#define SNDLVL_105dB	0.0	// helicopter, power mower
#define SNDLVL_110dB	0.0	// snowmobile drvrs seat, inboard motorboat, sandblasting
#define SNDLVL_120dB	0.0	// auto horn, propeller aircraft
#define SNDLVL_130dB	0.0	// air raid siren
#define SNDLVL_140dB	0.0	// THRESHOLD OF PAIN, gunshot, jet engine
#define SNDLVL_180dB	0.0	// rocket launching
*/


// This is a limit due to network encoding.
// It encodes attenuation * 64 in 8 bits, so the maximum is (255 / 64)
#define MAX_ATTENUATION		3.98f


//-----------------------------------------------------------------------------
// Flags to be or-ed together for the iFlags field
//-----------------------------------------------------------------------------
enum SoundFlags_t
{
	SND_CHANGE_VOL		= (1<<0),		// change sound vol
	SND_CHANGE_PITCH	= (1<<1),		// change sound pitch
	SND_STOP			= (1<<2),		// stop the sound
	SND_SPAWNING		= (1<<3),		// we're spawning, used in some cases for ambients
										// not sent over net, only a param between dll and server.
	SND_DELAY			= (1<<4),		// sound has an initial delay
	SND_STOP_LOOPING	= (1<<5),		// stop all looping sounds on the entity.
	SND_SPEAKER			= (1<<6),		// being played again by a microphone through a speaker
};
#define SND_FLAG_SHIFT_BASE 6

#define MAX_SOUND_DELAY_MSEC_ENCODE_BITS	(13)
// Subtract one to leave room for the sign bit
#define MAX_SOUND_DELAY_MSEC				(1<<(MAX_SOUND_DELAY_MSEC_ENCODE_BITS-1))    // 4096 msec or about 4 seconds

//-----------------------------------------------------------------------------
// common pitch values
//-----------------------------------------------------------------------------
#define	PITCH_NORM		100			// non-pitch shifted
#define PITCH_LOW		95			// other values are possible - 0-255, where 255 is very high
#define PITCH_HIGH		120


#endif // SOUNDFLAGS_H
