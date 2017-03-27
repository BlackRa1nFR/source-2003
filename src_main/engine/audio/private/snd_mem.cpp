// snd_mem.c: sound caching

#include "tier0/dbg.h"

#include "voice.h"
#include "voice_sound_engine_interface.h"

#include "sound.h"
#include "sound_private.h"
#include "snd_wave_source.h"
#include "snd_channels.h"
#include "snd_device.h"
#include "snd_sfx.h"
#include "snd_convars.h"

#include "soundservice.h"

#if DEAD
int			cache_full_cycle;
#endif

//=============================================================================

inline bool IsSoundChar(char c)
{
	bool b;

	b = (c == CHAR_STREAM || c == CHAR_USERVOX || c == CHAR_SENTENCE || c == CHAR_DRYMIX);
	b = b || (c == CHAR_DOPPLER || c == CHAR_DIRECTIONAL || c == CHAR_DISTVARIANT );

	return b;
}

// return pointer to first valid character in file name
// by skipping over CHAR_STREAM...CHAR_DRYMIX

char *PSkipSoundChars(const char *pch)
{
	int i;
	char *pcht = (char *)pch;

	// check first 2 characters

	for (i = 0; i < 2; i++)
	{
		if (!IsSoundChar(*pcht))
			break;
		pcht++;
	}

	return pcht;
}

// return true if char 'c' is one of 1st 2 characters in pch

bool TestSoundChar(const char *pch, char c)
{
	int i;
	char *pcht = (char *)pch;

	// check first 2 characters

	for (i = 0; i < 2; i++)
	{
		if (*pcht == c)
			return true;
		pcht++;
	}

	return false;
}


