#if DEAD
#include <stdio.h>
#include "snd_audio_source.h"



extern CAudioSource *Audio_CreateMemoryWave( const char *pName );
extern CAudioSource *Audio_CreateStreamedWave( const char *pName );

//-----------------------------------------------------------------------------
// Purpose: Simple wrapper to crack naming convention and create the proper wave source
// Input  : *pName - WAVE filename
// Output : CAudioSource
//-----------------------------------------------------------------------------
CAudioSource *AudioSource_Create( const char *pName )
{
	if ( !pName )
		return NULL;

//	if ( TestSoundChars(pName, CHAR_SENTENCE) )		// sentence
		;

	// Names that begin with CHAR_STREAM are streaming.
	// Skip over the CHAR_STREAM and create a streamed source
	if ( TestSoundChar( pName, CHAR_STREAM ) )		// stream
		return Audio_CreateStreamedWave( PSkipSoundChars(pName) );

	// These are loaded into memory directly
	return Audio_CreateMemoryWave( PSkipSoundCars(pName) );
}

#endif
