//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: client sound i/o functions
//
//=============================================================================
#ifndef SOUND_H
#define SOUND_H
#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "vector.h"
#include "mathlib.h"
#include "vstdlib/strtools.h"
#include "soundflags.h"
#include "soundinfo.h"

#define MAX_SFX  2048

class CSfxTable;
enum soundlevel_t;

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_PITCH	100

void S_Init (void);
void S_Shutdown (void);
void S_StopAllSounds(qboolean clear);
void S_Update( Vector const &origin, Vector const &v_forward, Vector const &v_right, Vector const &v_up );
void S_ExtraUpdate (void);
void S_ClearBuffer (void);
void S_BlockSound (void);
void S_UnblockSound (void);
float S_GetMasterVolume( void );
void S_SoundFade( float percent, float holdtime, float intime, float outtime );

void S_StartDynamicSound (
	int entnum, 
	int entchannel, 
	CSfxTable *pSfx, 
	const Vector &origin, 
	const Vector &direction, 
	bool bUpdatePositions,
	float fvol,  
	soundlevel_t soundlevel, 
	int flags, 
	int pitch, 
	bool fromserver = false,
	float delay = 0.0f);
	
void S_StartStaticSound (
	int entnum, 
	int entchannel, 
	CSfxTable *pSfx, 
	const Vector &origin, 
	const Vector &direction, 
	bool bUpdatePositions,
	float vol, 
	soundlevel_t soundlevel, 
	int flags, 
	int pitch, 
	bool fromserver = false,
	float delay = 0.0f);

inline void S_StartDynamicSound (int entnum, int entchannel, CSfxTable *pSfx, 
	const Vector &origin, float fvol, soundlevel_t soundlevel, int flags, int pitch, bool fromserver = false, float delay = 0.0f)
{ 
	S_StartDynamicSound(entnum, entchannel, pSfx, origin, vec3_origin, true, fvol, soundlevel, flags, pitch, fromserver, delay);
}
inline void S_StartStaticSound (int entnum, int entchannel, CSfxTable *pSfx, 
	const Vector &origin, float vol, soundlevel_t soundlevel, int flags, int pitch, bool fromserver = false, float delay = 0.0f)
{ 
	S_StartStaticSound(entnum, entchannel, pSfx, origin, vec3_origin, true, vol, soundlevel, flags, pitch, fromserver, delay);
}

void S_StopSound (int entnum, int entchannel);

CSfxTable *S_DummySfx( const char *name );
CSfxTable *S_PrecacheSound (const char *sample);

vec_t S_GetNominalClipDist();

extern bool TestSoundChar(const char *pch, char c);
extern char *PSkipSoundChars(const char *pch);

#define CHAR_STREAM			'*'		// as one of 1st 2 chars in name, indicates streaming wav data
#define CHAR_USERVOX		'?'		// as one of 1st 2 chars in name, indicates user realtime voice data
#define CHAR_SENTENCE		'!'		// as one of 1st 2 chars in name, indicates sentence wav
#define CHAR_DRYMIX			'#'		// as one of 1st 2 chars in name, indicates wav bypasses dsp fx
#define CHAR_DOPPLER		'>'		// as one of 1st 2 chars in name, indicates doppler encoded stereo wav
#define CHAR_DIRECTIONAL	'<'		// as one of 1st 2 chars in name, indicates mono or stereo wav has direction cone
#define CHAR_DISTVARIANT	'^'		// as one of 1st 2 chars in name, indicates distance variant encoded stereo wav
#define CHAR_OMNI			'@'		// as one of 1st 2 chars in name, indicates non-directional wav (default mono or stereo)

// for recording movies
void SND_MovieStart( void );
void SND_MovieEnd( void );

//-------------------------------------

int S_GetCurrentStaticSounds( SoundInfo_t *pResult, int nSizeResult, int entchannel );

//-----------------------------------------------------------------------------

#endif // SOUND_H
