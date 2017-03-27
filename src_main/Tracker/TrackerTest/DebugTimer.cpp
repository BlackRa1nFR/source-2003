//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "../common/DebugTimer.h"
#include "../common/winlite.h"

static LARGE_INTEGER startTime;
static double ticksPerSecond = -1;

//-----------------------------------------------------------------------------
// Purpose: resets the timer
//-----------------------------------------------------------------------------
void Timer_Start()
{
	if (ticksPerSecond < 0)
	{
		LARGE_INTEGER ticks;
		::QueryPerformanceFrequency(&ticks);

		ticksPerSecond = 1.0 / ticks.QuadPart;
	}

	::QueryPerformanceCounter(&startTime);
}

//-----------------------------------------------------------------------------
// Purpose: returns the time since Timer_Start() was called, in seconds
// Output : double
//-----------------------------------------------------------------------------
double Timer_End()
{
	LARGE_INTEGER endTime;

	::QueryPerformanceCounter(&endTime);

	double time = (endTime.QuadPart - startTime.QuadPart) * ticksPerSecond;
//	g_pConsole->Print(0, "Timer(%d)\n", (int)(time * 1000));

	return (time * 1000);
}
