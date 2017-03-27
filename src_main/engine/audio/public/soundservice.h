//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose: Applicaton-level hooks for clients of the audio subsystem
//
// $NoKeywords: $
//=============================================================================

#ifndef SOUNDSERVICE_H
#define SOUNDSERVICE_H

#if defined( _WIN32 )
#pragma once
#endif

class Vector;
class QAngle;
class CAudioSource;
typedef int SoundSource;
struct SpatializationInfo_t;

//-----------------------------------------------------------------------------
// Purpose: Services required by the audio system to function, this facade
//			defines the bridge between the audio code and higher level
//			systems.
//
//			Note that some of these currently suggest that certain
//			functionality would like to exist at a deeper layer so
//			systems like audio can take advantage of them
//			diectly (toml 05-02-02)
//-----------------------------------------------------------------------------

class ISoundServices
{
public:
	//---------------------------------
	// Purpose: Returns the position (1 to argc-1) in the program's argument 
	// list where the given parameter apears, or 0 if not present
	//---------------------------------
	virtual int CheckParm( const char *pszParm ) = 0;

	//---------------------------------
	// Console printing
	// Note (toml 05-02-02): this is only here because at this
	// time the dbg library has no equivalent notion
	//---------------------------------
	virtual void ConSafePrint( const char *pszFormat ) = 0;

	//---------------------------------
	// Allocate a block of memory that will be automatically
	// cleaned up on level change
	//---------------------------------
	virtual void *LevelAlloc( int nBytes, const char *pszTag ) = 0;

	//---------------------------------
	// Query if the DirectSound desired/supported
	//---------------------------------
	virtual bool IsDirectSoundSupported() = 0;

	//---------------------------------
	// Get the value of a config setting.
	//---------------------------------
	virtual bool QuerySetting( const char *pszCvar, bool bDefault ) = 0;

	//---------------------------------
	// Notification that someone called S_ExtraUpdate()
	//---------------------------------
	virtual void OnExtraUpdate() = 0;

	//---------------------------------
	// Return false if the entity doesn't exist or is out of the PVS, in which case the sound shouldn't be heard.
	//---------------------------------
	virtual bool GetSoundSpatialization( int entIndex, SpatializationInfo_t& info ) = 0;

	//---------------------------------
	//---------------------------------
	virtual long RandomLong( long lLow, long lHigh ) = 0;

	//---------------------------------
	// This is the client's clock, which follows the servers and thus isn't 100% smooth all the time (it is in single player)
	//---------------------------------
	virtual double GetClientTime() = 0;

	//---------------------------------
	// This is the engine's filtered timer, it's pretty smooth all the time
	//---------------------------------
	virtual double GetHostTime() = 0;

	//---------------------------------
	//---------------------------------
	virtual int GetViewEntity() = 0;

	//---------------------------------
	//---------------------------------
	virtual double GetHostFrametime() = 0;

	//---------------------------------
	//---------------------------------
	virtual int GetServerCount() = 0;

	//---------------------------------
	//---------------------------------
	virtual bool IsPlayer( SoundSource source ) = 0;

	//---------------------------------
	//---------------------------------
	virtual float GetRealTime() = 0;

	//---------------------------------
	//---------------------------------
	virtual void OnChangeVoiceStatus( int entity, bool status) = 0;
};

//-------------------------------------

extern ISoundServices *g_pSoundServices;

//=============================================================================

#endif // SOUNDSERVICE_H
