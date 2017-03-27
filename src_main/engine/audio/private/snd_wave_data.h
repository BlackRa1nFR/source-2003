//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SND_WAVE_DATA_H
#define SND_WAVE_DATA_H
#ifdef _WIN32
#pragma once
#endif

#include "snd_audio_source.h"

//-----------------------------------------------------------------------------
// Purpose: Linear iterator over source data.
//			Keeps track of position in source, and maintains necessary buffers
//-----------------------------------------------------------------------------
class IWaveData
{
public:
	virtual						~IWaveData( void ) {}
	virtual CAudioSource		&Source( void ) = 0;
	virtual int					ReadSourceData( void **pData, int sampleIndex, int sampleCount, char copyBuf[AUDIOSOURCE_COPYBUF_SIZE] ) = 0;
};

class IWaveStreamSource
{
public:
	virtual int UpdateLoopingSamplePosition( int samplePosition ) = 0;
	virtual void UpdateSamples( char *pData, int sampleCount ) = 0;
};

class IFileReadBinary;

extern IWaveData *CreateWaveDataStream( CAudioSource &source, IWaveStreamSource *pStreamSource, IFileReadBinary &io, const char *pFileName, int dataOffset, int dataSize );
extern IWaveData *CreateWaveDataMemory( CAudioSource &source );

#endif // SND_WAVE_DATA_H
