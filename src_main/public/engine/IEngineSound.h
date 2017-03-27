//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Client-server neutral sound interface
//
// $NoKeywords: $
//=============================================================================

#ifndef IENGINESOUND_H
#define IENGINESOUND_H

#ifdef _WIN32
#pragma once
#endif


#include "basetypes.h"
#include "interface.h"
#include "soundflags.h"
#include "irecipientfilter.h"


//-----------------------------------------------------------------------------
// forward declaration
//-----------------------------------------------------------------------------
class Vector;

// Handy defines for EmitSound
#define SOUND_FROM_LOCAL_PLAYER		-1
#define SOUND_FROM_WORLD			0
	
//-----------------------------------------------------------------------------
// Client-server neutral effects interface
//-----------------------------------------------------------------------------
#define IENGINESOUND_CLIENT_INTERFACE_VERSION	"IEngineSoundClient002"
#define IENGINESOUND_SERVER_INTERFACE_VERSION	"IEngineSoundServer002"


class IEngineSound
{
public:
	// Precache a particular sample
	virtual bool PrecacheSound( const char *pSample, bool preload = false ) = 0;

	// Pitch of 100 is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
	// down to 1 is a lower pitch.   150 to 70 is the realistic range.
	// EmitSound with pitch != 100 should be used sparingly, as it's not quite as
	// fast (the pitchshift mixer is not native coded).

	// NOTE: setting iEntIndex to -1 will cause the sound to be emitted from the local
	// player (client-side only)
	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, float flAttenuation, int iFlags = 0, int iPitch = PITCH_NORM, 
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, bool bUpdatePositions = true, float soundtime = 0.0f ) = 0;

	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM, 
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, bool bUpdatePositions = true, float soundtime = 0.0f ) = 0;

	virtual void EmitSentenceByIndex( IRecipientFilter& filter, int iEntIndex, int iChannel, int iSentenceIndex, 
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM,
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, bool bUpdatePositions = true, float soundtime = 0.0f ) = 0;

	virtual void StopSound( int iEntIndex, int iChannel, const char *pSample ) = 0;

	// Set the room type for a player
	virtual void SetRoomType( IRecipientFilter& filter, int roomType ) = 0;
	
	// emit an "ambient" sound that isn't spatialized
	// only available on the client, assert on server
	virtual void EmitAmbientSound( const char *pSample, float flVolume, int iPitch = PITCH_NORM, int flags = 0, float soundtime = 0.0f ) = 0;

//	virtual void StopAllSounds() = 0;
//	virtual EntChannel_t	CreateEntChannel() = 0;
};


#endif // IENGINESOUND_H
