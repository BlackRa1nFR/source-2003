//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "Tracker.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static bool g_bStandaloneMode;

//-----------------------------------------------------------------------------
// Purpose: gets the standalone flag
// Output : returns true if tracker is running as a standalone window, false otherwise
//-----------------------------------------------------------------------------
bool Tracker_StandaloneMode()
{
	return g_bStandaloneMode;
}

//-----------------------------------------------------------------------------
// Purpose: sets the windowed mode
// Input  : mode - 
//-----------------------------------------------------------------------------
void Tracker_SetStandaloneMode(bool mode)
{
	g_bStandaloneMode = mode;
}

#include <VGUI_Controls.h>
#include <VGUI_ISystem.h>

//-----------------------------------------------------------------------------
// Purpose: Log-writing function for debugging purposes
// Input  : *format - 
//			... - 
//-----------------------------------------------------------------------------
void WriteToLog(const char *format, ...)
{
	// always append
	static bool firstThrough = false;

	FILE *f;
	if (firstThrough)
	{
		f = fopen("trackerlog.txt", "wt");
		firstThrough = false;
	}
	else
	{
		f = fopen("trackerlog.txt", "at");
	}
	if (!f)
		return;

	char    buf[2048];
	va_list argList;

	va_start(argList,format);
	vsprintf(buf, format, argList);
	va_end(argList);

	float time = vgui::system()->GetTimeMillis() * 1000.0f;
	fprintf(f, "(%.2f) %s", time, buf);

	fclose(f);
}

