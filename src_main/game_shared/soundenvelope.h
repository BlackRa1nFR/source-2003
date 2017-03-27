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
// $NoKeywords: $
//=============================================================================

#ifndef SOUNDENVELOPE_H
#define SOUNDENVELOPE_H

#ifdef _WIN32
#pragma once
#endif

#include "engine/IEngineSound.h"

class CSoundPatch;

enum soundcommands_t
{
	SOUNDCTRL_CHANGE_VOLUME,
	SOUNDCTRL_CHANGE_PITCH,
	SOUNDCTRL_STOP,
};

//Envelope point
struct envelopePoint_t
{
	float	amplitudeMin, amplitudeMax;
	float	durationMin, durationMax;
};

//Envelope description
struct envelopeDescription_t
{
	envelopePoint_t	*pPoints;
	int				nNumPoints;
};

class IRecipientFilter;

class CSoundEnvelopeController
{
public:
	virtual void		SystemReset( void ) = 0;
	virtual void		SystemUpdate( void ) = 0;
	virtual void		Play( CSoundPatch *pSound, float volume, float pitch ) = 0;
	virtual void		CommandAdd( CSoundPatch *pSound, float executeDeltaTime, soundcommands_t command, float commandTime, float value ) = 0;
	virtual void		CommandClear( CSoundPatch *pSound ) = 0;
	virtual void		Shutdown( CSoundPatch *pSound ) = 0;

	virtual CSoundPatch	*SoundCreate( IRecipientFilter& filter, int nEntIndex, const char *pSoundName ) = 0;
	virtual CSoundPatch	*SoundCreate( IRecipientFilter& filter, int nEntIndex, int channel, const char *pSoundName, 
							float attenuation ) = 0;
	virtual CSoundPatch	*SoundCreate( IRecipientFilter& filter, int nEntIndex, int channel, const char *pSoundName, 
							soundlevel_t soundlevel ) = 0;
	virtual void		SoundDestroy( CSoundPatch	* ) = 0;
	virtual void		SoundChangePitch( CSoundPatch *pSound, float pitchTarget, float deltaTime ) = 0;
	virtual void		SoundChangeVolume( CSoundPatch *pSound, float volumeTarget, float deltaTime ) = 0;
	virtual void		SoundFadeOut( CSoundPatch *pSound, float deltaTime ) = 0;
	virtual float		SoundGetPitch( CSoundPatch *pSound ) = 0;
	virtual float		SoundGetVolume( CSoundPatch *pSound ) = 0;
	
	virtual float		SoundPlayEnvelope( CSoundPatch *pSound, soundcommands_t soundCommand, envelopePoint_t *points, int numPoints ) = 0;
	virtual float		SoundPlayEnvelope( CSoundPatch *pSound, soundcommands_t soundCommand, envelopeDescription_t *envelope ) = 0;

	virtual void		CheckLoopingSoundsForPlayer( CBasePlayer *pPlayer ) = 0;

	static	CSoundEnvelopeController &GetController( void );
};


//-----------------------------------------------------------------------------
// Save/restore
//-----------------------------------------------------------------------------
class ISaveRestoreOps;

ISaveRestoreOps *GetSoundSaveRestoreOps( );

#define DEFINE_SOUNDPATCH(type,name) \
	{ FIELD_CUSTOM, #name, { offsetof(type,name), 0 }, 1, FTYPEDESC_SAVE, NULL, GetSoundSaveRestoreOps( ), NULL }


#endif // SOUNDENVELOPE_H
