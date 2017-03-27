//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#ifndef CONPRINT_H
#define CONPRINT_H
#ifdef _WIN32
#pragma once
#endif

#include <Color.h>

void Con_Print (const char *msg);
void Con_Printf (const char *fmt, ...);
void Con_DPrintf (const char *fmt, ...);
void Con_SafePrintf (const char *fmt, ...);
void Con_ColorPrintf( Color& clr, const char *fmt, ... );

#endif // CONPRINT_H
