//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TRACKER_H
#define TRACKER_H
#pragma once

#include "interface.h"

// returns true if tracker is running as a standalone window, false otherwise
bool Tracker_StandaloneMode();

// sets the windowed mode
void Tracker_SetStandaloneMode(bool mode);

// sends a command to the engine, only works if we're not in standalone mode
class IRunGameEngine;
IRunGameEngine *Tracker_GetRunGameEngineInterface();

// returns handle
CreateInterfaceFn Tracker_GetGameFactory();

// writes out to log file
void WriteToLog(const char *format, ...);



#endif // TRACKER_H
