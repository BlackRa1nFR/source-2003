//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef ENGINESOUNDINTERNAL_H
#define ENGINESOUNDINTERNAL_H

#if defined( _WIN32 )
#pragma once
#endif

#include "engine/IEngineSound.h"

//-----------------------------------------------------------------------------
// Additional sound flags which shouldn't directly be used
//-----------------------------------------------------------------------------
enum
{
	SND_SENTENCE		= (1<<(SND_FLAG_SHIFT_BASE+1)),	// set if sound num is actually a sentence num 
	SND_VOLUME			= (1<<(SND_FLAG_SHIFT_BASE+2)),	// we're sending down the volume
	SND_SOUNDLEVEL		= (1<<(SND_FLAG_SHIFT_BASE+3)),	// we're sending down the dB level
	SND_PITCH			= (1<<(SND_FLAG_SHIFT_BASE+4)),	// we're sending down the pitch
};

#define SND_FLAG_BITS_ENCODE	(SND_FLAG_SHIFT_BASE+5)

//-----------------------------------------------------------------------------
// Method to get at the singleton implementations of the engine sound interfaces
//-----------------------------------------------------------------------------
IEngineSound* EngineSoundClient();
IEngineSound* EngineSoundServer();


//-----------------------------------------------------------------------------
// Purpose: Forces ambient sounds to be restarted so that the recording system
//			can seed the recording stream with the initial state that these 
//			sounds were playing prior to recording. A pre-e3 (2002) hack that 
//			would probably be better implemented, once save and load are reliable,
//			as a forced save-start record-load sequence, which would force
//			all existing state to be rebroadcast to the client, thus recorded.
//			(toml 05-09-02)
//-----------------------------------------------------------------------------
void Host_RestartAmbientSounds();


#endif // SOUNDENGINEINTERNAL_H
