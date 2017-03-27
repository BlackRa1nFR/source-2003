//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef SOUNDINFO_H
#define SOUNDINFO_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
struct SoundInfo_t
{
	int			 entity;
	int			 channel;
	char		 name[256];
	Vector		 origin;
	Vector		 direction;
	float		 volume;
	soundlevel_t soundlevel;
	bool		looping;
	int			pitch;
	
	//---------------------------------
	
	void Set(int newEntity, int newChannel, const char *pszNewName, const Vector &newOrigin, const Vector& newDirection, 
			float newVolume, soundlevel_t newSoundLevel, bool newLooping, int newPitch )
	{
		entity = newEntity;
		channel = newChannel;
		Q_strncpy( name, pszNewName, sizeof(name) );
		origin = newOrigin;
		direction = newDirection;
		volume = newVolume;
		soundlevel = newSoundLevel;
		looping = newLooping;
		pitch = newPitch;
	}
};

struct SpatializationInfo_t
{
	typedef enum
	{
		SI_INCREATION = 0,
		SI_INSPATIALIZATION
	} SPATIALIZATIONTYPE;

	// Inputs
	SPATIALIZATIONTYPE	type;
	// Info about the sound, channel, origin, direction, etc.
	SoundInfo_t			info;

	// Requested Outputs ( NULL == not requested )
	Vector				*pOrigin;
	QAngle				*pAngles;
	float				*pflRadius;
};

#endif // SOUNDINFO_H
