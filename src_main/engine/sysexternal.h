//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef SYSEXTERNAL_H
#define SYSEXTERNAL_H
#ifdef _WIN32
#pragma once
#endif

// an error will cause the entire program to exit
void Sys_Error(const char *psz, ...);

// send text to the console
void Sys_Printf (char *fmt, ...);

// Exit the engine
void Sys_Quit (void);

#endif // SYSEXTERNAL_H
