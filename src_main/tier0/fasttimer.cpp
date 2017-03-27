//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include "tier0/fasttimer.h"



int64 g_ClockSpeed;	// Clocks/sec
unsigned long g_dwClockSpeed;
double g_ClockSpeedMicrosecondsMultiplier;
double g_ClockSpeedMillisecondsMultiplier;
double g_ClockSpeedSecondsMultiplier;


// Constructor init the clock speed.
CClockSpeedInit g_ClockSpeedInit;

