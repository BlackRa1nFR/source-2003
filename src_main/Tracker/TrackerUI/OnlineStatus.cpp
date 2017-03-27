//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "OnlineStatus.h"

#include <string.h>

// online status list
static const char *g_OnlineStatusStrings[] =
{
	"online",
	"busy",
	"away",
	"in-game",
	"snooze"
};

static int g_OnlineStatusStringsLen = sizeof(g_OnlineStatusStrings) / sizeof(g_OnlineStatusStrings[0]);

// offline status strings (negative status IDs)
static const char *g_OfflineStatusStrings[] =
{
	"offline",
	"connecting",
	"recently online",
	"awaiting authorization",
	"requesting authorization"
};
static int g_OfflineStatusStringsLen = sizeof(g_OfflineStatusStrings) / sizeof(g_OfflineStatusStrings[0]);

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : statusID - 
// Output : const char
//-----------------------------------------------------------------------------
const char *COnlineStatus::IDToString(int statusID)
{
	if (statusID > g_OnlineStatusStringsLen)
		return g_OfflineStatusStrings[0];

	if (statusID > 0)
	{
		return g_OnlineStatusStrings[statusID-1];
	}

	if (-statusID >= g_OfflineStatusStringsLen)
		return g_OfflineStatusStrings[0];

	return g_OfflineStatusStrings[-statusID];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *statusString - 
// Output : int
//-----------------------------------------------------------------------------
int COnlineStatus::StringToID(const char *statusString)
{
	// search the lists for the string
	for (int i = 0; i < (sizeof(g_OnlineStatusStrings)/sizeof(g_OnlineStatusStrings[0])); i++)
	{
		if (!stricmp(statusString, g_OnlineStatusStrings[i]))
		{
			return i + 1;
		}
	}
	
	for (i = 0; i < (sizeof(g_OfflineStatusStrings)/sizeof(g_OfflineStatusStrings[0])); i++)
	{
		if (!stricmp(statusString, g_OfflineStatusStrings[i]))
		{
			return -i;
		}
	}
	
	return 0;
}


